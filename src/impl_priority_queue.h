#ifndef CCC_IMPL_PRIORITY_QUEUE_H
#define CCC_IMPL_PRIORITY_QUEUE_H

#include "types.h"

#include <stddef.h>

struct ccc_pq_elem_
{
    /* If any of the below fields are changed make sure to adjust how
       the offset of this struct is determined when returning the
       user struct base to them. See implementation code
       (struct_base function). */
    struct ccc_pq_elem_ *left_child;
    struct ccc_pq_elem_ *next_sibling;
    struct ccc_pq_elem_ *prev_sibling;
    struct ccc_pq_elem_ *parent;
};

struct ccc_pq_
{
    struct ccc_pq_elem_ *root;
    size_t sz;
    size_t pq_elem_offset;
    size_t elem_sz;
    ccc_alloc_fn *alloc;
    ccc_cmp_fn *cmp;
    ccc_threeway_cmp order;
    void *aux;
};

#define CCC_IMPL_PQ_INIT(struct_name, pq_elem_field, pq_order, alloc_fn,       \
                         cmp_fn, aux_data)                                     \
    {                                                                          \
        .impl_ = {                                                             \
            .root = NULL,                                                      \
            .sz = 0,                                                           \
            .pq_elem_offset = offsetof(struct_name, pq_elem_field),            \
            .elem_sz = sizeof(struct_name),                                    \
            .alloc = (alloc_fn),                                               \
            .cmp = (cmp_fn),                                                   \
            .order = (pq_order),                                               \
            .aux = (aux_data),                                                 \
        },                                                                     \
    }

#endif /* CCC_IMPL_PRIORITY_QUEUE_H */
