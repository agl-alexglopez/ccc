/** @file
@brief The Flat Priority Queue Interface

A flat priority queue is a contiguous container storing storing elements in
heap order. This offers tightly packed data for efficient push, pop, min/max
operations in O(lg N). Also, this container requires no intrusive elements.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_FLAT_PRIORITY_QUEUE_H
#define CCC_FLAT_PRIORITY_QUEUE_H

/** @cond */
#include <stdbool.h>
#include <stddef.h>
/** @endcond */

#include "impl/impl_flat_priority_queue.h"
#include "types.h"

/** @brief A container offering direct storage and sorting of user data by heap
order.
@warning it is undefined behavior to access an uninitialized container.

A flat priority queue can be initialized on the stack, heap, or data segment at
runtime or compile time.*/
typedef struct ccc_fpq_ ccc_flat_priority_queue;

/** @brief Initialize a fpq as a min or max heap.
@param [in] mem_ptr a pointer to an array of user types or ((T *)NULL).
@param [in] capacity the capacity of contiguous elements at mem_ptr.
@param [in] cmp_order CCC_LES or CCC_GRT for min or max heap, respectively.
@param [in] alloc_fn the allocation function or NULL if no allocation.
@param [in] cmp_fn the user defined comarison function for user types.
@param [in] aux_data any auxiliary data needed for destruction of elements.
@return the initilialized priority queue on the right hand side of an equality
operator. (i.e. ccc_flat_priority_queue q = ccc_fpq_init(...);).

Note that to avoid temporary or unpredictable allocation the fpq requires one
slot for swapping. Therefore if the user wants a fixed size fpq of size N,
N + 1 capacity is required. */
#define ccc_fpq_init(mem_ptr, capacity, cmp_order, alloc_fn, cmp_fn, aux_data) \
    ccc_impl_fpq_init(mem_ptr, capacity, cmp_order, alloc_fn, cmp_fn, aux_data)

/** @name Insert and Remove Interface
Insert or remove elements from the flat priority queue. */
/**@{*/

/** @brief Order an existing array of elements as a min or max heap. O(N).
@param [in] mem_ptr a pointer to an array of user types or ((T *)NULL).
@param [in] capacity the capacity of contiguous elements at mem_ptr.
@param [in] size the size <= capacity.
@param [in] cmp_order CCC_LES or CCC_GRT for min or max heap, respectively.
@param [in] alloc_fn the allocation function or NULL if no allocation.
@param [in] cmp_fn the user defined comarison function for user types.
@param [in] aux_data any auxiliary data needed for destruction of elements.
@return the initilialized priority queue on the right hand side of an equality
operator. (i.e. ccc_flat_priority_queue q = ccc_fpq_heapify_init(...);).

Note that to avoid temporary or unpredictable allocation the fpq requires one
slot for swapping. Therefore if the user wants a fixed size fpq of size N,
N + 1 capacity is required. */
#define ccc_fpq_heapify_init(mem_ptr, capacity, size, cmp_order, alloc_fn,     \
                             cmp_fn, aux_data)                                 \
    ccc_impl_fpq_heapify_init(mem_ptr, capacity, size, cmp_order, alloc_fn,    \
                              cmp_fn, aux_data)

/** @brief Write a type directly to a priority queue slot. O(lgN).
@param [in] fpq a pointer to the priority queue.
@param [in] val_initializer the compound literal or direct scalar type.
@return a reference to the inserted element or NULL if allocation failed. */
#define ccc_fpq_emplace(fpq, val_initializer...)                               \
    ccc_impl_fpq_emplace(fpq, val_initializer)

/** @brief Copy input array into the fpq, organizing into heap. O(N).
@param [in] fpq a pointer to the priority queue.
@param [in] input_array an array of elements of the same size as the type used
to initialize fpq.
@param [in] input_n the number of contiguous elements at input_array.
@param [in] input_elem_size size of each element in input_array matching element
size of fpq.
@return OK if sorting was successful or an input error if bad input is
provided. A permission error will occur if no allocation is allowed and the
input array is larger than the fixed fpq capacity. A memory error will
occur if reallocation is required to fit all elements but reallocation fails.

Note that this version of heapify copies elements from the input array. If an
in place heapify is required use the initializer version of this method. */
ccc_result ccc_fpq_heapify(ccc_flat_priority_queue *fpq, void *input_array,
                           size_t input_n, size_t input_elem_size);

/** @brief Many allocate memory for the fpq.
@param [in] fpq a pointer to the priority queue.
@param [in] new_capacity the desirect capacity for the fpq.
@param [in] fn the allocation function. May be the same as used on init.
@return OK if allocation was successful or a memory error on failure. */
ccc_result ccc_fpq_alloc(ccc_flat_priority_queue *fpq, size_t new_capacity,
                         ccc_alloc_fn *fn);

/** @brief Pushes element pointed to at e into fpq. O(lgN).
@param [in] fpq a pointer to the priority queue.
@param [in] e a pointer to the user element of same type as in fpq.
@return a pointer to the inserted element or NULl if NULL args are provided or
push required more memory and failed. Failure can occur if the fpq is full and
allocation is not allowed or a resize failed when allocation is allowed. */
[[nodiscard]] void *ccc_fpq_push(ccc_flat_priority_queue *fpq, void const *e);

/** @brief Pop the front element (min or max) element in the fpq. O(lgN).
@param [in] fpq a pointer to the priority queue.
@return OK if the pop succeeds or an input error if fpq is NULL or empty. */
ccc_result ccc_fpq_pop(ccc_flat_priority_queue *fpq);

