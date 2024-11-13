#ifndef CCC_IMPL_PRIORITY_QUEUE_H
#define CCC_IMPL_PRIORITY_QUEUE_H

#include "types.h"

/** @cond */
#include <stddef.h>
/** @endcond */

/** @private */
struct ccc_pq_elem_
{
    struct ccc_pq_elem_ *left_child_;
    struct ccc_pq_elem_ *next_sibling_;
    struct ccc_pq_elem_ *prev_sibling_;
    struct ccc_pq_elem_ *parent_;
};

/** @private */
struct ccc_pq_
{
    struct ccc_pq_elem_ *root_;
    size_t sz_;
    size_t pq_elem_offset_;
    size_t elem_sz_;
    ccc_alloc_fn *alloc_;
    ccc_cmp_fn *cmp_;
    ccc_threeway_cmp order_;
    void *aux_;
};

#define ccc_impl_pq_init(struct_name, pq_elem_field, pq_order, alloc_fn,       \
                         cmp_fn, aux_data)                                     \
    {                                                                          \
        .root_ = NULL, .sz_ = 0,                                               \
        .pq_elem_offset_ = offsetof(struct_name, pq_elem_field),               \
        .elem_sz_ = sizeof(struct_name), .alloc_ = (alloc_fn),                 \
        .cmp_ = (cmp_fn), .order_ = (pq_order), .aux_ = (aux_data),            \
    }

#endif /* CCC_IMPL_PRIORITY_QUEUE_H */
