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
CCC_flat_priority_queue_pop(&priority_queue, &(int){0});
```

Any user defined struct can also use this technique.

```
CCC_flat_priority_queue_pop(&priority_queue, &(struct my_type){});
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
#include "private/private_flat_priority_queue.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A container offering direct storage and sorting of user data by heap
order.
@warning it is undefined behavior to access an uninitialized container.

A flat priority queue can be initialized on the stack, heap, or data segment at
runtime or compile time.*/
typedef struct CCC_Flat_priority_queue CCC_Flat_priority_queue;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a flat_priority_queue as a min or max heap.
@param[in] mem_pointer a pointer to an array of user types or NULL.
@param[in] type_name the name of the user type.
@param[in] order_order CCC_ORDER_LESSER or CCC_ORDER_GREATER for min or max
heap, respectively.
@param[in] order_fn the user defined comarison function for user types.
@param[in] allocate the allocation function or NULL if no allocation.
@param[in] context_data any context data needed for destruction of elements.
@param[in] capacity the capacity of contiguous elements at mem_pointer.
@return the initialized priority queue on the right hand side of an equality
operator. (i.e. CCC_Flat_priority_queue q =
CCC_flat_priority_queue_initialize(...);). */
#define CCC_flat_priority_queue_initialize(mem_pointer, type_name,             \
                                           order_order, order_fn, allocate,    \
                                           context_data, capacity)             \
    CCC_private_flat_priority_queue_initialize(                                \
        mem_pointer, type_name, order_order, order_fn, allocate, context_data, \
        capacity)

/** @brief Partial order an array of elements as a min or max heap. O(N).
@param[in] mem_pointer a pointer to an array of user types or NULL.
@param[in] type_name the name of the user type.
@param[in] order_order CCC_ORDER_LESSER or CCC_ORDER_GREATER for min or max
heap, respectively.
@param[in] order_fn the user defined comparison function for user types.
@param[in] allocate the allocation function or NULL if no allocation.
@param[in] context_data any context data needed for destruction of elements.
@param[in] capacity the capacity of contiguous elements at mem_pointer.
@param[in] size the size <= capacity.
@return the initialized priority queue on the right hand side of an equality
operator. (i.e. CCC_Flat_priority_queue q =
CCC_flat_priority_queue_heapify_initialize(...);).
*/
#define CCC_flat_priority_queue_heapify_initialize(                            \
    mem_pointer, type_name, order_order, order_fn, allocate, context_data,     \
    capacity, size)                                                            \
    CCC_private_flat_priority_queue_heapify_initialize(                        \
        mem_pointer, type_name, order_order, order_fn, allocate, context_data, \
        capacity, size)

/** @brief Copy the flat_priority_queue from src to newly initialized dst.
@param[in] dst the destination that will copy the source flat_priority_queue.
@param[in] src the source of the flat_priority_queue.
@param[in] fn the allocation function in case resizing of dst is needed.
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
Flat_priority_queue src
    = flat_priority_queue_initialize((int[10]){}, CCC_ORDER_LESSER, NULL,
int_order, NULL, 10); push_rand_ints(&src); Flat_priority_queue dst =
flat_priority_queue_initialize((int[11]){}, CCC_ORDER_LESSER, NULL, int_order,
NULL, 11); CCC_Result res = flat_priority_queue_copy(&dst, &src, NULL);
```

The above requires dst capacity be greater than or equal to src capacity. Here
is memory management handed over to the copy function.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
Flat_priority_queue src
    = flat_priority_queue_initialize((int *)NULL, CCC_ORDER_LESSER,
std_allocate, int_order, NULL, 0); push_rand_ints(&src); Flat_priority_queue dst
= flat_priority_queue_initialize((int *)NULL, CCC_ORDER_LESSER, std_allocate,
int_order, NULL, 0); CCC_Result res = flat_priority_queue_copy(&dst, &src,
std_allocate);
```

The above allows dst to have a capacity less than that of the src as long as
copy has been provided an allocation function to resize dst. Note that this
would still work if copying to a destination that the user wants as a fixed
size flat_priority_queue.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
Flat_priority_queue src
    = flat_priority_queue_initialize((int *)NULL, CCC_ORDER_LESSER,
std_allocate, int_order, NULL, 0); push_rand_ints(&src); Flat_priority_queue dst
= flat_priority_queue_initialize((int *)NULL, CCC_ORDER_LESSER, NULL, int_order,
NULL, 0); CCC_Result res = flat_priority_queue_copy(&dst, &src, std_allocate);
```

The above sets up dst with fixed size while src is a dynamic
flat_priority_queue. Because an allocation function is provided, the dst is
resized once for the copy and retains its fixed size after the copy is complete.
This would require the user to manually free the underlying Buffer at dst
eventually if this method is used. Usually it is better to allocate the memory
explicitly before the copy if copying between ring buffers.

These options allow users to stay consistent across containers with their
memory management strategies. */
CCC_Result CCC_flat_priority_queue_copy(CCC_Flat_priority_queue *dst,
                                        CCC_Flat_priority_queue const *src,
                                        CCC_Allocator *fn);

