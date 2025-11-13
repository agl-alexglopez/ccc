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
struct CCC_fpq
{
    /** @private The underlying Buffer owned by the fpq. */
    CCC_Buffer buf;
    /** @private The order `CCC_ORDER_LESS` (min) or `CCC_ORDER_GREATER` (max)
     * of the fpq. */
    CCC_Order order;
    /** @private The user defined three way comparison function. */
    CCC_Type_comparator *cmp;
};

/*========================    Private Interface     =========================*/

/** @private */
size_t CCC_private_fpq_bubble_up(struct CCC_fpq *, void *, size_t);
/** @private */
void CCC_private_fpq_in_place_heapify(struct CCC_fpq *, size_t n, void *tmp);
/** @private */
void *CCC_private_fpq_update_fixup(struct CCC_fpq *, void *, void *tmp);

/*======================    Macro Implementations    ========================*/

/** @private */
#define CCC_private_fpq_initialize(                                            \
    private_mem_ptr, private_any_type_name, private_cmp_order, private_cmp_fn, \
    private_alloc_fn, private_aux_data, private_capacity)                      \
    {                                                                          \
        .buf = CCC_buffer_initialize(private_mem_ptr, private_any_type_name,   \
                                     private_alloc_fn, private_aux_data,       \
                                     private_capacity),                        \
        .cmp = (private_cmp_fn),                                               \
        .order = (private_cmp_order),                                          \
    }

/** @private */
#define CCC_private_fpq_heapify_initialize(                                    \
    private_mem_ptr, private_any_type_name, private_cmp_order, private_cmp_fn, \
    private_alloc_fn, private_aux_data, private_capacity, private_size)        \
    (__extension__({                                                           \
        __auto_type private_fpq_heapify_mem = (private_mem_ptr);               \
        struct CCC_fpq private_fpq_heapify_res = CCC_private_fpq_initialize(   \
            private_fpq_heapify_mem, private_any_type_name, private_cmp_order, \
            private_cmp_fn, private_alloc_fn, private_aux_data,                \
            private_capacity);                                                 \
        CCC_private_fpq_in_place_heapify(&private_fpq_heapify_res,             \
                                         (private_size),                       \
                                         &(private_any_type_name){0});         \
        private_fpq_heapify_res;                                               \
    }))

/** @private This macro "returns" a value thanks to clang and gcc statement
   expressions. See documentation in the flat pqueue header for usage. The ugly
   details of the macro are hidden here in the impl header. */
#define CCC_private_fpq_emplace(fpq, type_initializer...)                      \
    (__extension__({                                                           \
        struct CCC_fpq *private_fpq = (fpq);                                   \
        typeof(type_initializer) *private_fpq_res                              \
            = CCC_buffer_alloc_back(&private_fpq->buf);                        \
        if (private_fpq_res)                                                   \
        {                                                                      \
            *private_fpq_res = type_initializer;                               \
            if (private_fpq->buf.count > 1)                                    \
            {                                                                  \
                private_fpq_res = CCC_buffer_at(                               \
                    &private_fpq->buf,                                         \
                    CCC_private_fpq_bubble_up(private_fpq,                     \
                                              &(typeof(type_initializer)){0},  \
                                              private_fpq->buf.count - 1));    \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_fpq_res = CCC_buffer_at(&private_fpq->buf, 0);         \
            }                                                                  \
        }                                                                      \
        private_fpq_res;                                                       \
    }))

/** @private Only one update fn is needed because there is no advantage to
   updates if it is known they are min/max increase/decrease etc. */
#define CCC_private_fpq_update_w(fpq_ptr, T_ptr, update_closure_over_T...)     \
    (__extension__({                                                           \
        struct CCC_fpq *const private_fpq = (fpq_ptr);                         \
        typeof(*T_ptr) *T = (T_ptr);                                           \
        if (private_fpq && !CCC_buffer_is_empty(&private_fpq->buf) && T)       \
        {                                                                      \
            {update_closure_over_T} T = CCC_private_fpq_update_fixup(          \
                private_fpq, T, &(typeof(*T_ptr)){0});                         \
        }                                                                      \
        T;                                                                     \
    }))

/** @private */
#define CCC_private_fpq_increase_w(fpq_ptr, T_ptr, increase_closure_over_T...) \
    CCC_private_fpq_update_w(fpq_ptr, T_ptr, increase_closure_over_T)

/** @private */
#define CCC_private_fpq_decrease_w(fpq_ptr, T_ptr, decrease_closure_over_T...) \
    CCC_private_fpq_update_w(fpq_ptr, T_ptr, decrease_closure_over_T)

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_PRIORITY_QUEUE_H */
