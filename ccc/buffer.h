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
in this library that provide more specialized operations. A Buffer does not
require the user accommodate any intrusive elements.

A Buffer offers a more flexible interface than a standard C++ vector. There are
functions that assume elements are stored contiguously from [0, N) where count
is the count of elements. However, there are also functions that let the user
access any Buffer slot that is within the bounds of Buffer capacity. This
requires the user pay closer attention to Buffer usage but ultimately allows
them to build a wider variety of abstractions on top of the buffer.

Interface functions in the slot management section offer data movement and
writing operations that do not affect the size of the container. If writing a
more complex higher level container that does not need size management these
functions offer more custom control over the buffer.


A Buffer with allocation permission will re-size as required when a new element
is inserted in a contiguous fashion. Interface functions in the allocation
management section assume elements are stored contiguously and adjust size
accordingly.

If allocation is not permitted, resizing will not occur and the insertion
function will fail when capacity is reached, returning some value to indicate
failure.

If shorter names are desired, define the following preprocessor directive.

```
#define BUFFER_USING_NAMESPACE_CCC
```

Then, the `CCC_` prefix can be dropped from all types and functions. */
#ifndef CCC_BUFFER_H
#define CCC_BUFFER_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "private/private_buffer.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A contiguous block of storage for elements of the same type.
@warning it is undefined behavior to use an uninitialized buffer.

A Buffer may be initialized on the stack, heap, or data segment at compile time
or runtime. */
typedef struct CCC_Buffer CCC_Buffer;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a contiguous Buffer of user a specified type, allocation
policy, capacity, and optional starting size.
@param[in] mem_pointer the pointer to existing memory or NULL.
@param[in] type_name the name of the user type in the buffer.
@param[in] allocate CCC_Allocator or NULL if no allocation is permitted.
@param[in] context_data any context data needed for managing Buffer memory.
@param[in] capacity the capacity of memory at mem_pointer.
@param[in] optional_size optional starting size of the Buffer <= capacity.
@return the initialized buffer. Directly assign to Buffer on the right hand
side of the equality operator (e.g. CCC_Buffer b = CCC_buffer_initialize(...);).

Initialization of a Buffer can occur at compile time or run time depending
on the arguments. The memory pointer should be of the same type one intends to
store in the buffer.

```
#define BUFFER_USING_NAMESPACE_CCC
static Buffer stack = buffer_initialize(&(static int[4096]){}, int, NULL, NULL,
4096);
```

Initialize a fixed Buffer with some elements occupied.

```
#define BUFFER_USING_NAMESPACE_CCC
static Buffer stack
    = buffer_initialize(&(static int[4096]){0, 1, 2, 3}, int, NULL, NULL, 4096,
4);
```

This initializer determines memory control for the lifetime of the buffer. If
the Buffer points to memory of a predetermined and fixed capacity do not
provide an allocation function. If a dynamic Buffer is preferred, provide the
allocation function as defined by the signature in types.h. If resizing is
desired on memory that has already been allocated, ensure allocation has
occurred with the provided allocation function. */
#define CCC_buffer_initialize(mem_pointer, type_name, allocate, context_data,  \
                              capacity, optional_size...)                      \
    CCC_private_buffer_initialize(mem_pointer, type_name, allocate,            \
                                  context_data, capacity, optional_size)

/** @brief Initialize a Buffer from a compound literal array initializer.
@param[in] allocate CCC_Allocator or NULL if no allocation is permitted.
@param[in] context_data any context data needed for managing Buffer memory.
@param[in] optional_capacity optionally specify the capacity of the Buffer if
different from the size of the compound literal array initializer. If the
capacity is greater than the size of the compound literal array initializer, it
is respected and the capacity is reserved. If the capacity is less than the size
of the compound array initializer, the compound literal array initializer size
is set as the capacity. Therefore, 0 is valid if one is not concerned with the
underlying reservation.
@param[in] compound_literal_array the initializer of the type stored in buffer.
@return the initialized buffer. Directly assign to Buffer on the right hand
side of the equality operator (e.g. CCC_Buffer b = CCC_buffer_from(...);).

Initialize a dynamic Buffer with a compound literal array.

```
#define BUFFER_USING_NAMESPACE_CCC
int
main(void)
{
    Buffer b = buffer_from(std_allocate, NULL, 0,
        (int[]){ 0, 1, 2, 3 }
    );
    return 0;
}
```

Initialize a dynamic Buffer with a compound literal array with capacity.

```
#define BUFFER_USING_NAMESPACE_CCC
int
main(void)
{
    Buffer b = buffer_from(std_allocate, NULL, 4096,
        (int[]){ 0, 1, 2, 3 }
    );
    return 0;
}
```

Only dynamic buffers may be initialized this way. For static or stack based
initialization of fixed buffers with contents known at compile time, see the
CCC_buffer_initialize() macro. */
#define CCC_buffer_from(allocate, context_data, optional_capacity,             \
                        compound_literal_array...)                             \
    CCC_private_buffer_from(allocate, context_data, optional_capacity,         \
                            compound_literal_array)

