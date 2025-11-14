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
CCC_buffer_allocate(CCC_Buffer *const buf, size_t const capacity,
                    CCC_Allocator *const fn)
{
    if (!buf)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!fn)
    {
        return CCC_RESULT_NO_ALLOCATION_FUNCTION;
    }
    void *const new_mem = fn((CCC_Allocator_context){
        .input = buf->mem,
        .bytes = buf->sizeof_type * capacity,
        .context = buf->context,
    });
    if (capacity && !new_mem)
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    buf->mem = new_mem;
    buf->capacity = capacity;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_reserve(CCC_Buffer *const buf, size_t const to_add,
                   CCC_Allocator *const fn)
{
    if (!buf || !fn)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    size_t needed = buf->count + to_add;
    if (needed <= buf->capacity)
    {
        return CCC_RESULT_OK;
    }
    if (needed < START_CAPACITY)
    {
        needed = START_CAPACITY;
    }
    void *const new_mem = fn((CCC_Allocator_context){
        .input = buf->mem,
        .bytes = buf->sizeof_type * needed,
        .context = buf->context,
    });
    if (!new_mem)
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    buf->mem = new_mem;
    buf->capacity = needed;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_clear(CCC_Buffer *const buf, CCC_Type_destructor *const destructor)
{
    if (!buf)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destructor)
    {
        buf->count = 0;
        return CCC_RESULT_OK;
    }
    for (void *i = CCC_buffer_begin(buf); i != CCC_buffer_end(buf);
         i = CCC_buffer_next(buf, i))
    {
        destructor((CCC_Type_context){
            .type = i,
            .context = buf->context,
        });
    }
    buf->count = 0;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_clear_and_free(CCC_Buffer *const buf,
                          CCC_Type_destructor *const destructor)
{
    if (!buf || !buf->allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destructor)
    {
        (void)buf->allocate((CCC_Allocator_context){
            .input = buf->mem,
            .bytes = 0,
            .context = buf->context,
        });
        buf->mem = NULL;
        buf->count = 0;
        buf->capacity = 0;
        return CCC_RESULT_OK;
    }
    for (void *i = CCC_buffer_begin(buf); i != CCC_buffer_end(buf);
         i = CCC_buffer_next(buf, i))
    {
        destructor((CCC_Type_context){
            .type = i,
            .context = buf->context,
        });
    }
    (void)buf->allocate((CCC_Allocator_context){
        .input = buf->mem,
        .bytes = 0,
        .context = buf->context,
    });
    buf->mem = NULL;
    buf->count = 0;
    buf->capacity = 0;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_clear_and_free_reserve(CCC_Buffer *const buf,
                                  CCC_Type_destructor *const destructor,
                                  CCC_Allocator *const allocate)
{
    if (!buf || !allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destructor)
    {
        (void)allocate((CCC_Allocator_context){
            .input = buf->mem,
            .bytes = 0,
            .context = buf->context,
        });
        buf->mem = NULL;
        buf->count = 0;
        buf->capacity = 0;
        return CCC_RESULT_OK;
    }
    for (void *i = CCC_buffer_begin(buf); i != CCC_buffer_end(buf);
         i = CCC_buffer_next(buf, i))
    {
        destructor((CCC_Type_context){
            .type = i,
            .context = buf->context,
        });
    }
    (void)allocate((CCC_Allocator_context){
        .input = buf->mem,
        .bytes = 0,
        .context = buf->context,
    });
    buf->mem = NULL;
    buf->count = 0;
    buf->capacity = 0;
    return CCC_RESULT_OK;
}

void *
CCC_buffer_at(CCC_Buffer const *const buf, size_t const i)
{
    if (!buf || i >= buf->capacity)
    {
        return NULL;
    }
    return ((char *)buf->mem + (i * buf->sizeof_type));
}

void *
CCC_buffer_back(CCC_Buffer const *const buf)
{
    return CCC_buffer_at(buf, buf->count - 1);
}

void *
CCC_buffer_front(CCC_Buffer const *const buf)
{
    return CCC_buffer_at(buf, 0);
}

void *
CCC_buffer_allocate_back(CCC_Buffer *const buf)
{
    if (!buf)
    {
        return NULL;
    }
    if (buf->count == buf->capacity)
    {
        CCC_Result const resize_res = CCC_buffer_allocate(
            buf, max(buf->capacity * 2, START_CAPACITY), buf->allocate);
        if (resize_res != CCC_RESULT_OK)
        {
            return NULL;
        }
    }
    void *const ret = ((char *)buf->mem + (buf->sizeof_type * buf->count));
    ++buf->count;
    return ret;
}

void *
CCC_buffer_push_back(CCC_Buffer *const buf, void const *const data)
{
    void *const mem = CCC_buffer_allocate_back(buf);
    if (mem)
    {
        (void)memcpy(mem, data, buf->sizeof_type);
    }
    return mem;
}

CCC_Result
CCC_buffer_swap(CCC_Buffer *const buf, void *const tmp, size_t const i,
                size_t const j)
{
    if (!buf || !tmp || i >= buf->capacity || j >= buf->capacity || j == i)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    (void)memcpy(tmp, at(buf, i), buf->sizeof_type);
    (void)memcpy(at(buf, i), at(buf, j), buf->sizeof_type);
    (void)memcpy(at(buf, j), tmp, buf->sizeof_type);
    return CCC_RESULT_OK;
}

void *
CCC_buffer_move(CCC_Buffer *const buf, size_t const dst, size_t const src)
{
    if (!buf || dst >= buf->capacity || src >= buf->capacity)
    {
        return NULL;
    }
    if (dst == src)
    {
        return at(buf, dst);
    }
    return memcpy(at(buf, dst), at(buf, src), buf->sizeof_type);
}

CCC_Result
CCC_buffer_write(CCC_Buffer *const buf, size_t const i, void const *const data)
{
    if (!buf || !buf->mem || !data)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    void *const pos = CCC_buffer_at(buf, i);
    if (!pos || data == pos)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    (void)memcpy(pos, data, buf->sizeof_type);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_erase(CCC_Buffer *const buf, size_t const i)
{
    if (!buf || !buf->count || i >= buf->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (1 == buf->count)
    {
        buf->count = 0;
        return CCC_RESULT_OK;
    }
    if (i == buf->count - 1)
    {
        --buf->count;
        return CCC_RESULT_OK;
    }
    (void)memmove(at(buf, i), at(buf, i + 1),
                  buf->sizeof_type * (buf->count - (i + 1)));
    --buf->count;
    return CCC_RESULT_OK;
}

void *
CCC_buffer_insert(CCC_Buffer *const buf, size_t const i, void const *const data)
{
    if (!buf || !buf->mem || i > buf->count)
    {
        return NULL;
    }
    if (i == buf->count)
    {
        return CCC_buffer_push_back(buf, data);
    }
    if (buf->count == buf->capacity)
    {
        CCC_Result const r = CCC_buffer_allocate(
            buf, max(buf->count * 2, START_CAPACITY), buf->allocate);
        if (r != CCC_RESULT_OK)
        {
            return NULL;
        }
    }
    (void)memmove(at(buf, i + 1), at(buf, i),
                  buf->sizeof_type * (buf->count - i));
    ++buf->count;
    return memcpy(at(buf, i), data, buf->sizeof_type);
}

CCC_Result
CCC_buffer_pop_back_n(CCC_Buffer *const buf, size_t n)
{
    if (!buf || n > buf->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    buf->count -= n;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_pop_back(CCC_Buffer *const buf)
{
    return CCC_buffer_pop_back_n(buf, 1);
}

CCC_Count
CCC_buffer_count(CCC_Buffer const *const buf)
{
    if (!buf)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = buf->count};
}

CCC_Count
CCC_buffer_capacity(CCC_Buffer const *const buf)
{
    if (!buf)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = buf->capacity};
}

CCC_Count
CCC_buffer_sizeof_type(CCC_Buffer const *const buf)
{
    if (!buf)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = buf->sizeof_type};
}

CCC_Tribool
CCC_buffer_is_empty(CCC_Buffer const *const buf)
{
    if (!buf)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !buf->count;
}

CCC_Tribool
CCC_buffer_is_full(CCC_Buffer const *const buf)
{
    if (!buf)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (!buf->capacity)
    {
        return CCC_FALSE;
    }
    return buf->count == buf->capacity ? CCC_TRUE : CCC_FALSE;
}

void *
CCC_buffer_begin(CCC_Buffer const *const buf)
{
    return buf ? buf->mem : NULL;
}

void *
CCC_buffer_reverse_begin(CCC_Buffer const *const buf)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    /* OK if count is 0. Negative offset puts at reverse_end anyway. */
    return (unsigned char *)buf->mem + ((buf->count - 1) * buf->sizeof_type);
}

void *
CCC_buffer_next(CCC_Buffer const *const buf, void const *const iterator)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    if (iterator >= CCC_buffer_capacity_end(buf))
    {
        return CCC_buffer_end(buf);
    }
    return (unsigned char *)iterator + buf->sizeof_type;
}

void *
CCC_buffer_reverse_next(CCC_Buffer const *const buf, void const *const iterator)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    if (iterator <= CCC_buffer_reverse_end(buf))
    {
        return CCC_buffer_reverse_end(buf);
    }
    return (char *)iterator - buf->sizeof_type;
}

