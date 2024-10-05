#ifndef CCC_IMPL_BUFFER_H
#define CCC_IMPL_BUFFER_H

#include "types.h"

#include <assert.h>
#include <stddef.h>

struct ccc_buf_
{
    void *mem_;
    size_t elem_sz_;
    size_t sz_;
    size_t capacity_;
    ccc_alloc_fn *alloc_;
};

#define ccc_impl_buf_init(mem, type, capacity, alloc_fn)                       \
    {                                                                          \
        .mem_ = (mem), .elem_sz_ = sizeof(type), .sz_ = 0,                     \
        .capacity_ = (capacity), .alloc_ = (alloc_fn),                         \
    }

#define ccc_impl_buf_emplace(ccc_buf_ptr, index, type_initializer...)          \
    ({                                                                         \
        typeof(type_initializer) *buf_res_;                                    \
        __auto_type i_ = (index);                                              \
        __auto_type emplace_buff_ptr_ = (ccc_buf_ptr);                         \
        assert(sizeof(typeof(*buf_res_))                                       \
               == ccc_buf_elem_size(emplace_buff_ptr_));                       \
        buf_res_ = ccc_buf_at(emplace_buff_ptr_, index);                       \
        if (buf_res_)                                                          \
        {                                                                      \
            *buf_res_ = type_initializer;                                      \
        }                                                                      \
        buf_res_;                                                              \
    })

#define ccc_impl_buf_emplace_back(ccc_buf_ptr, type_initializer...)            \
    ({                                                                         \
        typeof(type_initializer) *buf_res_;                                    \
        __auto_type emplace_back_buf_ptr_ = (ccc_buf_ptr);                     \
        assert(sizeof(typeof(*buf_res_))                                       \
               == ccc_buf_elem_size(emplace_back_buf_ptr_));                   \
        buf_res_ = ccc_buf_alloc_back((emplace_back_buf_ptr_));                \
        if (buf_res_)                                                          \
        {                                                                      \
            *buf_res_ = type_initializer;                                      \
        }                                                                      \
        buf_res_;                                                              \
    })

#endif /* CCC_IMPL_BUF_H */