/** @brief Initialize a Buffer with a capacity.
@param[in] type_name any user or language standard type name.
@param[in] allocate CCC_Allocator or NULL if no allocation is permitted.
@param[in] context_data any context data needed for managing Buffer memory.
@param[in] capacity the capacity of the Buffer to reserve.
@return the initialized buffer. Directly assign to Buffer on the right hand
side of the equality operator (e.g. CCC_Buffer b =
CCC_buffer_with_capacity(...);).

Initialize a dynamic buffer.

```
#define BUFFER_USING_NAMESPACE_CCC
int
main(void)
{
    Buffer b = buffer_from(std_allocate, NULL, 4096);
    return 0;
}
```

Only dynamic buffers may be initialized this way. For static or stack based
initialization of fixed buffers with contents known at compile time, see the
CCC_buffer_initialize() macro. */
#define CCC_buffer_with_capacity(type_name, allocate, context_data, capacity)  \
    CCC_private_buffer_with_capacity(type_name, allocate, context_data,        \
                                     capacity)

/** @brief Reserves space for at least to_add more elements.
@param[in] buffer a pointer to the buffer.
@param[in] to_add the number of elements to add to the current size.
@param[in] allocate the allocation function to use to reserve memory.
@return the result of the reservation. OK if successful, otherwise an error
status is returned.
@note see the CCC_buffer_clear_and_free_reserve function if this function is
being used for a one-time dynamic reservation.

This function can be used for a dynamic buffer with or without allocation
permission. If the buffer has allocation permission, it will reserve the
required space and later resize if more space is needed.

If the buffer has been initialized with no allocation permission and no memory
this function can serve as a one-time reservation. To free the buffer in such a
case see the CCC_buffer_clear_and_free_reserve function. */
[[nodiscard]] CCC_Result CCC_buffer_reserve(CCC_Buffer *buffer, size_t to_add,
                                            CCC_Allocator *allocate);

/** @brief Copy the buffer from source to newly initialized destination.
@param[in] destination the destination that will copy the source buf.
@param[in] source the source of the buf.
@param[in] allocate the allocation function in case resizing of destination is
needed.
@return the result of the copy operation. If the destination capacity is less
than the source capacity and no allocation function is provided an input error
is returned. If resizing is required and resizing of destination fails a memory
error is returned.
@note destination must have capacity greater than or equal to source. If
destination capacity is less than source, an allocation function must be
provided with the allocate argument.

Note that there are two ways to copy data from source to destination: provide
sufficient memory and pass NULL as allocate, or allow the copy function to take
care of allocation for the copy.

Manual memory management with no allocation function provided.

```
#define BUFFER_USING_NAMESPACE_CCC
Buffer source = buffer_initialize((int[10]){}, int, NULL, NULL, 10);
int *new_data = malloc(sizeof(int) * buffer_capacity(&source).count);
Buffer destination
    = buffer_initialize(new_data, int, NULL, NULL,
buffer_capacity(&source).count); CCC_Result res = buffer_copy(&destination,
&source, NULL);
```

The above requires destination capacity be greater than or equal to source
capacity. Here is memory management handed over to the copy function.

```
#define BUFFER_USING_NAMESPACE_CCC
Buffer source = buffer_initialize(NULL, int, std_allocate, NULL, 0);
(void)CCC_buffer_push_back_range(&source, 5, (int[5]){0,1,2,3,4});
Buffer destination = buffer_initialize(NULL, int, std_allocate, NULL, 0);
CCC_Result res = buffer_copy(&destination, &source, std_allocate);
```

The above allows destination to have a capacity less than that of the source as
long as copy has been provided an allocation function to resize destination.
Note that this would still work if copying to a destination that the user wants
as a fixed size buffer (ring buffer).

```
#define BUFFER_USING_NAMESPACE_CCC
Buffer source = buffer_initialize(NULL, int, std_allocate, NULL, 0);
(void)CCC_buffer_push_back_range(&source, 5, (int[5]){0,1,2,3,4});
Buffer destination = buffer_initialize(NULL, int, NULL, NULL, 0);
CCC_Result res = buffer_copy(&destination, &source, std_allocate);
```

Because an allocation function is provided, the destination is resized once for
the copy and retains its fixed size after the copy is complete. This would
require the user to manually free the underlying Buffer at destination
eventually if this method is used. Usually it is better to allocate the memory
explicitly before the copy if copying between ring buffers.

These options allow users to stay consistent across containers with their
memory management strategies. */
[[nodiscard]] CCC_Result CCC_buffer_copy(CCC_Buffer *destination,
                                         CCC_Buffer const *source,
                                         CCC_Allocator *allocate);

