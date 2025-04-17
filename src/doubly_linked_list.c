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

#include "doubly_linked_list.h"
#include "impl/impl_doubly_linked_list.h"
#include "types.h"

/*===========================   Prototypes    ===============================*/

static void *struct_base(struct ccc_dll_ const *, struct ccc_dll_elem_ const *);
static size_t extract_range(struct ccc_dll_ const *l,
                            struct ccc_dll_elem_ *begin,
                            struct ccc_dll_elem_ *end);
static size_t erase_range(struct ccc_dll_ const *l, struct ccc_dll_elem_ *begin,
                          struct ccc_dll_elem_ *end);
static size_t len(struct ccc_dll_elem_ const *begin,
                  struct ccc_dll_elem_ const *end);
static struct ccc_dll_elem_ *pop_front(struct ccc_dll_ *);
static void push_front(struct ccc_dll_ *l, struct ccc_dll_elem_ *e);
static struct ccc_dll_elem_ *elem_in(ccc_doubly_linked_list const *,
                                     void const *struct_base);
static void push_back(ccc_doubly_linked_list *, struct ccc_dll_elem_ *);
static void splice_range(struct ccc_dll_elem_ *pos, struct ccc_dll_elem_ *begin,
                         struct ccc_dll_elem_ *end);
static struct ccc_dll_elem_ *first_less(struct ccc_dll_ const *dll,
                                        struct ccc_dll_elem_ *);
static struct ccc_dll_elem_ *merge(struct ccc_dll_ *, struct ccc_dll_elem_ *,
                                   struct ccc_dll_elem_ *,
                                   struct ccc_dll_elem_ *);
static ccc_threeway_cmp cmp(struct ccc_dll_ const *dll,
                            struct ccc_dll_elem_ const *lhs,
                            struct ccc_dll_elem_ const *rhs);

/*===========================     Interface   ===============================*/

void *
ccc_dll_push_front(ccc_doubly_linked_list *const l, ccc_dll_elem *elem)
{
    if (!l || !elem)
    {
        return NULL;
    }
    if (l->alloc_)
    {
        void *const node = l->alloc_(NULL, l->elem_sz_, l->aux_);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(l, elem), l->elem_sz_);
        elem = elem_in(l, node);
    }
    push_front(l, elem);
    return struct_base(l, l->sentinel_.n_);
}

void *
ccc_dll_push_back(ccc_doubly_linked_list *const l, ccc_dll_elem *elem)
{
    if (!l || !elem)
    {
        return NULL;
    }
    if (l->alloc_)
    {
        void *const node = l->alloc_(NULL, l->elem_sz_, l->aux_);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(l, elem), l->elem_sz_);
        elem = elem_in(l, node);
    }
    push_back(l, elem);
    return struct_base(l, l->sentinel_.p_);
}

void *
ccc_dll_front(ccc_doubly_linked_list const *l)
{
    if (!l || !l->sz_)
    {
        return NULL;
    }
    return struct_base(l, l->sentinel_.n_);
}

void *
ccc_dll_back(ccc_doubly_linked_list const *l)
{
    if (!l || !l->sz_)
    {
        return NULL;
    }
    return struct_base(l, l->sentinel_.p_);
}

