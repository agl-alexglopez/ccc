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

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_BITSET
#define CCC_BITSET

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "impl/impl_bitset.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief The bit set type that may be stored and initialized on the stack,
heap, or data segment at compile time or runtime.

A bit set offers fast membership testing and bit range manipulations when the
data can be modeled as a 0-indexed key value data set. In the case of a bit
set the key is the index in the bit set and the value is 1 or 0 depending on
how the bit has been set. Operations on single bits occur in O(1) time. All
scanning operations operate in O(N) time. */
typedef struct ccc_bitset_ ccc_bitset;

/** @brief The type used to efficiently store bits in the bit set.

A block is a pre-determined integer width that allows for efficient block sized
bit operations for various scanning and setting tasks. The user must participate
in the storage of the bit set by using the provided ccc_bs_blocks macro to
determine how many blocks are needed for the desired bits in the bit set. */
typedef ccc_bitblock_ ccc_bitblock;

/**@}*/

/** @name Container Initialization
Initialize and create containers with memory and permissions. */
/**@{*/

/** @brief Get the number of bit blocks needed for the desired bit set capacity.
@param [in] bit_cap the number of bits representing this bit set.
@return the number of blocks needed for the desired bit set.
@warning bit_cap must be >= 1.
@note this macro is not needed if capacity is 0 to start.

This method can be used for compile time initialization of bit sets so the user
need to worry about the underlying bit set representation. For example:

```
static ccc_bitset b
    = ccc_bs_init((static ccc_bitblock[ccc_bs_blocks(256)]){}, NULL, NULL, 256);
```

The above example also illustrates the benefits of a static compound literal
to encapsulate the bits within the bit set array to prevent dangling references.
If your compiler has not implemented storage duration of compound literals the
more traditional example would look like this.

```
static ccc_bitblock blocks[ccc_bs_blocks(256)];
static ccc_bitset b = ccc_bs_init(blocks, NULL, NULL, 256);
```

This macro is required for any initialization where the bit block memory comes
from the stack or data segment as determined by the user. */
#define ccc_bs_blocks(bit_cap) ccc_impl_bs_blocks(bit_cap)

/** @brief Initialize the bit set with memory and allocation permissions.
@param [in] bitblock_ptr the pointer to existing blocks or NULL.
@param [in] alloc_fn the allocation function for a dynamic bit set or NULL.
@param [in] aux auxiliary data needed for allocation of the bit set.
@param [in] cap the number of bits that will be stored in this bit set.
@param [in] optional_size an optional starting size <= capacity. If the bitset
is of fixed size with no allocation permission, and dynamic push and pop are not
needed, the optional size parameter should be set equivalent to capacity.
@return the initialized bit set on the right hand side of an equality operator
@warning the user must use the ccc_bs_blocks macro to help determine the size of
the bitblock array if a fixed size bitblock array is provided at compile time;
the necessary conversion from bits requested to number of blocks required to
store those bits must occur before use. If capacity is zero the helper macro
is not needed.

```
#define BITSET_USING_NAMESPACE_CCC
bitset bs = bs_init((bitblock[bs_blocks(9)]){}, 9, NULL, NULL, 9);
```
Or, initialize with zero capacity for a dynamic bit set.

```
#define BITSET_USING_NAMESPACE_CCC
bitset bs = bs_init(NULL, std_alloc, NULL, 0);
```

See types.h for more on allocation functions. */
#define ccc_bs_init(bitblock_ptr, alloc_fn, aux, cap, optional_size...)        \
    ccc_impl_bs_init(bitblock_ptr, alloc_fn, aux, cap, optional_size)

