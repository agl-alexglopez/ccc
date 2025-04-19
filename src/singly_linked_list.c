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
/* Citation:
[1] See the sort methods for citations and change lists regarding the pintOS
educational operating system natural merge sort algorithm used for linked lists.
Code in the pintOS source is at  `src/lib/kernel.list.c`, but this may change if
they refactor. */
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "impl/impl_singly_linked_list.h"
#include "singly_linked_list.h"
#include "types.h"

/** @brief When sorting, a singly linked list is at a disadvantage for iterative
O(1) space merge sort: it doesn't have a prev pointer. This will help list
elements remember their previous element for splicing and merging. */
struct list_link
{
    /** The previous element of cur. Must manually update and manage. */
    struct ccc_sll_elem *prev;
    /** The current element. Must manually manage and update. */
    struct ccc_sll_elem *i;
};

/*===========================    Prototypes     =============================*/

static void *struct_base(struct ccc_sll const *, struct ccc_sll_elem const *);
static struct ccc_sll_elem *before(struct ccc_sll const *,
                                   struct ccc_sll_elem const *to_find);
static size_t len(struct ccc_sll_elem const *begin,
                  struct ccc_sll_elem const *end);

static void push_front(struct ccc_sll *sll, struct ccc_sll_elem *elem);
static size_t extract_range(struct ccc_sll *, struct ccc_sll_elem *begin,
                            struct ccc_sll_elem *end);
static size_t erase_range(struct ccc_sll *, struct ccc_sll_elem *begin,
                          struct ccc_sll_elem *end);
static struct ccc_sll_elem *pop_front(struct ccc_sll *);
static struct ccc_sll_elem *elem_in(struct ccc_sll const *,
                                    void const *any_struct);
static struct list_link merge(struct ccc_sll *, struct list_link,
                              struct list_link, struct list_link);
static struct list_link first_less(struct ccc_sll const *, struct list_link);
static ccc_threeway_cmp cmp(struct ccc_sll const *sll,
                            struct ccc_sll_elem const *lhs,
                            struct ccc_sll_elem const *rhs);

/*===========================     Interface     =============================*/

void *
ccc_sll_push_front(ccc_singly_linked_list *const sll, ccc_sll_elem *elem)
{
    if (!sll || !elem)
    {
        return NULL;
    }
    if (sll->alloc)
    {
        void *const node = sll->alloc(NULL, sll->sizeof_type, sll->aux);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(sll, elem), sll->sizeof_type);
        elem = elem_in(sll, node);
    }
    push_front(sll, elem);
    return struct_base(sll, sll->nil.n);
}

void *
ccc_sll_front(ccc_singly_linked_list const *const sll)
{
    if (!sll || sll->nil.n == &sll->nil)
    {
        return NULL;
    }
    return struct_base(sll, sll->nil.n);
}

ccc_sll_elem *
ccc_sll_begin_elem(ccc_singly_linked_list const *const sll)
{
    return sll ? sll->nil.n : NULL;
}

ccc_sll_elem *
ccc_sll_begin_sentinel(ccc_singly_linked_list const *const sll)
{
    return sll ? (ccc_sll_elem *)&sll->nil : NULL;
}

