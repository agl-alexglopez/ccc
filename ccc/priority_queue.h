/** @file
@brief The Priority Queue Interface

A priority queue offers simple, fast, pointer stable management of a priority
queue. Push is O(1). The cost to execute the increase key in a max heap and
decrease key in a min heap is O(1). However, due to the restructuring this
causes that increases the cost of later pops, the more accurate runtime is o(lg
N). The cost of a pop operation is O(lg N).

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define PRIORITY_QUEUE_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_PRIORITY_QUEUE_H
#define CCC_PRIORITY_QUEUE_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "impl/impl_priority_queue.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

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

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a priority queue at runtime or compile time.
@param [in] struct_name the name of the user type wrapping pq elems.
@param [in] pq_elem_field the name of the field for the pq elem.
@param [in] pq_order CCC_LES for a min pq or CCC_GRT for a max pq.
@param [in] cmp_fn the function used to compare two user types.
@param [in] alloc_fn the allocation function or NULL if allocation is banned.
@param [in] aux_data auxiliary data needed for comparison or destruction.
@return the initialized pq on the right side of an equality operator
(e.g. ccc_priority_queue pq = ccc_pq_init(...);) */
#define ccc_pq_init(struct_name, pq_elem_field, pq_order, cmp_fn, alloc_fn,    \
                    aux_data)                                                  \
    ccc_impl_pq_init(struct_name, pq_elem_field, pq_order, cmp_fn, alloc_fn,   \
                     aux_data)

/**@}*/

/** @name Insert and Remove Interface
Insert and remove elements from the priority queue. */
/**@{*/

/** @brief Adds an element to the priority queue in correct total order. O(1).
@param [in] pq a pointer to the priority queue.
@param [in] elem a pointer to the intrusive element in the user type.
@return a reference to the newly inserted user type or NULL if NULL arguments
are provided or allocation fails when permitted.

Note that if allocation is permitted the user type is copied into a newly
allocated node.

If allocation is not permitted this function assumes the memory wrapping elem
has been allocated with the appropriate lifetime for the user's needs. */
[[nodiscard]] void *ccc_pq_push(ccc_priority_queue *pq, ccc_pq_elem *elem);

/** @brief Write user type directly to a newly allocated priority queue elem.
@param [in] priority_queue_ptr a pointer to the priority queue.
@param [in] lazy_value the compound literal to write to the allocation.
@return a reference to the successfully inserted element or NULL if allocation
fails or is not allowed.

Note that the priority queue must be initialized with allocation permission to
use this macro. */
#define ccc_pq_emplace(priority_queue_ptr, lazy_value...)                      \
    ccc_impl_pq_emplace(priority_queue_ptr, lazy_value)

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

/** @brief Update the priority in the user type wrapping elem.
@param [in] pq a pointer to the priority queue.
@param [in] elem a pointer to the intrusive element in the user type.
@param [in] fn the update function to act on the type wrapping elem.
@param [in] aux any auxiliary data needed for the update function.
@return true if the update occurred. Error if parameters were invalid or the
function can deduce elem is not in the pq.
@warning the user must ensure elem is in the pq.

Note that this operation may incur unnecessary overhead if the user can't
deduce if an increase or decrease is occurring. See the increase and decrease
operations. O(1) best case, O(lgN) worst case. */
ccc_tribool ccc_pq_update(ccc_priority_queue *pq, ccc_pq_elem *elem,
                          ccc_update_fn *fn, void *aux);

/** @brief Update the priority in the user type stored in the container.
@param [in] pq_ptr a pointer to the priority queue.
@param [in] pq_elem_ptr a pointer to the intrusive handle in the user type.
@param [in] update_closure_over_T the semicolon separated statements to execute
on the user type which wraps pq_elem_ptr (optionally wrapping {code here} in
braces may help with formatting). This closure may safely modify the key used to
track the user element's priority in the priority queue.
@return true if the update occurred. Error if parameters were invalid or the
function can deduce elem is not in the pq.
@warning the user must ensure elem is in the pq and pq_elem_ptr points to the
intrusive element in the same user type that is being updated.

```
#define PRIORITY_QUEUE_USING_NAMESPACE_CCC
struct val
{
    pq_elem e;
    int key;
};
priority_queue pq = build_rand_pq();
struct val *i = get_rand_pq_elem(&pq);
pq_update_w(&pq, &i->e, { i->key = rand_key(); });
```

Note that this operation may incur unnecessary overhead if the user can't
deduce if an increase or decrease is occurring. See the increase and decrease
operations. O(1) best case, O(lgN) worst case. */
#define ccc_pq_update_w(pq_ptr, pq_elem_ptr, update_closure_over_T...)         \
    ccc_impl_pq_update_w(pq_ptr, pq_elem_ptr, update_closure_over_T)

/** @brief Increases the priority of the type wrapping elem. O(1) or O(lgN)
@param [in] pq a pointer to the priority queue.
@param [in] elem a pointer to the intrusive element in the user type.
@param [in] fn the update function to act on the type wrapping elem.
@param [in] aux any auxiliary data needed for the update function.
@return true if the increase occurred. Error if parameters were invalid or the
function can deduce elem is not in the pq.
@warning the data structure will be in an invalid state if the user decreases
the priority by mistake in this function.

Note that this is optimal update technique if the priority queue has been
initialized as a max queue and the new value is known to be greater than the old
value. If this is a max heap O(1), otherwise O(lgN).

While the best case operation is O(1) the impact of restructuring on future pops
from the pq creates an amortized o(lgN) runtime for this function. */
ccc_tribool ccc_pq_increase(ccc_priority_queue *pq, ccc_pq_elem *elem,
                            ccc_update_fn *fn, void *aux);

