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

/** @private */
struct ccc_fpq
{
    ccc_buffer buf;
    ccc_any_type_cmp_fn *cmp;
    ccc_threeway_cmp order;
};

/*========================    Private Interface     =========================*/

/** @private */
size_t ccc_impl_fpq_bubble_up(struct ccc_fpq *, char[], size_t);
/** @private */
void ccc_impl_fpq_in_place_heapify(struct ccc_fpq *, size_t n);
/** @private */
void *ccc_impl_fpq_update_fixup(struct ccc_fpq *, void *);

/*======================    Macro Implementations    ========================*/

/** @private */
#define ccc_impl_fpq_init(impl_mem_ptr, impl_cmp_order, impl_cmp_fn,           \
                          impl_alloc_fn, impl_aux_data, impl_capacity)         \
    {                                                                          \
        .buf = ccc_buf_init(impl_mem_ptr, impl_alloc_fn, impl_aux_data,        \
                            impl_capacity),                                    \
        .cmp = (impl_cmp_fn),                                                  \
        .order = (impl_cmp_order),                                             \
    }

/** @private */
#define ccc_impl_fpq_heapify_init(impl_mem_ptr, impl_cmp_order, impl_cmp_fn,   \
                                  impl_alloc_fn, impl_aux_data, impl_capacity, \
                                  impl_size)                                   \
    (__extension__({                                                           \
        __auto_type impl_fpq_heapify_mem = (impl_mem_ptr);                     \
        struct ccc_fpq impl_fpq_heapify_res = ccc_impl_fpq_init(               \
            impl_fpq_heapify_mem, impl_cmp_order, impl_cmp_fn, impl_alloc_fn,  \
            impl_aux_data, impl_capacity);                                     \
        ccc_impl_fpq_in_place_heapify(&impl_fpq_heapify_res, (impl_size));     \
        impl_fpq_heapify_res;                                                  \
    }))

/** @private This macro "returns" a value thanks to clang and gcc statement
   expressions. See documentation in the flat pqueue header for usage. The ugly
   details of the macro are hidden here in the impl header. */
#define ccc_impl_fpq_emplace(fpq, type_initializer...)                         \
    (__extension__({                                                           \
        typeof(type_initializer) *impl_fpq_res;                                \
        struct ccc_fpq *impl_fpq = (fpq);                                      \
        assert(sizeof(*impl_fpq_res)                                           \
               == ccc_buf_sizeof_type(&impl_fpq->buf).count);                  \
        impl_fpq_res = ccc_buf_alloc_back(&impl_fpq->buf);                     \
        if (ccc_buf_size(&impl_fpq->buf).count                                 \
            == ccc_buf_capacity(&impl_fpq->buf).count)                         \
        {                                                                      \
            impl_fpq_res = NULL;                                               \
            ccc_result const impl_extra_space = ccc_buf_alloc(                 \
                &impl_fpq->buf, ccc_buf_capacity(&impl_fpq->buf).count * 2,    \
                impl_fpq->buf.alloc);                                          \
            if (impl_extra_space == CCC_RESULT_OK)                             \
            {                                                                  \
                impl_fpq_res = ccc_buf_back(&impl_fpq->buf);                   \
            }                                                                  \
        }                                                                      \
        if (impl_fpq_res)                                                      \
        {                                                                      \
            *impl_fpq_res = type_initializer;                                  \
            if (ccc_buf_size(&impl_fpq->buf).count > 1)                        \
            {                                                                  \
                void *impl_fpq_tmp = ccc_buf_at(                               \
                    &impl_fpq->buf, ccc_buf_size(&impl_fpq->buf).count);       \
                impl_fpq_res                                                   \
                    = ccc_buf_at(&impl_fpq->buf,                               \
                                 ccc_impl_fpq_bubble_up(                       \
                                     impl_fpq, impl_fpq_tmp,                   \
                                     ccc_buf_size(&impl_fpq->buf).count - 1)); \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_fpq_res = ccc_buf_at(&impl_fpq->buf, 0);                  \
            }                                                                  \
        }                                                                      \
        impl_fpq_res;                                                          \
    }))

/** @private Only one update fn is needed because there is no advantage to
   updates if it is known they are min/max increase/decrease etc. */
#define ccc_impl_fpq_update_w(fpq_ptr, T_ptr, update_closure_over_T...)        \
    (__extension__({                                                           \
        struct ccc_fpq *const impl_fpq = (fpq_ptr);                            \
        void *impl_fpq_update_res = NULL;                                      \
        void *const impl_fpq_t_ptr = (T_ptr);                                  \
        if (impl_fpq && impl_fpq_t_ptr && !ccc_buf_is_empty(&impl_fpq->buf))   \
        {                                                                      \
            {update_closure_over_T} impl_fpq_update_res                        \
                = ccc_impl_fpq_update_fixup(impl_fpq, impl_fpq_t_ptr);         \
        }                                                                      \
        impl_fpq_update_res;                                                   \
    }))

/** @private */
#define ccc_impl_fpq_increase_w(fpq_ptr, T_ptr, increase_closure_over_T...)    \
    ccc_impl_fpq_update_w(fpq_ptr, T_ptr, increase_closure_over_T)

/** @private */
#define ccc_impl_fpq_decrease_w(fpq_ptr, T_ptr, decrease_closure_over_T...)    \
    ccc_impl_fpq_update_w(fpq_ptr, T_ptr, decrease_closure_over_T)

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_PRIORITY_QUEUE_H */