/**@}*/

/** @name Insert and Remove Interface
These functions assume contiguity of elements in the Buffer and increase or
decrease size accordingly. */
/**@{*/

/** @brief allocates the Buffer to the specified size according to the user
defined allocation function.
@param[in] buffer a pointer to the buffer.
@param[in] capacity the newly desired capacity.
@param[in] allocate the allocation function defined by the user.
@return the result of reallocation.

This function takes the allocation function as an argument in case no
allocation function has been provided upon initialization and the user is
managing allocations and resizing directly. If an allocation function has
been provided than the use of this function should be rare as the buffer
will reallocate more memory when necessary. */
[[nodiscard]] CCC_Result CCC_buffer_allocate(CCC_Buffer *buffer,
                                             size_t capacity,
                                             CCC_Allocator *allocate);

/** @brief allocates a new slot from the Buffer at the end of the contiguous
array. A slot is equivalent to one of the element type specified when the
Buffer is initialized.
@param[in] buffer a pointer to the buffer.
@return a pointer to the newly allocated memory or NULL if no Buffer is
provided or the Buffer is unable to allocate more memory.
@note this function modifies the size of the container.

A Buffer can be used as the backing for more complex data structures.
Requesting new space from a Buffer as an allocator can be helpful for these
higher level organizations. */
[[nodiscard]] void *CCC_buffer_allocate_back(CCC_Buffer *buffer);

/** @brief return the newly pushed data into the last slot of the buffer
according to size.
@param[in] buffer the pointer to the buffer.
@param[in] data the pointer to the data of element size.
@return the pointer to the newly pushed element or NULL if no Buffer exists or
resizing has failed due to memory exhuastion or no allocation allowed.
@note this function modifies the size of the container.

The data is copied into the Buffer at the final slot if there is remaining
capacity. If size is equal to capacity resizing will be attempted but may
fail if no allocation function is provided or the allocator provided is
exhausted. */
[[nodiscard]] void *CCC_buffer_push_back(CCC_Buffer *buffer, void const *data);

/** @brief Pushes the user provided compound literal directly to back of buffer
and increments the size to reflect the newly added element.
@param[in] buffer_pointer a pointer to the buffer.
@param[in] type_compound_literal the direct compound literal as provided.
@return a pointer to the inserted element or NULL if insertion failed.

Any function calls that set fields of the compound literal will not be evaluated
if the Buffer fails to allocate a slot at the back of the buffer. This may occur
if resizing fails or is prohibited. */
#define CCC_buffer_emplace_back(buffer_pointer, type_compound_literal...)      \
    CCC_private_buffer_emplace_back(buffer_pointer, type_compound_literal)

