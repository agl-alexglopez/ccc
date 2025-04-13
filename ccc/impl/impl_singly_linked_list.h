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
typedef struct ccc_sll_elem_
{
    struct ccc_sll_elem_ *n_;
} ccc_sll_elem_;

/** @private */
struct ccc_sll_
{
    struct ccc_sll_elem_ sentinel_;
    size_t sz_;
    size_t elem_sz_;
    size_t sll_elem_offset_;
    ccc_any_alloc_fn *alloc_;
    ccc_any_type_cmp_fn *cmp_;
    void *aux_;
};

/*=========================   Private Interface  ============================*/

/** @private */
void ccc_impl_sll_push_front(struct ccc_sll_ *, struct ccc_sll_elem_ *);

/*======================   Macro Implementations     ========================*/

/** @private */
#define ccc_impl_sll_init(sll_name, struct_name, sll_elem_field, cmp_fn,       \
                          alloc_fn, aux_data)                                  \
    {                                                                          \
        .sentinel_.n_ = &(sll_name).sentinel_,                                 \
        .elem_sz_ = sizeof(struct_name),                                       \
        .sll_elem_offset_ = offsetof(struct_name, sll_elem_field),             \
        .sz_ = 0,                                                              \
        .alloc_ = (alloc_fn),                                                  \
        .cmp_ = (cmp_fn),                                                      \
        .aux_ = (aux_data),                                                    \
    }

/** @private */
#define ccc_impl_sll_emplace_front(list_ptr, struct_initializer...)            \
    (__extension__({                                                           \
        typeof(struct_initializer) *sll_res_ = NULL;                           \
        struct ccc_sll_ *sll_ = (list_ptr);                                    \
        if (sll_)                                                              \
        {                                                                      \
            if (!sll_->alloc_)                                                 \
            {                                                                  \
                sll_res_ = NULL;                                               \
            }                                                                  \
            else                                                               \
            {                                                                  \
                sll_res_ = sll_->alloc_(NULL, sll_->elem_sz_);                 \
                if (sll_res_)                                                  \
                {                                                              \
                    *sll_res_ = struct_initializer;                            \
                    ccc_impl_sll_push_front(sll_,                              \
                                            ccc_sll_elem_in(sll_, sll_res_));  \
                }                                                              \
            }                                                                  \
        }                                                                      \
        sll_res_;                                                              \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_SINGLY_LINKED_LIST_H */
