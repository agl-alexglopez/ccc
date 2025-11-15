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
static void *at(struct CCC_Flat_double_ended_queue const *, size_t i);
static size_t increment(struct CCC_Flat_double_ended_queue const *, size_t i);
static size_t decrement(struct CCC_Flat_double_ended_queue const *, size_t i);
static size_t distance(struct CCC_Flat_double_ended_queue const *, size_t,
                       size_t);
static size_t rdistance(struct CCC_Flat_double_ended_queue const *, size_t,
                        size_t);
static CCC_Result
push_front_range(struct CCC_Flat_double_ended_queue *flat_double_ended_queue,
                 size_t n, char const *elems);
static CCC_Result
push_back_range(struct CCC_Flat_double_ended_queue *flat_double_ended_queue,
                size_t n, char const *elems);
static size_t back_free_slot(struct CCC_Flat_double_ended_queue const *);
static size_t front_free_slot(size_t front, size_t cap);
static size_t last_node_index(struct CCC_Flat_double_ended_queue const *);
static void *
push_range(struct CCC_Flat_double_ended_queue *flat_double_ended_queue,
           char const *pos, size_t n, char const *elems);
static void *
allocate_front(struct CCC_Flat_double_ended_queue *flat_double_ended_queue);
static void *
allocate_back(struct CCC_Flat_double_ended_queue *flat_double_ended_queue);
static size_t min(size_t a, size_t b);

/*==========================     Interface    ===============================*/

void *
CCC_flat_double_ended_queue_push_back(
    CCC_Flat_double_ended_queue *const flat_double_ended_queue,
    void const *const elem)
{
    if (!flat_double_ended_queue || !elem)
    {
        return NULL;
    }
    void *const slot = allocate_back(flat_double_ended_queue);
    if (slot && slot != elem)
    {
        (void)memcpy(slot, elem, flat_double_ended_queue->buf.sizeof_type);
    }
    return slot;
}

void *
CCC_flat_double_ended_queue_push_front(
    CCC_Flat_double_ended_queue *const flat_double_ended_queue,
    void const *const elem)
{
    if (!flat_double_ended_queue || !elem)
    {
        return NULL;
    }
    void *const slot = allocate_front(flat_double_ended_queue);
    if (slot && slot != elem)
    {
        (void)memcpy(slot, elem, flat_double_ended_queue->buf.sizeof_type);
    }
    return slot;
}

CCC_Result
CCC_flat_double_ended_queue_push_front_range(
    CCC_Flat_double_ended_queue *const flat_double_ended_queue, size_t const n,
    void const *const elems)
{
    if (!flat_double_ended_queue || !elems)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    return push_front_range(flat_double_ended_queue, n, elems);
}

CCC_Result
CCC_flat_double_ended_queue_push_back_range(
    CCC_Flat_double_ended_queue *const flat_double_ended_queue, size_t const n,
    void const *elems)
{
    if (!flat_double_ended_queue || !elems)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    return push_back_range(flat_double_ended_queue, n, elems);
}

void *
CCC_flat_double_ended_queue_insert_range(
    CCC_Flat_double_ended_queue *const flat_double_ended_queue, void *const pos,
    size_t const n, void const *elems)
{
    if (!flat_double_ended_queue || !elems)
    {
        return NULL;
    }
    if (!n)
    {
        return pos;
    }
    if (pos == CCC_flat_double_ended_queue_begin(flat_double_ended_queue))
    {
        return push_front_range(flat_double_ended_queue, n, elems)
                    != CCC_RESULT_OK
                 ? NULL
                 : at(flat_double_ended_queue, n - 1);
    }
    if (pos == CCC_flat_double_ended_queue_end(flat_double_ended_queue))
    {
        return push_back_range(flat_double_ended_queue, n, elems)
                    != CCC_RESULT_OK
                 ? NULL
                 : at(flat_double_ended_queue,
                      flat_double_ended_queue->buf.count - n);
    }
    return push_range(flat_double_ended_queue, pos, n, elems);
}

