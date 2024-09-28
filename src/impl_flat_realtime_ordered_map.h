#ifndef IMPL_FLAT_REALTIME_ORDERED_MAP_H
#define IMPL_FLAT_REALTIME_ORDERED_MAP_H

#include "buffer.h"
#include "types.h"

#include <stddef.h>
#include <stdint.h>

struct ccc_frm_elem_
{
    size_t branch_[2];
    size_t parent_;
    uint8_t parity_;
};

struct ccc_frm_
{
    ccc_buffer buf_;
    size_t root_;
    size_t key_offset_;
    size_t node_elem_offset_;
    ccc_key_cmp_fn *cmp_;
    void *aux_;
};

struct ccc_frm_entry_
{
    struct ccc_frm_ *frm_;
    ccc_threeway_cmp last_cmp_;
    struct ccc_entry_ entry_;
};

void *ccc_impl_frm_key_from_node(struct ccc_frm_ const *frm,
                                 struct ccc_frm_elem_ const *elem);
void *ccc_impl_frm_key_in_slot(struct ccc_frm_ const *frm, void const *slot);
struct ccc_frm_elem_ *ccc_impl_frm_elem_in_slot(struct ccc_frm_ const *frm,
                                                void const *slot);
struct ccc_frm_entry_ ccc_impl_frm_entry(struct ccc_frm_ const *frm,
                                         void const *key);
void *ccc_impl_frm_insert(struct ccc_frm_ *frm, size_t parent_i,
                          ccc_threeway_cmp last_cmp, size_t elem_i);

#define ccc_impl_frm_init(memory_ptr, capacity, struct_name, node_elem_field,  \
                          key_elem_field, alloc_fn, key_cmp_fn, aux_data)      \
    {                                                                          \
        .buf_ = ccc_buf_init(memory_ptr, struct_name, capacity, alloc_fn),     \
        .root_ = 0, .key_offset_ = offsetof(struct_name, key_elem_field),      \
        .node_elem_offset_ = offsetof(struct_name, node_elem_field),           \
        .cmp_ = (key_cmp_fn), .aux_ = (aux_data),                              \
    }

#endif /* IMPL_FLAT_REALTIME_ORDERED_MAP_H */
