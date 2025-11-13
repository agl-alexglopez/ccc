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
#ifndef CCC_IMPL_FLAT_PRIORITY_QUEUE_H
#define CCC_IMPL_FLAT_PRIORITY_QUEUE_H

/** @cond */
#include <assert.h>
#include <stddef.h>
/** @endcond */

#include "../buffer.h"
#include "../types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private A flat priority queue is a binary heap in a contiguous buffer
storing an implicit complete binary tree; elements are stored contiguously from
`[0, N)`. Starting at any node in the tree at index i the parent is at `(i - 1)
/ 2`, the left child is at `(i * 2) + 1`, and the right child is at `(i * 2) +
2`. The heap can be initialized as a min or max heap due to the use of the three
way comparison function. */
struct CCC_Flat_priority_queue
{
    /** @private The underlying Buffer owned by the flat_priority_queue. */
    CCC_Buffer buf;
    /** @private The order `CCC_ORDER_LESSER` (min) or `CCC_ORDER_GREATER` (max)
     * of the flat_priority_queue. */
    CCC_Order order;
    /** @private The user defined three way comparison function. */
    CCC_Type_comparator *cmp;
};

/*========================    Private Interface     =========================*/

/** @private */
size_t
CCC_private_flat_priority_queue_bubble_up(struct CCC_Flat_priority_queue *,
                                          void *, size_t);
/** @private */
void CCC_private_flat_priority_queue_in_place_heapify(
    struct CCC_Flat_priority_queue *, size_t n, void *tmp);
/** @private */
void *
CCC_private_flat_priority_queue_update_fixup(struct CCC_Flat_priority_queue *,
                                             void *, void *tmp);

/*======================    Macro Implementations    ========================*/

/** @private */
#define CCC_private_flat_priority_queue_initialize(                            \
    private_mem_ptr, private_any_type_name, private_cmp_order, private_cmp_fn, \
    private_alloc_fn, private_context_data, private_capacity)                  \
    {                                                                          \
        .buf = CCC_buffer_initialize(private_mem_ptr, private_any_type_name,   \
                                     private_alloc_fn, private_context_data,   \
                                     private_capacity),                        \
        .cmp = (private_cmp_fn),                                               \
        .order = (private_cmp_order),                                          \
    }

/** @private */
#define CCC_private_flat_priority_queue_heapify_initialize(                    \
    private_mem_ptr, private_any_type_name, private_cmp_order, private_cmp_fn, \
    private_alloc_fn, private_context_data, private_capacity, private_size)    \
    (__extension__({                                                           \
        __auto_type private_flat_priority_queue_heapify_mem                    \
            = (private_mem_ptr);                                               \
        struct CCC_Flat_priority_queue private_flat_priority_queue_heapify_res \
            = CCC_private_flat_priority_queue_initialize(                      \
                private_flat_priority_queue_heapify_mem,                       \
                private_any_type_name, private_cmp_order, private_cmp_fn,      \
                private_alloc_fn, private_context_data, private_capacity);     \
        CCC_private_flat_priority_queue_in_place_heapify(                      \
            &private_flat_priority_queue_heapify_res, (private_size),          \
            &(private_any_type_name){0});                                      \
        private_flat_priority_queue_heapify_res;                               \
    }))

/** @private This macro "returns" a value thanks to clang and gcc statement
   expressions. See documentation in the flat priority_queueueue header for
   usage. The ugly details of the macro are hidden here in the impl header. */
#define CCC_private_flat_priority_queue_emplace(flat_priority_queue,           \
                                                type_initializer...)           \
    (__extension__({                                                           \
        struct CCC_Flat_priority_queue *private_flat_priority_queue            \
            = (flat_priority_queue);                                           \
        typeof(type_initializer) *private_flat_priority_queue_res              \
            = CCC_buffer_alloc_back(&private_flat_priority_queue->buf);        \
        if (private_flat_priority_queue_res)                                   \
        {                                                                      \
            *private_flat_priority_queue_res = type_initializer;               \
            if (private_flat_priority_queue->buf.count > 1)                    \
            {                                                                  \
                private_flat_priority_queue_res = CCC_buffer_at(               \
                    &private_flat_priority_queue->buf,                         \
                    CCC_private_flat_priority_queue_bubble_up(                 \
                        private_flat_priority_queue,                           \
                        &(typeof(type_initializer)){0},                        \
                        private_flat_priority_queue->buf.count - 1));          \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_flat_priority_queue_res                                \
                    = CCC_buffer_at(&private_flat_priority_queue->buf, 0);     \
            }                                                                  \
        }                                                                      \
        private_flat_priority_queue_res;                                       \
    }))

/** @private Only one update fn is needed because there is no advantage to
   updates if it is known they are min/max increase/decrease etc. */
#define CCC_private_flat_priority_queue_update_w(                              \
    flat_priority_queue_ptr, T_ptr, update_closure_over_T...)                  \
    (__extension__({                                                           \
        struct CCC_Flat_priority_queue *const private_flat_priority_queue      \
            = (flat_priority_queue_ptr);                                       \
        typeof(*T_ptr) *T = (T_ptr);                                           \
        if (private_flat_priority_queue                                        \
            && !CCC_buffer_is_empty(&private_flat_priority_queue->buf) && T)   \
        {                                                                      \
            {update_closure_over_T} T                                          \
                = CCC_private_flat_priority_queue_update_fixup(                \
                    private_flat_priority_queue, T, &(typeof(*T_ptr)){0});     \
        }                                                                      \
        T;                                                                     \
    }))

/** @private */
#define CCC_private_flat_priority_queue_increase_w(                            \
    flat_priority_queue_ptr, T_ptr, increase_closure_over_T...)                \
    CCC_private_flat_priority_queue_update_w(flat_priority_queue_ptr, T_ptr,   \
                                             increase_closure_over_T)

/** @private */
#define CCC_private_flat_priority_queue_decrease_w(                            \
    flat_priority_queue_ptr, T_ptr, decrease_closure_over_T...)                \
    CCC_private_flat_priority_queue_update_w(flat_priority_queue_ptr, T_ptr,   \
                                             decrease_closure_over_T)

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_PRIORITY_QUEUE_H */
