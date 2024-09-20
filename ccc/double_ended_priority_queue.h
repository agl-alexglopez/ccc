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
#ifndef CCC_DOUBLE_ENDED_PRIORITY_QUEUE_H
#define CCC_DOUBLE_ENDED_PRIORITY_QUEUE_H

#include "impl_double_ended_priority_queue.h"
#include "impl_tree.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

typedef union
{
    struct ccc_node_ impl_;
} ccc_depq_elem;

typedef union
{
    struct ccc_tree_ impl_;
} ccc_double_ended_priority_queue;

/* Initialize the depq on the left hand side with this right hand side
   initializer. Pass the left hand side depq by name to this macro along
   with the comparison function and any necessary auxilliary data. This may
   be used at compile time or runtime. It is undefined to use the depq if
   this has not been called. */
#define ccc_depq_init(struct_name, depq_elem_field, key_field, depq_name,      \
                      alloc_fn, key_cmp_fn, aux)                               \
    ccc_impl_depq_init(struct_name, depq_elem_field, key_field, depq_name,     \
                       alloc_fn, key_cmp_fn, aux)

void ccc_depq_clear(ccc_double_ended_priority_queue *,
                    ccc_destructor_fn *destructor);

bool ccc_depq_empty(ccc_double_ended_priority_queue const *);

size_t ccc_depq_size(ccc_double_ended_priority_queue const *);

ccc_result ccc_depq_push(ccc_double_ended_priority_queue *, ccc_depq_elem *);

void ccc_depq_pop_max(ccc_double_ended_priority_queue *);

void ccc_depq_pop_min(ccc_double_ended_priority_queue *);

void *ccc_depq_max(ccc_double_ended_priority_queue *);

void *ccc_depq_min(ccc_double_ended_priority_queue *);

bool ccc_depq_is_max(ccc_double_ended_priority_queue *, ccc_depq_elem const *);

bool ccc_depq_is_min(ccc_double_ended_priority_queue *, ccc_depq_elem const *);

void *ccc_depq_erase(ccc_double_ended_priority_queue *, ccc_depq_elem *);

bool ccc_depq_update(ccc_double_ended_priority_queue *, ccc_depq_elem *,
                     ccc_update_fn *, void *);

bool ccc_depq_increase(ccc_double_ended_priority_queue *, ccc_depq_elem *,
                       ccc_update_fn *, void *);

bool ccc_depq_decrease(ccc_double_ended_priority_queue *, ccc_depq_elem *,
                       ccc_update_fn *, void *);

bool ccc_depq_contains(ccc_double_ended_priority_queue *, void const *key);

void *ccc_depq_begin(ccc_double_ended_priority_queue const *);

void *ccc_depq_rbegin(ccc_double_ended_priority_queue const *);

void *ccc_depq_next(ccc_double_ended_priority_queue const *,
                    ccc_depq_elem const *);

void *ccc_depq_rnext(ccc_double_ended_priority_queue const *,
                     ccc_depq_elem const *);

void *ccc_depq_end(ccc_double_ended_priority_queue const *);

void *ccc_depq_rend(ccc_double_ended_priority_queue const *);

ccc_range ccc_depq_equal_range(ccc_double_ended_priority_queue *,
                               void const *begin_key, void const *end_key);

ccc_rrange ccc_depq_equal_rrange(ccc_double_ended_priority_queue *,
                                 void const *rbegin_key, void const *end_key);

void *ccc_depq_root(ccc_double_ended_priority_queue const *);

void ccc_depq_print(ccc_double_ended_priority_queue const *,
                    ccc_depq_elem const *, ccc_print_fn *);

bool ccc_depq_validate(ccc_double_ended_priority_queue const *);

#endif /* CCC_DOUBLE_ENDED_PRIORITY_QUEUE_H */
