#include <stddef.h>
#include <string.h>

#include "buffer.h"
#include "types.h"

static ptrdiff_t const start_capacity = 8;

/*==========================   Prototypes    ================================*/

static void *at(ccc_buffer const *, ptrdiff_t);

/*==========================    Interface    ================================*/

ccc_result
ccc_buf_alloc(ccc_buffer *const buf, ptrdiff_t const capacity,
              ccc_alloc_fn *const fn)
{
    if (!buf)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!fn)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    void *const new_mem = fn(buf->mem_, buf->elem_sz_ * capacity, buf->aux_);
    if (capacity && !new_mem)
    {
        return CCC_RESULT_MEM_ERR;
    }
    buf->mem_ = new_mem;
    buf->capacity_ = capacity;
    return CCC_RESULT_OK;
}

void *
ccc_buf_at(ccc_buffer const *const buf, ptrdiff_t const i)
{
    if (!buf || i >= buf->capacity_)
    {
        return NULL;
    }
    return ((char *)buf->mem_ + (i * buf->elem_sz_));
}

void *
ccc_buf_back(ccc_buffer const *const buf)
{
    return ccc_buf_at(buf, buf->sz_ - 1);
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
    if (buf->sz_ == buf->capacity_
        && (CCC_RESULT_OK
            != (buf->capacity_
                    ? ccc_buf_alloc(buf, buf->capacity_ * 2, buf->alloc_)
                    : ccc_buf_alloc(buf, start_capacity, buf->alloc_))))
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
    void *const mem = ccc_buf_alloc_back(buf);
    if (mem)
    {
        (void)memcpy(mem, data, buf->elem_sz_);
    }
    return mem;
}

ccc_result
ccc_buf_swap(ccc_buffer *const buf, char tmp[], ptrdiff_t const i,
             ptrdiff_t const j)
{
    if (!buf || !tmp || i >= buf->capacity_ || j >= buf->capacity_ || j == i)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    (void)memcpy(tmp, at(buf, i), buf->elem_sz_);
    (void)memcpy(at(buf, i), at(buf, j), buf->elem_sz_);
    (void)memcpy(at(buf, j), tmp, buf->elem_sz_);
    return CCC_RESULT_OK;
}

