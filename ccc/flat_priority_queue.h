#ifndef CCC_FLAT_PRIORITY_QUEUE_H
#define CCC_FLAT_PRIORITY_QUEUE_H

#include "impl_flat_priority_queue.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

/* It does not make sense for a flat pqueue to be associated with any other
   buffer, comparison function, ordering, or auxiliarry data once it has been
   initialized. The ccc_fpq_init macro allows for initialization at compile
   time for static/global data, or runtime for dynamic data so initialization
   via construction of const fields is always possible. There is no reason to
   access the fields directly or modify them. */
typedef struct ccc_fpq_ ccc_flat_priority_queue;

#define ccc_fpq_init(mem_ptr, capacity, cmp_order, alloc_fn, cmp_fn, aux_data) \
    ccc_impl_fpq_init(mem_ptr, capacity, cmp_order, alloc_fn, cmp_fn, aux_data)

/** @brief Initializes a heap with the provided memory of size and capacity
provided in O(n) time. Elements are sorted by their values as provided. Size
must be less than or equal to (capacity - 1). Use on the right hand side of
of the assignment for the current heap, same as normal initialization. However,
this initializer must be used at runtime, not compile time. */
#define ccc_fpq_heapify_init(mem_ptr, capacity, size, cmp_order, alloc_fn,     \
                             cmp_fn, aux_data)                                 \
    ccc_impl_fpq_heapify_init(mem_ptr, capacity, size, cmp_order, alloc_fn,    \
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

   struct val const *res = ccc_fpq_emplace(&fpq, (struct val){.v = 10});
   struct val const *res
       = ccc_fpq_emplace(&fpq, (struct val){.v = rand_value(), .id = 0});
   struct val const *res
       = ccc_fpq_emplace(&fpq, (struct val){.v = 10, .id = 0});

   Older C notation requires all fields be specified on some compilers:

   struct val const *res = ccc_fpq_emplace(&fpq, (struct val){10, 0});

   This macro avoids an additional copy if the struct values are constructed
   by hand or from input of other functions, requiring no intermediate storage.
   If generating any values within the struct occurs via expensive function
   calls or calls with side effects, note that such functions do not execute
   if allocation fails due to a full buffer and no reallocation policy. */
#define ccc_fpq_emplace(fpq, val_initializer...)                               \
    ccc_impl_fpq_emplace(fpq, val_initializer)

/** @brief Builds a heap in O(n) time from the input data. If elements were
previously occupying the heap, they are overwritten and only elements in the
input array are considered part of the heap. */
ccc_result ccc_fpq_heapify(ccc_flat_priority_queue *, void *input_array,
                           size_t input_n, size_t input_elem_size);

ccc_result ccc_fpq_realloc(ccc_flat_priority_queue *, size_t new_capacity,
                           ccc_alloc_fn *);
void *ccc_fpq_push(ccc_flat_priority_queue *, void const *);
void *ccc_fpq_front(ccc_flat_priority_queue const *);
ccc_result ccc_fpq_pop(ccc_flat_priority_queue *);
void *ccc_fpq_extract(ccc_flat_priority_queue *, void *);
bool ccc_fpq_update(ccc_flat_priority_queue *, void *, ccc_update_fn *, void *);
bool ccc_fpq_increase(ccc_flat_priority_queue *, void *, ccc_update_fn *,
                      void *);
bool ccc_fpq_decrease(ccc_flat_priority_queue *, void *, ccc_update_fn *,
                      void *);

ccc_result ccc_fpq_clear(ccc_flat_priority_queue *, ccc_destructor_fn *);
ccc_result ccc_fpq_clear_and_free(ccc_flat_priority_queue *,
                                  ccc_destructor_fn *);
bool ccc_fpq_is_empty(ccc_flat_priority_queue const *);
size_t ccc_fpq_size(ccc_flat_priority_queue const *);
bool ccc_fpq_validate(ccc_flat_priority_queue const *);
ccc_threeway_cmp ccc_fpq_order(ccc_flat_priority_queue const *);

#ifdef FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
typedef ccc_flat_priority_queue flat_priority_queue;
#    define fpq_init(args...) ccc_fpq_init(args)
#    define fpq_emplace(args...) ccc_fpq_emplace(args)
#    define fpq_realloc(args...) ccc_fpq_realloc(args)
#    define fpq_push(args...) ccc_fpq_push(args)
#    define fpq_front(args...) ccc_fpq_front(args)
#    define fpq_pop(args...) ccc_fpq_pop(args)
#    define fpq_extract(args...) ccc_fpq_extract(args)
#    define fpq_update(args...) ccc_fpq_update(args)
#    define fpq_increase(args...) ccc_fpq_increase(args)
#    define fpq_decrease(args...) ccc_fpq_decrease(args)
#    define fpq_clear(args...) ccc_fpq_clear(args)
#    define fpq_is_empty(args...) ccc_fpq_is_empty(args)
#    define fpq_size(args...) ccc_fpq_size(args)
#    define fpq_validate(args...) ccc_fpq_validate(args)
#    define fpq_order(args...) ccc_fpq_order(args)
#endif /* FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC */

#endif /* CCC_FLAT_PRIORITY_QUEUE_H */