/** @brief Copy the bit set at source to destination.
@param [in] dst the initialized destination for the copy of the src set.
@param [in] src the initialized source of the set.
@param [in] fn the optional allocation function if resizing is needed.
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
#define BITSET_USING_NAMESPACE_CCC
static bitset src
    = bs_init((static bitblock[bs_blocks(11)]){}, NULL, NULL, 11);
set_rand_bits(&src);
static bitset src
    = bs_init((static bitblock[bs_blocks(13)]){}, NULL, NULL, 13);
ccc_result res = bs_copy(&dst, &src, NULL);
```

The above requires dst capacity be greater than or equal to src capacity. Here
is memory management handed over to the copy function.

```
#define BITSET_USING_NAMESPACE_CCC
static bitset src = bs_init((bitblock *)NULL, std_alloc, NULL, 0);
push_rand_bits(&src);
static bitset src = bs_init((bitblock *)NULL, std_alloc, NULL, 0);
ccc_result res = bs_copy(&dst, &src, std_alloc);
```

The above allows dst to have a capacity less than that of the src as long as
copy has been provided an allocation function to resize dst. Note that this
would still work if copying to a destination that the user wants as a fixed
size map.

```
#define BITSET_USING_NAMESPACE_CCC
static bitset src = bs_init((bitblock *)NULL, std_alloc, NULL, 0);
push_rand_bits(&src);
static bitset src = bs_init((bitblock *)NULL, NULL, NULL, 0);
ccc_result res = bs_copy(&dst, &src, std_alloc);
```

The above sets up dst with fixed size while src is a dynamic map. Because an
allocation function is provided, the dst is resized once for the copy and
retains its fixed size after the copy is complete. This would require the user
to manually free the underlying buffer at dst eventually if this method is used.
Usually it is better to allocate the memory explicitly before the copy if
copying between maps without allocation permission.

These options allow users to stay consistent across containers with their
memory management strategies. */
ccc_result ccc_bs_copy(ccc_bitset *dst, ccc_bitset const *src,
                       ccc_alloc_fn *fn);

/** @brief Reserves space for at least to_add more bits.
@param [in] bs a pointer to the bit set.
@param [in] to_add the number of elements to add to the current size.
@param [in] fn the allocation function to use to reserve memory.
@return the result of the reservation. OK if successful, otherwise an error
status is returned.
@note see the ccc_bs_clear_and_free_reserve function if this function is
being used for a one-time dynamic reservation.

This function can be used for a dynamic bit set with or without allocation
permission. If the bit set has allocation permission, it will reserve the
required space and later resize if more space is needed.

If the bit set has been initialized with no allocation permission and no memory
this function can serve as a one-time reservation. This is helpful when a fixed
size is needed but that size is only known dynamically at runtime. To free the
bit set in such a case see the ccc_bs_clear_and_free_reserve function. */
ccc_result ccc_bs_reserve(ccc_bitset *bs, size_t to_add, ccc_alloc_fn *fn);

/**@}*/

/** @name Bit Membership Interface
Test for the presence of bits. */
/**@{*/

/** @brief Test the bit at index i for boolean status (CCC_TRUE or CCC_FALSE).
@param [in] bs a pointer to the bit set.
@param [in] i the index identifying the bit to set.
@return the state of the bit, or CCC_TRIBOOL_ERROR if bs is NULL.
@note this function performs bounds checking in the release target. */
ccc_tribool ccc_bs_test(ccc_bitset const *bs, size_t i);

/**@}*/

/** @name Bit Modification Interface
Set and flip bits in the set. */
/**@{*/

/** @brief Set the bit at valid index i to value b (true or false).
@param [in] bs a pointer to the bit set.
@param [in] i the valid index identifying the bit to set.
@param [in] b the value to set at position i (CCC_TRUE or CCC_FALSE).
@return the state of the bit before the set operation, true if it was
previously true, false if it was previously false, or error if bs is NULL or
i is out of range.
@note this function performs bounds checking in the release target. */
ccc_tribool ccc_bs_set(ccc_bitset *bs, size_t i, ccc_tribool b);

/** @brief Set all the bits to the provided value (CCC_TRUE or CCC_FALSE).
@param [in] bs a pointer to the bit set.
@param [in] b the value to set (CCC_TRUE or CCC_FALSE).
@return the result of the operation. OK if successful, or an input error if
bs is NULL. */
ccc_result ccc_bs_set_all(ccc_bitset *bs, ccc_tribool b);

/** @brief Set all the bits in the specified range starting at the Least
Significant Bit of the range and ending at the Most Significant Bit of the
range (CCC_TRUE or CCC_FALSE).
@param [in] bs a pointer to the bit set.
@param [in] b the value to set (CCC_TRUE or CCC_FALSE).
@param [in] i the starting index to set.
@param [in] count the count of bits starting at i to set.
@return the result of the operation. OK if successful, or an input error if
bs is NULL or the range is invalid by position, count, or both.

Note that a range is defined from i to i + count, where i + count is the
exclusive end of the range. This is equivalent to moving from Least to Most
Significant bit in an integer. */
ccc_result ccc_bs_set_range(ccc_bitset *bs, size_t i, size_t count,
                            ccc_tribool b);

/** @brief Set the bit at valid index i to boolean value b (true or false).
@param [in] bs a pointer to the bit set.
@param [in] i the valid index identifying the bit to set.
@return the state of the bit before the set operation, true if it was
previously true, false if it was previously false, or error if bs is NULL or
i is out of range.
@note this function performs bounds checking in the release target. */
ccc_tribool ccc_bs_reset(ccc_bitset *bs, size_t i);

