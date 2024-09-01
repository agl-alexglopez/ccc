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
#ifndef CCC_MAP_H
#define CCC_MAP_H

#include "impl_tree.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    ccc_tree impl;
} ccc_map;

typedef struct
{
    ccc_node impl;
} ccc_mp_elem;

typedef struct
{
    struct ccc_tree_entry impl;
} ccc_mp_entry;

#define CCC_M_INIT(struct_name, set_elem_field, key_elem_field, set_name,      \
                   realloc_fn, key_cmp, aux)                                   \
    CCC_TREE_INIT(struct_name, set_elem_field, key_elem_field, set_name,       \
                  realloc_fn, key_cmp, aux)

ccc_mp_entry ccc_m_entry(ccc_map *s, void const *key);

void *ccc_m_or_insert(ccc_mp_entry e, ccc_mp_elem *elem);

ccc_mp_entry ccc_m_and_modify(ccc_mp_entry e, ccc_update_fn *fn);

void const *ccc_m_get(ccc_map *s, void const *key);

void *ccc_m_get_mut(ccc_map *s, void const *key);

ccc_mp_entry ccc_m_and_modify_with(ccc_mp_entry e, ccc_update_fn *fn,
                                   void *aux);

void const *ccc_m_unwrap(ccc_mp_entry e);

void *ccc_m_unwrap_mut(ccc_mp_entry e);

void *ccc_m_insert_entry(ccc_mp_entry e, ccc_mp_elem *elem);

ccc_mp_entry ccc_m_remove_entry(ccc_mp_entry e);

bool ccc_m_contains(ccc_map *, void const *key);

ccc_mp_entry ccc_m_insert(ccc_map *, ccc_mp_elem *out_handle);

void *ccc_m_remove(ccc_map *, ccc_mp_elem *out_handle);

bool ccc_m_const_contains(ccc_map *, ccc_mp_elem const *);

void *ccc_m_begin(ccc_map *);

void *ccc_m_rbegin(ccc_map *);

void *ccc_m_next(ccc_map *, ccc_mp_elem const *);

void *ccc_m_rnext(ccc_map *, ccc_mp_elem const *);

ccc_range ccc_m_equal_range(ccc_map *, void const *begin_key,
                            void const *end_key);

void *ccc_m_begin_range(ccc_range const *);

void *ccc_m_end_range(ccc_range const *);

ccc_rrange ccc_m_equal_rrange(ccc_map *, void const *rbegin_key,
                              void const *end_key);

void *ccc_m_begin_rrange(ccc_rrange const *);

void *ccc_m_end_rrange(ccc_rrange const *);

void *ccc_m_root(ccc_map const *);

void ccc_m_clear(ccc_map *, ccc_destructor_fn *destructor);

bool ccc_m_empty(ccc_map const *);

size_t ccc_m_size(ccc_map const *);

void ccc_m_print(ccc_map const *, ccc_mp_elem const *, ccc_print_fn *);

bool ccc_m_validate(ccc_map const *);

#endif /* CCC_MAP_H */
