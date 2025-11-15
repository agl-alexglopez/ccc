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
Code in the pintOS source is at  `src/lib/kernel.list.c`, but this may change
if they refactor. */
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
    struct CCC_Singly_linked_list_node *previous;
    /** The current element. Must manually manage and update. */
    struct CCC_Singly_linked_list_node *current;
};

/*===========================    Prototypes     =============================*/

static void *struct_base(struct CCC_Singly_linked_list const *,
                         struct CCC_Singly_linked_list_node const *);
static struct CCC_Singly_linked_list_node *
before(struct CCC_Singly_linked_list const *,
       struct CCC_Singly_linked_list_node const *);
static size_t len(struct CCC_Singly_linked_list_node const *,
                  struct CCC_Singly_linked_list_node const *);
static void push_front(struct CCC_Singly_linked_list *,
                       struct CCC_Singly_linked_list_node *);
static size_t extract_range(struct CCC_Singly_linked_list *,
                            struct CCC_Singly_linked_list_node *,
                            struct CCC_Singly_linked_list_node *);
static size_t erase_range(struct CCC_Singly_linked_list *,
                          struct CCC_Singly_linked_list_node const *,
                          struct CCC_Singly_linked_list_node *);
static struct CCC_Singly_linked_list_node *
pop_front(struct CCC_Singly_linked_list *);
static struct CCC_Singly_linked_list_node *
elem_in(struct CCC_Singly_linked_list const *, void const *);
static struct Link merge(struct CCC_Singly_linked_list *, struct Link,
                         struct Link, struct Link);
static struct Link first_less(struct CCC_Singly_linked_list const *,
                              struct Link);
static CCC_Order order(struct CCC_Singly_linked_list const *,
                       struct CCC_Singly_linked_list_node const *,
                       struct CCC_Singly_linked_list_node const *);

/*===========================     Interface     =============================*/

void *
CCC_singly_linked_list_push_front(CCC_Singly_linked_list *const list,
                                  CCC_Singly_linked_list_node *type_intruder)
{
    if (!list || !type_intruder)
    {
        return NULL;
    }
    if (list->allocate)
    {
        void *const node = list->allocate((CCC_Allocator_context){
            .input = NULL,
            .bytes = list->sizeof_type,
            .context = list->context,
        });
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(list, type_intruder), list->sizeof_type);
        type_intruder = elem_in(list, node);
    }
    push_front(list, type_intruder);
    return struct_base(list, list->nil.n);
}

void *
CCC_singly_linked_list_front(CCC_Singly_linked_list const *const list)
{
    if (!list || list->nil.n == &list->nil)
    {
        return NULL;
    }
    return struct_base(list, list->nil.n);
}

CCC_Singly_linked_list_node *
CCC_singly_linked_list_node_begin(CCC_Singly_linked_list const *const list)
{
    return list ? list->nil.n : NULL;
}

CCC_Singly_linked_list_node *
CCC_singly_linked_list_sentinel_begin(CCC_Singly_linked_list const *const list)
{
    return list ? (CCC_Singly_linked_list_node *)&list->nil : NULL;
}