/** @brief Set all the bits to CCC_FALSE.
@param [in] bs a pointer to the bit set.
@return the result of the operation. OK if successful, or an input error if
bs is NULL. */
ccc_result ccc_bs_reset_all(ccc_bitset *bs);

/** @brief Set all the bits in the specified range starting at the Least
Significant Bit of the range and ending at the Most Significant Bit of the
range to CCC_FALSE.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to reset.
@param [in] count the count of bits starting at i to reset.
@return the result of the operation. OK if successful, or an input error if
bs is NULL or the range is invalid by position, count, or both.

Note that a range is defined from i to i + count, where i + count is the
exclusive end of the range. This is equivalent to moving from Least to Most
Significant bit in an integer. */
ccc_result ccc_bs_reset_range(ccc_bitset *bs, size_t i, size_t count);

/** @brief Toggle the bit at index i.
@param [in] bs a pointer to the bit set.
@param [in] i the index identifying the bit to toggle
@return the state of the bit before the toggle operation, true if it was
previously true, false if it was previously false, or error if bs is NULL or
i is out of range.
@note this function performs bounds checking in the release target. */
ccc_tribool ccc_bs_flip(ccc_bitset *bs, size_t i);

/** @brief Toggle all of the bits to their opposing boolean value.
@param [in] bs a pointer to the bit set.
@return the result of the operation. OK if successful, or an input error if
bs is NULL. */
ccc_result ccc_bs_flip_all(ccc_bitset *bs);

/** @brief Flip all the bits in the range, starting at the Least Significant Bit
in range and ending at the Most Significant Bit, to their opposite value.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to reset.
@param [in] count the count of bits starting at i to reset.
@return the result of the operation. OK if successful, or an input error if
bs is NULL or the range is invalid by position, count, or both.

Note that a range is defined from i to i + count, where i + count is the
exclusive end of the range. This is equivalent to moving from Least to Most
Significant Bit in an integer. */
ccc_result ccc_bs_flip_range(ccc_bitset *bs, size_t i, size_t count);

/**@}*/

/** @name Bit Scan Interface
Find bits with a specific status. */
/**@{*/

/** @brief Return true if any bits in set are 1.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if any bits are 1, CCC_FALSE if no bits are 1,
CCC_TRIBOOL_ERROR if bs is NULL. */
ccc_tribool ccc_bs_any(ccc_bitset const *bs);

/** @brief Return true if any bits are 1 in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return CCC_TRUE if any bits are 1, CCC_FALSE if no bits are 1,
CCC_TRIBOOL_ERROR if bs is NULL, i is invalid, count is invalid, or both i and
count are invalid. */
ccc_tribool ccc_bs_any_range(ccc_bitset const *bs, size_t i, size_t count);

/** @brief Return true if all bits are set to 0.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if all bits are 0, CCC_FALSE if any bits are 1,
CCC_TRIBOOL_ERROR if bs is NULL. */
ccc_tribool ccc_bs_none(ccc_bitset const *bs);

/** @brief Return true if all bits are 0 in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return CCC_TRUE if all bits are 0, CCC_FALSE if any bits are 1,
CCC_TRIBOOL_ERROR if bs is NULL, i is invalid, count is invalid, or both i and
count are invalid. */
ccc_tribool ccc_bs_none_range(ccc_bitset const *bs, size_t i, size_t count);

/** @brief Return true if all bits in set are 1.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if all bits are 1, CCC_FALSE if any bits are 0,
CCC_TRIBOOL_ERROR if bs is NULL. */
ccc_tribool ccc_bs_all(ccc_bitset const *bs);

/** @brief Return true if all bits are set to 1 in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return CCC_TRUE if all bits are 1, CCC_FALSE if any bits are 0,
CCC_TRIBOOL_ERROR if bs is NULL, i is invalid, count is invalid, or both i and
count are invalid. */
ccc_tribool ccc_bs_all_range(ccc_bitset const *bs, size_t i, size_t count);

/** @brief Return the index of the first trailing bit set to 1 in the set.
@param [in] bs a pointer to the bit set.
@return an OK(0) status and index of the first trailing bit set to 1 or an error
set to CCC_RESULT_FAIL if no 1 bit is found. Argument error is set if bs is
NULL. */
ccc_ucount ccc_bs_first_trailing_one(ccc_bitset const *bs);

/** @brief Return the index of the first trailing bit set to 1 in the range,
starting from the Least Significant Bit at index 0.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@return an OK(0) status and the index of the first trailing bit set to 1 or
CCC_RESULT_FAIL if no 1 bit is found. Argument error is returned bs is NULL, or
the range is invalid. */
ccc_ucount ccc_bs_first_trailing_one_range(ccc_bitset const *bs, size_t i,
                                           size_t count);