/** @brief Reserves space for at least to_add more elements.
@param[in] flat_priority_queue a pointer to the flat priority queue.
@param[in] to_add the number of elements to add to the current size.
@param[in] fn the allocation function to use to reserve memory.
@return the result of the reservation. OK if successful, otherwise an error
status is returned.
@note see the CCC_flat_priority_queue_clear_and_free_reserve function if this
function is being used for a one-time dynamic reservation.

This function can be used for a dynamic flat_priority_queue with or without
allocation permission. If the flat_priority_queue has allocation permission, it
will reserve the required space and later resize if more space is needed.

If the flat_priority_queue has been initialized with no allocation permission
and no memory this function can serve as a one-time reservation. This is helpful
when a fixed size is needed but that size is only known dynamically at runtime.
To free the flat_priority_queue in such a case see the
CCC_flat_priority_queue_clear_and_free_reserve function. */
CCC_Result
CCC_flat_priority_queue_reserve(CCC_Flat_priority_queue *flat_priority_queue,
                                size_t to_add, CCC_Allocator *fn);

/**@}*/

/** @name Insert and Remove Interface
Insert or remove elements from the flat priority queue. */
/**@{*/

/** @brief Write a type directly to a priority queue slot. O(lgN).
@param[in] flat_priority_queue a pointer to the priority queue.
@param[in] val_initializer the compound literal or direct scalar type.
@return a reference to the inserted element or NULL if allocation failed. */
#define CCC_flat_priority_queue_emplace(flat_priority_queue,                   \
                                        val_initializer...)                    \
    CCC_private_flat_priority_queue_emplace(flat_priority_queue,               \
                                            val_initializer)

/** @brief Copy input array into the flat_priority_queue, organizing into heap.
O(N).
@param[in] flat_priority_queue a pointer to the priority queue.
@param[in] tmp a pointer to an additional element of array type for swapping.
@param[in] input_array an array of elements of the same size as the type used
to initialize flat_priority_queue.
@param[in] input_n the number of contiguous elements at input_array.
@param[in] input_sizeof_type size of each element in input_array matching
element size of flat_priority_queue.
@return OK if sorting was successful or an input error if bad input is
provided. A permission error will occur if no allocation is allowed and the
input array is larger than the fixed flat_priority_queue capacity. A memory
error will occur if reallocation is required to fit all elements but
reallocation fails.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`.

Note that this version of heapify copies elements from the input array. If an
in place heapify is required use the initializer version of this method. */
CCC_Result
CCC_flat_priority_queue_heapify(CCC_Flat_priority_queue *flat_priority_queue,
                                void *tmp, void *input_array, size_t input_n,
                                size_t input_sizeof_type);

