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
#ifndef CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H
#define CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../buffer.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private */
struct ccc_fdeq
{
    ccc_buffer buf;
    size_t front;
};

/*=======================    Private Interface   ============================*/
/** @private */
void *ccc_impl_fdeq_alloc_front(struct ccc_fdeq *);
/** @private */
void *ccc_impl_fdeq_alloc_back(struct ccc_fdeq *);

/*=======================  Macro Implementations   ==========================*/

/** @private */
#define ccc_impl_fdeq_init(impl_mem_ptr, impl_alloc_fn, impl_aux_data,         \
                           impl_capacity, optional_size...)                    \
    {                                                                          \
        .buf = ccc_buf_init(impl_mem_ptr, impl_alloc_fn, impl_aux_data,        \
                            impl_capacity, optional_size),                     \
        .front = 0,                                                            \
    }

/** @private */
#define ccc_impl_fdeq_emplace_back(fdeq_ptr, value...)                         \
    (__extension__({                                                           \
        __auto_type impl_fdeq_ptr = (fdeq_ptr);                                \
        void *const impl_fdeq_emplace_ret = nullptr;                           \
        if (impl_fdeq_ptr)                                                     \
        {                                                                      \
            void *const impl_fdeq_emplace_ret                                  \
                = ccc_impl_fdeq_alloc_back(impl_fdeq_ptr);                     \
            if (impl_fdeq_emplace_ret)                                         \
            {                                                                  \
                *((typeof(value) *)impl_fdeq_emplace_ret) = value;             \
            }                                                                  \
        }                                                                      \
        impl_fdeq_emplace_ret;                                                 \
    }))

/** @private */
#define ccc_impl_fdeq_emplace_front(fdeq_ptr, value...)                        \
    (__extension__({                                                           \
        __auto_type impl_fdeq_ptr = (fdeq_ptr);                                \
        void *const impl_fdeq_emplace_ret = nullptr;                           \
        if (impl_fdeq_ptr)                                                     \
        {                                                                      \
            void *const impl_fdeq_emplace_ret                                  \
                = ccc_impl_fdeq_alloc_front(impl_fdeq_ptr);                    \
            if (impl_fdeq_emplace_ret)                                         \
            {                                                                  \
                *((typeof(value) *)impl_fdeq_emplace_ret) = value;             \
            }                                                                  \
        }                                                                      \
        impl_fdeq_emplace_ret;                                                 \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H */