/** @brief Returns the index of the start of the first trailing num_ones
contiguous 1 bits.
@param [in] bs a pointer to the bit set.
@param [in] num_ones the number of trailing contiguous 1 bits to find.
@return an OK(0) status and the index in a search starting from the Least
Significant Bit of the set of the first 1 in a sequence of num_ones 1 bits. If
such a sequence cannot be found CCC_RESULT_FAIL result error is set. If bs is
NULL or num_ones is too large an argument error is set. */
ccc_ucount ccc_bs_first_trailing_ones(ccc_bitset const *bs, size_t num_ones);

/** @brief Returns the index of the start of the first trailing num_ones
contiguous 1 bits in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@param [in] num_ones the number of trailing contiguous 1 bits to find.
@return an OK(0) status and the index in a search starting from the Least
Significant Bit of the range of the first 1 in a sequence of num_ones 1 bits. If
no range is found CCC_RESULT_FAIL error is set. If bs is NULL or arguments are
out of range an argument error is set. */
ccc_ucount ccc_bs_first_trailing_ones_range(ccc_bitset const *bs, size_t i,
                                            size_t count, size_t num_ones);

/** @brief Return the index of the first bit set to 0 in the set.
@param [in] bs a pointer to the bit set.
@return an OK(0) status and the index of the first bit set to 0 or
CCC_RESULT_FAIL if no 0 bit is found. If bs is NULL an argument error is set. */
ccc_ucount ccc_bs_first_trailing_zero(ccc_bitset const *bs);

/** @brief Return the index of the first bit set to 0 in the range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@return an OK(0) status and the index of the first bit set to 0 or
CCC_RESULT_FAIL if no 0 bit is found. If bs is NULL, or the range is invalid, an
argument error is set. */
ccc_ucount ccc_bs_first_trailing_zero_range(ccc_bitset const *bs, size_t i,
                                            size_t count);

/** @brief Returns the index of the start of the first trailing num_zeros
contiguous 0 bits.
@param [in] bs a pointer to the bit set.
@param [in] num_zeros the number of trailing contiguous 0 bits to find.
@return an OK(0) status and the index in a search, starting from the Least
Significant Bit of the set, of the first 0 in a sequence of num_zeros 0 bits. If
such a sequence cannot be found CCC_RESULT_FAIL is returned. If bs is NULL or
num zeros is too large an argument error is set. */
ccc_ucount ccc_bs_first_trailing_zeros(ccc_bitset const *bs, size_t num_zeros);

/** @brief Returns the index of the start of the first trailing num_zeros
contiguous 0 bits in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@param [in] num_zeros the number of trailing contiguous 0 bits to find.
@return the index in a search, starting from the Least Significant Bit of the
range, of the first 0 in a sequence of num_zeros 0 bits. If the input is invalid
or such a sequence cannot be found CCC_RESULT_FAIL is returned. */
ccc_ucount ccc_bs_first_trailing_zeros_range(ccc_bitset const *bs, size_t i,
                                             size_t count, size_t num_zeros);

/** @brief Return the index of the first leading bit set to 1 in the set,
starting from the Most Significant Bit at index size - 1.
@param [in] bs a pointer to the bit set.
@return the index of the first leading bit set to 1 or CCC_RESULT_FAIL if no 1
bit is found or bs in NULL. */
ccc_ucount ccc_bs_first_leading_one(ccc_bitset const *bs);

/** @brief Return the index of the first leading bit set to 1 in the set,
starting from the Most Significant Bit at index size - 1.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check from i towards index 0.
@return the index of the first leading bit set to 1 or CCC_RESULT_FAIL if no 1
bit is found, bs is NULL, or the range is invalid.
@warning the user must validate their own range. A bit does not exist in an
invalid range therefore CCC_RESULT_FAIL is returned. To distinguish a valid
negative result signifying not found and a negative result indicating a range
error the user must check their input. */
ccc_ucount ccc_bs_first_leading_one_range(ccc_bitset const *bs, size_t i,
                                          size_t count);

/** @brief Returns the index of the start of the first leading num_ones
contiguous 1 bits.
@param [in] bs a pointer to the bit set.
@param [in] num_ones the number of leading contiguous 1 bits to find.
@return the index in a search starting from the Least Significant Bit of the
set of the first 1 in a sequence of num_ones 1 bits. If the input is invalid
or such a sequence cannot be found CCC_RESULT_FAIL is returned.
@warning the user must validate that bs is non-NULL and num_ones is less than
the size of the set in order to distinguish CCC_RESULT_FAIL returned as a result
of a failed search or bad input. */
ccc_ucount ccc_bs_first_leading_ones(ccc_bitset const *bs, size_t num_ones);

