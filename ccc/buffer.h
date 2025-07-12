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
@brief The Buffer Interface

Buffer usage is similar to a C++ vector, with more flexible functions
provided to support higher level containers and abstractions. While useful
on its own--a stack could be implemented with the provided functions--a buffer
is often used as the lower level abstraction for the flat data structures
in this library that provide more specialized operations. A buffer does not
require the user accommodate any intrusive elements.

A buffer with allocation permission will re-size as required when a new element
is inserted in a contiguous fashion. Interface functions in the allocation
management section assume elements are stored contiguously and adjust size
accordingly.

Interface functions in the slot management section offer data movement and
writing operations that do not affect the size of the container. If writing a
more complex higher level container that does not need size management these
functions offer more custom control over the buffer.

If allocation is not permitted, resizing will not occur and the insertion
function will fail when capacity is reached, returning some value to indicate
failure.

If shorter names are desired, define the following preprocessor directive.

```
#define BUFFER_USING_NAMESPACE_CCC
```

Then, the `ccc_` prefix can be dropped from all types and functions. */
#ifndef CCC_BUFFER_H
#define CCC_BUFFER_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "impl/impl_buffer.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A contiguous block of storage for elements of the same type.
@warning it is undefined behavior to use an uninitialized buffer.

A buffer may be initialized on the stack, heap, or data segment at compile time
or runtime. */
typedef struct ccc_buffer ccc_buffer;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a contiguous buffer of user a specified type, allocation
policy, capacity, and optional starting size.
@param [in] mem_ptr the pointer to existing memory or NULL.
@param [in] any_type_name the name of the user type in the buffer.
@param [in] alloc_fn ccc_any_alloc_fn or NULL if no allocation is permitted.
@param [in] aux_data any auxiliary data needed for managing buffer memory.
@param [in] capacity the capacity of memory at mem_ptr.
@param [in] optional_size optional starting size of the buffer <= capacity.
@return the initialized buffer. Directly assign to buffer on the right hand
side of the equality operator (e.g. ccc_buffer b = ccc_buf_init(...);).

Initialization of a buffer can occur at compile time or run time depending
on the arguments. The memory pointer should be of the same type one intends to
store in the buffer.

This initializer determines memory control for the lifetime of the buffer. If
the buffer points to memory of a predetermined and fixed capacity do not
provide an allocation function. If a dynamic buffer is preferred, provide the
allocation function as defined by the signature in types.h. If resizing is
desired on memory that has already been allocated, ensure allocation has
occurred with the provided allocation function. */
#define ccc_buf_init(mem_ptr, any_type_name, alloc_fn, aux_data, capacity,     \
                     optional_size...)                                         \
    ccc_impl_buf_init(mem_ptr, any_type_name, alloc_fn, aux_data, capacity,    \
                      optional_size)

/** @brief Reserves space for at least to_add more elements.
@param [in] buf a pointer to the buffer.
@param [in] to_add the number of elements to add to the current size.
@param [in] fn the allocation function to use to reserve memory.
@return the result of the reservation. OK if successful, otherwise an error
status is returned.
@note see the ccc_buf_clear_and_free_reserve function if this function is
being used for a one-time dynamic reservation.

This function can be used for a dynamic buf with or without allocation
permission. If the buf has allocation permission, it will reserve the required
space and later resize if more space is needed.

If the buf has been initialized with no allocation permission and no memory
this function can serve as a one-time reservation. To free the buf in such a
case see the ccc_buf_clear_and_free_reserve function. */
[[nodiscard]] ccc_result ccc_buf_reserve(ccc_buffer *buf, size_t to_add,
                                         ccc_any_alloc_fn *fn);

