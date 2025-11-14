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
@brief The Flat Double Ended Queue Interface

An FDEQ offers contiguous storage and random access, push, and pop in constant
time. The contiguous nature of the Buffer makes it well-suited to dynamic or
fixed size contexts where a double ended queue is needed.

If the container is initialized with allocation permission it will resize when
needed but support constant time push and pop to the front and back when
resizing is not required, resulting in amortized O(1) operations.

If the FDEQ is initialized without allocation permission its behavior is
equivalent to a Ring Buffer. This is somewhat unique in that it does not fail
to insert elements when size is equal to capacity. This means that push front,
push back, pop front, and pop back are O(1) operations. However, if any push
exceeds capacity an element where the push should occur is overwritten.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `CCC_` prefix. */
#ifndef CCC_FLAT_DOUBLE_ENDED_QUEUE_H
#define CCC_FLAT_DOUBLE_ENDED_QUEUE_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "private/private_flat_double_ended_queue.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A contiguous Buffer for O(1) push and pop from front and back.
@warning it is undefined behavior to use an uninitialized flat double ended
queue.

A flat double ended queue can be initialized on the stack, heap, or data
segment at compile time or runtime. */
typedef struct CCC_Flat_double_ended_queue CCC_Flat_double_ended_queue;

/**@}*/

/** @name Initialization Interface
Initialize and create containers with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize the flat_double_ended_queue with memory and allocation
permission.
@param [in] mem_pointer a pointer to existing memory or ((T *)NULL).
@param [in] any_type_name the name of the user type.
@param [in] allocate the allocator function, if allocation is allowed.
@param [in] context_data any context data needed for element destruction.
@param [in] capacity the number of contiguous elements at mem_pointer
@param [in] optional_size an optional initial size between 1 and capacity.
@return the flat_double_ended_queue on the right hand side of an equality
operator at runtime or compiletime (e.g. CCC_Flat_double_ended_queue q =
CCC_flat_double_ended_queue_initialize(...);) */
#define CCC_flat_double_ended_queue_initialize(mem_pointer, any_type_name,     \
                                               allocate, context_data,         \
                                               capacity, optional_size...)     \
    CCC_private_flat_double_ended_queue_initialize(mem_pointer, any_type_name, \
                                                   allocate, context_data,     \
                                                   capacity, optional_size)

/** @brief Copy the flat_double_ended_queue from src to newly initialized dst.
@param [in] dst the destination that will copy the source
flat_double_ended_queue.
@param [in] src the source of the flat_double_ended_queue.
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
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
flat_double_ended_queue src = flat_double_ended_queue_initialize((int[10]){},
int, NULL, NULL, 10); int *new_mem = malloc(sizeof(int) *
flat_double_ended_queue_capacity(&src).count); flat_double_ended_queue dst =
flat_double_ended_queue_initialize(new_mem, int, NULL, NULL,
flat_double_ended_queue_capacity(&src).count); CCC_Result res =
flat_double_ended_queue_copy(&dst, &src, NULL);
```

The above requires dst capacity be greater than or equal to src capacity. Here
is memory management handed over to the copy function.

```
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
flat_double_ended_queue src = flat_double_ended_queue_initialize(NULL, int,
std_allocate, NULL, 0); (void)CCC_flat_double_ended_queue_push_back_range(&src,
5, (int[5]){0,1,2,3,4}); flat_double_ended_queue dst =
flat_double_ended_queue_initialize(NULL, int, std_allocate, NULL, 0); CCC_Result
res = flat_double_ended_queue_copy(&dst, &src, std_allocate);
```

The above allows dst to have a capacity less than that of the src as long as
copy has been provided an allocation function to resize dst. Note that this
would still work if copying to a destination that the user wants as a fixed
size flat_double_ended_queue (ring buffer).

```
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
flat_double_ended_queue src = flat_double_ended_queue_initialize(NULL, int,
std_allocate, NULL, 0); (void)CCC_flat_double_ended_queue_push_back_range(&src,
5, (int[5]){0,1,2,3,4}); flat_double_ended_queue dst =
flat_double_ended_queue_initialize(NULL, int, NULL, NULL, 0); CCC_Result res =
flat_double_ended_queue_copy(&dst, &src, std_allocate);
```

The above sets up dst as a ring Buffer while src is a dynamic
flat_double_ended_queue. Because an allocation function is provided, the dst is
resized once for the copy and retains its fixed size after the copy is complete.
This would require the user to manually free the underlying Buffer at dst
eventually if this method is used. Usually it is better to allocate the memory
explicitly before the copy if copying between ring buffers.

These options allow users to stay consistent across containers with their
memory management strategies. */
CCC_Result
CCC_flat_double_ended_queue_copy(CCC_Flat_double_ended_queue *dst,
                                 CCC_Flat_double_ended_queue const *src,
                                 CCC_Allocator *fn);