/** @brief Order n elements of the underlying flat_priority_queue Buffer as an
flat_priority_queue.
@param[in] flat_priority_queue a pointer to the flat priority queue.
@param[in] tmp a pointer to a dummy user type that will be used for swapping.
@param[in] n the number n of elements where  0 < (n + 1) <= capacity.
@return the result of the heapify operation, ok if successful or an error if
flat_priority_queue is NULL or n is larger than the initialized capacity of the
flat_priority_queue.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`.

This is another method to order a heap that already has all the elements one
needs sorted. The underlying Buffer will be interpreted to have n valid elements
starting at index 0 to index n - 1. */
CCC_Result CCC_flat_priority_queue_heapify_inplace(
    CCC_Flat_priority_queue *flat_priority_queue, void *tmp, size_t n);

/** @brief Pushes element pointed to at e into flat_priority_queue. O(lgN).
@param[in] flat_priority_queue a pointer to the priority queue.
@param[in] elem a pointer to the user element of same type as in
flat_priority_queue.
@param[in] tmp a pointer to a dummy user type that will be used for swapping.
@return a pointer to the inserted element or NULl if NULL args are provided or
push required more memory and failed. Failure can occur if the
flat_priority_queue is full and allocation is not allowed or a resize failed
when allocation is allowed.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`. */
[[nodiscard]] void *
CCC_flat_priority_queue_push(CCC_Flat_priority_queue *flat_priority_queue,
                             void const *elem, void *tmp);

/** @brief Pop the front element (min or max) element in the
flat_priority_queue. O(lgN).
@param[in] flat_priority_queue a pointer to the priority queue.
@param[in] tmp a pointer to a dummy user type that will be used for swapping.
@return OK if the pop succeeds or an input error if flat_priority_queue is NULL
or empty.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`. */
CCC_Result
CCC_flat_priority_queue_pop(CCC_Flat_priority_queue *flat_priority_queue,
                            void *tmp);

/** @brief Erase element e that is a handle to the stored flat_priority_queue
element.
@param[in] flat_priority_queue a pointer to the priority queue.
@param[in] elem a pointer to the stored flat_priority_queue element. Must be in
the flat_priority_queue.
@param[in] tmp a pointer to a dummy user type that will be used for swapping.
@return OK if the erase is successful or an input error if NULL args are
provided or the flat_priority_queue is empty.
@warning the user must ensure e is in the flat_priority_queue.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`.

Note that the reference to elem is invalidated after this call. */
CCC_Result
CCC_flat_priority_queue_erase(CCC_Flat_priority_queue *flat_priority_queue,
                              void *elem, void *tmp);

/** @brief Update e that is a handle to the stored flat_priority_queue element.
O(lgN).
@param[in] flat_priority_queue a pointer to the flat priority queue.
@param[in] elem a pointer to the stored flat_priority_queue element. Must be in
the flat_priority_queue.
@param[in] tmp a pointer to a dummy user type that will be used for swapping.
@param[in] fn the update function to act on e.
@param[in] context any context data needed for the update function.
@return a reference to the element at its new position in the
flat_priority_queue on success, NULL if parameters are invalid or
flat_priority_queue is empty.
@warning the user must ensure e is in the flat_priority_queue.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`. */
void *
CCC_flat_priority_queue_update(CCC_Flat_priority_queue *flat_priority_queue,
                               void *elem, void *tmp, CCC_Type_modifier *fn,
                               void *context);

