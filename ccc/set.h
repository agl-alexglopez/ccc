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

#include "tree.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    ccc_tree t;
} ccc_set;

typedef struct
{
    ccc_node n;
} ccc_set_elem;

#define CCC_SET_INIT(struct_name, set_elem_field, set_name, cmp, aux)          \
    {                                                                          \
        .t = CCC_TREE_INIT(struct_name, set_elem_field, set_name, cmp, aux)    \
    }

void ccc_set_clear(ccc_set *, ccc_destructor_fn *destructor);

bool ccc_set_empty(ccc_set const *);

size_t ccc_set_size(ccc_set const *);

bool ccc_set_contains(ccc_set *, ccc_set_elem const *);

bool ccc_set_insert(ccc_set *, ccc_set_elem *);

void *ccc_set_find(ccc_set *, ccc_set_elem const *);

void *ccc_set_erase(ccc_set *, ccc_set_elem *);

bool ccc_set_is_min(ccc_set *, ccc_set_elem const *);

bool ccc_set_is_max(ccc_set *, ccc_set_elem const *);

bool ccc_set_const_contains(ccc_set *, ccc_set_elem const *);

void *ccc_set_const_find(ccc_set *, ccc_set_elem const *);

void *ccc_set_begin(ccc_set *);

void *ccc_set_rbegin(ccc_set *);

void *ccc_set_next(ccc_set *, ccc_set_elem const *);

void *ccc_set_rnext(ccc_set *, ccc_set_elem const *);

ccc_range ccc_set_equal_range(ccc_set *, ccc_set_elem const *begin,
                              ccc_set_elem const *end);

void *ccc_set_begin_range(ccc_range const *);

void *ccc_set_end_range(ccc_range const *);

ccc_rrange ccc_set_equal_rrange(ccc_set *, ccc_set_elem const *rbegin,
                                ccc_set_elem const *end);

void *ccc_set_begin_rrange(ccc_rrange const *);

void *ccc_set_end_rrange(ccc_rrange const *);

void *ccc_set_root(ccc_set const *);

void ccc_set_print(ccc_set const *, ccc_set_elem const *, ccc_print_fn *);

#endif /* CCC_SET_H */