/** @brief Returns the index of the start of the first leading num_ones
contiguous 1 bits in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@param [in] num_ones the number of leading contiguous 1 bits to find.
@return an OK(0) status and the index in a search starting from the Least
Significant Bit of the range of the first 1 in a sequence of num_ones 1 bits. If
such a sequence cannot be found CCC_RESULT_FAIL is set. If bs is NULL or any
argument is out of range an argument error is set. */
ccc_ucount ccc_bs_first_leading_ones_range(ccc_bitset const *bs, size_t i,
                                           size_t count, size_t num_ones);

/** @brief Return the index of the first leading bit set to 0 in the set,
starting from the Most Significant Bit at index size - 1.
@param [in] bs a pointer to the bit set.
@return an OK(0) status and the index of the first bit set to 0 or
CCC_RESULT_FAIL if no 1 bit is found. If bs in NULL an argument error is set. */
ccc_ucount ccc_bs_first_leading_zero(ccc_bitset const *bs);

/** @brief Return the index of the first leading bit set to 0 in the set,
starting from the Most Significant Bit at index size - 1.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search for a 0 bit.
@param [in] count size to search from Most Significant Bit to Least in range.
@return an OK(0) status the index of the first bit set to 0 or CCC_RESULT_FAIL
if no 0 bit is found. If bs in NULL an argument error is set. */
ccc_ucount ccc_bs_first_leading_zero_range(ccc_bitset const *bs, size_t i,
                                           size_t count);

/** @brief Returns the index of the start of the first leading num_zeros
contiguous 0 bits.
@param [in] bs a pointer to the bit set.
@param [in] num_zeros the number of leading contiguous 0 bits to find.
@return an OK(0) status and the index in a search, starting from the Most
Significant Bit of the set, of the first 0 in a sequence of num_zeros 0 bits. If
such a sequence cannot be found CCC_RESULT_FAIL is set. If bs is NULL or
num_zeros is too large an argument error is set. */
ccc_ucount ccc_bs_first_leading_zeros(ccc_bitset const *bs, size_t num_zeros);

/** @brief Returns the index of the start of the first leading num_zeros
contiguous 0 bits in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@param [in] num_zeros the number of leading contiguous 0 bits to find.
@return an OK(0) status and the index in a search, starting from the Most
Significant Bit of the range, of the first 0 in a sequence of num_zeros 0 bits.
If such a sequence cannot be found CCC_RESULT_FAIL is returned. If bs is NULL or
the arguments are out of range an argument error is set. */
ccc_ucount ccc_bs_first_leading_zeros_range(ccc_bitset const *bs, size_t i,
                                            size_t count, size_t num_zeros);

/**@}*/

/** @name Bit Operations Interface
Use standard integer width bit operations on the entire set. */
/**@{*/

/** @brief Bitwise OR dst set with src set.
@param [in] dst the set to modified with the OR operation.
@param [in] src the set to be read as the source bits for the OR operation.
@return an OK result if the operation is successful or an input error if dst
or src are NULL.

Note that sets are aligned from their Least Significant Bit and a smaller src
set is conceptually padded with 0's in its higher order bits to align with
the larger dst set (no modifications to the smaller set are performed to achieve
this). This is done to stay consistent with how the operation would work on
a smaller integer being stored in a larger integer to align with the larger. */
ccc_result ccc_bs_or(ccc_bitset *dst, ccc_bitset const *src);

/** @brief Bitwise AND dst set with src set.
@param [in] dst the set to modified with the AND operation.
@param [in] src the set to be read as the source bits for the AND operation.
@return an OK result if the operation is successful or an input error if dst
or src are NULL.
@warning sets behave identically to integers for the AND operation when widening
occurs. If dst is larger than src, src is padded with zeros in its Most
Significant Bits. This means a bitwise AND operations will occur with the higher
order bits in dst and 0's in this padded range (this results in all bits in dst
being set to 0 in that range).

Note that sets are aligned from their Least Significant Bit and a smaller src
set is conceptually padded with 0's in its higher order bits to align with
the larger dst set (no modifications to the smaller set are performed to achieve
this). This is done to stay consistent with integer promotion and widening rules
in C. */
ccc_result ccc_bs_and(ccc_bitset *dst, ccc_bitset const *src);