CCC_Result
CCC_singly_linked_list_pop_front(CCC_Singly_linked_list *const list)
{
    if (!list || !list->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Singly_linked_list_node *const remove = pop_front(list);
    if (list->allocate)
    {
        (void)list->allocate((CCC_Allocator_context){
            .input = struct_base(list, remove),
            .bytes = 0,
            .context = list->context,
        });
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_singly_linked_list_splice(
    CCC_Singly_linked_list *const position_list,
    CCC_Singly_linked_list_node *const type_intruder_position,
    CCC_Singly_linked_list *const splice_list,
    CCC_Singly_linked_list_node *const type_intruder_splice)
{
    if (!position_list || !type_intruder_position || !type_intruder_splice
        || !splice_list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (type_intruder_splice == type_intruder_position
        || type_intruder_position->n == type_intruder_splice)
    {
        return CCC_RESULT_OK;
    }
    before(splice_list, type_intruder_splice)->n = type_intruder_splice->n;
    type_intruder_splice->n = type_intruder_position->n;
    type_intruder_position->n = type_intruder_splice;
    if (position_list != splice_list)
    {
        --splice_list->count;
        ++position_list->count;
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_singly_linked_list_splice_range(
    CCC_Singly_linked_list *const position_list,
    CCC_Singly_linked_list_node *const type_intruder_position,
    CCC_Singly_linked_list *const splice_list,
    CCC_Singly_linked_list_node *const type_intruder_begin,
    CCC_Singly_linked_list_node *const type_intruder_end)
{
    if (!position_list || !type_intruder_position || !type_intruder_begin
        || !type_intruder_end || !splice_list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (type_intruder_begin == type_intruder_position
        || type_intruder_end == type_intruder_position
        || type_intruder_position->n == type_intruder_begin)
    {
        return CCC_RESULT_OK;
    }
    if (type_intruder_begin == type_intruder_end)
    {
        (void)CCC_singly_linked_list_splice(position_list,
                                            type_intruder_position, splice_list,
                                            type_intruder_begin);
        return CCC_RESULT_OK;
    }
    struct CCC_Singly_linked_list_node *const found
        = before(splice_list, type_intruder_begin);
    found->n = type_intruder_end->n;

    type_intruder_end->n = type_intruder_position->n;
    type_intruder_position->n = type_intruder_begin;
    if (position_list != splice_list)
    {
        size_t const count = len(type_intruder_begin, type_intruder_end);
        splice_list->count -= count;
        position_list->count += count;
    }
    return CCC_RESULT_OK;
}

void *
CCC_singly_linked_list_erase(CCC_Singly_linked_list *const list,
                             CCC_Singly_linked_list_node *const type_intruder)
{
    if (!list || !type_intruder || !list->count || type_intruder == &list->nil)
    {
        return NULL;
    }
    struct CCC_Singly_linked_list_node const *const ret = type_intruder->n;
    before(list, type_intruder)->n = type_intruder->n;
    if (type_intruder != &list->nil)
    {
        type_intruder->n = NULL;
    }
    if (list->allocate)
    {
        (void)list->allocate((CCC_Allocator_context){
            .input = struct_base(list, type_intruder),
            .bytes = 0,
            .context = list->context,
        });
    }
    --list->count;
    return ret == &list->nil ? NULL : struct_base(list, ret);
}

void *
CCC_singly_linked_list_erase_range(
    CCC_Singly_linked_list *const list,
    CCC_Singly_linked_list_node *const type_intruder_begin,
    CCC_Singly_linked_list_node *type_intruder_end)
{
    if (!list || !type_intruder_begin || !type_intruder_end || !list->count
        || type_intruder_begin == &list->nil || type_intruder_end == &list->nil)
    {
        return NULL;
    }
    struct CCC_Singly_linked_list_node const *const ret = type_intruder_end->n;
    before(list, type_intruder_begin)->n = type_intruder_end->n;
    size_t const deleted
        = erase_range(list, type_intruder_begin, type_intruder_end);
    assert(deleted <= list->count);
    list->count -= deleted;
    return ret == &list->nil ? NULL : struct_base(list, ret);
}

void *
CCC_singly_linked_list_extract(CCC_Singly_linked_list *const list,
                               CCC_Singly_linked_list_node *const type_intruder)
{
    if (!list || !type_intruder || !list->count || type_intruder == &list->nil)
    {
        return NULL;
    }
    struct CCC_Singly_linked_list_node const *const ret = type_intruder->n;
    before(list, type_intruder)->n = type_intruder->n;
    if (type_intruder != &list->nil)
    {
        type_intruder->n = NULL;
    }
    --list->count;
    return ret == &list->nil ? NULL : struct_base(list, ret);
}

void *
CCC_singly_linked_list_extract_range(
    CCC_Singly_linked_list *const list,
    CCC_Singly_linked_list_node *const type_intruder_begin,
    CCC_Singly_linked_list_node *type_intruder_end)
{
    if (!list || !type_intruder_begin || !type_intruder_end || !list->count
        || type_intruder_begin == &list->nil || type_intruder_end == &list->nil)
    {
        return NULL;
    }
    struct CCC_Singly_linked_list_node const *const ret = type_intruder_end->n;
    before(list, type_intruder_begin)->n = type_intruder_end->n;
    size_t const deleted
        = extract_range(list, type_intruder_begin, type_intruder_end);
    assert(deleted <= list->count);
    list->count -= deleted;
    return ret == &list->nil ? NULL : struct_base(list, ret);
}

void *
CCC_singly_linked_list_begin(CCC_Singly_linked_list const *const list)
{
    if (!list || list->nil.n == &list->nil)
    {
        return NULL;
    }
    return struct_base(list, list->nil.n);
}

void *
CCC_singly_linked_list_end(CCC_Singly_linked_list const *const)
{
    return NULL;
}

void *
CCC_singly_linked_list_next(
    CCC_Singly_linked_list const *const list,
    CCC_Singly_linked_list_node const *const type_intruder)
{
    if (!list || !type_intruder || type_intruder->n == &list->nil)
    {
        return NULL;
    }
    return struct_base(list, type_intruder->n);
}

CCC_Result
CCC_singly_linked_list_clear(CCC_Singly_linked_list *const list,
                             CCC_Type_destructor *const destroy)
{
    if (!list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    while (!CCC_singly_linked_list_is_empty(list))
    {
        void *const data = struct_base(list, pop_front(list));
        if (destroy)
        {
            destroy((CCC_Type_context){.type = data, .context = list->context});
        }
        if (list->allocate)
        {
            (void)list->allocate((CCC_Allocator_context){
                .input = data,
                .bytes = 0,
                .context = list->context,
            });
        }
    }
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_singly_linked_list_validate(CCC_Singly_linked_list const *const list)
{
    if (!list)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t size = 0;
    for (struct CCC_Singly_linked_list_node *e = list->nil.n; e != &list->nil;
         e = e->n, ++size)
    {
        if (size >= list->count)
        {
            return CCC_FALSE;
        }
        if (!e || !e->n || e->n == e)
        {
            return CCC_FALSE;
        }
    }
    return size == list->count;
}

CCC_Count
CCC_singly_linked_list_count(CCC_Singly_linked_list const *const list)
{

    if (!list)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = list->count};
}

CCC_Tribool
CCC_singly_linked_list_is_empty(CCC_Singly_linked_list const *const list)
{
    if (!list)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !list->count;
}

/*==========================     Sorting     ================================*/

/** Returns true if the list is sorted in non-decreasing order. The user should
flip the return values of their comparison function if they want a different
order for elements.*/
CCC_Tribool
CCC_singly_linked_list_is_sorted(CCC_Singly_linked_list const *const list)
{
    if (!list)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (list->count <= 1)
    {
        return CCC_TRUE;
    }
    for (struct CCC_Singly_linked_list_node const *previous = list->nil.n,
                                                  *current = list->nil.n->n;
         current != &list->nil; previous = current, current = current->n)
    {
        if (order(list, previous, current) == CCC_ORDER_GREATER)
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
CCC_singly_linked_list_insert_sorted(CCC_Singly_linked_list *list,
                                     CCC_Singly_linked_list_node *type_intruder)
{
    if (!list || !type_intruder)
    {
        return NULL;
    }
    if (list->allocate)
    {
        void *const node = list->allocate((CCC_Allocator_context){
            .input = NULL,
            .bytes = list->sizeof_type,
            .context = list->context,
        });
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(list, type_intruder), list->sizeof_type);
        type_intruder = elem_in(list, node);
    }
    struct CCC_Singly_linked_list_node *prev = &list->nil;
    struct CCC_Singly_linked_list_node *i = list->nil.n;
    for (; i != &list->nil && order(list, type_intruder, i) != CCC_ORDER_LESSER;
         prev = i, i = i->n)
    {}
    type_intruder->n = i;
    prev->n = type_intruder;
    ++list->count;
    return struct_base(list, type_intruder);
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
CCC_singly_linked_list_sort(CCC_Singly_linked_list *const list)
{
    if (!list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* Algorithm is one pass if list is sorted. Merging is never true. */
    CCC_Tribool merging = CCC_FALSE;
    do
    {
        merging = CCC_FALSE;
        /* 0th index of the A list. The start of one list to merge. */
        struct Link a_first = {.previous = &list->nil, .current = list->nil.n};
        while (a_first.current != &list->nil)
        {
            /* The Nth index of list A (its size) aka 0th index of B list. */
            struct Link a_count_b_first = first_less(list, a_first);
            if (a_count_b_first.current == &list->nil)
            {
                break;
            }
            /* A picks up the exclusive end of this merge, B, in order
               to progress the sorting algorithm with the next run that needs
               fixing. Merge returns the final B element to indicate it is the
               final sentinel that has not yet been examined. */
            a_first = merge(list, a_first, a_count_b_first,
                            first_less(list, a_count_b_first));
            merging = CCC_TRUE;
        }
    }
    while (merging);
    return CCC_RESULT_OK;
}

/** Merges lists `[a_first, a_count_b_first)` with `[a_count_b_first, b_count)`
to form `[a_first, b_count)`. Returns the exclusive end of the range, `b_count`,
once the merge sort is complete.

Notice that all ranges treat the end of their range as an exclusive sentinel for
consistency. This function assumes the provided lists are already sorted
separately. A list link must be returned because the `b_count` previous field
may be updated due to arbitrary splices during comparison sorting. */
static inline struct Link
merge(CCC_Singly_linked_list *const list, struct Link a_first,
      struct Link a_count_b_first, struct Link b_count)
{
    while (a_first.current != a_count_b_first.current
           && a_count_b_first.current != b_count.current)
    {
        if (order(list, a_count_b_first.current, a_first.current)
            == CCC_ORDER_LESSER)
        {
            /* The current element is the lesser element that must be spliced
               out. However, a_count_b_first.previous is not updated because
               only current is spliced out. Algorithm will continue with new
               current, but same previous. */
            struct CCC_Singly_linked_list_node *const lesser
                = a_count_b_first.current;
            a_count_b_first.current = lesser->n;
            a_count_b_first.previous->n = lesser->n;
            /* This is so we return an accurate b_count list link at the end. */
            if (lesser == b_count.previous)
            {
                b_count.previous = a_count_b_first.previous;
            }
            a_first.previous->n = lesser;
            lesser->n = a_first.current;
            /* Another critical update reflected in our links, not the list. */
            a_first.previous = lesser;
        }
        else
        {
            a_first.previous = a_first.current;
            a_first.current = a_first.current->n;
        }
    }
    return b_count;
}

/** Returns a pair of elements marking the first list elem that is smaller than
its previous `CCC_ORDER_LESSER` according to the user comparison callback. The
list_link returned will have the out of order element as cur and the last
remaining in order element as prev. The cur element may be the sentinel if the
run is sorted. */
static inline struct Link
first_less(CCC_Singly_linked_list const *const list, struct Link link)
{
    do
    {
        link.previous = link.current;
        link.current = link.current->n;
    }
    while (link.current != &list->nil
           && order(list, link.current, link.previous) != CCC_ORDER_LESSER);
    return link;
}

/*=========================    Private Interface   ==========================*/

void
CCC_private_singly_linked_list_push_front(
    struct CCC_Singly_linked_list *const list,
    struct CCC_Singly_linked_list_node *const type_intruder)
{
    push_front(list, type_intruder);
}

/*===========================  Static Helpers   =============================*/

static inline void
push_front(struct CCC_Singly_linked_list *const list,
           struct CCC_Singly_linked_list_node *const node)
{
    node->n = list->nil.n;
    list->nil.n = node;
    ++list->count;
}

static inline struct CCC_Singly_linked_list_node *
pop_front(struct CCC_Singly_linked_list *const list)
{
    struct CCC_Singly_linked_list_node *const remove = list->nil.n;
    list->nil.n = remove->n;
    if (remove != &list->nil)
    {
        remove->n = NULL;
    }
    --list->count;
    return remove;
}

static inline struct CCC_Singly_linked_list_node *
before(struct CCC_Singly_linked_list const *const list,
       struct CCC_Singly_linked_list_node const *const to_find)
{
    struct CCC_Singly_linked_list_node const *i = &list->nil;
    for (; i->n != to_find; i = i->n)
    {}
    return (struct CCC_Singly_linked_list_node *)i;
}

static inline size_t
extract_range(struct CCC_Singly_linked_list *const list,
              struct CCC_Singly_linked_list_node *begin,
              struct CCC_Singly_linked_list_node *const end)
{
    size_t const count = len(begin, end);
    if (end != &list->nil)
    {
        end->n = NULL;
    }
    return count;
}

static size_t
erase_range(struct CCC_Singly_linked_list *const list,
            struct CCC_Singly_linked_list_node const *begin,
            struct CCC_Singly_linked_list_node *const end)
{
    if (!list->allocate)
    {
        size_t const count = len(begin, end);
        if (end != &list->nil)
        {
            end->n = NULL;
        }
        return count;
    }
    size_t count = 1;
    for (struct CCC_Singly_linked_list_node const *next = NULL; begin != end;
         begin = next, ++count)
    {
        assert(count <= list->count);
        next = begin->n;
        (void)list->allocate((CCC_Allocator_context){
            .input = struct_base(list, begin),
            .bytes = 0,
            .context = list->context,
        });
    }
    (void)list->allocate((CCC_Allocator_context){
        .input = struct_base(list, end),
        .bytes = 0,
        .context = list->context,
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
struct_base(struct CCC_Singly_linked_list const *const list,
            struct CCC_Singly_linked_list_node const *const node)
{
    return ((char *)&node->n) - list->singly_linked_list_node_offset;
}

/** Given the user struct provides the address of intrusive elem. */
static inline struct CCC_Singly_linked_list_node *
elem_in(struct CCC_Singly_linked_list const *const list,
        void const *const any_struct)
{
    return (struct CCC_Singly_linked_list_node
                *)((char *)any_struct + list->singly_linked_list_node_offset);
}

/** Calls the user provided three way comparison callback function on the user
type wrapping the provided intrusive handles. Returns the three way comparison
result value. */
static inline CCC_Order
order(struct CCC_Singly_linked_list const *const list,
      struct CCC_Singly_linked_list_node const *const left,
      struct CCC_Singly_linked_list_node const *const right)
{
    return list->compare((CCC_Type_comparator_context){
        .type_left = struct_base(list, left),
        .type_right = struct_base(list, right),
        .context = list->context,
    });
}
