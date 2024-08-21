#ifndef CCC_IMPL_BUF_H
#define CCC_IMPL_BUF_H

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

#define CCC_IMPL_BUF_EMPLACE(ccc_buf_ptr, index, struct_name,                  \
                             struct_initializer...)                            \
    ({                                                                         \
        ccc_result _macro_res_;                                                \
        {                                                                      \
            if (sizeof(struct_name) != ccc_buf_elem_size(ccc_buf_ptr))         \
            {                                                                  \
                _macro_res_ = CCC_BUF_ERR;                                     \
            }                                                                  \
            else                                                               \
            {                                                                  \
                void *_macro_pos_ = ccc_buf_at(ccc_buf_ptr, i);                \
                if (!_macro_pos_)                                              \
                {                                                              \
                    _macro_res_ = CCC_BUF_ERR;                                 \
                }                                                              \
                else                                                           \
                {                                                              \
                    *((struct_name *)_macro_pos_)                              \
                        = (struct_name)struct_initializer;                     \
                    _macro_res_ = CCC_BUF_OK;                                  \
                }                                                              \
            }                                                                  \
        };                                                                     \
        _macro_res_;                                                           \
    })

#endif /* CCC_IMPL_BUF_H */
