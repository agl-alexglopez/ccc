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
Code in the pintOS source is at  `source/lib/kernel.list.c`, but this may change
if they refactor. */
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "doubly_linked_list.h"
#include "private/private_doubly_linked_list.h"
#include "types.h"

/*===========================   Prototypes    ===============================*/

static void *struct_base(struct CCC_Doubly_linked_list const *,
                         struct CCC_Doubly_linked_list_node const *);
static size_t extract_range(struct CCC_Doubly_linked_list const *,
                            struct CCC_Doubly_linked_list_node *,
                            struct CCC_Doubly_linked_list_node *);
static size_t erase_range(struct CCC_Doubly_linked_list const *,
                          struct CCC_Doubly_linked_list_node *,
                          struct CCC_Doubly_linked_list_node *);
static size_t len(struct CCC_Doubly_linked_list_node const *,
                  struct CCC_Doubly_linked_list_node const *);
static struct CCC_Doubly_linked_list_node *
pop_front(struct CCC_Doubly_linked_list *);
static void push_front(struct CCC_Doubly_linked_list *,
                       struct CCC_Doubly_linked_list_node *);
static struct CCC_Doubly_linked_list_node *
type_intruder_in(CCC_Doubly_linked_list const *, void const *);
static void push_back(CCC_Doubly_linked_list *,
                      struct CCC_Doubly_linked_list_node *);
static void splice_range(struct CCC_Doubly_linked_list_node *,
                         struct CCC_Doubly_linked_list_node *,
                         struct CCC_Doubly_linked_list_node *);
static struct CCC_Doubly_linked_list_node *
first_less(struct CCC_Doubly_linked_list const *,
           struct CCC_Doubly_linked_list_node *);
static struct CCC_Doubly_linked_list_node *
merge(struct CCC_Doubly_linked_list *, struct CCC_Doubly_linked_list_node *,
      struct CCC_Doubly_linked_list_node *,
      struct CCC_Doubly_linked_list_node *);
static CCC_Order order(struct CCC_Doubly_linked_list const *,
                       struct CCC_Doubly_linked_list_node const *,
                       struct CCC_Doubly_linked_list_node const *);

/*===========================     Interface   ===============================*/

void *
CCC_doubly_linked_list_push_front(CCC_Doubly_linked_list *const list,
                                  CCC_Doubly_linked_list_node *type_intruder)
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
        type_intruder = type_intruder_in(list, node);
    }
    push_front(list, type_intruder);
    return struct_base(list, list->nil.next);
}

void *
CCC_doubly_linked_list_push_back(CCC_Doubly_linked_list *const list,
                                 CCC_Doubly_linked_list_node *type_intruder)
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
        type_intruder = type_intruder_in(list, node);
    }
    push_back(list, type_intruder);
    return struct_base(list, list->nil.previous);
}

void *
CCC_doubly_linked_list_front(CCC_Doubly_linked_list const *list)
{
    if (!list || !list->count)
    {
        return NULL;
    }
    return struct_base(list, list->nil.next);
}

void *
CCC_doubly_linked_list_back(CCC_Doubly_linked_list const *list)
{
    if (!list || !list->count)
    {
        return NULL;
    }
    return struct_base(list, list->nil.previous);
}

