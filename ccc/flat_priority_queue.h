/** @cond
Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
@endcond */
/** @file
@brief The Flat Priority Queue Interface

A flat priority queue is a contiguous container storing elements in heap order.
This offers tightly packed data for efficient push, pop, min/max operations in
O(lg N) time.

A flat priority queue can use memory sources from the stack, heap, or data
segment and can be initialized at compile or runtime. The container offers
efficient initialization options such as an `O(N)` heap building initializer.
The flat priority queue also offers a destructive heap sort option if the user
desires an in-place strict `O(N * log(N))` and `O(1)` space sort that does not
use recursion.

Many functions in the interface request a temporary argument be passed as a swap
slot. This is because a flat priority queue is backed by a binary heap and
swaps elements to maintain its properties. Because the user may decide the
flat priority queue has no allocation permission, the user must provide this
swap slot. An easy way to do this in C99 and later is with anonymous compound
literal references. For example, if we have a `int` flat priority queue we can
provide a temporary slot inline to a function as follows.

```
CCC_fpq_pop(&pq, &(int){0});
```

Any user defined struct can also use this technique.

```
CCC_fpq_pop(&pq, &(struct my_type){});
```

This is the preferred method because the storage remains anonymous and
inaccessible to other code in the calling scope.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `CCC_` prefix. */
#ifndef CCC_FLAT_PRIORITY_QUEUE_H
#define CCC_FLAT_PRIORITY_QUEUE_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "buffer.h"
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
typedef struct CCC_fpq CCC_flat_priority_queue;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a fpq as a min or max heap.
@param [in] mem_ptr a pointer to an array of user types or NULL.
@param [in] any_type_name the name of the user type.
@param [in] cmp_order CCC_LES or CCC_GRT for min or max heap, respectively.
@param [in] cmp_fn the user defined comarison function for user types.
@param [in] alloc_fn the allocation function or NULL if no allocation.
@param [in] aux_data any auxiliary data needed for destruction of elements.
@param [in] capacity the capacity of contiguous elements at mem_ptr.
@return the initialized priority queue on the right hand side of an equality
operator. (i.e. CCC_flat_priority_queue q = CCC_fpq_init(...);). */
#define CCC_fpq_init(mem_ptr, any_type_name, cmp_order, cmp_fn, alloc_fn,      \
                     aux_data, capacity)                                       \
    CCC_impl_fpq_init(mem_ptr, any_type_name, cmp_order, cmp_fn, alloc_fn,     \
                      aux_data, capacity)

