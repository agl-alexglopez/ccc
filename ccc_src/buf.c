#include "buf.h"
#include "attrib.h"

#include <alloca.h>
#include <stdint.h>
#include <string.h>

static void *at(ccc_buf const[static 1], size_t) ATTRIB_NONNULL(1);

ccc_buf_result
ccc_buf_init(ccc_buf buf[static const 1], size_t const elem_sz,
             size_t const capacity, ccc_buf_realloc_fn *const fn)
{
    buf->mem = NULL;
    buf->elem_sz = elem_sz;
    buf->capacity = capacity;
    buf->sz = 0;
    buf->fn = fn;
    if (!capacity)
    {
        return CCC_BUF_OK;
    }
    buf->mem = buf->fn(buf->mem, capacity * sizeof(elem_sz));
    if (!buf->mem)
    {
        return CCC_BUF_ERR;
    }
    return CCC_BUF_OK;
}

ccc_buf_result
ccc_buf_realloc(ccc_buf buf[static const 1], size_t const new_capacity)
{
    if (!buf->fn)
    {
        return CCC_BUF_FULL;
    }
    void *const new_mem = buf->fn(buf->mem, new_capacity);
    if (!new_mem)
    {
        return CCC_BUF_ERR;
    }
    buf->mem = new_mem;
    buf->capacity = new_capacity;
    return CCC_BUF_OK;
}

void *
ccc_buf_at(ccc_buf const buf[static 1], size_t const i)
{
    return ((uint8_t *)buf->mem + (i * buf->elem_sz));
}

void *
ccc_buf_back(ccc_buf const buf[static const 1])
{
    return ccc_buf_at(buf, buf->sz - 1ULL);
}

void *
ccc_buf_front(ccc_buf const buf[static const 1])
{
    return ccc_buf_at(buf, 0);
}

void *
ccc_buf_alloc(ccc_buf buf[static const 1])
{
    if (buf->sz != buf->capacity)
    {
        return ((uint8_t *)buf->mem + (buf->elem_sz * buf->sz++));
    }
    if (ccc_buf_realloc(buf, buf->capacity * 2) == CCC_BUF_OK)
    {
        return ccc_buf_back(buf);
    }
    return NULL;
}

ccc_buf_result
ccc_buf_swap(ccc_buf buf[static const 1], uint8_t tmp[static const 1],
             size_t const i, size_t const j)
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
ccc_buf_copy(ccc_buf buf[static 1], size_t const dst, size_t const src)
{
    if (!buf->sz || dst >= buf->sz || src >= buf->sz)
    {
        return NULL;
    }
    if (dst == src)
    {
        return at(buf, dst);
    }
    void *const ret = at(buf, dst);
    (void)memcpy(ret, at(buf, src), buf->elem_sz);
    return ret;
}

ccc_buf_result
ccc_buf_free(ccc_buf buf[static 1], size_t const i)
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
    (void)memcpy(at(buf, i), at(buf, buf->sz - 1), buf->elem_sz);
    buf->sz--;
    return CCC_BUF_OK;
}

ccc_buf_result
ccc_buf_pop_back_n(ccc_buf buf[static 1], size_t n)
{
    if (n > buf->sz)
    {
        return CCC_BUF_ERR;
    }
    buf->sz -= n;
    return CCC_BUF_OK;
}

ccc_buf_result
ccc_buf_pop_back(ccc_buf buf[static 1])
{
    return ccc_buf_pop_back_n(buf, 1);
}

size_t
ccc_buf_size(ccc_buf const buf[static const 1])
{
    return buf->sz;
}

size_t
ccc_buf_capacity(ccc_buf const buf[static const 1])
{
    return buf->capacity;
}

size_t
ccc_buf_elem_size(ccc_buf const buf[static const 1])
{
    return buf->elem_sz;
}

bool
ccc_buf_full(ccc_buf const buf[static const 1])
{
    return buf->sz == buf->capacity;
}

bool
ccc_buf_empty(ccc_buf const buf[static 1])
{
    return !buf->sz;
}

void *
ccc_buf_base(ccc_buf buf[static 1])
{
    return buf->mem;
}

static inline void *
at(ccc_buf const buf[static 1], size_t const i)
{
    return ((uint8_t *)buf->mem + (i * buf->elem_sz));
}
