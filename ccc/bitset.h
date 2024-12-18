#ifndef CCC_BITSET
#define CCC_BITSET

#include <stddef.h>
#include <stdint.h>

#include "impl/impl_bitset.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

typedef ccc_bitblock_ ccc_bitblock;

typedef struct ccc_bitset_ ccc_bitset;

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
    = ccc_btst_init((static ccc_bitblock[ccc_bitblocks(256)]){}, 256, NULL,
                    NULL);
```

The above example also illustrates the benefits of a static compound literal
to encapsulate the bits within the bit set array to prevent dangling references.
If your compiler has not implemented storage duration of compound literals the
more traditional example would look like this.

```
static ccc_bitblock blocks[ccc_bitblocks(256)];
static ccc_bitset b = ccc_btst_init(blocks, 256, NULL, NULL);
```

This macro is required for any initialization where the bit block memory comes
from the stack or data segment as determined by the user. */
#define ccc_bitblocks(bit_cap) ccc_impl_bitblocks(bit_cap)

/** @brief Initialize the bit set with memory and allocation permissions.
@param [in] bitblock_ptr the pointer to existing blocks or NULL.
@param [in] cap the number of bits that will be stored in this bit set.
@param [in] alloc_fn the allocation function for a dynamic bit set or NULL.
@param [in] aux auxiliary data needed for allocation of the bit set.
@return the initialized bit set on the right hand side of an equality operator
(e.g. ccc_bitset b = ccc_btst_init(...);). */
#define ccc_btst_init(bitblock_ptr, cap, alloc_fn, aux)                        \
    ccc_impl_btst_init(bitblock_ptr, cap, alloc_fn, aux)

/**@}*/

/** @name Bit Modification Interface
Set and flip bits in the set. */
/**@{*/

/** @brief Set the bit at index i to value b (CCC_TRUE or CCC_FALSE).
@param [in] btst a pointer to the bit set.
@param [in] i the index identifying the bit to set.
@param [in] b the value to set at position i (CCC_TRUE or CCC_FALSE).
@return the state of the bit before the set operation, true if it was
previously true, false if it was previously false, or error if btst is NULL.
@warning no bounds checking occurs in the release target. For bounds checking,
see ccc_btst_set_at(). */
ccc_tribool ccc_btst_set(ccc_bitset *btst, size_t i, ccc_tribool b);

/** @brief Set the bit at valid index i to value b (true or false).
@param [in] btst a pointer to the bit set.
@param [in] i the valid index identifying the bit to set.
@param [in] b the value to set at position i (CCC_TRUE or CCC_FALSE).
@return the state of the bit before the set operation, true if it was
previously true, false if it was previously false, or error if btst is NULL or
i is out of range.
@note this function performs bounds checking in the release target. */
ccc_tribool ccc_btst_set_at(ccc_bitset *btst, size_t i, ccc_tribool b);

/** @brief Set all the bits to the provided value (CCC_TRUE or CCC_FALSE).
@param [in] btst a pointer to the bit set.
@param [in] b the value to set (CCC_TRUE or CCC_FALSE).
@return the result of the operation. OK if successful, or an input error if
btst is NULL. */
ccc_result ccc_btst_set_all(ccc_bitset *btst, ccc_tribool b);

/** @brief Set the bit at index i to CCC_FALSE.
@param [in] btst a pointer to the bit set.
@param [in] i the index identifying the bit to reset.
@return the state of the bit before the reset operation, true if it was
previously true, false if it was previously false, or error if btsts is NULL.
@warning no bounds checking occurs in the release target. For bounds checking,
see ccc_btst_set_at(). */
ccc_tribool ccc_btst_reset(ccc_bitset *btst, size_t i);

/** @brief Set the bit at valid index i to boolean value b (true or false).
@param [in] btst a pointer to the bit set.
@param [in] i the valid index identifying the bit to set.
@return the state of the bit before the set operation, true if it was
previously true, false if it was previously false, or error if btst is NULL or
i is out of range.
@note this function performs bounds checking in the release target. */
ccc_tribool ccc_btst_reset_at(ccc_bitset *btst, size_t i);

/** @brief Set all the bits to CCC_FALSE.
@param [in] btst a pointer to the bit set.
@return the result of the operation. OK if successful, or an input error if
btst is NULL. */
ccc_result ccc_btst_reset_all(ccc_bitset *btst);

/** @brief Toggle the bit at index i.
@param [in] btst a pointer to the bit set.
@param [in] i the index identifying the bit to toggle
@return the state of the bit before the toggle operation, true if it was
previously true, false if it was previously false, or error if btsts is NULL.
@warning no bounds checking occurs in the release target. For bounds checking,
see ccc_btst_set_at(). */
ccc_tribool ccc_btst_flip(ccc_bitset *btst, size_t i);

/** @brief Toggle the bit at index i.
@param [in] btst a pointer to the bit set.
@param [in] i the index identifying the bit to toggle
@return the state of the bit before the toggle operation, true if it was
previously true, false if it was previously false, or error if btst is NULL or
i is out of range.
@note this function performs bounds checking in the release target. */
ccc_tribool ccc_btst_flip_at(ccc_bitset *btst, size_t i);

/** @brief Toggle all of the bits to their opposing boolean value.
@param [in] btst a pointer to the bit set.
@return the result of the operation. OK if successful, or an input error if
btst is NULL. */
ccc_result ccc_btst_flip_all(ccc_bitset *btst);

/**@}*/

/** @name State Interface
Obtain state from the container. */
/**@{*/

/** @brief Return total number of bits tracked by the set.
@param [in] btst a pointer to the bit set.
@return the total number of bits currently tracked by the set regardless of
true or false state of each. 0 is returned if btst is NULL. */
size_t ccc_btst_capacity(ccc_bitset const *btst);

/** @brief Return the number of bits set to CCC_TRUE. O(n).
@param [in] btst a pointer to the bit set.
@return the total number of bits currently set to CCC_TRUE. 0 is returned if
btst is NULL. */
size_t ccc_btst_popcount(ccc_bitset const *btst);

/**@}*/

#endif /* CCC_BITSET */
