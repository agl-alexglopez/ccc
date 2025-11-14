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
@brief The Bit Set Interface

A bit set offers efficient set membership operations when the range of values
can be tracked via an index. Both a fixed size and dynamic variant are possible
depending on initialization options.

Conceptually, the bit set can be thought of as an arbitrary length integer with
index 0 being the Least Significant Bit and index N - 1 being the Most
Significant Bit of the integer. Internally, this is implemented by populating
each block of the bit set from Least Significant Bit to Most Significant Bit.
Therefore, "trailing" means starting from the Least Significant Bit and
"leading" means starting from the Most Significant Bit; this is done to stay
consistent with upcoming bit operations introduced to the C23 standard.

A bit set can be used for modeling integer operations on integers that exceed
the widths available on one's platform. The provided bitwise operation
functions are helpful for these types of manipulations.

A bit set can also be used for modeling data that can be abstracted to a
position and binary value. For example, disk blocks in a file system, free
blocks in a memory allocator, and many other resource abstractions can benefit
from a bit set. For these use cases the bit set offers efficient functions to
find the first bits set to zero or one from either the trailing or leading
direction. A bit set can also efficiently report if contiguous ranges of zeros
or ones are available.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define BITSET_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `CCC_` prefix. */
#ifndef CCC_BITSET
#define CCC_BITSET

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "private/private_bitset.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief The bit set type that may be stored and initialized on the stack,
heap, or data segment at compile or run time.

A bit set offers fast membership testing and bit range manipulations when the
data can be modeled as a 0-indexed key value data set. In the case of a bit
set the key is the index in the bit set and the value is 1 or 0. Operations on
single bits occur in O(1) time. All scanning operations operate in O(N) time. */
typedef struct CCC_Bitset CCC_Bitset;

/**@}*/

/** @name Container Initialization
Initialize and create containers with memory and permissions. */
/**@{*/

enum : size_t
{
    /** @brief The number of bits in a block of the bit set. */
    CCC_BITSET_BLOCK_BITS = CCC_PRIVATE_BITSET_BLOCK_BITS,
};

/** @brief Get the number of bit blocks needed for the desired bit set capacity.
@param [in] bit_cap the number of bits representing this bit set.
@return the number of blocks needed for the desired bit set.
@warning bit_cap must be >= 1. */
#define CCC_bitset_block_count(bit_cap) CCC_private_bitset_block_count(bit_cap)

/** @brief Get the number of bytes needed for the desired bit set capacity.
@param [in] bit_cap the number of bits representing this bit set.
@return the number of bytes needed to support the bit capacity. This is the
number of bytes occupied by the number of bit blocks that must be allocated. */
#define CCC_bitset_block_bytes(bit_cap) CCC_private_bitset_block_bytes(bit_cap)

/** @brief Allocate the necessary number of blocks at compile or runtime on the
stack or data segment.
@param [in] bit_cap the desired number of bits to store in the bit set.
@param [in] optional_storage_duration an optional storage duration specifier
such as static or automatic.
@return a compound literal array of the necessary bit block type allocated in
the scope it is created with any storage duration specifiers added.

This method can be used for compile time initialization of bit set. For example:

```
static CCC_Bitset b = CCC_bitset_initialize(CCC_bitset_blocks(256, static),
NULL, NULL, 256);
```

The above example also illustrates the benefits of a static compound literal
to encapsulate the bits within the bit set array to prevent dangling references.
If the compiler does not support storage duration of compound literals the more
traditional example follows:

```
static CCC_Bitset b = CCC_bitset_initialize(CCC_bitset_blocks(256), NULL, NULL,
256);
```

This macro is required for any initialization where the bit block memory comes
from the stack or data segment. For one time dynamic reservations of bit block
memory see the CCC_bitset_reserve and CCC_bitset_clear_and_free_reserve
interface. */
#define CCC_bitset_blocks(bit_cap, optional_storage_duration...)               \
    CCC_private_bitset_blocks(bit_cap, optional_storage_duration)

/** @brief Initialize the bit set with memory and allocation permissions.
@param [in] bitblock_pointer the pointer to existing blocks or NULL.
@param [in] allocate the allocation function for a dynamic bit set or NULL.
@param [in] context context data needed for allocation of the bit set.
@param [in] cap the number of bits that will be stored in this bit set.
@param [in] optional_size an optional starting size <= capacity. This value
defaults to the same value as capacity which is appropriate for most cases. For
any case where this is not desirable, set the size manually (for example, a
fixed size bit set that is pushed to dynamically would have a non-zero capacity
and 0 size).
@return the initialized bit set on the right hand side of an equality operator
@warning the user must use the CCC_bitset_blocks macro to help determine the
size of the bitblock array if a fixed size bitblock array is provided at compile
time; the necessary conversion from bits requested to number of blocks required
to store those bits must occur before use. If capacity is zero the helper macro
is not needed.

A fixed size bit set with size equal to capacity.

```
#define BITSET_USING_NAMESPACE_CCC
Bitset bs = bitset_initialize(bitset_blocks(9), NULL, NULL, 9);
```
A fixed size bit set with dynamic push and pop.

```
#define BITSET_USING_NAMESPACE_CCC
Bitset bs = bitset_initialize(bitset_blocks(9), NULL, NULL, 9, 0);
```

A dynamic bit set initialization.

```
#define BITSET_USING_NAMESPACE_CCC
Bitset bs = bitset_initialize(NULL, std_allocate, NULL, 0);
```

See types.h for more on allocation functions. */
#define CCC_bitset_initialize(bitblock_pointer, allocate, context, cap,        \
                              optional_size...)                                \
    CCC_private_bitset_initialize(bitblock_pointer, allocate, context, cap,    \
                                  optional_size)

