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

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define BITSET_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_BITSET
#define CCC_BITSET

#include <stddef.h>
#include <stdint.h>

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

This method can be used for compile time initialization of bit sets so the user
need to worry about the underlying bit set representation. For example:

```
static ccc_bitset b
    = ccc_bs_init((static ccc_bitblock[ccc_bs_blocks(256)]){}, 256, NULL, NULL);
```

The above example also illustrates the benefits of a static compound literal
to encapsulate the bits within the bit set array to prevent dangling references.
If your compiler has not implemented storage duration of compound literals the
more traditional example would look like this.

```
static ccc_bitblock blocks[ccc_bs_blocks(256)];
static ccc_bitset b = ccc_bs_init(blocks, 256, NULL, NULL);
```

This macro is required for any initialization where the bit block memory comes
from the stack or data segment as determined by the user. */
#define ccc_bs_blocks(bit_cap) ccc_impl_bs_blocks(bit_cap)

/** @brief Initialize the bit set with memory and allocation permissions.
@param [in] bitblock_ptr the pointer to existing blocks or NULL.
@param [in] cap the number of bits that will be stored in this bit set.
@param [in] alloc_fn the allocation function for a dynamic bit set or NULL.
@param [in] aux auxiliary data needed for allocation of the bit set.
@param [in] optional_size an optional starting size <= capacity. If the bitset
is of fixed size with no allocation permission, and dynamic push and pop are not
needed, the optional size parameter should be set equivalent to capacity.
@return the initialized bit set on the right hand side of an equality operator
@warning the user must use the ccc_bs_blocks macro to help determine the size of
the bitblock array if a fixed size bitblock array is provided at compile time;
the necessary conversion from bits requested to number of blocks required to
store those bits must occur before use.
(e.g. ccc_bitset b = ccc_bs_init((ccc_bitblock[ccc_bs_blocks(9)]){},...);). */
#define ccc_bs_init(bitblock_ptr, cap, alloc_fn, aux, optional_size...)        \
    ccc_impl_bs_init(bitblock_ptr, cap, alloc_fn, aux, optional_size)

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
    = bs_init((static bitblock[bs_blocks(11)]){}, 11, NULL, NULL);
set_rand_bits(&src);
static bitset src
    = bs_init((static bitblock[bs_blocks(13)]){}, 13, NULL, NULL);
ccc_result res = bs_copy(&dst, &src, NULL);
```

The above requires dst capacity be greater than or equal to src capacity. Here
is memory management handed over to the copy function.

```
#define BITSET_USING_NAMESPACE_CCC
static bitset src = bs_init((bitblock *)NULL, 0, std_alloc, NULL);
push_rand_bits(&src);
static bitset src = bs_init((bitblock *)NULL, 0, std_alloc, NULL);
ccc_result res = bs_copy(&dst, &src, std_alloc);
```

The above allows dst to have a capacity less than that of the src as long as
copy has been provided an allocation function to resize dst. Note that this
would still work if copying to a destination that the user wants as a fixed
size map.

```
#define BITSET_USING_NAMESPACE_CCC
static bitset src = bs_init((bitblock *)NULL, 0, std_alloc, NULL);
push_rand_bits(&src);
static bitset src = bs_init((bitblock *)NULL, 0, NULL, NULL);
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

/**@}*/

/** @name Bit Membership Interface
Test for the presence of bits. */
/**@{*/

/** @brief Test the bit at index i for boolean status (CCC_TRUE or CCC_FALSE).
@param [in] bs a pointer to the bit set.
@param [in] i the index identifying the bit to set.
@return the state of the bit, or CCC_BOOL_ERR if bs is NULL.
@warning no bounds checking occurs in the release target. For bounds checking,
see ccc_bs_test_at(). */
ccc_tribool ccc_bs_test(ccc_bitset const *bs, size_t i);

/** @brief Test the bit at index i for boolean status (CCC_TRUE or CCC_FALSE).
@param [in] bs a pointer to the bit set.
@param [in] i the index identifying the bit to set.
@return the state of the bit, or CCC_BOOL_ERR if bs is NULL.
@note this function performs bounds checking in the release target. */
ccc_tribool ccc_bs_test_at(ccc_bitset const *bs, size_t i);

/**@}*/

/** @name Dynamic Interface
Allows adding to and removing from the set. */
/**@{*/