/** @brief Partial order an array of elements as a min or max heap. O(N).
@param [in] mem_ptr a pointer to an array of user types or NULL.
@param [in] any_type_name the name of the user type.
@param [in] cmp_order CCC_LES or CCC_GRT for min or max heap, respectively.
@param [in] cmp_fn the user defined comparison function for user types.
@param [in] alloc_fn the allocation function or NULL if no allocation.
@param [in] aux_data any auxiliary data needed for destruction of elements.
@param [in] capacity the capacity of contiguous elements at mem_ptr.
@param [in] size the size <= capacity.
@return the initialized priority queue on the right hand side of an equality
operator. (i.e. CCC_flat_priority_queue q = CCC_fpq_heapify_init(...);). */
#define CCC_fpq_heapify_init(mem_ptr, any_type_name, cmp_order, cmp_fn,        \
                             alloc_fn, aux_data, capacity, size)               \
    CCC_impl_fpq_heapify_init(mem_ptr, any_type_name, cmp_order, cmp_fn,       \
                              alloc_fn, aux_data, capacity, size)

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
CCC_result res = fpq_copy(&dst, &src, NULL);
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
CCC_result res = fpq_copy(&dst, &src, std_alloc);
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
CCC_result res = fpq_copy(&dst, &src, std_alloc);
```

The above sets up dst with fixed size while src is a dynamic fpq. Because an
allocation function is provided, the dst is resized once for the copy and
retains its fixed size after the copy is complete. This would require the user
to manually free the underlying buffer at dst eventually if this method is used.
Usually it is better to allocate the memory explicitly before the copy if
copying between ring buffers.

These options allow users to stay consistent across containers with their
memory management strategies. */
CCC_result CCC_fpq_copy(CCC_flat_priority_queue *dst,
                        CCC_flat_priority_queue const *src,
                        CCC_any_alloc_fn *fn);

/** @brief Reserves space for at least to_add more elements.
@param [in] fpq a pointer to the flat priority queue.
@param [in] to_add the number of elements to add to the current size.
@param [in] fn the allocation function to use to reserve memory.
@return the result of the reservation. OK if successful, otherwise an error
status is returned.
@note see the CCC_fpq_clear_and_free_reserve function if this function is
being used for a one-time dynamic reservation.

This function can be used for a dynamic fpq with or without allocation
permission. If the fpq has allocation permission, it will reserve the required
space and later resize if more space is needed.

If the fpq has been initialized with no allocation permission and no memory
this function can serve as a one-time reservation. This is helpful when a fixed
size is needed but that size is only known dynamically at runtime. To free the
fpq in such a case see the CCC_fpq_clear_and_free_reserve function. */
CCC_result CCC_fpq_reserve(CCC_flat_priority_queue *fpq, size_t to_add,
                           CCC_any_alloc_fn *fn);

/**@}*/

/** @name Insert and Remove Interface
Insert or remove elements from the flat priority queue. */
/**@{*/

/** @brief Write a type directly to a priority queue slot. O(lgN).
@param [in] fpq a pointer to the priority queue.
@param [in] val_initializer the compound literal or direct scalar type.
@return a reference to the inserted element or NULL if allocation failed. */
#define CCC_fpq_emplace(fpq, val_initializer...)                               \
    CCC_impl_fpq_emplace(fpq, val_initializer)

/** @brief Copy input array into the fpq, organizing into heap. O(N).
@param [in] fpq a pointer to the priority queue.
@param [in] tmp a pointer to an additional element of array type for swapping.
@param [in] input_array an array of elements of the same size as the type used
to initialize fpq.
@param [in] input_n the number of contiguous elements at input_array.
@param [in] input_sizeof_type size of each element in input_array matching
element size of fpq.
@return OK if sorting was successful or an input error if bad input is
provided. A permission error will occur if no allocation is allowed and the
input array is larger than the fixed fpq capacity. A memory error will
occur if reallocation is required to fit all elements but reallocation fails.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`.

Note that this version of heapify copies elements from the input array. If an
in place heapify is required use the initializer version of this method. */
CCC_result CCC_fpq_heapify(CCC_flat_priority_queue *fpq, void *tmp,
                           void *input_array, size_t input_n,
                           size_t input_sizeof_type);

/** @brief Order n elements of the underlying fpq buffer as an fpq.
@param [in] fpq a pointer to the flat priority queue.
@param [in] tmp a pointer to a dummy user type that will be used for swapping.
@param [in] n the number n of elements where  0 < (n + 1) <= capacity.
@return the result of the heapify operation, ok if successful or an error if
fpq is NULL or n is larger than the initialized capacity of the fpq.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`.

This is another method to order a heap that already has all the elements one
needs sorted. The underlying buffer will be interpreted to have n valid elements
starting at index 0 to index n - 1. */
CCC_result CCC_fpq_heapify_inplace(CCC_flat_priority_queue *fpq, void *tmp,
                                   size_t n);

/** @brief Pushes element pointed to at e into fpq. O(lgN).
@param [in] fpq a pointer to the priority queue.
@param [in] elem a pointer to the user element of same type as in fpq.
@param [in] tmp a pointer to a dummy user type that will be used for swapping.
@return a pointer to the inserted element or NULl if NULL args are provided or
push required more memory and failed. Failure can occur if the fpq is full and
allocation is not allowed or a resize failed when allocation is allowed.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`. */
[[nodiscard]] void *CCC_fpq_push(CCC_flat_priority_queue *fpq, void const *elem,
                                 void *tmp);

/** @brief Pop the front element (min or max) element in the fpq. O(lgN).
@param [in] fpq a pointer to the priority queue.
@param [in] tmp a pointer to a dummy user type that will be used for swapping.
@return OK if the pop succeeds or an input error if fpq is NULL or empty.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`. */
CCC_result CCC_fpq_pop(CCC_flat_priority_queue *fpq, void *tmp);

/** @brief Erase element e that is a handle to the stored fpq element.
@param [in] fpq a pointer to the priority queue.
@param [in] elem a pointer to the stored fpq element. Must be in the fpq.
@param [in] tmp a pointer to a dummy user type that will be used for swapping.
@return OK if the erase is successful or an input error if NULL args are
provided or the fpq is empty.
@warning the user must ensure e is in the fpq.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`.

Note that the reference to elem is invalidated after this call. */
CCC_result CCC_fpq_erase(CCC_flat_priority_queue *fpq, void *elem, void *tmp);

