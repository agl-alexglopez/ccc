#ifndef CCC_IMPL_BITSET
#define CCC_IMPL_BITSET

/** @cond */
#include <limits.h>
#include <stddef.h>
/** @endcond */

#include "../types.h"

/** @private */
typedef unsigned ccc_bitblock_;

/** @private */
struct ccc_bitset_
{
    ccc_bitblock_ *mem_;
    size_t sz_;
    size_t cap_;
    ccc_alloc_fn *alloc_;
    void *aux_;
};

/** @private */
#define ccc_impl_bs_blocks(bit_cap)                                            \
    (((bit_cap) + ((sizeof(ccc_bitblock_) * CHAR_BIT) - 1))                    \
     / (sizeof(ccc_bitblock_) * CHAR_BIT))

/** @private */
#define IMPL_BS_NON_IMPL_BS_DEFAULT_SIZE(...) __VA_ARGS__
/** @private */
#define IMPL_BS_DEFAULT_SIZE(...) 0
/** @private */
#define IMPL_BS_OPTIONAL_SIZE(...)                                             \
    __VA_OPT__(IMPL_BS_NON_)##IMPL_BS_DEFAULT_SIZE(__VA_ARGS__)

/** @private */
#define ccc_impl_bs_init(bitblock_ptr, cap, alloc_fn, aux, ...)                \
    {                                                                          \
        .mem_ = (bitblock_ptr),                                                \
        .sz_ = IMPL_BS_OPTIONAL_SIZE(__VA_ARGS__),                             \
        .cap_ = (cap),                                                         \
        .alloc_ = (alloc_fn),                                                  \
        .aux_ = (aux),                                                         \
    }

#endif /* CCC_IMPL_BITSET */
