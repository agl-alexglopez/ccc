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
typedef unsigned ccc_bitblock;

/** @private A bitset is a contiguous array of fixed size integers. These aid
in cache friendly storage and operations.

By default a bit set is initialized with size equal to capacity but the user may
select to initialize a 0 sized bit set with non-zero capacity for pushing bits
back dynamically. */
struct ccc_bitset
{
    /** The array of bit blocks, a platform defined standard bit width. */
    ccc_bitblock *blocks;
    /** The number of active bits in the set available for reads and writes. */
    size_t count;
    /** The number of bits capable of being tracked in the bit block array. */
    size_t capacity;
    /** The user provided allocation function for resizing, if any. */
    ccc_any_alloc_fn *alloc;
    /** Auxiliary data for resizing, if any. */
    void *aux;
};

/** @private */
#define ccc_impl_bs_blocks(impl_bit_cap)                                       \
    (((impl_bit_cap) + ((sizeof(ccc_bitblock) * CHAR_BIT) - 1))                \
     / (sizeof(ccc_bitblock) * CHAR_BIT))

/** @private */
#define IMPL_BS_NON_IMPL_BS_DEFAULT_SIZE(impl_cap, ...) __VA_ARGS__
/** @private */
#define IMPL_BS_DEFAULT_SIZE(impl_cap, ...) impl_cap
/** @private */
#define IMPL_BS_OPTIONAL_SIZE(impl_cap, ...)                                   \
    __VA_OPT__(IMPL_BS_NON_)##IMPL_BS_DEFAULT_SIZE(impl_cap, __VA_ARGS__)

/** @private Capacity is required argument from the user while size is optional.
The optional size param defaults equal to capacity if not provided. This covers
most common cases--fixed size bit set, 0 sized dynamic bit set--and when the
user wants a fixed size dynamic bit set they provide 0 as size argument. */
#define ccc_impl_bs_init(impl_bitblock_ptr, impl_alloc_fn, impl_aux, impl_cap, \
                         ...)                                                  \
    {                                                                          \
        .blocks = (impl_bitblock_ptr),                                         \
        .count = IMPL_BS_OPTIONAL_SIZE((impl_cap), __VA_ARGS__),               \
        .capacity = (impl_cap),                                                \
        .alloc = (impl_alloc_fn),                                              \
        .aux = (impl_aux),                                                     \
    }

#endif /* CCC_IMPL_BITSET */