CCC_Result
CCC_flat_double_ended_queue_pop_front(
    CCC_Flat_double_ended_queue *const flat_double_ended_queue)
{
    if (!flat_double_ended_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (CCC_buffer_is_empty(&flat_double_ended_queue->buf))
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    flat_double_ended_queue->front
        = increment(flat_double_ended_queue, flat_double_ended_queue->front);
    return CCC_buffer_size_minus(&flat_double_ended_queue->buf, 1);
}

CCC_Result
CCC_flat_double_ended_queue_pop_back(
    CCC_Flat_double_ended_queue *const flat_double_ended_queue)
{
    if (!flat_double_ended_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    return CCC_buffer_size_minus(&flat_double_ended_queue->buf, 1);
}

void *
CCC_flat_double_ended_queue_front(
    CCC_Flat_double_ended_queue const *const flat_double_ended_queue)
{
    if (!flat_double_ended_queue
        || CCC_buffer_is_empty(&flat_double_ended_queue->buf))
    {
        return NULL;
    }
    return CCC_buffer_at(&flat_double_ended_queue->buf,
                         flat_double_ended_queue->front);
}

void *
CCC_flat_double_ended_queue_back(
    CCC_Flat_double_ended_queue const *const flat_double_ended_queue)
{
    if (!flat_double_ended_queue
        || CCC_buffer_is_empty(&flat_double_ended_queue->buf))
    {
        return NULL;
    }
    return CCC_buffer_at(&flat_double_ended_queue->buf,
                         last_node_index(flat_double_ended_queue));
}

CCC_Tribool
CCC_flat_double_ended_queue_is_empty(
    CCC_Flat_double_ended_queue const *const flat_double_ended_queue)
{
    if (!flat_double_ended_queue)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !flat_double_ended_queue->buf.count;
}

CCC_Count
CCC_flat_double_ended_queue_count(
    CCC_Flat_double_ended_queue const *const flat_double_ended_queue)
{
    if (!flat_double_ended_queue)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = flat_double_ended_queue->buf.count};
}

CCC_Count
CCC_flat_double_ended_queue_capacity(
    CCC_Flat_double_ended_queue const *const flat_double_ended_queue)
{
    if (!flat_double_ended_queue)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = flat_double_ended_queue->buf.capacity};
}

void *
CCC_flat_double_ended_queue_at(
    CCC_Flat_double_ended_queue const *const flat_double_ended_queue,
    size_t const i)
{
    if (!flat_double_ended_queue || i >= flat_double_ended_queue->buf.capacity)
    {
        return NULL;
    }
    return CCC_buffer_at(&flat_double_ended_queue->buf,
                         (flat_double_ended_queue->front + i)
                             % flat_double_ended_queue->buf.capacity);
}

void *
CCC_flat_double_ended_queue_begin(
    CCC_Flat_double_ended_queue const *const flat_double_ended_queue)
{
    if (!flat_double_ended_queue || !flat_double_ended_queue->buf.count)
    {
        return NULL;
    }
    return CCC_buffer_at(&flat_double_ended_queue->buf,
                         flat_double_ended_queue->front);
}

void *
CCC_flat_double_ended_queue_reverse_begin(
    CCC_Flat_double_ended_queue const *const flat_double_ended_queue)
{
    if (!flat_double_ended_queue || !flat_double_ended_queue->buf.count)
    {
        return NULL;
    }
    return CCC_buffer_at(&flat_double_ended_queue->buf,
                         last_node_index(flat_double_ended_queue));
}

void *
CCC_flat_double_ended_queue_next(
    CCC_Flat_double_ended_queue const *const flat_double_ended_queue,
    void const *const iterator_pointer)
{
    size_t const next_i
        = increment(flat_double_ended_queue,
                    index_of(flat_double_ended_queue, iterator_pointer));
    if (next_i == flat_double_ended_queue->front
        || distance(flat_double_ended_queue, next_i,
                    flat_double_ended_queue->front)
               >= flat_double_ended_queue->buf.count)
    {
        return NULL;
    }
    return CCC_buffer_at(&flat_double_ended_queue->buf, next_i);
}