/** @brief Update e that is a handle to the stored fpq element. O(lgN).
@param [in] fpq a pointer to the flat priority queue.
@param [in] elem a pointer to the stored fpq element. Must be in the fpq.
@param [in] tmp a pointer to a dummy user type that will be used for swapping.
@param [in] fn the update function to act on e.
@param [in] aux any auxiliary data needed for the update function.
@return a reference to the element at its new position in the fpq on success,
NULL if parameters are invalid or fpq is empty.
@warning the user must ensure e is in the fpq.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`. */
void *CCC_fpq_update(CCC_flat_priority_queue *fpq, void *elem, void *tmp,
                     CCC_any_type_update_fn *fn, void *aux);

/** @brief Update the user type stored in the priority queue directly. O(lgN).
@param [in] fpq_ptr a pointer to the flat priority queue.
@param [in] any_type_ptr a pointer to the user type being updated.
@param [in] update_closure_over_T the semicolon separated statements to execute
on the user type at T (optionally wrapping {code here} in braces may help
with formatting). This closure may safely modify the key used to track the user
element's priority in the priority queue.
@return a reference to the element at its new position in the fpq on success,
NULL if parameters are invalid or fpq is empty.
@warning the user must ensure any_type_ptr is in the fpq.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
flat_priority_queue fpq = build_rand_int_fpq();
(void)fpq_update_w(&fpq, get_rand_fpq_elem(&fpq), { *T = rand_key(); });
```

Note that whether the key increases or decreases does not affect runtime. */
#define CCC_fpq_update_w(fpq_ptr, any_type_ptr, update_closure_over_T...)      \
    CCC_impl_fpq_update_w(fpq_ptr, any_type_ptr, update_closure_over_T)

/** @brief Increase e that is a handle to the stored fpq element. O(lgN).
@param [in] fpq a pointer to the flat priority queue.
@param [in] elem a pointer to the stored fpq element. Must be in the fpq.
@param [in] tmp a pointer to a dummy user type that will be used for swapping.
@param [in] fn the update function to act on e.
@param [in] aux any auxiliary data needed for the update function.
@return a reference to the element at its new position in the fpq on success,
NULL if parameters are invalid or fpq is empty.
@warning the user must ensure e is in the fpq.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`. */
void *CCC_fpq_increase(CCC_flat_priority_queue *fpq, void *elem, void *tmp,
                       CCC_any_type_update_fn *fn, void *aux);

/** @brief Increase the user type stored in the priority queue directly. O(lgN).
@param [in] fpq_ptr a pointer to the flat priority queue.
@param [in] any_type_ptr a pointer to the user type being updated.
@param [in] increase_closure_over_T the semicolon separated statements to
execute on the user type at T (optionally wrapping {code here} in
braces may help with formatting). This closure may safely modify the key used to
track the user element's priority in the priority queue.
@return a reference to the element at its new position in the fpq on success,
NULL if parameters are invalid or fpq is empty.
@warning the user must ensure any_type_ptr is in the fpq.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
flat_priority_queue fpq = build_rand_int_fpq();
(void)fpq_increase_w(&fpq, get_rand_fpq_elem(&fpq), { (*T)++; });
```

Note that if this priority queue is min or max, the runtime is the same. */
#define CCC_fpq_increase_w(fpq_ptr, any_type_ptr, increase_closure_over_T...)  \
    CCC_impl_fpq_increase_w(fpq_ptr, any_type_ptr, increase_closure_over_T)

/** @brief Decrease e that is a handle to the stored fpq element. O(lgN).
@param [in] fpq a pointer to the flat priority queue.
@param [in] elem a pointer to the stored fpq element. Must be in the fpq.
@param [in] tmp a pointer to a dummy user type that will be used for swapping.
@param [in] fn the update function to act on e.
@param [in] aux any auxiliary data needed for the update function.
@return a reference to the element at its new position in the fpq on success,
NULL if parameters are invalid or fpq is empty.
@warning the user must ensure e is in the fpq.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`. */
void *CCC_fpq_decrease(CCC_flat_priority_queue *fpq, void *elem, void *tmp,
                       CCC_any_type_update_fn *fn, void *aux);