/** @brief Initialize the bit set with a custom input string.
@param [in] allocate the allocation function for the dynamic bit set.
@param [in] context context data needed for allocation of the bit set.
@param [in] start_string_index the index of the input string to start reading
is CCC_Tribool input.
@param [in] count number of characters to read from start_string_index.
@param [in] bit_on_char the character that when encountered equates to CCC_TRUE
and results in the corresponding bit in the bit set being set CCC_TRUE. Any
other character encountered results in the corresponding bit being set to
CCC_FALSE.
@param [in] input_string the string literal or pointer to a string.
@param [in] optional_capacity the custom capacity other than the count passed to
this initializer. If a greater capacity than the input string is desired because
more bits will be pushed later, specify with this input. If this input is less
than count, count becomes the capacity.
@return the initialized bit set on the right hand side of an equality operator
with count bits pushed. If the string ends early due to a null terminator, the
count will be less than that passed as input. This can be checked by checking
the CCC_bitset_count() function.
@warning the input string is assumed to adhere to the count. If the string is
shorter than the count input and not null terminated the behavior is undefined.

A dynamic bit set with input string pushed.

```
#define BITSET_USING_NAMESPACE_CCC
Bitset bs = bitset_from(std_allocate, NULL, 0, 4, '1', "1011");
```
A dynamic bit set that allocates greater capacity.

```
#define BITSET_USING_NAMESPACE_CCC
Bitset bs = bitset_from(std_allocate, NULL, 0, 4, 'A', "GCAT", 4096);
```

This initializer is only available to dynamic bit sets due to the inability to
run such input code at compile time. */
#define CCC_bitset_from(allocate, context, start_string_index, count,          \
                        bit_on_char, input_string, optional_capacity...)       \
    CCC_private_bitset_from(allocate, context, start_string_index, count,      \
                            bit_on_char, input_string, optional_capacity)

/** @brief Initialize the bit set with a starting capacity and size at runtime.
@param [in] allocate the allocation function for a dynamic bit.
@param [in] context context data needed for allocation of the bit set.
@param [in] capacity the number of bits that will be stored in this bit set.
@param [in] optional_size an optional starting size <= capacity. This value
defaults to the same value as capacity which is appropriate for most cases. For
any case where this is not desirable, set the size manually (for example, a
bit set that will push bits back would have a non-zero capacity and 0 size).
@return the initialized bit set on the right hand side of an equality operator

A fixed size bit set with size equal to capacity.

```
#define BITSET_USING_NAMESPACE_CCC
int
main(void)
{
    Bitset bs = bitset_with_capacity(std_allocate, NULL, 4096);
}
```
A bit set with dynamic push and pop.

```
#define BITSET_USING_NAMESPACE_CCC
int
main(void)
{
    Bitset bs = bitset_with_capacity(std_allocate, NULL, 4096, 0);
}
```

This initialization can only be used at runtime. See the normal initializer for
static and stack based initialization options. */
#define CCC_bitset_with_capacity(allocate, context, capacity,                  \
                                 optional_size...)                             \
    CCC_private_bitset_with_capacity(allocate, context, capacity, optional_size)

/** @brief Copy the bit set at source to destination.
@param [in] dst the initialized destination for the copy of the src set.
@param [in] src the initialized source of the set.
@param [in] fn the optional allocation function if resizing is needed.
@return the result of the copy operation. If the destination capacity is
less than the source capacity and no allocation function is provided an
input error is returned. If resizing is required and resizing of dst fails a
memory error is returned.
@note dst must have capacity greater than or equal to src. If dst capacity
is less than src, an allocation function must be provided with the fn
argument.

Note that there are two ways to copy data from source to destination:
provide sufficient memory and pass NULL as fn, or allow the copy function to
take care of allocation for the copy.

Manual memory management with no allocation function provided.

```
#define BITSET_USING_NAMESPACE_CCC
static Bitset src = bitset_initialize(bitset_blocks(11, static), NULL, NULL,
11); set_rand_bits(&src); static Bitset src =
bitset_initialize(bitset_blocks(13, static), NULL, NULL, 13); CCC_Result res =
bitset_copy(&dst, &src, NULL);
```

The above requires dst capacity be greater than or equal to src capacity.
Here is memory management handed over to the copy function.

```
#define BITSET_USING_NAMESPACE_CCC
static Bitset src = bitset_initialize(NULL, std_allocate, NULL, 0);
push_rand_bits(&src);
static Bitset src = bitset_initialize(NULL, std_allocate, NULL, 0);
CCC_Result res = bitset_copy(&dst, &src, std_allocate);
```

The above allows dst to have a capacity less than that of the src as long as
copy has been provided an allocation function to resize dst. Note that this
would still work if copying to a destination that the user wants as a fixed
size map.

```
#define BITSET_USING_NAMESPACE_CCC
static Bitset src = bitset_initialize(NULL, std_allocate, NULL, 0);
push_rand_bits(&src);
static Bitset src = bitset_initialize(NULL, NULL, NULL, 0);
CCC_Result res = bitset_copy(&dst, &src, std_allocate);
```

The above sets up dst with fixed size while src is a dynamic map. Because an
allocation function is provided, the dst is resized once for the copy and
retains its fixed size after the copy is complete. This would require the
user to manually free the underlying Buffer at dst eventually if this method
is used. Usually it is better to allocate the memory explicitly before the
copy if copying between maps without allocation permission.

These options allow users to stay consistent across containers with their
memory management strategies. */
CCC_Result CCC_bitset_copy(CCC_Bitset *dst, CCC_Bitset const *src,
                           CCC_Allocator *fn);