/** @brief Update the user type stored in the priority queue directly. O(lgN).
@param[in] flat_priority_queue_pointer a pointer to the flat priority queue.
@param[in] type_pointer a pointer to the user type being updated.
@param[in] update_closure_over_T the semicolon separated statements to execute
on the user type at T (optionally wrapping {code here} in braces may help
with formatting). This closure may safely modify the key used to track the user
element's priority in the priority queue.
@return a reference to the element at its new position in the
flat_priority_queue on success, NULL if parameters are invalid or
flat_priority_queue is empty.
@warning the user must ensure type_pointer is in the flat_priority_queue.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
Flat_priority_queue flat_priority_queue = build_rand_int_flat_priority_queue();
(void)flat_priority_queue_update_with(&flat_priority_queue,
get_rand_flat_priority_queue_node(&flat_priority_queue), { *T = rand_key(); });
```

Note that whether the key increases or decreases does not affect runtime. */
#define CCC_flat_priority_queue_update_with(                                   \
    flat_priority_queue_pointer, type_pointer, update_closure_over_T...)       \
    CCC_private_flat_priority_queue_update_with(                               \
        flat_priority_queue_pointer, type_pointer, update_closure_over_T)

/** @brief Increase e that is a handle to the stored flat_priority_queue
element. O(lgN).
@param[in] flat_priority_queue a pointer to the flat priority queue.
@param[in] elem a pointer to the stored flat_priority_queue element. Must be in
the flat_priority_queue.
@param[in] tmp a pointer to a dummy user type that will be used for swapping.
@param[in] fn the update function to act on e.
@param[in] context any context data needed for the update function.
@return a reference to the element at its new position in the
flat_priority_queue on success, NULL if parameters are invalid or
flat_priority_queue is empty.
@warning the user must ensure e is in the flat_priority_queue.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`. */
void *
CCC_flat_priority_queue_increase(CCC_Flat_priority_queue *flat_priority_queue,
                                 void *elem, void *tmp, CCC_Type_modifier *fn,
                                 void *context);

/** @brief Increase the user type stored in the priority queue directly. O(lgN).
@param[in] flat_priority_queue_pointer a pointer to the flat priority queue.
@param[in] type_pointer a pointer to the user type being updated.
@param[in] increase_closure_over_T the semicolon separated statements to
execute on the user type at T (optionally wrapping {code here} in
braces may help with formatting). This closure may safely modify the key used to
track the user element's priority in the priority queue.
@return a reference to the element at its new position in the
flat_priority_queue on success, NULL if parameters are invalid or
flat_priority_queue is empty.
@warning the user must ensure type_pointer is in the flat_priority_queue.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
Flat_priority_queue flat_priority_queue = build_rand_int_flat_priority_queue();
(void)flat_priority_queue_increase_with(&flat_priority_queue,
get_rand_flat_priority_queue_node(&flat_priority_queue), { (*T)++; });
```

Note that if this priority queue is min or max, the runtime is the same. */
#define CCC_flat_priority_queue_increase_with(                                 \
    flat_priority_queue_pointer, type_pointer, increase_closure_over_T...)     \
    CCC_private_flat_priority_queue_increase_with(                             \
        flat_priority_queue_pointer, type_pointer, increase_closure_over_T)

/** @brief Decrease e that is a handle to the stored flat_priority_queue
element. O(lgN).
@param[in] flat_priority_queue a pointer to the flat priority queue.
@param[in] elem a pointer to the stored flat_priority_queue element. Must be in
the flat_priority_queue.
@param[in] tmp a pointer to a dummy user type that will be used for swapping.
@param[in] fn the update function to act on e.
@param[in] context any context data needed for the update function.
@return a reference to the element at its new position in the
flat_priority_queue on success, NULL if parameters are invalid or
flat_priority_queue is empty.
@warning the user must ensure e is in the flat_priority_queue.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`. */
void *
CCC_flat_priority_queue_decrease(CCC_Flat_priority_queue *flat_priority_queue,
                                 void *elem, void *tmp, CCC_Type_modifier *fn,
                                 void *context);

