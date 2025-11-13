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

#include "private/private_singly_linked_list.h"
#include "singly_linked_list.h"
#include "types.h"

/** @brief When sorting, a singly linked list is at a disadvantage for iterative
O(1) space merge sort: it doesn't have a prev pointer. This will help list
elements remember their previous element for splicing and merging. */
struct list_link
{
    /** The previous element of cur. Must manually update and manage. */
    struct CCC_sll_elem *prev;
    /** The current element. Must manually manage and update. */
    struct CCC_sll_elem *i;
};

/*===========================    Prototypes     =============================*/

static void *struct_base(struct CCC_sll const *, struct CCC_sll_elem const *);
static struct CCC_sll_elem *before(struct CCC_sll const *,
                                   struct CCC_sll_elem const *to_find);
static size_t len(struct CCC_sll_elem const *begin,
                  struct CCC_sll_elem const *end);

static void push_front(struct CCC_sll *sll, struct CCC_sll_elem *elem);
static size_t extract_range(struct CCC_sll *, struct CCC_sll_elem *begin,
                            struct CCC_sll_elem *end);
static size_t erase_range(struct CCC_sll *, struct CCC_sll_elem const *begin,
                          struct CCC_sll_elem *end);
static struct CCC_sll_elem *pop_front(struct CCC_sll *);
static struct CCC_sll_elem *elem_in(struct CCC_sll const *,
                                    void const *any_struct);
static struct list_link merge(struct CCC_sll *, struct list_link,
                              struct list_link, struct list_link);
static struct list_link first_less(struct CCC_sll const *, struct list_link);
static CCC_Order cmp(struct CCC_sll const *sll, struct CCC_sll_elem const *lhs,
                     struct CCC_sll_elem const *rhs);

/*===========================     Interface     =============================*/

void *
CCC_sll_push_front(CCC_singly_linked_list *const sll, CCC_sll_elem *elem)
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
CCC_sll_front(CCC_singly_linked_list const *const sll)
{
    if (!sll || sll->nil.n == &sll->nil)
    {
        return NULL;
    }
    return struct_base(sll, sll->nil.n);
}

CCC_sll_elem *
CCC_sll_elem_begin(CCC_singly_linked_list const *const sll)
{
    return sll ? sll->nil.n : NULL;
}

CCC_sll_elem *
CCC_sll_sentinel_begin(CCC_singly_linked_list const *const sll)
{
    return sll ? (CCC_sll_elem *)&sll->nil : NULL;
}

