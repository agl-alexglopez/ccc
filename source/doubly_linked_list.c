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

static struct CCC_Doubly_linked_list_node *
push_back(struct CCC_Doubly_linked_list *,
          struct CCC_Doubly_linked_list_node *);
static struct CCC_Doubly_linked_list_node *
push_front(struct CCC_Doubly_linked_list *,
           struct CCC_Doubly_linked_list_node *);
static struct CCC_Doubly_linked_list_node *
remove_node(struct CCC_Doubly_linked_list *,
            struct CCC_Doubly_linked_list_node *);
static inline struct CCC_Doubly_linked_list_node *
insert_node(struct CCC_Doubly_linked_list *,
            struct CCC_Doubly_linked_list_node *,
            struct CCC_Doubly_linked_list_node *);
static void *struct_base(struct CCC_Doubly_linked_list const *,
                         struct CCC_Doubly_linked_list_node const *);
static size_t erase_range(struct CCC_Doubly_linked_list const *,
                          struct CCC_Doubly_linked_list_node *,
                          struct CCC_Doubly_linked_list_node *);
static size_t len(struct CCC_Doubly_linked_list_node const *,
                  struct CCC_Doubly_linked_list_node const *);
static struct CCC_Doubly_linked_list_node *
type_intruder_in(CCC_Doubly_linked_list const *, void const *);
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
        void *const copy = list->allocate((CCC_Allocator_context){
            .input = NULL,
            .bytes = list->sizeof_type,
            .context = list->context,
        });
        if (!copy)
        {
            return NULL;
        }
        memcpy(copy, struct_base(list, type_intruder), list->sizeof_type);
        type_intruder = type_intruder_in(list, copy);
    }

    type_intruder = push_front(list, type_intruder);
    ++list->count;
    return struct_base(list, type_intruder);
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
    type_intruder = push_back(list, type_intruder);
    ++list->count;
    return struct_base(list, type_intruder);
}

void *
CCC_doubly_linked_list_front(CCC_Doubly_linked_list const *list)
{
    if (!list)
    {
        return NULL;
    }
    return struct_base(list, list->head);
}

void *
CCC_doubly_linked_list_back(CCC_Doubly_linked_list const *list)
{
    if (!list)
    {
        return NULL;
    }
    return struct_base(list, list->tail);
}

