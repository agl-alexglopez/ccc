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
#ifndef CCC_IMPL_PRIORITY_QUEUE_H
#define CCC_IMPL_PRIORITY_QUEUE_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private */
struct ccc_pq_elem
{
    struct ccc_pq_elem *left_child;
    struct ccc_pq_elem *next_sibling;
    struct ccc_pq_elem *prev_sibling;
    struct ccc_pq_elem *parent;
};

/** @private */
struct ccc_pq
{
    struct ccc_pq_elem *root;
    size_t count;
    size_t pq_elem_offset;
    size_t sizeof_type;
    ccc_any_alloc_fn *alloc;
    ccc_any_type_cmp_fn *cmp;
    ccc_threeway_cmp order;
    void *aux;
};

/*=========================  Private Interface     ==========================*/

/** @private */
void ccc_impl_pq_push(struct ccc_pq *, struct ccc_pq_elem *);
/** @private */
struct ccc_pq_elem *ccc_impl_pq_elem_in(struct ccc_pq const *, void const *);
/** @private */
ccc_threeway_cmp ccc_impl_pq_cmp(struct ccc_pq const *,
                                 struct ccc_pq_elem const *,
                                 struct ccc_pq_elem const *);
/** @private */
struct ccc_pq_elem *ccc_impl_pq_merge(struct ccc_pq *pq,
                                      struct ccc_pq_elem *old,
                                      struct ccc_pq_elem *new);
/** @private */
void ccc_impl_pq_cut_child(struct ccc_pq_elem *);
/** @private */
void ccc_impl_pq_init_node(struct ccc_pq_elem *);
/** @private */
struct ccc_pq_elem *ccc_impl_pq_delete_node(struct ccc_pq *,
                                            struct ccc_pq_elem *);

/*=========================  Macro Implementations     ======================*/

/** @private */
#define ccc_impl_pq_init(impl_struct_name, impl_pq_elem_field, impl_pq_order,  \
                         impl_cmp_fn, impl_alloc_fn, impl_aux_data)            \
    {                                                                          \
        .root = nullptr,                                                       \
        .count = 0,                                                            \
        .pq_elem_offset = offsetof(impl_struct_name, impl_pq_elem_field),      \
        .sizeof_type = sizeof(impl_struct_name),                               \
        .alloc = (impl_alloc_fn),                                              \
        .cmp = (impl_cmp_fn),                                                  \
        .order = (impl_pq_order),                                              \
        .aux = (impl_aux_data),                                                \
    }