/** @brief insert data at slot index according to size of the Buffer maintaining
contiguous storage of elements between 0 and size.
@param[in] buffer the pointer to the buffer.
@param[in] index the index at which to insert data.
@param[in] data the data copied into the Buffer at index index of the same size
as elements stored in the buffer.
@return the pointer to the inserted element or NULL if bad input is provided,
the Buffer is full and no resizing is allowed, or resizing fails when resizing
is allowed.
@note this function modifies the size of the container.

Note that this function assumes elements must be maintained contiguously
according to size of the Buffer meaning a bulk move of elements sliding down
to accommodate index will occur. */
[[nodiscard]] void *CCC_buffer_insert(CCC_Buffer *buffer, size_t index,
                                      void const *data);

/** @brief pop the back element from the Buffer according to size.
@param[in] buffer the pointer to the buffer.
@return the result of the attempted pop. CCC_RESULT_OK upon success or an input
error if bad input is provided.
@note this function modifies the size of the container. */
CCC_Result CCC_buffer_pop_back(CCC_Buffer *buffer);

/** @brief pop count elements from the back of the Buffer according to size.
@param[in] buffer the pointer to the buffer.
@param[in] count the number of elements to pop.
@return the result of the attempted pop. CCC_RESULT_OK if the Buffer exists and
n is within the bounds of size. If the Buffer does not exist an input error is
returned. If count is greater than the size of the Buffer size is set to zero
and input error is returned.
@note this function modifies the size of the container. */
CCC_Result CCC_buffer_pop_back_n(CCC_Buffer *buffer, size_t count);

/** @brief erase element at slot index according to size of the Buffer
maintaining contiguous storage of elements between 0 and size.
@param[in] buffer the pointer to the buffer.
@param[in] index the index of the element to be erased.
@return the result, CCC_RESULT_OK if the input is valid. If no Buffer exists or
i is out of range of size then an input error is returned.
@note this function modifies the size of the container.

Note that this function assumes elements must be maintained contiguously
according to size meaning a bulk copy of elements sliding down to fill the
space left by index will occur. */
CCC_Result CCC_buffer_erase(CCC_Buffer *buffer, size_t index);

/**@}*/

/** @name Slot Management Interface
These functions interact with slots in the Buffer directly and do not modify
the size of the buffer. These are best used for custom container types operating
at a higher level of abstraction. */
/**@{*/

/** @brief return the element at slot index in buf.
@param[in] buffer the pointer to the buffer.
@param[in] index the index within capacity range of the buffer.
@return a pointer to the element in the slot at position index or NULL if index
is out of capacity range.

Note that as long as the index is valid within the capacity of the Buffer a
valid pointer is returned, which may result in a slot of old or uninitialized
data. It is up to the user to ensure the index provided is within the current
size of the buffer. */
[[nodiscard]] void *CCC_buffer_at(CCC_Buffer const *buffer, size_t index);

/** @brief Access en element at the specified index as the stored type.
@param[in] buffer_pointer the pointer to the buffer.
@param[in] type_name the name of the stored type.
@param[in] index the index within capacity range of the buffer.
@return a pointer to the element in the slot at position index or NULL if index
is out of capacity range.

Note that as long as the index is valid within the capacity of the Buffer a
valid pointer is returned, which may result in a slot of old or uninitialized
data. It is up to the user to ensure the index provided is within the current
size of the buffer. */
#define CCC_buffer_as(buffer_pointer, type_name, index)                        \
    ((type_name *)CCC_buffer_at(buffer_pointer, index))

/** @brief return the index of an element known to be in the buffer.
@param[in] buffer the pointer to the buffer.
@param[in] slot the pointer to the element stored in the buffer.
@return the index if the slot provided is within the capacity range of the
buffer, otherwise an argument error is set. */
[[nodiscard]] CCC_Count CCC_buffer_i(CCC_Buffer const *buffer,
                                     void const *slot);

/** @brief return the final element in the Buffer according the current size.
@param[in] buffer the pointer to the buffer.
@return the pointer the final element in the Buffer according to the current
size or NULL if the Buffer does not exist or is empty. */
[[nodiscard]] void *CCC_buffer_back(CCC_Buffer const *buffer);

