#ifndef CCC_IMPL_BUFFER_H
#define CCC_IMPL_BUFFER_H

#include "types.h"

#include <stddef.h>

struct ccc_impl_buf
{
    void *mem;
    size_t elem_sz;
    size_t sz;
    size_t capacity;
    ccc_realloc_fn *realloc_fn;
};

#define CCC_IMPL_BUF_INIT(mem, type, capacity, realloc_fn)                     \
    {                                                                          \
        .impl = {                                                              \
            (mem), sizeof(type), 0, (capacity), (realloc_fn),                  \
        },                                                                     \
    }

#define CCC_IMPL_BUF_EMPLACE(ccc_buf_ptr, index, type_initializer...)          \
    ({                                                                         \
        typeof(type_initializer) *_buf_res_;                                   \
        if (sizeof(typeof(*_buf_res_)) != ccc_buf_elem_size(ccc_buf_ptr))      \
        {                                                                      \
            _buf_res_ = NULL;                                                  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            _buf_res_ = ccc_buf_at(ccc_buf_ptr, index);                        \
            if (_buf_res_)                                                     \
            {                                                                  \
                *_buf_res_ = (struct_name)type_initializer;                    \
            }                                                                  \
        }                                                                      \
        _buf_res_;                                                             \
    })

#define CCC_IMPL_BUF_EMPLACE_BACK(ccc_buf_ptr, type_initializer...)            \
    ({                                                                         \
        typeof(type_initializer) *_buf_res_;                                   \
        if (sizeof(typeof(*_buf_res_)) != ccc_buf_elem_size(ccc_buf_ptr))      \
        {                                                                      \
            _buf_res_ = NULL;                                                  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            _buf_res_ = ccc_buf_alloc((ccc_buf_ptr));                          \
            if (_buf_res_)                                                     \
            {                                                                  \
                *_buf_res_ = type_initializer;                                 \
            }                                                                  \
        }                                                                      \
        _buf_res_;                                                             \
    })

#endif /* CCC_IMPL_BUF_H */