/** @brief Reserves space for at least to_add more bits.
@param [in] bs a pointer to the bit set.
@param [in] to_add the number of elements to add to the current
size.
@param [in] fn the allocation function to use to reserve
memory.
@return the result of the reservation. OK if successful,
otherwise an error status is returned.
@note see the CCC_bitset_clear_and_free_reserve function if this
function is being used for a one-time dynamic reservation.

This function can be used for a dynamic bit set with or without
allocation permission. If the bit set has allocation
permission, it will reserve the required space and later resize
if more space is needed.

If the bit set has been initialized with no allocation
permission and no memory this function can serve as a one-time
reservation. This is helpful when a fixed size is needed but
that size is only known dynamically at runtime. To free the bit
set in such a case see the CCC_bitset_clear_and_free_reserve
function. */
CCC_Result CCC_bitset_reserve(CCC_Bitset *bs, size_t to_add, CCC_Allocator *fn);

/**@}*/

/** @name Bit Membership Interface
Test for the presence of bits. */
/**@{*/

/** @brief Test the bit at index i for boolean status (CCC_TRUE
or CCC_FALSE).
@param [in] bs a pointer to the bit set.
@param [in] i the index identifying the bit to set.
@return the state of the bit, or CCC_TRIBOOL_ERROR if bs is
NULL or i is out of range. */
CCC_Tribool CCC_bitset_test(CCC_Bitset const *bs, size_t i);

/**@}*/

/** @name Bit Modification Interface
Set and flip bits in the set. */
/**@{*/

/** @brief Set the bit at valid index i to value b (true or
false).
@param [in] bs a pointer to the bit set.
@param [in] i the valid index identifying the bit to set.
@param [in] b the value to set at position i (CCC_TRUE or
CCC_FALSE).
@return the state of the bit before the set operation, true if
it was previously true, false if it was previously false, or
error if bs is NULL or i is out of range. */
CCC_Tribool CCC_bitset_set(CCC_Bitset *bs, size_t i, CCC_Tribool b);

/** @brief Set all the bits to the provided value (CCC_TRUE or
CCC_FALSE).
@param [in] bs a pointer to the bit set.
@param [in] b the value to set (CCC_TRUE or CCC_FALSE).
@return the result of the operation. OK if successful, or an
input error if bs is NULL. */
CCC_Result CCC_bitset_set_all(CCC_Bitset *bs, CCC_Tribool b);

/** @brief Set all the bits in the specified range starting at
the Least Significant Bit of the range and ending at the Most
Significant Bit of the range (CCC_TRUE or CCC_FALSE).
@param [in] bs a pointer to the bit set.
@param [in] b the value to set (CCC_TRUE or CCC_FALSE).
@param [in] i the starting index to set.
@param [in] count the count of bits starting at i to set.
@return the result of the operation. OK if successful, or an
input error if bs is NULL or the range is invalid by position,
count, or both.

Note that a range is defined from i to i + count, where i +
count is the exclusive end of the range. This is equivalent to
moving from Least to Most Significant bit in an integer. */
CCC_Result CCC_bitset_set_range(CCC_Bitset *bs, size_t i, size_t count,
                                CCC_Tribool b);

/** @brief Set the bit at valid index i to boolean value b
(true or false).
@param [in] bs a pointer to the bit set.
@param [in] i the valid index identifying the bit to set.
@return the state of the bit before the set operation, true if
it was previously true, false if it was previously false, or
error if bs is NULL or i is out of range. */
CCC_Tribool CCC_bitset_reset(CCC_Bitset *bs, size_t i);

/** @brief Set all the bits to CCC_FALSE.
@param [in] bs a pointer to the bit set.
@return the result of the operation. OK if successful, or an
input error if bs is NULL. */
CCC_Result CCC_bitset_reset_all(CCC_Bitset *bs);

/** @brief Set all the bits in the specified range starting at
the Least Significant Bit of the range and ending at the Most
Significant Bit of the range to CCC_FALSE.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to reset.
@param [in] count the count of bits starting at i to reset.
@return the result of the operation. OK if successful, or an
input error if bs is NULL or the range is invalid by position,
count, or both.

Note that a range is defined from i to i + count, where i +
count is the exclusive end of the range. This is equivalent to
moving from Least to Most Significant bit in an integer. */
CCC_Result CCC_bitset_reset_range(CCC_Bitset *bs, size_t i, size_t count);

/** @brief Toggle the bit at index i.
@param [in] bs a pointer to the bit set.
@param [in] i the index identifying the bit to toggle
@return the state of the bit before the toggle operation, true
if it was previously true, false if it was previously false, or
error if bs is NULL or i is out of range. */
CCC_Tribool CCC_bitset_flip(CCC_Bitset *bs, size_t i);

