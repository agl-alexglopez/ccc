#ifndef CCC_PRIORITY_QUEUE_H
#define CCC_PRIORITY_QUEUE_H

#include "impl_priority_queue.h"
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
typedef struct ccc_pq_elem_ ccc_pq_elem;

/* The structure used to manage the data in a priority queue. Stack allocation
   is recommended for easy cleanup and speed. However, this structure may be
   placed anywhere that is convenient for the user. Consider the fields
   private. This structure can be initialized upon declaration witht the
   provided initialization macro. */
typedef struct ccc_pq_ ccc_priority_queue;

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
void ccc_pq_push(ccc_priority_queue *, ccc_pq_elem *);

/* Pops the front element from the priority queue. O(lgN). */
void ccc_pq_pop(ccc_priority_queue *);

/* Erase the specified element from the priority queue. This need not be
   the front element. O(lgN). */
void ccc_pq_erase(ccc_priority_queue *, ccc_pq_elem *);

/* Returns true if the priority queue is empty false if not. */
bool ccc_pq_empty(ccc_priority_queue const *);

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
void ccc_pq_clear(ccc_priority_queue *, ccc_destructor_fn *);

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
#    define pq_erase(args...) ccc_pq_erase(args)
#    define pq_empty(args...) ccc_pq_empty(args)
#    define pq_size(args...) ccc_pq_size(args)
#    define pq_update(args...) ccc_pq_update(args)
#    define pq_increase(args...) ccc_pq_increase(args)
#    define pq_decrease(args...) ccc_pq_decrease(args)
#    define pq_order(args...) ccc_pq_order(args)
#    define pq_clear(args...) ccc_pq_clear(args)
#    define pq_validate(args...) ccc_pq_validate(args)
#endif /* PRIORITY_QUEUE_USING_NAMESPACE_CCC */

#endif /* CCC_PRIORITY_QUEUE_H */