/** @brief Increase the user type stored in the priority queue directly. O(lgN).
@param[in] flat_priority_queue_pointer a pointer to the flat priority queue.
@param[in] type_pointer a pointer to the user type being updated.
@param[in] decrease_closure_over_T the semicolon separated statements to
execute on the user type at T (optionally wrapping {code here} in
braces may help with formatting). This closure may safely modify the key used to
track the user element's priority in the priority queue.
@return a reference to the element at its new position in the
flat_priority_queue on success, NULL if parameters are invalid or
flat_priority_queue is empty.
@warning the user must ensure type_pointer is in the flat_priority_queue.

```
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
Flat_priority_queue flat_priority_queue = build_rand_int_flat_priority_queue();
(void)flat_priority_queue_decrease_with(&flat_priority_queue,
get_rand_flat_priority_queue_node(&flat_priority_queue), { (*T)--; });
```

Note that if this priority queue is min or max, the runtime is the same. */
#define CCC_flat_priority_queue_decrease_with(                                 \
    flat_priority_queue_pointer, type_pointer, decrease_closure_over_T...)     \
    CCC_private_flat_priority_queue_decrease_with(                             \
        flat_priority_queue_pointer, type_pointer, decrease_closure_over_T)

/**@}*/

/** @name Deallocation Interface
Deallocate the container or destroy the heap invariants. */
/**@{*/

/** @brief Destroys the flat_priority_queue by sorting its data and returning
the underlying buffer. The data is sorted in `O(N * log(N))` time and `O(1)`
space.
@param[in] flat_priority_queue the flat_priority_queue to be sorted and
destroyed.
@param[in] tmp a pointer to a dummy user type that will be used for swapping.
@return a Buffer filled from the back to the front by the flat_priority_queue
order. If the flat_priority_queue is initialized CCC_ORDER_LESSER the returned
Buffer is sorted in non-increasing order from index [0, N). If the
flat_priority_queue is initialized CCC_ORDER_GREATER the buffer is sorted in
non-descending order from index [0, N). If flat_priority_queue is NULL, the
buffer is default initialized and unusable.
@warning all fields of the flat_priority_queue are cleared or otherwise default
initialized so the flat_priority_queue is unusable as a container after sorting.
This function assumes the flat_priority_queue has been previously initialized.
Therefore, if the returned Buffer value is not used the flat_priority_queue
memory is leaked.

A simple way to provide a temp for swapping is with an inline compound literal
reference provided directly to the function argument `&(name_of_type){}`.

The underlying memory storage source for the flat_priority_queue, a buffer, is
not moved or copied during the sort. If a copy of the sorted data is preferred
copy the data the data to another initialized flat_priority_queue with the
`CCC_flat_priority_queue_copy` function first then sort that copy.

The sort is not inherently stable and uses the provided comparison function to
the flat_priority_queue to order the elements. */
[[nodiscard]] CCC_Buffer
CCC_flat_priority_queue_heapsort(CCC_Flat_priority_queue *flat_priority_queue,
                                 void *tmp);

/** @brief Clears the flat_priority_queue calling fn on every element if
provided. O(1)-O(N).
@param[in] flat_priority_queue a pointer to the flat priority queue.
@param[in] fn the destructor function or NULL if not needed.
@return OK if input is valid and clear succeeds, otherwise input error.

Note that because the priority queue is flat there is no need to free
elements stored in the flat_priority_queue. However, the destructor is free to
manage cleanup in other parts of user code as needed upon destruction of each
element.

If the destructor is NULL, the function is O(1) and no attempt is made to
free capacity of the flat_priority_queue. */
CCC_Result
CCC_flat_priority_queue_clear(CCC_Flat_priority_queue *flat_priority_queue,
                              CCC_Type_destructor *fn);

/** @brief Clears the flat_priority_queue calling fn on every element if
provided and frees the underlying buffer. O(1)-O(N).
@param[in] flat_priority_queue a pointer to the flat priority queue.
@param[in] fn the destructor function or NULL if not needed.
@return OK if input is valid and clear succeeds, otherwise input error. If the
Buffer attempts to free but is not allowed a no allocate error is returned.

Note that because the priority queue is flat there is no need to free elements
stored in the flat_priority_queue. However, the destructor is free to manage
cleanup in other parts of user code as needed upon destruction of each element.

If the destructor is NULL, the function is O(1) and only relies on the runtime
of the provided allocation function free operation. */
CCC_Result CCC_flat_priority_queue_clear_and_free(
    CCC_Flat_priority_queue *flat_priority_queue, CCC_Type_destructor *fn);

