#include "buffer.h"
#include "types.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static size_t const start_capacity = 8;

static void *at(ccc_buffer const *, size_t);

ccc_result
ccc_buf_realloc(ccc_buffer *buf, size_t const new_capacity,
                ccc_alloc_fn *const fn)
{
    if (!fn)
    {
        return CCC_NO_REALLOC;
    }
    void *const new_mem = fn(buf->mem_, buf->elem_sz_ * new_capacity);
    if (new_capacity && !new_mem)
    {
        return CCC_MEM_ERR;
    }
    buf->mem_ = new_mem;
    buf->capacity_ = new_capacity;
    return CCC_OK;
}

void *
ccc_buf_at(ccc_buffer const *buf, size_t const i)
{
    if (i >= buf->capacity_)
    {
        return NULL;
    }
    return ((char *)buf->mem_ + (i * buf->elem_sz_));
}

void *
ccc_buf_back(ccc_buffer const *buf)
{
    return ccc_buf_at(buf, buf->sz_ - 1);
}

void *
ccc_buf_front(ccc_buffer const *buf)
{
    return ccc_buf_at(buf, 0);
}

void *
ccc_buf_alloc(ccc_buffer *buf)
{
    if (buf->sz_ == buf->capacity_
        && (CCC_OK != buf->capacity_
                ? ccc_buf_realloc(buf, buf->capacity_ * 2, buf->alloc_)
                : ccc_buf_realloc(buf, start_capacity, buf->alloc_)))
    {
        return NULL;
    }
    void *const ret = ((char *)buf->mem_ + (buf->elem_sz_ * buf->sz_));
    ++buf->sz_;
    return ret;
}

void *
ccc_buf_push_back(ccc_buffer *const buf, void const *const data)
{
    void *const mem = ccc_buf_alloc(buf);
    if (mem)
    {
        memcpy(mem, data, buf->elem_sz_);
    }
    return mem;
}

ccc_result
ccc_buf_swap(ccc_buffer *buf, char tmp[], size_t const i, size_t const j)
{
    if (!buf->sz_ || i >= buf->sz_ || j >= buf->sz_ || j == i)
    {
        return CCC_MEM_ERR;
    }
    (void)memcpy(tmp, at(buf, i), buf->elem_sz_);
    (void)memcpy(at(buf, i), at(buf, j), buf->elem_sz_);
    (void)memcpy(at(buf, j), tmp, buf->elem_sz_);
    return CCC_OK;
}

void *
ccc_buf_copy(ccc_buffer *buf, size_t const dst, size_t const src)
{
    if (!buf->sz_ || dst >= buf->sz_ || src >= buf->sz_)
    {
        return NULL;
    }
    if (dst == src)
    {
        return at(buf, dst);
    }
    return memcpy(at(buf, dst), at(buf, src), buf->elem_sz_);
}

ccc_result
ccc_buf_write(ccc_buffer *const buf, size_t const i, void const *const data)
{
    void *pos = ccc_buf_at(buf, i);
    if (!pos || data == pos)
    {
        return CCC_INPUT_ERR;
    }
    (void)memcpy(pos, data, buf->elem_sz_);
    return CCC_OK;
}

ccc_result
ccc_buf_erase(ccc_buffer *buf, size_t const i)
{
    if (!buf->sz_ || i >= buf->sz_)
    {
        return CCC_MEM_ERR;
    }
    if (1 == buf->sz_)
    {
        buf->sz_ = 0;
        return CCC_OK;
    }
    if (i == buf->sz_ - 1)
    {
        --buf->sz_;
        return CCC_OK;
    }
    (void)memcpy(at(buf, i), at(buf, i + 1),
                 buf->elem_sz_ * (buf->sz_ - (i + 1)));
    --buf->sz_;
    return CCC_OK;
}

ccc_result
ccc_buf_free(ccc_buffer *buf, ccc_alloc_fn *fn)
{
    if (!buf->capacity_ || !fn)
    {
        return CCC_MEM_ERR;
    }
    buf->capacity_ = 0;
    buf->sz_ = 0;
    fn(buf->mem_, 0);
    return CCC_OK;
}

ccc_result
ccc_buf_pop_back_n(ccc_buffer *buf, size_t n)
{
    if (n > buf->sz_)
    {
        return CCC_MEM_ERR;
    }
    buf->sz_ -= n;
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
    return buf->sz_;
}

size_t
ccc_buf_capacity(ccc_buffer const *buf)
{
    return buf->capacity_;
}

size_t
ccc_buf_elem_size(ccc_buffer const *buf)
{
    return buf->elem_sz_;
}

bool
ccc_buf_full(ccc_buffer const *buf)
{
    return buf->sz_ == buf->capacity_;
}

bool
ccc_buf_empty(ccc_buffer const *buf)
{
    return !buf->sz_;
}

void *
ccc_buf_base(ccc_buffer const *const buf)
{
    return buf->mem_;
}

void *
ccc_buf_begin(ccc_buffer const *const buf)
{
    return buf->mem_;
}

void *
ccc_buf_rbegin(ccc_buffer const *const buf)
{
    return (char *)buf->mem_ + (buf->sz_ * buf->elem_sz_);
}

void *
ccc_buf_next(ccc_buffer const *const buf, void const *const pos)
{
    if (pos >= ccc_buf_capacity_end(buf))
    {
        return NULL;
    }
    return (char *)pos + buf->elem_sz_;
}

void *
ccc_buf_rnext(ccc_buffer const *const buf, void const *const pos)
{
    if (pos <= ccc_buf_rend(buf))
    {
        return NULL;
    }
    return (char *)pos - buf->elem_sz_;
}

void *
ccc_buf_end([[maybe_unused]] ccc_buffer const *const buf)
{
    return (char *)buf->mem_ + (buf->sz_ * buf->elem_sz_);
}

void *
ccc_buf_rend([[maybe_unused]] ccc_buffer const *const buf)
{
    return (char *)buf->mem_ - buf->elem_sz_;
}

void *
ccc_buf_capacity_end(ccc_buffer const *const buf)
{
    return (char *)buf->mem_ + (buf->elem_sz_ * buf->capacity_);
}

size_t
ccc_buf_index_of(ccc_buffer const *const buf, void const *const slot)
{
    assert(slot >= buf->mem_);
    assert((char *)slot
           < ((char *)buf->mem_ + (buf->capacity_ * buf->elem_sz_)));
    return ((char *)slot - ((char *)buf->mem_)) / buf->elem_sz_;
}

void
ccc_buf_size_plus(ccc_buffer *const buf, size_t const n)
{
    buf->sz_ += n;
}

void
ccc_buf_size_minus(ccc_buffer *const buf, size_t const n)
{
    buf->sz_ = n > buf->sz_ ? 0 : buf->sz_ - n;
}

void
ccc_buf_size_set(ccc_buffer *const buf, size_t const n)
{
    buf->sz_ = n;
}

/*======================  Static Helpers  ==================================*/

static inline void *
at(ccc_buffer const *buf, size_t const i)
{
    return ((char *)buf->mem_ + (i * buf->elem_sz_));
}