/** @brief return the final element in the Buffer according the current size.
@param[in] buffer_pointer the pointer to the buffer.
@param[in] type_name the name of the stored type.
@return the pointer the final element in the Buffer according to the current
size or NULL if the Buffer does not exist or is empty. */
#define CCC_buffer_back_as(buffer_pointer, type_name)                          \
    ((type_name *)CCC_buffer_back(buffer_pointer))

/** @brief return the first element in the Buffer at index 0.
@param[in] buffer the pointer to the buffer.
@return the pointer to the front element or NULL if the Buffer does not exist
or is empty. */
[[nodiscard]] void *CCC_buffer_front(CCC_Buffer const *buffer);

/** @brief return the first element in the Buffer at index 0.
@param[in] buffer_pointer the pointer to the buffer.
@param[in] type_name the name of the stored type.
@return the pointer to the front element or NULL if the Buffer does not exist
or is empty. */
#define CCC_buffer_front_as(buffer_pointer, type_name)                         \
    ((type_name *)CCC_buffer_front(buffer_pointer))

/** @brief Move data at index source to destination according to capacity.
@param[in] buffer the pointer to the buffer.
@param[in] destination the index of destination within bounds of capacity.
@param[in] source the index of source within bounds of capacity.
@return a pointer to the slot at destination or NULL if bad input is provided.
@note this function does NOT modify the size of the container.

Note that destination and source are only required to be valid within bounds
of capacity of the buffer. It is up to the user to ensure destination and
source are within the size bounds of the buffer, if required. */
void *CCC_buffer_move(CCC_Buffer *buffer, size_t destination, size_t source);

/** @brief write data to Buffer at slot at index index according to capacity.
@param[in] buffer the pointer to the buffer.
@param[in] index the index within bounds of capacity of the buffer.
@param[in] data the data that will be written to slot at i.
@return the result of the write, CCC_RESULT_OK if success. If no Buffer or data
exists input error is returned. If index is outside of the range of capacity
input error is returned.
@note this function does NOT modify the size of the container.

Note that data will be written to the slot at index i, according to the
capacity of the buffer. It is up to the user to ensure index is within size
of the Buffer if such behavior is desired. No elements are moved to be
preserved meaning any data at index is overwritten. */
CCC_Result CCC_buffer_write(CCC_Buffer *buffer, size_t index, void const *data);

/** @brief Writes a user provided compound literal directly to a Buffer slot.
@param[in] buffer_pointer a pointer to the buffer.
@param[in] index the desired index at which to insert an element.
@param[in] type_compound_literal the direct compound literal as provided.
@return a pointer to the inserted element or NULL if insertion failed.
@warning The index provided is only checked to be within capacity bounds so it
is the user's responsibility to ensure the index is within the contiguous range
of [0, size). This insert method does not increment the size of the buffer.

Any function calls that set fields of the compound literal will not be evaluated
if the provided index is out of range of the Buffer capacity. */
#define CCC_buffer_emplace(buffer_pointer, index, type_compound_literal...)    \
    CCC_private_buffer_emplace(buffer_pointer, index, type_compound_literal)

/** @brief swap elements at index and swap_index according to capacity of the
bufer.
@param[in] buffer the pointer to the buffer.
@param[in] temp the pointer to the temporary Buffer of the same size as an
element stored in the buffer.
@param[in] index the index of an element in the buffer.
@param[in] swap_index the index of an element in the buffer.
@return the result of the swap, CCC_RESULT_OK if no error occurs. If no buffer
exists, no temp exists, index is out of capacity range, or swap_index is out of
capacity range, an input error is returned.
@note this function does NOT modify the size of the container.

Note that index and swap_index are only checked to be within capacity range of
the buffer. It is the user's responsibility to check for index and swap_index
within bounds of size if such behavior is needed. */
CCC_Result CCC_buffer_swap(CCC_Buffer *buffer, void *temp, size_t index,
                           size_t swap_index);

/**@}*/

/** @name Iteration Interface
The following functions implement iterators over the buffer. */
/**@{*/

/** @brief obtain the base address of the Buffer in preparation for iteration.
@param[in] buffer the pointer to the buffer.
@return the base address of the buffer. This will be equivalent to the buffer
end iterator if the Buffer size is 0. NULL is returned if a NULL argument is
provided or the Buffer has not yet been allocated. */
[[nodiscard]] void *CCC_buffer_begin(CCC_Buffer const *buffer);

