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
#include <stddef.h>
/** @endcond */

#include "impl/impl_flat_priority_queue.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A container offering direct storage and sorting of user data by heap
order.
@warning it is undefined behavior to access an uninitialized container.

A flat priority queue can be initialized on the stack, heap, or data segment at
runtime or compile time.*/
typedef struct ccc_fpq_ ccc_flat_priority_queue;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a fpq as a min or max heap.
@param [in] mem_ptr a pointer to an array of user types or ((T *)NULL).
@param [in] cmp_order CCC_LES or CCC_GRT for min or max heap, respectively.
@param [in] cmp_fn the user defined comarison function for user types.
@param [in] alloc_fn the allocation function or NULL if no allocation.
@param [in] aux_data any auxiliary data needed for destruction of elements.
@param [in] capacity the capacity of contiguous elements at mem_ptr.
@return the initilialized priority queue on the right hand side of an equality
operator. (i.e. ccc_flat_priority_queue q = ccc_fpq_init(...);).

Note that to avoid temporary or unpredictable allocation the fpq requires one
slot for swapping. Therefore if the user wants a fixed size fpq of size N,
N + 1 capacity is required. */
#define ccc_fpq_init(mem_ptr, cmp_order, cmp_fn, alloc_fn, aux_data, capacity) \
    ccc_impl_fpq_init(mem_ptr, cmp_order, cmp_fn, alloc_fn, aux_data, capacity)

/** @brief Order an existing array of elements as a min or max heap. O(N).
@param [in] mem_ptr a pointer to an array of user types or ((T *)NULL).
@param [in] cmp_order CCC_LES or CCC_GRT for min or max heap, respectively.
@param [in] cmp_fn the user defined comparison function for user types.
@param [in] alloc_fn the allocation function or NULL if no allocation.
@param [in] aux_data any auxiliary data needed for destruction of elements.
@param [in] capacity the capacity of contiguous elements at mem_ptr.
@param [in] size the size <= capacity.
@return the initilialized priority queue on the right hand side of an equality
operator. (i.e. ccc_flat_priority_queue q = ccc_fpq_heapify_init(...);).

Note that to avoid temporary or unpredictable allocation the fpq requires one
slot for swapping. Therefore if the user wants a fixed size fpq of size N,
N + 1 capacity is required. */
#define ccc_fpq_heapify_init(mem_ptr, cmp_order, cmp_fn, alloc_fn, aux_data,   \
                             capacity, size)                                   \
    ccc_impl_fpq_heapify_init(mem_ptr, cmp_order, cmp_fn, alloc_fn, aux_data,  \
                              capacity, size)

/** @brief Copy the fpq from src to newly initialized dst.
@param [in] dst the destination that will copy the source fpq.
@param [in] src the source of the fpq.
@param [in] fn the allocation function in case resizing of dst is needed.
@return the result of the copy operation. If the destination capacity is less
than the source capacity and no allocation function is provided an input error
is returned. If resizing is required and resizing of dst fails a memory error
is returned.
@note dst must have capacity greater than or equal to src. If dst capacity is
less than src, an allocation function must be provided with the fn argument.

Note that there are two ways to copy data from source to destination: provide
sufficient memory and pass NULL as fn, or allow the copy function to take care
of allocation for the copy.

Manual memory management with no allocation function provided.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
flat_priority_queue src
    = fpq_init((int[10]){}, CCC_LES, NULL, int_cmp, NULL, 10);
push_rand_ints(&src);
flat_priority_queue dst
    = fpq_init((int[11]){}, CCC_LES, NULL, int_cmp, NULL, 11);
ccc_result res = fpq_copy(&dst, &src, NULL);
```

The above requires dst capacity be greater than or equal to src capacity. Here
is memory management handed over to the copy function.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
flat_priority_queue src
    = fpq_init((int *)NULL, CCC_LES, std_alloc, int_cmp, NULL, 0);
push_rand_ints(&src);
flat_priority_queue dst
    = fpq_init((int *)NULL, CCC_LES, std_alloc, int_cmp, NULL, 0);
ccc_result res = fpq_copy(&dst, &src, std_alloc);
```

The above allows dst to have a capacity less than that of the src as long as
copy has been provided an allocation function to resize dst. Note that this
would still work if copying to a destination that the user wants as a fixed
size fpq.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
flat_priority_queue src
    = fpq_init((int *)NULL, CCC_LES, std_alloc, int_cmp, NULL, 0);
push_rand_ints(&src);
flat_priority_queue dst
    = fpq_init((int *)NULL, CCC_LES, NULL, int_cmp, NULL, 0);
ccc_result res = fpq_copy(&dst, &src, std_alloc);
```

The above sets up dst with fixed size while src is a dynamic fpq. Because an
allocation function is provided, the dst is resized once for the copy and
retains its fixed size after the copy is complete. This would require the user
to manually free the underlying buffer at dst eventually if this method is used.
Usually it is better to allocate the memory explicitly before the copy if
copying between ring buffers.

These options allow users to stay consistent across containers with their
memory management strategies. */
ccc_result ccc_fpq_copy(ccc_flat_priority_queue *dst,
                        ccc_flat_priority_queue const *src, ccc_alloc_fn *fn);