/** @brief Toggle all of the bits to their opposing boolean
value.
@param [in] bs a pointer to the bit set.
@return the result of the operation. OK if successful, or an
input error if bs is NULL. */
CCC_Result CCC_bitset_flip_all(CCC_Bitset *bs);

/** @brief Flip all the bits in the range, starting at the
Least Significant Bit in range and ending at the Most
Significant Bit, to their opposite value.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to reset.
@param [in] count the count of bits starting at i to reset.
@return the result of the operation. OK if successful, or an
input error if bs is NULL or the range is invalid by position,
count, or both.

Note that a range is defined from i to i + count, where i +
count is the exclusive end of the range. This is equivalent to
moving from Least to Most Significant Bit in an integer. */
CCC_Result CCC_bitset_flip_range(CCC_Bitset *bs, size_t i, size_t count);

/**@}*/

/** @name Bit Scan Interface
Find bits with a specific status. */
/**@{*/

/** @brief Return true if any bits in set are 1.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if any bits are 1, CCC_FALSE if no bits are 1,
CCC_TRIBOOL_ERROR if bs is NULL. */
CCC_Tribool CCC_bitset_any(CCC_Bitset const *bs);

/** @brief Return true if any bits are 1 in the specified
range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return CCC_TRUE if any bits are 1, CCC_FALSE if no bits are 1,
CCC_TRIBOOL_ERROR if bs is NULL, i is invalid, count is
invalid, or both i and count are invalid. */
CCC_Tribool CCC_bitset_any_range(CCC_Bitset const *bs, size_t i, size_t count);

/** @brief Return true if all bits are set to 0.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if all bits are 0, CCC_FALSE if any bits are
1, CCC_TRIBOOL_ERROR if bs is NULL. */
CCC_Tribool CCC_bitset_none(CCC_Bitset const *bs);

/** @brief Return true if all bits are 0 in the specified
range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return CCC_TRUE if all bits are 0, CCC_FALSE if any bits are
1, CCC_TRIBOOL_ERROR if bs is NULL, i is invalid, count is
invalid, or both i and count are invalid. */
CCC_Tribool CCC_bitset_none_range(CCC_Bitset const *bs, size_t i, size_t count);

/** @brief Return true if all bits in set are 1.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if all bits are 1, CCC_FALSE if any bits are
0, CCC_TRIBOOL_ERROR if bs is NULL. */
CCC_Tribool CCC_bitset_all(CCC_Bitset const *bs);

/** @brief Return true if all bits are set to 1 in the
specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return CCC_TRUE if all bits are 1, CCC_FALSE if any bits are
0, CCC_TRIBOOL_ERROR if bs is NULL, i is invalid, count is
invalid, or both i and count are invalid. */
CCC_Tribool CCC_bitset_all_range(CCC_Bitset const *bs, size_t i, size_t count);

/** @brief Return the index of the first trailing bit set to 1
in the set.
@param [in] bs a pointer to the bit set.
@return an OK(0) status and index of the first trailing bit set
to 1 or an error set to CCC_RESULT_FAIL if no 1 bit is found.
Argument error is set if bs is NULL. */
CCC_Count CCC_bitset_first_trailing_one(CCC_Bitset const *bs);

/** @brief Return the index of the first trailing bit set to 1
in the range
`[i, i + count)`.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@return an OK(0) status and the index of the first trailing bit
set to 1 or CCC_RESULT_FAIL if no 1 bit is found. Argument
error is returned bs is NULL, or the range is invalid. */
CCC_Count CCC_bitset_first_trailing_one_range(CCC_Bitset const *bs, size_t i,
                                              size_t count);

/** @brief Returns the index of the start of the first trailing
num_ones contiguous 1 bits.
@param [in] bs a pointer to the bit set.
@param [in] num_ones the number of trailing contiguous 1 bits
to find.
@return an OK(0) status and the index in a search starting from
the Least Significant Bit of the set of the first 1 in a
sequence of num_ones 1 bits. If such a sequence cannot be found
CCC_RESULT_FAIL result error is set. If bs is NULL or num_ones
is too large an argument error is set. */
CCC_Count CCC_bitset_first_trailing_ones(CCC_Bitset const *bs, size_t num_ones);

/** @brief Returns the index of the start of the first trailing
num_ones contiguous 1 bits in the range `[i, i + count)`.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@param [in] num_ones the number of trailing contiguous 1 bits
to find.
@return an OK(0) status and the starting index of the first 1
in a sequence of num_ones 1 bits. If no range is found
CCC_RESULT_FAIL error is set. If bs is NULL or arguments are
out of range an argument error is set. */
CCC_Count CCC_bitset_first_trailing_ones_range(CCC_Bitset const *bs, size_t i,
                                               size_t count, size_t num_ones);

/** @brief Return the index of the first bit set to 0 in the
set.
@param [in] bs a pointer to the bit set.
@return an OK(0) status and the index of the first bit set to 0
or CCC_RESULT_FAIL if no 0 bit is found. If bs is NULL an
argument error is set. */
CCC_Count CCC_bitset_first_trailing_zero(CCC_Bitset const *bs);

/** @brief Return the index of the first bit set to 0 in the
range
`[i, i + count)`.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@return an OK(0) status and the index of the first bit set to 0
or CCC_RESULT_FAIL if no 0 bit is found. If bs is NULL, or the
range is invalid, an argument error is set. */
CCC_Count CCC_bitset_first_trailing_zero_range(CCC_Bitset const *bs, size_t i,
                                               size_t count);