/** @brief Frees all slots in the flat_priority_queue and frees the underlying
Buffer that was previously dynamically reserved with the reserve function.
@param[in] flat_priority_queue the flat_priority_queue to be cleared.
@param[in] destructor the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the flat_priority_queue before their
slots are dropped.
@param[in] allocate the required allocation function to provide to a
dynamically reserved flat_priority_queue. Any context data provided upon
initialization will be passed to the allocation function when called.
@return the result of free operation. OK if success, or an error status to
indicate the error.
@warning It is an error to call this function on a flat_priority_queue that was
not reserved with the provided CCC_Allocator. The flat_priority_queue must have
existing memory to free.

This function covers the edge case of reserving a dynamic capacity for a
flat_priority_queue at runtime but denying the flat_priority_queue allocation
permission to resize. This can help prevent a flat_priority_queue from growing
unbounded. The user in this case knows the flat_priority_queue does not have
allocation permission and therefore no further memory will be dedicated to the
flat_priority_queue.

However, to free the flat_priority_queue in such a case this function must be
used because the flat_priority_queue has no ability to free itself. Just as the
allocation function is required to reserve memory so to is it required to free
memory.

This function will work normally if called on a flat_priority_queue with
allocation permission however the normal CCC_flat_priority_queue_clear_and_free
is sufficient for that use case. */
CCC_Result CCC_flat_priority_queue_clear_and_free_reserve(
    CCC_Flat_priority_queue *flat_priority_queue,
    CCC_Type_destructor *destructor, CCC_Allocator *allocate);

/**@}*/

/** @name State Interface
Obtain state from the container. */
/**@{*/

/** @brief Return a pointer to the front (min or max) element in the
flat_priority_queue. O(1).
@param[in] flat_priority_queue a pointer to the priority queue.
@return A pointer to the front element or NULL if empty or flat_priority_queue
is NULL. */
[[nodiscard]] void *CCC_flat_priority_queue_front(
    CCC_Flat_priority_queue const *flat_priority_queue);

/** @brief Returns true if the flat_priority_queue is empty false if not. O(1).
@param[in] flat_priority_queue a pointer to the flat priority queue.
@return true if the size is 0, false if not empty. Error if flat_priority_queue
is NULL. */
[[nodiscard]] CCC_Tribool CCC_flat_priority_queue_is_empty(
    CCC_Flat_priority_queue const *flat_priority_queue);

/** @brief Returns the count of the flat_priority_queue active slots.
@param[in] flat_priority_queue a pointer to the flat priority queue.
@return the size of the flat_priority_queue or an argument error is set if
flat_priority_queue is NULL. */
[[nodiscard]] CCC_Count CCC_flat_priority_queue_count(
    CCC_Flat_priority_queue const *flat_priority_queue);

/** @brief Returns the capacity of the flat_priority_queue representing total
possible slots.
@param[in] flat_priority_queue a pointer to the flat priority queue.
@return the capacity of the flat_priority_queue or an argument error is set if
flat_priority_queue is NULL. */
[[nodiscard]] CCC_Count CCC_flat_priority_queue_capacity(
    CCC_Flat_priority_queue const *flat_priority_queue);

/** @brief Return a pointer to the base of the backing array. O(1).
@param[in] flat_priority_queue a pointer to the priority queue.
@return A pointer to the base of the backing array or NULL if
flat_priority_queue is NULL.
@note this reference starts at index 0 of the backing array. All
flat_priority_queue elements are stored contiguously starting at the base
through size of the flat_priority_queue.
@warning it is the users responsibility to ensure that access to any data is
within the capacity of the backing buffer. */
[[nodiscard]] void *CCC_flat_priority_queue_data(
    CCC_Flat_priority_queue const *flat_priority_queue);