CCC_Result
CCC_sll_pop_front(CCC_singly_linked_list *const sll)
{
    if (!sll || !sll->count)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    struct CCC_sll_elem *const remove = pop_front(sll);
    if (sll->alloc)
    {
        (void)sll->alloc(struct_base(sll, remove), 0, sll->aux);
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_sll_splice(CCC_singly_linked_list *const pos_sll, CCC_sll_elem *const pos,
               CCC_singly_linked_list *const to_splice_sll,
               CCC_sll_elem *const to_splice)
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

CCC_Result
CCC_sll_splice_range(CCC_singly_linked_list *const pos_sll,
                     CCC_sll_elem *const pos,
                     CCC_singly_linked_list *const splice_sll,
                     CCC_sll_elem *const begin, CCC_sll_elem *const end)
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
        (void)CCC_sll_splice(pos_sll, pos, splice_sll, begin);
        return CCC_RESULT_OK;
    }
    struct CCC_sll_elem *const found = before(splice_sll, begin);
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
CCC_sll_erase(CCC_singly_linked_list *const sll, CCC_sll_elem *const elem)
{
    if (!sll || !elem || !sll->count || elem == &sll->nil)
    {
        return NULL;
    }
    struct CCC_sll_elem const *const ret = elem->n;
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
CCC_sll_erase_range(CCC_singly_linked_list *const sll,
                    CCC_sll_elem *const begin, CCC_sll_elem *end)
{
    if (!sll || !begin || !end || !sll->count || begin == &sll->nil
        || end == &sll->nil)
    {
        return NULL;
    }
    struct CCC_sll_elem const *const ret = end->n;
    before(sll, begin)->n = end->n;
    size_t const deleted = erase_range(sll, begin, end);
    assert(deleted <= sll->count);
    sll->count -= deleted;
    return ret == &sll->nil ? NULL : struct_base(sll, ret);
}

void *
CCC_sll_extract(CCC_singly_linked_list *const sll, CCC_sll_elem *const elem)
{
    if (!sll || !elem || !sll->count || elem == &sll->nil)
    {
        return NULL;
    }
    struct CCC_sll_elem const *const ret = elem->n;
    before(sll, elem)->n = elem->n;
    if (elem != &sll->nil)
    {
        elem->n = NULL;
    }
    --sll->count;
    return ret == &sll->nil ? NULL : struct_base(sll, ret);
}

void *
CCC_sll_extract_range(CCC_singly_linked_list *const sll,
                      CCC_sll_elem *const begin, CCC_sll_elem *end)
{
    if (!sll || !begin || !end || !sll->count || begin == &sll->nil
        || end == &sll->nil)
    {
        return NULL;
    }
    struct CCC_sll_elem const *const ret = end->n;
    before(sll, begin)->n = end->n;
    size_t const deleted = extract_range(sll, begin, end);
    assert(deleted <= sll->count);
    sll->count -= deleted;
    return ret == &sll->nil ? NULL : struct_base(sll, ret);
}

void *
CCC_sll_begin(CCC_singly_linked_list const *const sll)
{
    if (!sll || sll->nil.n == &sll->nil)
    {
        return NULL;
    }
    return struct_base(sll, sll->nil.n);
}

void *
CCC_sll_end(CCC_singly_linked_list const *const)
{
    return NULL;
}

void *
CCC_sll_next(CCC_singly_linked_list const *const sll,
             CCC_sll_elem const *const elem)
{
    if (!sll || !elem || elem->n == &sll->nil)
    {
        return NULL;
    }
    return struct_base(sll, elem->n);
}

CCC_Result
CCC_sll_clear(CCC_singly_linked_list *const sll, CCC_Type_destructor *const fn)
{
    if (!sll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    while (!CCC_sll_is_empty(sll))
    {
        void *const mem = struct_base(sll, pop_front(sll));
        if (fn)
        {
            fn((CCC_Type_context){.any_type = mem, .aux = sll->aux});
        }
        if (sll->alloc)
        {
            (void)sll->alloc(mem, 0, sll->aux);
        }
    }
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_sll_validate(CCC_singly_linked_list const *const sll)
{
    if (!sll)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t size = 0;
    for (struct CCC_sll_elem *e = sll->nil.n; e != &sll->nil; e = e->n, ++size)
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

CCC_Count
CCC_sll_count(CCC_singly_linked_list const *const sll)
{

    if (!sll)
    {
        return (CCC_Count){.error = CCC_RESULT_ARG_ERROR};
    }
    return (CCC_Count){.count = sll->count};
}

CCC_Tribool
CCC_sll_is_empty(CCC_singly_linked_list const *const sll)
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
CCC_Tribool
CCC_sll_is_sorted(CCC_singly_linked_list const *const sll)
{
    if (!sll)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (sll->count <= 1)
    {
        return CCC_TRUE;
    }
    for (struct CCC_sll_elem const *prev = sll->nil.n, *cur = sll->nil.n->n;
         cur != &sll->nil; prev = cur, cur = cur->n)
    {
        if (cmp(sll, prev, cur) == CCC_ORDER_GREATER)
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
CCC_sll_insert_sorted(CCC_singly_linked_list *sll, CCC_sll_elem *e)
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
    struct CCC_sll_elem *prev = &sll->nil;
    struct CCC_sll_elem *i = sll->nil.n;
    for (; i != &sll->nil && cmp(sll, e, i) != CCC_ORDER_LESS;
         prev = i, i = i->n)
    {}
    e->n = i;
    prev->n = e;
    ++sll->count;
    return struct_base(sll, e);
}

/** Sorts the list in `O(N * log(N))` time with `O(1)` auxiliary space (no
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
- there is a single sentinel node rather than two.
- splicing in the merge operation has been simplified along with other tweaks.
- comparison callbacks are handled with three way comparison.

If the runtime is not obvious from the code, consider that this algorithm runs
bottom up on sorted sub-ranges. It roughly "halves" the remaining sub-ranges
that need to be sorted by roughly "doubling" the length of a sorted range on
each merge step. Therefore the number of times we must perform the merge step is
`O(log(N))`. The most elements we would have to merge in the merge step is all
`N` elements so together that gives us the runtime of `O(N * log(N))`. */
CCC_Result
CCC_sll_sort(CCC_singly_linked_list *const sll)
{
    if (!sll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Algorithm is one pass if list is sorted. Merging is never true. */
    CCC_Tribool merging = CCC_FALSE;
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
            /* A picks up the exclusive end of this merge, B, in order
               to progress the sorting algorithm with the next run that needs
               fixing. Merge returns the final B element to indicate it is the
               final sentinel that has not yet been examined. */
            a_0 = merge(sll, a_0, a_n_b_0, first_less(sll, a_n_b_0));
            merging = CCC_TRUE;
        }
    }
    while (merging);
    return CCC_RESULT_OK;
}

/** Merges lists `[a_0, a_n_b_0)` with `[a_n_b_0, b_n)` to form `[a_0, b_n)`.
Returns the exclusive end of the range, `b_n`, once the merge sort is complete.

Notice that all ranges treat the end of their range as an exclusive sentinel for
consistency. This function assumes the provided lists are already sorted
separately. A list link must be returned because the `b_n` previous field may be
updated due to arbitrary splices during comparison sorting. */
static inline struct list_link
merge(CCC_singly_linked_list *const sll, struct list_link a_0,
      struct list_link a_n_b_0, struct list_link b_n)
{
    while (a_0.i != a_n_b_0.i && a_n_b_0.i != b_n.i)
    {
        if (cmp(sll, a_n_b_0.i, a_0.i) == CCC_ORDER_LESS)
        {
            /* The i element is the lesser element that must be spliced out.
               However, a_n_b_0.prev is not updated because only i is spliced
               out. Algorithm will continue with new i, but same prev. */
            struct CCC_sll_elem *const lesser = a_n_b_0.i;
            a_n_b_0.i = lesser->n;
            a_n_b_0.prev->n = lesser->n;
            /* This is so we return an accurate b_n list link at the end. */
            if (lesser == b_n.prev)
            {
                b_n.prev = a_n_b_0.prev;
            }
            a_0.prev->n = lesser;
            lesser->n = a_0.i;
            /* Another critical update reflected in our links, not the list. */
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

/** Returns a pair of elements marking the first list elem that is smaller than
its previous `CCC_ORDER_LESS` according to the user comparison callback. The
list_link returned will have the out of order element as cur and the last
remaining in order element as prev. The cur element may be the sentinel if the
run is sorted. */
static inline struct list_link
first_less(CCC_singly_linked_list const *const sll, struct list_link k)
{
    do
    {
        k.prev = k.i;
        k.i = k.i->n;
    }
    while (k.i != &sll->nil && cmp(sll, k.i, k.prev) != CCC_ORDER_LESS);
    return k;
}

/*=========================    Private Interface   ==========================*/

void
CCC_private_sll_push_front(struct CCC_sll *const sll,
                           struct CCC_sll_elem *const elem)
{
    push_front(sll, elem);
}

/*===========================  Static Helpers   =============================*/

static inline void
push_front(struct CCC_sll *const sll, struct CCC_sll_elem *const elem)
{
    elem->n = sll->nil.n;
    sll->nil.n = elem;
    ++sll->count;
}

static inline struct CCC_sll_elem *
pop_front(struct CCC_sll *const sll)
{
    struct CCC_sll_elem *const remove = sll->nil.n;
    sll->nil.n = remove->n;
    if (remove != &sll->nil)
    {
        remove->n = NULL;
    }
    --sll->count;
    return remove;
}

static inline struct CCC_sll_elem *
before(struct CCC_sll const *const sll,
       struct CCC_sll_elem const *const to_find)
{
    struct CCC_sll_elem const *i = &sll->nil;
    for (; i->n != to_find; i = i->n)
    {}
    return (struct CCC_sll_elem *)i;
}

static inline size_t
extract_range(struct CCC_sll *const sll, struct CCC_sll_elem *begin,
              struct CCC_sll_elem *const end)
{
    size_t const count = len(begin, end);
    if (end != &sll->nil)
    {
        end->n = NULL;
    }
    return count;
}

static size_t
erase_range(struct CCC_sll *const sll, struct CCC_sll_elem const *begin,
            struct CCC_sll_elem *const end)
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
    for (struct CCC_sll_elem const *next = NULL; begin != end;
         begin = next, ++count)
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
len(struct CCC_sll_elem const *begin, struct CCC_sll_elem const *const end)
{
    size_t s = 1;
    for (; begin != end; begin = begin->n, ++s)
    {}
    return s;
}

/** Provides the base address of the user struct holding e. */
static inline void *
struct_base(struct CCC_sll const *const l, struct CCC_sll_elem const *const e)
{
    return ((char *)&e->n) - l->sll_elem_offset;
}

/** Given the user struct provides the address of intrusive elem. */
static inline struct CCC_sll_elem *
elem_in(struct CCC_sll const *const sll, void const *const any_struct)
{
    return (struct CCC_sll_elem *)((char *)any_struct + sll->sll_elem_offset);
}

/** Calls the user provided three way comparison callback function on the user
type wrapping the provided intrusive handles. Returns the three way comparison
result value. */
static inline CCC_Order
cmp(struct CCC_sll const *const sll, struct CCC_sll_elem const *const lhs,
    struct CCC_sll_elem const *const rhs)
{
    return sll->cmp((CCC_Type_comparator_context){
        .any_type_lhs = struct_base(sll, lhs),
        .any_type_rhs = struct_base(sll, rhs),
        .aux = sll->aux,
    });
}
