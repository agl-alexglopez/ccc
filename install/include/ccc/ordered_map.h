/* Author: Alexander G. Lopez
   --------------------------
   This is the Map interface for the Splay Tree
   Map. In this case we modify a Splay Tree to allow for
   a true set (naturally sorted unique elements). See the
   priority queue for another use case of this data
   structure. A set can be an interesting option for a
   LRU cache. Any application such that there is biased
   distribution of access via lookup, insertion, and
   removal brings those elements closer to the root of
   the tree, approaching constant time operations. See
   also the multiset for great benefits of duplicates
   being taken from a data structure. The runtime is
   amortized O(lgN) but with the right use cases we
   may benefit from the O(1) capabilities of the working
   set. The anti-pattern is to seek and splay all elements
   to the tree in sequential order. However, any random
   variants will help maintain tree health and this interface
   provides robust iterators that can be used if read only
   access is required of all elements or only conditional
   modifications. This may combat such an anti-pattern. */
#ifndef CCC_ORDERED_MAP_H
#define CCC_ORDERED_MAP_H

#include "impl_tree.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    ccc_tree impl;
} ccc_ordered_map;

typedef struct
{
    ccc_node impl;
} ccc_o_map_elem;

typedef struct
{
    struct ccc_tree_entry impl;
} ccc_o_map_entry;

#define CCC_OM_INIT(struct_name, set_elem_field, key_elem_field, set_name,     \
                    alloc_fn, key_cmp, aux)                                    \
    CCC_TREE_INIT(struct_name, set_elem_field, key_elem_field, set_name,       \
                  alloc_fn, key_cmp, aux)

ccc_o_map_entry ccc_om_entry(ccc_ordered_map *s, void const *key);

void *ccc_om_or_insert(ccc_o_map_entry e, ccc_o_map_elem *elem);

ccc_o_map_entry ccc_om_and_modify(ccc_o_map_entry e, ccc_update_fn *fn);

void const *ccc_om_get(ccc_ordered_map *s, void const *key);

void *ccc_om_get_mut(ccc_ordered_map *s, void const *key);

ccc_o_map_entry ccc_om_and_modify_with(ccc_o_map_entry e, ccc_update_fn *fn,
                                       void *aux);

void const *ccc_om_unwrap(ccc_o_map_entry e);

void *ccc_om_unwrap_mut(ccc_o_map_entry e);

void *ccc_om_insert_entry(ccc_o_map_entry e, ccc_o_map_elem *elem);

ccc_o_map_entry ccc_om_remove_entry(ccc_o_map_entry e);

bool ccc_om_contains(ccc_ordered_map *, void const *key);

ccc_o_map_entry ccc_om_insert(ccc_ordered_map *, ccc_o_map_elem *out_handle);

void *ccc_om_remove(ccc_ordered_map *, ccc_o_map_elem *out_handle);

bool ccc_om_const_contains(ccc_ordered_map *, ccc_o_map_elem const *);

void *ccc_om_begin(ccc_ordered_map *);

void *ccc_om_rbegin(ccc_ordered_map *);

void *ccc_om_next(ccc_ordered_map *, ccc_o_map_elem const *);

void *ccc_om_rnext(ccc_ordered_map *, ccc_o_map_elem const *);

ccc_range ccc_om_equal_range(ccc_ordered_map *, void const *begin_key,
                             void const *end_key);

void *ccc_om_begin_range(ccc_range const *);

void *ccc_om_end_range(ccc_range const *);

ccc_rrange ccc_om_equal_rrange(ccc_ordered_map *, void const *rbegin_key,
                               void const *end_key);

void *ccc_om_begin_rrange(ccc_rrange const *);

void *ccc_om_end_rrange(ccc_rrange const *);

void *ccc_om_root(ccc_ordered_map const *);

void ccc_om_clear(ccc_ordered_map *, ccc_destructor_fn *destructor);

bool ccc_om_empty(ccc_ordered_map const *);

size_t ccc_om_size(ccc_ordered_map const *);

void ccc_om_print(ccc_ordered_map const *, ccc_o_map_elem const *,
                  ccc_print_fn *);

bool ccc_om_validate(ccc_ordered_map const *);

#endif /* CCC_ORDERED_MAP_H */
