#ifndef CCC_IMPL_BUFFER_H
#define CCC_IMPL_BUFFER_H

#include "types.h"

#include <assert.h>
#include <stddef.h>

/** \internal */
struct ccc_buf_
{
    void *mem_;
    size_t elem_sz_;
    size_t sz_;
    size_t capacity_;
    ccc_alloc_fn *alloc_;
};

#define IMPL_BUF_NON_IMPL_BUF_DEFAULT_SIZE(...) __VA_ARGS__
#define IMPL_BUF_DEFAULT_SIZE(...) 0
#define IMPL_BUF_OPTIONAL_SIZE(...)                                            \
    __VA_OPT__(IMPL_BUF_NON_)##IMPL_BUF_DEFAULT_SIZE(__VA_ARGS__)

#define ccc_impl_buf_init(mem, alloc_fn, capacity, ...)                        \
    {                                                                          \
        .mem_ = (mem), .elem_sz_ = sizeof(*(mem)),                             \
        .sz_ = IMPL_BUF_OPTIONAL_SIZE(__VA_ARGS__), .capacity_ = (capacity),   \
        .alloc_ = (alloc_fn),                                                  \
    }

/* NOLINTBEGIN(readability-identifier-naming) */

#define ccc_impl_buf_emplace(ccc_buf_ptr, index, type_initializer...)          \
    ({                                                                         \
        typeof(type_initializer) *buf_res_ = NULL;                             \
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
        typeof(type_initializer) *buf_res_ = NULL;                             \
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

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_BUF_H */
