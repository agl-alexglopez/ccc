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

/** @private A bitset is a contiguous array of fixed size integers. These aid
in cache friendly storage and operations.

By default a bit set is initialized with size equal to capacity but the user may
select to initialize a 0 sized bit set with non-zero capacity for pushing bits
back dynamically. */
struct ccc_bitset
{
    /** The array of bit blocks, a platform defined standard bit width. */
    unsigned *blocks;
    /** The number of active bits in the set available for reads and writes. */
    size_t count;
    /** The number of bits capable of being tracked in the bit block array. */
    size_t capacity;
    /** The user provided allocation function for resizing, if any. */
    ccc_any_alloc_fn *alloc;
    /** Auxiliary data for resizing, if any. */
    void *aux;
};

enum : size_t
{
    /** @private The number of bits in a bit block. In sync with set type. */
    CCC_IMPL_BS_BLOCK_BITS = (sizeof(*(struct ccc_bitset){}.blocks) * CHAR_BIT),
};

/*=========================     Private Interface   =========================*/

ccc_result ccc_impl_bs_reserve(struct ccc_bitset *, size_t to_add,
                               ccc_any_alloc_fn *);
ccc_tribool ccc_impl_bs_set(struct ccc_bitset *bs, size_t i, ccc_tribool b);

/*================================     Macros     ===========================*/

/** @private Returns the number of blocks needed to support a given capacity
of bits. Assumes the given capacity is greater than 0. Classic div round up. */
#define ccc_impl_bs_block_count(impl_bit_cap)                                  \
    (((impl_bit_cap) + (CCC_IMPL_BS_BLOCK_BITS - 1)) / CCC_IMPL_BS_BLOCK_BITS)

/** @private Returns the number of bytes needed for the required blocks. */
#define ccc_impl_bs_block_bytes(impl_bit_cap)                                  \
    (sizeof(*(struct ccc_bitset){}.blocks) * (impl_bit_cap))

/** @private Allocates a compound literal bit block array in the scope at which
the macro is used. However, the optional parameter supports storage duration
specifiers which is a feature of C23. Not all compilers support this yet. */
#define ccc_impl_bs_blocks(impl_bit_cap, ...)                                  \
    (__VA_OPT__(__VA_ARGS__) typeof (                                          \
        *(struct ccc_bitset){}.blocks)[ccc_impl_bs_block_count(impl_bit_cap)]) \
    {}

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

/** @private Rare instance where we don't need typing or macro trickery and
we get to use static inline for improved functionality. Once the macro learns
whether the user wants a capacity we pass the functionality of parsing the
input string to this function. */
static inline struct ccc_bitset
ccc_impl_bs_from_fn(ccc_any_alloc_fn *const fn, void *const aux, size_t i,
                    size_t const count, size_t cap, char const on_char,
                    char const *string)
{
    struct ccc_bitset bs = ccc_impl_bs_init(NULL, fn, aux, 0);
    (void)ccc_impl_bs_reserve(&bs, cap < count ? count : cap, fn);
    bs.count = count;
    while (i < count && string[i])
    {
        (void)ccc_impl_bs_set(&bs, i, string[i] == on_char);
        ++i;
    }
    bs.count = i;
    return bs;
}

/** @private Determine if user wants capacity different than count. Then pass
to inline function for bit set construction. */
#define ccc_impl_bs_from(impl_alloc_fn, impl_aux, impl_start_index,            \
                         impl_count, impl_on_char, impl_string, ...)           \
    ccc_impl_bs_from_fn(impl_alloc_fn, impl_aux, impl_start_index, impl_count, \
                        IMPL_BS_OPTIONAL_SIZE((impl_count), __VA_ARGS__),      \
                        impl_on_char, impl_string)

#endif /* CCC_IMPL_BITSET */