/** @brief advance the iterator to the next slot in the Buffer according to
size.
@param[in] buffer the pointer to the buffer.
@param[in] iterator the pointer to the current slot of the buffer.
@return the next iterator position according to size. */
[[nodiscard]] void *CCC_buffer_next(CCC_Buffer const *buffer,
                                    void const *iterator);

/** @brief return the end position of the Buffer according to size.
@param[in] buffer the pointer to the buffer.
@return the address of the end position. It is undefined to access this
position for any reason. NULL is returned if NULL is provided or Buffer has
not yet been allocated.

Note that end is determined by the size of the Buffer dynamically. */
[[nodiscard]] void *CCC_buffer_end(CCC_Buffer const *buffer);

/** @brief return the end position of the Buffer according to capacity.
@param[in] buffer the pointer to the buffer.
@return the address of the position one past capacity. It is undefined to
access this position for any reason. NULL is returned if NULL is provided or
Buffer has not yet been allocated.

Note that end is determined by the capcity of the Buffer and will not change
until a resize has occured, if permitted. */
[[nodiscard]] void *CCC_buffer_capacity_end(CCC_Buffer const *buffer);

/** @brief obtain the address of the last element in the Buffer in preparation
for iteration according to size.
@param[in] buffer the pointer to the buffer.
@return the address of the last element buffer. This will be equivalent to the
Buffer reverse_end iterator if the Buffer size is 0. NULL is returned if a NULL
argument is provided or the Buffer has not yet been allocated. */
[[nodiscard]] void *CCC_buffer_reverse_begin(CCC_Buffer const *buffer);

/** @brief advance the iterator to the next slot in the Buffer according to size
and in reverse order.
@param[in] buffer the pointer to the buffer.
@param[in] iterator the pointer to the current slot of the buffer.
@return the next iterator position according to size and in reverse order. NULL
is returned if bad input is provided or the Buffer has not been allocated. */
[[nodiscard]] void *CCC_buffer_reverse_next(CCC_Buffer const *buffer,
                                            void const *iterator);

/** @brief return the reverse_end position of the buffer.
@param[in] buffer the pointer to the buffer.
@return the address of the reverse_end position. It is undefined to access this
position for any reason. NULL is returned if NULL is provided or Buffer has
not yet been allocated. */
[[nodiscard]] void *CCC_buffer_reverse_end(CCC_Buffer const *buffer);

/**@}*/

/** @name State Interface
These functions help manage or obtain state of the buffer. */
/**@{*/

/** @brief add count to the size of the buffer.
@param[in] buffer the pointer to the buffer.
@param[in] count the quantity to add to the current Buffer size.
@return the result of resizing. CCC_RESULT_OK if no errors occur or an error
indicating bad input has been provided.

If count would exceed the current capacity of the Buffer the size is set to
capacity and the input error status is returned. */
CCC_Result CCC_buffer_size_plus(CCC_Buffer *buffer, size_t count);

/** @brief Subtract count from the size of the buffer.
@param[in] buffer the pointer to the buffer.
@param[in] count the quantity to subtract from the current Buffer size.
@return the result of resizing. CCC_RESULT_OK if no errors occur or an error
indicating bad input has been provided.

If count would reduce the size to less than 0, the Buffer size is set to 0 and
the input error status is returned. */
CCC_Result CCC_buffer_size_minus(CCC_Buffer *buffer, size_t count);

/** @brief Set the Buffer size to n.
@param[in] buffer the pointer to the buffer.
@param[in] count the new size of the buffer.
@return the result of setting the size. CCC_RESULT_OK if no errors occur or an
error indicating bad input has been provided.

If count is larger than the capacity of the Buffer the size is set equal to the
capacity and an error is returned. */
CCC_Result CCC_buffer_size_set(CCC_Buffer *buffer, size_t count);

/** @brief obtain the count of Buffer active slots.
@param[in] buffer the pointer to the buffer.
@return the quantity of elements stored in the buffer. An argument error is set
if buffer is NULL.

Note that size must be less than or equal to capacity. */
[[nodiscard]] CCC_Count CCC_buffer_count(CCC_Buffer const *buffer);

