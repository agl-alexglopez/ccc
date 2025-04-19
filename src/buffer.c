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
#include "types.h"

enum : size_t
{
    START_CAPACITY = 8,
};

/*==========================   Prototypes    ================================*/

static void *at(ccc_buffer const *, size_t);

/*==========================    Interface    ================================*/

ccc_result
ccc_buf_alloc(ccc_buffer *const buf, size_t const capacity,
              ccc_any_alloc_fn *const fn)
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

void *
ccc_buf_at(ccc_buffer const *const buf, size_t const i)
{
    if (!buf || i >= buf->capacity)
    {
        return NULL;
    }
    return ((char *)buf->mem + (i * buf->sizeof_type));
}

void *
ccc_buf_back(ccc_buffer const *const buf)
{
    return ccc_buf_at(buf, buf->count - 1);
}

void *
ccc_buf_front(ccc_buffer const *const buf)
{
    return ccc_buf_at(buf, 0);
}

void *
ccc_buf_alloc_back(ccc_buffer *const buf)
{
    if (!buf)
    {
        return NULL;
    }
    if (buf->count == buf->capacity
        && (CCC_RESULT_OK
            != (buf->capacity
                    ? ccc_buf_alloc(buf, buf->capacity * 2, buf->alloc)
                    : ccc_buf_alloc(buf, START_CAPACITY, buf->alloc))))
    {
        return NULL;
    }
    void *const ret = ((char *)buf->mem + (buf->sizeof_type * buf->count));
    ++buf->count;
    return ret;
}

void *
ccc_buf_push_back(ccc_buffer *const buf, void const *const data)
{
    void *const mem = ccc_buf_alloc_back(buf);
    if (mem)
    {
        (void)memcpy(mem, data, buf->sizeof_type);
    }
    return mem;
}

ccc_result
ccc_buf_swap(ccc_buffer *const buf, char tmp[const], size_t const i,
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
ccc_buf_copy(ccc_buffer *const buf, size_t const dst, size_t const src)
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

ccc_result
ccc_buf_write(ccc_buffer *const buf, size_t const i, void const *const data)
{
    if (!buf || !buf->mem || !data)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    void *const pos = ccc_buf_at(buf, i);
    if (!pos || data == pos)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    (void)memcpy(pos, data, buf->sizeof_type);
    return CCC_RESULT_OK;
}

ccc_result
ccc_buf_erase(ccc_buffer *const buf, size_t const i)
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
    (void)memcpy(at(buf, i), at(buf, i + 1),
                 buf->sizeof_type * (buf->count - (i + 1)));
    --buf->count;
    return CCC_RESULT_OK;
}

void *
ccc_buf_insert(ccc_buffer *const buf, size_t const i, void const *const data)
{
    if (!buf || !buf->mem || i > buf->count)
    {
        return NULL;
    }
    if (i == buf->count)
    {
        return ccc_buf_push_back(buf, data);
    }
    (void)memmove(at(buf, i + 1), at(buf, i),
                  buf->sizeof_type * (buf->count - i));
    ++buf->count;
    return at(buf, i);
}

ccc_result
ccc_buf_pop_back_n(ccc_buffer *const buf, size_t n)
{
    if (!buf || n > buf->count)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    buf->count -= n;
    return CCC_RESULT_OK;
}

ccc_result
ccc_buf_pop_back(ccc_buffer *const buf)
{
    return ccc_buf_pop_back_n(buf, 1);
}

ccc_ucount
ccc_buf_size(ccc_buffer const *const buf)
{
    if (!buf)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = buf->count};
}

ccc_ucount
ccc_buf_capacity(ccc_buffer const *const buf)
{
    if (!buf)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = buf->capacity};
}

ccc_ucount
ccc_buf_sizeof_type(ccc_buffer const *const buf)
{
    if (!buf)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = buf->sizeof_type};
}

ccc_tribool
ccc_buf_is_empty(ccc_buffer const *const buf)
{
    if (!buf)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !buf->count;
}

ccc_tribool
ccc_buf_is_full(ccc_buffer const *const buf)
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
ccc_buf_begin(ccc_buffer const *const buf)
{
    return buf ? buf->mem : NULL;
}

void *
ccc_buf_rbegin(ccc_buffer const *const buf)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    return (char *)buf->mem + (buf->count * buf->sizeof_type);
}

void *
ccc_buf_next(ccc_buffer const *const buf, void const *const iter)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    if (iter >= ccc_buf_capacity_end(buf))
    {
        return ccc_buf_end(buf);
    }
    return (char *)iter + buf->sizeof_type;
}

void *
ccc_buf_rnext(ccc_buffer const *const buf, void const *const iter)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    if (iter <= ccc_buf_rend(buf))
    {
        return ccc_buf_rend(buf);
    }
    return (char *)iter - buf->sizeof_type;
}

void *
ccc_buf_end(ccc_buffer const *const buf)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    return (char *)buf->mem + (buf->count * buf->sizeof_type);
}

void *
ccc_buf_rend(ccc_buffer const *const buf)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    return (char *)buf->mem - buf->sizeof_type;
}

void *
ccc_buf_capacity_end(ccc_buffer const *const buf)
{
    if (!buf || !buf->mem)
    {
        return NULL;
    }
    return (char *)buf->mem + (buf->sizeof_type * buf->capacity);
}

ccc_ucount
ccc_buf_i(ccc_buffer const *const buf, void const *const slot)
{
    if (!buf || !buf->mem || !slot || slot < buf->mem
        || (char *)slot
               >= ((char *)buf->mem + (buf->capacity * buf->sizeof_type)))
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){
        .count = (((char *)slot - ((char *)buf->mem)) / buf->sizeof_type),
    };
}

ccc_result
ccc_buf_size_plus(ccc_buffer *const buf, size_t const n)
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

ccc_result
ccc_buf_size_minus(ccc_buffer *const buf, size_t const n)
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

ccc_result
ccc_buf_size_set(ccc_buffer *const buf, size_t const n)
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

/*======================  Static Helpers  ==================================*/

static inline void *
at(ccc_buffer const *const buf, size_t const i)
{
    return ((char *)buf->mem + (i * buf->sizeof_type));
}
