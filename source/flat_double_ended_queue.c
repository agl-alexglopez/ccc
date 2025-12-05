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
#include "flat_double_ended_queue.h"
#include "private/private_flat_double_ended_queue.h"
#include "types.h"

enum : size_t
{
    START_CAPACITY = 8,
};

/*==========================    Prototypes    ===============================*/

static CCC_Result maybe_resize(struct CCC_Flat_double_ended_queue *, size_t,
                               CCC_Allocator *);
static size_t index_of(struct CCC_Flat_double_ended_queue const *,
                       void const *);
static void *at(struct CCC_Flat_double_ended_queue const *, size_t);
static size_t increment(struct CCC_Flat_double_ended_queue const *, size_t);
static size_t decrement(struct CCC_Flat_double_ended_queue const *, size_t);
static size_t distance(struct CCC_Flat_double_ended_queue const *, size_t,
                       size_t);
static size_t rdistance(struct CCC_Flat_double_ended_queue const *, size_t,
                        size_t);
static CCC_Result push_front_range(struct CCC_Flat_double_ended_queue *, size_t,
                                   char const *);
static CCC_Result push_back_range(struct CCC_Flat_double_ended_queue *, size_t,
                                  char const *);
static size_t back_free_slot(struct CCC_Flat_double_ended_queue const *);
static size_t front_free_slot(size_t, size_t);
static size_t last_node_index(struct CCC_Flat_double_ended_queue const *);
static void *push_range(struct CCC_Flat_double_ended_queue *, char const *,
                        size_t, char const *);
static void *allocate_front(struct CCC_Flat_double_ended_queue *);
static void *allocate_back(struct CCC_Flat_double_ended_queue *);
static size_t min(size_t, size_t);

/*==========================     Interface    ===============================*/

void *
CCC_flat_double_ended_queue_push_back(CCC_Flat_double_ended_queue *const queue,
                                      void const *const type)
{
    if (!queue || !type)
    {
        return NULL;
    }
    void *const slot = allocate_back(queue);
    if (slot && slot != type)
    {
        (void)memcpy(slot, type, queue->buffer.sizeof_type);
    }
    return slot;
}

void *
CCC_flat_double_ended_queue_push_front(CCC_Flat_double_ended_queue *const queue,
                                       void const *const type)
{
    if (!queue || !type)
    {
        return NULL;
    }
    void *const slot = allocate_front(queue);
    if (slot && slot != type)
    {
        (void)memcpy(slot, type, queue->buffer.sizeof_type);
    }
    return slot;
}

CCC_Result
CCC_flat_double_ended_queue_push_front_range(
    CCC_Flat_double_ended_queue *const queue, size_t const count,
    void const *const type_array)
{
    if (!queue || !type_array)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    return push_front_range(queue, count, type_array);
}

CCC_Result
CCC_flat_double_ended_queue_push_back_range(
    CCC_Flat_double_ended_queue *const queue, size_t const count,
    void const *type_array)
{
    if (!queue || !type_array)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    return push_back_range(queue, count, type_array);
}

void *
CCC_flat_double_ended_queue_insert_range(
    CCC_Flat_double_ended_queue *const queue, void *const position,
    size_t const count, void const *type_array)
{
    if (!queue || !type_array)
    {
        return NULL;
    }
    if (!count)
    {
        return position;
    }
    if (position == CCC_flat_double_ended_queue_begin(queue))
    {
        return push_front_range(queue, count, type_array) != CCC_RESULT_OK
                 ? NULL
                 : at(queue, count - 1);
    }
    if (position == CCC_flat_double_ended_queue_end(queue))
    {
        return push_back_range(queue, count, type_array) != CCC_RESULT_OK
                 ? NULL
                 : at(queue, queue->buffer.count - count);
    }
    return push_range(queue, position, count, type_array);
}