/** @brief Add a bit value to the set as the new Most Significant Bit.
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

/** @name Bit Modification Interface
Set and flip bits in the set. */
/**@{*/

/** @brief Set the bit at index i to value b (CCC_TRUE or CCC_FALSE).
@param [in] bs a pointer to the bit set.
@param [in] i the index identifying the bit to set.
@param [in] b the value to set at position i (CCC_TRUE or CCC_FALSE).
@return the state of the bit before the set operation, true if it was
previously true, false if it was previously false, or error if bs is NULL.
@warning no bounds checking occurs in the release target. For bounds checking,
see ccc_bs_set_at(). */
ccc_tribool ccc_bs_set(ccc_bitset *bs, size_t i, ccc_tribool b);

/** @brief Set the bit at valid index i to value b (true or false).
@param [in] bs a pointer to the bit set.
@param [in] i the valid index identifying the bit to set.
@param [in] b the value to set at position i (CCC_TRUE or CCC_FALSE).
@return the state of the bit before the set operation, true if it was
previously true, false if it was previously false, or error if bs is NULL or
i is out of range.
@note this function performs bounds checking in the release target. */
ccc_tribool ccc_bs_set_at(ccc_bitset *bs, size_t i, ccc_tribool b);

/** @brief Set all the bits to the provided value (CCC_TRUE or CCC_FALSE).
@param [in] bs a pointer to the bit set.
@param [in] b the value to set (CCC_TRUE or CCC_FALSE).
@return the result of the operation. OK if successful, or an input error if
bs is NULL. */
ccc_result ccc_bs_set_all(ccc_bitset *bs, ccc_tribool b);

/** @brief Set all the bits in the specified range (CCC_TRUE or CCC_FALSE).
@param [in] bs a pointer to the bit set.
@param [in] b the value to set (CCC_TRUE or CCC_FALSE).
@param [in] i the starting index to set.
@param [in] count the count of bits starting at i to set.
@return the result of the operation. OK if successful, or an input error if
bs is NULL or the range is invalid by position, count, or both. */
ccc_result ccc_bs_set_range(ccc_bitset *bs, size_t i, size_t count,
                            ccc_tribool b);

/** @brief Set the bit at index i to CCC_FALSE.
@param [in] bs a pointer to the bit set.
@param [in] i the index identifying the bit to reset.
@return the state of the bit before the reset operation, true if it was
previously true, false if it was previously false, or error if bss is NULL.
@warning no bounds checking occurs in the release target. For bounds checking,
see ccc_bs_set_at(). */
ccc_tribool ccc_bs_reset(ccc_bitset *bs, size_t i);

/** @brief Set the bit at valid index i to boolean value b (true or false).
@param [in] bs a pointer to the bit set.
@param [in] i the valid index identifying the bit to set.
@return the state of the bit before the set operation, true if it was
previously true, false if it was previously false, or error if bs is NULL or
i is out of range.
@note this function performs bounds checking in the release target. */
ccc_tribool ccc_bs_reset_at(ccc_bitset *bs, size_t i);

/** @brief Set all the bits to CCC_FALSE.
@param [in] bs a pointer to the bit set.
@return the result of the operation. OK if successful, or an input error if
bs is NULL. */
ccc_result ccc_bs_reset_all(ccc_bitset *bs);

/** @brief Set all the bits in the range to CCC_FALSE.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to reset.
@param [in] count the count of bits starting at i to reset.
@return the result of the operation. OK if successful, or an input error if
bs is NULL or the range is invalid by position, count, or both. */
ccc_result ccc_bs_reset_range(ccc_bitset *bs, size_t i, size_t count);

/** @brief Toggle the bit at index i.
@param [in] bs a pointer to the bit set.
@param [in] i the index identifying the bit to toggle
@return the state of the bit before the toggle operation, true if it was
previously true, false if it was previously false, or error if bss is NULL.
@warning no bounds checking occurs in the release target. For bounds checking,
see ccc_bs_set_at(). */
ccc_tribool ccc_bs_flip(ccc_bitset *bs, size_t i);

/** @brief Toggle the bit at index i.
@param [in] bs a pointer to the bit set.
@param [in] i the index identifying the bit to toggle
@return the state of the bit before the toggle operation, true if it was
previously true, false if it was previously false, or error if bs is NULL or
i is out of range.
@note this function performs bounds checking in the release target. */
ccc_tribool ccc_bs_flip_at(ccc_bitset *bs, size_t i);

