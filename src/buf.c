#include "buf.h"

#include <alloca.h>
#include <stdint.h>
#include <string.h>

static void *at(ccc_buf const *, size_t);

ccc_buf_result
ccc_buf_realloc(ccc_buf *buf, size_t const new_capacity,
                ccc_buf_realloc_fn *const fn)
{
    if (!fn)
    {
        return CCC_BUF_FULL;
    }
    void *const new_mem = fn(buf->mem, new_capacity);
    if (!new_mem)
    {
        return CCC_BUF_ERR;
    }
    buf->mem = new_mem;
    buf->capacity = new_capacity;
    return CCC_BUF_OK;
}

void *
ccc_buf_at(ccc_buf const *buf, size_t const i)
{
    if (i >= buf->capacity)
    {
        return NULL;
    }
    return ((uint8_t *)buf->mem + (i * buf->elem_sz));
}

void *
ccc_buf_back(ccc_buf const *buf)
{
    return ccc_buf_at(buf, buf->sz - 1ULL);
}

void *
ccc_buf_front(ccc_buf const *buf)
{
    return ccc_buf_at(buf, 0);
}

void *
ccc_buf_alloc(ccc_buf *buf)
{
    if (buf->sz != buf->capacity)
    {
        return ((uint8_t *)buf->mem + (buf->elem_sz * buf->sz++));
    }
    if (ccc_buf_realloc(buf, buf->capacity * 2, buf->realloc_fn) == CCC_BUF_OK)
    {
        return ccc_buf_back(buf);
    }
    return NULL;
}

ccc_buf_result
ccc_buf_swap(ccc_buf *buf, uint8_t tmp[], size_t const i, size_t const j)
{
    if (!buf->sz || i >= buf->sz || j >= buf->sz || j == i)
    {
        return CCC_BUF_ERR;
    }
    (void)memcpy(tmp, at(buf, i), buf->elem_sz);
    (void)memcpy(at(buf, i), at(buf, j), buf->elem_sz);
    (void)memcpy(at(buf, j), tmp, buf->elem_sz);
    return CCC_BUF_OK;
}

void *
ccc_buf_copy(ccc_buf *buf, size_t const dst, size_t const src)
{
    if (!buf->sz || dst >= buf->sz || src >= buf->sz)
    {
        return NULL;
    }
    if (dst == src)
    {
        return at(buf, dst);
    }
    return memcpy(at(buf, dst), at(buf, src), buf->elem_sz);
}

ccc_buf_result
ccc_buf_erase(ccc_buf *buf, size_t const i)
{
    if (!buf->sz || i >= buf->sz)
    {
        return CCC_BUF_ERR;
    }
    if (1 == buf->sz)
    {
        buf->sz = 0;
        return CCC_BUF_OK;
    }
    if (i == buf->sz - 1)
    {
        --buf->sz;
        return CCC_BUF_OK;
    }
    (void)memcpy(at(buf, i), at(buf, i + 1),
                 buf->elem_sz * (buf->sz - (i + 1)));
    --buf->sz;
    return CCC_BUF_OK;
}

ccc_buf_result
ccc_buf_free(ccc_buf *buf, ccc_buf_free_fn *fn)
{
    if (!buf->capacity)
    {
        return CCC_BUF_ERR;
    }
    buf->capacity = 0;
    buf->sz = 0;
    fn(buf->mem);
    return CCC_BUF_OK;
}

ccc_buf_result
ccc_buf_pop_back_n(ccc_buf *buf, size_t n)
{
    if (n > buf->sz)
    {
        return CCC_BUF_ERR;
    }
    buf->sz -= n;
    return CCC_BUF_OK;
}

ccc_buf_result
ccc_buf_pop_back(ccc_buf *buf)
{
    return ccc_buf_pop_back_n(buf, 1);
}

size_t
ccc_buf_size(ccc_buf const *buf)
{
    return buf->sz;
}

size_t
ccc_buf_capacity(ccc_buf const *buf)
{
    return buf->capacity;
}

size_t
ccc_buf_elem_size(ccc_buf const *buf)
{
    return buf->elem_sz;
}

bool
ccc_buf_full(ccc_buf const *buf)
{
    return buf->sz == buf->capacity;
}

bool
ccc_buf_empty(ccc_buf const *buf)
{
    return !buf->sz;
}

void *
ccc_buf_base(ccc_buf *buf)
{
    return buf->mem;
}

void *
ccc_buf_begin(ccc_buf const *const buf)
{
    return buf->mem;
}

void *
ccc_buf_next(ccc_buf const *const buf, void const *const pos)
{
    return (uint8_t *)pos + buf->elem_sz;
}

void *
ccc_buf_size_end(ccc_buf const *const buf)
{
    return (uint8_t *)buf->mem + (buf->elem_sz * buf->sz);
}

void *
ccc_buf_capcity_end(ccc_buf const *const buf)
{
    return (uint8_t *)buf->mem + (buf->elem_sz * buf->capacity);
}

/*======================  Static Helpers  ==================================*/

static inline void *
at(ccc_buf const *buf, size_t const i)
{
    return ((uint8_t *)buf->mem + (i * buf->elem_sz));
}