/** @brief Reserves space for at least to_add more elements.
@param [in] flat_double_ended_queue a pointer to the flat double ended queue.
@param [in] to_add the number of elements to add to the current size.
@param [in] fn the allocation function to use to reserve memory.
@return the result of the reservation. OK if successful, otherwise an error
status is returned.
@note see the CCC_flat_double_ended_queue_clear_and_free_reserve function if
this function is being used for a one-time dynamic reservation.

This function can be used for a dynamic flat_double_ended_queue with or without
allocation permission. If the flat_double_ended_queue has allocation permission,
it will reserve the required space and later resize if more space is needed.

If the flat_double_ended_queue has been initialized with no allocation
permission and no memory this function can serve as a one-time reservation. The
flat_double_ended_queue will then act as as ring Buffer when space runs out.
This is helpful when a fixed size is needed but that size is only known
dynamically at runtime. To free the flat_double_ended_queue in such a case see
the CCC_flat_double_ended_queue_clear_and_free_reserve function. */
CCC_Result CCC_flat_double_ended_queue_reserve(
    CCC_Flat_double_ended_queue *flat_double_ended_queue, size_t to_add,
    CCC_Allocator *fn);

/**@}*/

/** @name Insert and Remove Interface
Add or remove elements from the FDEQ. */
/**@{*/

/** @brief Write an element directly to the back slot of the
flat_double_ended_queue. O(1) if no allocation permission amortized O(1) if
allocation permission is given and a resize is required.
@param [in] flat_double_ended_queue_pointer a pointer to the
flat_double_ended_queue.
@param [in] value for integral types, the direct value. For structs and
unions use compound literal syntax.
@return a reference to the inserted element. If allocation is permitted and a
resizing is required to insert the element but fails, NULL is returned. */
#define CCC_flat_double_ended_queue_emplace_back(                              \
    flat_double_ended_queue_pointer, value...)                                 \
    CCC_private_flat_double_ended_queue_emplace_back(                          \
        flat_double_ended_queue_pointer, value)

/** @brief Write an element directly to the front slot of the
flat_double_ended_queue. O(1) if no allocation permission amortized O(1) if
allocation permission is given and a resize is required.
@param [in] flat_double_ended_queue_pointer a pointer to the
flat_double_ended_queue.
@param [in] value for integral types, the direct value. For structs and
unions use compound literal syntax.
@return a reference to the inserted element. If allocation is permitted and a
resizing is required to insert the element but fails, NULL is returned. */
#define CCC_flat_double_ended_queue_emplace_front(                             \
    flat_double_ended_queue_pointer, value...)                                 \
    CCC_private_flat_double_ended_queue_emplace_front(                         \
        flat_double_ended_queue_pointer, value)

/** @brief Push the user type to the back of the flat_double_ended_queue. O(1)
if no allocation permission amortized O(1) if allocation permission is given and
a resize is required.
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@param [in] elem a pointer to the user type to insert into the
flat_double_ended_queue.
@return a reference to the inserted element. */
[[nodiscard]] void *CCC_flat_double_ended_queue_push_back(
    CCC_Flat_double_ended_queue *flat_double_ended_queue, void const *elem);