/** We accept that end may be the address past Buffer capacity. */
void *
CCC_buffer_end(CCC_Buffer const *const buf)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    return (unsigned char *)buf->mem + (buf->count * buf->sizeof_type);
}

/** We accept that reverse_end is out of bounds and the address before start.
Even if the array base was somehow 0 and wrapping occurred upon subtraction the
iterator would eventually reach this same address through reverse_next and be
compared to it in the main user loop. */
void *
CCC_buffer_reverse_end(CCC_Buffer const *const buf)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    return (unsigned char *)buf->mem - buf->sizeof_type;
}

/** Will always be the address after capacity. */
void *
CCC_buffer_capacity_end(CCC_Buffer const *const buf)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    return (unsigned char *)buf->mem + (buf->sizeof_type * buf->capacity);
}

CCC_Count
CCC_buffer_i(CCC_Buffer const *const buf, void const *const slot)
{
    if (!buf || !buf->mem || !slot || slot < buf->mem
        || (char *)slot
               >= ((char *)buf->mem + (buf->capacity * buf->sizeof_type)))
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){
        .count = (((char *)slot - ((char *)buf->mem)) / buf->sizeof_type),
    };
}

CCC_Result
CCC_buffer_size_plus(CCC_Buffer *const buf, size_t const n)
{
    if (!buf)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    size_t const new_count = buf->count + n;
    if (new_count > buf->capacity)
    {
        buf->count = buf->capacity;
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    buf->count = new_count;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_size_minus(CCC_Buffer *const buf, size_t const n)
{
    if (!buf)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (n > buf->count)
    {
        buf->count = 0;
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    buf->count -= n;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_buffer_size_set(CCC_Buffer *const buf, size_t const n)
{
    if (!buf)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (n > buf->capacity)
    {
        buf->count = buf->capacity;
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    buf->count = n;
    return CCC_RESULT_OK;
}

CCC_Count
CCC_buffer_count_bytes(CCC_Buffer const *buf)
{
    if (!buf)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = buf->count * buf->sizeof_type};
}

CCC_Count
CCC_buffer_capacity_bytes(CCC_Buffer const *buf)
{
    if (!buf)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = buf->capacity * buf->sizeof_type};
}

CCC_Result
CCC_buffer_copy(CCC_Buffer *const dst, CCC_Buffer const *const src,
                CCC_Allocator *const fn)
{
    if (!dst || !src || src == dst || (dst->capacity < src->capacity && !fn))
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* Copy everything so we don't worry about staying in sync with future
       changes to buf container. But we have to give back original destination
       memory in case it has already been allocated. Alloc will remain the
       same as in dst initialization because that controls permission. */
    void *const dst_mem = dst->mem;
    size_t const dst_cap = dst->capacity;
    CCC_Allocator *const dst_allocate = dst->allocate;
    *dst = *src;
    dst->mem = dst_mem;
    dst->capacity = dst_cap;
    dst->allocate = dst_allocate;
    if (!src->capacity)
    {
        return CCC_RESULT_OK;
    }
    if (dst->capacity < src->capacity)
    {
        CCC_Result const r = CCC_buffer_allocate(dst, src->capacity, fn);
        if (r != CCC_RESULT_OK)
        {
            return r;
        }
        dst->capacity = src->capacity;
    }
    if (!src->mem || !dst->mem)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    (void)memcpy(dst->mem, src->mem, src->capacity * src->sizeof_type);
    return CCC_RESULT_OK;
}

/*======================  Static Helpers  ==================================*/

static inline void *
at(struct CCC_Buffer const *const buf, size_t const i)
{
    return ((char *)buf->mem + (i * buf->sizeof_type));
}

static inline size_t
max(size_t const a, size_t const b)
{
    return a > b ? a : b;
}
