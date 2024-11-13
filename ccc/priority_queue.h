/** @file
@brief The Priority Queue Interface

A priority queue offers simple, fast, pointer stable management of a prioriry
queue. Push is O(1). The cost to execute the increase key in a max heap and
decrease key in a min heap is O(1). However, due to the restructuring this
causes that increases the cost of later pops, the more accurate runtime is o(lg
N). The cost of a pop operation is O(lgN).

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define PRIORITY_QUEUE_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_PRIORITY_QUEUE_H
#define CCC_PRIORITY_QUEUE_H

#include "impl_priority_queue.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

/** @brief A container for pointer stability and an O(1) push and amortized o(lg
N) increase/decrease key.
@warning it is undefined behavior to access an uninitialized container.

A priority queue can be initialized on the stack, heap, or data segment at
runtime or compile time.*/
typedef struct ccc_pq_ ccc_priority_queue;

/** @brief The embedded struct type for operation of the priority queue.

It can be used in an allocating or non allocating container. If allocation is
prohibited the container assumes the element is wrapped in pre-allocated
memory with the appropriate lifetime and scope for the user's needs; the
container does not allocate or free in this case. If allocation is allowed
the container will handle copying the data wrapping the element to allocations
and deallocating when necessary. */
typedef struct ccc_pq_elem_ ccc_pq_elem;

/** @brief Initialize a priority queue at runtime or compile time.
@param [in] struct_name the name of the user type wrapping pq elems.
@param [in] pq_elem_field the name of the field for the pq elem.
@param [in] pq_order CCC_LES for a min pq or CCC_GRT for a max pq.
@param [in] alloc_fn the allocation function or NULL if allocation is banned.
@param [in] cmp_fn the function used to compare two user types.
@param [in] aux_data auxiliary data needed for comparison or destruction.
@return the initialized pq on the right side of an equality operator
(e.g. ccc_priority_queue pq = ccc_pq_init(...);) */
#define ccc_pq_init(struct_name, pq_elem_field, pq_order, alloc_fn, cmp_fn,    \
                    aux_data)                                                  \
    ccc_impl_pq_init(struct_name, pq_elem_field, pq_order, alloc_fn, cmp_fn,   \
                     aux_data)

/** @name Insert and Remove Interface
Insert and remove elements from the priority queue. */
/**@{*/

/** @brief Adds an element to the priority queue in correct total order. O(1).
@param [in] pq a pointer to the priority queue.
@param [in] elem a pointer to the intrusive element in the user type.
@return ok if the push succeeded. If NULL arguments are provided an input error
is returned. If allocation is permitted but allocation fails a memory error
is returned.

Note that if allocation is permitted the user type is copied into a newly
allocated node.

If allocation is not permitted this function assumes the memory wrapping elem
has been allocated with the appropriate lifetime for the user's needs. */
ccc_result ccc_pq_push(ccc_priority_queue *pq, ccc_pq_elem *elem);

/** @brief Pops the front element from the priority queue. Amortized O(lgN).
@param [in] pq a pointer to the priority queue.
@return ok if pop was successful or an input error if pq is NULL or empty. */
ccc_result ccc_pq_pop(ccc_priority_queue *pq);

/** Extract the element known to be in the pq without freeing memory. Amortized
O(lgN).
@param [in] pq a pointer to the priority queue.
@param [in] elem a pointer to the intrusive element in the user type.
@return a pointer to the extracted user type.

Note that the user must ensure that elem is in the priority queue. */
[[nodiscard]] void *ccc_pq_extract(ccc_priority_queue *pq, ccc_pq_elem *elem);

/** @brief Erase elem from the pq. Amortized O(lgN).
@param [in] pq a pointer to the priority queue.
@param [in] elem a pointer to the intrusive element in the user type.
@return ok if erase was successful or an input error if pq or elem is NULL or
pq is empty.

Note that the user must ensure that elem is in the priority queue. */
ccc_result ccc_pq_erase(ccc_priority_queue *pq, ccc_pq_elem *elem);

/** @brief Update the value in the user type wrapping elem.
@param [in] pq a pointer to the priority queue.
@param [in] elem a pointer to the intrusive element in the user type.
@param [in] fn the update function to act on the type wrapping elem.
@param [in] aux any auxiliary data needed for the update function.
@return true if the update occured false if parameters were invalid or the
function can deduce elem is not in the pq.
@warning the user must ensure elem is in the pq.

Note that this operation may incur uneccessary overhead if the user can't
deduce if an increase or decrease is occuring. See the increase and decrease
operations. O(1) best case, O(lgN) worst case. */
bool ccc_pq_update(ccc_priority_queue *pq, ccc_pq_elem *elem, ccc_update_fn *fn,
                   void *aux);