/** @brief Push the range of user types to the back of the
flat_double_ended_queue. O(N).
@param [in] flat_double_ended_queue pointer to the flat_double_ended_queue.
@param [in] n the number of user types in the elems range.
@param [in] elems a pointer to the array of user types.
@return ok if insertion was successful. If allocation is permitted and a resize
is needed but fails an error is returned. If bad input is provided an input
error is returned.

Note that if no allocation is permitted the flat_double_ended_queue behaves as a
ring buffer. Therefore, pushing a range that will exceed capacity will overwrite
elements at the beginning of the flat_double_ended_queue. */
CCC_Result CCC_flat_double_ended_queue_push_back_range(
    CCC_Flat_double_ended_queue *flat_double_ended_queue, size_t n,
    void const *elems);

/** @brief Push the user type to the front of the flat_double_ended_queue. O(1)
if no allocation permission amortized O(1) if allocation permission is given and
a resize is required.
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@param [in] elem a pointer to the user type to insert into the
flat_double_ended_queue.
@return a reference to the inserted element. */
[[nodiscard]] void *CCC_flat_double_ended_queue_push_front(
    CCC_Flat_double_ended_queue *flat_double_ended_queue, void const *elem);

/** @brief Push the range of user types to the front of the
flat_double_ended_queue. O(N).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@param [in] n the number of user types in the elems range.
@param [in] elems a pointer to the array of user types.
@return ok if insertion was successful. If allocation is permitted and a resize
is needed but fails an error is returned. If bad input is provided an input
error is returned.

Note that if no allocation is permitted the flat_double_ended_queue behaves as a
ring buffer. Therefore, pushing a range that will exceed capacity will overwrite
elements at the back of the flat_double_ended_queue. */
CCC_Result CCC_flat_double_ended_queue_push_front_range(
    CCC_Flat_double_ended_queue *flat_double_ended_queue, size_t n,
    void const *elems);

/** @brief Push the range of user types before pos of the
flat_double_ended_queue. O(N).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@param [in] pos the position in the flat_double_ended_queue before which to push
the range.
@param [in] n the number of user types in the elems range.
@param [in] elems a pointer to the array of user types.
@return a pointer to the start of the inserted range or NULL if a resize was
required and could not complete.

Note that if no allocation is permitted the flat_double_ended_queue behaves as a
ring buffer. Therefore, pushing a range that will exceed capacity will overwrite
elements at the start of the flat_double_ended_queue.

Pushing a range of elements prioritizes the range and allows the range to
overwrite elements instead of pushing those elements over the start of the
range. For example, push a range `{3,4,5}` over a flat_double_ended_queue with
capacity 5 before pos with value 6.

```
 front pos        front
┌─┬┴┬─┬┴┬─┐    ┌─┬─┬┴┬─┬─┐
│ │1│2│6│ │ -> │5│6│2│3│4│
└─┴─┴─┴─┴─┘    └─┴─┴─┴─┴─┘
```

Notice that 1 and 2 were NOT moved to overwrite the start of the range, the
values 3 and 4. The only way the start of a range will be overwritten is if
the range itself is too large for the capacity. For example, push a range
`{0,0,3,3,4,4,5,5}` over the same flat_double_ended_queue.

```
 front pos    front
┌─┬┴┬─┬┴┬─┐    ┌┴┬─┬─┬─┬─┐
│ │1│2│6│ │ -> │3│4│4│5│5│
└─┴─┴─┴─┴─┘    └─┴─┴─┴─┴─┘
```

Notice that the start of the range, `{0,0,3,...}`, is overwritten. */
[[nodiscard]] void *CCC_flat_double_ended_queue_insert_range(
    CCC_Flat_double_ended_queue *flat_double_ended_queue, void *pos, size_t n,
    void const *elems);