ccc_result
ccc_dll_pop_front(ccc_doubly_linked_list *const l)
{
    if (!l || !l->sz_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    struct ccc_dll_elem_ *remove = pop_front(l);
    if (l->alloc_)
    {
        (void)l->alloc_(struct_base(l, remove), 0, l->aux_);
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_dll_pop_back(ccc_doubly_linked_list *const l)
{
    if (!l || !l->sz_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    struct ccc_dll_elem_ *remove = l->sentinel_.p_;
    remove->p_->n_ = &l->sentinel_;
    l->sentinel_.p_ = remove->p_;
    if (remove != &l->sentinel_)
    {
        remove->n_ = remove->p_ = NULL;
    }
    if (l->alloc_)
    {
        (void)l->alloc_(struct_base(l, remove), 0, l->aux_);
    }
    --l->sz_;
    return CCC_RESULT_OK;
}

void *
ccc_dll_insert(ccc_doubly_linked_list *const l, ccc_dll_elem *const pos,
               ccc_dll_elem *elem)
{
    if (!l || !pos->n_ || !pos->p_)
    {
        return NULL;
    }
    if (l->alloc_)
    {
        void *const node = l->alloc_(NULL, l->elem_sz_, l->aux_);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(l, elem), l->elem_sz_);
        elem = elem_in(l, node);
    }
    elem->n_ = pos;
    elem->p_ = pos->p_;

    pos->p_->n_ = elem;
    pos->p_ = elem;
    ++l->sz_;
    return struct_base(l, elem);
}

void *
ccc_dll_erase(ccc_doubly_linked_list *const l, ccc_dll_elem *const elem)
{
    if (!l || !elem || !l->sz_)
    {
        return NULL;
    }
    elem->n_->p_ = elem->p_;
    elem->p_->n_ = elem->n_;
    void *const ret = elem->n_ == &l->sentinel_ ? NULL : struct_base(l, elem);
    if (l->alloc_)
    {
        (void)l->alloc_(struct_base(l, elem), 0, l->aux_);
    }
    --l->sz_;
    return ret;
}

void *
ccc_dll_erase_range(ccc_doubly_linked_list *const l,
                    ccc_dll_elem *const elem_begin, ccc_dll_elem *elem_end)
{
    if (!l || !elem_begin || !elem_end || !elem_begin->n_ || !elem_begin->p_
        || !elem_end->n_ || !elem_end->p_ || !l->sz_)
    {
        return NULL;
    }
    if (elem_begin == elem_end)
    {
        return ccc_dll_erase(l, elem_begin);
    }
    struct ccc_dll_elem_ const *const ret = elem_end;
    elem_end = elem_end->p_;
    elem_end->n_->p_ = elem_begin->p_;
    elem_begin->p_->n_ = elem_end->n_;

    size_t const deleted = erase_range(l, elem_begin, elem_end);

    assert(deleted <= l->sz_);
    l->sz_ -= deleted;
    return ret == &l->sentinel_ ? NULL : struct_base(l, ret);
}

ccc_dll_elem *
ccc_dll_begin_elem(ccc_doubly_linked_list const *const l)
{
    return l ? l->sentinel_.n_ : NULL;
}

ccc_dll_elem *
ccc_dll_end_elem(ccc_doubly_linked_list const *const l)
{
    return l ? l->sentinel_.p_ : NULL;
}

ccc_dll_elem *
ccc_dll_end_sentinel(ccc_doubly_linked_list const *const l)
{
    return l ? (struct ccc_dll_elem_ *)&l->sentinel_ : NULL;
}

void *
ccc_dll_extract(ccc_doubly_linked_list *const l,
                ccc_dll_elem *const elem_in_list)
{
    if (!l || !elem_in_list || !elem_in_list->n_ || !elem_in_list->p_
        || !l->sz_)
    {
        return NULL;
    }
    struct ccc_dll_elem_ const *const ret = elem_in_list->n_;
    elem_in_list->n_->p_ = elem_in_list->p_;
    elem_in_list->p_->n_ = elem_in_list->n_;
    if (elem_in_list != &l->sentinel_)
    {
        elem_in_list->n_ = elem_in_list->p_ = NULL;
    }
    --l->sz_;
    return ret == &l->sentinel_ ? NULL : struct_base(l, ret);
}

void *
ccc_dll_extract_range(ccc_doubly_linked_list *const l,
                      ccc_dll_elem *const elem_begin, ccc_dll_elem *elem_end)
{
    if (!l || !elem_begin || !elem_end || !elem_begin->n_ || !elem_begin->p_
        || !elem_end->n_ || !elem_end->p_ || !l->sz_)
    {
        return NULL;
    }
    if (elem_begin == elem_end)
    {
        return ccc_dll_extract(l, elem_begin);
    }
    struct ccc_dll_elem_ const *const ret = elem_end;
    elem_end = elem_end->p_;
    elem_end->n_->p_ = elem_begin->p_;
    elem_begin->p_->n_ = elem_end->n_;

    size_t const deleted = extract_range(l, elem_begin, elem_end);

    assert(deleted <= l->sz_);
    l->sz_ -= deleted;
    return ret == &l->sentinel_ ? NULL : struct_base(l, ret);
}

ccc_result
ccc_dll_splice(ccc_doubly_linked_list *const pos_dll, ccc_dll_elem *pos,
               ccc_doubly_linked_list *const to_cut_dll,
               ccc_dll_elem *const to_cut)
{
    if (!to_cut || !pos || !pos_dll || !to_cut_dll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (pos == to_cut || to_cut->n_ == pos || to_cut == &to_cut_dll->sentinel_)
    {
        return CCC_RESULT_OK;
    }
    to_cut->n_->p_ = to_cut->p_;
    to_cut->p_->n_ = to_cut->n_;

    to_cut->p_ = pos->p_;
    to_cut->n_ = pos;
    pos->p_->n_ = to_cut;
    pos->p_ = to_cut;
    if (pos_dll != to_cut_dll)
    {
        ++pos_dll->sz_;
        --to_cut_dll->sz_;
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_dll_splice_range(ccc_doubly_linked_list *const pos_dll, ccc_dll_elem *pos,
                     ccc_doubly_linked_list *const to_cut_dll,
                     ccc_dll_elem *begin, ccc_dll_elem *end)
{
    if (!begin || !end || !pos || !pos_dll || !to_cut_dll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (pos == begin || pos == end || begin == end
        || begin == &to_cut_dll->sentinel_ || end->p_ == &to_cut_dll->sentinel_)
    {
        return CCC_RESULT_OK;
    }
    struct ccc_dll_elem_ *const inclusive_end = end->p_;
    splice_range(pos, begin, end);
    if (pos_dll != to_cut_dll)
    {
        size_t const sz = len(begin, inclusive_end);
        assert(sz <= to_cut_dll->sz_);
        pos_dll->sz_ += sz;
        to_cut_dll->sz_ -= sz;
    }
    return CCC_RESULT_OK;
}

void *
ccc_dll_begin(ccc_doubly_linked_list const *const l)
{
    if (!l || l->sentinel_.n_ == &l->sentinel_)
    {
        return NULL;
    }
    return struct_base(l, l->sentinel_.n_);
}

void *
ccc_dll_rbegin(ccc_doubly_linked_list const *const l)
{
    if (!l || l->sentinel_.p_ == &l->sentinel_)
    {
        return NULL;
    }
    return struct_base(l, l->sentinel_.p_);
}

void *
ccc_dll_end(ccc_doubly_linked_list const *const)
{
    return NULL;
}

void *
ccc_dll_rend(ccc_doubly_linked_list const *const)
{
    return NULL;
}

void *
ccc_dll_next(ccc_doubly_linked_list const *const l, ccc_dll_elem const *e)
{
    if (!l || !e || e->n_ == &l->sentinel_)
    {
        return NULL;
    }
    return struct_base(l, e->n_);
}

void *
ccc_dll_rnext(ccc_doubly_linked_list const *const l, ccc_dll_elem const *e)
{
    if (!l || !e || e->p_ == &l->sentinel_)
    {
        return NULL;
    }
    return struct_base(l, e->p_);
}

ccc_ucount
ccc_dll_size(ccc_doubly_linked_list const *const l)
{
    if (!l)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = l->sz_};
}

ccc_tribool
ccc_dll_is_empty(ccc_doubly_linked_list const *const l)
{
    return l ? !l->sz_ : CCC_TRUE;
}

ccc_result
ccc_dll_clear(ccc_doubly_linked_list *const l, ccc_any_type_destructor_fn *fn)
{
    if (!l)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    while (!ccc_dll_is_empty(l))
    {
        void *const node = struct_base(l, pop_front(l));
        if (fn)
        {
            fn((ccc_any_type){.any_type = node, .aux = l->aux_});
        }
        if (l->alloc_)
        {
            (void)l->alloc_(node, 0, l->aux_);
        }
    }
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_dll_validate(ccc_doubly_linked_list const *const l)
{
    if (!l)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t size = 0;
    for (struct ccc_dll_elem_ const *e = l->sentinel_.n_; e != &l->sentinel_;
         e = e->n_, ++size)
    {
        if (size >= l->sz_)
        {
            return CCC_FALSE;
        }
        if (!e || !e->n_ || !e->p_ || e->n_ == e || e->p_ == e)
        {
            return CCC_FALSE;
        }
    }
    return size == l->sz_;
}

/*==========================     Sorting     ================================*/

ccc_tribool
ccc_dll_is_sorted(ccc_doubly_linked_list const *const dll)
{
    if (!dll)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (dll->sz_ <= 1)
    {
        return CCC_TRUE;
    }
    for (struct ccc_dll_elem_ const *cur = dll->sentinel_.n_->n_;
         cur != &dll->sentinel_; cur = cur->n_)
    {
        if (cmp(dll, cur->p_, cur) == CCC_GRT)
        {
            return CCC_FALSE;
        }
    }
    return CCC_TRUE;
}

void *
ccc_dll_insert_sorted(ccc_doubly_linked_list *const dll, ccc_dll_elem *e)
{
    if (!dll || !e)
    {
        return NULL;
    }
    if (dll->alloc_)
    {
        void *const node = dll->alloc_(NULL, dll->elem_sz_, dll->aux_);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(dll, e), dll->elem_sz_);
        e = elem_in(dll, node);
    }
    struct ccc_dll_elem_ *pos = dll->sentinel_.n_;
    for (; pos != &dll->sentinel_ && cmp(dll, e, pos) != CCC_LES; pos = pos->n_)
    {}
    e->n_ = pos;
    e->p_ = pos->p_;
    pos->p_->n_ = e;
    pos->p_ = e;
    ++dll->sz_;
    return struct_base(dll, e);
}

/** Sorts the list into non-decreasing order according to the user comparison
callback function in O(NlgN) time and O(1) space. */
ccc_result
ccc_dll_sort(ccc_doubly_linked_list *const dll)
{
    if (!dll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Algorithm runs a single pass if list is already sorted aka run = 1. */
    size_t run = 0;
    do
    {
        run = 0;
        /* 0th index of the A list. The start of one list to merge. */
        struct ccc_dll_elem_ *a_0 = dll->sentinel_.n_;
        while (a_0 != &dll->sentinel_)
        {
            ++run;
            /* The Nth index of list A (its size) aka 0th index of B list. */
            struct ccc_dll_elem_ *const a_n_b_0 = first_less(dll, a_0);
            if (a_n_b_0 == &dll->sentinel_)
            {
                break;
            }
            /* a_0 picks up the exclusive end of this merge, b_n, in order
               to progress the sorting algorithm with the next run that needs
               fixing. Merge returns this b_n element to indicate it is the
               final element that has not been processed by merge comparison. */
            a_0 = merge(dll, a_0, a_n_b_0, first_less(dll, a_n_b_0));
        }
    } while (run > 1);
    return CCC_RESULT_OK;
}

/** Finds the first element lesser than it's previous element as defined by
the user comparison callback function. If no out of order element can be
found the list sentinel is returned. */
static inline struct ccc_dll_elem_ *
first_less(struct ccc_dll_ const *const dll, struct ccc_dll_elem_ *start)
{
    assert(dll && start);
    do
    {
        start = start->n_;
    } while (start != &dll->sentinel_ && cmp(dll, start, start->p_) != CCC_LES);
    return start;
}

/** Merges lists [a_0, a_n_b_0) with [a_n_b_0, b_n) to form [a_0, b_n). Returns
the exclusive end of the range, b_n, once the merge sort is complete.

Notice that all ranges exclude their final element from the merge for
consistency. This function assumes the provided lists are already sorted
separately. */
static inline struct ccc_dll_elem_ *
merge(struct ccc_dll_ *const dll, struct ccc_dll_elem_ *a_0,
      struct ccc_dll_elem_ *a_n_b_0, struct ccc_dll_elem_ *const b_n)
{
    assert(dll && a_0 && a_n_b_0 && b_n);
    while (a_0 != a_n_b_0 && a_n_b_0 != b_n)
    {
        if (cmp(dll, a_n_b_0, a_0) == CCC_LES)
        {
            struct ccc_dll_elem_ *const lesser = a_n_b_0;
            a_n_b_0 = lesser->n_;
            lesser->n_->p_ = lesser->p_;
            lesser->p_->n_ = lesser->n_;
            lesser->p_ = a_0->p_;
            lesser->n_ = a_0;
            a_0->p_->n_ = lesser;
            a_0->p_ = lesser;
        }
        else
        {
            a_0 = a_0->n_;
        }
    }
    return b_n;
}

/*=======================     Private Interface   ===========================*/

void
ccc_impl_dll_push_back(struct ccc_dll_ *const l, struct ccc_dll_elem_ *const e)
{
    if (!l || !e)
    {
        return;
    }
    push_back(l, e);
}

void
ccc_impl_dll_push_front(struct ccc_dll_ *const l, struct ccc_dll_elem_ *const e)
{
    if (!l || !e)
    {
        return;
    }
    push_front(l, e);
}

struct ccc_dll_elem_ *
ccc_impl_dll_elem_in(struct ccc_dll_ const *const l,
                     void const *const any_struct)
{
    return elem_in(l, any_struct);
}

/*=======================       Static Helpers    ===========================*/

/** Places the range [begin, end) at position before pos. This means end is not
moved or altered due to the exclusive range. If begin is equal to end the
function returns early changing no nodes. */
static inline void
splice_range(struct ccc_dll_elem_ *const pos, struct ccc_dll_elem_ *const begin,
             struct ccc_dll_elem_ *end)
{
    if (begin == end)
    {
        return;
    }
    end = end->p_;
    end->n_->p_ = begin->p_;
    begin->p_->n_ = end->n_;

    begin->p_ = pos->p_;
    end->n_ = pos;
    pos->p_->n_ = begin;
    pos->p_ = end;
}

static inline void
push_front(struct ccc_dll_ *const l, struct ccc_dll_elem_ *const e)
{
    e->p_ = &l->sentinel_;
    e->n_ = l->sentinel_.n_;
    l->sentinel_.n_->p_ = e;
    l->sentinel_.n_ = e;
    ++l->sz_;
}

static inline void
push_back(ccc_doubly_linked_list *const l, struct ccc_dll_elem_ *const e)
{
    e->n_ = &l->sentinel_;
    e->p_ = l->sentinel_.p_;
    l->sentinel_.p_->n_ = e;
    l->sentinel_.p_ = e;
    ++l->sz_;
}

static inline struct ccc_dll_elem_ *
pop_front(struct ccc_dll_ *const dll)
{
    struct ccc_dll_elem_ *ret = dll->sentinel_.n_;
    ret->n_->p_ = &dll->sentinel_;
    dll->sentinel_.n_ = ret->n_;
    if (ret != &dll->sentinel_)
    {
        ret->n_ = ret->p_ = NULL;
    }
    --dll->sz_;
    return ret;
}

static inline size_t
extract_range([[maybe_unused]] struct ccc_dll_ const *const l,
              struct ccc_dll_elem_ *begin, struct ccc_dll_elem_ *const end)
{
    if (begin != &l->sentinel_)
    {
        begin->p_ = NULL;
    }
    size_t sz = len(begin, end);
    if (end != &l->sentinel_)
    {
        end->n_ = NULL;
    }
    return sz;
}

static size_t
erase_range(struct ccc_dll_ const *const l, struct ccc_dll_elem_ *begin,
            struct ccc_dll_elem_ *const end)
{
    if (begin != &l->sentinel_)
    {
        begin->p_ = NULL;
    }
    if (!l->alloc_)
    {
        size_t const sz = len(begin, end);
        if (end != &l->sentinel_)
        {
            end->n_ = NULL;
        }
        return sz;
    }
    size_t sz = 1;
    for (; begin != end; ++sz)
    {
        assert(sz <= l->sz_);
        struct ccc_dll_elem_ *const next = begin->n_;
        (void)l->alloc_(struct_base(l, begin), 0, l->aux_);
        begin = next;
    }
    (void)l->alloc_(struct_base(l, end), 0, l->aux_);
    return sz;
}

/** Finds the length from [begin, end]. End is counted. */
static size_t
len(struct ccc_dll_elem_ const *begin, struct ccc_dll_elem_ const *const end)
{
    size_t s = 1;
    for (; begin != end; begin = begin->n_, ++s)
    {}
    return s;
}

static inline void *
struct_base(struct ccc_dll_ const *const l, struct ccc_dll_elem_ const *const e)
{
    return ((char *)&e->n_) - l->dll_elem_offset_;
}

static inline struct ccc_dll_elem_ *
elem_in(ccc_doubly_linked_list const *const dll, void const *const struct_base)
{
    return (struct ccc_dll_elem_ *)((char *)struct_base
                                    + dll->dll_elem_offset_);
}

/** Calls the user provided three way comparison callback function on the user
type wrapping the provided intrusive handles. Returns the three way comparison
result value. */
static inline ccc_threeway_cmp
cmp(struct ccc_dll_ const *const dll, struct ccc_dll_elem_ const *const lhs,
    struct ccc_dll_elem_ const *const rhs)
{
    return dll->cmp_((ccc_any_type_cmp){.any_type_lhs = struct_base(dll, lhs),
                                        .any_type_rhs = struct_base(dll, rhs),
                                        .aux = dll->aux_});
}