/** @brief Return the current capacity of total possible slots.
@param[in] buffer the pointer to the buffer.
@return the total number of elements the can be stored in the buffer. This
value remains the same until a resize occurs. An argument error is set if buffer
is NULL. */
[[nodiscard]] CCC_Count CCC_buffer_capacity(CCC_Buffer const *buffer);

/** @brief The size of the type being stored contiguously in the buffer.
@param[in] buffer the pointer to the buffer.
@return the size of the type being stored in the buffer. 0 if buffer is NULL
because a zero sized object is not possible for a buffer. */
[[nodiscard]] CCC_Count CCC_buffer_sizeof_type(CCC_Buffer const *buffer);

/** @brief Return the bytes in the Buffer given the current count of active
elements.
@param[in] buffer the pointer to the buffer.
@return the number of bytes occupied by the current count of elements.

For total possible bytes that can be stored in the Buffer see
CCC_buffer_capacity_bytes. */
[[nodiscard]] CCC_Count CCC_buffer_count_bytes(CCC_Buffer const *buffer);

/** @brief Return the bytes in the Buffer given the current capacity elements.
@param[in] buffer the pointer to the buffer.
@return the number of bytes occupied by the current capacity elements.

For total possible bytes that can be stored in the Buffer given the current
element count see CCC_buffer_count_bytes. */
[[nodiscard]] CCC_Count CCC_buffer_capacity_bytes(CCC_Buffer const *buffer);

/** @brief return true if the size of the Buffer is 0.
@param[in] buffer the pointer to the buffer.
@return true if the size is 0 false if not. Error if buffer is NULL. */
[[nodiscard]] CCC_Tribool CCC_buffer_is_empty(CCC_Buffer const *buffer);

/** @brief return true if the size of the Buffer equals capacity.
@param[in] buffer the pointer to the buffer.
@return true if the size equals the capacity. Error if buffer is NULL. */
[[nodiscard]] CCC_Tribool CCC_buffer_is_full(CCC_Buffer const *buffer);

/**@}*/

/** @name Deallocation Interface
Free the elements of the container and the underlying buffer. */
/**@{*/

/** @brief Frees all slots in the buffer and frees the underlying Buffer that
was previously dynamically reserved with the reserve function.
@param[in] buffer the Buffer to be cleared.
@param[in] destroy the destroy for each element. NULL can be passed if no
maintenance is required on the elements in the buffer before their slots are
dropped.
@param[in] allocate the required allocation function to provide to a
dynamically reserved buf. Any context data provided upon initialization will be
passed to the allocation function when called.
@return the result of free operation. OK if success, or an error status to
indicate the error.
@warning It is an error to call this function on a buffer that was not reserved
with the provided CCC_Allocator. The buffer must have existing memory to free.

This function covers the edge case of reserving a dynamic capacity for a buf
at runtime but denying the buffer allocation permission to resize. This can help
prevent a buffer from growing unbounded. The user in this case knows the buffer
does not have allocation permission and therefore no further memory will be
dedicated to the buf.

However, to free the buffer in such a case this function must be used because
the buf has no ability to free itself. Just as the allocation function is
required to reserve memory so to is it required to free memory.

This function will work normally if called on a buffer with allocation
permission however the normal CCC_buffer_clear_and_free is sufficient for that
use case. Elements are assumed to be contiguous from the 0th index to index at
size - 1.*/
CCC_Result CCC_buffer_clear_and_free_reserve(CCC_Buffer *buffer,
                                             CCC_Type_destructor *destroy,
                                             CCC_Allocator *allocate);

/** @brief Set size of buffer to 0 and call destroy on each element if
needed. Free the underlying Buffer setting the capacity to 0. O(1) if no
destructor is provided, else O(N).
@param[in] buffer a pointer to the buf.
@param[in] destroy the destroy if needed or NULL.

Note that if destroy is non-NULL it will be called on each element in the
buf. After all elements are processed the Buffer is freed and capacity is 0.
If destroy is NULL the Buffer is freed directly and capacity is 0. Elements
are assumed to be contiguous from the 0th index to index at size - 1.*/
CCC_Result CCC_buffer_clear_and_free(CCC_Buffer *buffer,
                                     CCC_Type_destructor *destroy);

