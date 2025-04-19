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
struct ccc_buf
{
    void *mem;
    size_t elem_sz;
    size_t sz;
    size_t capacity;
    ccc_any_alloc_fn *alloc;
    void *aux;
};

/** @private */
#define IMPL_BUF_NON_IMPL_BUF_DEFAULT_SIZE(...) __VA_ARGS__
/** @private */
#define IMPL_BUF_DEFAULT_SIZE(...) 0
/** @private */
#define IMPL_BUF_OPTIONAL_SIZE(...)                                            \
    __VA_OPT__(IMPL_BUF_NON_)##IMPL_BUF_DEFAULT_SIZE(__VA_ARGS__)

/** @private */
#define ccc_impl_buf_init(impl_mem, impl_alloc_fn, impl_aux_data,              \
                          impl_capacity, ...)                                  \
    {                                                                          \
        .mem = (impl_mem),                                                     \
        .elem_sz = sizeof(*(impl_mem)),                                        \
        .sz = IMPL_BUF_OPTIONAL_SIZE(__VA_ARGS__),                             \
        .capacity = (impl_capacity),                                           \
        .alloc = (impl_alloc_fn),                                              \
        .aux = (impl_aux_data),                                                \
    }

/** @private */
#define ccc_impl_buf_emplace(buf_ptr, index, type_initializer...)              \
    (__extension__({                                                           \
        typeof(type_initializer) *impl_buf_res = NULL;                         \
        __auto_type impl_i = (index);                                          \
        __auto_type impl_emplace_buff_ptr = (buf_ptr);                         \
        impl_buf_res = ccc_buf_at(impl_emplace_buff_ptr, index);               \
        if (impl_buf_res)                                                      \
        {                                                                      \
            *impl_buf_res = type_initializer;                                  \
        }                                                                      \
        impl_buf_res;                                                          \
    }))

/** @private */
#define ccc_impl_buf_emplace_back(buf_ptr, type_initializer...)                \
    (__extension__({                                                           \
        typeof(type_initializer) *impl_buf_res = NULL;                         \
        __auto_type impl_emplace_back_buf_ptr = (buf_ptr);                     \
        assert(sizeof(typeof(*impl_buf_res))                                   \
               == ccc_buf_elem_size(impl_emplace_back_buf_ptr));               \
        impl_buf_res = ccc_buf_alloc_back((impl_emplace_back_buf_ptr));        \
        if (impl_buf_res)                                                      \
        {                                                                      \
            *impl_buf_res = type_initializer;                                  \
        }                                                                      \
        impl_buf_res;                                                          \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_BUF_H */
