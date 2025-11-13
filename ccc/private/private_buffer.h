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

/** @private A Buffer is a contiguous array of a uniform type. The user can
specify any type. The Buffer can be fixed size if no allocation permission is
given or dynamic if allocation permission is granted. The Buffer can also be
manually resized via the interface. */
struct CCC_Buffer
{
    /** @private The contiguous memory of uniform type. */
    void *mem;
    /** @private The current count of active Buffer slots. */
    size_t count;
    /** @private The total Buffer slots possible for this array. */
    size_t capacity;
    /** @private The size of the type the user stores in the buffer. */
    size_t sizeof_type;
    /** @private An allocation function for resizing, if allowed. */
    CCC_Allocator *alloc;
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

/** @private Initializes the Buffer with a default size of 0. However the user
can specify that the Buffer has some count of elements from index
`[0, capacity - 1)` at initialization time. The Buffer assumes these elements
are contiguous. */
#define CCC_private_buffer_initialize(private_mem, private_any_type_name,      \
                                      private_alloc_fn, private_aux_data,      \
                                      private_capacity, ...)                   \
    {                                                                          \
        .mem = (private_mem),                                                  \
        .sizeof_type = sizeof(private_any_type_name),                          \
        .count = IMPL_BUF_OPTIONAL_SIZE(__VA_ARGS__),                          \
        .capacity = (private_capacity),                                        \
        .alloc = (private_alloc_fn),                                           \
        .aux = (private_aux_data),                                             \
    }

/** @private For dynamic containers to perform the allocation and
initialization in one convenient step for user. */
#define CCC_private_buffer_from(private_alloc_fn, private_aux_data,            \
                                private_optional_capacity,                     \
                                private_compound_literal_array...)             \
    (__extension__({                                                           \
        typeof(*private_compound_literal_array)                                \
            *private_buffer_initializer_list                                   \
            = private_compound_literal_array;                                  \
        struct CCC_Buffer private_buf = CCC_private_buffer_initialize(         \
            NULL, typeof(*private_buffer_initializer_list), private_alloc_fn,  \
            private_aux_data, 0);                                              \
        size_t const private_n = sizeof(private_compound_literal_array)        \
                               / sizeof(*private_buffer_initializer_list);     \
        size_t const private_cap = private_optional_capacity;                  \
        if (CCC_buffer_reserve(                                                \
                &private_buf,                                                  \
                (private_n > private_cap ? private_n : private_cap),           \
                private_alloc_fn)                                              \
            == CCC_RESULT_OK)                                                  \
        {                                                                      \
            (void)memcpy(private_buf.mem, private_buffer_initializer_list,     \
                         private_n                                             \
                             * sizeof(*private_buffer_initializer_list));      \
            private_buf.count = private_n;                                     \
        }                                                                      \
        private_buf;                                                           \
    }))

/** @private For dynamic containers to perform initialization and reservation
of memory in one step. */
#define CCC_private_buffer_with_capacity(private_any_type_name,                \
                                         private_alloc_fn, private_aux_data,   \
                                         private_capacity)                     \
    (__extension__({                                                           \
        struct CCC_Buffer private_buf = CCC_private_buffer_initialize(         \
            NULL, private_any_type_name, private_alloc_fn, private_aux_data,   \
            0);                                                                \
        (void)CCC_buffer_reserve(&private_buf, (private_capacity),             \
                                 private_alloc_fn);                            \
        private_buf;                                                           \
    }))

/** @private */
#define CCC_private_buffer_emplace(private_buffer_ptr, index,                  \
                                   private_type_initializer...)                \
    (__extension__({                                                           \
        typeof(private_type_initializer) *private_buffer_res = NULL;           \
        __auto_type private_i = (index);                                       \
        __auto_type private_emplace_buff_ptr = (private_buffer_ptr);           \
        private_buffer_res                                                     \
            = CCC_buffer_at(private_emplace_buff_ptr, private_i);              \
        if (private_buffer_res)                                                \
        {                                                                      \
            *private_buffer_res = private_type_initializer;                    \
        }                                                                      \
        private_buffer_res;                                                    \
    }))

/** @private */
#define CCC_private_buffer_emplace_back(private_buffer_ptr,                    \
                                        private_type_initializer...)           \
    (__extension__({                                                           \
        typeof(private_type_initializer) *private_buffer_res = NULL;           \
        __auto_type private_emplace_back_private_buffer_ptr                    \
            = (private_buffer_ptr);                                            \
        private_buffer_res = CCC_buffer_alloc_back(                            \
            (private_emplace_back_private_buffer_ptr));                        \
        if (private_buffer_res)                                                \
        {                                                                      \
            *private_buffer_res = private_type_initializer;                    \
        }                                                                      \
        private_buffer_res;                                                    \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_BUF_H */
