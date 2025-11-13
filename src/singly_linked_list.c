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
struct Link
{
    /** The previous element of cur. Must manually update and manage. */
    struct CCC_Singly_linked_list_node *prev;
    /** The current element. Must manually manage and update. */
    struct CCC_Singly_linked_list_node *i;
};

/*===========================    Prototypes     =============================*/

static void *struct_base(struct CCC_Singly_linked_list const *,
                         struct CCC_Singly_linked_list_node const *);
static struct CCC_Singly_linked_list_node *
before(struct CCC_Singly_linked_list const *,
       struct CCC_Singly_linked_list_node const *to_find);
static size_t len(struct CCC_Singly_linked_list_node const *begin,
                  struct CCC_Singly_linked_list_node const *end);

static void push_front(struct CCC_Singly_linked_list *singly_linked_list,
                       struct CCC_Singly_linked_list_node *elem);
static size_t extract_range(struct CCC_Singly_linked_list *,
                            struct CCC_Singly_linked_list_node *begin,
                            struct CCC_Singly_linked_list_node *end);
static size_t erase_range(struct CCC_Singly_linked_list *,
                          struct CCC_Singly_linked_list_node const *begin,
                          struct CCC_Singly_linked_list_node *end);
static struct CCC_Singly_linked_list_node *
pop_front(struct CCC_Singly_linked_list *);
static struct CCC_Singly_linked_list_node *
elem_in(struct CCC_Singly_linked_list const *, void const *any_struct);
static struct Link merge(struct CCC_Singly_linked_list *, struct Link,
                         struct Link, struct Link);
static struct Link first_less(struct CCC_Singly_linked_list const *,
                              struct Link);
static CCC_Order cmp(struct CCC_Singly_linked_list const *singly_linked_list,
                     struct CCC_Singly_linked_list_node const *lhs,
                     struct CCC_Singly_linked_list_node const *rhs);

/*===========================     Interface     =============================*/

void *
CCC_singly_linked_list_push_front(
    CCC_Singly_linked_list *const singly_linked_list,
    CCC_Singly_linked_list_node *elem)
{
    if (!singly_linked_list || !elem)
    {
        return NULL;
    }
    if (singly_linked_list->alloc)
    {
        void *const node = singly_linked_list->alloc((CCC_Allocator_context){
            .input = NULL,
            .bytes = singly_linked_list->sizeof_type,
            .context = singly_linked_list->context,
        });
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(singly_linked_list, elem),
                     singly_linked_list->sizeof_type);
        elem = elem_in(singly_linked_list, node);
    }
    push_front(singly_linked_list, elem);
    return struct_base(singly_linked_list, singly_linked_list->nil.n);
}

void *
CCC_singly_linked_list_front(
    CCC_Singly_linked_list const *const singly_linked_list)
{
    if (!singly_linked_list
        || singly_linked_list->nil.n == &singly_linked_list->nil)
    {
        return NULL;
    }
    return struct_base(singly_linked_list, singly_linked_list->nil.n);
}

CCC_Singly_linked_list_node *
CCC_singly_linked_list_node_begin(
    CCC_Singly_linked_list const *const singly_linked_list)
{
    return singly_linked_list ? singly_linked_list->nil.n : NULL;
}

CCC_Singly_linked_list_node *
CCC_singly_linked_list_sentinel_begin(
    CCC_Singly_linked_list const *const singly_linked_list)
{
    return singly_linked_list
             ? (CCC_Singly_linked_list_node *)&singly_linked_list->nil
             : NULL;
}