/** @brief Copy the buf from src to newly initialized dst.
@param [in] dst the destination that will copy the source buf.
@param [in] src the source of the buf.
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
#define BUFFER_USING_NAMESPACE_CCC
buffer src = buf_init((int[10]){}, int, NULL, NULL, 10);
int *new_mem = malloc(sizeof(int) * buf_capacity(&src).count);
buffer dst
    = buf_init(new_mem, int, NULL, NULL, buf_capacity(&src).count);
ccc_result res = buf_copy(&dst, &src, NULL);
```

The above requires dst capacity be greater than or equal to src capacity. Here
is memory management handed over to the copy function.

```
#define BUFFER_USING_NAMESPACE_CCC
buffer src = buf_init(NULL, int, std_alloc, NULL, 0);
(void)ccc_buf_push_back_range(&src, 5, (int[5]){0,1,2,3,4});
buffer dst = buf_init(NULL, int, std_alloc, NULL, 0);
ccc_result res = buf_copy(&dst, &src, std_alloc);
```

The above allows dst to have a capacity less than that of the src as long as
copy has been provided an allocation function to resize dst. Note that this
would still work if copying to a destination that the user wants as a fixed
size buf (ring buffer).

```
#define BUFFER_USING_NAMESPACE_CCC
buffer src = buf_init(NULL, int, std_alloc, NULL, 0);
(void)ccc_buf_push_back_range(&src, 5, (int[5]){0,1,2,3,4});
buffer dst = buf_init(NULL, int, NULL, NULL, 0);
ccc_result res = buf_copy(&dst, &src, std_alloc);
```

Because an allocation function is provided, the dst is resized once for the copy
and retains its fixed size after the copy is complete. This would require the
user to manually free the underlying buffer at dst eventually if this method is
used. Usually it is better to allocate the memory explicitly before the copy if
copying between ring buffers.

These options allow users to stay consistent across containers with their
memory management strategies. */
[[nodiscard]] ccc_result ccc_buf_copy(ccc_buffer *dst, ccc_buffer const *src,
                                      ccc_any_alloc_fn *fn);

/**@}*/

/** @name Insert and Remove Interface
These functions assume contiguity of elements in the buffer and increase or
decrease size accordingly. */
/**@{*/

/** @brief allocates the buffer to the specified size according to the user
defined allocation function.
@param [in] buf a pointer to the buffer.
@param [in] capacity the newly desired capacity.
@param [in] fn the allocation function defined by the user.
@return the result of reallocation.

This function takes the allocation function as an argument in case no
allocation function has been provided upon initialization and the user is
managing allocations and resizing directly. If an allocation function has
been provided than the use of this function should be rare as the buffer
will reallocate more memory when necessary. */
[[nodiscard]] ccc_result ccc_buf_alloc(ccc_buffer *buf, size_t capacity,
                                       ccc_any_alloc_fn *fn);

/** @brief allocates a new slot from the buffer at the end of the contiguous
array. A slot is equivalent to one of the element type specified when the
buffer is initialized.
@param [in] buf a pointer to the buffer.
@return a pointer to the newly allocated memory or NULL if no buffer is
provided or the buffer is unable to allocate more memory.
@note this function modifies the size of the container.

A buffer can be used as the backing for more complex data structures.
Requesting new space from a buffer as an allocator can be helpful for these
higher level organizations. */
[[nodiscard]] void *ccc_buf_alloc_back(ccc_buffer *buf);

/** @brief return the newly pushed data into the last slot of the buffer
according to size.
@param [in] buf the pointer to the buffer.
@param [in] data the pointer to the data of element size.
@return the pointer to the newly pushed element or NULL if no buffer exists or
resizing has failed due to memory exhuastion or no allocation allowed.
@note this function modifies the size of the container.

The data is copied into the buffer at the final slot if there is remaining
capacity. If size is equal to capacity resizing will be attempted but may
fail if no allocation function is provided or the allocator provided is
exhausted. */
[[nodiscard]] void *ccc_buf_push_back(ccc_buffer *buf, void const *data);

/** @brief Pushes the user provided compound literal directly to back of buffer
and increments the size to reflect the newly added element.
@param [in] buf_ptr a pointer to the buffer.
@param [in] type_compound_literal the direct compound literal as provided.
@return a pointer to the inserted element or NULL if insertion failed.

Any function calls that set fields of the compound literal will not be evaluated
if the buffer fails to allocate a slot at the back of the buffer. This may occur
if resizing fails or is prohibited. */
#define ccc_buf_emplace_back(buf_ptr, type_compound_literal...)                \
    ccc_impl_buf_emplace_back(buf_ptr, type_compound_literal)