/** @brief Pop an element from the front of the flat_double_ended_queue. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return ok if the pop was successful. If flat_double_ended_queue is NULL or the
flat_double_ended_queue is empty an input error is returned. */
CCC_Result CCC_flat_double_ended_queue_pop_front(
    CCC_Flat_double_ended_queue *flat_double_ended_queue);

/** @brief Pop an element from the back of the flat_double_ended_queue. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return ok if the pop was successful. If flat_double_ended_queue is NULL or the
flat_double_ended_queue is empty an input error is returned. */
CCC_Result CCC_flat_double_ended_queue_pop_back(
    CCC_Flat_double_ended_queue *flat_double_ended_queue);

/**@}*/

/** @name Deallocation Interface
Destroy the container. */
/**@{*/

/** @brief Set size of flat_double_ended_queue to 0 and call destructor on each
element if needed. O(1) if no destructor is provided, else O(N).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@param [in] destructor the destructor if needed or NULL.

Note that if destructor is non-NULL it will be called on each element in the
flat_double_ended_queue. However, the underlying Buffer for the
flat_double_ended_queue is not freed. If the destructor is NULL, setting the
size to 0 is O(1). */
CCC_Result CCC_flat_double_ended_queue_clear(
    CCC_Flat_double_ended_queue *flat_double_ended_queue,
    CCC_Type_destructor *destructor);

/** @brief Set size of flat_double_ended_queue to 0 and call destructor on each
element if needed. Free the underlying Buffer setting the capacity to 0. O(1) if
no destructor is provided, else O(N).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@param [in] destructor the destructor if needed or NULL.

Note that if destructor is non-NULL it will be called on each element in the
flat_double_ended_queue. After all elements are processed the Buffer is freed
and capacity is 0. If destructor is NULL the Buffer is freed directly and
capacity is 0. */
CCC_Result CCC_flat_double_ended_queue_clear_and_free(
    CCC_Flat_double_ended_queue *flat_double_ended_queue,
    CCC_Type_destructor *destructor);

/** @brief Frees all slots in the flat_double_ended_queue and frees the
underlying Buffer that was previously dynamically reserved with the reserve
function.
@param [in] flat_double_ended_queue the flat_double_ended_queue to be cleared.
@param [in] destructor the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the flat_double_ended_queue before
their slots are dropped.
@param [in] allocate the required allocation function to provide to a
dynamically reserved flat_double_ended_queue. Any context data provided upon
initialization will be passed to the allocation function when called.
@return the result of free operation. OK if success, or an error status to
indicate the error.
@warning It is an error to call this function on a flat_double_ended_queue that
was not reserved with the provided CCC_Allocator. The flat_double_ended_queue
must have existing memory to free.

This function covers the edge case of reserving a dynamic capacity for a
flat_double_ended_queue at runtime but denying the flat_double_ended_queue
allocation permission to resize. This can help prevent a flat_double_ended_queue
from growing unbounded. The user in this case knows the flat_double_ended_queue
does not have allocation permission and therefore no further memory will be
dedicated to the flat_double_ended_queue.

However, to free the flat_double_ended_queue in such a case this function must
be used because the flat_double_ended_queue has no ability to free itself. Just
as the allocation function is required to reserve memory so to is it required to
free memory.

This function will work normally if called on a flat_double_ended_queue with
allocation permission however the normal
CCC_flat_double_ended_queue_clear_and_free is sufficient for that use case. */
CCC_Result CCC_flat_double_ended_queue_clear_and_free_reserve(
    CCC_Flat_double_ended_queue *flat_double_ended_queue,
    CCC_Type_destructor *destructor, CCC_Allocator *allocate);

/**@}*/

/** @name State Interface
Interact with the state of the FDEQ. */
/**@{*/

/** @brief Return a pointer to the front element of the flat_double_ended_queue.
O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return a pointer to the user type at the start of the flat_double_ended_queue.
NULL if empty. */
[[nodiscard]] void *CCC_flat_double_ended_queue_begin(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue);

