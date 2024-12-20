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

typedef struct ccc_bitset_ ccc_bitset;

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
@return the initialized bit set on the right hand side of an equality operator
@warning the user must use the ccc_bs_blocks macro to help determine the size of
the bitblock array if a fixed size bitblock array is provided at compile time;
the necessary conversion from bits requested to number of blocks required to
store those bits must occur before use.
(e.g. ccc_bitset b = ccc_bs_init((ccc_bitblock[ccc_bs_blocks(9)]){},...);). */
#define ccc_bs_init(bitblock_ptr, cap, alloc_fn, aux)                          \
    ccc_impl_bs_init(bitblock_ptr, cap, alloc_fn, aux)

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
ccc_tribool ccc_bs_test(ccc_bitset const *, size_t i);

/** @brief Test the bit at index i for boolean status (CCC_TRUE or CCC_FALSE).
@param [in] bs a pointer to the bit set.
@param [in] i the index identifying the bit to set.
@return the state of the bit, or CCC_BOOL_ERR if bs is NULL.
@note this function performs bounds checking in the release target. */
ccc_tribool ccc_bs_test_at(ccc_bitset const *, size_t i);

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

/** @name State Interface
Obtain state from the container. */
/**@{*/

/** @brief Return total number of bits tracked by the set.
@param [in] bs a pointer to the bit set.
@return the total number of bits currently tracked by the set regardless of
true or false state of each. 0 is returned if bs is NULL. */
size_t ccc_bs_capacity(ccc_bitset const *bs);

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

/** @brief Return true if any bits in set are on.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if any bits are on, CCC_FALSE if no bits are on, CCC_BOOL_ERR
if bs is NULL. */
ccc_tribool ccc_bs_any(ccc_bitset const *bs);

/** @brief Return true if any bits are on in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return CCC_TRUE if any bits are on, CCC_FALSE if no bits are on, CCC_BOOL_ERR
if bs is NULL, i is invalid, count is invalid, or both i and count are
invalid. */
ccc_tribool ccc_bs_any_range(ccc_bitset const *bs, size_t i, size_t count);

/** @brief Return true if no bits in set are on.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if no bits are on, CCC_FALSE if any bits are on, CCC_BOOL_ERR
if bs is NULL. */
ccc_tribool ccc_bs_none(ccc_bitset const *bs);

/** @brief Return true if no bits are on in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return CCC_TRUE if no bits are on, CCC_FALSE if any bits are on, CCC_BOOL_ERR
if bs is NULL, i is invalid, count is invalid, or both i and count are
invalid. */
ccc_tribool ccc_bs_none_range(ccc_bitset const *bs, size_t i, size_t count);

/** @brief Return true if no bits in set are on.
@param [in] bs a pointer to the bit set.
@return CCC_TRUE if no bits are on, CCC_FALSE if any bits are on, CCC_BOOL_ERR
if bs is NULL. */
ccc_tribool ccc_bs_all(ccc_bitset const *bs);

/** @brief Return true if no bits are on in the specified range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting position.
@param [in] count the size of the range to check.
@return CCC_TRUE if no bits are on, CCC_FALSE if any bits are on, CCC_BOOL_ERR
if bs is NULL, i is invalid, count is invalid, or both i and count are
invalid. */
ccc_tribool ccc_bs_all_range(ccc_bitset const *bs, size_t i, size_t count);

/** @brief Return the index of the first bit set to 1 in the set.
@param [in] bs a pointer to the bit set.
@return the index of the first bit set to 1 or -1 if no 1 bit is found or bs in
NULL. */
ptrdiff_t ccc_bs_first_trailing_1(ccc_bitset const *bs);

/** @brief Return the index of the first bit set to 1 in the range.
@param [in] bs a pointer to the bit set.
@param [in] i the starting index to search.
@param [in] count the size of the range to check.
@return the index of the first bit set to 1 or -1 if no 1 bit is found, bs is
NULL, or the range is invalid.
@warning the user must validate their own range. A bit does not exist in an
invalid range therefore -1 is returned. To distinguish a valid negative result
signifying not found and a negative result indicating a range error the user
must check their input. */
ptrdiff_t ccc_bs_first_trailing_1_range(ccc_bitset const *bs, size_t i,
                                        size_t count);

/** @brief Return the index of the first bit set to 0 in the set.
@param [in] bs a pointer to the bit set.
@return the index of the first bit set to 0 or -1 if no 0 bit is found or bs in
NULL. */
ptrdiff_t ccc_bs_first_trailing_0(ccc_bitset const *bs);

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
ptrdiff_t ccc_bs_first_trailing_0_range(ccc_bitset const *bs, size_t i,
                                        size_t count);

/**@}*/

#endif /* CCC_BITSET */