/** @brief Set size of buffer to 0 and call destroy on each element if
needed. O(1) if no destroy is provided, else O(N).
@param[in] buffer a pointer to the buf.
@param[in] destroy the destroy if needed or NULL.

Note that if destroy is non-NULL it will be called on each element in the
buf. However, the underlying Buffer for the buffer is not freed. If the
destructor is NULL, setting the size to 0 is O(1). Elements are assumed to be
contiguous from the 0th index to index at size - 1.*/
CCC_Result CCC_buffer_clear(CCC_Buffer *buffer, CCC_Type_destructor *destroy);

/**@}*/

/** Define this preprocessor directive to drop the ccc prefix from all buffer
related types and methods. By default the prefix is required but may be
dropped with this directive if one is sure no namespace collisions occur. */
#ifdef BUFFER_USING_NAMESPACE_CCC
typedef CCC_Buffer Buffer;
#    define buffer_initialize(args...) CCC_buffer_initialize(args)
#    define buffer_allocate(args...) CCC_buffer_allocate(args)
#    define buffer_reserve(args...) CCC_buffer_reserve(args)
#    define buffer_copy(args...) CCC_buffer_copy(args)
#    define buffer_clear(args...) CCC_buffer_clear(args)
#    define buffer_clear_and_free(args...) CCC_buffer_clear_and_free(args)
#    define buffer_clear_and_free_reserve(args...)                             \
        CCC_buffer_clear_and_free_reserve(args)
#    define buffer_count(args...) CCC_buffer_count(args)
#    define buffer_count_bytes(args...) CCC_buffer_count_bytes(args)
#    define buffer_size_plus(args...) CCC_buffer_size_plus(args)
#    define buffer_size_minus(args...) CCC_buffer_size_minus(args)
#    define buffer_size_set(args...) CCC_buffer_size_set(args)
#    define buffer_capacity(args...) CCC_buffer_capacity(args)
#    define buffer_capacity_bytes(args...) CCC_buffer_capacity_bytes(args)
#    define buffer_sizeof_type(args...) CCC_buffer_sizeof_type(args)
#    define buffer_i(args...) CCC_buffer_i(args)
#    define buffer_is_full(args...) CCC_buffer_is_full(args)
#    define buffer_is_empty(args...) CCC_buffer_is_empty(args)
#    define buffer_at(args...) CCC_buffer_at(args)
#    define buffer_as(args...) CCC_buffer_as(args)
#    define buffer_back(args...) CCC_buffer_back(args)
#    define buffer_back_as(args...) CCC_buffer_back_as(args)
#    define buffer_front(args...) CCC_buffer_front(args)
#    define buffer_front_as(args...) CCC_buffer_front_as(args)
#    define buffer_allocate_back(args...) CCC_buffer_allocate_back(args)
#    define buffer_emplace(args...) CCC_buffer_emplace(args)
#    define buffer_emplace_back(args...) CCC_buffer_emplace_back(args)
#    define buffer_push_back(args...) CCC_buffer_push_back(args)
#    define buffer_pop_back(args...) CCC_buffer_pop_back(args)
#    define buffer_pop_back_n(args...) CCC_buffer_pop_back_n(args)
#    define buffer_move(args...) CCC_buffer_move(args)
#    define buffer_swap(args...) CCC_buffer_swap(args)
#    define buffer_write(args...) CCC_buffer_write(args)
#    define buffer_erase(args...) CCC_buffer_erase(args)
#    define buffer_insert(args...) CCC_buffer_insert(args)
#    define buffer_begin(args...) CCC_buffer_begin(args)
#    define buffer_next(args...) CCC_buffer_next(args)
#    define buffer_end(args...) CCC_buffer_end(args)
#    define buffer_reverse_begin(args...) CCC_buffer_reverse_begin(args)
#    define buffer_reverse_next(args...) CCC_buffer_reverse_next(args)
#    define buffer_reverse_end(args...) CCC_buffer_reverse_end(args)
#    define buffer_capacity_end(args...) CCC_buffer_capacity_end(args)
#endif /* BUFFER_USING_NAMESPACE_CCC */

#endif /* CCC_BUFFER_H */