/** @brief Returns the index of the start of the first trailing
num_zeros contiguous 0 bits in the set.
@param [in] bs a pointer to the bit set.
@param [in] num_zeros the number of trailing contiguous 0 bits
to find.
@return an OK(0) status and the index in a search, starting
from the Least Significant Bit of the set, of the first 0 in a
sequence of num_zeros 0 bits. If such a sequence cannot be
found CCC_RESULT_FAIL is returned. If bs is NULL or num zeros
is too large an argument error is set. */
CCC_Count CCC_bitset_first_trailing_zeros(CCC_Bitset const *bs,
                                          size_t num_zeros);

/** @brief Returns the index of the start of the first trailing
num_zeros contiguous 0 bits in the range `[i, i + count)`.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@param [in] num_zeros the number of trailing contiguous 0 bits
to find.
@return the index in a search, starting from the Least
Significant Bit of the range, of the first 0 in a sequence of
num_zeros 0 bits. If the input is invalid or such a sequence
cannot be found CCC_RESULT_FAIL is returned. */
CCC_Count CCC_bitset_first_trailing_zeros_range(CCC_Bitset const *bs, size_t i,
                                                size_t count, size_t num_zeros);

/** @brief Return the index of the first leading bit set to 1
in the set, starting from the Most Significant Bit at index
size - 1.
@param [in] bs a pointer to the bit set.
@return the index of the first leading bit set to 1 or
CCC_RESULT_FAIL if no 1 bit is found or bs in NULL. */
CCC_Count CCC_bitset_first_leading_one(CCC_Bitset const *bs);

/** @brief Return the index of the first leading bit set to 1
in the range
`[i, i - count)`.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check from i towards
index 0.
@return the index of the first leading bit set to 1 or
CCC_RESULT_FAIL if no 1 bit is found, bs is NULL, or the range
is invalid. */
CCC_Count CCC_bitset_first_leading_one_range(CCC_Bitset const *bs, size_t i,
                                             size_t count);

/** @brief Returns the index of the start of the first leading
num_ones contiguous 1 bits.
@param [in] bs a pointer to the bit set.
@param [in] num_ones the number of leading contiguous 1 bits to
find.
@return the index in a search starting from the Least
Significant Bit of the set of the first 1 in a sequence of
num_ones 1 bits. If the input is invalid or such a sequence
cannot be found CCC_RESULT_FAIL is returned. */
CCC_Count CCC_bitset_first_leading_ones(CCC_Bitset const *bs, size_t num_ones);

/** @brief Returns the index of the start of the first leading
num_ones contiguous 1 bits in the range `[i, i - count)`.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@param [in] num_ones the number of leading contiguous 1 bits to
find.
@return an OK(0) status and the index in a search starting from
the Most Significant Bit of the range of the first 1 in a
sequence of num_ones 1 bits. If such a sequence cannot be found
CCC_RESULT_FAIL is set. If bs is NULL or any argument is out of
range an argument error is set. */
CCC_Count CCC_bitset_first_leading_ones_range(CCC_Bitset const *bs, size_t i,
                                              size_t count, size_t num_ones);

/** @brief Return the index of the first leading bit set to 0
in the set, starting from the Most Significant Bit at index
size - 1.
@param [in] bs a pointer to the bit set.
@return an OK(0) status and the index of the first bit set to 0
or CCC_RESULT_FAIL if no 1 bit is found. If bs in NULL an
argument error is set. */
CCC_Count CCC_bitset_first_leading_zero(CCC_Bitset const *bs);

/** @brief Return the index of the first leading bit set to 0
in the range
`[i, i - count)`.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search for a 0 bit.
@param [in] count size to search from Most Significant Bit to
Least in range.
@return an OK(0) status and the index of the first bit set to 0
in the specified range CCC_RESULT_FAIL if no 0 bit is found. If
bs in NULL an argument error is set. */
CCC_Count CCC_bitset_first_leading_zero_range(CCC_Bitset const *bs, size_t i,
                                              size_t count);

/** @brief Returns the index of the start of the first leading
num_zeros contiguous 0 bits.
@param [in] bs a pointer to the bit set.
@param [in] num_zeros the number of leading contiguous 0 bits
to find.
@return an OK(0) status and the index in a search, starting
from the Most Significant Bit of the set, of the first 0 in a
sequence of num_zeros 0 bits. If such a sequence cannot be
found CCC_RESULT_FAIL is set. If bs is NULL or num_zeros is too
large an argument error is set. */
CCC_Count CCC_bitset_first_leading_zeros(CCC_Bitset const *bs,
                                         size_t num_zeros);

/** @brief Returns the index of the start of the first leading
num_zeros contiguous 0 bits in the range `[i, i - count)`.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@param [in] num_zeros the number of leading contiguous 0 bits
to find.
@return an OK(0) status and the index in a search, starting
from the Most Significant Bit of the range, of the first 0 in a
sequence of num_zeros 0 bits. If such a sequence cannot be
found CCC_RESULT_FAIL is returned. If bs is NULL or the
arguments are out of range an argument error is set. */
CCC_Count CCC_bitset_first_leading_zeros_range(CCC_Bitset const *bs, size_t i,
                                               size_t count, size_t num_zeros);

/**@}*/

/** @name Bit Operations Interface
Use standard integer width bit operations on the entire set. */
/**@{*/