/** @brief Verifies the internal invariants of the flat_priority_queue hold.
@param[in] flat_priority_queue a pointer to the flat priority queue.
@return true if the flat_priority_queue is valid false if invalid. Error if
flat_priority_queue is NULL. */
[[nodiscard]] CCC_Tribool CCC_flat_priority_queue_validate(
    CCC_Flat_priority_queue const *flat_priority_queue);

/** @brief Return the order used to initialize the flat_priority_queue.
@param[in] flat_priority_queue a pointer to the flat priority queue.
@return LES or GRT ordering. Any other ordering is invalid. */
[[nodiscard]] CCC_Order CCC_flat_priority_queue_order(
    CCC_Flat_priority_queue const *flat_priority_queue);

/**@}*/

/** Define this preprocessor directive if shortened names are desired for the
flat priority queue container. Check for collisions before name shortening. */
#ifdef FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
typedef CCC_Flat_priority_queue Flat_priority_queue;
#    define flat_priority_queue_initialize(args...)                            \
        CCC_flat_priority_queue_initialize(args)
#    define flat_priority_queue_heapify_initialize(args...)                    \
        CCC_flat_priority_queue_heapify_initialize(args)
#    define flat_priority_queue_copy(args...) CCC_flat_priority_queue_copy(args)
#    define flat_priority_queue_reserve(args...)                               \
        CCC_flat_priority_queue_reserve(args)
#    define flat_priority_queue_heapify(args...)                               \
        CCC_flat_priority_queue_heapify(args)
#    define flat_priority_queue_heapify_inplace(args...)                       \
        CCC_flat_priority_queue_heapify_inplace(args)
#    define flat_priority_queue_heapsort(args...)                              \
        CCC_flat_priority_queue_heapsort(args)
#    define flat_priority_queue_emplace(args...)                               \
        CCC_flat_priority_queue_emplace(args)
#    define flat_priority_queue_push(args...) CCC_flat_priority_queue_push(args)
#    define flat_priority_queue_front(args...)                                 \
        CCC_flat_priority_queue_front(args)
#    define flat_priority_queue_pop(args...) CCC_flat_priority_queue_pop(args)
#    define flat_priority_queue_extract(args...)                               \
        CCC_flat_priority_queue_extract(args)
#    define flat_priority_queue_update(args...)                                \
        CCC_flat_priority_queue_update(args)
#    define flat_priority_queue_increase(args...)                              \
        CCC_flat_priority_queue_increase(args)
#    define flat_priority_queue_decrease(args...)                              \
        CCC_flat_priority_queue_decrease(args)
#    define flat_priority_queue_update_with(args...)                           \
        CCC_flat_priority_queue_update_with(args)
#    define flat_priority_queue_increase_with(args...)                         \
        CCC_flat_priority_queue_increase_with(args)
#    define flat_priority_queue_decrease_with(args...)                         \
        CCC_flat_priority_queue_decrease_with(args)
#    define flat_priority_queue_clear(args...)                                 \
        CCC_flat_priority_queue_clear(args)
#    define flat_priority_queue_clear_and_free(args...)                        \
        CCC_flat_priority_queue_clear_and_free(args)
#    define flat_priority_queue_clear_and_free_reserve(args...)                \
        CCC_flat_priority_queue_clear_and_free_reserve(args)
#    define flat_priority_queue_is_empty(args...)                              \
        CCC_flat_priority_queue_is_empty(args)
#    define flat_priority_queue_count(args...)                                 \
        CCC_flat_priority_queue_count(args)
#    define flat_priority_queue_data(args...) CCC_flat_priority_queue_data(args)
#    define flat_priority_queue_validate(args...)                              \
        CCC_flat_priority_queue_validate(args)
#    define flat_priority_queue_order(args...)                                 \
        CCC_flat_priority_queue_order(args)
#endif /* FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC */

#endif /* CCC_FLAT_PRIORITY_QUEUE_H */