/** @brief insert data at slot i according to size of the buffer maintaining
contiguous storage of elements between 0 and size.
@param [in] buf the pointer to the buffer.
@param [in] i the index at which to insert data.
@param [in] data the data copied into the buffer at index i of the same size
as elements stored in the buffer.
@return the pointer to the inserted element or NULL if bad input is provided,
the buffer is full and no resizing is allowed, or resizing fails when resizing
is allowed.
@note this function modifies the size of the container.

Note that this function assumes elements must be maintained contiguously
according to size of the buffer meaning a bulk move of elements sliding down
to accommodate i will occur. */
[[nodiscard]] void *ccc_buf_insert(ccc_buffer *buf, size_t i, void const *data);

/** @brief pop the back element from the buffer according to size.
@param [in] buf the pointer to the buffer.
@return the result of the attempted pop. CCC_RESULT_OK upon success or an input
error if bad input is provided.
@note this function modifies the size of the container. */
ccc_result ccc_buf_pop_back(ccc_buffer *buf);

/** @brief pop n elements from the back of the buffer according to size.
@param [in] buf the pointer to the buffer.
@param [in] n the number of elements to pop.
@return the result of the attempted pop. CCC_RESULT_OK if the buffer exists and
n is within the bounds of size. If the buffer does not exist an input error is
returned. If n is greater than the size of the buffer size is set to zero
and input error is returned.
@note this function modifies the size of the container. */
ccc_result ccc_buf_pop_back_n(ccc_buffer *buf, size_t n);

/** @brief erase element at slot i according to size of the buffer maintaining
contiguous storage of elements between 0 and size.
@param [in] buf the pointer to the buffer.
@param [in] i the index of the element to be erased.
@return the result, CCC_RESULT_OK if the input is valid. If no buffer exists or
i is out of range of size then an input error is returned.
@note this function modifies the size of the container.

Note that this function assumes elements must be maintained contiguously
according to size meaning a bulk copy of elements sliding down to fill the
space left by i will occur. */
ccc_result ccc_buf_erase(ccc_buffer *buf, size_t i);

/**@}*/

/** @name Slot Management Interface
These functions interact with slots in the buffer directly and do not modify
the size of the buffer. These are best used for custom container
implementations operating at a higher level of abstraction. */
/**@{*/

/** @brief return the element at slot i in buf.
@param [in] buf the pointer to the buffer.
@param [in] i the index within capacity range of the buffer.
@return a pointer to the element in the slot at position i or NULL if i is out
of capacity range.

Note that as long as the index is valid within the capacity of the buffer a
valid pointer is returned, which may result in a slot of old or uninitialized
data. It is up to the user to ensure the index provided is within the current
size of the buffer. */
[[nodiscard]] void *ccc_buf_at(ccc_buffer const *buf, size_t i);

/** @brief Access en element at the specified index as the stored type.
@param [in] buf_ptr the pointer to the buffer.
@param [in] type_name the name of the stored type.
@param [in] index the index within capacity range of the buffer.
@return a pointer to the element in the slot at position i or NULL if i is out
of capacity range.

Note that as long as the index is valid within the capacity of the buffer a
valid pointer is returned, which may result in a slot of old or uninitialized
data. It is up to the user to ensure the index provided is within the current
size of the buffer. */
#define ccc_buf_as(buf_ptr, type_name, index)                                  \
    ((type_name *)ccc_buf_at(buf_ptr, index))

/** @brief return the index of an element known to be in the buffer.
@param [in] buf the pointer to the buffer.
@param [in] slot the pointer to the element stored in the buffer.
@return the index if the slot provided is within the capacity range of the
buffer, otherwise an argument error is set. */
[[nodiscard]] ccc_ucount ccc_buf_i(ccc_buffer const *buf, void const *slot);

