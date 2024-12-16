/** @file
@brief The Buffer Interface

Buffer usage is similar to a C++ vector, with more flexible functions
provided to support higher level containers and abstractions. While useful
on its own--a stack could be implemented with the provided functions--a buffer
is often used as the lower level abstraction for the flat data structures
in this library that provide more specialized operations. A buffer does not
require the user accommodate any intrusive elements.

A buffer with allocation permission will re-size when a new element is inserted
in a contiguous fashion. Interface in the allocation management section assume
elements are stored contiguously and adjust size accordingly.

Interface in the slot management section offer data movement and writing
operations that do not affect the size of the container. If writing a more
complex higher level container that does not need size management these
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
#include <stdbool.h>
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
typedef struct ccc_buf_ ccc_buffer;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a contiguous buffer of user a specified type, allocation
policy, capacity, and optional starting size.
@param [in] mem_ptr the pointer to existing memory or ((Type *)NULL).
@param [in] alloc_fn ccc_alloc_fn or NULL if no allocation is permitted.
@param [in] aux_data any auxiliary data needed for managing buffer memory.
@param [in] capacity the capacity of memory at mem_ptr.
@param [in] optional_size optional starting size of the buffer <= capacity.
@return the initialized buffer. Directly assign to buffer on the right hand
side of the equality operator (e.g. ccc_buffer b = ccc_buf_init(...);).

Initialization of a buffer can occur at compile time or run time depending
on the arguments. The memory pointer should be of the same type one intends to
store in the buffer. Therefore, if one desires a dynamic buffer with a starting
capacity of 0 and mem_ptr of NULL, casting to a type is required
(e.g. (int*)NULL).

This initializer determines memory control for the lifetime of the buffer. If
the buffer points to memory of a predetermined and fixed capacity do not
provide an allocation function. If a dynamic buffer is preferred, provide the
allocation function as defined by the signature in types.h. If resizing is
desired on memory that has already been allocated, ensure allocation has
occurred with the provided allocation function. */
#define ccc_buf_init(mem_ptr, alloc_fn, aux_data, capacity, optional_size...)  \
    ccc_impl_buf_init(mem_ptr, alloc_fn, aux_data, capacity, optional_size)

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
                                       ccc_alloc_fn *fn);

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
@return the result of the attempted pop. CCC_OK upon success or an input error
if bad input is provided.
@note this function modifies the size of the container. */
ccc_result ccc_buf_pop_back(ccc_buffer *buf);

/** @brief pop n elements from the back of the buffer according to size.
@param [in] buf the pointer to the buffer.
@param [in] n the number of elements to pop.
@return the result of the attempted pop. CCC_OK if the buffer exists and n
is within the bounds of size. If the buffer does not exist an input error is
returned. If n is greater than the size of the buffer size is set to zero
and input error is returned.
@note this function modifies the size of the container. */
ccc_result ccc_buf_pop_back_n(ccc_buffer *buf, size_t n);

/** @brief erase element at slot i according to size of the buffer maintaining
contiguous storage of elements between 0 and size.
@param [in] buf the pointer to the buffer.
@param [in] i the index of the element to be erased.
@return the result, CCC_OK if the input is valid. If no buffer exists or i is
out of range of size then an input error is returned.
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

/** @brief return the index of an element known to be in the buffer.
@param [in] buf the pointer to the buffer.
@param [in] slot the pointer to the element stored in the buffer.
@return the index if the slot provided is within the capacity range of the
buffer, otherwise -1. */
[[nodiscard]] ptrdiff_t ccc_buf_i(ccc_buffer const *buf, void const *slot);

/** @brief return the final element in the buffer according the current size.
@param [in] buf the pointer to the buffer.
@return the pointer the final element in the buffer according to the current
size or NULL if the buffer does not exist or is empty. */
[[nodiscard]] void *ccc_buf_back(ccc_buffer const *buf);

/** @brief return the first element in the buffer at index 0.
@param [in] buf the pointer to the buffer.
@return the pointer to the front element or NULL if the buffer does not exist
or is empty. */
[[nodiscard]] void *ccc_buf_front(ccc_buffer const *buf);

/** @brief copy data at index src to dst according to capacity.
@param [in] buf the pointer to the buffer.
@param [in] dst the index of destination within bounds of capacity.
@param [in] src the index of source within bounds of capacity.
@return a pointer to the slot at dst or NULL if bad input is provided.
@note this function does NOT modify the size of the container.

Note that destination and source are only required to be valid within bounds
of capacity of the buffer. It is up to the user to ensure destination and
source are within the size bounds of the buffer. */
void *ccc_buf_copy(ccc_buffer *buf, size_t dst, size_t src);