void *
CCC_flat_double_ended_queue_reverse_next(
    CCC_Flat_double_ended_queue const *const flat_double_ended_queue,
    void const *const iterator_pointer)
{
    size_t const cur_i = index_of(flat_double_ended_queue, iterator_pointer);
    size_t const next_i = decrement(flat_double_ended_queue, cur_i);
    size_t const reverse_begin = last_node_index(flat_double_ended_queue);
    if (next_i == reverse_begin
        || rdistance(flat_double_ended_queue, next_i, reverse_begin)
               >= flat_double_ended_queue->buf.count)
    {
        return NULL;
    }
    return CCC_buffer_at(&flat_double_ended_queue->buf, next_i);
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
CCC_flat_double_ended_queue_data(
    CCC_Flat_double_ended_queue const *const flat_double_ended_queue)
{
    return flat_double_ended_queue
             ? CCC_buffer_begin(&flat_double_ended_queue->buf)
             : NULL;
}

CCC_Result
CCC_flat_double_ended_queue_copy(
    CCC_Flat_double_ended_queue *const destination,
    CCC_Flat_double_ended_queue const *const source, CCC_Allocator *const fn)
{
    if (!destination || !source || source == destination
        || (destination->buf.capacity < source->buf.capacity && !fn))
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* Copy everything so we don't worry about staying in sync with future
       changes to buf container. But we have to give back original destination
       memory in case it has already been allocated. Alloc will remain the
       same as in destination initialization because that controls whether the
       flat_double_ended_queue is a ring Buffer or dynamic
       flat_double_ended_queue. */
    void *const destination_data = destination->buf.data;
    size_t const destination_cap = destination->buf.capacity;
    CCC_Allocator *const destination_allocate = destination->buf.allocate;
    *destination = *source;
    destination->buf.data = destination_data;
    destination->buf.capacity = destination_cap;
    destination->buf.allocate = destination_allocate;
    /* Copying from an empty source is odd but supported. */
    if (!source->buf.capacity)
    {
        return CCC_RESULT_OK;
    }
    if (destination->buf.capacity < source->buf.capacity)
    {
        CCC_Result resize_res
            = CCC_buffer_allocate(&destination->buf, source->buf.capacity, fn);
        if (resize_res != CCC_RESULT_OK)
        {
            return resize_res;
        }
        destination->buf.capacity = source->buf.capacity;
    }
    if (!destination->buf.data || !source->buf.data)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (destination->buf.capacity > source->buf.capacity)
    {
        size_t const first_chunk
            = min(source->buf.count, source->buf.capacity - source->front);
        (void)memcpy(destination->buf.data,
                     CCC_buffer_at(&source->buf, source->front),
                     source->buf.sizeof_type * first_chunk);
        if (first_chunk < source->buf.count)
        {
            (void)memcpy((char *)destination->buf.data
                             + (source->buf.sizeof_type * first_chunk),
                         source->buf.data,
                         source->buf.sizeof_type
                             * (source->buf.count - first_chunk));
        }
        destination->front = 0;
        return CCC_RESULT_OK;
    }
    (void)memcpy(destination->buf.data, source->buf.data,
                 source->buf.capacity * source->buf.sizeof_type);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_flat_double_ended_queue_reserve(
    CCC_Flat_double_ended_queue *const flat_double_ended_queue,
    size_t const to_add, CCC_Allocator *const fn)
{
    if (!flat_double_ended_queue || !fn)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    return maybe_resize(flat_double_ended_queue, to_add, fn);
}

CCC_Result
CCC_flat_double_ended_queue_clear(
    CCC_Flat_double_ended_queue *const flat_double_ended_queue,
    CCC_Type_destructor *const destructor)
{
    if (!flat_double_ended_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destructor)
    {
        flat_double_ended_queue->front = 0;
        return CCC_buffer_size_set(&flat_double_ended_queue->buf, 0);
    }
    size_t const back = back_free_slot(flat_double_ended_queue);
    for (size_t i = flat_double_ended_queue->front; i != back;
         i = increment(flat_double_ended_queue, i))
    {
        destructor((CCC_Type_context){
            .type = CCC_buffer_at(&flat_double_ended_queue->buf, i),
            .context = flat_double_ended_queue->buf.context,
        });
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_flat_double_ended_queue_clear_and_free(
    CCC_Flat_double_ended_queue *const flat_double_ended_queue,
    CCC_Type_destructor *const destructor)
{
    if (!flat_double_ended_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destructor)
    {
        flat_double_ended_queue->buf.count = flat_double_ended_queue->front = 0;
        return CCC_buffer_allocate(&flat_double_ended_queue->buf, 0,
                                   flat_double_ended_queue->buf.allocate);
    }
    size_t const back = back_free_slot(flat_double_ended_queue);
    for (size_t i = flat_double_ended_queue->front; i != back;
         i = increment(flat_double_ended_queue, i))
    {
        destructor((CCC_Type_context){
            .type = CCC_buffer_at(&flat_double_ended_queue->buf, i),
            .context = flat_double_ended_queue->buf.context,
        });
    }
    CCC_Result const r
        = CCC_buffer_allocate(&flat_double_ended_queue->buf, 0,
                              flat_double_ended_queue->buf.allocate);
    if (r == CCC_RESULT_OK)
    {
        flat_double_ended_queue->buf.count = flat_double_ended_queue->front = 0;
    }
    return r;
}

CCC_Result
CCC_flat_double_ended_queue_clear_and_free_reserve(
    CCC_Flat_double_ended_queue *const flat_double_ended_queue,
    CCC_Type_destructor *const destructor, CCC_Allocator *const allocate)
{
    if (!flat_double_ended_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destructor)
    {
        flat_double_ended_queue->buf.count = flat_double_ended_queue->front = 0;
        return CCC_buffer_allocate(&flat_double_ended_queue->buf, 0, allocate);
    }
    size_t const back = back_free_slot(flat_double_ended_queue);
    for (size_t i = flat_double_ended_queue->front; i != back;
         i = increment(flat_double_ended_queue, i))
    {
        destructor((CCC_Type_context){
            .type = CCC_buffer_at(&flat_double_ended_queue->buf, i),
            .context = flat_double_ended_queue->buf.context,
        });
    }
    CCC_Result const r
        = CCC_buffer_allocate(&flat_double_ended_queue->buf, 0, allocate);
    if (r == CCC_RESULT_OK)
    {
        flat_double_ended_queue->buf.count = flat_double_ended_queue->front = 0;
    }
    return r;
}

CCC_Tribool
CCC_flat_double_ended_queue_validate(
    CCC_Flat_double_ended_queue const *const flat_double_ended_queue)
{
    if (!flat_double_ended_queue)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (CCC_flat_double_ended_queue_is_empty(flat_double_ended_queue))
    {
        return CCC_TRUE;
    }
    void *iterator = CCC_flat_double_ended_queue_begin(flat_double_ended_queue);
    if (CCC_buffer_i(&flat_double_ended_queue->buf, iterator).count
        != flat_double_ended_queue->front)
    {
        return CCC_FALSE;
    }
    size_t size = 0;
    for (; iterator != CCC_flat_double_ended_queue_end(flat_double_ended_queue);
         iterator
         = CCC_flat_double_ended_queue_next(flat_double_ended_queue, iterator),
         ++size)
    {
        if (size
            >= CCC_flat_double_ended_queue_count(flat_double_ended_queue).count)
        {
            return CCC_FALSE;
        }
    }
    if (size
        != CCC_flat_double_ended_queue_count(flat_double_ended_queue).count)
    {
        return CCC_FALSE;
    }
    size = 0;
    iterator
        = CCC_flat_double_ended_queue_reverse_begin(flat_double_ended_queue);
    if (CCC_buffer_i(&flat_double_ended_queue->buf, iterator).count
        != last_node_index(flat_double_ended_queue))
    {
        return CCC_FALSE;
    }
    for (; iterator
           != CCC_flat_double_ended_queue_reverse_end(flat_double_ended_queue);
         iterator = CCC_flat_double_ended_queue_reverse_next(
             flat_double_ended_queue, iterator),
         ++size)
    {
        if (size
            >= CCC_flat_double_ended_queue_count(flat_double_ended_queue).count)
        {
            return CCC_FALSE;
        }
    }
    return size
        == CCC_flat_double_ended_queue_count(flat_double_ended_queue).count;
}

/*======================   Private Interface   ==============================*/

void *
CCC_private_flat_double_ended_queue_allocate_back(
    struct CCC_Flat_double_ended_queue *const flat_double_ended_queue)
{
    return allocate_back(flat_double_ended_queue);
}

void *
CCC_private_flat_double_ended_queue_allocate_front(
    struct CCC_Flat_double_ended_queue *const flat_double_ended_queue)
{
    return allocate_front(flat_double_ended_queue);
}

/*======================     Static Helpers    ==============================*/

static void *
allocate_front(
    struct CCC_Flat_double_ended_queue *const flat_double_ended_queue)
{
    CCC_Tribool const full = maybe_resize(flat_double_ended_queue, 1,
                                          flat_double_ended_queue->buf.allocate)
                          != CCC_RESULT_OK;
    /* Should have been able to resize. Bad error. */
    if (flat_double_ended_queue->buf.allocate && full)
    {
        return NULL;
    }
    flat_double_ended_queue->front = front_free_slot(
        flat_double_ended_queue->front, flat_double_ended_queue->buf.capacity);
    void *const new_slot = CCC_buffer_at(&flat_double_ended_queue->buf,
                                         flat_double_ended_queue->front);
    if (!full)
    {
        (void)CCC_buffer_size_plus(&flat_double_ended_queue->buf, 1);
    }
    return new_slot;
}

static void *
allocate_back(struct CCC_Flat_double_ended_queue *const flat_double_ended_queue)
{
    CCC_Tribool const full = maybe_resize(flat_double_ended_queue, 1,
                                          flat_double_ended_queue->buf.allocate)
                          != CCC_RESULT_OK;
    /* Should have been able to resize. Bad error. */
    if (flat_double_ended_queue->buf.allocate && full)
    {
        return NULL;
    }
    void *const new_slot = CCC_buffer_at(
        &flat_double_ended_queue->buf, back_free_slot(flat_double_ended_queue));
    /* If no reallocation policy is given we are a ring buffer. */
    if (full)
    {
        flat_double_ended_queue->front = increment(
            flat_double_ended_queue, flat_double_ended_queue->front);
    }
    else
    {
        (void)CCC_buffer_size_plus(&flat_double_ended_queue->buf, 1);
    }
    return new_slot;
}

static CCC_Result
push_back_range(
    struct CCC_Flat_double_ended_queue *const flat_double_ended_queue,
    size_t const n, char const *elems)
{
    size_t const sizeof_type = flat_double_ended_queue->buf.sizeof_type;
    CCC_Tribool const full = maybe_resize(flat_double_ended_queue, n,
                                          flat_double_ended_queue->buf.allocate)
                          != CCC_RESULT_OK;
    size_t const cap = flat_double_ended_queue->buf.capacity;
    if (flat_double_ended_queue->buf.allocate && full)
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    /* If a range is too large we can make various simplifications to preserve
       the final capacity elements. */
    if (n >= cap)
    {
        elems += ((n - cap) * sizeof_type);
        flat_double_ended_queue->front = 0;
        (void)memcpy(CCC_buffer_at(&flat_double_ended_queue->buf, 0), elems,
                     sizeof_type * cap);
        (void)CCC_buffer_size_set(&flat_double_ended_queue->buf, cap);
        return CCC_RESULT_OK;
    }
    size_t const new_size = flat_double_ended_queue->buf.count + n;
    size_t const back_slot = back_free_slot(flat_double_ended_queue);
    size_t const chunk = min(n, cap - back_slot);
    size_t const remainder_back_slot = (back_slot + chunk) % cap;
    size_t const remainder = (n - chunk);
    char const *const second_chunk = elems + (chunk * sizeof_type);
    (void)memcpy(CCC_buffer_at(&flat_double_ended_queue->buf, back_slot), elems,
                 chunk * sizeof_type);
    if (remainder)
    {
        (void)memcpy(
            CCC_buffer_at(&flat_double_ended_queue->buf, remainder_back_slot),
            second_chunk, remainder * sizeof_type);
    }
    if (new_size > cap)
    {
        flat_double_ended_queue->front
            = (flat_double_ended_queue->front + (new_size - cap)) % cap;
    }
    (void)CCC_buffer_size_set(&flat_double_ended_queue->buf,
                              min(cap, new_size));
    return CCC_RESULT_OK;
}

static CCC_Result
push_front_range(
    struct CCC_Flat_double_ended_queue *const flat_double_ended_queue,
    size_t const n, char const *elems)
{
    size_t const sizeof_type = flat_double_ended_queue->buf.sizeof_type;
    CCC_Tribool const full = maybe_resize(flat_double_ended_queue, n,
                                          flat_double_ended_queue->buf.allocate)
                          != CCC_RESULT_OK;
    size_t const cap = flat_double_ended_queue->buf.capacity;
    if (flat_double_ended_queue->buf.allocate && full)
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    /* If a range is too large we can make various simplifications to preserve
       the final capacity elements. */
    if (n >= cap)
    {
        elems += ((n - cap) * sizeof_type);
        flat_double_ended_queue->front = 0;
        (void)memcpy(CCC_buffer_at(&flat_double_ended_queue->buf, 0), elems,
                     sizeof_type * cap);
        (void)CCC_buffer_size_set(&flat_double_ended_queue->buf, cap);
        return CCC_RESULT_OK;
    }
    size_t const space_ahead
        = front_free_slot(flat_double_ended_queue->front, cap) + 1;
    size_t const i = n > space_ahead ? 0 : space_ahead - n;
    size_t const chunk = min(n, space_ahead);
    size_t const remainder = (n - chunk);
    char const *const first_chunk = elems + ((n - chunk) * sizeof_type);
    (void)memcpy(CCC_buffer_at(&flat_double_ended_queue->buf, i), first_chunk,
                 chunk * sizeof_type);
    if (remainder)
    {
        (void)memcpy(
            CCC_buffer_at(&flat_double_ended_queue->buf, cap - remainder),
            elems, remainder * sizeof_type);
    }
    (void)CCC_buffer_size_set(&flat_double_ended_queue->buf,
                              min(cap, flat_double_ended_queue->buf.count + n));
    flat_double_ended_queue->front = remainder ? cap - remainder : i;
    return CCC_RESULT_OK;
}

static void *
push_range(struct CCC_Flat_double_ended_queue *const flat_double_ended_queue,
           char const *const pos, size_t n, char const *elems)
{
    size_t const sizeof_type = flat_double_ended_queue->buf.sizeof_type;
    CCC_Tribool const full = maybe_resize(flat_double_ended_queue, n,
                                          flat_double_ended_queue->buf.allocate)
                          != CCC_RESULT_OK;
    if (flat_double_ended_queue->buf.allocate && full)
    {
        return NULL;
    }
    size_t const cap = flat_double_ended_queue->buf.capacity;
    size_t const new_size = flat_double_ended_queue->buf.count + n;
    if (n >= cap)
    {
        elems += ((n - cap) * sizeof_type);
        flat_double_ended_queue->front = 0;
        void *const ret = CCC_buffer_at(&flat_double_ended_queue->buf, 0);
        (void)memcpy(ret, elems, sizeof_type * cap);
        (void)CCC_buffer_size_set(&flat_double_ended_queue->buf, cap);
        return ret;
    }
    size_t const pos_i = index_of(flat_double_ended_queue, pos);
    size_t const back = back_free_slot(flat_double_ended_queue);
    size_t const to_move = back > pos_i ? back - pos_i : cap - pos_i + back;
    size_t const move_i = (pos_i + n) % cap;
    size_t move_chunk = move_i + to_move > cap ? cap - move_i : to_move;
    move_chunk = back < pos_i ? min(cap - pos_i, move_chunk)
                              : min(back - pos_i, move_chunk);
    size_t const move_remain = to_move - move_chunk;
    (void)memmove(CCC_buffer_at(&flat_double_ended_queue->buf, move_i),
                  CCC_buffer_at(&flat_double_ended_queue->buf, pos_i),
                  move_chunk * sizeof_type);
    if (move_remain)
    {
        size_t const move_remain_i = (move_i + move_chunk) % cap;
        size_t const remaining_start_i = (pos_i + move_chunk) % cap;
        (void)memmove(
            CCC_buffer_at(&flat_double_ended_queue->buf, move_remain_i),
            CCC_buffer_at(&flat_double_ended_queue->buf, remaining_start_i),
            move_remain * sizeof_type);
    }
    size_t const elems_chunk = min(n, cap - pos_i);
    size_t const elems_remain = n - elems_chunk;
    (void)memcpy(CCC_buffer_at(&flat_double_ended_queue->buf, pos_i), elems,
                 elems_chunk * sizeof_type);
    if (elems_remain)
    {
        char const *const second_chunk = elems + (elems_chunk * sizeof_type);
        size_t const second_chunk_i = (pos_i + elems_chunk) % cap;
        (void)memcpy(
            CCC_buffer_at(&flat_double_ended_queue->buf, second_chunk_i),
            second_chunk, elems_remain * sizeof_type);
    }
    if (new_size > cap)
    {
        /* Wrapping behavior stops if it would overwrite the start of the
           range being inserted. This is to preserve as much info about
           the range as possible. If wrapping occurs the range is the new
           front. */
        size_t const excess = (new_size - cap);
        size_t const front_to_pos_dist
            = (pos_i + cap - flat_double_ended_queue->front) % cap;
        flat_double_ended_queue->front
            = (flat_double_ended_queue->front + min(excess, front_to_pos_dist))
            % cap;
    }
    (void)CCC_buffer_size_set(&flat_double_ended_queue->buf,
                              min(cap, new_size));
    return CCC_buffer_at(&flat_double_ended_queue->buf, pos_i);
}

static CCC_Result
maybe_resize(struct CCC_Flat_double_ended_queue *const q,
             size_t const additional_nodes_to_add, CCC_Allocator *const fn)
{
    size_t required = q->buf.count + additional_nodes_to_add;
    if (required < q->buf.count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (required <= q->buf.capacity)
    {
        return CCC_RESULT_OK;
    }
    if (!fn)
    {
        return CCC_RESULT_NO_ALLOCATION_FUNCTION;
    }
    size_t const sizeof_type = q->buf.sizeof_type;
    if (!q->buf.capacity && additional_nodes_to_add == 1)
    {
        required = START_CAPACITY;
    }
    else if (additional_nodes_to_add == 1)
    {
        required = q->buf.capacity * 2;
    }
    void *const new_data = fn((CCC_Allocator_context){
        .input = NULL,
        .bytes = sizeof_type * required,
        .context = q->buf.context,
    });
    if (!new_data)
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    if (q->buf.count)
    {
        size_t const first_chunk
            = min(q->buf.count, q->buf.capacity - q->front);
        (void)memcpy(new_data, CCC_buffer_at(&q->buf, q->front),
                     sizeof_type * first_chunk);
        if (first_chunk < q->buf.count)
        {
            (void)memcpy((char *)new_data + (sizeof_type * first_chunk),
                         CCC_buffer_begin(&q->buf),
                         sizeof_type * (q->buf.count - first_chunk));
        }
    }
    (void)CCC_buffer_allocate(&q->buf, 0, fn);
    q->buf.data = new_data;
    q->front = 0;
    q->buf.capacity = required;
    return CCC_RESULT_OK;
}

/** Returns the distance between the current iterator position and the origin
position. Distance is calculated in ascending indices, meaning the result is
the number of forward steps in the Buffer origin would need to take reach
iterator, possibly accounting for wrapping around the end of the buffer. */
static inline size_t
distance(
    struct CCC_Flat_double_ended_queue const *const flat_double_ended_queue,
    size_t const iterator, size_t const origin)
{
    return iterator > origin
             ? iterator - origin
             : (flat_double_ended_queue->buf.capacity - origin) + iterator;
}

/** Returns the rdistance between the current iterator position and the origin
position. Rdistance is calculated in descending indices, meaning the result is
the number of backward steps in the Buffer origin would need to take to reach
iterator, possibly accounting for wrapping around the beginning of buffer. */
static inline size_t
rdistance(
    struct CCC_Flat_double_ended_queue const *const flat_double_ended_queue,
    size_t const iterator, size_t const origin)
{
    return iterator > origin
             ? (flat_double_ended_queue->buf.capacity - iterator) + origin
             : origin - iterator;
}

static inline size_t
index_of(
    struct CCC_Flat_double_ended_queue const *const flat_double_ended_queue,
    void const *const pos)
{
    assert(pos >= CCC_buffer_begin(&flat_double_ended_queue->buf));
    assert(pos < CCC_buffer_capacity_end(&flat_double_ended_queue->buf));
    return (size_t)(((char *)pos
                     - (char *)CCC_buffer_begin(&flat_double_ended_queue->buf))
                    / flat_double_ended_queue->buf.sizeof_type);
}

static inline void *
at(struct CCC_Flat_double_ended_queue const *const flat_double_ended_queue,
   size_t const i)
{
    return CCC_buffer_at(&flat_double_ended_queue->buf,
                         (flat_double_ended_queue->front + i)
                             % flat_double_ended_queue->buf.capacity);
}

static inline size_t
increment(
    struct CCC_Flat_double_ended_queue const *const flat_double_ended_queue,
    size_t const i)
{
    return i == (flat_double_ended_queue->buf.capacity - 1) ? 0 : i + 1;
}

static inline size_t
decrement(
    struct CCC_Flat_double_ended_queue const *const flat_double_ended_queue,
    size_t const i)
{
    return i ? i - 1 : flat_double_ended_queue->buf.capacity - 1;
}

static inline size_t
back_free_slot(
    struct CCC_Flat_double_ended_queue const *const flat_double_ended_queue)
{
    return (flat_double_ended_queue->front + flat_double_ended_queue->buf.count)
         % flat_double_ended_queue->buf.capacity;
}

static inline size_t
front_free_slot(size_t const front, size_t const cap)
{
    return front ? front - 1 : cap - 1;
}

/** Returns index of the last element in the flat_double_ended_queue or front if
 * empty. */
static inline size_t
last_node_index(
    struct CCC_Flat_double_ended_queue const *const flat_double_ended_queue)
{
    return flat_double_ended_queue->buf.count
             ? (flat_double_ended_queue->front
                + flat_double_ended_queue->buf.count - 1)
                   % flat_double_ended_queue->buf.capacity
             : flat_double_ended_queue->front;
}

static inline size_t
min(size_t const a, size_t const b)
{
    return a < b ? a : b;
}