/** @brief Toggle all of the bits to their opposing boolean value.
@param [in] bs a pointer to the bit set.
@return the result of the operation. OK if successful, or an input error if
bs is NULL. */
ccc_result ccc_bs_flip_all(ccc_bitset *bs);

/** @brief Flip all the bits in the range to their opposite value.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to reset.
@param [in] count the count of bits starting at i to reset.
@return the result of the operation. OK if successful, or an input error if
bs is NULL or the range is invalid by position, count, or both. */
ccc_result ccc_bs_flip_range(ccc_bitset *bs, size_t i, size_t count);

/**@}*/

/** @name Scan Interface
Find bits with a specific status. */
/**@{*/

/** @brief Return true if any bits in set are 1.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if any bits are 1, CCC_FALSE if no bits are 1, CCC_BOOL_ERR
if bs is NULL. */
ccc_tribool ccc_bs_any(ccc_bitset const *bs);

/** @brief Return true if any bits are 1 in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return CCC_TRUE if any bits are 1, CCC_FALSE if no bits are 1, CCC_BOOL_ERR
if bs is NULL, i is invalid, count is invalid, or both i and count are
invalid. */
ccc_tribool ccc_bs_any_range(ccc_bitset const *bs, size_t i, size_t count);

/** @brief Return true if all bits are set to 0.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if all bits are 0, CCC_FALSE if any bits are 1, CCC_BOOL_ERR
if bs is NULL. */
ccc_tribool ccc_bs_none(ccc_bitset const *bs);

/** @brief Return true if all bits are 0 in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return CCC_TRUE if all bits are 0, CCC_FALSE if any bits are 1, CCC_BOOL_ERR
if bs is NULL, i is invalid, count is invalid, or both i and count are
invalid. */
ccc_tribool ccc_bs_none_range(ccc_bitset const *bs, size_t i, size_t count);

/** @brief Return true if all bits in set are 1.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if all bits are 1, CCC_FALSE if any bits are 0, CCC_BOOL_ERR
if bs is NULL. */
ccc_tribool ccc_bs_all(ccc_bitset const *bs);

/** @brief Return true if all bits are set to 1 in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return CCC_TRUE if all bits are 1, CCC_FALSE if any bits are 0, CCC_BOOL_ERR
if bs is NULL, i is invalid, count is invalid, or both i and count are
invalid. */
ccc_tribool ccc_bs_all_range(ccc_bitset const *bs, size_t i, size_t count);

/** @brief Return the index of the first trailing bit set to 1 in the set.
@param [in] bs a pointer to the bit set.
@return the index of the first trailing bit set to 1 or -1 if no 1 bit is found
or bs in NULL. */
ptrdiff_t ccc_bs_first_trailing_one(ccc_bitset const *bs);

/** @brief Return the index of the first trailing bit set to 1 in the range,
starting from the Least Significant Bit at index 0.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@return the index of the first trailing bit set to 1 or -1 if no 1 bit is found,
bs is NULL, or the range is invalid.
@warning the user must validate their own range. A bit does not exist in an
invalid range therefore -1 is returned. To distinguish a valid negative result
signifying not found and a negative result indicating a range error the user
must check their input. */
ptrdiff_t ccc_bs_first_trailing_one_range(ccc_bitset const *bs, size_t i,
                                          size_t count);

/** @brief Returns the index of the start of the first trailing num_ones
contiguous 1 bits.
@param [in] bs a pointer to the bit set.
@param [in] num_ones the number of trailing contiguous 1 bits to find.
@return the index in a search starting from the Least Significant Bit of the
set of the first 1 in a sequence of num_ones 1 bits. If the input is invalid
or such a sequence cannot be found -1 is returned.
@warning the user must validate that bs is non-NULL and num_ones is less than
the size of the set in order to distinguish -1 returned as a result of a failed
search or bad input. */
ptrdiff_t ccc_bs_first_trailing_ones(ccc_bitset const *bs, size_t num_ones);

/** @brief Returns the index of the start of the first trailing num_ones
contiguous 1 bits in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@param [in] num_ones the number of trailing contiguous 1 bits to find.
@return the index in a search starting from the Least Significant Bit of the
range of the first 1 in a sequence of num_ones 1 bits. If the input is invalid
or such a sequence cannot be found -1 is returned.
@warning the user must validate their own range. A group of 1's does not exist
in an invalid range therefore -1 is returned. To distinguish a valid negative
result signifying not found and a negative result indicating a range error the
user must check their input. */
ptrdiff_t ccc_bs_first_trailing_ones_range(ccc_bitset const *bs, size_t i,
                                           size_t count, size_t num_ones);

