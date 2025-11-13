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

/** @private A flat doubled ended queue is a single Buffer with push and pop
at the front and back. If no allocation is permitted it is a ring buffer.
Because the `CCC_Buffer` abstraction already exists, the flat_double_ended_queue
can be implemented with a single additional field rather than a front and back
pointer. The back of the flat_double_ended_queue is always known if we know
where the front is and how many elements are stored in the buffer. */
struct CCC_Flat_double_ended_queue
{
    /** @private The helper Buffer abstraction the flat_double_ended_queue owns.
     */
    CCC_Buffer buf;
    /** @private The front of the flat_double_ended_queue. The back is implicit
     * given the size. */
    size_t front;
};

/*=======================    Private Interface   ============================*/
/** @private */
void *CCC_private_flat_double_ended_queue_allocate_front(
    struct CCC_Flat_double_ended_queue *);
/** @private */
void *CCC_private_flat_double_ended_queue_allocate_back(
    struct CCC_Flat_double_ended_queue *);

/*=======================  Macro Implementations   ==========================*/

/** @private */
#define CCC_private_flat_double_ended_queue_initialize(                        \
    private_mem_ptr, private_any_type_name, private_allocate,                  \
    private_context_data, private_capacity, optional_size...)                  \
    {                                                                          \
        .buf = CCC_buffer_initialize(private_mem_ptr, private_any_type_name,   \
                                     private_allocate, private_context_data,   \
                                     private_capacity, optional_size),         \
        .front = 0,                                                            \
    }

/** @private */
#define CCC_private_flat_double_ended_queue_emplace_back(                      \
    flat_double_ended_queue_ptr, value...)                                     \
    (__extension__({                                                           \
        __auto_type private_flat_double_ended_queue_ptr                        \
            = (flat_double_ended_queue_ptr);                                   \
        void *const private_flat_double_ended_queue_emplace_ret = NULL;        \
        if (private_flat_double_ended_queue_ptr)                               \
        {                                                                      \
            void *const private_flat_double_ended_queue_emplace_ret            \
                = CCC_private_flat_double_ended_queue_allocate_back(           \
                    private_flat_double_ended_queue_ptr);                      \
            if (private_flat_double_ended_queue_emplace_ret)                   \
            {                                                                  \
                *((typeof(value) *)                                            \
                      private_flat_double_ended_queue_emplace_ret)             \
                    = value;                                                   \
            }                                                                  \
        }                                                                      \
        private_flat_double_ended_queue_emplace_ret;                           \
    }))

/** @private */
#define CCC_private_flat_double_ended_queue_emplace_front(                     \
    flat_double_ended_queue_ptr, value...)                                     \
    (__extension__({                                                           \
        __auto_type private_flat_double_ended_queue_ptr                        \
            = (flat_double_ended_queue_ptr);                                   \
        void *const private_flat_double_ended_queue_emplace_ret = NULL;        \
        if (private_flat_double_ended_queue_ptr)                               \
        {                                                                      \
            void *const private_flat_double_ended_queue_emplace_ret            \
                = CCC_private_flat_double_ended_queue_allocate_front(          \
                    private_flat_double_ended_queue_ptr);                      \
            if (private_flat_double_ended_queue_emplace_ret)                   \
            {                                                                  \
                *((typeof(value) *)                                            \
                      private_flat_double_ended_queue_emplace_ret)             \
                    = value;                                                   \
            }                                                                  \
        }                                                                      \
        private_flat_double_ended_queue_emplace_ret;                           \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H */