/** @brief Bitwise OR dst set with src set.
@param [in] dst the set to modified with the OR operation.
@param [in] src the set to be read as the source bits for the
OR operation.
@return an OK result if the operation is successful or an input
error if dst or src are NULL.

Note that sets are aligned from their Least Significant Bit and
a smaller src set is conceptually padded with 0's in its higher
order bits to align with the larger dst set (no modifications
to the smaller set are performed to achieve this). This is done
to stay consistent with how the operation would work on a
smaller integer being stored in a larger integer to align with
the larger. */
CCC_Result CCC_bitset_or(CCC_Bitset *dst, CCC_Bitset const *src);

/** @brief Bitwise AND dst set with src set.
@param [in] dst the set to modified with the AND operation.
@param [in] src the set to be read as the source bits for the
AND operation.
@return an OK result if the operation is successful or an input
error if dst or src are NULL.
@warning sets behave identically to integers for the AND
operation when widening occurs. If dst is larger than src, src
is padded with zeros in its Most Significant Bits. This means a
bitwise AND operations will occur with the higher order bits in
dst and 0's in this padded range (this results in all bits in
dst being set to 0 in that range).

Note that sets are aligned from their Least Significant Bit and
a smaller src set is conceptually padded with 0's in its higher
order bits to align with the larger dst set (no modifications
to the smaller set are performed to achieve this). This is done
to stay consistent with integer promotion and widening rules in
C. */
CCC_Result CCC_bitset_and(CCC_Bitset *dst, CCC_Bitset const *src);

/** @brief Bitwise XOR dst set with src set.
@param [in] dst the set to modified with the XOR operation.
@param [in] src the set to be read as the source bits for the
XOR operation.
@return an OK result if the operation is successful or an input
error if dst or src are NULL.

Note that sets are aligned from their Least Significant Bit and
a smaller src set is conceptually padded with 0's in its higher
order bits to align with the larger dst set (no modifications
to the smaller set are performed to achieve this). This is done
to stay consistent with how the operation would work on a
smaller integer being stored in a larger integer to align with
the larger. */
CCC_Result CCC_bitset_xor(CCC_Bitset *dst, CCC_Bitset const *src);

/** @brief Shift the bit set left by left_shifts amount.
@param [in] bs a pointer to the bit set.
@param [in] left_shifts the number of left shifts to perform.
@return an ok result if the operation was successful or an
error if the Bitset is NULL.

Note that if the number of shifts is greater than the bit set
size the bit set is zeroed out rather than exhibiting undefined
behavior as in the equivalent integer operation. */
CCC_Result CCC_bitset_shiftl(CCC_Bitset *bs, size_t left_shifts);

/** @brief Shift the bit set right by right_shifts amount.
@param [in] bs a pointer to the bit set.
@param [in] right_shifts the number of right shifts to perform.
@return an ok result if the operation was successful or an
error if the Bitset is NULL.

Note that if the number of shifts is greater than the bit set
size the bit set is zeroed out rather than exhibiting undefined
behavior as in the equivalent integer operation. */
CCC_Result CCC_bitset_shiftr(CCC_Bitset *bs, size_t right_shifts);

/** @brief Checks two bit sets of the same size (not capacity)
for equality.
@param [in] a pointer to a bit set.
@param [in] b pointer to another bit set of equal size.
@return true if the bit sets are of equal size with identical
bit values at every position, false if the sets are different
sizes or have mismatched bits. A bool error is returned if
either pointer is NULL. */
CCC_Tribool CCC_bitset_eq(CCC_Bitset const *a, CCC_Bitset const *b);

/**@}*/

/** @name Set Operations Interface
Perform basic mathematical set operations on the bit set. */
/**@{*/

/** @brief Return CCC_TRUE if subset is a proper subset of set
(subset ⊂ set).
@param [set] the set to check subset against.
@param [subset] the subset to confirm as a proper subset of
set.
@return CCC_TRUE if all bit positions in subset share the same
value--0 or 1--as the bit positions in set and set is of
greater size than subset. A CCC_TRIBOOL_ERROR is returned if
set or subset is NULL.

If set is of size 0 the function returns CCC_FALSE regardless
of the size of subset. If set is not of size 0 and subset is of
size 0 the function returns CCC_TRUE. */
CCC_Tribool CCC_bitset_is_proper_subset(CCC_Bitset const *set,
                                        CCC_Bitset const *subset);

/** @brief Return CCC_TRUE if subset is a subset of set (subset
⊆ set).
@param [set] the set to check subset against.
@param [subset] the subset to confirm as a subset of set.
@return CCC_TRUE if all bit positions in subset share the same
value--0 or 1--as the bit positions in set. A CCC_TRIBOOL_ERROR
is returned if set or subset is NULL.

If set is size zero subset must also be of size 0 to return
CCC_TRUE. If subset is size 0 the function returns CCC_TRUE
regardless of the size of set. */
CCC_Tribool CCC_bitset_is_subset(CCC_Bitset const *set,
                                 CCC_Bitset const *subset);

/**@}*/

/** @name State Interface
Obtain state from the container. */
/**@{*/

/** @brief Return a reference to the base of the underlying
block array.
@param [in] bs a pointer to the bit set.
@return a reference to the base of the first block of the bit
set block array or NULL if bs is NULL or has no capacity.

Every block populates bits from Least Significant Bit (LSB) to
Most Significant Bit (MSB) so this reference is to the base or
LSB of the entire set. */
void *CCC_bitset_data(CCC_Bitset const *bs);

