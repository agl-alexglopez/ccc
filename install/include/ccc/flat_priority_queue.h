#ifndef CCC_FLAT_PRIORITY_QUEUE_H
#define CCC_FLAT_PRIORITY_QUEUE_H

/* Privately linked implementation. */
#include "impl_flat_priority_queue.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

/* It does not make sense for a flat pqueue to be associated with any other
   buffer, comparison function, ordering, or auxiliarry data once it has been
   initialized. The CCC_FPQ_INIT macro allows for initialization at compile
   time for static/global data, or runtime for dynamic data so initialization
   via construction of const fields is always possible. There is no reason to
   access the fields directly or modify them. */
typedef struct
{
    struct ccc_impl_flat_priority_queue impl;
} ccc_flat_priority_queue;

#define CCC_FPQ_INIT(mem_ptr, capacity, type_name, cmp_order, alloc_fn,        \
                     cmp_fn, aux_data)                                         \
    CCC_IMPL_FPQ_INIT(mem_ptr, capacity, type_name, cmp_order, alloc_fn,       \
                      cmp_fn, aux_data)

/* Given an initialized flat priority queue, a struct type, and its
   initializer, attempts to write an r-value of one's struct type into the
   backing buffer directly, returning a pointer to the element in storage.
   If a memory permission error occurs NULL is returned. Use as follows:

   struct val
   {
       int v;
       int id;
   };

   Various forms of designated initializers:

   struct val const *res = CCC_FPQ_EMPLACE(&fpq, (struct val){.v = 10});
   struct val const *res
       = CCC_FPQ_EMPLACE(&fpq, (struct val){.v = rand_value(), .id = 0});
   struct val const *res
       = CCC_FPQ_EMPLACE(&fpq, (struct val){.v = 10, .id = 0});

   Older C notation requires all fields be specified on some compilers:

   struct val const *res = CCC_FPQ_EMPLACE(&fpq, (struct val){10, 0});

   This macro avoids an additional copy if the struct values are constructed
   by hand or from input of other functions, requiring no intermediate storage.
   If generating any values within the struct occurs via expensive function
   calls or calls with side effects, note that such functions do not execute
   if allocation fails due to a full buffer and no reallocation policy. */
#define FPQ_EMPLACE(fpq, struct_initializer...)                                \
    CCC_IMPL_FPQ_EMPLACE(fpq, struct_initializer)

ccc_result ccc_fpq_realloc(ccc_flat_priority_queue *, size_t new_capacity,
                           ccc_alloc_fn *);
void *ccc_fpq_push(ccc_flat_priority_queue *, void const *);
void const *ccc_fpq_front(ccc_flat_priority_queue const *);
void *ccc_fpq_pop(ccc_flat_priority_queue *);
void *ccc_fpq_erase(ccc_flat_priority_queue *, void *);
bool ccc_fpq_update(ccc_flat_priority_queue *, void *, ccc_update_fn *, void *);

void ccc_fpq_clear(ccc_flat_priority_queue *, ccc_destructor_fn *);
bool ccc_fpq_empty(ccc_flat_priority_queue const *);
size_t ccc_fpq_size(ccc_flat_priority_queue const *);
bool ccc_fpq_validate(ccc_flat_priority_queue const *);
ccc_threeway_cmp ccc_fpq_order(ccc_flat_priority_queue const *);

void ccc_fpq_print(ccc_flat_priority_queue const *, size_t, ccc_print_fn *);

#endif /* CCC_FLAT_PRIORITY_QUEUE_H */