/**@}*/

/** @name Insert and Remove Interface
Insert or remove elements from the flat priority queue. */
/**@{*/

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

/** @brief Order n elements of the underlying fpq buffer as an fpq.
@param [in] fpq a pointer to the flat priority queue.
@param [in] n the number n of elements to be ordered. n + 1 must be <= capacity.
@return the result of the heapify operation, ok if successful or an error if
fpq is NULL or n is larger than the initialized capacity of the fpq.

This is another method to order a heap that already has all the elements one
needs sorted. The underlying buffer will be interpreted to have n valid elements
starting at index 0 to index n - 1. */
ccc_result ccc_fpq_heapify_inplace(ccc_flat_priority_queue *fpq, size_t n);

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
@return a reference to the element at its new position in the fpq on success,
NULL if parameters are invalid or fpq is empty.
@warning the user must ensure e is in the fpq. */
void *ccc_fpq_update(ccc_flat_priority_queue *fpq, void *e, ccc_update_fn *fn,
                     void *aux);

/** @brief Update the user type stored in the priority queue directly. O(lgN).
@param [in] fpq_ptr a pointer to the flat priority queue.
@param [in] T_ptr a pointer to the user type being updated.
@param [in] update_closure_over_T the semicolon separated statements to execute
on the user type at T_ptr (optionally wrapping {code here} in braces may help
with formatting). This closure may safely modify the key used to track the user
element's priority in the priority queue.
@return a reference to the element at its new position in the fpq on success,
NULL if parameters are invalid or fpq is empty.
@warning the user must ensure T_ptr is in the fpq.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
flat_priority_queue fpq = build_rand_int_fpq();
int *i = get_rand_fpq_elem(&fpq);
(void)fpq_update_w(&fpq, i, { *i = rand_key(); });
```

Note that whether the key increases or decreases does not affect runtime. */
#define ccc_fpq_update_w(fpq_ptr, T_ptr, update_closure_over_T...)             \
    ccc_impl_fpq_update_w(fpq_ptr, T_ptr, update_closure_over_T)

/** @brief Increase e that is a handle to the stored fpq element. O(lgN).
@param [in] fpq a pointer to the flat priority queue.
@param [in] e a handle to the stored fpq element. Must be in the fpq.
@param [in] fn the update function to act on e.
@param [in] aux any auxiliary data needed for the update function.
@return a reference to the element at its new position in the fpq on success,
NULL if parameters are invalid or fpq is empty.
@warning the user must ensure e is in the fpq. */
void *ccc_fpq_increase(ccc_flat_priority_queue *fpq, void *e, ccc_update_fn *fn,
                       void *aux);

/** @brief Increase the user type stored in the priority queue directly. O(lgN).
@param [in] fpq_ptr a pointer to the flat priority queue.
@param [in] T_ptr a pointer to the user type being updated.
@param [in] increase_closure_over_T the semicolon separated statements to
execute on the user type at T_ptr (optionally wrapping {code here} in braces may
help with formatting). This closure may safely modify the key used to track the
user element's priority in the priority queue.
@return a reference to the element at its new position in the fpq on success,
NULL if parameters are invalid or fpq is empty.
@warning the user must ensure T_ptr is in the fpq.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
flat_priority_queue fpq = build_rand_int_fpq();
int *i = get_rand_fpq_elem(&fpq);
(void)fpq_increase_w(&fpq, i, { (*i)++; });
```

Note that if this priority queue is min or max, the runtime is the same. */
#define ccc_fpq_increase_w(fpq_ptr, T_ptr, increase_closure_over_T...)         \
    ccc_impl_fpq_increase_w(fpq_ptr, T_ptr, increase_closure_over_T)

/** @brief Decrease e that is a handle to the stored fpq element. O(lgN).
@param [in] fpq a pointer to the flat priority queue.
@param [in] e a handle to the stored fpq element. Must be in the fpq.
@param [in] fn the update function to act on e.
@param [in] aux any auxiliary data needed for the update function.
@return a reference to the element at its new position in the fpq on success,
NULL if parameters are invalid or fpq is empty.
@warning the user must ensure e is in the fpq. */
void *ccc_fpq_decrease(ccc_flat_priority_queue *fpq, void *e, ccc_update_fn *fn,
                       void *aux);