/** @brief Return total number of bits the capacity of the set
can hold.
@param [in] bs a pointer to the bit set.
@return an OK(0) status and the capacity of bits capable of
being stored in the current set. If bs is NULL an argument
error is set. */
CCC_Count CCC_bitset_capacity(CCC_Bitset const *bs);

/** @brief Return total number of bits actively tracked by the
user and set.
@param [in] bs a pointer to the bit set.
@return an OK(0) status and the total number of bits currently
tracked by the set regardless of true or false state of each.
If bs is NULL an argument error is set. */
CCC_Count CCC_bitset_count(CCC_Bitset const *bs);

/** @brief Return number of CCC_bitblocks used by bit set for
capacity bits.
@param [in] bs a pointer to the bit set.
@return an OK(0) status and the capacity in number of bit
blocks used to hold the entire capacity of bits in the set. If
bs is NULL an argument error is set.

Capacity may be greater than or equal to size. */
CCC_Count CCC_bitset_blocks_capacity(CCC_Bitset const *bs);

/** @brief Return number of CCC_bitblocks used by the bit set
for size bits.
@param [in] bs a pointer to the bit set.
@return size in number of bit blocks used to hold the current
size of bits in the set. An argument error is set if bs is
NULL.

Size may be less than or equal to capacity. */
CCC_Count CCC_bitset_blocks_count(CCC_Bitset const *bs);

/** @brief Return true if no bits are actively tracked by the
user and set.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if the size of the set is 0 meaning no bits,
regardless of 0 or 1 status, are tracked by the set.
CCC_TRIBOOL_ERROR is returned if bs is NULL.
@warning if the number of bits set to 1 is desired see
CCC_bitset_popcount. */
CCC_Tribool CCC_bitset_empty(CCC_Bitset const *bs);

/** @brief Return the number of bits set to CCC_TRUE. O(n).
@param [in] bs a pointer to the bit set.
@return the total number of bits currently set to CCC_TRUE.
CCC_RESULT_FAIL is returned if bs is NULL. */
CCC_Count CCC_bitset_popcount(CCC_Bitset const *bs);

/** @brief Return the number of bits set to CCC_TRUE in the
range. O(n).
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return the total number of bits currently set in the range to
CCC_TRUE. An argument error is set if bs is NULL, i is invalid,
count is invalid, or both i and count are invalid. */
CCC_Count CCC_bitset_popcount_range(CCC_Bitset const *bs, size_t i,
                                    size_t count);

/**@}*/

/** @name Destructor Interface
Clear the set and manage its memory. */
/**@{*/

/** @brief Clears the bit set by setting the size to 0 and all
bits to 0. The underlying memory capacity remains unchanged.
@param [in] bs a pointer to the bit set.
@return the result of the clear operation, error is returned if
bs is NULL . */
CCC_Result CCC_bitset_clear(CCC_Bitset *bs);

/** @brief Clears the bit set by setting the size to 0 and
freeing the underlying memory. Capacity becomes 0 as well.
@param [in] bs a pointer to the bit set.
@return the result of the clear operation, error is returned if
bs is NULL or the set cannot be freed because no allocation
function was provided upon initialization. */
CCC_Result CCC_bitset_clear_and_free(CCC_Bitset *bs);

/** @brief Frees the Buffer for the bit set that was previously
dynamically reserved with the reserve function.
@param [in] bs the bs to be cleared.
@param [in] fn the required allocation function to provide to a
dynamically reserved bs. Any context data provided upon
initialization will be passed to the allocation function when
called.
@return the result of free operation. OK if success, or an
error status to indicate the error.
@warning It is an error to call this function on a bs that was
not reserved with the provided CCC_Allocator. The bs must
have existing memory to free.

This function covers the edge case of reserving a dynamic
capacity for a bs at runtime but denying the bs allocation
permission to resize. This can help prevent a bs from growing
unbounded. The user in this case knows the bs does not have
allocation permission and therefore no further memory will be
dedicated to the bs.

However, to free the bs in such a case this function must be
used because the bs has no ability to free itself. Just as the
allocation function is required to reserve memory so to is it
required to free memory.

This function will work normally if called on a bs with
allocation permission however the normal CCC_bitset_clear_and_free
is sufficient for that use case. */
CCC_Result CCC_bitset_clear_and_free_reserve(CCC_Bitset *bs, CCC_Allocator *fn);

/**@}*/

/** @name Dynamic Interface
Allows adding to and removing from the set. */
/**@{*/

/** @brief Append a bit value to the set as the new Most
Significant Bit.
@param [in] bs a pointer to the bit set.
@param [in] b the value to push at the Most Significant Bit
CCC_TRUE/CCC_FALSE.
@return the result of the operation, ok if successful or an
error if bad parameters are provided or resizing is required to
accommodate a new bit but resizing fails. */
CCC_Result CCC_bitset_push_back(CCC_Bitset *bs, CCC_Tribool b);

/** @brief Remove the Most Significant Bit from the set.
@param [in] bs a pointer to the bit set.
@return the previous value of the Most Significant Bit
(CCC_TRUE or CCC_FALSE) or a bool error if bs is NULL or bs is
empty. */
CCC_Tribool CCC_bitset_pop_back(CCC_Bitset *bs);

/**@}*/

