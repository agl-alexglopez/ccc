#include "buffer.h"
#include "impl_buffer.h"
#include "types.h"

#include <alloca.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

static void *at(ccc_buffer const *, size_t);

ccc_result
ccc_buf_realloc(ccc_buffer *buf, size_t const new_capacity,
                ccc_realloc_fn *const fn)
{
    if (!fn)
    {
        return CCC_NO_REALLOC;
    }
    void *const new_mem = fn(buf->impl.mem, new_capacity);
    if (new_capacity && !new_mem)
    {
        return CCC_MEM_ERR;
    }
    buf->impl.mem = new_mem;
    buf->impl.capacity = new_capacity;
    return CCC_OK;
}

void *
ccc_buf_at(ccc_buffer const *buf, size_t const i)
{
    if (i >= buf->impl.capacity)
    {
        return NULL;
    }
    return ((uint8_t *)buf->impl.mem + (i * buf->impl.elem_sz));
}

void *
ccc_buf_back(ccc_buffer const *buf)
{
    return ccc_buf_at(buf, buf->impl.sz - (size_t)1);
}

void *
ccc_buf_front(ccc_buffer const *buf)
{
    return ccc_buf_at(buf, 0);
}

void *
ccc_buf_alloc(ccc_buffer *buf)
{
    if (buf->impl.sz != buf->impl.capacity)
    {
        return ((uint8_t *)buf->impl.mem
                + (buf->impl.elem_sz * buf->impl.sz++));
    }
    if (ccc_buf_realloc(buf, buf->impl.capacity * 2, buf->impl.realloc_fn)
        == CCC_OK)
    {
        return ccc_buf_back(buf);
    }
    return NULL;
}

ccc_result
ccc_buf_swap(ccc_buffer *buf, uint8_t tmp[], size_t const i, size_t const j)
{
    if (!buf->impl.sz || i >= buf->impl.sz || j >= buf->impl.sz || j == i)
    {
        return CCC_MEM_ERR;
    }
    (void)memcpy(tmp, at(buf, i), buf->impl.elem_sz);
    (void)memcpy(at(buf, i), at(buf, j), buf->impl.elem_sz);
    (void)memcpy(at(buf, j), tmp, buf->impl.elem_sz);
    return CCC_OK;
}

void *
ccc_buf_copy(ccc_buffer *buf, size_t const dst, size_t const src)
{
    if (!buf->impl.sz || dst >= buf->impl.sz || src >= buf->impl.sz)
    {
        return NULL;
    }
    if (dst == src)
    {
        return at(buf, dst);
    }
    return memcpy(at(buf, dst), at(buf, src), buf->impl.elem_sz);
}

ccc_result
ccc_buf_write(ccc_buffer *const buf, size_t const i, void const *const data)
{
    void *pos = ccc_buf_at(buf, i);
    if (!pos || data == pos)
    {
        return CCC_INPUT_ERR;
    }
    (void)memcpy(pos, data, ccc_buf_elem_size(buf));
    return CCC_OK;
}

ccc_result
ccc_buf_erase(ccc_buffer *buf, size_t const i)
{
    if (!buf->impl.sz || i >= buf->impl.sz)
    {
        return CCC_MEM_ERR;
    }
    if (1 == buf->impl.sz)
    {
        buf->impl.sz = 0;
        return CCC_OK;
    }
    if (i == buf->impl.sz - 1)
    {
        --buf->impl.sz;
        return CCC_OK;
    }
    (void)memcpy(at(buf, i), at(buf, i + 1),
                 buf->impl.elem_sz * (buf->impl.sz - (i + 1)));
    --buf->impl.sz;
    return CCC_OK;
}

ccc_result
ccc_buf_free(ccc_buffer *buf, ccc_realloc_fn *fn)
{
    if (!buf->impl.capacity || !fn)
    {
        return CCC_MEM_ERR;
    }
    buf->impl.capacity = 0;
    buf->impl.sz = 0;
    fn(buf->impl.mem, 0);
    return CCC_OK;
}

ccc_result
ccc_buf_pop_back_n(ccc_buffer *buf, size_t n)
{
    if (n > buf->impl.sz)
    {
        return CCC_MEM_ERR;
    }
    buf->impl.sz -= n;
    return CCC_OK;
}

ccc_result
ccc_buf_pop_back(ccc_buffer *buf)
{
    return ccc_buf_pop_back_n(buf, 1);
}

size_t
ccc_buf_size(ccc_buffer const *buf)
{
    return buf->impl.sz;
}

size_t
ccc_buf_capacity(ccc_buffer const *buf)
{
    return buf->impl.capacity;
}

size_t
ccc_buf_elem_size(ccc_buffer const *buf)
{
    return buf->impl.elem_sz;
}

bool
ccc_buf_full(ccc_buffer const *buf)
{
    return buf->impl.sz == buf->impl.capacity;
}

bool
ccc_buf_empty(ccc_buffer const *buf)
{
    return !buf->impl.sz;
}

void *
ccc_buf_base(ccc_buffer const *const buf)
{
    return buf->impl.mem;
}

void *
ccc_buf_begin(ccc_buffer const *const buf)
{
    return buf->impl.mem;
}

void *
ccc_buf_next(ccc_buffer const *const buf, void const *const pos)
{
    assert(pos < ccc_buf_capacity_end(buf));
    return (uint8_t *)pos + buf->impl.elem_sz;
}

void *
ccc_buf_size_end(ccc_buffer const *const buf)
{
    return (uint8_t *)buf->impl.mem + (buf->impl.elem_sz * buf->impl.sz);
}

void *
ccc_buf_capacity_end(ccc_buffer const *const buf)
{
    return (uint8_t *)buf->impl.mem + (buf->impl.elem_sz * buf->impl.capacity);
}

size_t
ccc_buf_index_of(ccc_buffer const *const buf, void const *const slot)
{
    assert(slot >= ccc_buf_base(buf));
    assert((uint8_t *)slot
           < ((uint8_t *)ccc_buf_base(buf)
              + (ccc_buf_capacity(buf) * ccc_buf_elem_size(buf))));
    return ((uint8_t *)slot - ((uint8_t *)ccc_buf_base(buf)))
           / ccc_buf_elem_size(buf);
}

/*======================  Static Helpers  ==================================*/

static inline void *
at(ccc_buffer const *buf, size_t const i)
{
    return ((uint8_t *)buf->impl.mem + (i * buf->impl.elem_sz));
}
