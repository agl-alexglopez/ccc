/* Author: Alexander G. Lopez
   --------------------------
   This is the Double Ended Priority Queue interface implemented via Splay
   Tree. In this case we modify a Splay Tree to allow for
   a Double Ended Priority Queue (aka a sorted Multi-Set). See the
   normal set interface as well. While a Red-Black Tree
   would be the more optimal data structure to support
   a DEPQ the underlying implementation of a Splay Tree
   offers some interesting tradeoffs for systems programmers.
   They are working sets that keep frequently (Least
   Recently Used) elements close to the root even if their
   runtime is amortized O(lgN). With the right use cases we
   can frequently benefit from O(1) operations. */
#ifndef CCC_DEPQUEUE_H
#define CCC_DEPQUEUE_H

#include "tree.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    ccc_node n;
} ccc_depq_elem;

typedef struct
{
    ccc_tree t;
} ccc_depqueue;

/* Initialize the depq on the left hand side with this right hand side
   initializer. Pass the left hand side depq by name to this macro along
   with the comparison function and any necessary auxilliary data. This may
   be used at compile time or runtime. It is undefined to use the depq if
   this has not been called. */
#define CCC_DEPQ_INIT(struct_name, depq_elem_field, depq_name, cmp, aux)       \
    {                                                                          \
        .t = CCC_TREE_INIT(struct_name, depq_elem_field, depq_name, cmp, aux)  \
    }

void ccc_depq_clear(ccc_depqueue *, ccc_destructor_fn *destructor);

bool ccc_depq_empty(ccc_depqueue const *);

size_t ccc_depq_size(ccc_depqueue *);

void ccc_depq_push(ccc_depqueue *, void *);

void *ccc_depq_pop_max(ccc_depqueue *);

void *ccc_depq_pop_min(ccc_depqueue *);

void *ccc_depq_max(ccc_depqueue *);

void *ccc_depq_min(ccc_depqueue *);

bool ccc_depq_is_max(ccc_depqueue *, void *);

bool ccc_depq_is_min(ccc_depqueue *, void *);

void const *ccc_depq_const_max(ccc_depqueue const *);

void const *ccc_depq_const_min(ccc_depqueue const *);

void *ccc_depq_erase(ccc_depqueue *, void *);

bool ccc_depq_update(ccc_depqueue *, void *, ccc_update_fn *, void *);

bool ccc_depq_contains(ccc_depqueue *, void *);

void *ccc_depq_begin(ccc_depqueue *);

void *ccc_depq_rbegin(ccc_depqueue *);

void *ccc_depq_next(ccc_depqueue *, void *);

void *ccc_depq_rnext(ccc_depqueue *, void *);

ccc_range ccc_depq_equal_range(ccc_depqueue *, void *begin, void *end);

void *ccc_depq_begin_range(ccc_range const *);

void *ccc_depq_end_range(ccc_range const *);

ccc_rrange ccc_depq_equal_rrange(ccc_depqueue *, void *rbegin, void *end);

void *ccc_depq_begin_rrange(ccc_rrange const *);

void *ccc_depq_end_rrange(ccc_rrange const *);

void *ccc_depq_root(ccc_depqueue const *);

void ccc_depq_print(ccc_depqueue const *, void const *, ccc_print_fn *);

#endif /* CCC_DEPQUEUE_H */