/** @brief return the final element in the buffer according the current size.
@param [in] buf the pointer to the buffer.
@return the pointer the final element in the buffer according to the current
size or NULL if the buffer does not exist or is empty. */
[[nodiscard]] void *ccc_buf_back(ccc_buffer const *buf);

/** @brief return the final element in the buffer according the current size.
@param [in] buf_ptr the pointer to the buffer.
@param [in] type_name the name of the stored type.
@return the pointer the final element in the buffer according to the current
size or NULL if the buffer does not exist or is empty. */
#define ccc_buf_back_as(buf_ptr, type_name) ((type_name *)ccc_buf_back(buf_ptr))

/** @brief return the first element in the buffer at index 0.
@param [in] buf the pointer to the buffer.
@return the pointer to the front element or NULL if the buffer does not exist
or is empty. */
[[nodiscard]] void *ccc_buf_front(ccc_buffer const *buf);

/** @brief return the first element in the buffer at index 0.
@param [in] buf_ptr the pointer to the buffer.
@param [in] type_name the name of the stored type.
@return the pointer to the front element or NULL if the buffer does not exist
or is empty. */
#define ccc_buf_front_as(buf_ptr, type_name)                                   \
    ((type_name *)ccc_buf_front(buf_ptr))

/** @brief Move data at index src to dst according to capacity.
@param [in] buf the pointer to the buffer.
@param [in] dst the index of destination within bounds of capacity.
@param [in] src the index of source within bounds of capacity.
@return a pointer to the slot at dst or NULL if bad input is provided.
@note this function does NOT modify the size of the container.

Note that destination and source are only required to be valid within bounds
of capacity of the buffer. It is up to the user to ensure destination and
source are within the size bounds of the buffer, if required. */
void *ccc_buf_move(ccc_buffer *buf, size_t dst, size_t src);

/** @brief write data to buffer at slot at index i according to capacity.
@param [in] buf the pointer to the buffer.
@param [in] i the index within bounds of capacity of the buffer.
@param [in] data the data that will be written to slot at i.
@return the result of the write, CCC_RESULT_OK if success. If no buffer or data
exists input error is returned. If i is outside of the range of capacity
input error is returned.
@note this function does NOT modify the size of the container.

Note that data will be written to the slot at index i, according to the
capacity of the buffer. It is up to the user to ensure i is within size
of the buffer if such behavior is desired. No elements are moved to be
preserved meaning any data at i is overwritten. */
ccc_result ccc_buf_write(ccc_buffer *buf, size_t i, void const *data);

/** @brief Writes a user provided compound literal directly to a buffer slot.
@param [in] buf_ptr a pointer to the buffer.
@param [in] index the desired index at which to insert an element.
@param [in] type_compound_literal the direct compound literal as provided.
@return a pointer to the inserted element or NULL if insertion failed.
@warning The index provided is only checked to be within capacity bounds so it
is the user's responsibility to ensure the index is within the contiguous range
of [0, size). This insert method does not increment the size of the buffer.

Any function calls that set fields of the compound literal will not be evaluated
if the provided index is out of range of the buffer capacity. */
#define ccc_buf_emplace(buf_ptr, index, type_compound_literal...)              \
    ccc_impl_buf_emplace(buf_ptr, index, type_compound_literal)

/** @brief swap elements at i and j according to capacity of the bufer.
@param [in] buf the pointer to the buffer.
@param [in] tmp the pointer to the temporary buffer of the same size as an
element stored in the buffer.
@param [in] i the index of an element in the buffer.
@param [in] j the index of an element in the buffer.
@return the result of the swap, CCC_RESULT_OK if no error occurs. If no buffer
exists, no tmp exists, i is out of capacity range, or j is out of capacity
range, an input error is returned.
@note this function does NOT modify the size of the container.

Note that i and j are only checked to be within capacity range of the buffer.
It is the user's responsibility to check for i and j within bounds of size
if such behavior is needed. */
ccc_result ccc_buf_swap(ccc_buffer *buf, void *tmp, size_t i, size_t j);

/**@}*/

/** @name Iteration Interface
The following functions implement iterators over the buffer. */
/**@{*/