ccc_result
ccc_sll_pop_front(ccc_singly_linked_list *const sll)
{
    if (!sll || !sll->count)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    struct ccc_sll_elem *const remove = pop_front(sll);
    if (sll->alloc)
    {
        (void)sll->alloc(struct_base(sll, remove), 0, sll->aux);
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_sll_splice(ccc_singly_linked_list *const pos_sll, ccc_sll_elem *const pos,
               ccc_singly_linked_list *const to_splice_sll,
               ccc_sll_elem *const to_splice)
{
    if (!pos_sll || !pos || !to_splice || !to_splice_sll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (to_splice == pos || pos->n == to_splice)
    {
        return CCC_RESULT_OK;
    }
    before(to_splice_sll, to_splice)->n = to_splice->n;
    to_splice->n = pos->n;
    pos->n = to_splice;
    if (pos_sll != to_splice_sll)
    {
        --to_splice_sll->count;
        ++pos_sll->count;
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
    if (begin == pos || end == pos || pos->n == begin)
    {
        return CCC_RESULT_OK;
    }
    if (begin == end)
    {
        (void)ccc_sll_splice(pos_sll, pos, splice_sll, begin);
        return CCC_RESULT_OK;
    }
    struct ccc_sll_elem *const found = before(splice_sll, begin);
    found->n = end->n;

    end->n = pos->n;
    pos->n = begin;
    if (pos_sll != splice_sll)
    {
        size_t const count = len(begin, end);
        splice_sll->count -= count;
        pos_sll->count += count;
    }
    return CCC_RESULT_OK;
}

void *
ccc_sll_erase(ccc_singly_linked_list *const sll, ccc_sll_elem *const elem)
{
    if (!sll || !elem || !sll->count || elem == &sll->nil)
    {
        return NULL;
    }
    struct ccc_sll_elem const *const ret = elem->n;
    before(sll, elem)->n = elem->n;
    if (elem != &sll->nil)
    {
        elem->n = NULL;
    }
    if (sll->alloc)
    {
        (void)sll->alloc(struct_base(sll, elem), 0, sll->aux);
    }
    --sll->count;
    return ret == &sll->nil ? NULL : struct_base(sll, ret);
}

void *
ccc_sll_erase_range(ccc_singly_linked_list *const sll,
                    ccc_sll_elem *const begin, ccc_sll_elem *end)
{
    if (!sll || !begin || !end || !sll->count || begin == &sll->nil
        || end == &sll->nil)
    {
        return NULL;
    }
    struct ccc_sll_elem const *const ret = end->n;
    before(sll, begin)->n = end->n;
    size_t const deleted = erase_range(sll, begin, end);
    assert(deleted <= sll->count);
    sll->count -= deleted;
    return ret == &sll->nil ? NULL : struct_base(sll, ret);
}

void *
ccc_sll_extract(ccc_singly_linked_list *const sll, ccc_sll_elem *const elem)
{
    if (!sll || !elem || !sll->count || elem == &sll->nil)
    {
        return NULL;
    }
    struct ccc_sll_elem const *const ret = elem->n;
    before(sll, elem)->n = elem->n;
    if (elem != &sll->nil)
    {
        elem->n = NULL;
    }
    --sll->count;
    return ret == &sll->nil ? NULL : struct_base(sll, ret);
}

void *
ccc_sll_extract_range(ccc_singly_linked_list *const sll,
                      ccc_sll_elem *const begin, ccc_sll_elem *end)
{
    if (!sll || !begin || !end || !sll->count || begin == &sll->nil
        || end == &sll->nil)
    {
        return NULL;
    }
    struct ccc_sll_elem const *const ret = end->n;
    before(sll, begin)->n = end->n;
    size_t const deleted = extract_range(sll, begin, end);
    assert(deleted <= sll->count);
    sll->count -= deleted;
    return ret == &sll->nil ? NULL : struct_base(sll, ret);
}

void *
ccc_sll_begin(ccc_singly_linked_list const *const sll)
{
    if (!sll || sll->nil.n == &sll->nil)
    {
        return NULL;
    }
    return struct_base(sll, sll->nil.n);
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
    if (!sll || !elem || elem->n == &sll->nil)
    {
        return NULL;
    }
    return struct_base(sll, elem->n);
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
            fn((ccc_any_type){.any_type = mem, .aux = sll->aux});
        }
        if (sll->alloc)
        {
            (void)sll->alloc(mem, 0, sll->aux);
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
    for (struct ccc_sll_elem *e = sll->nil.n; e != &sll->nil; e = e->n, ++size)
    {
        if (size >= sll->count)
        {
            return CCC_FALSE;
        }
        if (!e || !e->n || e->n == e)
        {
            return CCC_FALSE;
        }
    }
    return size == sll->count;
}

ccc_ucount
ccc_sll_size(ccc_singly_linked_list const *const sll)
{

    if (!sll)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = sll->count};
}

ccc_tribool
ccc_sll_is_empty(ccc_singly_linked_list const *const sll)
{
    if (!sll)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !sll->count;
}

/*==========================     Sorting     ================================*/

/** Returns true if the list is sorted in non-decreasing order. The user should
flip the return values of their comparison function if they want a different
order for elements.*/
ccc_tribool
ccc_sll_is_sorted(ccc_singly_linked_list const *const sll)
{
    if (!sll)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (sll->count <= 1)
    {
        return CCC_TRUE;
    }
    for (struct ccc_sll_elem const *prev = sll->nil.n, *cur = sll->nil.n->n;
         cur != &sll->nil; prev = cur, cur = cur->n)
    {
        if (cmp(sll, prev, cur) == CCC_GRT)
        {
            return CCC_FALSE;
        }
    }
    return CCC_TRUE;
}

/** Inserts an element in non-decreasing order. This means an element will go
to the end of a section of duplicate values which is good for round-robin style
list use. */
void *
ccc_sll_insert_sorted(ccc_singly_linked_list *sll, ccc_sll_elem *e)
{
    if (!sll || !e)
    {
        return NULL;
    }
    if (sll->alloc)
    {
        void *const node = sll->alloc(NULL, sll->sizeof_type, sll->aux);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(sll, e), sll->sizeof_type);
        e = elem_in(sll, node);
    }
    struct ccc_sll_elem *prev = &sll->nil;
    struct ccc_sll_elem *i = sll->nil.n;
    for (; i != &sll->nil && cmp(sll, e, i) != CCC_LES; prev = i, i = i->n)
    {}
    e->n = i;
    prev->n = e;
    ++sll->count;
    return struct_base(sll, e);
}

/** Sorts the list in `O(N*log(N))` time with `O(1)` auxiliary space (no
recursion). If the list is already sorted this algorithm only needs one pass.

The following merging algorithm and associated helper functions are based on
the iterative natural merge sort used in the list module of the pintOS project
for learning operating systems. Currently the original is located at the
following path in the pintOS source code:

`src/lib/kernel/list.c`

However, if refactors change this location, seek the list intrusive container
module for original implementations. The code has been changed for the C
Container Collection as follows:

- the algorithm is adapted to work with a singly linked list rather than doubly
- there single sentinel node rather than two.
- splicing in the merge operation has been simplified along with other tweaks.
- comparison callbacks are handled with three way comparison. */
ccc_result
ccc_sll_sort(ccc_singly_linked_list *const sll)
{
    if (!sll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Algorithm is one pass if list is sorted. Merging is never true. */
    ccc_tribool merging = CCC_FALSE;
    do
    {
        merging = CCC_FALSE;
        /* 0th index of the A list. The start of one list to merge. */
        struct list_link a_0 = {.prev = &sll->nil, .i = sll->nil.n};
        while (a_0.i != &sll->nil)
        {
            /* The Nth index of list A (its size) aka 0th index of B list. */
            struct list_link a_n_b_0 = first_less(sll, a_0);
            if (a_n_b_0.i == &sll->nil)
            {
                break;
            }
            /* a_0 picks up the exclusive end of this merge, b_n, in order
               to progress the sorting algorithm with the next run that needs
               fixing. Merge returns this b_n element to indicate it is the
               final element that has not been processed by merge comparison. */
            a_0 = merge(sll, a_0, a_n_b_0, first_less(sll, a_n_b_0));
            merging = CCC_TRUE;
        }
    } while (merging);
    return CCC_RESULT_OK;
}

/** Returns a pair of elements marking the first list elem that is smaller than
its previous `CCC_LES` according to the user comparison callback. The
list_link returned will have the out of order element as cur and the last
remaining in order element as prev. The cur element may be the sentinel if the
run is sorted. */
static inline struct list_link
first_less(ccc_singly_linked_list const *const sll, struct list_link k)
{
    do
    {
        k.prev = k.i;
        k.i = k.i->n;
    } while (k.i != &sll->nil && cmp(sll, k.i, k.prev) != CCC_LES);
    return k;
}

/** Merges lists `[a_0, a_n_b_0)` with `[a_n_b_0, b_n)` to form `[a_0, b_n)`.
Returns the exclusive end of the range, `b_n`, once the merge sort is complete.

Notice that all ranges exclude their final element from the merge for
consistency. This function assumes the provided lists are already sorted
separately. A list link must be returned because the b_n previous field may be
updated due to arbitrary splices during comparison sorting. */
static inline struct list_link
merge(ccc_singly_linked_list *const sll, struct list_link a_0,
      struct list_link a_n_b_0, struct list_link b_n)
{
    while (a_0.i != a_n_b_0.i && a_n_b_0.i != b_n.i)
    {
        if (cmp(sll, a_n_b_0.i, a_0.i) == CCC_LES)
        {
            struct ccc_sll_elem *const lesser = a_n_b_0.i;
            /* Must continue these checks after where lesser was but the prev
               does not change because only lesser was spliced out. */
            a_n_b_0.i = lesser->n;
            a_n_b_0.prev->n = lesser->n;
            /* Critical, otherwise algo breaks. b_n must be accurate. */
            if (lesser == b_n.prev)
            {
                b_n.prev = a_n_b_0.prev;
            }
            a_0.prev->n = lesser;
            lesser->n = a_0.i;
            /* Another critical update that breaks algorithm if forgotten. */
            a_0.prev = lesser;
        }
        else
        {
            a_0.prev = a_0.i;
            a_0.i = a_0.i->n;
        }
    }
    return b_n;
}

/*=========================    Private Interface   ==========================*/

void
ccc_impl_sll_push_front(struct ccc_sll *const sll,
                        struct ccc_sll_elem *const elem)
{
    push_front(sll, elem);
}

/*===========================  Static Helpers   =============================*/

static inline void
push_front(struct ccc_sll *const sll, struct ccc_sll_elem *const elem)
{
    elem->n = sll->nil.n;
    sll->nil.n = elem;
    ++sll->count;
}

static inline struct ccc_sll_elem *
pop_front(struct ccc_sll *const sll)
{
    struct ccc_sll_elem *remove = sll->nil.n;
    sll->nil.n = remove->n;
    if (remove != &sll->nil)
    {
        remove->n = NULL;
    }
    --sll->count;
    return remove;
}

static inline struct ccc_sll_elem *
before(struct ccc_sll const *const sll,
       struct ccc_sll_elem const *const to_find)
{
    struct ccc_sll_elem const *i = &sll->nil;
    for (; i->n != to_find; i = i->n)
    {}
    return (struct ccc_sll_elem *)i;
}

static inline size_t
extract_range([[maybe_unused]] struct ccc_sll *const sll,
              struct ccc_sll_elem *begin, struct ccc_sll_elem *const end)
{
    size_t const count = len(begin, end);
    if (end != &sll->nil)
    {
        end->n = NULL;
    }
    return count;
}

static size_t
erase_range([[maybe_unused]] struct ccc_sll *const sll,
            struct ccc_sll_elem *begin, struct ccc_sll_elem *const end)
{
    if (!sll->alloc)
    {
        size_t const count = len(begin, end);
        if (end != &sll->nil)
        {
            end->n = NULL;
        }
        return count;
    }
    size_t count = 1;
    for (struct ccc_sll_elem *next = NULL; begin != end; begin = next, ++count)
    {
        assert(count <= sll->count);
        next = begin->n;
        (void)sll->alloc(struct_base(sll, begin), 0, sll->aux);
    }
    (void)sll->alloc(struct_base(sll, end), 0, sll->aux);
    return count;
}

/** Returns the length [begin, end] inclusive. Assumes end follows begin. */
static size_t
len(struct ccc_sll_elem const *begin, struct ccc_sll_elem const *const end)
{
    size_t s = 1;
    for (; begin != end; begin = begin->n, ++s)
    {}
    return s;
}

/** Provides the base address of the user struct holding e. */
static inline void *
struct_base(struct ccc_sll const *const l, struct ccc_sll_elem const *const e)
{
    return ((char *)&e->n) - l->sll_elem_offset;
}

/** Given the user struct provides the address of intrusive elem. */
static inline struct ccc_sll_elem *
elem_in(struct ccc_sll const *const sll, void const *const any_struct)
{
    return (struct ccc_sll_elem *)((char *)any_struct + sll->sll_elem_offset);
}

/** Calls the user provided three way comparison callback function on the user
type wrapping the provided intrusive handles. Returns the three way comparison
result value. */
static inline ccc_threeway_cmp
cmp(struct ccc_sll const *const sll, struct ccc_sll_elem const *const lhs,
    struct ccc_sll_elem const *const rhs)
{
    return sll->cmp((ccc_any_type_cmp){.any_type_lhs = struct_base(sll, lhs),
                                       .any_type_rhs = struct_base(sll, rhs),
                                       .aux = sll->aux});
}