/** @brief Return a pointer to the back element of the flat_double_ended_queue.
O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return a pointer to the user type at the back of the flat_double_ended_queue.
NULL if empty. */
[[nodiscard]] void *CCC_flat_double_ended_queue_reverse_begin(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue);

/** @brief Return the next element in the flat_double_ended_queue moving front
to back. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@param [in] iter_pointer the current element in the flat_double_ended_queue.
@return the element following iter_pointer or NULL if no elements follow. */
[[nodiscard]] void *CCC_flat_double_ended_queue_next(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue,
    void const *iter_pointer);

/** @brief Return the next element in the flat_double_ended_queue moving back to
front. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@param [in] iter_pointer the current element in the flat_double_ended_queue.
@return the element preceding iter_pointer or NULL if no elements follow. */
[[nodiscard]] void *CCC_flat_double_ended_queue_reverse_next(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue,
    void const *iter_pointer);

/** @brief Return a pointer to the end element. It may not be accessed. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return a pointer to the end sentinel element that may not be accessed. */
[[nodiscard]] void *CCC_flat_double_ended_queue_end(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue);

/** @brief Return a pointer to the start element. It may not be accessed. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return a pointer to the start sentinel element that may not be accessed. */
[[nodiscard]] void *CCC_flat_double_ended_queue_reverse_end(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue);

/**@}*/

/** @name State Interface
Interact with the state of the FDEQ. */
/**@{*/

/** @brief Return a reference to the element at index position i. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@param [in] i the 0 based index in the flat_double_ended_queue.
@return a reference to the element at i if 0 <= i < capacity.

Note that the front of the flat_double_ended_queue is considered index 0, so the
user need not worry about where the front is for indexing purposes. */
[[nodiscard]] void *CCC_flat_double_ended_queue_at(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue, size_t i);

/** @brief Return a reference to the front of the flat_double_ended_queue. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return a reference to the front element or NULL if flat_double_ended_queue is
NULL or the flat_double_ended_queue is empty. */
[[nodiscard]] void *CCC_flat_double_ended_queue_front(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue);

/** @brief Return a reference to the back of the flat_double_ended_queue. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return a reference to the back element or NULL if flat_double_ended_queue is
NULL or the flat_double_ended_queue is empty. */
[[nodiscard]] void *CCC_flat_double_ended_queue_back(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue);

/** @brief Return true if the size of the flat_double_ended_queue is 0. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return true if the size is 0 or false. Error if flat_double_ended_queue is
NULL. */
[[nodiscard]] CCC_Tribool CCC_flat_double_ended_queue_is_empty(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue);

/** @brief Return the count of active flat_double_ended_queue slots. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return the size of the flat_double_ended_queue or an argument error is set if
flat_double_ended_queue is NULL. */
[[nodiscard]] CCC_Count CCC_flat_double_ended_queue_count(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue);

/** @brief Return the capacity representing total possible slots. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return the capacity of the flat_double_ended_queue or an argument error is set
if flat_double_ended_queue is NULL. */
[[nodiscard]] CCC_Count CCC_flat_double_ended_queue_capacity(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue);

/** @brief Return a reference to the base of backing array. O(1).
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return a reference to the base of the backing array.
@note the reference is to the base of the backing array at index 0 with no
consideration to where the front index of the flat_double_ended_queue may be.
@warning it is the users responsibility to ensure that access to any data is
within the capacity of the backing buffer.

This method is exposed for serialization or writing purposes but the base of
the array may not point to valid data in terms of organization of the
flat_double_ended_queue. */
[[nodiscard]] void *CCC_flat_double_ended_queue_data(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue);

