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
#ifndef CCC_PRIVATE_BITSET
#define CCC_PRIVATE_BITSET

/** @cond */
#include <limits.h>
#include <stddef.h>
/** @endcond */

#include "../types.h"

/** @private A Bitset is a contiguous array of fixed size integers. These aid
in cache friendly storage and operations.

By default a bit set is initialized with size equal to capacity but the user may
select to initialize a 0 sized bit set with non-zero capacity for pushing bits
back dynamically. */
struct CCC_Bitset
{
    /** The array of bit blocks, a platform defined standard bit width. */
    unsigned *blocks;
    /** The number of active bits in the set available for reads and writes. */
    size_t count;
    /** The number of bits capable of being tracked in the bit block array. */
    size_t capacity;
    /** The user provided allocation function for resizing, if any. */
    CCC_Allocator *allocate;
    /** Auxiliary data for resizing, if any. */
    void *context;
};

enum : size_t
{
    /** @private The number of bits in a bit block. In sync with set type. */
    CCC_PRIVATE_BITSET_BLOCK_BITS
        = (sizeof(*(struct CCC_Bitset){}.blocks) * CHAR_BIT),
};

/*=========================     Private Interface   =========================*/

CCC_Result CCC_private_bitset_reserve(struct CCC_Bitset *, size_t to_add,
                                      CCC_Allocator *);
CCC_Tribool CCC_private_bitset_set(struct CCC_Bitset *bs, size_t i,
                                   CCC_Tribool b);

/*================================     Macros     ===========================*/

/** @private Returns the number of blocks needed to support a given capacity
of bits. Assumes the given capacity is greater than 0. Classic div round up. */
#define CCC_private_bitset_block_count(private_bit_cap)                        \
    (((private_bit_cap) + (CCC_PRIVATE_BITSET_BLOCK_BITS - 1))                 \
     / CCC_PRIVATE_BITSET_BLOCK_BITS)

/** @private Returns the number of bytes needed for the required blocks. */
#define CCC_private_bitset_block_bytes(private_bit_cap)                        \
    (sizeof(*(struct CCC_Bitset){}.blocks) * (private_bit_cap))

/** @private Allocates a compound literal bit block array in the scope at which
the macro is used. However, the optional parameter supports storage duration
specifiers which is a feature of C23. Not all compilers support this yet. */
#define CCC_private_bitset_blocks(private_bit_cap, ...)                        \
    (__VA_OPT__(__VA_ARGS__) typeof (                                          \
        *(struct CCC_Bitset){}                                                 \
             .blocks)[CCC_private_bitset_block_count(private_bit_cap)])        \
    {}

/** @private NOLINTNEXTLINE */
#define CCC_private_bitset_non_CCC_private_bitset_default_size(private_cap,    \
                                                               ...)            \
    __VA_ARGS__
/** @private */
#define CCC_private_bitset_default_size(private_cap, ...) private_cap
/** @private */
#define CCC_private_bitset_optional_size(private_cap, ...)                     \
    __VA_OPT__(CCC_private_bitset_non_)                                        \
    ##CCC_private_bitset_default_size(private_cap, __VA_ARGS__)

/** @private Capacity is required argument from the user while size is optional.
The optional size param defaults equal to capacity if not provided. This covers
most common cases--fixed size bit set, 0 sized dynamic bit set--and when the
user wants a fixed size dynamic bit set they provide 0 as size argument. */
#define CCC_private_bitset_initialize(private_bitblock_pointer,                \
                                      private_allocate, private_context,       \
                                      private_cap, ...)                        \
    {                                                                          \
        .blocks = (private_bitblock_pointer),                                  \
        .count = CCC_private_bitset_optional_size((private_cap), __VA_ARGS__), \
        .capacity = (private_cap),                                             \
        .allocate = (private_allocate),                                        \
        .context = (private_context),                                          \
    }

/** @private Rare instance where we don't need typing or macro trickery and
we get to use static inline for improved functionality. Once the macro learns
whether the user wants a capacity we pass the functionality of parsing the
input string to this function. */
static inline struct CCC_Bitset
CCC_private_bitset_from_fn(CCC_Allocator *const fn, void *const context,
                           size_t i, size_t const count, size_t cap,
                           char const on_char, char const *string)
{
    struct CCC_Bitset b = CCC_private_bitset_initialize(NULL, fn, context, 0);
    if (CCC_private_bitset_reserve(&b, cap < count ? count : cap, fn)
        != CCC_RESULT_OK)
    {
        return b;
    }
    b.count = count;
    while (i < count && string[i])
    {
        (void)CCC_private_bitset_set(&b, i, string[i] == on_char);
        ++i;
    }
    b.count = i;
    return b;
}

/** @private Returns a bit set with the memory reserved for the blocks and
the size set. */
static inline struct CCC_Bitset
CCC_private_bitset_with_capacity_fn(CCC_Allocator *const fn,
                                    void *const context, size_t const cap,
                                    size_t const count)
{
    struct CCC_Bitset b = CCC_private_bitset_initialize(NULL, fn, context, 0);
    if (CCC_private_bitset_reserve(&b, cap, fn) == CCC_RESULT_OK)
    {
        b.count = count;
    }
    return b;
}

/** @private Determine if user wants capacity different than count. Then pass
to inline function for bit set construction. */
#define CCC_private_bitset_from(private_allocate, private_context,             \
                                private_start_index, private_count,            \
                                private_on_char, private_string, ...)          \
    CCC_private_bitset_from_fn(                                                \
        private_allocate, private_context, private_start_index, private_count, \
        CCC_private_bitset_optional_size((private_count), __VA_ARGS__),        \
        private_on_char, private_string)

/** @private Macro wrapper allowing user to optionally specify size. */
#define CCC_private_bitset_with_capacity(private_allocate, private_context,    \
                                         private_cap, ...)                     \
    CCC_private_bitset_with_capacity_fn(                                       \
        private_allocate, private_context, private_cap,                        \
        CCC_private_bitset_optional_size((private_cap), __VA_ARGS__))

#endif /* CCC_PRIVATE_BITSET */
