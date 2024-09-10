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
                ccc_alloc_fn *const fn)
{
    if (!fn)
    {
        return CCC_NO_REALLOC;
    }
    void *const new_mem = fn(buf->impl_.mem_, new_capacity);
    if (new_capacity && !new_mem)
    {
        return CCC_MEM_ERR;
    }
    buf->impl_.mem_ = new_mem;
    buf->impl_.capacity_ = new_capacity;
    return CCC_OK;
}

void *
ccc_buf_at(ccc_buffer const *buf, size_t const i)
{
    if (i >= buf->impl_.capacity_)
    {
        return NULL;
    }
    return ((uint8_t *)buf->impl_.mem_ + (i * buf->impl_.elem_sz_));
}

void *
ccc_buf_back(ccc_buffer const *buf)
{
    return ccc_buf_at(buf, buf->impl_.sz_ - (size_t)1);
}

void *
ccc_buf_front(ccc_buffer const *buf)
{
    return ccc_buf_at(buf, 0);
}

void *
ccc_buf_alloc(ccc_buffer *buf)
{
    if (buf->impl_.sz_ != buf->impl_.capacity_)
    {
        return ((uint8_t *)buf->impl_.mem_
                + (buf->impl_.elem_sz_ * buf->impl_.sz_++));
    }
    if (ccc_buf_realloc(buf, buf->impl_.capacity_ * 2, buf->impl_.alloc_)
        == CCC_OK)
    {
        return ccc_buf_back(buf);
    }
    return NULL;
}

ccc_result
ccc_buf_swap(ccc_buffer *buf, uint8_t tmp[], size_t const i, size_t const j)
{
    if (!buf->impl_.sz_ || i >= buf->impl_.sz_ || j >= buf->impl_.sz_ || j == i)
    {
        return CCC_MEM_ERR;
    }
    (void)memcpy(tmp, at(buf, i), buf->impl_.elem_sz_);
    (void)memcpy(at(buf, i), at(buf, j), buf->impl_.elem_sz_);
    (void)memcpy(at(buf, j), tmp, buf->impl_.elem_sz_);
    return CCC_OK;
}

void *
ccc_buf_copy(ccc_buffer *buf, size_t const dst, size_t const src)
{
    if (!buf->impl_.sz_ || dst >= buf->impl_.sz_ || src >= buf->impl_.sz_)
    {
        return NULL;
    }
    if (dst == src)
    {
        return at(buf, dst);
    }
    return memcpy(at(buf, dst), at(buf, src), buf->impl_.elem_sz_);
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
    if (!buf->impl_.sz_ || i >= buf->impl_.sz_)
    {
        return CCC_MEM_ERR;
    }
    if (1 == buf->impl_.sz_)
    {
        buf->impl_.sz_ = 0;
        return CCC_OK;
    }
    if (i == buf->impl_.sz_ - 1)
    {
        --buf->impl_.sz_;
        return CCC_OK;
    }
    (void)memcpy(at(buf, i), at(buf, i + 1),
                 buf->impl_.elem_sz_ * (buf->impl_.sz_ - (i + 1)));
    --buf->impl_.sz_;
    return CCC_OK;
}

ccc_result
ccc_buf_free(ccc_buffer *buf, ccc_alloc_fn *fn)
{
    if (!buf->impl_.capacity_ || !fn)
    {
        return CCC_MEM_ERR;
    }
    buf->impl_.capacity_ = 0;
    buf->impl_.sz_ = 0;
    fn(buf->impl_.mem_, 0);
    return CCC_OK;
}

ccc_result
ccc_buf_pop_back_n(ccc_buffer *buf, size_t n)
{
    if (n > buf->impl_.sz_)
    {
        return CCC_MEM_ERR;
    }
    buf->impl_.sz_ -= n;
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
    return buf->impl_.sz_;
}

size_t
ccc_buf_capacity(ccc_buffer const *buf)
{
    return buf->impl_.capacity_;
}

size_t
ccc_buf_elem_size(ccc_buffer const *buf)
{
    return buf->impl_.elem_sz_;
}

bool
ccc_buf_full(ccc_buffer const *buf)
{
    return buf->impl_.sz_ == buf->impl_.capacity_;
}

bool
ccc_buf_empty(ccc_buffer const *buf)
{
    return !buf->impl_.sz_;
}

void *
ccc_buf_base(ccc_buffer const *const buf)
{
    return buf->impl_.mem_;
}

void *
ccc_buf_begin(ccc_buffer const *const buf)
{
    return buf->impl_.mem_;
}

void *
ccc_buf_next(ccc_buffer const *const buf, void const *const pos)
{
    assert(pos < ccc_buf_capacity_end(buf));
    return (uint8_t *)pos + buf->impl_.elem_sz_;
}

void *
ccc_buf_size_end(ccc_buffer const *const buf)
{
    return (uint8_t *)buf->impl_.mem_ + (buf->impl_.elem_sz_ * buf->impl_.sz_);
}

void *
ccc_buf_capacity_end(ccc_buffer const *const buf)
{
    return (uint8_t *)buf->impl_.mem_
           + (buf->impl_.elem_sz_ * buf->impl_.capacity_);
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

void
ccc_buf_size_plus(ccc_buffer *const buf)
{
    ++buf->impl_.sz_;
}

void
ccc_buf_size_minus(ccc_buffer *const buf)
{
    --buf->impl_.sz_;
}

/*======================  Static Helpers  ==================================*/

static inline void *
at(ccc_buffer const *buf, size_t const i)
{
    return ((uint8_t *)buf->impl_.mem_ + (i * buf->impl_.elem_sz_));
}