/** @private */
#define ccc_impl_pq_emplace(pq_ptr, lazy_value...)                             \
    (__extension__({                                                           \
        typeof(lazy_value) *impl_pq_res = nullptr;                             \
        struct ccc_pq *impl_pq = (pq_ptr);                                     \
        if (impl_pq)                                                           \
        {                                                                      \
            if (!impl_pq->alloc)                                               \
            {                                                                  \
                impl_pq_res = nullptr;                                         \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_pq_res = impl_pq->alloc(nullptr, impl_pq->sizeof_type,    \
                                             impl_pq->aux);                    \
                if (impl_pq_res)                                               \
                {                                                              \
                    *impl_pq_res = lazy_value;                                 \
                    ccc_impl_pq_push(                                          \
                        impl_pq, ccc_impl_pq_elem_in(impl_pq, impl_pq_res));   \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_pq_res;                                                           \
    }))

/** @private */
#define ccc_impl_pq_update_w(pq_ptr, pq_elem_ptr, update_closure_over_T...)    \
    (__extension__({                                                           \
        struct ccc_pq *const impl_pq = (pq_ptr);                               \
        ccc_tribool impl_pq_update_res = CCC_FALSE;                            \
        struct ccc_pq_elem *const impl_pq_node_ptr = (pq_elem_ptr);            \
        if (impl_pq && impl_pq_node_ptr && impl_pq_node_ptr->next_sibling      \
            && impl_pq_node_ptr->prev_sibling)                                 \
        {                                                                      \
            impl_pq_update_res = CCC_TRUE;                                     \
            if (impl_pq_node_ptr->parent                                       \
                && ccc_impl_pq_cmp(impl_pq, impl_pq_node_ptr,                  \
                                   impl_pq_node_ptr->parent)                   \
                       == impl_pq->order)                                      \
            {                                                                  \
                ccc_impl_pq_cut_child(impl_pq_node_ptr);                       \
                {update_closure_over_T} impl_pq->root = ccc_impl_pq_merge(     \
                    impl_pq, impl_pq->root, impl_pq_node_ptr);                 \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_pq->root                                                  \
                    = ccc_impl_pq_delete_node(impl_pq, impl_pq_node_ptr);      \
                ccc_impl_pq_init_node(impl_pq_node_ptr);                       \
                {update_closure_over_T} impl_pq->root = ccc_impl_pq_merge(     \
                    impl_pq, impl_pq->root, impl_pq_node_ptr);                 \
            }                                                                  \
        }                                                                      \
        impl_pq_update_res;                                                    \
    }))

/** @private */
#define ccc_impl_pq_increase_w(pq_ptr, pq_elem_ptr,                            \
                               increase_closure_over_T...)                     \
    (__extension__({                                                           \
        struct ccc_pq *const impl_pq = (pq_ptr);                               \
        ccc_tribool impl_pq_increase_res = CCC_FALSE;                          \
        struct ccc_pq_elem *const impl_pq_elem_ptr = (pq_elem_ptr);            \
        if (impl_pq && impl_pq_elem_ptr && impl_pq_elem_ptr->next_sibling      \
            && impl_pq_elem_ptr->prev_sibling)                                 \
        {                                                                      \
            impl_pq_increase_res = CCC_TRUE;                                   \
            if (impl_pq->order == CCC_GRT)                                     \
            {                                                                  \
                ccc_impl_pq_cut_child(impl_pq_elem_ptr);                       \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_pq->root                                                  \
                    = ccc_impl_pq_delete_node(impl_pq, impl_pq_elem_ptr);      \
                ccc_impl_pq_init_node(impl_pq_elem_ptr);                       \
            }                                                                  \
            {increase_closure_over_T} impl_pq->root                            \
                = ccc_impl_pq_merge(impl_pq, impl_pq->root, impl_pq_elem_ptr); \
        }                                                                      \
        impl_pq_increase_res;                                                  \
    }))

/** @private */
#define ccc_impl_pq_decrease_w(pq_ptr, pq_elem_ptr,                            \
                               decrease_closure_over_T...)                     \
    (__extension__({                                                           \
        struct ccc_pq *const impl_pq = (pq_ptr);                               \
        ccc_tribool impl_pq_decrease_res = CCC_FALSE;                          \
        struct ccc_pq_elem *const impl_pq_elem_ptr = (pq_elem_ptr);            \
        if (impl_pq && impl_pq_elem_ptr && impl_pq_elem_ptr->next_sibling      \
            && impl_pq_elem_ptr->prev_sibling)                                 \
        {                                                                      \
            impl_pq_decrease_res = CCC_TRUE;                                   \
            if (impl_pq->order == CCC_LES)                                     \
            {                                                                  \
                ccc_impl_pq_cut_child(impl_pq_elem_ptr);                       \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_pq->root                                                  \
                    = ccc_impl_pq_delete_node(impl_pq, impl_pq_elem_ptr);      \
                ccc_impl_pq_init_node(impl_pq_elem_ptr);                       \
            }                                                                  \
            {decrease_closure_over_T} impl_pq->root                            \
                = ccc_impl_pq_merge(impl_pq, impl_pq->root, impl_pq_elem_ptr); \
        }                                                                      \
        impl_pq_decrease_res;                                                  \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_PRIORITY_QUEUE_H */