void *
ccc_buf_copy(ccc_buffer *const buf, ptrdiff_t const dst, ptrdiff_t const src)
{
    if (!buf || dst >= buf->capacity_ || src >= buf->capacity_)
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
ccc_buf_write(ccc_buffer *const buf, ptrdiff_t const i, void const *const data)
{
    if (!buf || !buf->mem_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    void *const pos = ccc_buf_at(buf, i);
    if (!pos || data == pos)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    (void)memcpy(pos, data, buf->elem_sz_);
    return CCC_RESULT_OK;
}

ccc_result
ccc_buf_erase(ccc_buffer *const buf, ptrdiff_t const i)
{
    if (!buf || !buf->sz_ || i >= buf->sz_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (1 == buf->sz_)
    {
        buf->sz_ = 0;
        return CCC_RESULT_OK;
    }
    if (i == buf->sz_ - 1)
    {
        --buf->sz_;
        return CCC_RESULT_OK;
    }
    (void)memcpy(at(buf, i), at(buf, i + 1),
                 buf->elem_sz_ * (buf->sz_ - (i + 1)));
    --buf->sz_;
    return CCC_RESULT_OK;
}

void *
ccc_buf_insert(ccc_buffer *const buf, ptrdiff_t const i, void const *const data)
{
    if (!buf || !buf->mem_ || i > buf->sz_)
    {
        return NULL;
    }
    if (i == buf->sz_)
    {
        return ccc_buf_push_back(buf, data);
    }
    (void)memmove(at(buf, i + 1), at(buf, i), buf->elem_sz_ * (buf->sz_ - i));
    ++buf->sz_;
    return at(buf, i);
}

ccc_result
ccc_buf_pop_back_n(ccc_buffer *const buf, ptrdiff_t n)
{
    if (!buf || n > buf->sz_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    buf->sz_ -= n;
    return CCC_RESULT_OK;
}

ccc_result
ccc_buf_pop_back(ccc_buffer *const buf)
{
    return ccc_buf_pop_back_n(buf, 1);
}

ptrdiff_t
ccc_buf_size(ccc_buffer const *const buf)
{
    return buf ? buf->sz_ : -1;
}

ptrdiff_t
ccc_buf_capacity(ccc_buffer const *const buf)
{
    return buf ? buf->capacity_ : -1;
}

size_t
ccc_buf_elem_size(ccc_buffer const *const buf)
{
    return buf ? buf->elem_sz_ : 0;
}

ccc_tribool
ccc_buf_is_empty(ccc_buffer const *const buf)
{
    if (!buf)
    {
        return CCC_BOOL_ERR;
    }
    return !buf->sz_;
}

ccc_tribool
ccc_buf_is_full(ccc_buffer const *const buf)
{
    if (!buf)
    {
        return CCC_BOOL_ERR;
    }
    if (!buf->capacity_)
    {
        return CCC_FALSE;
    }
    return buf->sz_ == buf->capacity_ ? CCC_TRUE : CCC_FALSE;
}

void *
ccc_buf_begin(ccc_buffer const *const buf)
{
    return buf ? buf->mem_ : NULL;
}

void *
ccc_buf_rbegin(ccc_buffer const *const buf)
{
    if (!buf || !buf->mem_)
    {
        return NULL;
    }
    return (char *)buf->mem_ + (buf->sz_ * buf->elem_sz_);
}

void *
ccc_buf_next(ccc_buffer const *const buf, void const *const iter)
{
    if (!buf || !buf->mem_)
    {
        return NULL;
    }
    if (iter >= ccc_buf_capacity_end(buf))
    {
        return ccc_buf_end(buf);
    }
    return (char *)iter + buf->elem_sz_;
}

void *
ccc_buf_rnext(ccc_buffer const *const buf, void const *const iter)
{
    if (!buf || !buf->mem_)
    {
        return NULL;
    }
    if (iter <= ccc_buf_rend(buf))
    {
        return ccc_buf_rend(buf);
    }
    return (char *)iter - buf->elem_sz_;
}

void *
ccc_buf_end(ccc_buffer const *const buf)
{
    if (!buf || !buf->mem_)
    {
        return NULL;
    }
    return (char *)buf->mem_ + (buf->sz_ * buf->elem_sz_);
}

void *
ccc_buf_rend(ccc_buffer const *const buf)
{
    if (!buf || !buf->mem_)
    {
        return NULL;
    }
    return (char *)buf->mem_ - buf->elem_sz_;
}

void *
ccc_buf_capacity_end(ccc_buffer const *const buf)
{
    if (!buf || !buf->mem_)
    {
        return NULL;
    }
    return (char *)buf->mem_ + (buf->elem_sz_ * buf->capacity_);
}

ptrdiff_t
ccc_buf_i(ccc_buffer const *const buf, void const *const slot)
{
    if (!buf || !buf->mem_ || !slot || slot < buf->mem_
        || (char *)slot
               >= ((char *)buf->mem_ + (buf->capacity_ * buf->elem_sz_)))
    {
        return -1;
    }
    return (ptrdiff_t)(((char *)slot - ((char *)buf->mem_)) / buf->elem_sz_);
}

ccc_result
ccc_buf_size_plus(ccc_buffer *const buf, ptrdiff_t const n)
{
    if (!buf)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    ptrdiff_t const new_sz = buf->sz_ + n;
    if (new_sz > buf->capacity_)
    {
        buf->sz_ = buf->capacity_;
        return CCC_RESULT_ARG_ERROR;
    }
    buf->sz_ = new_sz;
    return CCC_RESULT_OK;
}

ccc_result
ccc_buf_size_minus(ccc_buffer *const buf, ptrdiff_t const n)
{
    if (!buf)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (n > buf->sz_)
    {
        buf->sz_ = 0;
        return CCC_RESULT_ARG_ERROR;
    }
    buf->sz_ -= n;
    return CCC_RESULT_OK;
}

ccc_result
ccc_buf_size_set(ccc_buffer *const buf, ptrdiff_t const n)
{
    if (!buf)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (n > buf->capacity_)
    {
        buf->sz_ = buf->capacity_;
        return CCC_RESULT_ARG_ERROR;
    }
    buf->sz_ = n;
    return CCC_RESULT_OK;
}

/*======================  Static Helpers  ==================================*/

static inline void *
at(ccc_buffer const *const buf, ptrdiff_t const i)
{
    return ((char *)buf->mem_ + (i * buf->elem_sz_));
}
