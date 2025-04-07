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
struct ccc_fdeq_
{
    ccc_buffer buf_;
    size_t front_;
};

/*=======================    Private Interface   ============================*/
/** @private */
void *ccc_impl_fdeq_alloc_front(struct ccc_fdeq_ *);
/** @private */
void *ccc_impl_fdeq_alloc_back(struct ccc_fdeq_ *);

/*=======================  Macro Implementations   ==========================*/

/** @private */
#define ccc_impl_fdeq_init(mem_ptr, alloc_fn, aux_data, capacity,              \
                           optional_size...)                                   \
    {                                                                          \
        .buf_                                                                  \
        = ccc_buf_init(mem_ptr, alloc_fn, aux_data, capacity, optional_size),  \
        .front_ = 0,                                                           \
    }

/** @private */
#define ccc_impl_fdeq_emplace_back(fdeq_ptr, value...)                         \
    (__extension__({                                                           \
        __auto_type fdeq_ptr_ = (fdeq_ptr);                                    \
        void *const fdeq_emplace_ret_ = NULL;                                  \
        if (fdeq_ptr_)                                                         \
        {                                                                      \
            void *const fdeq_emplace_ret_                                      \
                = ccc_impl_fdeq_alloc_back(fdeq_ptr_);                         \
            if (fdeq_emplace_ret_)                                             \
            {                                                                  \
                *((typeof(value) *)fdeq_emplace_ret_) = value;                 \
            }                                                                  \
        }                                                                      \
        fdeq_emplace_ret_;                                                     \
    }))

/** @private */
#define ccc_impl_fdeq_emplace_front(fdeq_ptr, value...)                        \
    (__extension__({                                                           \
        __auto_type fdeq_ptr_ = (fdeq_ptr);                                    \
        void *const fdeq_emplace_ret_ = NULL;                                  \
        if (fdeq_ptr_)                                                         \
        {                                                                      \
            void *const fdeq_emplace_ret_                                      \
                = ccc_impl_fdeq_alloc_front(fdeq_ptr_);                        \
            if (fdeq_emplace_ret_)                                             \
            {                                                                  \
                *((typeof(value) *)fdeq_emplace_ret_) = value;                 \
            }                                                                  \
        }                                                                      \
        fdeq_emplace_ret_;                                                     \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H */