/** @brief Increases the value of the type wrapping elem. O(1) or O(lgN)
@param [in] pq a pointer to the priority queue.
@param [in] elem a pointer to the intrusive element in the user type.
@param [in] fn the update function to act on the type wrapping elem.
@param [in] aux any auxiliary data needed for the update function.
@return true if the increase occured false if parameters were invalid or the
function can deduce elem is not in the pq.

Note that this is optimal update technique if the priority queue has been
initialized as a max queue and the new value is known to be greater than the old
value. If this is a max heap O(1), otherwise O(lgN).

While the best case operation is O(1) the impact of restructuring on future pops
from the pq creates an amortized o(lgN) runtime for this function. */
bool ccc_pq_increase(ccc_priority_queue *pq, ccc_pq_elem *elem,
                     ccc_update_fn *fn, void *aux);

/** @brief Decreases the value of the type wrapping elem. O(1) or O(lgN)
@param [in] pq a pointer to the priority queue.
@param [in] elem a pointer to the intrusive element in the user type.
@param [in] fn the update function to act on the type wrapping elem.
@param [in] aux any auxiliary data needed for the update function.
@return true if the decrease occured false if parameters were invalid or the
function can deduce elem is not in the pq.

Note that this is optimal update technique if the priority queue has been
initialized as a min queue and the new value is known to be less than the old
value. If this is a min heap O(1), otherwise O(lgN).

While the best case operation is O(1) the impact of restructuring on future pops
from the pq creates an amortized o(lgN) runtime for this function. */
bool ccc_pq_decrease(ccc_priority_queue *pq, ccc_pq_elem *elem,
                     ccc_update_fn *fn, void *aux);

/**@}*/

/** @name Deallocation Interface
Deallocate the container. */
/**@{*/

/** @brief Removes all elements from the pq, freeing if needed.
@param [in] pq a pointer to the priority queue.
@param [in] fn the destructor function or NULL if not needed.
@return ok if the clear was successful or an input error for NULL args.

Note that if allocation is allowed the container will free the user type
wrapping each element in the pq. Therefore, the user should not free in the
destructor function. Only perform auxiliary cleanup operations if needed.

If allocation is not allowed, the user may free their stored types in the
destructor function if they wish to do so. The container simply removes all
the elements from the pq, calling fn on each user type if provided, and sets the
size to zero. */
ccc_result ccc_pq_clear(ccc_priority_queue *pq, ccc_destructor_fn *fn);

/**@}*/

/** @name State Interface
Obtain state from the container. */
/**@{*/

/** @brief Obtain a reference to the front of the priority queue. O(1).
@param [in] pq a pointer to the priority queue.
@return a reference to the front element in the pq. */
[[nodiscard]] void *ccc_pq_front(ccc_priority_queue const *pq);

/** @brief Returns true if the priority queue is empty false if not. O(1).
@param [in] pq a pointer to the priority queue.
@return true if the size is 0 or pq is NULL, false if not empty.  */
[[nodiscard]] bool ccc_pq_is_empty(ccc_priority_queue const *pq);

/** @brief Returns the size of the priority queue.
@param [in] pq a pointer to the priority queue.
@return the size of the pq or 0 if pq is NULL or pq is empty. */
[[nodiscard]] size_t ccc_pq_size(ccc_priority_queue const *pq);

/** @brief Verifies the internal invariants of the pq hold.
@param [in] pq a pointer to the priority queue.
@return true if the pq is valid false if pq is NULL or invalid. */
[[nodiscard]] bool ccc_pq_validate(ccc_priority_queue const *pq);

/** @brief Return the order used to initialize the pq.
@param [in] pq a pointer to the priority queue.
@return LES or GRT ordering. Any other ordering is invalid. */
[[nodiscard]] ccc_threeway_cmp ccc_pq_order(ccc_priority_queue const *pq);

/**@}*/

/** Define this preprocessor directive if shortened names are desired for the
priority queue container. Check for collisions before name shortening. */
#ifdef PRIORITY_QUEUE_USING_NAMESPACE_CCC
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
