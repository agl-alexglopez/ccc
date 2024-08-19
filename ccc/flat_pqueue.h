#ifndef CCC_FLAT_PQUEUE_H
#define CCC_FLAT_PQUEUE_H

/* Privately linked implementation. */
#include "impl_flat_pqueue.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum
{
    CCC_FPQ_LES = CCC_IMPL_FPQ_LES,
    CCC_FPQ_EQL = CCC_IMPL_FPQ_EQL,
    CCC_FPQ_GRT = CCC_IMPL_FPQ_GRT,
} ccc_fpq_threeway_cmp;

typedef enum
{
    CCC_FPQ_OK = CCC_IMPL_FPQ_OK,
    CCC_FPQ_FULL = CCC_IMPL_FPQ_FULL,
    CCC_FPQ_ERR = CCC_IMPL_FPQ_ERR,
} ccc_fpq_result;

typedef bool ccc_fpq_cmp_fn(void const *, void const *, void *);

typedef void ccc_fpq_destructor_fn(void *);

typedef void ccc_fpq_update_fn(void *, void *);

typedef void ccc_fpq_print_fn(void const *);

/* It does not make sense for a flat pqueue to be associated with any other
   buffer, comparison function, ordering, or auxiliarry data once it has been
   initialized. The CCC_FPQ_INIT macro allows for initialization at compile
   time for static/global data, or runtime for dynamic data so initialization
   via construction of const fields is always possible. There is no reason to
   access the fields directly or modify them. */
typedef struct
{
    struct ccc_impl_flat_pqueue impl;
} ccc_flat_pqueue;

#define CCC_FPQ_INIT(mem_buf, struct_name, fpq_elem_field, cmp_order, cmp_fn,  \
                     aux_data)                                                 \
    CCC_IMPL_FPQ_INIT(mem_buf, struct_name, fpq_elem_field, cmp_order, cmp_fn, \
                      aux_data)

/* Given an initialized flat priority queue, a struct type, and its
   initializer, attempts to write an r-value of one's struct type into the
   backing buffer directly, returning the ccc_fpq_result according to the
   underlying buffer's allocation policy. If allocation fails because
   the underlying buffer does not define a reallocation policy and is full,
   CCC_FPQ_FULL is returned, otherwise CCC_FPQ_OK is returned. If the provided
   struct type does not match the size of the elements stored in the buffer
   CCC_FPQ_ERR is returned. Use as follows:

   struct val
   {
       int v;
       int id;
   };

   Various forms of designated initializers:

   ccc_fpq_result const res = CCC_FPQ_EMPLACE(&fpq, struct val, {.v = 10});
   ccc_fpq_result const res
       = CCC_FPQ_EMPLACE(&fpq, struct val, {.v = rand_value(), .id = 0});
   ccc_fpq_result const res
       = CCC_FPQ_EMPLACE(&fpq, struct val, {.v = 10, .id = 0});

   Older C notation requires all fields be specified on some compilers:

   ccc_fpq_result const res = CCC_FPQ_EMPLACE(&fpq, struct val, {10, 0});

   This macro avoids an additional copy if the struct values are constructed
   by hand or from input of other functions, requiring no intermediate storage.
   If generating any values within the struct occurs via expensive function
   calls or calls with side effects, note that such functions do not execute
   if allocation fails due to a full buffer and no reallocation policy. */
#define CCC_FPQ_EMPLACE(fpq, struct_name, struct_initializer...)               \
    (ccc_fpq_result)(                                                          \
        CCC_IMPL_FPQ_EMPLACE((fpq), struct_name, struct_initializer))

ccc_fpq_result ccc_fpq_realloc(ccc_flat_pqueue *, size_t new_capacity,
                               ccc_buf_realloc_fn *);
ccc_fpq_result ccc_fpq_push(ccc_flat_pqueue *, void const *);
void const *ccc_fpq_front(ccc_flat_pqueue const *);
void *ccc_fpq_pop(ccc_flat_pqueue *);
void *ccc_fpq_erase(ccc_flat_pqueue *, void *);
bool ccc_fpq_update(ccc_flat_pqueue *, void *, ccc_fpq_update_fn *, void *);

void ccc_fpq_clear(ccc_flat_pqueue *, ccc_fpq_destructor_fn *);
bool ccc_fpq_empty(ccc_flat_pqueue const *);
size_t ccc_fpq_size(ccc_flat_pqueue const *);
bool ccc_fpq_validate(ccc_flat_pqueue const *);
ccc_fpq_threeway_cmp ccc_fpq_order(ccc_flat_pqueue const *);

void ccc_fpq_print(ccc_flat_pqueue const *, size_t, ccc_fpq_print_fn *);

#endif /* CCC_FLAT_PQUEUE_H */
