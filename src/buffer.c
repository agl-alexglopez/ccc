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
#include "impl/impl_buffer.h"
#include "types.h"

enum : size_t
{
    START_CAPACITY = 8,
};

/*==========================   Prototypes    ================================*/

static void *at(CCC_buffer const *, size_t);
static size_t max(size_t a, size_t b);

/*==========================    Interface    ================================*/

CCC_result
CCC_buf_alloc(CCC_buffer *const buf, size_t const capacity,
              CCC_any_alloc_fn *const fn)
{
    if (!buf)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!fn)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    void *const new_mem = fn(buf->mem, buf->sizeof_type * capacity, buf->aux);
    if (capacity && !new_mem)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    buf->mem = new_mem;
    buf->capacity = capacity;
    return CCC_RESULT_OK;
}

CCC_result
CCC_buf_reserve(CCC_buffer *const buf, size_t const to_add,
                CCC_any_alloc_fn *const fn)
{
    if (!buf || !fn)
    {
        return CCC_RESULT_ARG_ERROR;
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
    void *const new_mem = fn(buf->mem, buf->sizeof_type * needed, buf->aux);
    if (!new_mem)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    buf->mem = new_mem;
    buf->capacity = needed;
    return CCC_RESULT_OK;
}

CCC_result
CCC_buf_clear(CCC_buffer *const buf,
              CCC_any_type_destructor_fn *const destructor)
{
    if (!buf)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!destructor)
    {
        buf->count = 0;
        return CCC_RESULT_OK;
    }
    for (void *i = CCC_buf_begin(buf); i != CCC_buf_end(buf);
         i = CCC_buf_next(buf, i))
    {
        destructor((CCC_any_type){
            .any_type = i,
            .aux = buf->aux,
        });
    }
    buf->count = 0;
    return CCC_RESULT_OK;
}

CCC_result
CCC_buf_clear_and_free(CCC_buffer *const buf,
                       CCC_any_type_destructor_fn *const destructor)
{
    if (!buf || !buf->alloc)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!destructor)
    {
        (void)buf->alloc(buf->mem, 0, buf->aux);
        buf->mem = NULL;
        buf->count = 0;
        buf->capacity = 0;
        return CCC_RESULT_OK;
    }
    for (void *i = CCC_buf_begin(buf); i != CCC_buf_end(buf);
         i = CCC_buf_next(buf, i))
    {
        destructor((CCC_any_type){
            .any_type = i,
            .aux = buf->aux,
        });
    }
    (void)buf->alloc(buf->mem, 0, buf->aux);
    buf->mem = NULL;
    buf->count = 0;
    buf->capacity = 0;
    return CCC_RESULT_OK;
}

CCC_result
CCC_buf_clear_and_free_reserve(CCC_buffer *const buf,
                               CCC_any_type_destructor_fn *const destructor,
                               CCC_any_alloc_fn *const alloc)
{
    if (!buf || !alloc)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!destructor)
    {
        (void)alloc(buf->mem, 0, buf->aux);
        buf->mem = NULL;
        buf->count = 0;
        buf->capacity = 0;
        return CCC_RESULT_OK;
    }
    for (void *i = CCC_buf_begin(buf); i != CCC_buf_end(buf);
         i = CCC_buf_next(buf, i))
    {
        destructor((CCC_any_type){
            .any_type = i,
            .aux = buf->aux,
        });
    }
    (void)alloc(buf->mem, 0, buf->aux);
    buf->mem = NULL;
    buf->count = 0;
    buf->capacity = 0;
    return CCC_RESULT_OK;
}

void *
CCC_buf_at(CCC_buffer const *const buf, size_t const i)
{
    if (!buf || i >= buf->capacity)
    {
        return NULL;
    }
    return ((char *)buf->mem + (i * buf->sizeof_type));
}

void *
CCC_buf_back(CCC_buffer const *const buf)
{
    return CCC_buf_at(buf, buf->count - 1);
}

void *
CCC_buf_front(CCC_buffer const *const buf)
{
    return CCC_buf_at(buf, 0);
}