/** @brief obtain the base address of the buffer in preparation for iteration.
@param [in] buf the pointer to the buffer.
@return the base address of the buffer. This will be equivalent to the buffer
end iterator if the buffer size is 0. NULL is returned if a NULL argument is
provided or the buffer has not yet been allocated. */
[[nodiscard]] void *ccc_buf_begin(ccc_buffer const *buf);

/** @brief advance the iter to the next slot in the buffer according to size.
@param [in] buf the pointer to the buffer.
@param [in] iter the pointer to the current slot of the buffer.
@return the next iterator position according to size. */
[[nodiscard]] void *ccc_buf_next(ccc_buffer const *buf, void const *iter);

/** @brief return the end position of the buffer according to size.
@param [in] buf the pointer to the buffer.
@return the address of the end position. It is undefined to access this
position for any reason. NULL is returned if NULL is provided or buffer has
not yet been allocated.

Note that end is determined by the size of the buffer dynamically. */
[[nodiscard]] void *ccc_buf_end(ccc_buffer const *buf);

/** @brief return the end position of the buffer according to capacity.
@param [in] buf the pointer to the buffer.
@return the address of the position one past capacity. It is undefined to
access this position for any reason. NULL is returned if NULL is provided or
buffer has not yet been allocated.

Note that end is determined by the capcity of the buffer and will not change
until a resize has occured, if permitted. */
[[nodiscard]] void *ccc_buf_capacity_end(ccc_buffer const *buf);

/** @brief obtain the address of the last element in the buffer in preparation
for iteration according to size.
@param [in] buf the pointer to the buffer.
@return the address of the last element buffer. This will be equivalent to the
buffer rend iterator if the buffer size is 0. NULL is returned if a NULL
argument is provided or the buffer has not yet been allocated. */
[[nodiscard]] void *ccc_buf_rbegin(ccc_buffer const *buf);

/** @brief advance the iter to the next slot in the buffer according to size and
in reverse order.
@param [in] buf the pointer to the buffer.
@param [in] iter the pointer to the current slot of the buffer.
@return the next iterator position according to size and in reverse order. NULL
is returned if bad input is provided or the buffer has not been allocated. */
[[nodiscard]] void *ccc_buf_rnext(ccc_buffer const *buf, void const *iter);

/** @brief return the rend position of the buffer.
@param [in] buf the pointer to the buffer.
@return the address of the rend position. It is undefined to access this
position for any reason. NULL is returned if NULL is provided or buffer has
not yet been allocated. */
[[nodiscard]] void *ccc_buf_rend(ccc_buffer const *buf);

/**@}*/

/** @name State Interface
These functions help manage or obtain state of the buffer. */
/**@{*/

/** @brief add n to the size of the buffer.
@param [in] buf the pointer to the buffer.
@param [in] n the quantity to add to the current buffer size.
@return the result of resizing. CCC_RESULT_OK if no errors occur or an error
indicating bad input has been provided.

If n would exceed the current capacity of the buffer the size is set to
capacity and the input error status is returned. */
ccc_result ccc_buf_size_plus(ccc_buffer *buf, size_t n);

/** @brief Subtract n from the size of the buffer.
@param [in] buf the pointer to the buffer.
@param [in] n the quantity to subtract from the current buffer size.
@return the result of resizing. CCC_RESULT_OK if no errors occur or an error
indicating bad input has been provided.

If n would reduce the size to less than 0, the buffer size is set to 0 and the
input error status is returned. */
ccc_result ccc_buf_size_minus(ccc_buffer *buf, size_t n);

/** @brief Set the buffer size to n.
@param [in] buf the pointer to the buffer.
@param [in] n the new size of the buffer.
@return the result of setting the size. CCC_RESULT_OK if no errors occur or an
error indicating bad input has been provided.

If n is larger than the capacity of the buffer the size is set equal to the
capacity and an error is returned. */
ccc_result ccc_buf_size_set(ccc_buffer *buf, size_t n);