/** Define this preprocessor macro if shorter names are desired
for the bit set container. Check for namespace clashes before
name shortening. */
#ifdef BITSET_USING_NAMESPACE_CCC
typedef CCC_Bitset Bitset;
#    define BITSET_BLOCK_BITS CCC_BITSET_BLOCK_BITS
#    define bitset_block_count(args...) CCC_bitset_block_count(args)
#    define bitset_block_bytes(args...) CCC_bitset_block_bytes(args)
#    define bitset_blocks(args...) CCC_bitset_blocks(args)
#    define bitset_initialize(args...) CCC_bitset_initialize(args)
#    define bitset_from(args...) CCC_bitset_from(args)
#    define bitset_copy(args...) CCC_bitset_copy(args)
#    define bitset_reserve(args...) CCC_bitset_reserve(args)
#    define bitset_test(args...) CCC_bitset_test(args)
#    define bitset_set(args...) CCC_bitset_set(args)
#    define bitset_set_all(args...) CCC_bitset_set_all(args)
#    define bitset_set_range(args...) CCC_bitset_set_range(args)
#    define bitset_reset(args...) CCC_bitset_reset(args)
#    define bitset_reset_all(args...) CCC_bitset_reset_all(args)
#    define bitset_reset_range(args...) CCC_bitset_reset_range(args)
#    define bitset_flip(args...) CCC_bitset_flip(args)
#    define bitset_flip_all(args...) CCC_bitset_flip_all(args)
#    define bitset_flip_range(args...) CCC_bitset_flip_range(args)
#    define bitset_any(args...) CCC_bitset_any(args)
#    define bitset_any_range(args...) CCC_bitset_any_range(args)
#    define bitset_none(args...) CCC_bitset_none(args)
#    define bitset_none_range(args...) CCC_bitset_none_range(args)
#    define bitset_all(args...) CCC_bitset_all(args)
#    define bitset_all_range(args...) CCC_bitset_all_range(args)
#    define bitset_first_trailing_one(args...)                                 \
        CCC_bitset_first_trailing_one(args)
#    define bitset_first_trailing_one_range(args...)                           \
        CCC_bitset_first_trailing_one_range(args)
#    define bitset_first_trailing_ones(args...)                                \
        CCC_bitset_first_trailing_ones(args)
#    define bitset_first_trailing_ones_range(args...)                          \
        CCC_bitset_first_trailing_ones_range(args)
#    define bitset_first_trailing_zero(args...)                                \
        CCC_bitset_first_trailing_zero(args)
#    define bitset_first_trailing_zero_range(args...)                          \
        CCC_bitset_first_trailing_zero_range(args)
#    define bitset_first_trailing_zeros(args...)                               \
        CCC_bitset_first_trailing_zeros(args)
#    define bitset_first_trailing_zeros_range(args...)                         \
        CCC_bitset_first_trailing_zeros_range(args)
#    define bitset_first_leading_one(args...) CCC_bitset_first_leading_one(args)
#    define bitset_first_leading_one_range(args...)                            \
        CCC_bitset_first_leading_one_range(args)
#    define bitset_first_leading_ones(args...)                                 \
        CCC_bitset_first_leading_ones(args)
#    define bitset_first_leading_ones_range(args...)                           \
        CCC_bitset_first_leading_ones_range(args)
#    define bitset_first_leading_zero(args...)                                 \
        CCC_bitset_first_leading_zero(args)
#    define bitset_first_leading_zero_range(args...)                           \
        CCC_bitset_first_leading_zero_range(args)
#    define bitset_first_leading_zeros(args...)                                \
        CCC_bitset_first_leading_zeros(args)
#    define bitset_first_leading_zeros_range(args...)                          \
        CCC_bitset_first_leading_zeros_range(args)
#    define bitset_or(args...) CCC_bitset_or(args)
#    define bitset_and(args...) CCC_bitset_and(args)
#    define bitset_xor(args...) CCC_bitset_xor(args)
#    define bitset_shiftl(args...) CCC_bitset_shiftl(args)
#    define bitset_shiftr(args...) CCC_bitset_shiftr(args)
#    define bitset_eq(args...) CCC_bitset_eq(args)
#    define bitset_is_proper_subset(args...) CCC_bitset_is_proper_subset(args)
#    define bitset_is_subset(args...) CCC_bitset_is_subset(args)
#    define bitset_data(args...) CCC_bitset_data(args)
#    define bitset_capacity(args...) CCC_bitset_capacity(args)
#    define bitset_blocks_capacity(args...) CCC_bitset_blocks_capacity(args)
#    define bitset_count(args...) CCC_bitset_count(args)
#    define bitset_blocks_count(args...) CCC_bitset_blocks_count(args)
#    define bitset_empty(args...) CCC_bitset_empty(args)
#    define bitset_popcount(args...) CCC_bitset_popcount(args)
#    define bitset_popcount_range(args...) CCC_bitset_popcount_range(args)
#    define bitset_clear(args...) CCC_bitset_clear(args)
#    define bitset_clear_and_free(args...) CCC_bitset_clear_and_free(args)
#    define bitset_clear_and_free_reserve(args...)                             \
        CCC_bitset_clear_and_free_reserve(args)
#    define bitset_push_back(args...) CCC_bitset_push_back(args)
#    define bitset_pop_back(args...) CCC_bitset_pop_back(args)
#endif /* BITSET_USING_NAMESPACE_CCC */

#endif /* CCC_BITSET */
