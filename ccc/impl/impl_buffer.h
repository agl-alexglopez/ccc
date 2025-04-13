/** @cond
Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
@endcond */
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
    size_t sz_;
    size_t capacity_;
    ccc_any_alloc_fn *alloc_;
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
