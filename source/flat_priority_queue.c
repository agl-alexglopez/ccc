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
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "flat_priority_queue.h"
#include "private/private_flat_priority_queue.h"
#include "types.h"

enum : size_t
{
    START_CAP = 8,
};

/*=====================      Prototypes      ================================*/

static void *at(struct CCC_Flat_priority_queue const *, size_t);
static size_t index_of(struct CCC_Flat_priority_queue const *, void const *);
static CCC_Tribool wins(struct CCC_Flat_priority_queue const *, size_t, size_t);
static CCC_Order order(struct CCC_Flat_priority_queue const *, size_t, size_t);
static void swap(struct CCC_Flat_priority_queue *, void *, size_t, size_t);
static size_t bubble_up(struct CCC_Flat_priority_queue *, void *, size_t);
static size_t bubble_down(struct CCC_Flat_priority_queue *, void *, size_t,
                          size_t);
static size_t update_fixup(struct CCC_Flat_priority_queue *, void *, void *);
static void heapify(struct CCC_Flat_priority_queue *, size_t, void *);
static void destroy_each(struct CCC_Flat_priority_queue *,
                         CCC_Type_destructor *);

/*=====================       Interface      ================================*/

CCC_Result
CCC_flat_priority_queue_heapify(CCC_Flat_priority_queue *const priority_queue,
                                void *const temp, void *const type_array,
                                size_t const count, size_t const sizeof_type)
{
    if (!priority_queue || !type_array || !temp
        || type_array == priority_queue->buffer.data
        || sizeof_type != priority_queue->buffer.sizeof_type)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (count > priority_queue->buffer.capacity)
    {
        CCC_Result const resize_res = CCC_buffer_allocate(
            &priority_queue->buffer, count, priority_queue->buffer.allocate);
        if (resize_res != CCC_RESULT_OK)
        {
            return resize_res;
        }
    }
    (void)memcpy(priority_queue->buffer.data, type_array, count * sizeof_type);
    heapify(priority_queue, count, temp);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_flat_priority_queue_heapify_inplace(CCC_Flat_priority_queue *priority_queue,
                                        void *const temp, size_t const count)
{
    if (!priority_queue || !temp || count > priority_queue->buffer.capacity)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    heapify(priority_queue, count, temp);
    return CCC_RESULT_OK;
}

CCC_Buffer
CCC_flat_priority_queue_heapsort(CCC_Flat_priority_queue *const priority_queue,
                                 void *const temp)
{
    CCC_Buffer ret = {};
    if (!priority_queue || !temp)
    {
        return ret;
    }
    ret = priority_queue->buffer;
    if (priority_queue->buffer.count > 1)
    {
        size_t end = priority_queue->buffer.count;
        while (--end)
        {
            swap(priority_queue, temp, 0, end);
            (void)bubble_down(priority_queue, temp, 0, end);
        }
    }
    *priority_queue = (CCC_Flat_priority_queue){};
    return ret;
}

void *
CCC_flat_priority_queue_push(CCC_Flat_priority_queue *const priority_queue,
                             void const *const type, void *const temp)
{
    if (!priority_queue || !type || !temp)
    {
        return NULL;
    }
    void *const new = CCC_buffer_allocate_back(&priority_queue->buffer);
    if (!new)
    {
        return NULL;
    }
    if (new != type)
    {
        (void)memcpy(new, type, priority_queue->buffer.sizeof_type);
    }
    assert(temp);
    size_t const i
        = bubble_up(priority_queue, temp, priority_queue->buffer.count - 1);
    assert(i < priority_queue->buffer.count);
    return CCC_buffer_at(&priority_queue->buffer, i);
}

CCC_Result
CCC_flat_priority_queue_pop(CCC_Flat_priority_queue *const priority_queue,
                            void *const temp)
{
    if (!priority_queue || !temp || !priority_queue->buffer.count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    --priority_queue->buffer.count;
    if (!priority_queue->buffer.count)
    {
        return CCC_RESULT_OK;
    }
    swap(priority_queue, temp, 0, priority_queue->buffer.count);
    (void)bubble_down(priority_queue, temp, 0, priority_queue->buffer.count);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_flat_priority_queue_erase(CCC_Flat_priority_queue *const priority_queue,
                              void *const type, void *const temp)
{
    if (!priority_queue || !type || !temp || !priority_queue->buffer.count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    size_t const i = index_of(priority_queue, type);
    --priority_queue->buffer.count;
    if (i == priority_queue->buffer.count)
    {
        return CCC_RESULT_OK;
    }
    swap(priority_queue, temp, i, priority_queue->buffer.count);
    CCC_Order const order_res
        = order(priority_queue, i, priority_queue->buffer.count);
    if (order_res == priority_queue->order)
    {
        (void)bubble_up(priority_queue, temp, i);
    }
    else if (order_res != CCC_ORDER_EQUAL)
    {
        (void)bubble_down(priority_queue, temp, i,
                          priority_queue->buffer.count);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return CCC_RESULT_OK;
}

void *
CCC_flat_priority_queue_update(CCC_Flat_priority_queue *const priority_queue,
                               void *const type, void *const temp,
                               CCC_Type_modifier *const modify,
                               void *const context)
{
    if (!priority_queue || !type || !temp || !modify
        || !priority_queue->buffer.count)
    {
        return NULL;
    }
    modify((CCC_Type_context){
        .type = type,
        .context = context,
    });
    return CCC_buffer_at(&priority_queue->buffer,
                         update_fixup(priority_queue, type, temp));
}

/* There are no efficiency benefits in knowing an increase will occur. */
void *
CCC_flat_priority_queue_increase(CCC_Flat_priority_queue *const priority_queue,
                                 void *const type, void *const temp,
                                 CCC_Type_modifier *const modify,
                                 void *const context)
{
    return CCC_flat_priority_queue_update(priority_queue, type, temp, modify,
                                          context);
}

/* There are no efficiency benefits in knowing an decrease will occur. */
void *
CCC_flat_priority_queue_decrease(CCC_Flat_priority_queue *const priority_queue,
                                 void *const type, void *const temp,
                                 CCC_Type_modifier *const modify,
                                 void *const context)
{
    return CCC_flat_priority_queue_update(priority_queue, type, temp, modify,
                                          context);
}

void *
CCC_flat_priority_queue_front(
    CCC_Flat_priority_queue const *const priority_queue)
{
    if (!priority_queue || !priority_queue->buffer.count)
    {
        return NULL;
    }
    return at(priority_queue, 0);
}

CCC_Tribool
CCC_flat_priority_queue_is_empty(
    CCC_Flat_priority_queue const *const priority_queue)
{
    if (!priority_queue)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return CCC_buffer_is_empty(&priority_queue->buffer);
}

CCC_Count
CCC_flat_priority_queue_count(
    CCC_Flat_priority_queue const *const priority_queue)
{
    if (!priority_queue)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return CCC_buffer_count(&priority_queue->buffer);
}

CCC_Count
CCC_flat_priority_queue_capacity(
    CCC_Flat_priority_queue const *const priority_queue)
{
    if (!priority_queue)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return CCC_buffer_capacity(&priority_queue->buffer);
}

void *
CCC_flat_priority_queue_data(
    CCC_Flat_priority_queue const *const priority_queue)
{
    return priority_queue ? CCC_buffer_begin(&priority_queue->buffer) : NULL;
}

CCC_Order
CCC_flat_priority_queue_order(
    CCC_Flat_priority_queue const *const priority_queue)
{
    return priority_queue ? priority_queue->order : CCC_ORDER_ERROR;
}

CCC_Result
CCC_flat_priority_queue_reserve(CCC_Flat_priority_queue *const priority_queue,
                                size_t const to_add,
                                CCC_Allocator *const allocate)
{
    if (!priority_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    return CCC_buffer_reserve(&priority_queue->buffer, to_add, allocate);
}

CCC_Result
CCC_flat_priority_queue_copy(CCC_Flat_priority_queue *const destination,
                             CCC_Flat_priority_queue const *const source,
                             CCC_Allocator *const allocate)
{
    if (!destination || !source || source == destination
        || (destination->buffer.capacity < source->buffer.capacity
            && !allocate))
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!source->buffer.count)
    {
        return CCC_RESULT_OK;
    }
    if (destination->buffer.capacity < source->buffer.capacity)
    {
        CCC_Result const r = CCC_buffer_allocate(
            &destination->buffer, source->buffer.capacity, allocate);
        if (r != CCC_RESULT_OK)
        {
            return r;
        }
        destination->buffer.capacity = source->buffer.capacity;
    }
    if (!source->buffer.data || !destination->buffer.data)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    destination->buffer.count = source->buffer.count;
    /* It is ok to only copy count elements because we know that all elements
       in a binary heap are contiguous from [0, C), where C is count. */
    (void)memcpy(destination->buffer.data, source->buffer.data,
                 source->buffer.count * source->buffer.sizeof_type);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_flat_priority_queue_clear(CCC_Flat_priority_queue *const priority_queue,
                              CCC_Type_destructor *const destroy)
{
    if (!priority_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (destroy)
    {
        destroy_each(priority_queue, destroy);
    }
    return CCC_buffer_size_set(&priority_queue->buffer, 0);
}

CCC_Result
CCC_flat_priority_queue_clear_and_free(
    CCC_Flat_priority_queue *const priority_queue,
    CCC_Type_destructor *const destroy)
{
    if (!priority_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (destroy)
    {
        destroy_each(priority_queue, destroy);
    }
    return CCC_buffer_allocate(&priority_queue->buffer, 0,
                               priority_queue->buffer.allocate);
}

CCC_Result
CCC_flat_priority_queue_clear_and_free_reserve(
    CCC_Flat_priority_queue *const priority_queue,
    CCC_Type_destructor *const destructor, CCC_Allocator *const allocate)
{
    if (!priority_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (destructor)
    {
        destroy_each(priority_queue, destructor);
    }
    return CCC_buffer_allocate(&priority_queue->buffer, 0, allocate);
}

CCC_Tribool
CCC_flat_priority_queue_validate(
    CCC_Flat_priority_queue const *const priority_queue)
{
    if (!priority_queue)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const count = priority_queue->buffer.count;
    if (count <= 1)
    {
        return CCC_TRUE;
    }
    for (size_t i = 0, left = (i * 2) + 1, right = (i * 2) + 2,
                end = (count - 2) / 2;
         i <= end; ++i, left = (i * 2) + 1, right = (i * 2) + 2)
    {
        /* Putting the child in the comparison function first evaluates
           the child's three way comparison in relation to the parent. If
           the child beats the parent in total ordering (min/max) something
           has gone wrong. */
        if (left < count && wins(priority_queue, left, i))
        {
            return CCC_FALSE;
        }
        if (right < count && wins(priority_queue, right, i))
        {
            return CCC_FALSE;
        }
    }
    return CCC_TRUE;
}

/*===================     Private Interface     =============================*/

size_t
CCC_private_flat_priority_queue_bubble_up(
    struct CCC_Flat_priority_queue *const priority_queue, void *const temp,
    size_t index)
{
    return bubble_up(priority_queue, temp, index);
}

void *
CCC_private_flat_priority_queue_update_fixup(
    struct CCC_Flat_priority_queue *const priority_queue, void *const type,
    void *const temp)
{
    return CCC_buffer_at(&priority_queue->buffer,
                         update_fixup(priority_queue, type, temp));
}

void
CCC_private_flat_priority_queue_in_place_heapify(
    struct CCC_Flat_priority_queue *const priority_queue, size_t const count,
    void *const temp)
{
    if (!priority_queue || priority_queue->buffer.capacity < count)
    {
        return;
    }
    heapify(priority_queue, count, temp);
}

/*====================     Static Helpers     ===============================*/

/* Orders the heap in O(N) time. Assumes n > 0 and n <= capacity. */
static void
heapify(struct CCC_Flat_priority_queue *const priority_queue,
        size_t const count, void *const temp)
{
    assert(count);
    assert(count <= priority_queue->buffer.capacity);
    priority_queue->buffer.count = count;
    size_t i = ((count - 1) / 2) + 1;
    while (i--)
    {
        (void)bubble_down(priority_queue, temp, i,
                          priority_queue->buffer.count);
    }
}

/* Fixes the position of element e after its key value has been changed. */
static size_t
update_fixup(struct CCC_Flat_priority_queue *const priority_queue,
             void *const type, void *const temp)
{
    size_t const index = index_of(priority_queue, type);
    if (!index)
    {
        return bubble_down(priority_queue, temp, 0,
                           priority_queue->buffer.count);
    }
    CCC_Order const parent_order
        = order(priority_queue, index, (index - 1) / 2);
    if (parent_order == priority_queue->order)
    {
        return bubble_up(priority_queue, temp, index);
    }
    if (parent_order != CCC_ORDER_EQUAL)
    {
        return bubble_down(priority_queue, temp, index,
                           priority_queue->buffer.count);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return index;
}

/* Returns the sorted position of the element starting at position i. */
static size_t
bubble_up(struct CCC_Flat_priority_queue *const priority_queue,
          void *const temp, size_t index)
{
    for (size_t parent = (index - 1) / 2; index;
         index = parent, parent = (parent - 1) / 2)
    {
        /* Not winning here means we are in correct order or equal. */
        if (!wins(priority_queue, index, parent))
        {
            return index;
        }
        swap(priority_queue, temp, index, parent);
    }
    return 0;
}

/* Returns the sorted position of the element starting at position i. */
static size_t
bubble_down(struct CCC_Flat_priority_queue *const priority_queue,
            void *const temp, size_t index, size_t const count)
{
    for (size_t next = 0, left = (index * 2) + 1, right = left + 1;
         left < count; index = next, left = (index * 2) + 1, right = left + 1)
    {
        /* Avoid one comparison call if there is no right child. */
        next = (right < count && wins(priority_queue, right, left)) ? right
                                                                    : left;
        /* If the child beats the parent we must swap. Equal is OK to break. */
        if (!wins(priority_queue, next, index))
        {
            return index;
        }
        swap(priority_queue, temp, next, index);
    }
    return index;
}

/* Returns true if the winner (the "left hand side") wins the comparison.
   Winning in a three-way comparison means satisfying the total order of the
   priority queue. So, there is no winner if the elements are equal and this
   function would return false. If the winner is in the wrong order, thus
   losing the total order comparison, the function also returns false. */
static inline CCC_Tribool
wins(struct CCC_Flat_priority_queue const *const priority_queue,
     size_t const winner, size_t const loser)
{
    return order(priority_queue, winner, loser) == priority_queue->order;
}

static inline CCC_Order
order(struct CCC_Flat_priority_queue const *const priority_queue, size_t left,
      size_t right)
{
    return priority_queue->compare((CCC_Type_comparator_context){
        .type_left = at(priority_queue, left),
        .type_right = at(priority_queue, right),
        .context = priority_queue->buffer.context,
    });
}

/* Swaps i and j using the underlying Buffer capabilities. Not checked for
   an error in release. */
static inline void
swap(struct CCC_Flat_priority_queue *const priority_queue, void *const temp,
     size_t const index, size_t const swap_index)
{
    [[maybe_unused]] CCC_Result const res
        = CCC_buffer_swap(&priority_queue->buffer, temp, index, swap_index);
    assert(res == CCC_RESULT_OK);
}

/* Thin wrapper just for sanity checking in debug mode as index should always
   be valid when this function is used. */
static inline void *
at(struct CCC_Flat_priority_queue const *const priority_queue,
   size_t const index)
{
    void *const addr = CCC_buffer_at(&priority_queue->buffer, index);
    assert(addr);
    return addr;
}

/* Flat priority queue code that uses indices of the underlying Buffer should
   always be within the Buffer range. It should never exceed the current size
   and start at or after the Buffer base. Only checked in debug. */
static inline size_t
index_of(struct CCC_Flat_priority_queue const *const priority_queue,
         void const *const slot)
{
    assert(slot >= priority_queue->buffer.data);
    size_t const i = ((char *)slot - (char *)priority_queue->buffer.data)
                   / priority_queue->buffer.sizeof_type;
    assert(i < priority_queue->buffer.count);
    return i;
}

static inline void
destroy_each(struct CCC_Flat_priority_queue *const priority_queue,
             CCC_Type_destructor *const destroy)
{
    size_t const count = priority_queue->buffer.count;
    for (size_t i = 0; i < count; ++i)
    {
        destroy((CCC_Type_context){
            .type = at(priority_queue, i),
            .context = priority_queue->buffer.context,
        });
    }
}