/** @brief Increase the user type stored in the priority queue directly. O(lgN).
@param [in] fpq_ptr a pointer to the flat priority queue.
@param [in] T_ptr a pointer to the user type being updated.
@param [in] decrease_closure_over_T the semicolon separated statements to
execute on the user type at T_ptr (optionally wrapping {code here} in braces may
help with formatting). This closure may safely modify the key used to track the
user element's priority in the priority queue.
@return a reference to the element at its new position in the fpq on success,
NULL if parameters are invalid or fpq is empty.
@warning the user must ensure T_ptr is in the fpq.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
flat_priority_queue fpq = build_rand_int_fpq();
int *i = get_rand_fpq_elem(&fpq);
(void)fpq_decrease_w(&fpq, i, { (*i)--; });
```

Note that if this priority queue is min or max, the runtime is the same. */
#define ccc_fpq_decrease_w(fpq_ptr, T_ptr, decrease_closure_over_T...)         \
    ccc_impl_fpq_decrease_w(fpq_ptr, T_ptr, decrease_closure_over_T)

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

/** @brief Return the index of an element known to be in the fpq. O(1).
@param [in] fpq a pointer to the priority queue.
@param [in] e an element known to be in the fpq.
@return the index of the element known to be in the fpq or -1 if e is out of
range of the fpq. */
[[nodiscard]] ptrdiff_t ccc_fpq_i(ccc_flat_priority_queue const *fpq,
                                  void const *e);

/** @brief Returns true if the fpq is empty false if not. O(1).
@param [in] fpq a pointer to the flat priority queue.
@return true if the size is 0, false if not empty. Error if fpq is NULL. */
[[nodiscard]] ccc_tribool ccc_fpq_is_empty(ccc_flat_priority_queue const *fpq);

/** @brief Returns the size of the fpq.
@param [in] fpq a pointer to the flat priority queue.
@return the size of the fpq or 0 if fpq is NULL or fpq is empty. */
[[nodiscard]] size_t ccc_fpq_size(ccc_flat_priority_queue const *fpq);

/** @brief Returns the capacity of the fpq.
@param [in] fpq a pointer to the flat priority queue.
@return the capacity of the fpq or 0 if fpq is NULL or fpq is empty. */
[[nodiscard]] size_t ccc_fpq_capacity(ccc_flat_priority_queue const *fpq);

/** @brief Return a pointer to the base of the backing array. O(1).
@param [in] fpq a pointer to the priority queue.
@return A pointer to the base of the backing array or NULL if fpq is NULL.
@note this reference starts at index 0 of the backing array. All fpq elements
are stored contiguously starting at the base through size of the fpq.
@warning it is the users responsibility to ensure that access to any data is
within the capacity of the backing buffer. */
[[nodiscard]] void *ccc_fpq_data(ccc_flat_priority_queue const *fpq);

/** @brief Verifies the internal invariants of the fpq hold.
@param [in] fpq a pointer to the flat priority queue.
@return true if the fpq is valid false if invalid. Error if fpq is NULL. */
[[nodiscard]] ccc_tribool ccc_fpq_validate(ccc_flat_priority_queue const *fpq);

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
#    define fpq_heapify_init(args...) ccc_fpq_heapify_init(args)
#    define fpq_copy(args...) ccc_fpq_copy(args)
#    define fpq_heapify(args...) ccc_fpq_heapify(args)
#    define fpq_emplace(args...) ccc_fpq_emplace(args)
#    define fpq_realloc(args...) ccc_fpq_realloc(args)
#    define fpq_push(args...) ccc_fpq_push(args)
#    define fpq_front(args...) ccc_fpq_front(args)
#    define fpq_i(args...) ccc_fpq_i(args)
#    define fpq_pop(args...) ccc_fpq_pop(args)
#    define fpq_extract(args...) ccc_fpq_extract(args)
#    define fpq_update(args...) ccc_fpq_update(args)
#    define fpq_increase(args...) ccc_fpq_increase(args)
#    define fpq_decrease(args...) ccc_fpq_decrease(args)
#    define fpq_update_w(args...) ccc_fpq_update_w(args)
#    define fpq_increase_w(args...) ccc_fpq_increase_w(args)
#    define fpq_decrease_w(args...) ccc_fpq_decrease_w(args)
#    define fpq_clear(args...) ccc_fpq_clear(args)
#    define fpq_clear_and_free(args...) ccc_fpq_clear_and_free(args)
#    define fpq_is_empty(args...) ccc_fpq_is_empty(args)
#    define fpq_size(args...) ccc_fpq_size(args)
#    define fpq_data(args...) ccc_fpq_data(args)
#    define fpq_validate(args...) ccc_fpq_validate(args)
#    define fpq_order(args...) ccc_fpq_order(args)
#endif /* FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC */

#endif /* CCC_FLAT_PRIORITY_QUEUE_H */