/** @brief Increases the priority of the user type stored in the container.
@param [in] pq_ptr a pointer to the priority queue.
@param [in] pq_elem_ptr a pointer to the intrusive handle in the user type.
@param [in] increase_closure_over_T the semicolon separated statements to
execute on the user type which wraps pq_elem_ptr (optionally wrapping {code
here} in braces may help with formatting). This closure may safely increase the
key used to track the user element's priority in the priority queue.
@return true if the increase occurred. Error if parameters were invalid or the
function can deduce elem is not in the pq.
@warning the user must ensure elem is in the pq and pq_elem_ptr points to the
intrusive element in the same user type that is being increased. The data
structure will be in an invalid state if the user decreases the priority by
mistake in this function.

```
#define PRIORITY_QUEUE_USING_NAMESPACE_CCC
struct val
{
    pq_elem e;
    int key;
};
priority_queue pq = build_rand_pq();
struct val *i = get_rand_pq_elem(&pq);
pq_increase_w(&pq, &i->e, { i->key++; });
```

Note that this is optimal update technique if the priority queue has been
initialized as a max queue and the new value is known to be greater than the old
value. If this is a max heap O(1), otherwise O(lgN).

While the best case operation is O(1) the impact of restructuring on future pops
from the pq creates an amortized o(lgN) runtime for this function. */
#define ccc_pq_increase_w(pq_ptr, pq_elem_ptr, increase_closure_over_T...)     \
    ccc_impl_pq_increase_w(pq_ptr, pq_elem_ptr, increase_closure_over_T)

/** @brief Decreases the value of the type wrapping elem. O(1) or O(lgN)
@param [in] pq a pointer to the priority queue.
@param [in] elem a pointer to the intrusive element in the user type.
@param [in] fn the update function to act on the type wrapping elem.
@param [in] aux any auxiliary data needed for the update function.
@return true if the decrease occurred. Error if parameters were invalid or the
function can deduce elem is not in the pq.

Note that this is optimal update technique if the priority queue has been
initialized as a min queue and the new value is known to be less than the old
value. If this is a min heap O(1), otherwise O(lgN).

While the best case operation is O(1) the impact of restructuring on future pops
from the pq creates an amortized o(lgN) runtime for this function. */
ccc_tribool ccc_pq_decrease(ccc_priority_queue *pq, ccc_pq_elem *elem,
                            ccc_update_fn *fn, void *aux);

/** @brief Decreases the priority of the user type stored in the container.
@param [in] pq_ptr a pointer to the priority queue.
@param [in] pq_elem_ptr a pointer to the intrusive handle in the user type.
@param [in] decrease_closure_over_T the semicolon separated statements to
execute on the user type which wraps pq_elem_ptr (optionally wrapping {code
here} in braces may help with formatting). This closure may safely decrease the
key used to track the user element's priority in the priority queue.
@return true if the decrease occurred. Error if parameters were invalid or the
function can deduce elem is not in the pq.
@warning the user must ensure elem is in the pq and pq_elem_ptr points to the
intrusive element in the same user type that is being decreased. The data
structure will be in an invalid state if the user decreases the priority by
mistake in this function.

```
#define PRIORITY_QUEUE_USING_NAMESPACE_CCC
struct val
{
    pq_elem e;
    int key;
};
priority_queue pq = build_rand_pq();
struct val *i = get_rand_pq_elem(&pq);
pq_decrease_w(&pq, &i->e, { i->key--; });
```
Note that this is optimal update technique if the priority queue has been
initialized as a min queue and the new value is known to be less than the old
value. If this is a min heap O(1), otherwise O(lgN).

While the best case operation is O(1) the impact of restructuring on future pops
from the pq creates an amortized o(lgN) runtime for this function. */
#define ccc_pq_decrease_w(pq_ptr, pq_elem_ptr, decrease_closure_over_T...)     \
    ccc_impl_pq_decrease_w(pq_ptr, pq_elem_ptr, decrease_closure_over_T)

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
@return true if the size is 0, false if not empty. Error if pq is NULL. */
[[nodiscard]] ccc_tribool ccc_pq_is_empty(ccc_priority_queue const *pq);

/** @brief Returns the size of the priority queue.
@param [in] pq a pointer to the priority queue.
@return the size of the pq or 0 if pq is NULL or pq is empty. */
[[nodiscard]] size_t ccc_pq_size(ccc_priority_queue const *pq);

/** @brief Verifies the internal invariants of the pq hold.
@param [in] pq a pointer to the priority queue.
@return true if the pq is valid false if pq is invalid. Error if pq is NULL. */
[[nodiscard]] ccc_tribool ccc_pq_validate(ccc_priority_queue const *pq);

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
#    define pq_emplace(args...) ccc_pq_emplace(args)
#    define pq_pop(args...) ccc_pq_pop(args)
#    define pq_extract(args...) ccc_pq_extract(args)
#    define pq_is_empty(args...) ccc_pq_is_empty(args)
#    define pq_size(args...) ccc_pq_size(args)
#    define pq_update(args...) ccc_pq_update(args)
#    define pq_increase(args...) ccc_pq_increase(args)
#    define pq_decrease(args...) ccc_pq_decrease(args)
#    define pq_update_w(args...) ccc_pq_update_w(args)
#    define pq_increase_w(args...) ccc_pq_increase_w(args)
#    define pq_decrease_w(args...) ccc_pq_decrease_w(args)
#    define pq_order(args...) ccc_pq_order(args)
#    define pq_clear(args...) ccc_pq_clear(args)
#    define pq_validate(args...) ccc_pq_validate(args)
#endif /* PRIORITY_QUEUE_USING_NAMESPACE_CCC */

#endif /* CCC_PRIORITY_QUEUE_H */
