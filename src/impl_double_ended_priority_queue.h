#ifndef CCC_IMPL_DOUBLE_ENDED_PRIORITY_QUEUE_H
#define CCC_IMPL_DOUBLE_ENDED_PRIORITY_QUEUE_H

#include "impl_tree.h"

struct ccc_node_;
struct ccc_tree_;

#define ccc_impl_depq_init(struct_name, node_elem_field, key_elem_field,       \
                           tree_name, alloc_fn, key_cmp_fn, aux_data)          \
    ccc_tree_init(struct_name, node_elem_field, key_elem_field, tree_name,     \
                  alloc_fn, key_cmp_fn, aux_data)

void *ccc_impl_depq_key_in_slot(struct ccc_tree_ const *t, void const *slot);
ccc_node_ *ccc_impl_depq_elem_in_slot(struct ccc_tree_ const *t,
                                      void const *slot);
void *ccc_impl_depq_key_from_node(struct ccc_tree_ const *t,
                                  ccc_node_ const *n);

#endif /* CCC_IMPL_DOUBLE_ENDED_PRIORITY_QUEUE_H */
