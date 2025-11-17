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
#ifndef CCC_PRIVATE_FLAT_PRIORITY_QUEUE_H
#define CCC_PRIVATE_FLAT_PRIORITY_QUEUE_H

/** @cond */
#include <assert.h>
#include <stddef.h>
/** @endcond */

#include "../buffer.h"
#include "../types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @internal A flat priority queue is a binary heap in a contiguous buffer
storing an implicit complete binary tree; elements are stored contiguously from
`[0, N)`. Starting at any node in the tree at index i the parent is at `(i - 1)
/ 2`, the left child is at `(i * 2) + 1`, and the right child is at `(i * 2) +
2`. The heap can be initialized as a min or max heap due to the use of the three
way comparison function. */
struct CCC_Flat_priority_queue
{
    /** @internal The underlying Buffer owned by the flat_priority_queue. */
    CCC_Buffer buffer;
    /** @internal The order `CCC_ORDER_LESSER` (min) or `CCC_ORDER_GREATER`
     * (max) of the flat_priority_queue. */
    CCC_Order order;
    /** @internal The user defined three way comparison function. */
    CCC_Type_comparator *compare;
};

/*========================    Private Interface     =========================*/

/** @internal */
size_t
CCC_private_flat_priority_queue_bubble_up(struct CCC_Flat_priority_queue *,
                                          void *, size_t);
/** @internal */
void CCC_private_flat_priority_queue_in_place_heapify(
    struct CCC_Flat_priority_queue *, size_t, void *);
/** @internal */
void *
CCC_private_flat_priority_queue_update_fixup(struct CCC_Flat_priority_queue *,
                                             void *, void *);

/*======================    Macro Implementations    ========================*/

/** @internal */
#define CCC_private_flat_priority_queue_initialize(                            \
    private_data_pointer, private_type_name, private_order, private_compare,   \
    private_allocate, private_context_data, private_capacity)                  \
    {                                                                          \
        .buffer = CCC_buffer_initialize(                                       \
            private_data_pointer, private_type_name, private_allocate,         \
            private_context_data, private_capacity),                           \
        .order = (private_order),                                              \
        .compare = (private_compare),                                          \
    }

/** @internal */
#define CCC_private_flat_priority_queue_heapify_initialize(                    \
    private_data_pointer, private_type_name, private_order, private_compare,   \
    private_allocate, private_context_data, private_capacity, private_size)    \
    (__extension__({                                                           \
        typeof(*(                                                              \
            private_data_pointer)) *private_flat_priority_queue_heapify_data   \
            = (private_data_pointer);                                          \
        struct CCC_Flat_priority_queue private_flat_priority_queue_heapify_res \
            = CCC_private_flat_priority_queue_initialize(                      \
                private_flat_priority_queue_heapify_data, private_type_name,   \
                private_order, private_compare, private_allocate,              \
                private_context_data, private_capacity);                       \
        CCC_private_flat_priority_queue_in_place_heapify(                      \
            &private_flat_priority_queue_heapify_res, (private_size),          \
            &(private_type_name){0});                                          \
        private_flat_priority_queue_heapify_res;                               \
    }))

/** @internal */
#define CCC_private_flat_priority_queue_from(                                  \
    private_order, private_compare, private_allocate, private_context_data,    \
    private_optional_capacity, private_compound_literal_array...)              \
    (__extension__({                                                           \
        struct CCC_Flat_priority_queue private_flat_priority_queue = {         \
            .buffer = CCC_buffer_from(private_allocate, private_context_data,  \
                                      private_optional_capacity,               \
                                      private_compound_literal_array),         \
            .order = private_order,                                            \
            .compare = private_compare,                                        \
        };                                                                     \
        if (private_flat_priority_queue.buffer.count)                          \
        {                                                                      \
            CCC_private_flat_priority_queue_in_place_heapify(                  \
                &private_flat_priority_queue,                                  \
                private_flat_priority_queue.buffer.count,                      \
                &(typeof(*private_compound_literal_array)){0});                \
        }                                                                      \
        private_flat_priority_queue;                                           \
    }))

