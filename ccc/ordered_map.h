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
    struct ccc_om_ impl_;
} ccc_ordered_map;

typedef struct
{
    ccc_om_elem_ impl_;
} ccc_o_map_elem;

typedef struct
{
    struct ccc_om_entry_ impl_;
} ccc_o_map_entry;

#define CCC_OM_INIT(struct_name, set_elem_field, key_elem_field, set_name,     \
                    alloc_fn, key_cmp, aux)                                    \
    CCC_TREE_INIT(struct_name, set_elem_field, key_elem_field, set_name,       \
                  alloc_fn, key_cmp, aux)

#define CCC_OM_ENTRY(ordered_map_ptr, key...)                                  \
    &(ccc_o_map_entry)                                                         \
    {                                                                          \
        CCC_IMPL_TREE_ENTRY(ordered_map_ptr, key)                              \
    }

#define CCC_OM_GET(ordered_map_ptr, key...)                                    \
    CCC_IMPL_TREE_GET(ordered_map_ptr, key)

#define CCC_OM_GET_MUT(ordered_map_ptr, key...)                                \
    CCC_IMPL_TREE_GET_MUT(ordered_map_ptr, key)

#define CCC_OM_AND_MODIFY(ordered_map_entry_ptr, mod_fn)                       \
    &(ccc_o_ordered_map_entry_ptr)                                             \
    {                                                                          \
        CCC_IMPL_TREE_AND_MODIFY(ordered_map_entry_ptr, mod_fn)                \
    }

#define CCC_OM_AND_MODIFY_W(ordered_map_entry_ptr, mod_fn, aux_data)           \
    &(ccc_o_ordered_map_entry_ptr)                                             \
    {                                                                          \
        CCC_IMPL_TREE_AND_MODIFY_WITH(ordered_map_entry_ptr, mod_fn, aux_data) \
    }

#define CCC_OM_INSERT_ENTRY(ordered_map_entry_ptr, key_value...)               \
    CCC_IMPL_TREE_INSERT_ENTRY(ordered_map_entry_ptr, key_value)

#define CCC_OM_OR_INSERT(ordered_map_entry_ptr, key_value...)                  \
    CCC_IMPL_TREE_OR_INSERT(ordered_map_entry_ptr, key_value)

bool ccc_om_contains(ccc_ordered_map *, void const *key);

void const *ccc_om_get(ccc_ordered_map *s, void const *key);

void *ccc_om_get_mut(ccc_ordered_map *s, void const *key);

/*===========================   Entry API   =================================*/

/* Retain access to old values in the map. See types.h for ccc_entry. */

#define ccc_om_insert_lv(ordered_map_ptr, out_handle_ptr)                      \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_om_insert((ordered_map_ptr), (out_handle_ptr)).impl_               \
    }

#define ccc_om_remove_lv(ordered_map_ptr, out_handle_ptr)                      \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_om_remove((ordered_map_ptr), (out_handle_ptr)).impl_               \
    }

#define ccc_om_remove_entry_lv(ordered_map_entry_ptr)                          \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_om_remove_entry((ordered_map_entry_ptr)).impl_                     \
    }

ccc_entry ccc_om_insert(ccc_ordered_map *, ccc_o_map_elem *out_handle);

ccc_entry ccc_om_remove(ccc_ordered_map *, ccc_o_map_elem *out_handle);

ccc_entry ccc_om_remove_entry(ccc_o_map_entry *e);

/* Standard Entry API. */

#define ccc_om_entry_lv(ordered_map_ptr, key_ptr)                              \
    &(ccc_o_map_entry)                                                         \
    {                                                                          \
        ccc_om_entry((ordered_map_ptr), (key_ptr)).impl_                       \
    }

#define ccc_om_and_modify_lv(ordered_map_ptr, mod_fn)                          \
    &(ccc_o_map_entry)                                                         \
    {                                                                          \
        ccc_om_and_modify((ordered_map_ptr), (mod_fn)).impl_                   \
    }

#define ccc_om_and_modify_with_lv(ordered_map_ptr, mod_fn, aux_data)           \
    &(ccc_o_map_entry)                                                         \
    {                                                                          \
        ccc_om_and_modify((ordered_map_ptr), (mod_fn), (aux_data)).impl_       \
    }

ccc_o_map_entry ccc_om_entry(ccc_ordered_map *s, void const *key);

ccc_o_map_entry ccc_om_and_modify(ccc_o_map_entry const *e, ccc_update_fn *fn);

ccc_o_map_entry ccc_om_and_modify_with(ccc_o_map_entry const *e,
                                       ccc_update_fn *fn, void *aux);

void *ccc_om_or_insert(ccc_o_map_entry const *e, ccc_o_map_elem *elem);

void *ccc_om_insert_entry(ccc_o_map_entry const *e, ccc_o_map_elem *elem);

void const *ccc_om_unwrap(ccc_o_map_entry const *e);

void *ccc_om_unwrap_mut(ccc_o_map_entry const *e);

bool ccc_om_insert_error(ccc_o_map_entry const *e);

/*===========================   Entry API   =================================*/

void *ccc_om_begin(ccc_ordered_map const *);

void *ccc_om_rbegin(ccc_ordered_map const *);

void *ccc_om_end(ccc_ordered_map const *);

void *ccc_om_rend(ccc_ordered_map const *);

void *ccc_om_next(ccc_ordered_map const *, ccc_o_map_elem const *);

void *ccc_om_rnext(ccc_ordered_map const *, ccc_o_map_elem const *);

ccc_range ccc_om_equal_range(ccc_ordered_map *, void const *begin_key,
                             void const *end_key);

ccc_rrange ccc_om_equal_rrange(ccc_ordered_map *, void const *rbegin_key,
                               void const *end_key);

void *ccc_om_root(ccc_ordered_map const *);

void ccc_om_clear(ccc_ordered_map *, ccc_destructor_fn *destructor);

bool ccc_om_empty(ccc_ordered_map const *);

size_t ccc_om_size(ccc_ordered_map const *);

void ccc_om_print(ccc_ordered_map const *, ccc_o_map_elem const *,
                  ccc_print_fn *);

bool ccc_om_validate(ccc_ordered_map const *);

#endif /* CCC_ORDERED_MAP_H */
