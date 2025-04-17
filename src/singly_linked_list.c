/** Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "impl/impl_singly_linked_list.h"
#include "singly_linked_list.h"
#include "types.h"

/** @brief When sorting a singly linked list is at a disadvantage for iterative
O(1) space merge sort: it doesn't have a prev pointer. This will help list
elements remember their previous element for splicing and merging. */
struct list_link
{
    /** The previous element of cur. Must manually update and manage. */
    struct ccc_sll_elem_ *prev;
    /** The current element. Must manually manage and update. */
    struct ccc_sll_elem_ *cur;
};

/*===========================    Prototypes     =============================*/

static void *struct_base(struct ccc_sll_ const *, struct ccc_sll_elem_ const *);
static struct ccc_sll_elem_ *before(struct ccc_sll_ const *,
                                    struct ccc_sll_elem_ const *to_find);
static size_t len(struct ccc_sll_ const *, struct ccc_sll_elem_ const *begin,
                  struct ccc_sll_elem_ const *end);

static void push_front(struct ccc_sll_ *sll, struct ccc_sll_elem_ *elem);
static size_t extract_range(struct ccc_sll_ *, struct ccc_sll_elem_ *begin,
                            struct ccc_sll_elem_ *end);
static size_t erase_range(struct ccc_sll_ *, struct ccc_sll_elem_ *begin,
                          struct ccc_sll_elem_ *end);
static struct ccc_sll_elem_ *pop_front(struct ccc_sll_ *);
static struct ccc_sll_elem_ *elem_in(struct ccc_sll_ const *,
                                     void const *any_struct);
static struct list_link merge(struct ccc_sll_ *, struct list_link,
                              struct list_link, struct list_link);
static struct list_link first_unsorted(struct ccc_sll_ const *,
                                       struct list_link);
static ccc_threeway_cmp cmp(struct ccc_sll_ const *sll,
                            struct ccc_sll_elem_ const *lhs,
                            struct ccc_sll_elem_ const *rhs);

/*===========================     Interface     =============================*/

void *
ccc_sll_push_front(ccc_singly_linked_list *const sll, ccc_sll_elem *elem)
{
    if (!sll || !elem)
    {
        return NULL;
    }
    if (sll->alloc_)
    {
        void *const node = sll->alloc_(NULL, sll->elem_sz_, sll->aux_);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(sll, elem), sll->elem_sz_);
        elem = elem_in(sll, node);
    }
    push_front(sll, elem);
    return struct_base(sll, sll->sentinel_.n_);
}

void *
ccc_sll_front(ccc_singly_linked_list const *const sll)
{
    if (!sll || sll->sentinel_.n_ == &sll->sentinel_)
    {
        return NULL;
    }
    return struct_base(sll, sll->sentinel_.n_);
}

ccc_sll_elem *
ccc_sll_begin_elem(ccc_singly_linked_list const *const sll)
{
    return sll ? sll->sentinel_.n_ : NULL;
}

ccc_sll_elem *
ccc_sll_begin_sentinel(ccc_singly_linked_list const *const sll)
{
    return sll ? (ccc_sll_elem *)&sll->sentinel_ : NULL;
}

