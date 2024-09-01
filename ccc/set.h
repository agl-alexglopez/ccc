/* Author: Alexander G. Lopez
   --------------------------
   This is the Set interface for the Splay Tree
   Set. In this case we modify a Splay Tree to allow for
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
#ifndef CCC_SET_H
#define CCC_SET_H

#include "impl_tree.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    ccc_tree impl;
} ccc_set;

typedef struct
{
    ccc_node impl;
} ccc_set_elem;

typedef struct
{
    struct ccc_tree_entry impl;
} ccc_set_entry;

#define CCC_SET_INIT(struct_name, set_elem_field, key_elem_field, set_name,    \
                     realloc_fn, key_cmp, aux)                                 \
    CCC_TREE_INIT(struct_name, set_elem_field, key_elem_field, set_name,       \
                  realloc_fn, key_cmp, aux)

ccc_set_entry ccc_s_entry(ccc_set *s, void const *key);

void *ccc_s_or_insert(ccc_set_entry e, ccc_set_elem *elem);

ccc_set_entry ccc_s_and_modify(ccc_set_entry e, ccc_update_fn *fn);

void const *ccc_s_get(ccc_set *s, void const *key);

void *ccc_s_get_mut(ccc_set *s, void const *key);

ccc_set_entry ccc_s_and_modify_with(ccc_set_entry e, ccc_update_fn *fn,
                                    void *aux);

void const *ccc_s_unwrap(ccc_set_entry e);

void *ccc_s_unwrap_mut(ccc_set_entry e);

void *ccc_s_insert_entry(ccc_set_entry e, ccc_set_elem *elem);

ccc_set_entry ccc_s_remove_entry(ccc_set_entry e);

bool ccc_s_contains(ccc_set *, void const *key);

ccc_set_entry ccc_s_insert(ccc_set *, ccc_set_elem *out_handle);

void *ccc_s_remove(ccc_set *, ccc_set_elem *out_handle);

bool ccc_s_const_contains(ccc_set *, ccc_set_elem const *);

void *ccc_s_begin(ccc_set *);

void *ccc_s_rbegin(ccc_set *);

void *ccc_s_next(ccc_set *, ccc_set_elem const *);

void *ccc_s_rnext(ccc_set *, ccc_set_elem const *);

ccc_range ccc_s_equal_range(ccc_set *, void const *begin_key,
                            void const *end_key);

void *ccc_s_begin_range(ccc_range const *);

void *ccc_s_end_range(ccc_range const *);

ccc_rrange ccc_s_equal_rrange(ccc_set *, void const *rbegin_key,
                              void const *end_key);

void *ccc_s_begin_rrange(ccc_rrange const *);

void *ccc_s_end_rrange(ccc_rrange const *);

void *ccc_s_root(ccc_set const *);

void ccc_s_clear(ccc_set *, ccc_destructor_fn *destructor);

bool ccc_s_empty(ccc_set const *);

size_t ccc_s_size(ccc_set const *);

void ccc_s_print(ccc_set const *, ccc_set_elem const *, ccc_print_fn *);

bool ccc_s_validate(ccc_set const *);

#endif /* CCC_SET_H */
