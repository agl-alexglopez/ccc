#ifndef CCC_IMPL_BUFFER_H
#define CCC_IMPL_BUFFER_H

/** @cond */
#include <assert.h>
#include <stddef.h>
/** @endcond */

#include "../types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private */
struct ccc_buf_
{
    void *mem_;
    size_t elem_sz_;
    ptrdiff_t sz_;
    ptrdiff_t capacity_;
    ccc_alloc_fn *alloc_;
    void *aux_;
};

/** @private */
#define IMPL_BUF_NON_IMPL_BUF_DEFAULT_SIZE(...) __VA_ARGS__
/** @private */
#define IMPL_BUF_DEFAULT_SIZE(...) 0
/** @private */
#define IMPL_BUF_OPTIONAL_SIZE(...)                                            \
    __VA_OPT__(IMPL_BUF_NON_)##IMPL_BUF_DEFAULT_SIZE(__VA_ARGS__)

/** @private */
#define ccc_impl_buf_init(mem, alloc_fn, aux_data, capacity, ...)              \
    {                                                                          \
        .mem_ = (mem),                                                         \
        .elem_sz_ = sizeof(*(mem)),                                            \
        .sz_ = IMPL_BUF_OPTIONAL_SIZE(__VA_ARGS__),                            \
        .capacity_ = (capacity),                                               \
        .alloc_ = (alloc_fn),                                                  \
        .aux_ = (aux_data),                                                    \
    }

/** @private */
#define ccc_impl_buf_emplace(ccc_buf_ptr, index, type_initializer...)          \
    (__extension__({                                                           \
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
    }))

/** @private */
#define ccc_impl_buf_emplace_back(ccc_buf_ptr, type_initializer...)            \
    (__extension__({                                                           \
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
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_BUF_H */