/** @brief Increase the user type stored in the priority queue directly. O(lgN).
@param [in] fpq_ptr a pointer to the flat priority queue.
@param [in] any_type_ptr a pointer to the user type being updated.
@param [in] decrease_closure_over_T the semicolon separated statements to
execute on the user type at T (optionally wrapping {code here} in
braces may help with formatting). This closure may safely modify the key used to
track the user element's priority in the priority queue.
@return a reference to the element at its new position in the fpq on success,
NULL if parameters are invalid or fpq is empty.
@warning the user must ensure any_type_ptr is in the fpq.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
flat_priority_queue fpq = build_rand_int_fpq();
(void)fpq_decrease_w(&fpq, get_rand_fpq_elem(&fpq), { (*T)--; });
```

Note that if this priority queue is min or max, the runtime is the same. */
#define CCC_fpq_decrease_w(fpq_ptr, any_type_ptr, decrease_closure_over_T...)  \
    CCC_impl_fpq_decrease_w(fpq_ptr, any_type_ptr, decrease_closure_over_T)

/**@}*/

/** @name Deallocation Interface
Deallocate the container or destroy the heap invariants. */
/**@{*/

/** @brief Destroys the fpq by sorting its data and returning the underlying
buffer. The data is sorted in `O(N * log(N))` time and `O(1)` space.
@param [in] fpq the fpq to be sorted and destroyed.
@param [in] tmp a pointer to a dummy user type that will be used for swapping.
@return a buffer filled from the back to the front by the fpq order. If the fpq
is initialized CCC_LES the returned buffer is sorted in non-increasing order
from index [0, N). If the fpq is initialized CCC_GRT the buffer is sorted in
non-descending order from index [0, N). If fpq is NULL, the buffer is default
initialized and unusable.
@warning all fields of the fpq are cleared or otherwise default initialized so
the fpq is unusable as a container after sorting. This function assumes the fpq
has been previously initialized. Therefore, if the returned buffer value is not
used the fpq memory is leaked.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`.

The underlying memory storage source for the fpq, a buffer, is not moved or
copied during the sort. If a copy of the sorted data is preferred copy the data
the data to another initialized fpq with the `CCC_fpq_copy` function first then
sort that copy.

The sort is not inherently stable and uses the provided comparison function to
the fpq to order the elements. */
[[nodiscard]] CCC_buffer CCC_fpq_heapsort(CCC_flat_priority_queue *fpq,
                                          void *tmp);

/** @brief Clears the fpq calling fn on every element if provided.
O(1)-O(N).
@param [in] fpq a pointer to the flat priority queue.
@param [in] fn the destructor function or NULL if not needed.
@return OK if input is valid and clear succeeds, otherwise input error.

Note that because the priority queue is flat there is no need to free
elements stored in the fpq. However, the destructor is free to manage
cleanup in other parts of user code as needed upon destruction of each
element.

If the destructor is NULL, the function is O(1) and no attempt is made to
free capacity of the fpq. */
CCC_result CCC_fpq_clear(CCC_flat_priority_queue *fpq,
                         CCC_any_type_destructor_fn *fn);

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
CCC_result CCC_fpq_clear_and_free(CCC_flat_priority_queue *fpq,
                                  CCC_any_type_destructor_fn *fn);

/** @brief Frees all slots in the fpq and frees the underlying buffer that was
previously dynamically reserved with the reserve function.
@param [in] fpq the fpq to be cleared.
@param [in] destructor the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the fpq before their slots are
dropped.
@param [in] alloc the required allocation function to provide to a dynamically
reserved fpq. Any auxiliary data provided upon initialization will be passed to
the allocation function when called.
@return the result of free operation. OK if success, or an error status to
indicate the error.
@warning It is an error to call this function on a fpq that was not reserved
with the provided CCC_any_alloc_fn. The fpq must have existing memory to free.

This function covers the edge case of reserving a dynamic capacity for a fpq
at runtime but denying the fpq allocation permission to resize. This can help
prevent a fpq from growing unbounded. The user in this case knows the fpq does
not have allocation permission and therefore no further memory will be dedicated
to the fpq.

However, to free the fpq in such a case this function must be used because the
fpq has no ability to free itself. Just as the allocation function is required
to reserve memory so to is it required to free memory.

This function will work normally if called on a fpq with allocation permission
however the normal CCC_fpq_clear_and_free is sufficient for that use case. */
CCC_result
CCC_fpq_clear_and_free_reserve(CCC_flat_priority_queue *fpq,
                               CCC_any_type_destructor_fn *destructor,
                               CCC_any_alloc_fn *alloc);

/**@}*/

/** @name State Interface
Obtain state from the container. */
/**@{*/

