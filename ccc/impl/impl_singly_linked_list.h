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
#ifndef CCC_IMPL_SINGLY_LINKED_LIST_H
#define CCC_IMPL_SINGLY_LINKED_LIST_H

/** @cond */
#include <assert.h>
#include <stddef.h>
/** @endcond */

#include "../types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private */
struct ccc_sll_elem
{
    struct ccc_sll_elem *n;
};

/** @private */
struct ccc_sll
{
    struct ccc_sll_elem nil;
    size_t count;
    size_t sizeof_type;
    size_t sll_elem_offset;
    ccc_any_alloc_fn *alloc;
    ccc_any_type_cmp_fn *cmp;
    void *aux;
};

/*=========================   Private Interface  ============================*/

/** @private */
void ccc_impl_sll_push_front(struct ccc_sll *, struct ccc_sll_elem *);

/*======================   Macro Implementations     ========================*/

/** @private */
#define ccc_impl_sll_init(impl_sll_name, impl_struct_name,                     \
                          impl_sll_elem_field, impl_cmp_fn, impl_alloc_fn,     \
                          impl_aux_data)                                       \
    {                                                                          \
        .nil.n = &(impl_sll_name).nil,                                         \
        .sizeof_type = sizeof(impl_struct_name),                               \
        .sll_elem_offset = offsetof(impl_struct_name, impl_sll_elem_field),    \
        .count = 0,                                                            \
        .alloc = (impl_alloc_fn),                                              \
        .cmp = (impl_cmp_fn),                                                  \
        .aux = (impl_aux_data),                                                \
    }

/** @private */
#define ccc_impl_sll_emplace_front(list_ptr, struct_initializer...)            \
    (__extension__({                                                           \
        typeof(struct_initializer) *impl_sll_res = NULL;                       \
        struct ccc_sll *impl_sll = (list_ptr);                                 \
        if (impl_sll)                                                          \
        {                                                                      \
            if (!impl_sll->alloc)                                              \
            {                                                                  \
                impl_sll_res = NULL;                                           \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_sll_res = impl_sll->alloc(NULL, impl_sll->sizeof_type);   \
                if (impl_sll_res)                                              \
                {                                                              \
                    *impl_sll_res = struct_initializer;                        \
                    ccc_impl_sll_push_front(                                   \
                        impl_sll, ccc_sll_elem_in(impl_sll, impl_sll_res));    \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_sll_res;                                                          \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_SINGLY_LINKED_LIST_H */
