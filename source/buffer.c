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
#include <stddef.h>
#include <string.h>

#include "buffer.h"
#include "private/private_buffer.h"
#include "types.h"

enum : size_t
{
    START_CAPACITY = 8,
};

/*==========================   Prototypes    ================================*/

static void *at(CCC_Buffer const *, size_t);
static size_t max(size_t a, size_t b);

/*==========================    Interface    ================================*/

CCC_Result
CCC_buffer_allocate(CCC_Buffer *const buffer, size_t const capacity,
                    CCC_Allocator *const allocate)
{
    if (!buffer)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!allocate)
    {
        return CCC_RESULT_NO_ALLOCATION_FUNCTION;
    }
    void *const new_data = allocate((CCC_Allocator_context){
        .input = buffer->data,
        .bytes = buffer->sizeof_type * capacity,
        .context = buffer->context,
    });
    if (capacity && !new_data)
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    buffer->data = new_data;
    buffer->capacity = capacity;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_reserve(CCC_Buffer *const buffer, size_t const to_add,
                   CCC_Allocator *const allocate)
{
    if (!buffer || !allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    size_t needed = buffer->count + to_add;
    if (needed <= buffer->capacity)
    {
        return CCC_RESULT_OK;
    }
    if (needed < START_CAPACITY)
    {
        needed = START_CAPACITY;
    }
    void *const new_data = allocate((CCC_Allocator_context){
        .input = buffer->data,
        .bytes = buffer->sizeof_type * needed,
        .context = buffer->context,
    });
    if (!new_data)
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    buffer->data = new_data;
    buffer->capacity = needed;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_clear(CCC_Buffer *const buffer, CCC_Type_destructor *const destroy)
{
    if (!buffer)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destroy)
    {
        buffer->count = 0;
        return CCC_RESULT_OK;
    }
    for (void *i = CCC_buffer_begin(buffer); i != CCC_buffer_end(buffer);
         i = CCC_buffer_next(buffer, i))
    {
        destroy((CCC_Type_context){
            .type = i,
            .context = buffer->context,
        });
    }
    buffer->count = 0;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_clear_and_free(CCC_Buffer *const buffer,
                          CCC_Type_destructor *const destroy)
{
    if (!buffer || !buffer->allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destroy)
    {
        (void)buffer->allocate((CCC_Allocator_context){
            .input = buffer->data,
            .bytes = 0,
            .context = buffer->context,
        });
        buffer->data = NULL;
        buffer->count = 0;
        buffer->capacity = 0;
        return CCC_RESULT_OK;
    }
    for (void *i = CCC_buffer_begin(buffer); i != CCC_buffer_end(buffer);
         i = CCC_buffer_next(buffer, i))
    {
        destroy((CCC_Type_context){
            .type = i,
            .context = buffer->context,
        });
    }
    (void)buffer->allocate((CCC_Allocator_context){
        .input = buffer->data,
        .bytes = 0,
        .context = buffer->context,
    });
    buffer->data = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_clear_and_free_reserve(CCC_Buffer *const buffer,
                                  CCC_Type_destructor *const destroy,
                                  CCC_Allocator *const allocate)
{
    if (!buffer || !allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destroy)
    {
        (void)allocate((CCC_Allocator_context){
            .input = buffer->data,
            .bytes = 0,
            .context = buffer->context,
        });
        buffer->data = NULL;
        buffer->count = 0;
        buffer->capacity = 0;
        return CCC_RESULT_OK;
    }
    for (void *i = CCC_buffer_begin(buffer); i != CCC_buffer_end(buffer);
         i = CCC_buffer_next(buffer, i))
    {
        destroy((CCC_Type_context){
            .type = i,
            .context = buffer->context,
        });
    }
    (void)allocate((CCC_Allocator_context){
        .input = buffer->data,
        .bytes = 0,
        .context = buffer->context,
    });
    buffer->data = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
    return CCC_RESULT_OK;
}

void *
CCC_buffer_at(CCC_Buffer const *const buffer, size_t const i)
{
    if (!buffer || i >= buffer->capacity)
    {
        return NULL;
    }
    return ((char *)buffer->data + (i * buffer->sizeof_type));
}

void *
CCC_buffer_back(CCC_Buffer const *const buffer)
{
    return CCC_buffer_at(buffer, buffer->count - 1);
}

void *
CCC_buffer_front(CCC_Buffer const *const buffer)
{
    return CCC_buffer_at(buffer, 0);
}

void *
CCC_buffer_allocate_back(CCC_Buffer *const buffer)
{
    if (!buffer)
    {
        return NULL;
    }
    if (buffer->count == buffer->capacity)
    {
        CCC_Result const resize_res = CCC_buffer_allocate(
            buffer, max(buffer->capacity * 2, START_CAPACITY),
            buffer->allocate);
        if (resize_res != CCC_RESULT_OK)
        {
            return NULL;
        }
    }
    void *const ret
        = ((char *)buffer->data + (buffer->sizeof_type * buffer->count));
    ++buffer->count;
    return ret;
}

void *
CCC_buffer_push_back(CCC_Buffer *const buffer, void const *const data)
{
    void *const slot = CCC_buffer_allocate_back(buffer);
    if (slot)
    {
        (void)memcpy(slot, data, buffer->sizeof_type);
    }
    return slot;
}

CCC_Result
CCC_buffer_swap(CCC_Buffer *const buffer, void *const temp, size_t const index,
                size_t const swap_index)
{
    if (!buffer || !temp || index >= buffer->capacity
        || swap_index >= buffer->capacity || swap_index == index)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    (void)memcpy(temp, at(buffer, index), buffer->sizeof_type);
    (void)memcpy(at(buffer, index), at(buffer, swap_index),
                 buffer->sizeof_type);
    (void)memcpy(at(buffer, swap_index), temp, buffer->sizeof_type);
    return CCC_RESULT_OK;
}

void *
CCC_buffer_move(CCC_Buffer *const buffer, size_t const destination,
                size_t const source)
{
    if (!buffer || destination >= buffer->capacity
        || source >= buffer->capacity)
    {
        return NULL;
    }
    if (destination == source)
    {
        return at(buffer, destination);
    }
    return memcpy(at(buffer, destination), at(buffer, source),
                  buffer->sizeof_type);
}

CCC_Result
CCC_buffer_write(CCC_Buffer *const buffer, size_t const i,
                 void const *const data)
{
    if (!buffer || !buffer->data || !data)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    void *const pos = CCC_buffer_at(buffer, i);
    if (!pos || data == pos)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    (void)memcpy(pos, data, buffer->sizeof_type);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_erase(CCC_Buffer *const buffer, size_t const i)
{
    if (!buffer || !buffer->count || i >= buffer->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (1 == buffer->count)
    {
        buffer->count = 0;
        return CCC_RESULT_OK;
    }
    if (i == buffer->count - 1)
    {
        --buffer->count;
        return CCC_RESULT_OK;
    }
    (void)memmove(at(buffer, i), at(buffer, i + 1),
                  buffer->sizeof_type * (buffer->count - (i + 1)));
    --buffer->count;
    return CCC_RESULT_OK;
}

void *
CCC_buffer_insert(CCC_Buffer *const buffer, size_t const i,
                  void const *const data)
{
    if (!buffer || !buffer->data || i > buffer->count)
    {
        return NULL;
    }
    if (i == buffer->count)
    {
        return CCC_buffer_push_back(buffer, data);
    }
    if (buffer->count == buffer->capacity)
    {
        CCC_Result const r = CCC_buffer_allocate(
            buffer, max(buffer->count * 2, START_CAPACITY), buffer->allocate);
        if (r != CCC_RESULT_OK)
        {
            return NULL;
        }
    }
    (void)memmove(at(buffer, i + 1), at(buffer, i),
                  buffer->sizeof_type * (buffer->count - i));
    ++buffer->count;
    return memcpy(at(buffer, i), data, buffer->sizeof_type);
}

CCC_Result
CCC_buffer_pop_back_n(CCC_Buffer *const buffer, size_t count)
{
    if (!buffer || count > buffer->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    buffer->count -= count;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_pop_back(CCC_Buffer *const buffer)
{
    return CCC_buffer_pop_back_n(buffer, 1);
}

CCC_Count
CCC_buffer_count(CCC_Buffer const *const buffer)
{
    if (!buffer)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = buffer->count};
}

CCC_Count
CCC_buffer_capacity(CCC_Buffer const *const buffer)
{
    if (!buffer)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = buffer->capacity};
}

CCC_Count
CCC_buffer_sizeof_type(CCC_Buffer const *const buffer)
{
    if (!buffer)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = buffer->sizeof_type};
}

CCC_Tribool
CCC_buffer_is_empty(CCC_Buffer const *const buffer)
{
    if (!buffer)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !buffer->count;
}

CCC_Tribool
CCC_buffer_is_full(CCC_Buffer const *const buffer)
{
    if (!buffer)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (!buffer->capacity)
    {
        return CCC_FALSE;
    }
    return buffer->count == buffer->capacity ? CCC_TRUE : CCC_FALSE;
}

void *
CCC_buffer_begin(CCC_Buffer const *const buffer)
{
    return buffer ? buffer->data : NULL;
}

void *
CCC_buffer_reverse_begin(CCC_Buffer const *const buffer)
{
    if (!buffer || !buffer->data)
    {
        return NULL;
    }
    /* OK if count is 0. Negative offset puts at reverse_end anyway. */
    return (unsigned char *)buffer->data
         + ((buffer->count - 1) * buffer->sizeof_type);
}

void *
CCC_buffer_next(CCC_Buffer const *const buffer, void const *const iterator)
{
    if (!buffer || !buffer->data)
    {
        return NULL;
    }
    if (iterator >= CCC_buffer_capacity_end(buffer))
    {
        return CCC_buffer_end(buffer);
    }
    return (unsigned char *)iterator + buffer->sizeof_type;
}

void *
CCC_buffer_reverse_next(CCC_Buffer const *const buffer,
                        void const *const iterator)
{
    if (!buffer || !buffer->data)
    {
        return NULL;
    }
    if (iterator <= CCC_buffer_reverse_end(buffer))
    {
        return CCC_buffer_reverse_end(buffer);
    }
    return (char *)iterator - buffer->sizeof_type;
}

/** We accept that end may be the address past Buffer capacity. */
void *
CCC_buffer_end(CCC_Buffer const *const buffer)
{
    if (!buffer || !buffer->data)
    {
        return NULL;
    }
    return (unsigned char *)buffer->data
         + (buffer->count * buffer->sizeof_type);
}

/** We accept that reverse_end is out of bounds and the address before start.
Even if the array base was somehow 0 and wrapping occurred upon subtraction the
iterator would eventually reach this same address through reverse_next and be
compared to it in the main user loop. */
void *
CCC_buffer_reverse_end(CCC_Buffer const *const buffer)
{
    if (!buffer || !buffer->data)
    {
        return NULL;
    }
    return (unsigned char *)buffer->data - buffer->sizeof_type;
}

/** Will always be the address after capacity. */
void *
CCC_buffer_capacity_end(CCC_Buffer const *const buffer)
{
    if (!buffer || !buffer->data)
    {
        return NULL;
    }
    return (unsigned char *)buffer->data
         + (buffer->sizeof_type * buffer->capacity);
}

CCC_Count
CCC_buffer_index(CCC_Buffer const *const buffer, void const *const slot)
{
    if (!buffer || !buffer->data || !slot || slot < buffer->data
        || (char *)slot >= ((char *)buffer->data
                            + (buffer->capacity * buffer->sizeof_type)))
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){
        .count
        = (((char *)slot - ((char *)buffer->data)) / buffer->sizeof_type),
    };
}

CCC_Result
CCC_buffer_size_plus(CCC_Buffer *const buffer, size_t const count)
{
    if (!buffer)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    size_t const new_count = buffer->count + count;
    if (new_count > buffer->capacity)
    {
        buffer->count = buffer->capacity;
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    buffer->count = new_count;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_size_minus(CCC_Buffer *const buffer, size_t const count)
{
    if (!buffer)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (count > buffer->count)
    {
        buffer->count = 0;
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    buffer->count -= count;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_size_set(CCC_Buffer *const buffer, size_t const count)
{
    if (!buffer)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (count > buffer->capacity)
    {
        buffer->count = buffer->capacity;
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    buffer->count = count;
    return CCC_RESULT_OK;
}

CCC_Count
CCC_buffer_count_bytes(CCC_Buffer const *buffer)
{
    if (!buffer)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = buffer->count * buffer->sizeof_type};
}

CCC_Count
CCC_buffer_capacity_bytes(CCC_Buffer const *buffer)
{
    if (!buffer)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = buffer->capacity * buffer->sizeof_type};
}

CCC_Result
CCC_buffer_copy(CCC_Buffer *const destination, CCC_Buffer const *const source,
                CCC_Allocator *const allocate)
{
    if (!destination || !source || source == destination
        || (destination->capacity < source->capacity && !allocate))
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* Copy everything so we don't worry about staying in sync with future
       changes to buffer container. But we have to give back original
       destination memory in case it has already been allocated. Alloc will
       remain the same as in destination initialization because that controls
       permission.
     */
    void *const destination_data = destination->data;
    size_t const destination_cap = destination->capacity;
    CCC_Allocator *const destination_allocate = destination->allocate;
    *destination = *source;
    destination->data = destination_data;
    destination->capacity = destination_cap;
    destination->allocate = destination_allocate;
    if (!source->capacity)
    {
        return CCC_RESULT_OK;
    }
    if (destination->capacity < source->capacity)
    {
        CCC_Result const r
            = CCC_buffer_allocate(destination, source->capacity, allocate);
        if (r != CCC_RESULT_OK)
        {
            return r;
        }
        destination->capacity = source->capacity;
    }
    if (!source->data || !destination->data)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    (void)memcpy(destination->data, source->data,
                 source->capacity * source->sizeof_type);
    return CCC_RESULT_OK;
}

/*======================  Static Helpers  ==================================*/

static inline void *
at(struct CCC_Buffer const *const buffer, size_t const i)
{
    return ((char *)buffer->data + (i * buffer->sizeof_type));
}

static inline size_t
max(size_t const a, size_t const b)
{
    return a > b ? a : b;
}