ccc_result
ccc_sll_pop_front(ccc_singly_linked_list *const sll)
{
    if (!sll || !sll->sz_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    struct ccc_sll_elem_ *const remove = pop_front(sll);
    if (sll->alloc_)
    {
        (void)sll->alloc_(struct_base(sll, remove), 0, sll->aux_);
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_sll_splice(ccc_singly_linked_list *const pos_sll,
               ccc_sll_elem *const pos_before,
               ccc_singly_linked_list *const to_splice_sll,
               ccc_sll_elem *const to_splice)
{
    if (!pos_sll || !pos_before || !to_splice || !to_splice_sll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (to_splice == pos_before || pos_before->n_ == to_splice)
    {
        return CCC_RESULT_OK;
    }
    before(to_splice_sll, to_splice)->n_ = to_splice->n_;
    to_splice->n_ = pos_before->n_;
    pos_before->n_ = to_splice;
    if (pos_sll != to_splice_sll)
    {
        --to_splice_sll->sz_;
        ++pos_sll->sz_;
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_sll_splice_range(ccc_singly_linked_list *const pos_sll,
                     ccc_sll_elem *const pos,
                     ccc_singly_linked_list *const splice_sll,
                     ccc_sll_elem *const begin, ccc_sll_elem *const end)
{
    if (!pos_sll || !pos || !begin || !end || !splice_sll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (begin == pos || end == pos || pos->n_ == begin)
    {
        return CCC_RESULT_OK;
    }
    if (begin == end)
    {
        (void)ccc_sll_splice(pos_sll, pos, splice_sll, begin);
        return CCC_RESULT_OK;
    }
    struct ccc_sll_elem_ *const found = before(splice_sll, begin);
    found->n_ = end->n_;

    end->n_ = pos->n_;
    pos->n_ = begin;
    if (pos_sll != splice_sll)
    {
        size_t const sz = len(splice_sll, begin, end);
        splice_sll->sz_ -= sz;
        pos_sll->sz_ += sz;
    }
    return CCC_RESULT_OK;
}

void *
ccc_sll_erase(ccc_singly_linked_list *const sll, ccc_sll_elem *const elem)
{
    if (!sll || !elem || !sll->sz_ || elem == &sll->sentinel_)
    {
        return NULL;
    }
    struct ccc_sll_elem_ const *const ret = elem->n_;
    before(sll, elem)->n_ = elem->n_;
    if (elem != &sll->sentinel_)
    {
        elem->n_ = NULL;
    }
    if (sll->alloc_)
    {
        (void)sll->alloc_(struct_base(sll, elem), 0, sll->aux_);
    }
    --sll->sz_;
    return ret == &sll->sentinel_ ? NULL : struct_base(sll, ret);
}

void *
ccc_sll_erase_range(ccc_singly_linked_list *const sll,
                    ccc_sll_elem *const begin, ccc_sll_elem *end)
{
    if (!sll || !begin || !end || !sll->sz_ || begin == &sll->sentinel_
        || end == &sll->sentinel_)
    {
        return NULL;
    }
    struct ccc_sll_elem_ const *const ret = end->n_;
    before(sll, begin)->n_ = end->n_;
    size_t const deleted = erase_range(sll, begin, end);
    assert(deleted <= sll->sz_);
    sll->sz_ -= deleted;
    return ret == &sll->sentinel_ ? NULL : struct_base(sll, ret);
}

void *
ccc_sll_extract(ccc_singly_linked_list *const sll, ccc_sll_elem *const elem)
{
    if (!sll || !elem || !sll->sz_ || elem == &sll->sentinel_)
    {
        return NULL;
    }
    struct ccc_sll_elem_ const *const ret = elem->n_;
    before(sll, elem)->n_ = elem->n_;
    if (elem != &sll->sentinel_)
    {
        elem->n_ = NULL;
    }
    --sll->sz_;
    return ret == &sll->sentinel_ ? NULL : struct_base(sll, ret);
}

void *
ccc_sll_extract_range(ccc_singly_linked_list *const sll,
                      ccc_sll_elem *const begin, ccc_sll_elem *end)
{
    if (!sll || !begin || !end || !sll->sz_ || begin == &sll->sentinel_
        || end == &sll->sentinel_)
    {
        return NULL;
    }
    struct ccc_sll_elem_ const *const ret = end->n_;
    before(sll, begin)->n_ = end->n_;
    size_t const deleted = extract_range(sll, begin, end);
    assert(deleted <= sll->sz_);
    sll->sz_ -= deleted;
    return ret == &sll->sentinel_ ? NULL : struct_base(sll, ret);
}

void *
ccc_sll_begin(ccc_singly_linked_list const *const sll)
{
    if (!sll || sll->sentinel_.n_ == &sll->sentinel_)
    {
        return NULL;
    }
    return struct_base(sll, sll->sentinel_.n_);
}

void *
ccc_sll_end(ccc_singly_linked_list const *const)
{
    return NULL;
}

void *
ccc_sll_next(ccc_singly_linked_list const *const sll,
             ccc_sll_elem const *const elem)
{
    if (!sll || !elem || elem->n_ == &sll->sentinel_)
    {
        return NULL;
    }
    return struct_base(sll, elem->n_);
}

ccc_result
ccc_sll_clear(ccc_singly_linked_list *const sll,
              ccc_any_type_destructor_fn *const fn)
{
    if (!sll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    while (!ccc_sll_is_empty(sll))
    {
        void *const mem = struct_base(sll, pop_front(sll));
        if (fn)
        {
            fn((ccc_any_type){.any_type = mem, .aux = sll->aux_});
        }
        if (sll->alloc_)
        {
            (void)sll->alloc_(mem, 0, sll->aux_);
        }
    }
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_sll_validate(ccc_singly_linked_list const *const sll)
{
    if (!sll)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t size = 0;
    for (struct ccc_sll_elem_ *e = sll->sentinel_.n_; e != &sll->sentinel_;
         e = e->n_, ++size)
    {
        if (size >= sll->sz_)
        {
            return CCC_FALSE;
        }
        if (!e || !e->n_ || e->n_ == e)
        {
            return CCC_FALSE;
        }
    }
    return size == sll->sz_;
}

ccc_ucount
ccc_sll_size(ccc_singly_linked_list const *const sll)
{

    if (!sll)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = sll->sz_};
}

ccc_tribool
ccc_sll_is_empty(ccc_singly_linked_list const *const sll)
{
    if (!sll)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !sll->sz_;
}

/*==========================     Sorting     ================================*/

/** Sorts the list in O(NlgN) time with O(1) auxiliary space (no recursion).
If the list is already sorted this algorithm only needs one pass. */
ccc_result
ccc_sll_sort(ccc_singly_linked_list *const sll)
{
    if (!sll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    size_t run;
    do
    {
        run = 0;
        struct list_link a_start
            = {.prev = &sll->sentinel_, .cur = sll->sentinel_.n_};
        while (a_start.cur != &sll->sentinel_)
        {
            ++run;
            struct list_link a_end_b_start = first_unsorted(sll, a_start);
            if (a_end_b_start.cur == &sll->sentinel_)
            {
                break;
            }
            struct list_link b_end = first_unsorted(sll, a_end_b_start);
            a_start = merge(sll, a_start, a_end_b_start, b_end);
        }
    } while (run > 1);
    return CCC_RESULT_OK;
}

/** Returns a pair of elements marking the first list elem that is smaller than
its previous (CCC_LES) according to the user comparison callback. The list_link
returned will have the out of order element as cur and the last remaining in
order element as prev. The cur element may be the sentinel if the run is
sorted. */
static inline struct list_link
first_unsorted(ccc_singly_linked_list const *const sll, struct list_link p)
{
    do
    {
        p.prev = p.cur;
        p.cur = p.cur->n_;
    } while (p.cur != &sll->sentinel_ && cmp(sll, p.cur, p.prev) != CCC_LES);
    return p;
}

/** Merges two in order list runs. The lists will be sorted according to ranges
[a_start, a_end_b_start) to [a_end_b_start, b_end), ending in [a_start, b_end).
Once merging is complete the b_end elements are returned to help the user
progress the running and merging algorithm. A list_link must be returned because
merging may alter the b_end previous element and the user needs this list link
to remain accurate to progress the sorting correctly. */
static inline struct list_link
merge(ccc_singly_linked_list *const sll, struct list_link a_start,
      struct list_link a_end_b_start, struct list_link b_end)
{
    while (a_start.cur != a_end_b_start.cur && a_end_b_start.cur != b_end.cur)
    {
        if (cmp(sll, a_end_b_start.cur, a_start.cur) == CCC_LES)
        {
            struct ccc_sll_elem_ *const lesser = a_end_b_start.cur;
            a_end_b_start.prev->n_ = lesser->n_;
            /* Critical, otherwise algo breaks. b_end must be accurate. */
            if (lesser == b_end.prev)
            {
                b_end.prev = a_end_b_start.prev;
            }
            /* Must continue these checks after where lesser was but the prev
               does not change because only lesser was spliced out. */
            a_end_b_start.cur = lesser->n_;
            a_start.prev->n_ = lesser;
            lesser->n_ = a_start.cur;
            /* Another critical update that breaks algorithm if forgotten. */
            a_start.prev = lesser;
        }
        else
        {
            a_start.prev = a_start.cur;
            a_start.cur = a_start.cur->n_;
        }
    }
    return b_end;
}

/*=========================    Private Interface   ==========================*/

void
ccc_impl_sll_push_front(struct ccc_sll_ *const sll,
                        struct ccc_sll_elem_ *const elem)
{
    push_front(sll, elem);
}

/*===========================  Static Helpers   =============================*/

static inline void
push_front(struct ccc_sll_ *const sll, struct ccc_sll_elem_ *const elem)
{
    elem->n_ = sll->sentinel_.n_;
    sll->sentinel_.n_ = elem;
    ++sll->sz_;
}

static inline struct ccc_sll_elem_ *
pop_front(struct ccc_sll_ *const sll)
{
    struct ccc_sll_elem_ *remove = sll->sentinel_.n_;
    sll->sentinel_.n_ = remove->n_;
    if (remove != &sll->sentinel_)
    {
        remove->n_ = NULL;
    }
    --sll->sz_;
    return remove;
}

static inline struct ccc_sll_elem_ *
before(struct ccc_sll_ const *const sll,
       struct ccc_sll_elem_ const *const to_find)
{
    struct ccc_sll_elem_ const *i = &sll->sentinel_;
    for (; i->n_ != to_find; i = i->n_)
    {}
    return (struct ccc_sll_elem_ *)i;
}

static inline size_t
extract_range([[maybe_unused]] struct ccc_sll_ *const sll,
              struct ccc_sll_elem_ *begin, struct ccc_sll_elem_ *const end)
{
    size_t const sz = len(sll, begin, end);
    if (end != &sll->sentinel_)
    {
        end->n_ = NULL;
    }
    return sz;
}

static size_t
erase_range([[maybe_unused]] struct ccc_sll_ *const sll,
            struct ccc_sll_elem_ *begin, struct ccc_sll_elem_ *const end)
{
    if (!sll->alloc_)
    {
        size_t const sz = len(sll, begin, end);
        if (end != &sll->sentinel_)
        {
            end->n_ = NULL;
        }
        return sz;
    }
    size_t sz = 1;
    for (struct ccc_sll_elem_ *next = NULL; begin != end; begin = next, ++sz)
    {
        assert(sz <= sll->sz_);
        next = begin->n_;
        (void)sll->alloc_(struct_base(sll, begin), 0, sll->aux_);
    }
    (void)sll->alloc_(struct_base(sll, end), 0, sll->aux_);
    return sz;
}

static size_t
len([[maybe_unused]] struct ccc_sll_ const *const sll,
    struct ccc_sll_elem_ const *begin, struct ccc_sll_elem_ const *const end)
{
    size_t s = 1;
    for (; begin != end; begin = begin->n_, ++s)
    {
        assert(s <= sll->sz_);
    }
    return s;
}

static inline void *
struct_base(struct ccc_sll_ const *const l, struct ccc_sll_elem_ const *const e)
{
    return ((char *)&e->n_) - l->sll_elem_offset_;
}

static inline struct ccc_sll_elem_ *
elem_in(struct ccc_sll_ const *const sll, void const *const any_struct)
{
    return (struct ccc_sll_elem_ *)((char *)any_struct + sll->sll_elem_offset_);
}

/** Calls the user provided three way comparison callback function on the user
type wrapping the provided intrusive handles. Returns the three way comparison
result value. */
static inline ccc_threeway_cmp
cmp(struct ccc_sll_ const *const sll, struct ccc_sll_elem_ const *const lhs,
    struct ccc_sll_elem_ const *const rhs)
{
    return sll->cmp_((ccc_any_type_cmp){.any_type_lhs = struct_base(sll, lhs),
                                        .any_type_rhs = struct_base(sll, rhs),
                                        .aux = sll->aux_});
}