void *
CCC_buf_alloc_back(CCC_buffer *const buf)
{
    if (!buf)
    {
        return NULL;
    }
    if (buf->count == buf->capacity)
    {
        CCC_result const resize_res = CCC_buf_alloc(
            buf, max(buf->capacity * 2, START_CAPACITY), buf->alloc);
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
CCC_buf_push_back(CCC_buffer *const buf, void const *const data)
{
    void *const mem = CCC_buf_alloc_back(buf);
    if (mem)
    {
        (void)memcpy(mem, data, buf->sizeof_type);
    }
    return mem;
}

CCC_result
CCC_buf_swap(CCC_buffer *const buf, void *const tmp, size_t const i,
             size_t const j)
{
    if (!buf || !tmp || i >= buf->capacity || j >= buf->capacity || j == i)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    (void)memcpy(tmp, at(buf, i), buf->sizeof_type);
    (void)memcpy(at(buf, i), at(buf, j), buf->sizeof_type);
    (void)memcpy(at(buf, j), tmp, buf->sizeof_type);
    return CCC_RESULT_OK;
}

void *
CCC_buf_move(CCC_buffer *const buf, size_t const dst, size_t const src)
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

CCC_result
CCC_buf_write(CCC_buffer *const buf, size_t const i, void const *const data)
{
    if (!buf || !buf->mem || !data)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    void *const pos = CCC_buf_at(buf, i);
    if (!pos || data == pos)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    (void)memcpy(pos, data, buf->sizeof_type);
    return CCC_RESULT_OK;
}

CCC_result
CCC_buf_erase(CCC_buffer *const buf, size_t const i)
{
    if (!buf || !buf->count || i >= buf->count)
    {
        return CCC_RESULT_ARG_ERROR;
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
CCC_buf_insert(CCC_buffer *const buf, size_t const i, void const *const data)
{
    if (!buf || !buf->mem || i > buf->count)
    {
        return NULL;
    }
    if (i == buf->count)
    {
        return CCC_buf_push_back(buf, data);
    }
    if (buf->count == buf->capacity)
    {
        CCC_result const r = CCC_buf_alloc(
            buf, max(buf->count * 2, START_CAPACITY), buf->alloc);
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

CCC_result
CCC_buf_pop_back_n(CCC_buffer *const buf, size_t n)
{
    if (!buf || n > buf->count)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    buf->count -= n;
    return CCC_RESULT_OK;
}

CCC_result
CCC_buf_pop_back(CCC_buffer *const buf)
{
    return CCC_buf_pop_back_n(buf, 1);
}

CCC_ucount
CCC_buf_count(CCC_buffer const *const buf)
{
    if (!buf)
    {
        return (CCC_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (CCC_ucount){.count = buf->count};
}

CCC_ucount
CCC_buf_capacity(CCC_buffer const *const buf)
{
    if (!buf)
    {
        return (CCC_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (CCC_ucount){.count = buf->capacity};
}

CCC_ucount
CCC_buf_sizeof_type(CCC_buffer const *const buf)
{
    if (!buf)
    {
        return (CCC_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (CCC_ucount){.count = buf->sizeof_type};
}

CCC_tribool
CCC_buf_is_empty(CCC_buffer const *const buf)
{
    if (!buf)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !buf->count;
}

CCC_tribool
CCC_buf_is_full(CCC_buffer const *const buf)
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
CCC_buf_begin(CCC_buffer const *const buf)
{
    return buf ? buf->mem : NULL;
}

void *
CCC_buf_rbegin(CCC_buffer const *const buf)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    /* OK if count is 0. Negative offset puts at rend anyway. */
    return (unsigned char *)buf->mem + ((buf->count - 1) * buf->sizeof_type);
}

void *
CCC_buf_next(CCC_buffer const *const buf, void const *const iter)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    if (iter >= CCC_buf_capacity_end(buf))
    {
        return CCC_buf_end(buf);
    }
    return (unsigned char *)iter + buf->sizeof_type;
}

void *
CCC_buf_rnext(CCC_buffer const *const buf, void const *const iter)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    if (iter <= CCC_buf_rend(buf))
    {
        return CCC_buf_rend(buf);
    }
    return (char *)iter - buf->sizeof_type;
}

/** We accept that end may be the address past buffer capacity. */
void *
CCC_buf_end(CCC_buffer const *const buf)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    return (unsigned char *)buf->mem + (buf->count * buf->sizeof_type);
}

/** We accept that rend is out of bounds and the address before start. Even if
the array base was somehow 0 and wrapping occurred upon subtraction the iterator
would eventually reach this same address through rnext and be compared to it
in the main user loop. */
void *
CCC_buf_rend(CCC_buffer const *const buf)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    return (unsigned char *)buf->mem - buf->sizeof_type;
}

/** Will always be the address after capacity. */
void *
CCC_buf_capacity_end(CCC_buffer const *const buf)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    return (unsigned char *)buf->mem + (buf->sizeof_type * buf->capacity);
}

CCC_ucount
CCC_buf_i(CCC_buffer const *const buf, void const *const slot)
{
    if (!buf || !buf->mem || !slot || slot < buf->mem
        || (char *)slot
               >= ((char *)buf->mem + (buf->capacity * buf->sizeof_type)))
    {
        return (CCC_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (CCC_ucount){
        .count = (((char *)slot - ((char *)buf->mem)) / buf->sizeof_type),
    };
}

CCC_result
CCC_buf_size_plus(CCC_buffer *const buf, size_t const n)
{
    if (!buf)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    size_t const new_count = buf->count + n;
    if (new_count > buf->capacity)
    {
        buf->count = buf->capacity;
        return CCC_RESULT_ARG_ERROR;
    }
    buf->count = new_count;
    return CCC_RESULT_OK;
}

CCC_result
CCC_buf_size_minus(CCC_buffer *const buf, size_t const n)
{
    if (!buf)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (n > buf->count)
    {
        buf->count = 0;
        return CCC_RESULT_ARG_ERROR;
    }
    buf->count -= n;
    return CCC_RESULT_OK;
}

CCC_result
CCC_buf_size_set(CCC_buffer *const buf, size_t const n)
{
    if (!buf)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (n > buf->capacity)
    {
        buf->count = buf->capacity;
        return CCC_RESULT_ARG_ERROR;
    }
    buf->count = n;
    return CCC_RESULT_OK;
}

CCC_ucount
CCC_buf_count_bytes(CCC_buffer const *buf)
{
    if (!buf)
    {
        return (CCC_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (CCC_ucount){.count = buf->count * buf->sizeof_type};
}

CCC_ucount
CCC_buf_capacity_bytes(CCC_buffer const *buf)
{
    if (!buf)
    {
        return (CCC_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (CCC_ucount){.count = buf->capacity * buf->sizeof_type};
}

CCC_result
CCC_buf_copy(CCC_buffer *const dst, CCC_buffer const *const src,
             CCC_any_alloc_fn *const fn)
{
    if (!dst || !src || src == dst || (dst->capacity < src->capacity && !fn))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Copy everything so we don't worry about staying in sync with future
       changes to buf container. But we have to give back original destination
       memory in case it has already been allocated. Alloc will remain the
       same as in dst initialization because that controls permission. */
    void *const dst_mem = dst->mem;
    size_t const dst_cap = dst->capacity;
    CCC_any_alloc_fn *const dst_alloc = dst->alloc;
    *dst = *src;
    dst->mem = dst_mem;
    dst->capacity = dst_cap;
    dst->alloc = dst_alloc;
    if (!src->capacity)
    {
        return CCC_RESULT_OK;
    }
    if (dst->capacity < src->capacity)
    {
        CCC_result const r = CCC_buf_alloc(dst, src->capacity, fn);
        if (r != CCC_RESULT_OK)
        {
            return r;
        }
        dst->capacity = src->capacity;
    }
    if (!src->mem || !dst->mem)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    (void)memcpy(dst->mem, src->mem, src->capacity * src->sizeof_type);
    return CCC_RESULT_OK;
}

/*======================  Static Helpers  ==================================*/

static inline void *
at(struct CCC_buffer const *const buf, size_t const i)
{
    return ((char *)buf->mem + (i * buf->sizeof_type));
}

static inline size_t
max(size_t const a, size_t const b)
{
    return a > b ? a : b;
}