/** @internal */
#define CCC_private_flat_priority_queue_with_capacity(                         \
    private_type_name, private_order, private_compare, private_allocate,       \
    private_context_data, private_capacity)                                    \
    (__extension__({                                                           \
        struct CCC_Flat_priority_queue private_flat_priority_queue = {         \
            .buffer = CCC_buffer_with_capacity(                                \
                private_type_name, private_allocate, private_context_data,     \
                private_capacity),                                             \
            .order = (private_order),                                          \
            .compare = (private_compare),                                      \
        };                                                                     \
        private_flat_priority_queue;                                           \
    }))

/** @internal This macro "returns" a value thanks to clang and gcc statement
   expressions. See documentation in the flat priority_queueueue header for
   usage. The ugly details of the macro are hidden here in the impl header. */
#define CCC_private_flat_priority_queue_emplace(flat_priority_queue,           \
                                                type_compound_literal...)      \
    (__extension__({                                                           \
        struct CCC_Flat_priority_queue *private_flat_priority_queue            \
            = (flat_priority_queue);                                           \
        typeof(type_compound_literal) *private_flat_priority_queue_res         \
            = CCC_buffer_allocate_back(&private_flat_priority_queue->buffer);  \
        if (private_flat_priority_queue_res)                                   \
        {                                                                      \
            *private_flat_priority_queue_res = type_compound_literal;          \
            if (private_flat_priority_queue->buffer.count > 1)                 \
            {                                                                  \
                private_flat_priority_queue_res = CCC_buffer_at(               \
                    &private_flat_priority_queue->buffer,                      \
                    CCC_private_flat_priority_queue_bubble_up(                 \
                        private_flat_priority_queue,                           \
                        &(typeof(type_compound_literal)){0},                   \
                        private_flat_priority_queue->buffer.count - 1));       \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_flat_priority_queue_res                                \
                    = CCC_buffer_at(&private_flat_priority_queue->buffer, 0);  \
            }                                                                  \
        }                                                                      \
        private_flat_priority_queue_res;                                       \
    }))

/** @internal Only one update fn is needed because there is no advantage to
   updates if it is known they are min/max increase/decrease etc. */
#define CCC_private_flat_priority_queue_update_with(                           \
    flat_priority_queue_pointer, T_pointer, update_closure_over_T...)          \
    (__extension__({                                                           \
        struct CCC_Flat_priority_queue *const private_flat_priority_queue      \
            = (flat_priority_queue_pointer);                                   \
        typeof(*T_pointer) *T = (T_pointer);                                   \
        if (private_flat_priority_queue                                        \
            && !CCC_buffer_is_empty(&private_flat_priority_queue->buffer)      \
            && T)                                                              \
        {                                                                      \
            {update_closure_over_T} T                                          \
                = CCC_private_flat_priority_queue_update_fixup(                \
                    private_flat_priority_queue, T, &(typeof(*T_pointer)){0}); \
        }                                                                      \
        T;                                                                     \
    }))

/** @internal */
#define CCC_private_flat_priority_queue_increase_with(                         \
    flat_priority_queue_pointer, T_pointer, increase_closure_over_T...)        \
    CCC_private_flat_priority_queue_update_with(                               \
        flat_priority_queue_pointer, T_pointer, increase_closure_over_T)

/** @internal */
#define CCC_private_flat_priority_queue_decrease_with(                         \
    flat_priority_queue_pointer, T_pointer, decrease_closure_over_T...)        \
    CCC_private_flat_priority_queue_update_with(                               \
        flat_priority_queue_pointer, T_pointer, decrease_closure_over_T)

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_PRIVATE_FLAT_PRIORITY_QUEUE_H */
