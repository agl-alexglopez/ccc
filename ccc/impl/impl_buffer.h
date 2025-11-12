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
#include <string.h>
/** @endcond */

#include "../types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private A buffer is a contiguous array of a uniform type. The user can
specify any type. The buffer can be fixed size if no allocation permission is
given or dynamic if allocation permission is granted. The buffer can also be
manually resized via the interface. */
struct ccc_buffer
{
    /** @private The contiguous memory of uniform type. */
    void *mem;
    /** @private The current count of active buffer slots. */
    size_t count;
    /** @private The total buffer slots possible for this array. */
    size_t capacity;
    /** @private The size of the type the user stores in the buffer. */
    size_t sizeof_type;
    /** @private An allocation function for resizing, if allowed. */
    ccc_any_alloc_fn *alloc;
    /** @private Auxiliary data, if any. */
    void *aux;
};

/** @private */
#define IMPL_BUF_NON_IMPL_BUF_DEFAULT_SIZE(...) __VA_ARGS__
/** @private */
#define IMPL_BUF_DEFAULT_SIZE(...) 0
/** @private */
#define IMPL_BUF_OPTIONAL_SIZE(...)                                            \
    __VA_OPT__(IMPL_BUF_NON_)##IMPL_BUF_DEFAULT_SIZE(__VA_ARGS__)

/** @private Initializes the buffer with a default size of 0. However the user
can specify that the buffer has some count of elements from index
`[0, capacity - 1)` at initialization time. The buffer assumes these elements
are contiguous. */
#define ccc_impl_buf_init(impl_mem, impl_any_type_name, impl_alloc_fn,         \
                          impl_aux_data, impl_capacity, ...)                   \
    {                                                                          \
        .mem = (impl_mem),                                                     \
        .sizeof_type = sizeof(impl_any_type_name),                             \
        .count = IMPL_BUF_OPTIONAL_SIZE(__VA_ARGS__),                          \
        .capacity = (impl_capacity),                                           \
        .alloc = (impl_alloc_fn),                                              \
        .aux = (impl_aux_data),                                                \
    }

/** @private For dynamic containers to perform the allocation and
initialization in one convenient step for user. */
#define ccc_impl_buf_from(impl_alloc_fn, impl_aux_data,                        \
                          impl_optional_capacity,                              \
                          impl_compound_literal_array...)                      \
    (__extension__({                                                           \
        typeof(*impl_compound_literal_array) *impl_buf_initializer_list        \
            = impl_compound_literal_array;                                     \
        struct ccc_buffer impl_buf                                             \
            = ccc_impl_buf_init(NULL, typeof(*impl_buf_initializer_list),      \
                                impl_alloc_fn, impl_aux_data, 0);              \
        size_t const impl_n = sizeof(impl_compound_literal_array)              \
                            / sizeof(*impl_buf_initializer_list);              \
        size_t const impl_cap = impl_optional_capacity;                        \
        if (ccc_buf_reserve(&impl_buf,                                         \
                            (impl_n > impl_cap ? impl_n : impl_cap),           \
                            impl_alloc_fn)                                     \
            == CCC_RESULT_OK)                                                  \
        {                                                                      \
            (void)memcpy(impl_buf.mem, impl_buf_initializer_list,              \
                         impl_n * sizeof(*impl_buf_initializer_list));         \
            impl_buf.count = impl_n;                                           \
        }                                                                      \
        impl_buf;                                                              \
    }))

/** @private For dynamic containers to perform initialization and reservation
of memory in one step. */
#define ccc_impl_buf_with_capacity(impl_any_type_name, impl_alloc_fn,          \
                                   impl_aux_data, impl_capacity)               \
    (__extension__({                                                           \
        struct ccc_buffer impl_buf = ccc_impl_buf_init(                        \
            NULL, impl_any_type_name, impl_alloc_fn, impl_aux_data, 0);        \
        (void)ccc_buf_reserve(&impl_buf, (impl_capacity), impl_alloc_fn);      \
        impl_buf;                                                              \
    }))

/** @private */
#define ccc_impl_buf_emplace(impl_buf_ptr, index, impl_type_initializer...)    \
    (__extension__({                                                           \
        typeof(impl_type_initializer) *impl_buf_res = NULL;                    \
        __auto_type impl_i = (index);                                          \
        __auto_type impl_emplace_buff_ptr = (impl_buf_ptr);                    \
        impl_buf_res = ccc_buf_at(impl_emplace_buff_ptr, impl_i);              \
        if (impl_buf_res)                                                      \
        {                                                                      \
            *impl_buf_res = impl_type_initializer;                             \
        }                                                                      \
        impl_buf_res;                                                          \
    }))

/** @private */
#define ccc_impl_buf_emplace_back(impl_buf_ptr, impl_type_initializer...)      \
    (__extension__({                                                           \
        typeof(impl_type_initializer) *impl_buf_res = NULL;                    \
        __auto_type impl_emplace_back_impl_buf_ptr = (impl_buf_ptr);           \
        impl_buf_res = ccc_buf_alloc_back((impl_emplace_back_impl_buf_ptr));   \
        if (impl_buf_res)                                                      \
        {                                                                      \
            *impl_buf_res = impl_type_initializer;                             \
        }                                                                      \
        impl_buf_res;                                                          \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_BUF_H */
