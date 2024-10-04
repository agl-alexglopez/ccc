#ifndef CCC_IMPL_FLAT_ORDERED_MAP_H
#define CCC_IMPL_FLAT_ORDERED_MAP_H

#include "buffer.h"
#include "types.h"

#include <stddef.h>

struct ccc_fom_elem_
{
    size_t branch_[2];
    size_t parent_;
};

struct ccc_fom_
{
    ccc_buffer buf_;
    size_t root_;
    size_t key_offset_;
    size_t node_elem_offset_;
    ccc_key_cmp_fn *cmp_;
    void *aux_;
};

struct ccc_fom_entry_
{
    struct ccc_fom_ *fom_;
    ccc_threeway_cmp last_cmp_;
    /* The types.h entry is not quite suitable for this container so change. */
    size_t i_;
    /* Keep these types in sync in cases status reporting changes. */
    typeof((ccc_entry){}.impl_.stats_) stats_;
};

#define ccc_impl_fom_init(mem_ptr, capacity, struct_name, node_elem_field,     \
                          key_elem_field, alloc_fn, key_cmp_fn, aux_data)      \
    {                                                                          \
        .buf_ = ccc_buf_init(mem_ptr, struct_name, capacity, alloc_fn),        \
        .root_ = 0, .key_offset_ = offsetof(struct_name, key_elem_field),      \
        .node_elem_offset_ = offsetof(struct_name, node_elem_field),           \
        .cmp_ = (key_cmp_fn), .aux_ = (aux_data),                              \
    }

#endif /* CCC_IMPL_FLAT_ORDERED_MAP_H */
