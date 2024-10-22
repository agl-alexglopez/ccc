#ifndef CCC_PRIORITY_QUEUE_H
#define CCC_PRIORITY_QUEUE_H

#include "impl_priority_queue.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

/** The structure used to manage the data in a priority queue. This priority
queue offers pointer stability and an O(1) push and amortized O(1) (technically
amortized o(lgN) by increasing the cost of the next pop, but the operation
iteself is O(1)) or O(lgN) increase or decrease key operation, depending on the
initialization of the queue. The cost of a pop operation is O(lgN). */
typedef struct ccc_pq_ ccc_priority_queue;

/** The embedded struct type for operation of the priority queue. */
typedef struct ccc_pq_elem_ ccc_pq_elem;

/* Given the desired total order of the priority queue, the comparison function,
   and any auxilliarry data needed for comparison, initialize an empty priority
   queue on the right hand side of it's declaration. This can be used at
   compile time or runtime. For example:
     ccc_priority_queue my_pq = ccc_pq_init(CCC_PQ_LES, my_cmp_fn, NULL);
   Such initialization must always occur or use of the priority queue is
   undefined. */
#define ccc_pq_init(struct_name, pq_elem_field, pq_order, alloc_fn, cmp_fn,    \
                    aux_data)                                                  \
    ccc_impl_pq_init(struct_name, pq_elem_field, pq_order, alloc_fn, cmp_fn,   \
                     aux_data)

/* Obtain a reference to the front of the priority queue. This will be a min
   or max depending on the initialization of the priority queue. O(1). */
void *ccc_pq_front(ccc_priority_queue const *);

/* Adds an element to the priority queue in correct total order. O(1). */
ccc_result ccc_pq_push(ccc_priority_queue *, ccc_pq_elem *);

/* Pops the front element from the priority queue. O(lgN). */
ccc_result ccc_pq_pop(ccc_priority_queue *);

/* Extract the specified element from the priority queue without freeing memory.
   This need not be the front element. O(lgN). */
void *ccc_pq_extract(ccc_priority_queue *, ccc_pq_elem *e);

/* Erase the specified element from the priority queue, freeing the user type
   wrapping the element if the container has permission to allocate; this
   invalidates the reference to elem. If the container does not have allocation
   permission it is the user's responsibility to manage the memory wrapping
   element. This need not be the front element. O(lgN). */
ccc_result ccc_pq_erase(ccc_priority_queue *, ccc_pq_elem *e);

/* Returns true if the priority queue is empty false if not. */
bool ccc_pq_is_empty(ccc_priority_queue const *);

/* Returns the size of the priority queue. */
size_t ccc_pq_size(ccc_priority_queue const *);

/* Update the value of a priority queue element if the new value is not
   known to be less than or greater than the old value. This operation
   may incur uneccessary overhead if the user can deduce if an increase
   or decrease is occuring. See the increase and decrease operations. O(1)
   best case, O(lgN) worst case. */
bool ccc_pq_update(ccc_priority_queue *, ccc_pq_elem *, ccc_update_fn *,
                   void *);

/* Optimal update technique if the priority queue has been initialized as
   a max queue and the new value is known to be greater than the old value.
   If this is a max heap O(1), otherwise O(lgN). */
bool ccc_pq_increase(ccc_priority_queue *, ccc_pq_elem *, ccc_update_fn *,
                     void *);

/* Optimal update technique if the priority queue has been initialized as
   a min queue and the new value is known to be less than the old value.
   If this is a min heap O(1), otherwise O(lgN). */
bool ccc_pq_decrease(ccc_priority_queue *, ccc_pq_elem *, ccc_update_fn *,
                     void *);

/* Return the order used to initialize the heap. */
ccc_threeway_cmp ccc_pq_order(ccc_priority_queue const *);

/* Calls the user provided destructor on each element in the priority queue.
   It is safe to free the struct if it has been heap allocated as elements
   are popped from the priority queue before the function is called. O(NlgN). */
ccc_result ccc_pq_clear(ccc_priority_queue *, ccc_destructor_fn *);

/* Internal validation function for the state of the heap. This should be of
   little interest to the user. */
bool ccc_pq_validate(ccc_priority_queue const *);

#ifndef PRIORITY_QUEUE_USING_NAMESPACE_CCC
typedef ccc_pq_elem pq_elem;
typedef ccc_priority_queue priority_queue;
#    define pq_init(args...) ccc_pq_init(args)
#    define pq_front(args...) ccc_pq_front(args)
#    define pq_push(args...) ccc_pq_push(args)
#    define pq_pop(args...) ccc_pq_pop(args)
#    define pq_extract(args...) ccc_pq_extract(args)
#    define pq_is_empty(args...) ccc_pq_is_empty(args)
#    define pq_size(args...) ccc_pq_size(args)
#    define pq_update(args...) ccc_pq_update(args)
#    define pq_increase(args...) ccc_pq_increase(args)
#    define pq_decrease(args...) ccc_pq_decrease(args)
#    define pq_order(args...) ccc_pq_order(args)
#    define pq_clear(args...) ccc_pq_clear(args)
#    define pq_validate(args...) ccc_pq_validate(args)
#endif /* PRIORITY_QUEUE_USING_NAMESPACE_CCC */

#endif /* CCC_PRIORITY_QUEUE_H */