CCC_Result
CCC_singly_linked_list_pop_front(
    CCC_Singly_linked_list *const singly_linked_list)
{
    if (!singly_linked_list || !singly_linked_list->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Singly_linked_list_node *const remove
        = pop_front(singly_linked_list);
    if (singly_linked_list->alloc)
    {
        (void)singly_linked_list->alloc((CCC_Allocator_context){
            .input = struct_base(singly_linked_list, remove),
            .bytes = 0,
            .context = singly_linked_list->context,
        });
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_singly_linked_list_splice(
    CCC_Singly_linked_list *const pos_singly_linked_list,
    CCC_Singly_linked_list_node *const pos,
    CCC_Singly_linked_list *const to_splice_singly_linked_list,
    CCC_Singly_linked_list_node *const to_splice)
{
    if (!pos_singly_linked_list || !pos || !to_splice
        || !to_splice_singly_linked_list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (to_splice == pos || pos->n == to_splice)
    {
        return CCC_RESULT_OK;
    }
    before(to_splice_singly_linked_list, to_splice)->n = to_splice->n;
    to_splice->n = pos->n;
    pos->n = to_splice;
    if (pos_singly_linked_list != to_splice_singly_linked_list)
    {
        --to_splice_singly_linked_list->count;
        ++pos_singly_linked_list->count;
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_singly_linked_list_splice_range(
    CCC_Singly_linked_list *const pos_singly_linked_list,
    CCC_Singly_linked_list_node *const pos,
    CCC_Singly_linked_list *const splice_singly_linked_list,
    CCC_Singly_linked_list_node *const begin,
    CCC_Singly_linked_list_node *const end)
{
    if (!pos_singly_linked_list || !pos || !begin || !end
        || !splice_singly_linked_list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (begin == pos || end == pos || pos->n == begin)
    {
        return CCC_RESULT_OK;
    }
    if (begin == end)
    {
        (void)CCC_singly_linked_list_splice(pos_singly_linked_list, pos,
                                            splice_singly_linked_list, begin);
        return CCC_RESULT_OK;
    }
    struct CCC_Singly_linked_list_node *const found
        = before(splice_singly_linked_list, begin);
    found->n = end->n;

    end->n = pos->n;
    pos->n = begin;
    if (pos_singly_linked_list != splice_singly_linked_list)
    {
        size_t const count = len(begin, end);
        splice_singly_linked_list->count -= count;
        pos_singly_linked_list->count += count;
    }
    return CCC_RESULT_OK;
}

void *
CCC_singly_linked_list_erase(CCC_Singly_linked_list *const singly_linked_list,
                             CCC_Singly_linked_list_node *const elem)
{
    if (!singly_linked_list || !elem || !singly_linked_list->count
        || elem == &singly_linked_list->nil)
    {
        return NULL;
    }
    struct CCC_Singly_linked_list_node const *const ret = elem->n;
    before(singly_linked_list, elem)->n = elem->n;
    if (elem != &singly_linked_list->nil)
    {
        elem->n = NULL;
    }
    if (singly_linked_list->alloc)
    {
        (void)singly_linked_list->alloc((CCC_Allocator_context){
            .input = struct_base(singly_linked_list, elem),
            .bytes = 0,
            .context = singly_linked_list->context,
        });
    }
    --singly_linked_list->count;
    return ret == &singly_linked_list->nil
             ? NULL
             : struct_base(singly_linked_list, ret);
}

void *
CCC_singly_linked_list_erase_range(
    CCC_Singly_linked_list *const singly_linked_list,
    CCC_Singly_linked_list_node *const begin, CCC_Singly_linked_list_node *end)
{
    if (!singly_linked_list || !begin || !end || !singly_linked_list->count
        || begin == &singly_linked_list->nil || end == &singly_linked_list->nil)
    {
        return NULL;
    }
    struct CCC_Singly_linked_list_node const *const ret = end->n;
    before(singly_linked_list, begin)->n = end->n;
    size_t const deleted = erase_range(singly_linked_list, begin, end);
    assert(deleted <= singly_linked_list->count);
    singly_linked_list->count -= deleted;
    return ret == &singly_linked_list->nil
             ? NULL
             : struct_base(singly_linked_list, ret);
}

void *
CCC_singly_linked_list_extract(CCC_Singly_linked_list *const singly_linked_list,
                               CCC_Singly_linked_list_node *const elem)
{
    if (!singly_linked_list || !elem || !singly_linked_list->count
        || elem == &singly_linked_list->nil)
    {
        return NULL;
    }
    struct CCC_Singly_linked_list_node const *const ret = elem->n;
    before(singly_linked_list, elem)->n = elem->n;
    if (elem != &singly_linked_list->nil)
    {
        elem->n = NULL;
    }
    --singly_linked_list->count;
    return ret == &singly_linked_list->nil
             ? NULL
             : struct_base(singly_linked_list, ret);
}

void *
CCC_singly_linked_list_extract_range(
    CCC_Singly_linked_list *const singly_linked_list,
    CCC_Singly_linked_list_node *const begin, CCC_Singly_linked_list_node *end)
{
    if (!singly_linked_list || !begin || !end || !singly_linked_list->count
        || begin == &singly_linked_list->nil || end == &singly_linked_list->nil)
    {
        return NULL;
    }
    struct CCC_Singly_linked_list_node const *const ret = end->n;
    before(singly_linked_list, begin)->n = end->n;
    size_t const deleted = extract_range(singly_linked_list, begin, end);
    assert(deleted <= singly_linked_list->count);
    singly_linked_list->count -= deleted;
    return ret == &singly_linked_list->nil
             ? NULL
             : struct_base(singly_linked_list, ret);
}

void *
CCC_singly_linked_list_begin(
    CCC_Singly_linked_list const *const singly_linked_list)
{
    if (!singly_linked_list
        || singly_linked_list->nil.n == &singly_linked_list->nil)
    {
        return NULL;
    }
    return struct_base(singly_linked_list, singly_linked_list->nil.n);
}

void *
CCC_singly_linked_list_end(CCC_Singly_linked_list const *const)
{
    return NULL;
}

void *
CCC_singly_linked_list_next(
    CCC_Singly_linked_list const *const singly_linked_list,
    CCC_Singly_linked_list_node const *const elem)
{
    if (!singly_linked_list || !elem || elem->n == &singly_linked_list->nil)
    {
        return NULL;
    }
    return struct_base(singly_linked_list, elem->n);
}

CCC_Result
CCC_singly_linked_list_clear(CCC_Singly_linked_list *const singly_linked_list,
                             CCC_Type_destructor *const fn)
{
    if (!singly_linked_list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    while (!CCC_singly_linked_list_is_empty(singly_linked_list))
    {
        void *const mem
            = struct_base(singly_linked_list, pop_front(singly_linked_list));
        if (fn)
        {
            fn((CCC_Type_context){.any_type = mem,
                                  .context = singly_linked_list->context});
        }
        if (singly_linked_list->alloc)
        {
            (void)singly_linked_list->alloc((CCC_Allocator_context){
                .input = mem,
                .bytes = 0,
                .context = singly_linked_list->context,
            });
        }
    }
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_singly_linked_list_validate(
    CCC_Singly_linked_list const *const singly_linked_list)
{
    if (!singly_linked_list)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t size = 0;
    for (struct CCC_Singly_linked_list_node *e = singly_linked_list->nil.n;
         e != &singly_linked_list->nil; e = e->n, ++size)
    {
        if (size >= singly_linked_list->count)
        {
            return CCC_FALSE;
        }
        if (!e || !e->n || e->n == e)
        {
            return CCC_FALSE;
        }
    }
    return size == singly_linked_list->count;
}

CCC_Count
CCC_singly_linked_list_count(
    CCC_Singly_linked_list const *const singly_linked_list)
{

    if (!singly_linked_list)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = singly_linked_list->count};
}

CCC_Tribool
CCC_singly_linked_list_is_empty(
    CCC_Singly_linked_list const *const singly_linked_list)
{
    if (!singly_linked_list)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !singly_linked_list->count;
}

/*==========================     Sorting     ================================*/

/** Returns true if the list is sorted in non-decreasing order. The user should
flip the return values of their comparison function if they want a different
order for elements.*/
CCC_Tribool
CCC_singly_linked_list_is_sorted(
    CCC_Singly_linked_list const *const singly_linked_list)
{
    if (!singly_linked_list)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (singly_linked_list->count <= 1)
    {
        return CCC_TRUE;
    }
    for (struct CCC_Singly_linked_list_node const *prev
         = singly_linked_list->nil.n,
         *cur = singly_linked_list->nil.n->n;
         cur != &singly_linked_list->nil; prev = cur, cur = cur->n)
    {
        if (cmp(singly_linked_list, prev, cur) == CCC_ORDER_GREATER)
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
CCC_singly_linked_list_insert_sorted(CCC_Singly_linked_list *singly_linked_list,
                                     CCC_Singly_linked_list_node *e)
{
    if (!singly_linked_list || !e)
    {
        return NULL;
    }
    if (singly_linked_list->alloc)
    {
        void *const node = singly_linked_list->alloc((CCC_Allocator_context){
            .input = NULL,
            .bytes = singly_linked_list->sizeof_type,
            .context = singly_linked_list->context,
        });
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(singly_linked_list, e),
                     singly_linked_list->sizeof_type);
        e = elem_in(singly_linked_list, node);
    }
    struct CCC_Singly_linked_list_node *prev = &singly_linked_list->nil;
    struct CCC_Singly_linked_list_node *i = singly_linked_list->nil.n;
    for (; i != &singly_linked_list->nil
           && cmp(singly_linked_list, e, i) != CCC_ORDER_LESSER;
         prev = i, i = i->n)
    {}
    e->n = i;
    prev->n = e;
    ++singly_linked_list->count;
    return struct_base(singly_linked_list, e);
}

/** Sorts the list in `O(N * log(N))` time with `O(1)` context space (no
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
CCC_singly_linked_list_sort(CCC_Singly_linked_list *const singly_linked_list)
{
    if (!singly_linked_list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* Algorithm is one pass if list is sorted. Merging is never true. */
    CCC_Tribool merging = CCC_FALSE;
    do
    {
        merging = CCC_FALSE;
        /* 0th index of the A list. The start of one list to merge. */
        struct Link a_0 = {.prev = &singly_linked_list->nil,
                           .i = singly_linked_list->nil.n};
        while (a_0.i != &singly_linked_list->nil)
        {
            /* The Nth index of list A (its size) aka 0th index of B list. */
            struct Link a_n_b_0 = first_less(singly_linked_list, a_0);
            if (a_n_b_0.i == &singly_linked_list->nil)
            {
                break;
            }
            /* A picks up the exclusive end of this merge, B, in order
               to progress the sorting algorithm with the next run that needs
               fixing. Merge returns the final B element to indicate it is the
               final sentinel that has not yet been examined. */
            a_0 = merge(singly_linked_list, a_0, a_n_b_0,
                        first_less(singly_linked_list, a_n_b_0));
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
static inline struct Link
merge(CCC_Singly_linked_list *const singly_linked_list, struct Link a_0,
      struct Link a_n_b_0, struct Link b_n)
{
    while (a_0.i != a_n_b_0.i && a_n_b_0.i != b_n.i)
    {
        if (cmp(singly_linked_list, a_n_b_0.i, a_0.i) == CCC_ORDER_LESSER)
        {
            /* The i element is the lesser element that must be spliced out.
               However, a_n_b_0.prev is not updated because only i is spliced
               out. Algorithm will continue with new i, but same prev. */
            struct CCC_Singly_linked_list_node *const lesser = a_n_b_0.i;
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
its previous `CCC_ORDER_LESSER` according to the user comparison callback. The
list_link returned will have the out of order element as cur and the last
remaining in order element as prev. The cur element may be the sentinel if the
run is sorted. */
static inline struct Link
first_less(CCC_Singly_linked_list const *const singly_linked_list,
           struct Link k)
{
    do
    {
        k.prev = k.i;
        k.i = k.i->n;
    }
    while (k.i != &singly_linked_list->nil
           && cmp(singly_linked_list, k.i, k.prev) != CCC_ORDER_LESSER);
    return k;
}

/*=========================    Private Interface   ==========================*/

void
CCC_private_singly_linked_list_push_front(
    struct CCC_Singly_linked_list *const singly_linked_list,
    struct CCC_Singly_linked_list_node *const elem)
{
    push_front(singly_linked_list, elem);
}

/*===========================  Static Helpers   =============================*/

static inline void
push_front(struct CCC_Singly_linked_list *const singly_linked_list,
           struct CCC_Singly_linked_list_node *const elem)
{
    elem->n = singly_linked_list->nil.n;
    singly_linked_list->nil.n = elem;
    ++singly_linked_list->count;
}

static inline struct CCC_Singly_linked_list_node *
pop_front(struct CCC_Singly_linked_list *const singly_linked_list)
{
    struct CCC_Singly_linked_list_node *const remove
        = singly_linked_list->nil.n;
    singly_linked_list->nil.n = remove->n;
    if (remove != &singly_linked_list->nil)
    {
        remove->n = NULL;
    }
    --singly_linked_list->count;
    return remove;
}

static inline struct CCC_Singly_linked_list_node *
before(struct CCC_Singly_linked_list const *const singly_linked_list,
       struct CCC_Singly_linked_list_node const *const to_find)
{
    struct CCC_Singly_linked_list_node const *i = &singly_linked_list->nil;
    for (; i->n != to_find; i = i->n)
    {}
    return (struct CCC_Singly_linked_list_node *)i;
}

static inline size_t
extract_range(struct CCC_Singly_linked_list *const singly_linked_list,
              struct CCC_Singly_linked_list_node *begin,
              struct CCC_Singly_linked_list_node *const end)
{
    size_t const count = len(begin, end);
    if (end != &singly_linked_list->nil)
    {
        end->n = NULL;
    }
    return count;
}

static size_t
erase_range(struct CCC_Singly_linked_list *const singly_linked_list,
            struct CCC_Singly_linked_list_node const *begin,
            struct CCC_Singly_linked_list_node *const end)
{
    if (!singly_linked_list->alloc)
    {
        size_t const count = len(begin, end);
        if (end != &singly_linked_list->nil)
        {
            end->n = NULL;
        }
        return count;
    }
    size_t count = 1;
    for (struct CCC_Singly_linked_list_node const *next = NULL; begin != end;
         begin = next, ++count)
    {
        assert(count <= singly_linked_list->count);
        next = begin->n;
        (void)singly_linked_list->alloc((CCC_Allocator_context){
            .input = struct_base(singly_linked_list, begin),
            .bytes = 0,
            .context = singly_linked_list->context,
        });
    }
    (void)singly_linked_list->alloc((CCC_Allocator_context){
        .input = struct_base(singly_linked_list, end),
        .bytes = 0,
        .context = singly_linked_list->context,
    });
    return count;
}

/** Returns the length [begin, end] inclusive. Assumes end follows begin. */
static size_t
len(struct CCC_Singly_linked_list_node const *begin,
    struct CCC_Singly_linked_list_node const *const end)
{
    size_t s = 1;
    for (; begin != end; begin = begin->n, ++s)
    {}
    return s;
}

/** Provides the base address of the user struct holding e. */
static inline void *
struct_base(struct CCC_Singly_linked_list const *const l,
            struct CCC_Singly_linked_list_node const *const e)
{
    return ((char *)&e->n) - l->singly_linked_list_node_offset;
}

/** Given the user struct provides the address of intrusive elem. */
static inline struct CCC_Singly_linked_list_node *
elem_in(struct CCC_Singly_linked_list const *const singly_linked_list,
        void const *const any_struct)
{
    return (struct CCC_Singly_linked_list_node
                *)((char *)any_struct
                   + singly_linked_list->singly_linked_list_node_offset);
}

/** Calls the user provided three way comparison callback function on the user
type wrapping the provided intrusive handles. Returns the three way comparison
result value. */
static inline CCC_Order
cmp(struct CCC_Singly_linked_list const *const singly_linked_list,
    struct CCC_Singly_linked_list_node const *const lhs,
    struct CCC_Singly_linked_list_node const *const rhs)
{
    return singly_linked_list->cmp((CCC_Type_comparator_context){
        .any_type_lhs = struct_base(singly_linked_list, lhs),
        .any_type_rhs = struct_base(singly_linked_list, rhs),
        .context = singly_linked_list->context,
    });
}
