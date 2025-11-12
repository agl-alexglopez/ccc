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

/** @private A flat doubled ended queue is a single buffer with push and pop
at the front and back. If no allocation is permitted it is a ring buffer.
Because the `CCC_buffer` abstraction already exists, the fdeq can be
implemented with a single additional field rather than a front and back pointer.
The back of the fdeq is always known if we know where the front is and how
many elements are stored in the buffer. */
struct CCC_fdeq
{
    /** @private The helper buffer abstraction the fdeq owns. */
    CCC_buffer buf;
    /** @private The front of the fdeq. The back is implicit given the size. */
    size_t front;
};

/*=======================    Private Interface   ============================*/
/** @private */
void *CCC_impl_fdeq_alloc_front(struct CCC_fdeq *);
/** @private */
void *CCC_impl_fdeq_alloc_back(struct CCC_fdeq *);

/*=======================  Macro Implementations   ==========================*/

/** @private */
#define CCC_impl_fdeq_init(impl_mem_ptr, impl_any_type_name, impl_alloc_fn,    \
                           impl_aux_data, impl_capacity, optional_size...)     \
    {                                                                          \
        .buf = CCC_buf_init(impl_mem_ptr, impl_any_type_name, impl_alloc_fn,   \
                            impl_aux_data, impl_capacity, optional_size),      \
        .front = 0,                                                            \
    }

/** @private */
#define CCC_impl_fdeq_emplace_back(fdeq_ptr, value...)                         \
    (__extension__({                                                           \
        __auto_type impl_fdeq_ptr = (fdeq_ptr);                                \
        void *const impl_fdeq_emplace_ret = NULL;                              \
        if (impl_fdeq_ptr)                                                     \
        {                                                                      \
            void *const impl_fdeq_emplace_ret                                  \
                = CCC_impl_fdeq_alloc_back(impl_fdeq_ptr);                     \
            if (impl_fdeq_emplace_ret)                                         \
            {                                                                  \
                *((typeof(value) *)impl_fdeq_emplace_ret) = value;             \
            }                                                                  \
        }                                                                      \
        impl_fdeq_emplace_ret;                                                 \
    }))

/** @private */
#define CCC_impl_fdeq_emplace_front(fdeq_ptr, value...)                        \
    (__extension__({                                                           \
        __auto_type impl_fdeq_ptr = (fdeq_ptr);                                \
        void *const impl_fdeq_emplace_ret = NULL;                              \
        if (impl_fdeq_ptr)                                                     \
        {                                                                      \
            void *const impl_fdeq_emplace_ret                                  \
                = CCC_impl_fdeq_alloc_front(impl_fdeq_ptr);                    \
            if (impl_fdeq_emplace_ret)                                         \
            {                                                                  \
                *((typeof(value) *)impl_fdeq_emplace_ret) = value;             \
            }                                                                  \
        }                                                                      \
        impl_fdeq_emplace_ret;                                                 \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H */
