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

#define CCC_IMPL_BUF_INIT(mem, type, capacity, alloc_fn)                       \
    {                                                                          \
        {                                                                      \
            .mem_ = (mem), .elem_sz_ = sizeof(type), .sz_ = 0,                 \
            .capacity_ = (capacity), .alloc_ = (alloc_fn),                     \
        }                                                                      \
    }

#define CCC_IMPL_BUF_EMPLACE(ccc_buf_ptr, index, type_initializer...)          \
    ({                                                                         \
        typeof(type_initializer) *buf_res_;                                    \
        assert(sizeof(typeof(*buf_res_)) == ccc_buf_elem_size(ccc_buf_ptr));   \
        buf_res_ = ccc_buf_at(ccc_buf_ptr, index);                             \
        if (buf_res_)                                                          \
        {                                                                      \
            *buf_res_ = type_initializer;                                      \
        }                                                                      \
        buf_res_;                                                              \
    })

#define CCC_IMPL_BUF_EMPLACE_BACK(ccc_buf_ptr, type_initializer...)            \
    ({                                                                         \
        typeof(type_initializer) *buf_res_;                                    \
        assert(sizeof(typeof(*buf_res_)) == ccc_buf_elem_size(ccc_buf_ptr));   \
        buf_res_ = ccc_buf_alloc((ccc_buf_ptr));                               \
        if (buf_res_)                                                          \
        {                                                                      \
            *buf_res_ = type_initializer;                                      \
        }                                                                      \
        buf_res_;                                                              \
    })

#endif /* CCC_IMPL_BUF_H */
