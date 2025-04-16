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
#ifndef CCC_IMPL_BITSET
#define CCC_IMPL_BITSET

/** @cond */
#include <limits.h>
#include <stddef.h>
/** @endcond */

#include "../types.h"

/** @private */
typedef unsigned ccc_bitblock_;

/** @private A bitset is a contiguous array of fixed size integers. These aid
in cache friendly storage and operations.

By default a bit set is initialized with size equal to capacity but the user may
select to initialize a 0 sized bit set with non-zero capacity for pushing bits
back dynamically. */
struct ccc_bitset_
{
    /** The array of bit blocks, a platform defined standard bit width. */
    ccc_bitblock_ *mem_;
    /** The number of active bits in the set available for reads and writes. */
    size_t sz_;
    /** The number of bits capable of being tracked in the bit block array. */
    size_t cap_;
    /** The user provided allocation function for resizing, if any. */
    ccc_any_alloc_fn *alloc_;
    /** Auxiliary data for resizing, if any. */
    void *aux_;
};

/** @private */
#define ccc_impl_bs_blocks(bit_cap)                                            \
    (((bit_cap) + ((sizeof(ccc_bitblock_) * CHAR_BIT) - 1))                    \
     / (sizeof(ccc_bitblock_) * CHAR_BIT))

/** @private */
#define IMPL_BS_NON_IMPL_BS_DEFAULT_SIZE(cap, ...) __VA_ARGS__
/** @private */
#define IMPL_BS_DEFAULT_SIZE(cap, ...) cap
/** @private */
#define IMPL_BS_OPTIONAL_SIZE(cap, ...)                                        \
    __VA_OPT__(IMPL_BS_NON_)##IMPL_BS_DEFAULT_SIZE(cap, __VA_ARGS__)

/** @private Capacity is required argument from the user while size is optional.
The optional size param defaults equal to capacity if not provided. This covers
most common cases--fixed size bit set, 0 sized dynamic bit set--and when the
user wants a fixed size dynamic bit set they provide 0 as size argument. */
#define ccc_impl_bs_init(bitblock_ptr, alloc_fn, aux, cap, ...)                \
    {                                                                          \
        .mem_ = (bitblock_ptr),                                                \
        .sz_ = IMPL_BS_OPTIONAL_SIZE((cap), __VA_ARGS__),                      \
        .cap_ = (cap),                                                         \
        .alloc_ = (alloc_fn),                                                  \
        .aux_ = (aux),                                                         \
    }

#endif /* CCC_IMPL_BITSET */