/** @brief Return the index of the first bit set to 0 in the set.
@param [in] bs a pointer to the bit set.
@return the index of the first bit set to 0 or -1 if no 0 bit is found or bs in
NULL. */
ptrdiff_t ccc_bs_first_trailing_zero(ccc_bitset const *bs);

/** @brief Return the index of the first bit set to 0 in the range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@return the index of the first bit set to 0 or -1 if no 0 bit is found, bs is
NULL, or the range is invalid.
@warning the user must validate their own range. A bit does not exist in an
invalid range therefore -1 is returned. To distinguish a valid negative result
signifying not found and a negative result indicating a range error the user
must check their input. */
ptrdiff_t ccc_bs_first_trailing_zero_range(ccc_bitset const *bs, size_t i,
                                           size_t count);

/** @brief Returns the index of the start of the first trailing num_zeros
contiguous 0 bits.
@param [in] bs a pointer to the bit set.
@param [in] num_zeros the number of trailing contiguous 0 bits to find.
@return the index in a search starting from the Least Significant Bit of the
set of the first 1 in a sequence of num_zeros 0 bits. If the input is invalid
or such a sequence cannot be found -1 is returned.
@warning the user must validate that bs is non-NULL and num_zeros is less than
the size of the set in order to distinguish -1 returned as a result of a failed
search or bad input. */
ptrdiff_t ccc_bs_first_trailing_zeros(ccc_bitset const *bs, size_t num_zeros);

/** @brief Returns the index of the start of the first trailing num_zeros
contiguous 0 bits in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@param [in] num_zeros the number of trailing contiguous 0 bits to find.
@return the index in a search starting from the Least Significant Bit of the
range of the first 1 in a sequence of num_zeros 0 bits. If the input is invalid
or such a sequence cannot be found -1 is returned.
@warning the user must validate their own range. A group of 1's does not exist
in an invalid range therefore -1 is returned. To distinguish a valid negative
result signifying not found and a negative result indicating a range error the
user must check their input. */
ptrdiff_t ccc_bs_first_trailing_zeros_range(ccc_bitset const *bs, size_t i,
                                            size_t count, size_t num_zeros);

/** @brief Return the index of the first leading bit set to 1 in the set,
starting from the Most Significant Bit at index size - 1.
@param [in] bs a pointer to the bit set.
@return the index of the first leading bit set to 1 or -1 if no 1 bit is found
or bs in NULL. */
ptrdiff_t ccc_bs_first_leading_one(ccc_bitset const *bs);

/** @brief Return the index of the first leading bit set to 1 in the set,
starting from the Most Significant Bit at index size - 1.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check from i towards index 0.
@return the index of the first leading bit set to 1 or -1 if no 1 bit is found,
bs is NULL, or the range is invalid.
@warning the user must validate their own range. A bit does not exist in an
invalid range therefore -1 is returned. To distinguish a valid negative result
signifying not found and a negative result indicating a range error the user
must check their input. */
ptrdiff_t ccc_bs_first_leading_one_range(ccc_bitset const *bs, size_t i,
                                         size_t count);

/** @brief Returns the index of the start of the first leading num_ones
contiguous 1 bits.
@param [in] bs a pointer to the bit set.
@param [in] num_ones the number of leading contiguous 1 bits to find.
@return the index in a search starting from the Least Significant Bit of the
set of the first 1 in a sequence of num_ones 1 bits. If the input is invalid
or such a sequence cannot be found -1 is returned.
@warning the user must validate that bs is non-NULL and num_ones is less than
the size of the set in order to distinguish -1 returned as a result of a failed
search or bad input. */
ptrdiff_t ccc_bs_first_leading_ones(ccc_bitset const *bs, size_t num_ones);

/** @brief Returns the index of the start of the first leading num_ones
contiguous 1 bits in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@param [in] num_ones the number of leading contiguous 1 bits to find.
@return the index in a search starting from the Least Significant Bit of the
range of the first 1 in a sequence of num_ones 1 bits. If the input is invalid
or such a sequence cannot be found -1 is returned.
@warning the user must validate their own range. A group of 1's does not exist
in an invalid range therefore -1 is returned. To distinguish a valid negative
result signifying not found and a negative result indicating a range error the
user must check their input. */
ptrdiff_t ccc_bs_first_leading_ones_range(ccc_bitset const *bs, size_t i,
                                          size_t count, size_t num_ones);