/** @brief Erase element e that is a handle to the stored fpq element.
@param [in] fpq a pointer to the priority queue.
@param [in] e a handle to the stored fpq element. Must be in the fpq.
@return OK if the erase is successful or an input error if NULL args are
provided or the fpq is empty.
@warning the user must ensure e is in the fpq.

Note that the reference to e is invalidated after this call. */
ccc_result ccc_fpq_erase(ccc_flat_priority_queue *fpq, void *e);

/** @brief Update e that is a handle to the stored fpq element. O(lgN).
@param [in] fpq a pointer to the flat priority queue.
@param [in] e a handle to the stored fpq element. Must be in the fpq.
@param [in] fn the update function to act on e.
@param [in] aux any auxiliary data needed for the update function.
@return true on success, false if parameters are invalid or fpq is empty.
@warning the user must ensure e is in the fpq. */
bool ccc_fpq_update(ccc_flat_priority_queue *fpq, void *e, ccc_update_fn *fn,
                    void *aux);

/** @brief Increase e that is a handle to the stored fpq element. O(lgN).
@param [in] fpq a pointer to the flat priority queue.
@param [in] e a handle to the stored fpq element. Must be in the fpq.
@param [in] fn the update function to act on e.
@param [in] aux any auxiliary data needed for the update function.
@return true on success, false if parameters are invalid or fpq is empty.
@warning the user must ensure e is in the fpq. */
bool ccc_fpq_increase(ccc_flat_priority_queue *fpq, void *e, ccc_update_fn *fn,
                      void *aux);

/** @brief Decrease e that is a handle to the stored fpq element. O(lgN).
@param [in] fpq a pointer to the flat priority queue.
@param [in] e a handle to the stored fpq element. Must be in the fpq.
@param [in] fn the update function to act on e.
@param [in] aux any auxiliary data needed for the update function.
@return true on success, false if parameters are invalid or fpq is empty.
@warning the user must ensure e is in the fpq. */
bool ccc_fpq_decrease(ccc_flat_priority_queue *fpq, void *e, ccc_update_fn *fn,
                      void *aux);

/**@}*/

/** @name Deallocation Interface
Deallocate the container. */
/**@{*/

/** @brief Clears the fpq calling fn on every element if provided. O(1)-O(N).
@param [in] fpq a pointer to the flat priority queue.
@param [in] fn the destructor function or NULL if not needed.
@return OK if input is valid and clear succeeds, otherwise input error.

Note that because the priority queue is flat there is no need to free elements
stored in the fpq. However, the destructor is free to manage cleanup in other
parts of user code as needed upon destruction of each element.

If the destructor is NULL, the function is O(1) and no attempt is made to
free capacity of the fpq. */
ccc_result ccc_fpq_clear(ccc_flat_priority_queue *fpq, ccc_destructor_fn *fn);

/** @brief Clears the fpq calling fn on every element if provided and frees the
underlying buffer. O(1)-O(N).
@param [in] fpq a pointer to the flat priority queue.
@param [in] fn the destructor function or NULL if not needed.
@return OK if input is valid and clear succeeds, otherwise input error. If the
buffer attempts to free but is not allowed a no alloc error is returned.

Note that because the priority queue is flat there is no need to free elements
stored in the fpq. However, the destructor is free to manage cleanup in other
parts of user code as needed upon destruction of each element.

If the destructor is NULL, the function is O(1) and only relies on the runtime
of the provided allocation function free operation. */
ccc_result ccc_fpq_clear_and_free(ccc_flat_priority_queue *fpq,
                                  ccc_destructor_fn *fn);

/**@}*/

/** @name State Interface
Obtain state from the container. */
/**@{*/

/** @brief Return a pointer to the front (min or max) element in the fpq. O(1).
@param [in] fpq a pointer to the priority queue.
@return A pointer to the front element or NULL if empty or fpq is NULL. */
[[nodiscard]] void *ccc_fpq_front(ccc_flat_priority_queue const *fpq);

/** @brief Returns true if the fpq is empty false if not. O(1).
@param [in] fpq a pointer to the flat priority queue.
@return true if the size is 0 or fpq is NULL, false if not empty.  */
[[nodiscard]] bool ccc_fpq_is_empty(ccc_flat_priority_queue const *fpq);

/** @brief Returns the size of the fpq.
@param [in] fpq a pointer to the flat priority queue.
@return the size of the fpq or 0 if fpq is NULL or fpq is empty. */
[[nodiscard]] size_t ccc_fpq_size(ccc_flat_priority_queue const *fpq);

/** @brief Verifies the internal invariants of the fpq hold.
@param [in] fpq a pointer to the flat priority queue.
@return true if the fpq is valid false if fpq is NULL or invalid. */
[[nodiscard]] bool ccc_fpq_validate(ccc_flat_priority_queue const *fpq);

/** @brief Return the order used to initialize the fpq.
@param [in] fpq a pointer to the flat priority queue.
@return LES or GRT ordering. Any other ordering is invalid. */
[[nodiscard]] ccc_threeway_cmp
ccc_fpq_order(ccc_flat_priority_queue const *fpq);

/**@}*/

/** Define this preprocessor directive if shortened names are desired for the
flat priority queue container. Check for collisions before name shortening. */
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
#    define fpq_clear_and_free(args...) ccc_fpq_clear_and_free(args)
#    define fpq_is_empty(args...) ccc_fpq_is_empty(args)
#    define fpq_size(args...) ccc_fpq_size(args)
#    define fpq_validate(args...) ccc_fpq_validate(args)
#    define fpq_order(args...) ccc_fpq_order(args)
#endif /* FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC */

#endif /* CCC_FLAT_PRIORITY_QUEUE_H */