CCC_Result
CCC_flat_double_ended_queue_pop_front(CCC_Flat_double_ended_queue *const queue)
{
    if (!queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (CCC_buffer_is_empty(&queue->buffer))
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    queue->front = increment(queue, queue->front);
    return CCC_buffer_size_minus(&queue->buffer, 1);
}

CCC_Result
CCC_flat_double_ended_queue_pop_back(CCC_Flat_double_ended_queue *const queue)
{
    if (!queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    return CCC_buffer_size_minus(&queue->buffer, 1);
}

void *
CCC_flat_double_ended_queue_front(
    CCC_Flat_double_ended_queue const *const queue)
{
    if (!queue || CCC_buffer_is_empty(&queue->buffer))
    {
        return NULL;
    }
    return CCC_buffer_at(&queue->buffer, queue->front);
}

void *
CCC_flat_double_ended_queue_back(CCC_Flat_double_ended_queue const *const queue)
{
    if (!queue || CCC_buffer_is_empty(&queue->buffer))
    {
        return NULL;
    }
    return CCC_buffer_at(&queue->buffer, last_node_index(queue));
}

CCC_Tribool
CCC_flat_double_ended_queue_is_empty(
    CCC_Flat_double_ended_queue const *const queue)
{
    if (!queue)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !queue->buffer.count;
}

CCC_Count
CCC_flat_double_ended_queue_count(
    CCC_Flat_double_ended_queue const *const queue)
{
    if (!queue)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = queue->buffer.count};
}

CCC_Count
CCC_flat_double_ended_queue_capacity(
    CCC_Flat_double_ended_queue const *const queue)
{
    if (!queue)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = queue->buffer.capacity};
}

void *
CCC_flat_double_ended_queue_at(CCC_Flat_double_ended_queue const *const queue,
                               size_t const i)
{
    if (!queue || i >= queue->buffer.capacity)
    {
        return NULL;
    }
    return CCC_buffer_at(&queue->buffer,
                         (queue->front + i) % queue->buffer.capacity);
}

void *
CCC_flat_double_ended_queue_begin(
    CCC_Flat_double_ended_queue const *const queue)
{
    if (!queue || !queue->buffer.count)
    {
        return NULL;
    }
    return CCC_buffer_at(&queue->buffer, queue->front);
}

void *
CCC_flat_double_ended_queue_reverse_begin(
    CCC_Flat_double_ended_queue const *const queue)
{
    if (!queue || !queue->buffer.count)
    {
        return NULL;
    }
    return CCC_buffer_at(&queue->buffer, last_node_index(queue));
}

void *
CCC_flat_double_ended_queue_next(CCC_Flat_double_ended_queue const *const queue,
                                 void const *const iterator_pointer)
{
    size_t const next_i = increment(queue, index_of(queue, iterator_pointer));
    if (next_i == queue->front
        || distance(queue, next_i, queue->front) >= queue->buffer.count)
    {
        return NULL;
    }
    return CCC_buffer_at(&queue->buffer, next_i);
}

void *
CCC_flat_double_ended_queue_reverse_next(
    CCC_Flat_double_ended_queue const *const queue,
    void const *const iterator_pointer)
{
    size_t const cur_i = index_of(queue, iterator_pointer);
    size_t const next_i = decrement(queue, cur_i);
    size_t const reverse_begin = last_node_index(queue);
    if (next_i == reverse_begin
        || rdistance(queue, next_i, reverse_begin) >= queue->buffer.count)
    {
        return NULL;
    }
    return CCC_buffer_at(&queue->buffer, next_i);
}

void *
CCC_flat_double_ended_queue_end(CCC_Flat_double_ended_queue const *const)
{
    return NULL;
}

void *
CCC_flat_double_ended_queue_reverse_end(
    CCC_Flat_double_ended_queue const *const)
{
    return NULL;
}

void *
CCC_flat_double_ended_queue_data(CCC_Flat_double_ended_queue const *const queue)
{
    return queue ? CCC_buffer_begin(&queue->buffer) : NULL;
}