/** @brief Return the index of the first leading bit set to 0 in the set,
starting from the Most Significant Bit at index size - 1.
@param [in] bs a pointer to the bit set.
@return the index of the first bit set to 0 or -1 if no 1 bit is found or bs in
NULL. */
ptrdiff_t ccc_bs_first_leading_zero(ccc_bitset const *bs);

/** @brief Return the index of the first leading bit set to 0 in the set,
starting from the Most Significant Bit at index size - 1.
@param [in] bs a pointer to the bit set.
@return the index of the first bit set to 1 or -1 if no 1 bit is found or bs in
NULL. */
ptrdiff_t ccc_bs_first_leading_zero_range(ccc_bitset const *bs, size_t i,
                                          size_t count);

/** @brief Returns the index of the start of the first leading num_zeros
contiguous 0 bits.
@param [in] bs a pointer to the bit set.
@param [in] num_zeros the number of leading contiguous 0 bits to find.
@return the index in a search starting from the Least Significant Bit of the
set of the first 1 in a sequence of num_zeros 0 bits. If the input is invalid
or such a sequence cannot be found -1 is returned.
@warning the user must validate that bs is non-NULL and num_zeros is less than
the size of the set in order to distinguish -1 returned as a result of a failed
search or bad input. */
ptrdiff_t ccc_bs_first_leading_zeros(ccc_bitset const *bs, size_t num_zeros);

/** @brief Returns the index of the start of the first leading num_zeros
contiguous 0 bits in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@param [in] num_zeros the number of leading contiguous 0 bits to find.
@return the index in a search starting from the Least Significant Bit of the
range of the first 1 in a sequence of num_zeros 0 bits. If the input is invalid
or such a sequence cannot be found -1 is returned.
@warning the user must validate their own range. A group of 1's does not exist
in an invalid range therefore -1 is returned. To distinguish a valid negative
result signifying not found and a negative result indicating a range error the
user must check their input. */
ptrdiff_t ccc_bs_first_leading_zeros_range(ccc_bitset const *bs, size_t i,
                                           size_t count, size_t num_zeros);

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
@return capacity of bits capable of being stored in the current set. */
size_t ccc_bs_capacity(ccc_bitset const *bs);

/** @brief Return number of ccc_bitblocks used by bit set for capacity bits.
@param [in] bs a pointer to the bit set.
@return capacity in number of bit blocks used to hold the entire capacity of
bits in the set. Capacity may be greater than or equal to size. */
size_t ccc_bs_blocks_capacity(ccc_bitset const *bs);

/** @brief Return total number of bits actively tracked by the user and set.
@param [in] bs a pointer to the bit set.
@return the total number of bits currently tracked by the set regardless of
true or false state of each. 0 is returned if bs is NULL. */
size_t ccc_bs_size(ccc_bitset const *bs);

/** @brief Return number of ccc_bitblocks used by the bit set for size bits.
@param [in] bs a pointer to the bit set.
@return size in number of bit blocks used to hold the current size of bits in
the set. Size may be less than or equal to capacity. */
size_t ccc_bs_blocks_size(ccc_bitset const *bs);

/** @brief Return true if no bits are actively tracked by the user and set.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if the size of the set is 0 meaning no bits, regardless of
0 or 1 status, are tracked by the set. CCC_BOOL_ERR is returned if bs is NULL.
@warning if the number of bits set to 1 is desired see ccc_bs_popcount. */
ccc_tribool ccc_bs_empty(ccc_bitset const *bs);

/** @brief Return the number of bits set to CCC_TRUE. O(n).
@param [in] bs a pointer to the bit set.
@return the total number of bits currently set to CCC_TRUE. 0 is returned if
bs is NULL. */
size_t ccc_bs_popcount(ccc_bitset const *bs);

/** @brief Return the number of bits set to CCC_TRUE in the range. O(n).
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return the total number of bits currently set in the range to CCC_TRUE. A -1 is
returned if bs is NULL, i is invalid, count is invalid, or both i and count are
invalid. */
ptrdiff_t ccc_bs_popcount_range(ccc_bitset const *bs, size_t i, size_t count);

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

/**@}*/

#endif /* CCC_BITSET */
