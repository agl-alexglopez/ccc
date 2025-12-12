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
#ifndef CCC_PRIVATE_FLAT_DOUBLE_ENDED_QUEUE_H
#define CCC_PRIVATE_FLAT_DOUBLE_ENDED_QUEUE_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../buffer.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @internal A flat double ended queue is a single Buffer with push and pop
at the front and back. If no allocation is permitted it is a ring buffer.
Because the `CCC_Buffer` abstraction already exists, the flat_double_ended_queue
can be implemented with a single additional field rather than a front and back
pointer. The back of the flat_double_ended_queue is always known if we know
where the front is and how many elements are stored in the buffer. */
struct CCC_Flat_double_ended_queue
{
    /** @internal The helper Buffer abstraction the flat_double_ended_queue
     * owns.
     */
    CCC_Buffer buffer;
    /** @internal The front of the flat_double_ended_queue. The back is implicit
     * given the size. */
    size_t front;
};

/*=======================    Private Interface   ============================*/

/** @internal */
void *CCC_private_flat_double_ended_queue_allocate_front(
    struct CCC_Flat_double_ended_queue *);
/** @internal */
void *CCC_private_flat_double_ended_queue_allocate_back(
    struct CCC_Flat_double_ended_queue *);

/*=======================  Macro Implementations   ==========================*/

/** @internal */
#define CCC_private_flat_double_ended_queue_initialize(                        \
    private_data_pointer, private_type_name, private_allocate,                 \
    private_context_data, private_capacity, optional_size...)                  \
    {                                                                          \
        .buffer = CCC_buffer_initialize(                                       \
            private_data_pointer, private_type_name, private_allocate,         \
            private_context_data, private_capacity, optional_size),            \
        .front = 0,                                                            \
    }

/** @internal Takes a compound literal array to initialize the buffer. */
#define CCC_private_flat_double_ended_queue_from(                              \
    private_allocate, private_context_data, private_optional_capacity,         \
    private_compound_literal_array...)                                         \
    (__extension__({                                                           \
        struct CCC_Flat_double_ended_queue private_flat_double_ended_queue = { \
            .buffer = CCC_buffer_from(private_allocate, private_context_data,  \
                                      private_optional_capacity,               \
                                      private_compound_literal_array),         \
            .front = 0,                                                        \
        };                                                                     \
        private_flat_double_ended_queue;                                       \
    }))

#define CCC_private_flat_double_ended_queue_with_capacity(                     \
    private_type_name, private_allocate, private_context_data,                 \
    private_capacity)                                                          \
    (__extension__({                                                           \
        struct CCC_Flat_double_ended_queue private_flat_double_ended_queue = { \
            .buffer = CCC_buffer_with_capacity(                                \
                private_type_name, private_allocate, private_context_data,     \
                private_capacity),                                             \
            .front = 0,                                                        \
        };                                                                     \
        private_flat_double_ended_queue;                                       \
    }))

/** @internal */
#define CCC_private_flat_double_ended_with_compound_literal(                   \
    private_count, private_compound_literal...)                                \
    {                                                                          \
        .buffer = CCC_buffer_with_compound_literal(private_count,              \
                                                   private_compound_literal),  \
        .front = 0,                                                            \
    }

/** @internal */
#define CCC_private_flat_double_ended_with_context_compound_literal(           \
    private_context, private_count, private_compound_literal...)               \
    {                                                                          \
        .buffer = CCC_buffer_with_context_compound_literal(                    \
            private_context, private_count, private_compound_literal),         \
        .front = 0,                                                            \
    }

/** @internal */
#define CCC_private_flat_double_ended_queue_emplace_back(                      \
    flat_double_ended_queue_pointer, value...)                                 \
    (__extension__({                                                           \
        __auto_type private_flat_double_ended_queue_pointer                    \
            = (flat_double_ended_queue_pointer);                               \
        void *const private_flat_double_ended_queue_emplace_ret = NULL;        \
        if (private_flat_double_ended_queue_pointer)                           \
        {                                                                      \
            void *const private_flat_double_ended_queue_emplace_ret            \
                = CCC_private_flat_double_ended_queue_allocate_back(           \
                    private_flat_double_ended_queue_pointer);                  \
            if (private_flat_double_ended_queue_emplace_ret)                   \
            {                                                                  \
                *((typeof(value) *)                                            \
                      private_flat_double_ended_queue_emplace_ret)             \
                    = value;                                                   \
            }                                                                  \
        }                                                                      \
        private_flat_double_ended_queue_emplace_ret;                           \
    }))

/** @internal */
#define CCC_private_flat_double_ended_queue_emplace_front(                     \
    flat_double_ended_queue_pointer, value...)                                 \
    (__extension__({                                                           \
        __auto_type private_flat_double_ended_queue_pointer                    \
            = (flat_double_ended_queue_pointer);                               \
        void *const private_flat_double_ended_queue_emplace_ret = NULL;        \
        if (private_flat_double_ended_queue_pointer)                           \
        {                                                                      \
            void *const private_flat_double_ended_queue_emplace_ret            \
                = CCC_private_flat_double_ended_queue_allocate_front(          \
                    private_flat_double_ended_queue_pointer);                  \
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

#endif /* CCC_PRIVATE_FLAT_DOUBLE_ENDED_QUEUE_H */
