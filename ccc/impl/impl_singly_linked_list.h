#ifndef CCC_IMPL_SINGLY_LINKED_LIST_H
#define CCC_IMPL_SINGLY_LINKED_LIST_H

#include "../types.h"

/** @cond */
#include <assert.h>
#include <stddef.h>
/** @endcond */

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
    ccc_alloc_fn *alloc_;
    ccc_cmp_fn *cmp_;
    void *aux_;
};

#define ccc_impl_sll_init(sll_name, struct_name, sll_elem_field, alloc_fn,     \
                          cmp_fn, aux_data)                                    \
    {                                                                          \
        .sentinel_.n_ = &(sll_name).sentinel_,                                 \
        .elem_sz_ = sizeof(struct_name),                                       \
        .sll_elem_offset_ = offsetof(struct_name, sll_elem_field),             \
        .sz_ = 0,                                                              \
        .alloc_ = (alloc_fn),                                                  \
        .cmp_ = (cmp_fn),                                                      \
        .aux_ = (aux_data),                                                    \
    }

void ccc_impl_sll_push_front(struct ccc_sll_ *, struct ccc_sll_elem_ *);

/* NOLINTBEGIN(readability-identifier-naming) */

#define ccc_impl_sll_emplace_front(list_ptr, struct_initializer...)            \
    ({                                                                         \
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
    })

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_SINGLY_LINKED_LIST_H */