/** @brief Return a pointer to the front (min or max) element in the fpq. O(1).
@param [in] fpq a pointer to the priority queue.
@return A pointer to the front element or NULL if empty or fpq is NULL. */
[[nodiscard]] void *CCC_fpq_front(CCC_flat_priority_queue const *fpq);

/** @brief Returns true if the fpq is empty false if not. O(1).
@param [in] fpq a pointer to the flat priority queue.
@return true if the size is 0, false if not empty. Error if fpq is NULL. */
[[nodiscard]] CCC_tribool CCC_fpq_is_empty(CCC_flat_priority_queue const *fpq);

/** @brief Returns the count of the fpq active slots.
@param [in] fpq a pointer to the flat priority queue.
@return the size of the fpq or an argument error is set if fpq is NULL. */
[[nodiscard]] CCC_ucount CCC_fpq_count(CCC_flat_priority_queue const *fpq);

/** @brief Returns the capacity of the fpq representing total possible slots.
@param [in] fpq a pointer to the flat priority queue.
@return the capacity of the fpq or an argument error is set if fpq is NULL. */
[[nodiscard]] CCC_ucount CCC_fpq_capacity(CCC_flat_priority_queue const *fpq);

/** @brief Return a pointer to the base of the backing array. O(1).
@param [in] fpq a pointer to the priority queue.
@return A pointer to the base of the backing array or NULL if fpq is NULL.
@note this reference starts at index 0 of the backing array. All fpq elements
are stored contiguously starting at the base through size of the fpq.
@warning it is the users responsibility to ensure that access to any data is
within the capacity of the backing buffer. */
[[nodiscard]] void *CCC_fpq_data(CCC_flat_priority_queue const *fpq);

/** @brief Verifies the internal invariants of the fpq hold.
@param [in] fpq a pointer to the flat priority queue.
@return true if the fpq is valid false if invalid. Error if fpq is NULL. */
[[nodiscard]] CCC_tribool CCC_fpq_validate(CCC_flat_priority_queue const *fpq);

/** @brief Return the order used to initialize the fpq.
@param [in] fpq a pointer to the flat priority queue.
@return LES or GRT ordering. Any other ordering is invalid. */
[[nodiscard]] CCC_threeway_cmp
CCC_fpq_order(CCC_flat_priority_queue const *fpq);

/**@}*/

/** Define this preprocessor directive if shortened names are desired for the
flat priority queue container. Check for collisions before name shortening. */
#ifdef FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
typedef CCC_flat_priority_queue flat_priority_queue;
#    define fpq_init(args...) CCC_fpq_init(args)
#    define fpq_heapify_init(args...) CCC_fpq_heapify_init(args)
#    define fpq_copy(args...) CCC_fpq_copy(args)
#    define fpq_reserve(args...) CCC_fpq_reserve(args)
#    define fpq_heapify(args...) CCC_fpq_heapify(args)
#    define fpq_heapify_inplace(args...) CCC_fpq_heapify_inplace(args)
#    define fpq_heapsort(args...) CCC_fpq_heapsort(args)
#    define fpq_emplace(args...) CCC_fpq_emplace(args)
#    define fpq_push(args...) CCC_fpq_push(args)
#    define fpq_front(args...) CCC_fpq_front(args)
#    define fpq_pop(args...) CCC_fpq_pop(args)
#    define fpq_extract(args...) CCC_fpq_extract(args)
#    define fpq_update(args...) CCC_fpq_update(args)
#    define fpq_increase(args...) CCC_fpq_increase(args)
#    define fpq_decrease(args...) CCC_fpq_decrease(args)
#    define fpq_update_w(args...) CCC_fpq_update_w(args)
#    define fpq_increase_w(args...) CCC_fpq_increase_w(args)
#    define fpq_decrease_w(args...) CCC_fpq_decrease_w(args)
#    define fpq_clear(args...) CCC_fpq_clear(args)
#    define fpq_clear_and_free(args...) CCC_fpq_clear_and_free(args)
#    define fpq_clear_and_free_reserve(args...)                                \
        CCC_fpq_clear_and_free_reserve(args)
#    define fpq_is_empty(args...) CCC_fpq_is_empty(args)
#    define fpq_count(args...) CCC_fpq_count(args)
#    define fpq_data(args...) CCC_fpq_data(args)
#    define fpq_validate(args...) CCC_fpq_validate(args)
#    define fpq_order(args...) CCC_fpq_order(args)
#endif /* FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC */

#endif /* CCC_FLAT_PRIORITY_QUEUE_H */