/** @brief Bitwise XOR dst set with src set.
@param [in] dst the set to modified with the XOR operation.
@param [in] src the set to be read as the source bits for the XOR operation.
@return an OK result if the operation is successful or an input error if dst
or src are NULL.

Note that sets are aligned from their Least Significant Bit and a smaller src
set is conceptually padded with 0's in its higher order bits to align with
the larger dst set (no modifications to the smaller set are performed to achieve
this). This is done to stay consistent with how the operation would work on
a smaller integer being stored in a larger integer to align with the larger. */
ccc_result ccc_bs_xor(ccc_bitset *dst, ccc_bitset const *src);

/** @brief Shift the bit set left by left_shifts amount.
@param [in] bs a pointer to the bit set.
@param [in] left_shifts the number of left shifts to perform.
@return an ok result if the operation was successful or an error if the bitset
is NULL.

Note that if the number of shifts is greater than the bit set size the bit set
is zeroed out rather than exhibiting undefined behavior as in the equivalent
integer operation. */
ccc_result ccc_bs_shiftl(ccc_bitset *bs, size_t left_shifts);

/** @brief Shift the bit set right by right_shifts amount.
@param [in] bs a pointer to the bit set.
@param [in] right_shifts the number of right shifts to perform.
@return an ok result if the operation was successful or an error if the bitset
is NULL.

Note that if the number of shifts is greater than the bit set size the bit set
is zeroed out rather than exhibiting undefined behavior as in the equivalent
integer operation. */
ccc_result ccc_bs_shiftr(ccc_bitset *bs, size_t right_shifts);

/** @brief Checks two bit sets of the same size (not capacity) for equality.
@param [in] a pointer to a bit set.
@param [in] b pointer to another bit set of equal size.
@return true if the bit sets are of equal size with identical bit values at
every position, false if the sets are different sizes or have mismatched bits.
A bool error is returned if either pointer is NULL. */
ccc_tribool ccc_bs_eq(ccc_bitset const *a, ccc_bitset const *b);

/**@}*/

/** @name Set Operations Interface
Perform basic mathematical set operations on the bit set. */
/**@{*/

/** @brief Return CCC_TRUE if subset is a proper subset of set (subset ⊂ set).
@param [set] the set to check subset against.
@param [subset] the subset to confirm as a proper subset of set.
@return CCC_TRUE if all bit positions in subset share the same
value--0 or 1--as the bit positions in set and set is of greater size than
subset. A CCC_TRIBOOL_ERROR is returned if set or subset is NULL.

If set is of size 0 the function returns CCC_FALSE regardless of the size of
subset. If set is not of size 0 and subset is of size 0 the function returns
CCC_TRUE. */
ccc_tribool ccc_bs_is_proper_subset(ccc_bitset const *set,
                                    ccc_bitset const *subset);

/** @brief Return CCC_TRUE if subset is a subset of set (subset ⊆ set).
@param [set] the set to check subset against.
@param [subset] the subset to confirm as a subset of set.
@return CCC_TRUE if all bit positions in subset share the same
value--0 or 1--as the bit positions in set. A CCC_TRIBOOL_ERROR is returned if
set or subset is NULL.

If set is size zero subset must also be of size 0 to return CCC_TRUE. If
subset is size 0 the function returns CCC_TRUE regardless of the size of set. */
ccc_tribool ccc_bs_is_subset(ccc_bitset const *set, ccc_bitset const *subset);

/**@}*/

/** @name State Interface
Obtain state from the container. */
/**@{*/

/** @brief Return a reference to the base of the underlying block array.
@param [in] bs a pointer to the bit set.
@return a reference to the base of the first block of the bit set block array
or NULL if bs is NULL or has no capacity.

Every block populates bits from Least Significant Bit (LSB) to Most Significant
Bit (MSB) so this reference is to the base or LSB of the entire set. */
ccc_bitblock *ccc_bs_data(ccc_bitset const *bs);

/** @brief Return total number of bits the capacity of the set can hold.
@param [in] bs a pointer to the bit set.
@return an OK(0) status and the capacity of bits capable of being stored in the
current set. If bs is NULL an argument error is set. */
ccc_ucount ccc_bs_capacity(ccc_bitset const *bs);

/** @brief Return total number of bits actively tracked by the user and set.
@param [in] bs a pointer to the bit set.
@return an OK(0) status and the total number of bits currently tracked by the
set regardless of true or false state of each. If bs is NULL an argument error
is set. */
ccc_ucount ccc_bs_size(ccc_bitset const *bs);