CCC_Result
CCC_doubly_linked_list_pop_front(CCC_Doubly_linked_list *const list)
{
    if (!list || !list->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Doubly_linked_list_node *const r = remove_node(list, list->head);
    if (list->allocate)
    {
        assert(r);
        (void)list->allocate((CCC_Allocator_context){
            .input = struct_base(list, r),
            .bytes = 0,
            .context = list->context,
        });
    }
    --list->count;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_doubly_linked_list_pop_back(CCC_Doubly_linked_list *const list)
{
    if (!list || !list->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Doubly_linked_list_node *const r = remove_node(list, list->tail);
    if (list->allocate)
    {
        (void)list->allocate((CCC_Allocator_context){
            .input = struct_base(list, r),
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
    if (!list)
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
    type_intruder = insert_node(list, position, type_intruder);
    ++list->count;
    return struct_base(list, type_intruder);
}

void *
CCC_doubly_linked_list_erase(CCC_Doubly_linked_list *const list,
                             CCC_Doubly_linked_list_node *type_intruder)
{
    if (!list || !type_intruder || !list->count)
    {
        return NULL;
    }
    void *const ret = struct_base(list, type_intruder->next);
    type_intruder = remove_node(list, type_intruder);
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
    if (!list || list->count == 0 || !type_intruder_begin || !type_intruder_end)
    {
        return NULL;
    }

    if (type_intruder_begin == type_intruder_end)
    {
        return CCC_doubly_linked_list_erase(list, type_intruder_begin);
    }

    CCC_Doubly_linked_list_node *const previous = type_intruder_begin->previous;
    CCC_Doubly_linked_list_node *const next = type_intruder_end->next;

    if (previous)
    {
        previous->next = next;
    }
    else
    {
        list->head = next;
    }

    if (next)
    {
        next->previous = previous;
    }
    else
    {
        list->tail = previous;
    }

    size_t deleted = erase_range(list, type_intruder_begin, type_intruder_end);

    assert(deleted <= list->count);
    list->count -= deleted;

    return struct_base(list, next);
}

CCC_Doubly_linked_list_node *
CCC_doubly_linked_list_node_begin(CCC_Doubly_linked_list const *const list)
{
    return list ? list->head : NULL;
}

void *
CCC_doubly_linked_list_extract(CCC_Doubly_linked_list *const list,
                               CCC_Doubly_linked_list_node *type_intruder)
{
    if (!list || !type_intruder)
    {
        return NULL;
    }
    type_intruder = remove_node(list, type_intruder);
    --list->count;
    return struct_base(list, type_intruder);
}

void *
CCC_doubly_linked_list_extract_range(
    CCC_Doubly_linked_list *const list,
    CCC_Doubly_linked_list_node *type_intruder_begin,
    CCC_Doubly_linked_list_node *type_intruder_end)
{
    if (!list || !list->count || !type_intruder_begin
        || type_intruder_begin == type_intruder_end)
    {
        return NULL;
    }
    /* We extract exclusive ranges [start, end). Step back one. */
    if (type_intruder_end)
    {
        type_intruder_end = type_intruder_end->previous;
    }
    if (type_intruder_begin == type_intruder_end)
    {
        type_intruder_begin = remove_node(list, type_intruder_begin);
        --list->count;
        return struct_base(list, type_intruder_begin);
    }

    CCC_Doubly_linked_list_node *const previous = type_intruder_begin->previous;
    CCC_Doubly_linked_list_node *const next
        = type_intruder_end ? type_intruder_end->next : NULL;

    if (previous)
    {
        previous->next = next;
    }
    else
    {
        list->head = next;
    }

    if (next)
    {
        next->previous = previous;
    }
    else
    {
        list->tail = previous;
    }

    type_intruder_begin->previous = NULL;
    if (type_intruder_end)
    {
        type_intruder_end->next = NULL;
    }

    size_t removed = 1;
    CCC_Doubly_linked_list_node const *p = type_intruder_begin;
    while (p->next && p != type_intruder_end)
    {
        removed++;
        p = p->next;
    }
    list->count -= removed;
    return struct_base(list, next);
}

CCC_Result
CCC_doubly_linked_list_splice(
    CCC_Doubly_linked_list *const position_doubly_linked_list,
    CCC_Doubly_linked_list_node *position,
    CCC_Doubly_linked_list *const to_cut_doubly_linked_list,
    CCC_Doubly_linked_list_node *to_cut)
{
    if (!position_doubly_linked_list || !to_cut_doubly_linked_list || !to_cut)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if ((to_cut_doubly_linked_list == position_doubly_linked_list)
        && (to_cut == position || to_cut->next == position))
    {
        return CCC_RESULT_OK;
    }
    to_cut = remove_node(to_cut_doubly_linked_list, to_cut);
    (void)insert_node(position_doubly_linked_list, position, to_cut);
    if (to_cut_doubly_linked_list != position_doubly_linked_list)
    {
        to_cut_doubly_linked_list->count--;
        position_doubly_linked_list->count++;
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_doubly_linked_list_splice_range(
    CCC_Doubly_linked_list *const position_doubly_linked_list,
    CCC_Doubly_linked_list_node *const type_intruder_position,
    CCC_Doubly_linked_list *const to_cut_doubly_linked_list,
    CCC_Doubly_linked_list_node *const type_intruder_to_cut_begin,
    CCC_Doubly_linked_list_node *const type_intruder_to_cut_exclusive_end)
{
    if (!position_doubly_linked_list || !to_cut_doubly_linked_list
        || !type_intruder_to_cut_begin
        || type_intruder_to_cut_begin == type_intruder_to_cut_exclusive_end)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }

    CCC_Doubly_linked_list_node *const to_cut_inclusive_end
        = type_intruder_to_cut_exclusive_end
            ? type_intruder_to_cut_exclusive_end->previous
            : to_cut_doubly_linked_list->tail;
    if (type_intruder_to_cut_begin == to_cut_inclusive_end)
    {
        return CCC_doubly_linked_list_splice(
            position_doubly_linked_list, type_intruder_position,
            to_cut_doubly_linked_list, type_intruder_to_cut_begin);
    }

    size_t count = 0;
    CCC_Doubly_linked_list_node *iterator = type_intruder_to_cut_begin;
    while (iterator != type_intruder_to_cut_exclusive_end)
    {
        iterator = iterator->next;
        count++;
    }

    CCC_Doubly_linked_list_node *to_cut_previous
        = type_intruder_to_cut_begin->previous;

    if (to_cut_previous)
    {
        to_cut_previous->next = type_intruder_to_cut_exclusive_end;
    }
    else
    {
        to_cut_doubly_linked_list->head = type_intruder_to_cut_exclusive_end;
    }

    if (type_intruder_to_cut_exclusive_end)
    {
        type_intruder_to_cut_exclusive_end->previous = to_cut_previous;
    }
    else
    {
        to_cut_doubly_linked_list->tail = to_cut_previous;
    }
    CCC_Doubly_linked_list_node *position_previous
        = type_intruder_position ? type_intruder_position->previous
                                 : position_doubly_linked_list->tail;

    if (position_previous == position_doubly_linked_list->tail)
    {
        position_doubly_linked_list->tail = to_cut_inclusive_end;
    }
    if (position_previous)
    {
        position_previous->next = type_intruder_to_cut_begin;
    }
    else
    {
        position_doubly_linked_list->head = type_intruder_to_cut_begin;
    }

    type_intruder_to_cut_begin->previous = position_previous;

    if (to_cut_inclusive_end)
    {
        to_cut_inclusive_end->next = type_intruder_position;
    }

    if (type_intruder_position)
    {
        type_intruder_position->previous = to_cut_inclusive_end;
    }

    if (to_cut_doubly_linked_list != position_doubly_linked_list)
    {
        to_cut_doubly_linked_list->count -= count;
        position_doubly_linked_list->count += count;
    }

    return CCC_RESULT_OK;
}

void *
CCC_doubly_linked_list_begin(CCC_Doubly_linked_list const *const list)
{
    if (!list || list->head == NULL)
    {
        return NULL;
    }
    return struct_base(list, list->head);
}

void *
CCC_doubly_linked_list_reverse_begin(CCC_Doubly_linked_list const *const list)
{
    if (!list || list->tail == NULL)
    {
        return NULL;
    }
    return struct_base(list, list->tail);
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
    if (!list || !type_intruder || type_intruder->next == NULL)
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
    if (!list || !type_intruder || type_intruder->previous == NULL)
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
                             CCC_Type_destructor *const destroy)
{
    if (!list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    while (list->head)
    {
        void *const node = struct_base(list, remove_node(list, list->head));
        if (destroy)
        {
            destroy((CCC_Type_context){
                .type = node,
                .context = list->context,
            });
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
    list->head = list->tail = NULL;
    list->count = 0;
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_doubly_linked_list_validate(CCC_Doubly_linked_list const *const list)
{
    if (!list)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (!list->head && !list->tail)
    {
        if (list->count != 0)
        {
            return CCC_FALSE;
        }
        return CCC_TRUE;
    }
    if (!list->head || !list->tail)
    {
        return CCC_FALSE;
    }
    size_t size = 0;
    struct CCC_Doubly_linked_list_node const *forward = list->head;
    struct CCC_Doubly_linked_list_node const *reverse = list->tail;
    while (forward && reverse && forward != list->tail && reverse != list->head)
    {
        if (size >= list->count)
        {
            return CCC_FALSE;
        }
        if (!forward || forward->next == forward
            || forward->previous == forward)
        {
            return CCC_FALSE;
        }
        if (!reverse || reverse->next == reverse
            || reverse->previous == reverse)
        {
            return CCC_FALSE;
        }
        forward = forward->next;
        reverse = reverse->previous;
        ++size;
    }
    size += (forward == list->tail && reverse == list->head);
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
    for (struct CCC_Doubly_linked_list_node const *cur = list->head->next;
         cur != NULL; cur = cur->next)
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
    struct CCC_Doubly_linked_list_node *pos = list->head;
    for (; pos != NULL && order(list, type_intruder, pos) != CCC_ORDER_LESSER;
         pos = pos->next)
    {}
    struct CCC_Doubly_linked_list_node *const pos_previous
        = pos ? pos->previous : list->tail;
    type_intruder->next = pos;
    type_intruder->previous = pos_previous;
    if (pos)
    {
        pos->previous = type_intruder;
    }
    if (pos_previous)
    {
        pos_previous->next = type_intruder;
    }
    else
    {
        list->head = type_intruder;
    }
    if (pos_previous == list->tail)
    {
        list->tail = type_intruder;
    }
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
        struct CCC_Doubly_linked_list_node *a_first = list->head;
        while (a_first != NULL)
        {
            /* The Nth index of list A (its size) aka 0th index of B list. */
            struct CCC_Doubly_linked_list_node *const a_count_b_first
                = first_less(list, a_first);
            if (a_count_b_first == NULL)
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
    while (a_first && a_first != a_count_b_first && a_count_b_first
           && a_count_b_first != b_count)
    {
        if (order(list, a_count_b_first, a_first) == CCC_ORDER_LESSER)
        {
            struct CCC_Doubly_linked_list_node *const lesser = a_count_b_first;
            a_count_b_first = lesser->next;
            if (lesser->next)
            {
                lesser->next->previous = lesser->previous;
            }
            else
            {
                list->tail = lesser->previous;
            }
            if (lesser->previous)
            {
                lesser->previous->next = lesser->next;
            }
            else
            {
                list->head = lesser->next;
            }
            lesser->previous = a_first->previous;
            lesser->next = a_first;
            if (a_first->previous)
            {
                a_first->previous->next = lesser;
            }
            else
            {
                list->head = lesser;
            }
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
    while (start != NULL
           && order(list, start, start->previous) != CCC_ORDER_LESSER);
    return start;
}

/*=======================     Private Interface   ===========================*/

void
CCC_private_doubly_linked_list_push_back(
    struct CCC_Doubly_linked_list *const list,
    struct CCC_Doubly_linked_list_node *type_intruder)
{
    (void)push_back(list, type_intruder);
    ++list->count;
}

void
CCC_private_doubly_linked_list_push_front(
    struct CCC_Doubly_linked_list *const list,
    struct CCC_Doubly_linked_list_node *const type_intruder)
{
    (void)push_front(list, type_intruder);
    ++list->count;
}

struct CCC_Doubly_linked_list_node *
CCC_private_doubly_linked_list_node_in(
    struct CCC_Doubly_linked_list const *const list,
    void const *const any_struct)
{
    return type_intruder_in(list, any_struct);
}

/*=======================       Static Helpers    ===========================*/

static inline struct CCC_Doubly_linked_list_node *
push_front(struct CCC_Doubly_linked_list *const list,
           struct CCC_Doubly_linked_list_node *const node)
{
    node->previous = NULL;
    node->next = list->head;

    if (list->head)
    {
        list->head->previous = node;
    }
    else
    {
        list->tail = node;
    }
    list->head = node;
    return node;
}

static inline struct CCC_Doubly_linked_list_node *
push_back(struct CCC_Doubly_linked_list *const list,
          struct CCC_Doubly_linked_list_node *const node)
{
    node->next = NULL;
    node->previous = list->tail;

    if (list->tail)
    {
        list->tail->next = node;
    }
    else
    {
        list->head = node;
    }

    list->tail = node;
    return node;
}

static inline struct CCC_Doubly_linked_list_node *
insert_node(struct CCC_Doubly_linked_list *const list,
            struct CCC_Doubly_linked_list_node *const position,
            struct CCC_Doubly_linked_list_node *const node)
{
    if (!position)
    {
        return push_back(list, node);
    }
    node->next = position;
    node->previous = position->previous;

    if (position->previous)
    {
        position->previous->next = node;
    }
    else
    {
        list->head = node;
    }
    position->previous = node;
    return node;
}

static inline struct CCC_Doubly_linked_list_node *
remove_node(struct CCC_Doubly_linked_list *const list,
            struct CCC_Doubly_linked_list_node *const node)
{
    if (node->previous)
    {
        node->previous->next = node->next;
    }
    else
    {
        list->head = node->next;
    }

    if (node->next)
    {
        node->next->previous = node->previous;
    }
    else
    {
        list->tail = node->previous;
    }

    node->next = node->previous = NULL;
    return node;
}

static size_t
erase_range(struct CCC_Doubly_linked_list const *const list,
            struct CCC_Doubly_linked_list_node *begin,
            struct CCC_Doubly_linked_list_node *const end)
{
    if (!list->allocate)
    {
        return len(begin, end);
    }
    size_t count = 0;
    CCC_Doubly_linked_list_node *node = begin;
    for (;;)
    {
        assert(count < list->count);
        CCC_Doubly_linked_list_node *next = node->next;
        list->allocate((CCC_Allocator_context){
            .input = struct_base(list, node),
            .bytes = 0,
            .context = list->context,
        });
        count++;
        if (node == end)
        {
            break;
        }
        node = next;
    }
    return count;
}

/** Finds the length from [begin, end]. End is counted. */
static size_t
len(struct CCC_Doubly_linked_list_node const *begin,
    struct CCC_Doubly_linked_list_node const *const end)
{
    size_t s = 1;
    while (begin != end)
    {
        begin = begin->next;
        ++s;
    }
    return s;
}

static inline void *
struct_base(struct CCC_Doubly_linked_list const *const list,
            struct CCC_Doubly_linked_list_node const *const node)
{
    return node ? ((char *)&node->next) - list->type_intruder_offset : NULL;
}

static inline struct CCC_Doubly_linked_list_node *
type_intruder_in(struct CCC_Doubly_linked_list const *const list,
                 void const *const struct_base)
{
    return struct_base
             ? (struct CCC_Doubly_linked_list_node
                    *)((char *)struct_base + list->type_intruder_offset)
             : NULL;
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