/** @brief Return true if the internal invariants of the
flat_double_ended_queue.
@param [in] flat_double_ended_queue a pointer to the flat_double_ended_queue.
@return true if the internal invariants of the flat_double_ended_queue are held,
else false. Error if flat_double_ended_queue is NULL. */
[[nodiscard]] CCC_Tribool CCC_flat_double_ended_queue_validate(
    CCC_Flat_double_ended_queue const *flat_double_ended_queue);

/**@}*/

/** Define this preprocessor directive if you wish to use shorter names for the
flat_double_ended_queue container. Ensure no namespace collisions occur before
name shortening. */
#ifdef FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
typedef CCC_Flat_double_ended_queue Flat_double_ended_queue;
#    define flat_double_ended_queue_initialize(args...)                        \
        CCC_flat_double_ended_queue_initialize(args)
#    define flat_double_ended_queue_copy(args...)                              \
        CCC_flat_double_ended_queue_copy(args)
#    define flat_double_ended_queue_reserve(args...)                           \
        CCC_flat_double_ended_queue_reserve(args)
#    define flat_double_ended_queue_emplace(args...)                           \
        CCC_flat_double_ended_queue_emplace(args)
#    define flat_double_ended_queue_push_back(args...)                         \
        CCC_flat_double_ended_queue_push_back(args)
#    define flat_double_ended_queue_push_back_range(args...)                   \
        CCC_flat_double_ended_queue_push_back_range(args)
#    define flat_double_ended_queue_push_front(args...)                        \
        CCC_flat_double_ended_queue_push_front(args)
#    define flat_double_ended_queue_push_front_range(args...)                  \
        CCC_flat_double_ended_queue_push_front_range(args)
#    define flat_double_ended_queue_insert_range(args...)                      \
        CCC_flat_double_ended_queue_insert_range(args)
#    define flat_double_ended_queue_pop_front(args...)                         \
        CCC_flat_double_ended_queue_pop_front(args)
#    define flat_double_ended_queue_pop_back(args...)                          \
        CCC_flat_double_ended_queue_pop_back(args)
#    define flat_double_ended_queue_front(args...)                             \
        CCC_flat_double_ended_queue_front(args)
#    define flat_double_ended_queue_back(args...)                              \
        CCC_flat_double_ended_queue_back(args)
#    define flat_double_ended_queue_is_empty(args...)                          \
        CCC_flat_double_ended_queue_is_empty(args)
#    define flat_double_ended_queue_count(args...)                             \
        CCC_flat_double_ended_queue_count(args)
#    define flat_double_ended_queue_clear(args...)                             \
        CCC_flat_double_ended_queue_clear(args)
#    define flat_double_ended_queue_clear_and_free(args...)                    \
        CCC_flat_double_ended_queue_clear_and_free(args)
#    define flat_double_ended_queue_clear_and_free_reserve(args...)            \
        CCC_flat_double_ended_queue_clear_and_free_reserve(args)
#    define flat_double_ended_queue_at(args...)                                \
        CCC_flat_double_ended_queue_at(args)
#    define flat_double_ended_queue_data(args...)                              \
        CCC_flat_double_ended_queue_data(args)
#    define flat_double_ended_queue_begin(args...)                             \
        CCC_flat_double_ended_queue_begin(args)
#    define flat_double_ended_queue_reverse_begin(args...)                     \
        CCC_flat_double_ended_queue_reverse_begin(args)
#    define flat_double_ended_queue_next(args...)                              \
        CCC_flat_double_ended_queue_next(args)
#    define flat_double_ended_queue_reverse_next(args...)                      \
        CCC_flat_double_ended_queue_reverse_next(args)
#    define flat_double_ended_queue_end(args...)                               \
        CCC_flat_double_ended_queue_end(args)
#    define flat_double_ended_queue_reverse_end(args...)                       \
        CCC_flat_double_ended_queue_reverse_end(args)
#    define flat_double_ended_queue_validate(args...)                          \
        CCC_flat_double_ended_queue_validate(args)
#endif /* FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC */

#endif /* CCC_FLAT_DOUBLE_ENDED_QUEUE_H */
