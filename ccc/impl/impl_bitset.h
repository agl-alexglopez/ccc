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
#define ccc_impl_bs_init(bitblock_ptr, alloc_fn, aux, cap, ...)                \
    {                                                                          \
        .mem_ = (bitblock_ptr),                                                \
        .sz_ = IMPL_BS_OPTIONAL_SIZE(__VA_ARGS__),                             \
        .cap_ = (cap),                                                         \
        .alloc_ = (alloc_fn),                                                  \
        .aux_ = (aux),                                                         \
    }

#endif /* CCC_IMPL_BITSET */