/** @brief write data to buffer at slot at index i according to capacity.
@param [in] buf the pointer to the buffer.
@param [in] i the index within bounds of capacity of the buffer.
@param [in] data the data that will be written to slot at i.
@return the result of the write, CCC_OK if success. If no buffer or data
exists input error is returned. If i is outside of the range of capacity
input error is returned.
@note this function does NOT modify the size of the container.

Note that data will be written to the slot at index i, according to the
capacity of the buffer. It is up to the user to ensure i is within size
of the buffer if such behavior is desired. No elements are moved to be
preserved meaning any data at i is overwritten. */
ccc_result ccc_buf_write(ccc_buffer *buf, size_t i, void const *data);

/** @brief swap elements at i and j according to capacity of the bufer.
@param [in] buf the pointer to the buffer.
@param [in] tmp the pointer to the temporary buffer of the same size as an
element stored in the buffer.
@param [in] i the index of an element in the buffer.
@param [in] j the index of an element in the buffer.
@return the result of the swap, CCC_OK if no error occurs. If no buffer exists,
no tmp exists, i is out of capacity range, or j is out of capacity range, an
input error is returned.
@note this function does NOT modify the size of the container.

Note that i and j are only checked to be within capacity range of the buffer.
It is the user's responsibility to check for i and j within bounds of size
if such behavior is needed. */
ccc_result ccc_buf_swap(ccc_buffer *buf, char tmp[], size_t i, size_t j);

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
@return the result of resizing. CCC_OK if no errors occur or an error
indicating bad input has been provided.

If n would exceed the current capacity of the buffer the size is set to
capacity and the input error status is returned. */
ccc_result ccc_buf_size_plus(ccc_buffer *buf, size_t n);

/** @brief subtract n from the size of the buffer.
@param [in] buf the pointer to the buffer.
@param [in] n the quantity to subtract from the current buffer size.
@return the result of resizing. CCC_OK if no errors occur or an error
indicating bad input has been provided.

If n would reduce the size to less than 0, the buffer size is set to 0 and the
input error status is returned. */
ccc_result ccc_buf_size_minus(ccc_buffer *buf, size_t n);

/** @brief set the buffer size to n.
@param [in] buf the pointer to the buffer.
@param [in] n the new size of the buffer.
@return the result of setting the size. CCC_OK if no errors occur or an error
indicating bad input has been provided.

If n is larger than the capacity of the buffer the size is set equal to the
capacity and an error is returned. */
ccc_result ccc_buf_size_set(ccc_buffer *buf, size_t n);

/** @brief return the current capacity of the buffer.
@param [in] buf the pointer to the buffer.
@return the total number of elements the can be stored in the buffer. This
value remains the same until a resize occurs. */
[[nodiscard]] size_t ccc_buf_capacity(ccc_buffer const *buf);

/** @brief the size of the type being stored contiguously in the buffer.
@param [in] buf the pointer to the buffer.
@return the size of the type being stored in the buffer. */
[[nodiscard]] size_t ccc_buf_elem_size(ccc_buffer const *buf);

/** @brief obtain the size of the buffer.
@param [in] buf the pointer to the buffer.
@return the quantity of elements stored in the buffer.

Note that size must be less than or equal to capacity. */
[[nodiscard]] size_t ccc_buf_size(ccc_buffer const *buf);

/** @brief return true if the size of the buffer is 0.
@param [in] buf the pointer to the buffer.
@return true if the size is 0 false if not. */
[[nodiscard]] bool ccc_buf_is_empty(ccc_buffer const *buf);

/** @brief return true if the size of the buffer equals capacity.
@param [in] buf the pointer to the buffer.
@return true if the size equals the capacity. */
[[nodiscard]] bool ccc_buf_is_full(ccc_buffer const *buf);

/**@}*/

/** Define this preprocessor directive to drop the ccc prefix from all buffer
related types and methods. By default the prefix is required but may be
dropped with this directive if one is sure no namespace collisions occur. */
#ifdef BUFFER_USING_NAMESPACE_CCC
typedef ccc_buffer buffer;
#    define buf_init(args...) ccc_buf_init(args)
#    define buf_alloc(args...) ccc_buf_alloc(args)
#    define buf_size(args...) ccc_buf_size(args)
#    define buf_size_plus(args...) ccc_buf_size_plus(args)
#    define buf_size_minus(args...) ccc_buf_size_minus(args)
#    define buf_size_set(args...) ccc_buf_size_set(args)
#    define buf_capacity(args...) ccc_buf_capacity(args)
#    define buf_elem_size(args...) ccc_buf_elem_size(args)
#    define buf_i(args...) ccc_buf_i(args)
#    define buf_is_full(args...) ccc_buf_is_full(args)
#    define buf_is_empty(args...) ccc_buf_is_empty(args)
#    define buf_at(args...) ccc_buf_at(args)
#    define buf_back(args...) ccc_buf_back(args)
#    define buf_front(args...) ccc_buf_front(args)
#    define buf_alloc_back(args...) ccc_buf_alloc_back(args)
#    define buf_push_back(args...) ccc_buf_push_back(args)
#    define buf_pop_back(args...) ccc_buf_pop_back(args)
#    define buf_pop_back_n(args...) ccc_buf_pop_back_n(args)
#    define buf_copy(args...) ccc_buf_copy(args)
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
