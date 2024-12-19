#ifndef CCC_IMPL_BITSET
#define CCC_IMPL_BITSET

#include <limits.h>
#include <stddef.h>

#include "types.h"

/** @private */
typedef unsigned ccc_bitblock_;

/** @private */
struct ccc_bitset_
{
    ccc_bitblock_ *set_;
    size_t cap_;
    ccc_alloc_fn *alloc_;
    void *aux_;
};

/** @private Number of bits in a block. */
#define CCC_IMPL_BTST_BLOCK_BITS (sizeof(ccc_bitblock_) * CHAR_BIT)

/** @private */
#define ccc_impl_bitblocks(bit_cap)                                            \
    (((bit_cap) + (CCC_IMPL_BTST_BLOCK_BITS - 1)) / CCC_IMPL_BTST_BLOCK_BITS)

/** @private */
#define ccc_impl_btst_init(bitblock_ptr, cap, alloc_fn, aux)                   \
    {                                                                          \
        .set_ = (bitblock_ptr),                                                \
        .cap_ = (cap),                                                         \
        .alloc_ = (alloc_fn),                                                  \
        .aux_ = (aux),                                                         \
    }

#endif /* CCC_IMPL_BITSET */