/** @brief Return the current capacity of total possible slots.
@param [in] buf the pointer to the buffer.
@return the total number of elements the can be stored in the buffer. This
value remains the same until a resize occurs. An argument error is set if buf is
NULL. */
[[nodiscard]] ccc_ucount ccc_buf_capacity(ccc_buffer const *buf);

/** @brief The size of the type being stored contiguously in the buffer.
@param [in] buf the pointer to the buffer.
@return the size of the type being stored in the buffer. 0 if buf is NULL
because a zero sized object is not possible for a buffer. */
[[nodiscard]] ccc_ucount ccc_buf_sizeof_type(ccc_buffer const *buf);

/** @brief Return the bytes in the buffer given the current count of active
elements.
@param [in] buf the pointer to the buffer.
@return the number of bytes occupied by the current count of elements.

For total possible bytes that can be stored in the buffer see
ccc_buf_capacity_bytes. */
[[nodiscard]] ccc_ucount ccc_buf_count_bytes(ccc_buffer const *buf);

/** @brief Return the bytes in the buffer given the current capacity elements.
@param [in] buf the pointer to the buffer.
@return the number of bytes occupied by the current capacity elements.

For total possible bytes that can be stored in the buffer given the current
element count see ccc_buf_count_bytes. */
[[nodiscard]] ccc_ucount ccc_buf_capacity_bytes(ccc_buffer const *buf);

/** @brief obtain the count of buffer active slots.
@param [in] buf the pointer to the buffer.
@return the quantity of elements stored in the buffer. An argument error is set
if buf is NULL.

Note that size must be less than or equal to capacity. */
[[nodiscard]] ccc_ucount ccc_buf_count(ccc_buffer const *buf);

/** @brief return true if the size of the buffer is 0.
@param [in] buf the pointer to the buffer.
@return true if the size is 0 false if not. Error if buf is NULL. */
[[nodiscard]] ccc_tribool ccc_buf_is_empty(ccc_buffer const *buf);

/** @brief return true if the size of the buffer equals capacity.
@param [in] buf the pointer to the buffer.
@return true if the size equals the capacity. Error if buf is NULL. */
[[nodiscard]] ccc_tribool ccc_buf_is_full(ccc_buffer const *buf);

/**@}*/

/** @name Deallocation Interface
Free the elements of the container and the underlying buffer. */
/**@{*/

/** @brief Frees all slots in the buf and frees the underlying buffer that was
previously dynamically reserved with the reserve function.
@param [in] buf the buffer to be cleared.
@param [in] destructor the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the buf before their slots are
dropped.
@param [in] alloc the required allocation function to provide to a dynamically
reserved buf. Any auxiliary data provided upon initialization will be passed to
the allocation function when called.
@return the result of free operation. OK if success, or an error status to
indicate the error.
@warning It is an error to call this function on a buf that was not reserved
with the provided ccc_any_alloc_fn. The buf must have existing memory to free.

This function covers the edge case of reserving a dynamic capacity for a buf
at runtime but denying the buf allocation permission to resize. This can help
prevent a buf from growing unbounded. The user in this case knows the buf does
not have allocation permission and therefore no further memory will be dedicated
to the buf.

However, to free the buf in such a case this function must be used because the
buf has no ability to free itself. Just as the allocation function is required
to reserve memory so to is it required to free memory.

This function will work normally if called on a buf with allocation permission
however the normal ccc_buf_clear_and_free is sufficient for that use case.
Elements are assumed to be contiguous from the 0th index to index at size - 1.*/
ccc_result
ccc_buf_clear_and_free_reserve(ccc_buffer *buf,
                               ccc_any_type_destructor_fn *destructor,
                               ccc_any_alloc_fn *alloc);

/** @brief Set size of buf to 0 and call destructor on each element if needed.
Free the underlying buffer setting the capacity to 0. O(1) if no destructor is
provided, else O(N).
@param [in] buf a pointer to the buf.
@param [in] destructor the destructor if needed or NULL.

Note that if destructor is non-NULL it will be called on each element in the
buf. After all elements are processed the buffer is freed and capacity is 0.
If destructor is NULL the buffer is freed directly and capacity is 0. Elements
are assumed to be contiguous from the 0th index to index at size - 1.*/
ccc_result ccc_buf_clear_and_free(ccc_buffer *buf,
                                  ccc_any_type_destructor_fn *destructor);