CCC_Result
CCC_doubly_linked_list_pop_front(CCC_Doubly_linked_list *const list)
{
    if (!list || !list->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Doubly_linked_list_node *remove = pop_front(list);
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
CCC_doubly_linked_list_pop_back(CCC_Doubly_linked_list *const list)
{
    if (!list || !list->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Doubly_linked_list_node *remove = list->nil.previous;
    remove->previous->next = &list->nil;
    list->nil.previous = remove->previous;
    if (remove != &list->nil)
    {
        remove->next = remove->previous = NULL;
    }
    if (list->allocate)
    {
        (void)list->allocate((CCC_Allocator_context){
            .input = struct_base(list, remove),
            .bytes = 0,
            .context = list->context,
        });
    }
    --list->count;
    return CCC_RESULT_OK;
}

void *
CCC_doubly_linked_list_insert(CCC_Doubly_linked_list *const list,
                              CCC_Doubly_linked_list_node *const position,
                              CCC_Doubly_linked_list_node *type_intruder)
{
    if (!list || !position->next || !position->previous)
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
        type_intruder = type_intruder_in(list, node);
    }
    type_intruder->next = position;
    type_intruder->previous = position->previous;

    position->previous->next = type_intruder;
    position->previous = type_intruder;
    ++list->count;
    return struct_base(list, type_intruder);
}

void *
CCC_doubly_linked_list_erase(CCC_Doubly_linked_list *const list,
                             CCC_Doubly_linked_list_node *const type_intruder)
{
    if (!list || !type_intruder || !list->count)
    {
        return NULL;
    }
    type_intruder->next->previous = type_intruder->previous;
    type_intruder->previous->next = type_intruder->next;
    void *const ret = type_intruder->next == &list->nil
                        ? NULL
                        : struct_base(list, type_intruder);
    if (list->allocate)
    {
        (void)list->allocate((CCC_Allocator_context){
            .input = struct_base(list, type_intruder),
            .bytes = 0,
            .context = list->context,
        });
    }
    --list->count;
    return ret;
}

void *
CCC_doubly_linked_list_erase_range(
    CCC_Doubly_linked_list *const list,
    CCC_Doubly_linked_list_node *const type_intruder_begin,
    CCC_Doubly_linked_list_node *type_intruder_end)
{
    if (!list || !type_intruder_begin || !type_intruder_end
        || !type_intruder_begin->next || !type_intruder_begin->previous
        || !type_intruder_end->next || !type_intruder_end->previous
        || !list->count)
    {
        return NULL;
    }
    if (type_intruder_begin == type_intruder_end)
    {
        return CCC_doubly_linked_list_erase(list, type_intruder_begin);
    }
    struct CCC_Doubly_linked_list_node const *const ret = type_intruder_end;
    type_intruder_end = type_intruder_end->previous;
    type_intruder_end->next->previous = type_intruder_begin->previous;
    type_intruder_begin->previous->next = type_intruder_end->next;

    size_t const deleted
        = erase_range(list, type_intruder_begin, type_intruder_end);

    assert(deleted <= list->count);
    list->count -= deleted;
    return ret == &list->nil ? NULL : struct_base(list, ret);
}

CCC_Doubly_linked_list_node *
CCC_doubly_linked_list_node_begin(CCC_Doubly_linked_list const *const list)
{
    return list ? list->nil.next : NULL;
}

CCC_Doubly_linked_list_node *
CCC_doubly_linked_list_node_end(CCC_Doubly_linked_list const *const list)
{
    return list ? list->nil.previous : NULL;
}

CCC_Doubly_linked_list_node *
CCC_doubly_linked_list_sentinel_end(CCC_Doubly_linked_list const *const list)
{
    return list ? (CCC_Doubly_linked_list_node *)&list->nil : NULL;
}

void *
CCC_doubly_linked_list_extract(CCC_Doubly_linked_list *const list,
                               CCC_Doubly_linked_list_node *const type_intruder)
{
    if (!list || !type_intruder || !type_intruder->next
        || !type_intruder->previous || !list->count)
    {
        return NULL;
    }
    struct CCC_Doubly_linked_list_node const *const ret = type_intruder->next;
    type_intruder->next->previous = type_intruder->previous;
    type_intruder->previous->next = type_intruder->next;
    if (type_intruder != &list->nil)
    {
        type_intruder->next = type_intruder->previous = NULL;
    }
    --list->count;
    return ret == &list->nil ? NULL : struct_base(list, ret);
}

void *
CCC_doubly_linked_list_extract_range(
    CCC_Doubly_linked_list *const list,
    CCC_Doubly_linked_list_node *const type_intruder_begin,
    CCC_Doubly_linked_list_node *type_intruder_end)
{
    if (!list || !type_intruder_begin || !type_intruder_end
        || !type_intruder_begin->next || !type_intruder_begin->previous
        || !type_intruder_end->next || !type_intruder_end->previous
        || !list->count)
    {
        return NULL;
    }
    if (type_intruder_begin == type_intruder_end)
    {
        return CCC_doubly_linked_list_extract(list, type_intruder_begin);
    }
    struct CCC_Doubly_linked_list_node const *const ret = type_intruder_end;
    type_intruder_end = type_intruder_end->previous;
    type_intruder_end->next->previous = type_intruder_begin->previous;
    type_intruder_begin->previous->next = type_intruder_end->next;

    size_t const deleted
        = extract_range(list, type_intruder_begin, type_intruder_end);

    assert(deleted <= list->count);
    list->count -= deleted;
    return ret == &list->nil ? NULL : struct_base(list, ret);
}

CCC_Result
CCC_doubly_linked_list_splice(
    CCC_Doubly_linked_list *const position_doubly_linked_list,
    CCC_Doubly_linked_list_node *position,
    CCC_Doubly_linked_list *const to_cut_doubly_linked_list,
    CCC_Doubly_linked_list_node *const to_cut)
{
    if (!to_cut || !position || !position_doubly_linked_list
        || !to_cut_doubly_linked_list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (position == to_cut || to_cut->next == position
        || to_cut == &to_cut_doubly_linked_list->nil)
    {
        return CCC_RESULT_OK;
    }
    to_cut->next->previous = to_cut->previous;
    to_cut->previous->next = to_cut->next;

    to_cut->previous = position->previous;
    to_cut->next = position;
    position->previous->next = to_cut;
    position->previous = to_cut;
    if (position_doubly_linked_list != to_cut_doubly_linked_list)
    {
        ++position_doubly_linked_list->count;
        --to_cut_doubly_linked_list->count;
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_doubly_linked_list_splice_range(
    CCC_Doubly_linked_list *const position_doubly_linked_list,
    CCC_Doubly_linked_list_node *type_intruder_position,
    CCC_Doubly_linked_list *const to_cut_doubly_linked_list,
    CCC_Doubly_linked_list_node *type_intruder_to_cut_begin,
    CCC_Doubly_linked_list_node *type_intruder_to_cut_end)
{
    if (!type_intruder_to_cut_begin || !type_intruder_to_cut_end
        || !type_intruder_position || !position_doubly_linked_list
        || !to_cut_doubly_linked_list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (type_intruder_position == type_intruder_to_cut_begin
        || type_intruder_position == type_intruder_to_cut_end
        || type_intruder_to_cut_begin == type_intruder_to_cut_end
        || type_intruder_to_cut_begin == &to_cut_doubly_linked_list->nil
        || type_intruder_to_cut_end->previous
               == &to_cut_doubly_linked_list->nil)
    {
        return CCC_RESULT_OK;
    }
    struct CCC_Doubly_linked_list_node *const inclusive_end
        = type_intruder_to_cut_end->previous;
    splice_range(type_intruder_position, type_intruder_to_cut_begin,
                 type_intruder_to_cut_end);
    if (position_doubly_linked_list != to_cut_doubly_linked_list)
    {
        size_t const count = len(type_intruder_to_cut_begin, inclusive_end);
        assert(count <= to_cut_doubly_linked_list->count);
        position_doubly_linked_list->count += count;
        to_cut_doubly_linked_list->count -= count;
    }
    return CCC_RESULT_OK;
}

void *
CCC_doubly_linked_list_begin(CCC_Doubly_linked_list const *const list)
{
    if (!list || list->nil.next == &list->nil)
    {
        return NULL;
    }
    return struct_base(list, list->nil.next);
}

void *
CCC_doubly_linked_list_reverse_begin(CCC_Doubly_linked_list const *const list)
{
    if (!list || list->nil.previous == &list->nil)
    {
        return NULL;
    }
    return struct_base(list, list->nil.previous);
}

void *
CCC_doubly_linked_list_end(CCC_Doubly_linked_list const *const)
{
    return NULL;
}

void *
CCC_doubly_linked_list_reverse_end(CCC_Doubly_linked_list const *const)
{
    return NULL;
}

void *
CCC_doubly_linked_list_next(CCC_Doubly_linked_list const *const list,
                            CCC_Doubly_linked_list_node const *type_intruder)
{
    if (!list || !type_intruder || type_intruder->next == &list->nil)
    {
        return NULL;
    }
    return struct_base(list, type_intruder->next);
}

void *
CCC_doubly_linked_list_reverse_next(
    CCC_Doubly_linked_list const *const list,
    CCC_Doubly_linked_list_node const *type_intruder)
{
    if (!list || !type_intruder || type_intruder->previous == &list->nil)
    {
        return NULL;
    }
    return struct_base(list, type_intruder->previous);
}

CCC_Count
CCC_doubly_linked_list_count(CCC_Doubly_linked_list const *const list)
{
    if (!list)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = list->count};
}

CCC_Tribool
CCC_doubly_linked_list_is_empty(CCC_Doubly_linked_list const *const list)
{
    return list ? !list->count : CCC_TRUE;
}

CCC_Result
CCC_doubly_linked_list_clear(CCC_Doubly_linked_list *const list,
                             CCC_Type_destructor *destroy)
{
    if (!list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    while (!CCC_doubly_linked_list_is_empty(list))
    {
        void *const node = struct_base(list, pop_front(list));
        if (destroy)
        {
            destroy((CCC_Type_context){.type = node, .context = list->context});
        }
        if (list->allocate)
        {
            (void)list->allocate((CCC_Allocator_context){
                .input = node,
                .bytes = 0,
                .context = list->context,
            });
        }
    }
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_doubly_linked_list_validate(CCC_Doubly_linked_list const *const list)
{
    if (!list)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t size = 0;
    for (struct CCC_Doubly_linked_list_node const *e = list->nil.next;
         e != &list->nil; e = e->next, ++size)
    {
        if (size >= list->count)
        {
            return CCC_FALSE;
        }
        if (!e || !e->next || !e->previous || e->next == e || e->previous == e)
        {
            return CCC_FALSE;
        }
    }
    return size == list->count;
}

/*==========================     Sorting     ================================*/

/** Returns true if the list is sorted in non-decreasing order. The user should
flip the return values of their comparison function if they want a different
order for elements.*/
CCC_Tribool
CCC_doubly_linked_list_is_sorted(CCC_Doubly_linked_list const *const list)
{
    if (!list)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (list->count <= 1)
    {
        return CCC_TRUE;
    }
    for (struct CCC_Doubly_linked_list_node const *cur = list->nil.next->next;
         cur != &list->nil; cur = cur->next)
    {
        if (order(list, cur->previous, cur) == CCC_ORDER_GREATER)
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
CCC_doubly_linked_list_insert_sorted(CCC_Doubly_linked_list *const list,
                                     CCC_Doubly_linked_list_node *type_intruder)
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
        type_intruder = type_intruder_in(list, node);
    }
    struct CCC_Doubly_linked_list_node *pos = list->nil.next;
    for (; pos != &list->nil
           && order(list, type_intruder, pos) != CCC_ORDER_LESSER;
         pos = pos->next)
    {}
    type_intruder->next = pos;
    type_intruder->previous = pos->previous;
    pos->previous->next = type_intruder;
    pos->previous = type_intruder;
    ++list->count;
    return struct_base(list, type_intruder);
}

/** Sorts the list into non-decreasing order according to the user comparison
callback function in `O(N * log(N))` time and `O(1)` space.

The following merging algorithm and associated helper functions are based on
the iterative natural merge sort used in the list module of the pintOS project
for learning operating systems. Currently the original is located at the
following path in the pintOS source code:

`src/lib/kernel/list.c`

However, if refactors change this location, seek the list intrusive container
module for original implementations. The code has been changed for the C
Container Collection as follows:

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
CCC_doubly_linked_list_sort(CCC_Doubly_linked_list *const list)
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
        struct CCC_Doubly_linked_list_node *a_first = list->nil.next;
        while (a_first != &list->nil)
        {
            /* The Nth index of list A (its size) aka 0th index of B list. */
            struct CCC_Doubly_linked_list_node *const a_count_b_first
                = first_less(list, a_first);
            if (a_count_b_first == &list->nil)
            {
                break;
            }
            /* A picks up the exclusive end of this merge, B, in order
               to progress the sorting algorithm with the next run that needs
               fixing. Merge returns the end of B to indicate it is the final
               sentinel node yet to be examined. */
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
separately. */
static inline struct CCC_Doubly_linked_list_node *
merge(struct CCC_Doubly_linked_list *const list,
      struct CCC_Doubly_linked_list_node *a_first,
      struct CCC_Doubly_linked_list_node *a_count_b_first,
      struct CCC_Doubly_linked_list_node *const b_count)
{
    assert(list && a_first && a_count_b_first && b_count);
    while (a_first != a_count_b_first && a_count_b_first != b_count)
    {
        if (order(list, a_count_b_first, a_first) == CCC_ORDER_LESSER)
        {
            struct CCC_Doubly_linked_list_node *const lesser = a_count_b_first;
            a_count_b_first = lesser->next;
            lesser->next->previous = lesser->previous;
            lesser->previous->next = lesser->next;
            lesser->previous = a_first->previous;
            lesser->next = a_first;
            a_first->previous->next = lesser;
            a_first->previous = lesser;
        }
        else
        {
            a_first = a_first->next;
        }
    }
    return b_count;
}

/** Finds the first element lesser than it's previous element as defined by
the user comparison callback function. If no out of order element can be
found the list sentinel is returned. */
static inline struct CCC_Doubly_linked_list_node *
first_less(struct CCC_Doubly_linked_list const *const list,
           struct CCC_Doubly_linked_list_node *start)
{
    assert(list && start);
    do
    {
        start = start->next;
    }
    while (start != &list->nil
           && order(list, start, start->previous) != CCC_ORDER_LESSER);
    return start;
}

/*=======================     Private Interface   ===========================*/

void
CCC_private_doubly_linked_list_push_back(
    struct CCC_Doubly_linked_list *const list,
    struct CCC_Doubly_linked_list_node *const type_intruder)
{
    if (!list || !type_intruder)
    {
        return;
    }
    push_back(list, type_intruder);
}

void
CCC_private_doubly_linked_list_push_front(
    struct CCC_Doubly_linked_list *const list,
    struct CCC_Doubly_linked_list_node *const type_intruder)
{
    if (!list || !type_intruder)
    {
        return;
    }
    push_front(list, type_intruder);
}

struct CCC_Doubly_linked_list_node *
CCC_private_doubly_linked_list_node_in(
    struct CCC_Doubly_linked_list const *const list,
    void const *const any_struct)
{
    return type_intruder_in(list, any_struct);
}

/*=======================       Static Helpers    ===========================*/

/** Places the range `[begin, end)` at position before pos. This means end is
not moved or altered due to the exclusive range. If begin is equal to end the
function returns early changing no nodes. */
static inline void
splice_range(struct CCC_Doubly_linked_list_node *const position,
             struct CCC_Doubly_linked_list_node *const begin,
             struct CCC_Doubly_linked_list_node *end)
{
    if (begin == end)
    {
        return;
    }
    end = end->previous;
    end->next->previous = begin->previous;
    begin->previous->next = end->next;

    begin->previous = position->previous;
    end->next = position;
    position->previous->next = begin;
    position->previous = end;
}

static inline void
push_front(struct CCC_Doubly_linked_list *const list,
           struct CCC_Doubly_linked_list_node *const node)
{
    node->previous = &list->nil;
    node->next = list->nil.next;
    list->nil.next->previous = node;
    list->nil.next = node;
    ++list->count;
}

static inline void
push_back(struct CCC_Doubly_linked_list *const list,
          struct CCC_Doubly_linked_list_node *const node)
{
    node->next = &list->nil;
    node->previous = list->nil.previous;
    list->nil.previous->next = node;
    list->nil.previous = node;
    ++list->count;
}

static inline struct CCC_Doubly_linked_list_node *
pop_front(struct CCC_Doubly_linked_list *const list)
{
    struct CCC_Doubly_linked_list_node *const ret = list->nil.next;
    ret->next->previous = &list->nil;
    list->nil.next = ret->next;
    if (ret != &list->nil)
    {
        ret->next = ret->previous = NULL;
    }
    --list->count;
    return ret;
}

static inline size_t
extract_range(struct CCC_Doubly_linked_list const *const list,
              struct CCC_Doubly_linked_list_node *begin,
              struct CCC_Doubly_linked_list_node *const end)
{
    if (begin != &list->nil)
    {
        begin->previous = NULL;
    }
    size_t const count = len(begin, end);
    if (end != &list->nil)
    {
        end->next = NULL;
    }
    return count;
}

static size_t
erase_range(struct CCC_Doubly_linked_list const *const list,
            struct CCC_Doubly_linked_list_node *begin,
            struct CCC_Doubly_linked_list_node *const end)
{
    if (begin != &list->nil)
    {
        begin->previous = NULL;
    }
    if (!list->allocate)
    {
        size_t const count = len(begin, end);
        if (end != &list->nil)
        {
            end->next = NULL;
        }
        return count;
    }
    size_t count = 1;
    for (; begin != end; ++count)
    {
        assert(count <= list->count);
        struct CCC_Doubly_linked_list_node *const next = begin->next;
        (void)list->allocate((CCC_Allocator_context){
            .input = struct_base(list, begin),
            .bytes = 0,
            .context = list->context,
        });
        begin = next;
    }
    (void)list->allocate((CCC_Allocator_context){
        .input = struct_base(list, end),
        .bytes = 0,
        .context = list->context,
    });
    return count;
}

/** Finds the length from [begin, end]. End is counted. */
static size_t
len(struct CCC_Doubly_linked_list_node const *begin,
    struct CCC_Doubly_linked_list_node const *const end)
{
    size_t s = 1;
    for (; begin != end; begin = begin->next, ++s)
    {}
    return s;
}

static inline void *
struct_base(struct CCC_Doubly_linked_list const *const list,
            struct CCC_Doubly_linked_list_node const *const node)
{
    return ((char *)&node->next) - list->doubly_linked_list_node_offset;
}

static inline struct CCC_Doubly_linked_list_node *
type_intruder_in(struct CCC_Doubly_linked_list const *const list,
                 void const *const struct_base)
{
    return (struct CCC_Doubly_linked_list_node
                *)((char *)struct_base + list->doubly_linked_list_node_offset);
}

/** Calls the user provided three way comparison callback function on the user
type wrapping the provided intrusive handles. Returns the three way comparison
result value. */
static inline CCC_Order
order(struct CCC_Doubly_linked_list const *const list,
      struct CCC_Doubly_linked_list_node const *const left,
      struct CCC_Doubly_linked_list_node const *const right)
{
    return list->compare((CCC_Type_comparator_context){
        .type_left = struct_base(list, left),
        .type_right = struct_base(list, right),
        .context = list->context,
    });
}