/** @brief Return number of ccc_bitblocks used by bit set for capacity bits.
@param [in] bs a pointer to the bit set.
@return an OK(0) status and the capacity in number of bit blocks used to hold
the entire capacity of bits in the set. If bs is NULL an argument error is set.

Capacity may be greater than or equal to size. */
ccc_ucount ccc_bs_blocks_capacity(ccc_bitset const *bs);

/** @brief Return number of ccc_bitblocks used by the bit set for size bits.
@param [in] bs a pointer to the bit set.
@return size in number of bit blocks used to hold the current size of bits in
the set. An argument error is set if bs is NULL.

Size may be less than or equal to capacity. */
ccc_ucount ccc_bs_blocks_size(ccc_bitset const *bs);

/** @brief Return true if no bits are actively tracked by the user and set.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if the size of the set is 0 meaning no bits, regardless of
0 or 1 status, are tracked by the set. CCC_TRIBOOL_ERROR is returned if bs is
NULL.
@warning if the number of bits set to 1 is desired see ccc_bs_popcount. */
ccc_tribool ccc_bs_empty(ccc_bitset const *bs);

/** @brief Return the number of bits set to CCC_TRUE. O(n).
@param [in] bs a pointer to the bit set.
@return the total number of bits currently set to CCC_TRUE. CCC_RESULT_FAIL is
returned if bs is NULL. */
ccc_ucount ccc_bs_popcount(ccc_bitset const *bs);

/** @brief Return the number of bits set to CCC_TRUE in the range. O(n).
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return the total number of bits currently set in the range to CCC_TRUE. An
argument error is set if bs is NULL, i is invalid, count is invalid, or both i
and count are invalid. */
ccc_ucount ccc_bs_popcount_range(ccc_bitset const *bs, size_t i, size_t count);

/**@}*/

/** @name Destructor Interface
Clear the set and manage its memory. */
/**@{*/

/** @brief Clears the bit set by setting the size to 0 and all bits to 0.
The underlying memory capacity remains unchanged.
@param [in] bs a pointer to the bit set.
@return the result of the clear operation, error is returned if bs is NULL . */
ccc_result ccc_bs_clear(ccc_bitset *bs);

/** @brief Clears the bit set by setting the size to 0 and freeing the
underlying memory. Capacity becomes 0 as well.
@param [in] bs a pointer to the bit set.
@return the result of the clear operation, error is returned if bs is NULL or
the set cannot be freed because no allocation function was provided upon
initialization. */
ccc_result ccc_bs_clear_and_free(ccc_bitset *bs);

/** @brief Frees the buffer for the bit set that was previously dynamically
reserved with the reserve function.
@param [in] bs the bs to be cleared.
@param [in] fn the required allocation function to provide to a dynamically
reserved bs. Any auxiliary data provided upon initialization will be passed to
the allocation function when called.
@return the result of free operation. OK if success, or an error status to
indicate the error.
@warning It is an error to call this function on a bs that was not reserved
with the provided ccc_alloc_fn. The bs must have existing memory to free.

This function covers the edge case of reserving a dynamic capacity for a bs
at runtime but denying the bs allocation permission to resize. This can help
prevent a bs from growing unbounded. The user in this case knows the bs does
not have allocation permission and therefore no further memory will be dedicated
to the bs.

However, to free the bs in such a case this function must be used because the
bs has no ability to free itself. Just as the allocation function is required
to reserve memory so to is it required to free memory.

This function will work normally if called on a bs with allocation permission
however the normal ccc_bs_clear_and_free is sufficient for that use case. */
ccc_result ccc_bs_clear_and_free_reserve(ccc_bitset *bs, ccc_alloc_fn *fn);

/**@}*/

/** @name Dynamic Interface
Allows adding to and removing from the set. */
/**@{*/

/** @brief Append a bit value to the set as the new Most Significant Bit.
@param [in] bs a pointer to the bit set.
@param [in] b the value to push at the Most Significant Bit CCC_TRUE/CCC_FALSE.
@return the result of the operation, ok if successful or an error if bad
parameters are provided or resizing is required to accommodate a new bit but
resizing fails. */
ccc_result ccc_bs_push_back(ccc_bitset *bs, ccc_tribool b);

/** @brief Remove the Most Significant Bit from the set.
@param [in] bs a pointer to the bit set.
@return the previous value of the Most Significant Bit (CCC_TRUE or CCC_FALSE)
or a bool error if bs is NULL or bs is empty. */
ccc_tribool ccc_bs_pop_back(ccc_bitset *bs);

/**@}*/