/** @brief Set size of buf to 0 and call destructor on each element if needed.
O(1) if no destructor is provided, else O(N).
@param [in] buf a pointer to the buf.
@param [in] destructor the destructor if needed or NULL.

Note that if destructor is non-NULL it will be called on each element in the
buf. However, the underlying buffer for the buf is not freed. If the
destructor is NULL, setting the size to 0 is O(1). Elements are assumed to be
contiguous from the 0th index to index at size - 1.*/
ccc_result ccc_buf_clear(ccc_buffer *buf,
                         ccc_any_type_destructor_fn *destructor);

/**@}*/

/** Define this preprocessor directive to drop the ccc prefix from all buffer
related types and methods. By default the prefix is required but may be
dropped with this directive if one is sure no namespace collisions occur. */
#ifdef BUFFER_USING_NAMESPACE_CCC
typedef ccc_buffer buffer;
#    define buf_init(args...) ccc_buf_init(args)
#    define buf_alloc(args...) ccc_buf_alloc(args)
#    define buf_reserve(args...) ccc_buf_reserve(args)
#    define buf_copy(args...) ccc_buf_copy(args)
#    define buf_clear(args...) ccc_buf_clear(args)
#    define buf_clear_and_free(args...) ccc_buf_clear_and_free(args)
#    define buf_clear_and_free_reserve(args...)                                \
        ccc_buf_clear_and_free_reserve(args)
#    define buf_count(args...) ccc_buf_count(args)
#    define buf_count_bytes(args...) ccc_buf_count_bytes(args)
#    define buf_size_plus(args...) ccc_buf_size_plus(args)
#    define buf_size_minus(args...) ccc_buf_size_minus(args)
#    define buf_size_set(args...) ccc_buf_size_set(args)
#    define buf_capacity(args...) ccc_buf_capacity(args)
#    define buf_capacity_bytes(args...) ccc_buf_capacity_bytes(args)
#    define buf_sizeof_type(args...) ccc_buf_sizeof_type(args)
#    define buf_i(args...) ccc_buf_i(args)
#    define buf_is_full(args...) ccc_buf_is_full(args)
#    define buf_is_empty(args...) ccc_buf_is_empty(args)
#    define buf_at(args...) ccc_buf_at(args)
#    define buf_as(args...) ccc_buf_as(args)
#    define buf_back(args...) ccc_buf_back(args)
#    define buf_back_as(args...) ccc_buf_back_as(args)
#    define buf_front(args...) ccc_buf_front(args)
#    define buf_front_as(args...) ccc_buf_front_as(args)
#    define buf_alloc_back(args...) ccc_buf_alloc_back(args)
#    define buf_emplace(args...) ccc_buf_emplace(args)
#    define buf_emplace_back(args...) ccc_buf_emplace_back(args)
#    define buf_push_back(args...) ccc_buf_push_back(args)
#    define buf_pop_back(args...) ccc_buf_pop_back(args)
#    define buf_pop_back_n(args...) ccc_buf_pop_back_n(args)
#    define buf_move(args...) ccc_buf_move(args)
#    define buf_swap(args...) ccc_buf_swap(args)
#    define buf_write(args...) ccc_buf_write(args)
#    define buf_erase(args...) ccc_buf_erase(args)
#    define buf_insert(args...) ccc_buf_insert(args)
#    define buf_begin(args...) ccc_buf_begin(args)
#    define buf_next(args...) ccc_buf_next(args)
#    define buf_end(args...) ccc_buf_end(args)
#    define buf_rbegin(args...) ccc_buf_rbegin(args)
#    define buf_rnext(args...) ccc_buf_rnext(args)
#    define buf_rend(args...) ccc_buf_rend(args)
#    define buf_capacity_end(args...) ccc_buf_capacity_end(args)
#endif /* BUFFER_USING_NAMESPACE_CCC */

#endif /* CCC_BUFFER_H */