CCC_Result
CCC_flat_double_ended_queue_copy(
    CCC_Flat_double_ended_queue *const destination,
    CCC_Flat_double_ended_queue const *const source,
    CCC_Allocator *const allocate)
{
    if (!destination || !source || source == destination
        || (destination->buffer.capacity < source->buffer.capacity
            && !allocate))
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* Copying from an empty source is odd but supported. */
    if (!source->buffer.capacity)
    {
        return CCC_RESULT_OK;
    }
    if (destination->buffer.capacity < source->buffer.capacity)
    {
        CCC_Result resize_res = CCC_buffer_allocate(
            &destination->buffer, source->buffer.capacity, allocate);
        if (resize_res != CCC_RESULT_OK)
        {
            return resize_res;
        }
        destination->buffer.capacity = source->buffer.capacity;
    }
    if (!destination->buffer.data || !source->buffer.data)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    destination->buffer.count = source->buffer.count;
    if (destination->buffer.capacity > source->buffer.capacity)
    {
        size_t const first_chunk = min(source->buffer.count,
                                       source->buffer.capacity - source->front);
        (void)memcpy(destination->buffer.data,
                     CCC_buffer_at(&source->buffer, source->front),
                     source->buffer.sizeof_type * first_chunk);
        if (first_chunk < source->buffer.count)
        {
            (void)memcpy((char *)destination->buffer.data
                             + (source->buffer.sizeof_type * first_chunk),
                         source->buffer.data,
                         source->buffer.sizeof_type
                             * (source->buffer.count - first_chunk));
        }
        destination->front = 0;
        return CCC_RESULT_OK;
    }
    (void)memcpy(destination->buffer.data, source->buffer.data,
                 source->buffer.capacity * source->buffer.sizeof_type);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_flat_double_ended_queue_reserve(CCC_Flat_double_ended_queue *const queue,
                                    size_t const to_add,
                                    CCC_Allocator *const allocate)
{
    if (!queue || !allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    return maybe_resize(queue, to_add, allocate);
}

CCC_Result
CCC_flat_double_ended_queue_clear(CCC_Flat_double_ended_queue *const queue,
                                  CCC_Type_destructor *const destructor)
{
    if (!queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destructor)
    {
        queue->front = 0;
        return CCC_buffer_size_set(&queue->buffer, 0);
    }
    size_t const back = back_free_slot(queue);
    for (size_t i = queue->front; i != back; i = increment(queue, i))
    {
        destructor((CCC_Type_context){
            .type = CCC_buffer_at(&queue->buffer, i),
            .context = queue->buffer.context,
        });
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_flat_double_ended_queue_clear_and_free(
    CCC_Flat_double_ended_queue *const queue,
    CCC_Type_destructor *const destructor)
{
    if (!queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destructor)
    {
        queue->buffer.count = queue->front = 0;
        return CCC_buffer_allocate(&queue->buffer, 0, queue->buffer.allocate);
    }
    size_t const back = back_free_slot(queue);
    for (size_t i = queue->front; i != back; i = increment(queue, i))
    {
        destructor((CCC_Type_context){
            .type = CCC_buffer_at(&queue->buffer, i),
            .context = queue->buffer.context,
        });
    }
    CCC_Result const r
        = CCC_buffer_allocate(&queue->buffer, 0, queue->buffer.allocate);
    if (r == CCC_RESULT_OK)
    {
        queue->buffer.count = queue->front = 0;
    }
    return r;
}

CCC_Result
CCC_flat_double_ended_queue_clear_and_free_reserve(
    CCC_Flat_double_ended_queue *const queue,
    CCC_Type_destructor *const destructor, CCC_Allocator *const allocate)
{
    if (!queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destructor)
    {
        queue->buffer.count = queue->front = 0;
        return CCC_buffer_allocate(&queue->buffer, 0, allocate);
    }
    size_t const back = back_free_slot(queue);
    for (size_t i = queue->front; i != back; i = increment(queue, i))
    {
        destructor((CCC_Type_context){
            .type = CCC_buffer_at(&queue->buffer, i),
            .context = queue->buffer.context,
        });
    }
    CCC_Result const r = CCC_buffer_allocate(&queue->buffer, 0, allocate);
    if (r == CCC_RESULT_OK)
    {
        queue->buffer.count = queue->front = 0;
    }
    return r;
}

CCC_Tribool
CCC_flat_double_ended_queue_validate(
    CCC_Flat_double_ended_queue const *const queue)
{
    if (!queue)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (CCC_flat_double_ended_queue_is_empty(queue))
    {
        return CCC_TRUE;
    }
    void *iterator = CCC_flat_double_ended_queue_begin(queue);
    if (CCC_buffer_index(&queue->buffer, iterator).count != queue->front)
    {
        return CCC_FALSE;
    }
    size_t size = 0;
    for (; iterator != CCC_flat_double_ended_queue_end(queue);
         iterator = CCC_flat_double_ended_queue_next(queue, iterator), ++size)
    {
        if (size >= CCC_flat_double_ended_queue_count(queue).count)
        {
            return CCC_FALSE;
        }
    }
    if (size != CCC_flat_double_ended_queue_count(queue).count)
    {
        return CCC_FALSE;
    }
    size = 0;
    iterator = CCC_flat_double_ended_queue_reverse_begin(queue);
    if (CCC_buffer_index(&queue->buffer, iterator).count
        != last_node_index(queue))
    {
        return CCC_FALSE;
    }
    for (; iterator != CCC_flat_double_ended_queue_reverse_end(queue);
         iterator = CCC_flat_double_ended_queue_reverse_next(queue, iterator),
         ++size)
    {
        if (size >= CCC_flat_double_ended_queue_count(queue).count)
        {
            return CCC_FALSE;
        }
    }
    return size == CCC_flat_double_ended_queue_count(queue).count;
}

/*======================   Private Interface   ==============================*/

void *
CCC_private_flat_double_ended_queue_allocate_back(
    struct CCC_Flat_double_ended_queue *const queue)
{
    return allocate_back(queue);
}

void *
CCC_private_flat_double_ended_queue_allocate_front(
    struct CCC_Flat_double_ended_queue *const queue)
{
    return allocate_front(queue);
}

/*======================     Static Helpers    ==============================*/

static void *
allocate_front(struct CCC_Flat_double_ended_queue *const queue)
{
    CCC_Tribool const full
        = maybe_resize(queue, 1, queue->buffer.allocate) != CCC_RESULT_OK;
    /* Should have been able to resize. Bad error. */
    if ((full && !queue->buffer.capacity) || (queue->buffer.allocate && full))
    {
        return NULL;
    }
    queue->front = front_free_slot(queue->front, queue->buffer.capacity);
    void *const new_slot = CCC_buffer_at(&queue->buffer, queue->front);
    if (!full)
    {
        (void)CCC_buffer_size_plus(&queue->buffer, 1);
    }
    return new_slot;
}

static void *
allocate_back(struct CCC_Flat_double_ended_queue *const queue)
{
    CCC_Tribool const full
        = maybe_resize(queue, 1, queue->buffer.allocate) != CCC_RESULT_OK;
    /* Should have been able to resize. Bad error. */
    if ((full && !queue->buffer.capacity) || (queue->buffer.allocate && full))
    {
        return NULL;
    }
    void *const new_slot = CCC_buffer_at(&queue->buffer, back_free_slot(queue));
    /* If no reallocation policy is given we are a ring buffer. */
    if (full)
    {
        queue->front = increment(queue, queue->front);
    }
    else
    {
        (void)CCC_buffer_size_plus(&queue->buffer, 1);
    }
    return new_slot;
}

static CCC_Result
push_back_range(struct CCC_Flat_double_ended_queue *const queue, size_t const n,
                char const *elements)
{
    size_t const sizeof_type = queue->buffer.sizeof_type;
    CCC_Tribool const full
        = maybe_resize(queue, n, queue->buffer.allocate) != CCC_RESULT_OK;
    size_t const cap = queue->buffer.capacity;
    if ((full && !queue->buffer.capacity) || (queue->buffer.allocate && full))
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    /* If a range is too large we can make various simplifications to preserve
       the final capacity elements. */
    if (n >= cap)
    {
        elements += ((n - cap) * sizeof_type);
        queue->front = 0;
        (void)memcpy(CCC_buffer_at(&queue->buffer, 0), elements,
                     sizeof_type * cap);
        (void)CCC_buffer_size_set(&queue->buffer, cap);
        return CCC_RESULT_OK;
    }
    size_t const new_size = queue->buffer.count + n;
    size_t const back_slot = back_free_slot(queue);
    size_t const chunk = min(n, cap - back_slot);
    size_t const remainder_back_slot = (back_slot + chunk) % cap;
    size_t const remainder = (n - chunk);
    char const *const second_chunk = elements + (chunk * sizeof_type);
    (void)memcpy(CCC_buffer_at(&queue->buffer, back_slot), elements,
                 chunk * sizeof_type);
    if (remainder)
    {
        (void)memcpy(CCC_buffer_at(&queue->buffer, remainder_back_slot),
                     second_chunk, remainder * sizeof_type);
    }
    if (new_size > cap)
    {
        queue->front = (queue->front + (new_size - cap)) % cap;
    }
    (void)CCC_buffer_size_set(&queue->buffer, min(cap, new_size));
    return CCC_RESULT_OK;
}

static CCC_Result
push_front_range(struct CCC_Flat_double_ended_queue *const queue,
                 size_t const n, char const *elements)
{
    size_t const sizeof_type = queue->buffer.sizeof_type;
    CCC_Tribool const full
        = maybe_resize(queue, n, queue->buffer.allocate) != CCC_RESULT_OK;
    size_t const cap = queue->buffer.capacity;
    if ((full && !queue->buffer.capacity) || (queue->buffer.allocate && full))
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    /* If a range is too large we can make various simplifications to preserve
       the final capacity elements. */
    if (n >= cap)
    {
        elements += ((n - cap) * sizeof_type);
        queue->front = 0;
        (void)memcpy(CCC_buffer_at(&queue->buffer, 0), elements,
                     sizeof_type * cap);
        (void)CCC_buffer_size_set(&queue->buffer, cap);
        return CCC_RESULT_OK;
    }
    size_t const space_ahead = front_free_slot(queue->front, cap) + 1;
    size_t const i = n > space_ahead ? 0 : space_ahead - n;
    size_t const chunk = min(n, space_ahead);
    size_t const remainder = (n - chunk);
    char const *const first_chunk = elements + ((n - chunk) * sizeof_type);
    (void)memcpy(CCC_buffer_at(&queue->buffer, i), first_chunk,
                 chunk * sizeof_type);
    if (remainder)
    {
        (void)memcpy(CCC_buffer_at(&queue->buffer, cap - remainder), elements,
                     remainder * sizeof_type);
    }
    (void)CCC_buffer_size_set(&queue->buffer,
                              min(cap, queue->buffer.count + n));
    queue->front = remainder ? cap - remainder : i;
    return CCC_RESULT_OK;
}

static void *
push_range(struct CCC_Flat_double_ended_queue *const queue,
           char const *const position, size_t count, char const *elements)
{
    size_t const sizeof_type = queue->buffer.sizeof_type;
    CCC_Tribool const full
        = maybe_resize(queue, count, queue->buffer.allocate) != CCC_RESULT_OK;
    if ((full && !queue->buffer.capacity) || (queue->buffer.allocate && full))
    {
        return NULL;
    }
    size_t const cap = queue->buffer.capacity;
    size_t const new_size = queue->buffer.count + count;
    if (count >= cap)
    {
        elements += ((count - cap) * sizeof_type);
        queue->front = 0;
        void *const ret = CCC_buffer_at(&queue->buffer, 0);
        (void)memcpy(ret, elements, sizeof_type * cap);
        (void)CCC_buffer_size_set(&queue->buffer, cap);
        return ret;
    }
    size_t const pos_i = index_of(queue, position);
    size_t const back = back_free_slot(queue);
    size_t const to_move = back > pos_i ? back - pos_i : cap - pos_i + back;
    size_t const move_i = (pos_i + count) % cap;
    size_t move_chunk = move_i + to_move > cap ? cap - move_i : to_move;
    move_chunk = back < pos_i ? min(cap - pos_i, move_chunk)
                              : min(back - pos_i, move_chunk);
    size_t const move_remain = to_move - move_chunk;
    (void)memmove(CCC_buffer_at(&queue->buffer, move_i),
                  CCC_buffer_at(&queue->buffer, pos_i),
                  move_chunk * sizeof_type);
    if (move_remain)
    {
        size_t const move_remain_i = (move_i + move_chunk) % cap;
        size_t const remaining_start_i = (pos_i + move_chunk) % cap;
        (void)memmove(CCC_buffer_at(&queue->buffer, move_remain_i),
                      CCC_buffer_at(&queue->buffer, remaining_start_i),
                      move_remain * sizeof_type);
    }
    size_t const elements_chunk = min(count, cap - pos_i);
    size_t const elements_remain = count - elements_chunk;
    (void)memcpy(CCC_buffer_at(&queue->buffer, pos_i), elements,
                 elements_chunk * sizeof_type);
    if (elements_remain)
    {
        char const *const second_chunk
            = elements + (elements_chunk * sizeof_type);
        size_t const second_chunk_i = (pos_i + elements_chunk) % cap;
        (void)memcpy(CCC_buffer_at(&queue->buffer, second_chunk_i),
                     second_chunk, elements_remain * sizeof_type);
    }
    if (new_size > cap)
    {
        /* Wrapping behavior stops if it would overwrite the start of the
           range being inserted. This is to preserve as much info about
           the range as possible. If wrapping occurs the range is the new
           front. */
        size_t const excess = (new_size - cap);
        size_t const front_to_pos_dist = (pos_i + cap - queue->front) % cap;
        queue->front = (queue->front + min(excess, front_to_pos_dist)) % cap;
    }
    (void)CCC_buffer_size_set(&queue->buffer, min(cap, new_size));
    return CCC_buffer_at(&queue->buffer, pos_i);
}

static CCC_Result
maybe_resize(struct CCC_Flat_double_ended_queue *const queue,
             size_t const additional_nodes_to_add,
             CCC_Allocator *const allocate)
{
    size_t required = queue->buffer.count + additional_nodes_to_add;
    if (required < queue->buffer.count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (required <= queue->buffer.capacity)
    {
        return CCC_RESULT_OK;
    }
    if (!allocate)
    {
        return CCC_RESULT_NO_ALLOCATION_FUNCTION;
    }
    size_t const sizeof_type = queue->buffer.sizeof_type;
    if (!queue->buffer.capacity && additional_nodes_to_add == 1)
    {
        required = START_CAPACITY;
    }
    else if (additional_nodes_to_add == 1)
    {
        required = queue->buffer.capacity * 2;
    }
    void *const new_data = allocate((CCC_Allocator_context){
        .input = NULL,
        .bytes = sizeof_type * required,
        .context = queue->buffer.context,
    });
    if (!new_data)
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    if (queue->buffer.count)
    {
        size_t const first_chunk
            = min(queue->buffer.count, queue->buffer.capacity - queue->front);
        (void)memcpy(new_data, CCC_buffer_at(&queue->buffer, queue->front),
                     sizeof_type * first_chunk);
        if (first_chunk < queue->buffer.count)
        {
            (void)memcpy((char *)new_data + (sizeof_type * first_chunk),
                         CCC_buffer_begin(&queue->buffer),
                         sizeof_type * (queue->buffer.count - first_chunk));
        }
    }
    (void)CCC_buffer_allocate(&queue->buffer, 0, allocate);
    queue->buffer.data = new_data;
    queue->front = 0;
    queue->buffer.capacity = required;
    return CCC_RESULT_OK;
}

/** Returns the distance between the current iterator position and the origin
position. Distance is calculated in ascending indices, meaning the result is
the number of forward steps in the Buffer origin would need to take reach
iterator, possibly accounting for wrapping around the end of the buffer. */
static inline size_t
distance(struct CCC_Flat_double_ended_queue const *const queue,
         size_t const iterator, size_t const origin)
{
    return iterator > origin ? iterator - origin
                             : (queue->buffer.capacity - origin) + iterator;
}

/** Returns the rdistance between the current iterator position and the origin
position. Rdistance is calculated in descending indices, meaning the result is
the number of backward steps in the Buffer origin would need to take to reach
iterator, possibly accounting for wrapping around the beginning of buffer. */
static inline size_t
rdistance(struct CCC_Flat_double_ended_queue const *const queue,
          size_t const iterator, size_t const origin)
{
    return iterator > origin ? (queue->buffer.capacity - iterator) + origin
                             : origin - iterator;
}

static inline size_t
index_of(struct CCC_Flat_double_ended_queue const *const queue,
         void const *const position)
{
    assert(position >= CCC_buffer_begin(&queue->buffer));
    assert(position < CCC_buffer_capacity_end(&queue->buffer));
    return (
        size_t)(((char *)position - (char *)CCC_buffer_begin(&queue->buffer))
                / queue->buffer.sizeof_type);
}

static inline void *
at(struct CCC_Flat_double_ended_queue const *const queue, size_t const index)
{
    return CCC_buffer_at(&queue->buffer,
                         (queue->front + index) % queue->buffer.capacity);
}

static inline size_t
increment(struct CCC_Flat_double_ended_queue const *const queue,
          size_t const index)
{
    return index == (queue->buffer.capacity - 1) ? 0 : index + 1;
}

static inline size_t
decrement(struct CCC_Flat_double_ended_queue const *const queue,
          size_t const index)
{
    return index ? index - 1 : queue->buffer.capacity - 1;
}

static inline size_t
back_free_slot(struct CCC_Flat_double_ended_queue const *const queue)
{
    return (queue->front + queue->buffer.count) % queue->buffer.capacity;
}

static inline size_t
front_free_slot(size_t const front, size_t const capacity)
{
    return front ? front - 1 : capacity - 1;
}

/** Returns index of the last element in the double_ended_queue or front if
 * empty. */
static inline size_t
last_node_index(struct CCC_Flat_double_ended_queue const *const queue)
{
    return queue->buffer.count
             ? (queue->front + queue->buffer.count - 1) % queue->buffer.capacity
             : queue->front;
}

static inline size_t
min(size_t const left, size_t const right)
{
    return left < right ? left : right;
}
