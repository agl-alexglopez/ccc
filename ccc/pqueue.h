#ifndef CCC_PQUEUE_H
#define CCC_PQUEUE_H

#include "types.h"

#include <stdbool.h>
#include <stddef.h>

/* The embedded struct type for operation of the priority queue. The priority
   queue does not use the heap so it is the users responsibility to decide
   where elements are allocated in memory. For example:
       struct val
       {
           int val;
           ccc_pq_elem elem;
       };
   Do not access the fields of the struct directly. */
typedef struct ccc_pq_elem
{
    struct ccc_pq_elem *left_child;
    struct ccc_pq_elem *next_sibling;
    struct ccc_pq_elem *prev_sibling;
    struct ccc_pq_elem *parent;
} ccc_pq_elem;

/* The structure used to manage the data in a priority queue. Stack allocation
   is recommended for easy cleanup and speed. However, this structure may be
   placed anywhere that is convenient for the user. Consider the fields
   private. This structure can be initialized upon declaration witht the
   provided initialization macro. */
typedef struct
{
    ccc_pq_elem *root;
    size_t sz;
    size_t pq_elem_offset;
    ccc_cmp_fn *cmp;
    ccc_threeway_cmp order;
    void *aux;
} ccc_pqueue;

/* Given the desired total order of the priority queue, the comparison function,
   and any auxilliarry data needed for comparison, initialize an empty priority
   queue on the right hand side of it's declaration. This can be used at
   compile time or runtime. For example:
     ccc_pqueue my_pq = CCC_PQ_INIT(CCC_PQ_LES, my_cmp_fn, NULL);
   Such initialization must always occur or use of the priority queue is
   undefined. */
#define CCC_PQ_INIT(struct_name, pq_elem_field, pq_order, cmp_fn, aux_data)    \
    {                                                                          \
        .root = NULL, .sz = 0,                                                 \
        .pq_elem_offset = offsetof(struct_name, pq_elem_field),                \
        .cmp = (cmp_fn), .order = (pq_order), .aux = (aux_data)                \
    }

/* Obtain a reference to the front of the priority queue. This will be a min
   or max depending on the initialization of the priority queue. O(1). */
void const *ccc_pq_front(ccc_pqueue const *);

/* Adds an element to the priority queue in correct total order. O(1). */
void ccc_pq_push(ccc_pqueue *, void *);

/* Pops the front element from the priority queue. O(lgN). */
void *ccc_pq_pop(ccc_pqueue *);

/* Erase the specified element from the priority queue. This need not be
   the front element. O(lgN). */
void *ccc_pq_erase(ccc_pqueue *, void *);

/* Returns true if the priority queue is empty false if not. */
bool ccc_pq_empty(ccc_pqueue const *);

/* Returns the size of the priority queue. */
size_t ccc_pq_size(ccc_pqueue const *);

/* Update the value of a priority queue element if the new value is not
   known to be less than or greater than the old value. This operation
   may incur uneccessary overhead if the user can deduce if an increase
   or decrease is occuring. See the increase and decrease operations. O(1)
   best case, O(lgN) worst case. */
bool ccc_pq_update(ccc_pqueue *, void *, ccc_update_fn *, void *);

/* Optimal update technique if the priority queue has been initialized as
   a max queue and the new value is known to be greater than the old value.
   If this is a max heap O(1), otherwise O(lgN). */
bool ccc_pq_increase(ccc_pqueue *, void *, ccc_update_fn *, void *);

/* Optimal update technique if the priority queue has been initialized as
   a min queue and the new value is known to be less than the old value.
   If this is a min heap O(1), otherwise O(lgN). */
bool ccc_pq_decrease(ccc_pqueue *, void *, ccc_update_fn *, void *);

/* Return the order used to initialize the heap. */
ccc_threeway_cmp ccc_pq_order(ccc_pqueue const *);

/* Calls the user provided destructor on each element in the priority queue.
   It is safe to free the struct if it has been heap allocated as elements
   are popped from the priority queue before the function is called. O(NlgN). */
void ccc_pq_clear(ccc_pqueue *, ccc_destructor_fn *);

/* Internal validation function for the state of the heap. This should be of
   little interest to the user. */
bool ccc_pq_validate(ccc_pqueue const *);

#endif /* CCC_PQUEUE_H */