/** Define this preprocessor macro if shorter names are desired for the bit set
container. Check for namespace clashes before name shortening. */
#ifdef BITSET_USING_NAMESPACE_CCC
typedef ccc_bitset bitset;
typedef ccc_bitblock bitblock;
#    define bs_blocks(args...) ccc_bs_blocks(args)
#    define bs_init(args...) ccc_bs_init(args)
#    define bs_copy(args...) ccc_bs_copy(args)
#    define bs_reserve(args...) ccc_bs_reserve(args)
#    define bs_test(args...) ccc_bs_test(args)
#    define bs_set(args...) ccc_bs_set(args)
#    define bs_set_all(args...) ccc_bs_set_all(args)
#    define bs_set_range(args...) ccc_bs_set_range(args)
#    define bs_reset(args...) ccc_bs_reset(args)
#    define bs_reset_all(args...) ccc_bs_reset_all(args)
#    define bs_reset_range(args...) ccc_bs_reset_range(args)
#    define bs_flip(args...) ccc_bs_flip(args)
#    define bs_flip_all(args...) ccc_bs_flip_all(args)
#    define bs_flip_range(args...) ccc_bs_flip_range(args)
#    define bs_any(args...) ccc_bs_any(args)
#    define bs_any_range(args...) ccc_bs_any_range(args)
#    define bs_none(args...) ccc_bs_none(args)
#    define bs_none_range(args...) ccc_bs_none_range(args)
#    define bs_all(args...) ccc_bs_all(args)
#    define bs_all_range(args...) ccc_bs_all_range(args)
#    define bs_first_trailing_one(args...) ccc_bs_first_trailing_one(args)
#    define bs_first_trailing_one_range(args...)                               \
        ccc_bs_first_trailing_one_range(args)
#    define bs_first_trailing_ones(args...) ccc_bs_first_trailing_ones(args)
#    define bs_first_trailing_ones_range(args...)                              \
        ccc_bs_first_trailing_ones_range(args)
#    define bs_first_trailing_zero(args...) ccc_bs_first_trailing_zero(args)
#    define bs_first_trailing_zero_range(args...)                              \
        ccc_bs_first_trailing_zero_range(args)
#    define bs_first_trailing_zeros(args...) ccc_bs_first_trailing_zeros(args)
#    define bs_first_trailing_zeros_range(args...)                             \
        ccc_bs_first_trailing_zeros_range(args)
#    define bs_first_leading_one(args...) ccc_bs_first_leading_one(args)
#    define bs_first_leading_one_range(args...)                                \
        ccc_bs_first_leading_one_range(args)
#    define bs_first_leading_ones(args...) ccc_bs_first_leading_ones(args)
#    define bs_first_leading_ones_range(args...)                               \
        ccc_bs_first_leading_ones_range(args)
#    define bs_first_leading_zero(args...) ccc_bs_first_leading_zero(args)
#    define bs_first_leading_zero_range(args...)                               \
        ccc_bs_first_leading_zero_range(args)
#    define bs_first_leading_zeros(args...) ccc_bs_first_leading_zeros(args)
#    define bs_first_leading_zeros_range(args...)                              \
        ccc_bs_first_leading_zeros_range(args)
#    define bs_or(args...) ccc_bs_or(args)
#    define bs_and(args...) ccc_bs_and(args)
#    define bs_xor(args...) ccc_bs_xor(args)
#    define bs_shiftl(args...) ccc_bs_shiftl(args)
#    define bs_shiftr(args...) ccc_bs_shiftr(args)
#    define bs_eq(args...) ccc_bs_eq(args)
#    define bs_is_proper_subset(args...) ccc_bs_is_proper_subset(args)
#    define bs_is_subset(args...) ccc_bs_is_subset(args)
#    define bs_data(args...) ccc_bs_data(args)
#    define bs_capacity(args...) ccc_bs_capacity(args)
#    define bs_blocks_capacity(args...) ccc_bs_blocks_capacity(args)
#    define bs_size(args...) ccc_bs_size(args)
#    define bs_blocks_size(args...) ccc_bs_blocks_size(args)
#    define bs_empty(args...) ccc_bs_empty(args)
#    define bs_popcount(args...) ccc_bs_popcount(args)
#    define bs_popcount_range(args...) ccc_bs_popcount_range(args)
#    define bs_clear(args...) ccc_bs_clear(args)
#    define bs_clear_and_free(args...) ccc_bs_clear_and_free(args)
#    define bs_clear_and_free_reserve(args...)                                 \
        ccc_bs_clear_and_free_reserve(args)
#    define bs_push_back(args...) ccc_bs_push_back(args)
#    define bs_pop_back(args...) ccc_bs_pop_back(args)
#endif /* BITSET_USING_NAMESPACE_CCC */

#endif /* CCC_BITSET */
