/** Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file implements a Bit Set using blocks of platform defined integers
for speed and efficiency. The implementation aims for constant and linear time
operations at all times, specifically when implementing more complicated range
based operations over the set. */
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bitset.h"
#include "private/private_bitset.h"
#include "types.h"

/*=========================   Type Declarations  ============================*/

typedef typeof(*(struct CCC_Bitset){}.blocks) Bitblock;

/** @internal Used frequently so call the builtin just once. */
enum : size_t
{
    /** @internal Bytes of a bit block to help with byte calculations. */
    SIZEOF_BLOCK = sizeof(Bitblock),
};

/** @internal Various constants to support bit block size bit ops. */
enum : Bitblock
{
    /** @internal A mask of a bit block with all bits on. */
    BITBLOCK_ON = ((Bitblock)~0),
    /** @internal The Most Significant Bit of a bit block turned on to 1. */
    BITBLOCK_MSB = (((Bitblock)1) << (((SIZEOF_BLOCK * CHAR_BIT)) - 1)),
};

/** @internal An index into the block array or count of bit blocks. The block
array is bounded by the number of blocks required to support the current bit set
capacity. Assume this index type has range [0, block count to support N bits].

User input is given as a `size_t` so distinguish from that input with this type
to make it clear to the reader the index refers to a block not the given bit
index the user has provided. */
typedef size_t Block_count;

/** @internal A signed index into the block array. The block array is bounded by
the number of blocks required to support the current bit set capacity. Assume
this index type has range [-1, count of blocks needed to hold all bits in set].
This makes reverse iteration problems easier.

Most platforms will allow for this signed type to index any bit block that the
unsigned equivalent can index into. However, one should always check that the
unsigned value can safely be cast to signed.

User input is given as a `size_t` so distinguish from that input with this type
to make it clear to the reader the index refers to a block not the given bit
index the user has provided. */
typedef ptrdiff_t Block_signed_count;

/** @internal An index within a block. A block is bounded to some number of bits
as determined by the type used for each block. This type is intended to count
bits in a block and therefore cannot count up to arbitrary indices. Assume its
range is `[0, BITBLOCK_BITS]`, for ease of use and clarity.

There are many types of indexing and counting that take place in a bit set so
use this type specifically for counting bits in a block so the reader is clear
on intent. */
typedef uint8_t Bit_count;

enum : Bit_count
{
    /** @internal How many total bits that fit in a bit block. */
    BITBLOCK_BITS = (SIZEOF_BLOCK * CHAR_BIT),
    U8BLOCK_MAX = UINT8_MAX,
};
static_assert((Bit_count)~0 >= 0, "Bit_count must be unsigned");
static_assert(UINT8_MAX >= BITBLOCK_BITS, "Bit_count counts all block bits.");

/** @internal A signed index within a block. A block is bounded to some number
of bits as determined by the type used for each block. This type is intended to
count bits in a block and therefore cannot count up to arbitrary indices. Assume
its range is `[-1, BITBLOCK_BITS]`, for ease of use and clarity.

There are many types of indexing and counting that take place in a bit set so
use this type specifically for counting bits in a block so the reader is clear
on intent. It helps clean up algorithms for finding ranges of leading bits.
It also a wider type to avoid warnings or dangers when taking the value of a
`Bit_count` type. */
typedef int16_t Bit_signed_count;
static_assert(sizeof(Bit_signed_count) > sizeof(Bit_count),
              "Bit_signed_count x = (Bit_signed_count)x_Bit_count; is safe");
static_assert((Bit_signed_count)~0 < 0, "Bit_signed_count must be signed");
static_assert(INT16_MAX >= BITBLOCK_BITS,
              "Bit_signed_count counts all block bits");

/** @internal A helper to allow for an efficient linear scan for groups of 0's
or 1's in the set. */
struct Group_count
{
    /** A index [0, bit block bits] indicating the status of a search. */
    Bit_count i;
    /** The number of bits of same value found starting at block_start_i. */
    size_t count;
};

/** @internal A signed helper to allow for an efficient linear scan for groups
of 0's or 1's in the set. Created to support -1 index return values for cleaner
group scanning in reverse. */
struct Group_signed_count
{
    /** A index [-1, bit block bits] indicating the status of a search. */
    Bit_signed_count i;
    /** The number of bits of same value found starting at block_start_i. */
    size_t count;
};

/*=========================      Prototypes      ============================*/

static size_t block_count_index(size_t bitset_index);
static inline Bitblock *block_at(struct CCC_Bitset const *bs,
                                 size_t bitset_index);
static void set(Bitblock *, size_t bitset_index, CCC_Tribool);
static Bitblock on(size_t bitset_index);
static void fix_end(struct CCC_Bitset *);
static CCC_Tribool status(Bitblock const *, size_t bitset_index);
static size_t block_count(size_t bits);
static CCC_Tribool any_or_none_range(struct CCC_Bitset const *, size_t i,
                                     size_t count, CCC_Tribool);
static CCC_Tribool all_range(struct CCC_Bitset const *bs, size_t i,
                             size_t count);
static CCC_Count first_trailing_one_range(struct CCC_Bitset const *bs, size_t i,
                                          size_t count);
static CCC_Count first_trailing_zero_range(struct CCC_Bitset const *bs,
                                           size_t i, size_t count);
static CCC_Count first_leading_one_range(struct CCC_Bitset const *bs, size_t i,
                                         size_t count);
static CCC_Count first_leading_zero_range(struct CCC_Bitset const *bs, size_t i,
                                          size_t count);
static CCC_Count first_trailing_bits_range(struct CCC_Bitset const *bs,
                                           size_t i, size_t count,
                                           size_t num_bits, CCC_Tribool is_one);
static CCC_Count first_leading_bits_range(struct CCC_Bitset const *bs, size_t i,
                                          size_t count, size_t num_bits,
                                          CCC_Tribool is_one);
static struct Group_count max_trailing_ones(Bitblock b, Bit_count i,
                                            size_t num_ones_remain);
static struct Group_signed_count
max_leading_ones(Bitblock b, Bit_signed_count i, size_t num_ones_remaining);
static CCC_Result maybe_resize(struct CCC_Bitset *bs, size_t to_add,
                               CCC_Allocator *);
static size_t size_t_min(size_t, size_t);
static void set_all(struct CCC_Bitset *bs, CCC_Tribool b);
static Bit_count bit_count_index(size_t bitset_index);
static CCC_Tribool is_subset_of(struct CCC_Bitset const *set,
                                struct CCC_Bitset const *subset);
static Bit_count popcount(Bitblock);
static Bit_count ctz(Bitblock);
static Bit_count clz(Bitblock);

/*=======================   Public Interface   ==============================*/

CCC_Tribool
CCC_bitset_is_proper_subset(CCC_Bitset const *const set,
                            CCC_Bitset const *const subset)
{
    if (!set || !subset)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (set->count <= subset->count)
    {
        return CCC_FALSE;
    }
    return is_subset_of(set, subset);
}

CCC_Tribool
CCC_bitset_is_subset(CCC_Bitset const *const set,
                     CCC_Bitset const *const subset)
{
    if (!set || !subset)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (set->count < subset->count)
    {
        return CCC_FALSE;
    }
    return is_subset_of(set, subset);
}

CCC_Result
CCC_bitset_or(CCC_Bitset *const dst, CCC_Bitset const *const src)
{
    if (!dst || !src)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!dst->count || !src->count)
    {
        return CCC_RESULT_OK;
    }
    Block_count const end_block
        = block_count(size_t_min(dst->count, src->count));
    for (size_t b = 0; b < end_block; ++b)
    {
        dst->blocks[b] |= src->blocks[b];
    }
    fix_end(dst);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_bitset_xor(CCC_Bitset *const dst, CCC_Bitset const *const src)
{
    if (!dst || !src)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!dst->count || !src->count)
    {
        return CCC_RESULT_OK;
    }
    Block_count const end_block
        = block_count(size_t_min(dst->count, src->count));
    for (Block_count b = 0; b < end_block; ++b)
    {
        dst->blocks[b] ^= src->blocks[b];
    }
    fix_end(dst);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_bitset_and(CCC_Bitset *dst, CCC_Bitset const *src)
{
    if (!dst || !src)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!src->count)
    {
        set_all(dst, CCC_FALSE);
        return CCC_RESULT_OK;
    }
    if (!dst->count)
    {
        return CCC_RESULT_OK;
    }
    Block_count const end_block
        = block_count(size_t_min(dst->count, src->count));
    for (Block_count b = 0; b < end_block; ++b)
    {
        dst->blocks[b] &= src->blocks[b];
    }
    if (dst->count <= src->count)
    {
        return CCC_RESULT_OK;
    }
    /* The src widens to align with dst as integers would; same consequences. */
    Block_count const dst_blocks = block_count(dst->count);
    Block_count const remain = dst_blocks - end_block;
    (void)memset(dst->blocks + end_block, CCC_FALSE, remain * SIZEOF_BLOCK);
    fix_end(dst);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_bitset_shiftl(CCC_Bitset *const bs, size_t const left_shifts)
{
    if (!bs)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!bs->count || !left_shifts)
    {
        return CCC_RESULT_OK;
    }
    if (left_shifts >= bs->count)
    {
        set_all(bs, CCC_FALSE);
        return CCC_RESULT_OK;
    }
    Block_count const end = block_count_index(bs->count - 1);
    Block_count const blocks = block_count_index(left_shifts);
    Bit_count const split = bit_count_index(left_shifts);
    if (!split)
    {
        for (Block_count shift = end - blocks + 1, write = end; shift--;
             --write)
        {
            bs->blocks[write] = bs->blocks[shift];
        }
    }
    else
    {
        Bit_count const remain = BITBLOCK_BITS - split;
        for (Block_count shift = end - blocks, write = end; shift > 0;
             --shift, --write)
        {
            bs->blocks[write] = (bs->blocks[shift] << split)
                              | (bs->blocks[shift - 1] >> remain);
        }
        bs->blocks[blocks] = bs->blocks[0] << split;
    }
    /* Zero fills in lower bits just as an integer shift would. */
    for (Block_count i = 0; i < blocks; ++i)
    {
        bs->blocks[i] = 0;
    }
    fix_end(bs);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_bitset_shiftr(CCC_Bitset *const bs, size_t const right_shifts)
{
    if (!bs)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!bs->count || !right_shifts)
    {
        return CCC_RESULT_OK;
    }
    if (right_shifts >= bs->count)
    {
        set_all(bs, CCC_FALSE);
        return CCC_RESULT_OK;
    }
    Block_count const end = block_count_index(bs->count - 1);
    Block_count const blocks = block_count_index(right_shifts);
    Bit_count split = bit_count_index(right_shifts);
    if (!split)
    {
        for (Block_count shift = blocks, write = 0; shift < end + 1;
             ++shift, ++write)
        {
            bs->blocks[write] = bs->blocks[shift];
        }
    }
    else
    {
        Bit_count const remain = BITBLOCK_BITS - split;
        for (Block_count shift = blocks, write = 0; shift < end;
             ++shift, ++write)
        {
            bs->blocks[write] = (bs->blocks[shift + 1] << remain)
                              | (bs->blocks[shift] >> split);
        }
        bs->blocks[end - blocks] = bs->blocks[end] >> split;
    }
    /* This is safe for a few reasons:
       - If shifts equals count we set all to 0 and returned early.
       - If we only have one block i will be equal to end and we are done.
       - If end is the 0th block we will stop after 1. A meaningful shift
         occurred in the 0th block so zeroing would be a mistake.
       - All other cases ensure it is safe to decrease i (no underflow).
       This operation emulates the zeroing of high bits on a right shift and
       a bit set is considered unsigned so we don't sign bit fill. */
    for (Block_count i = end; i > end - blocks; --i)
    {
        bs->blocks[i] = 0;
    }
    fix_end(bs);
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_bitset_test(CCC_Bitset const *const bs, size_t const i)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (i >= bs->count)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return status(block_at(bs, i), i);
}

CCC_Tribool
CCC_bitset_set(CCC_Bitset *const bs, size_t const i, CCC_Tribool const b)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (i >= bs->count)
    {
        return CCC_TRIBOOL_ERROR;
    }
    Bitblock *const block = block_at(bs, i);
    CCC_Tribool const was = status(block, i);
    set(block, i, b);
    return was;
}

CCC_Result
CCC_bitset_set_all(CCC_Bitset *const bs, CCC_Tribool const b)
{
    if (!bs)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (bs->count)
    {
        set_all(bs, b);
    }
    return CCC_RESULT_OK;
}

/** A naive implementation might just call set for every index between the
start and start + count. However, calculating the block and index within each
block for every call to set costs a division and a modulo operation. This also
loads and stores a block multiple times just to set each bit within a block to
the same value. We can avoid this by handling the first and last block with one
operations and then handling everything in between with a bulk memset. */
CCC_Result
CCC_bitset_set_range(CCC_Bitset *const bs, size_t const i, size_t const count,
                     CCC_Tribool const b)
{
    if (!bs || i >= bs->count || i + count < i)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    size_t const end_i = i + count;
    Block_count start_block = block_count_index(i);
    Bit_count const start_bit = bit_count_index(i);
    Bitblock first_block_on = BITBLOCK_ON << start_bit;
    if (start_bit + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ON << (start_bit + count));
    }

    /* Logic is uniform except for key lines to turn bits on or off. */
    b ? (bs->blocks[start_block] |= first_block_on)
      : (bs->blocks[start_block] &= ~first_block_on);

    Block_count const end_block = block_count_index(end_i - 1);
    if (end_block == start_block)
    {
        fix_end(bs);
        return CCC_RESULT_OK;
    }
    if (++start_block != end_block)
    {
        /* Bulk setting blocks to 1 or 0 is OK. Only full blocks are set. */
        int const v = b ? ~0 : 0;
        (void)memset(&bs->blocks[start_block], v,
                     (end_block - start_block) * SIZEOF_BLOCK);
    }
    Bit_count const end_bit = bit_count_index(end_i - 1);
    Bitblock const last_block_on
        = BITBLOCK_ON >> ((BITBLOCK_BITS - end_bit) - 1);

    /* Same as first block but we are careful about bits past our range. */
    b ? (bs->blocks[end_block] |= last_block_on)
      : (bs->blocks[end_block] &= ~last_block_on);

    fix_end(bs);
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_bitset_reset(CCC_Bitset *const bs, size_t const i)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (i >= bs->count)
    {
        return CCC_TRIBOOL_ERROR;
    }
    Bitblock *const block = block_at(bs, i);
    CCC_Tribool const was = status(block, i);
    *block &= ~on(i);
    fix_end(bs);
    return was;
}

CCC_Result
CCC_bitset_reset_all(CCC_Bitset *const bs)
{
    if (!bs)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (bs->count)
    {
        (void)memset(bs->blocks, CCC_FALSE,
                     block_count(bs->count) * SIZEOF_BLOCK);
    }
    return CCC_RESULT_OK;
}

/** Same concept as set range but easier. Handle first and last then set
everything in between to false with memset. */
CCC_Result
CCC_bitset_reset_range(CCC_Bitset *const bs, size_t const i, size_t const count)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    size_t const end_i = i + count;
    Block_count start_block = block_count_index(i);
    Bit_count const start_bit = bit_count_index(i);
    Bitblock first_block_on = BITBLOCK_ON << start_bit;
    if (start_bit + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ON << (start_bit + count));
    }
    bs->blocks[start_block] &= ~first_block_on;
    Block_count const end_block = block_count_index(end_i - 1);
    if (end_block == start_block)
    {
        fix_end(bs);
        return CCC_RESULT_OK;
    }
    if (++start_block != end_block)
    {
        (void)memset(&bs->blocks[start_block], CCC_FALSE,
                     (end_block - start_block) * SIZEOF_BLOCK);
    }
    Bit_count const end_bit = bit_count_index(end_i - 1);
    Bitblock const last_block_on
        = BITBLOCK_ON >> ((BITBLOCK_BITS - end_bit) - 1);
    bs->blocks[end_block] &= ~last_block_on;
    fix_end(bs);
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_bitset_flip(CCC_Bitset *const bs, size_t const i)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    Block_count const b_i = block_count_index(i);
    if (b_i >= bs->count)
    {
        return CCC_TRIBOOL_ERROR;
    }
    Bitblock *const block = &bs->blocks[b_i];
    CCC_Tribool const was = status(block, i);
    *block ^= on(i);
    fix_end(bs);
    return was;
}

CCC_Result
CCC_bitset_flip_all(CCC_Bitset *const bs)
{
    if (!bs)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!bs->count)
    {
        return CCC_RESULT_OK;
    }
    Block_count const end = block_count(bs->count);
    for (size_t i = 0; i < end; ++i)
    {
        bs->blocks[i] = ~bs->blocks[i];
    }
    fix_end(bs);
    return CCC_RESULT_OK;
}

/** Maybe future SIMD vectorization could speed things up here because we use
the same strat of handling first and last which just leaves a simpler bulk
operation in the middle. But we don't benefit from memset here. */
CCC_Result
CCC_bitset_flip_range(CCC_Bitset *const bs, size_t const i, size_t const count)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    size_t const end_i = i + count;
    Block_count start_block = block_count_index(i);
    Bit_count const start_bit = bit_count_index(i);
    Bitblock first_block_on = BITBLOCK_ON << start_bit;
    if (start_bit + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ON << (start_bit + count));
    }
    bs->blocks[start_block] ^= first_block_on;
    Block_count const end_block = block_count_index(end_i - 1);
    if (end_block == start_block)
    {
        fix_end(bs);
        return CCC_RESULT_OK;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        bs->blocks[start_block] = ~bs->blocks[start_block];
    }
    Bit_count const end_bit = bit_count_index(end_i - 1);
    Bitblock const last_block_on
        = BITBLOCK_ON >> ((BITBLOCK_BITS - end_bit) - 1);
    bs->blocks[end_block] ^= last_block_on;
    fix_end(bs);
    return CCC_RESULT_OK;
}

CCC_Count
CCC_bitset_capacity(CCC_Bitset const *const bs)
{
    if (!bs)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = bs->capacity};
}

CCC_Count
CCC_bitset_blocks_capacity(CCC_Bitset const *const bs)
{
    if (!bs)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = block_count(bs->capacity)};
}

CCC_Count
CCC_bitset_count(CCC_Bitset const *const bs)
{
    if (!bs)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = bs->count};
}

CCC_Count
CCC_bitset_blocks_count(CCC_Bitset const *const bs)
{
    if (!bs)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = block_count(bs->count)};
}

CCC_Tribool
CCC_bitset_empty(CCC_Bitset const *const bs)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !bs->count;
}

CCC_Count
CCC_bitset_popcount(CCC_Bitset const *const bs)
{
    if (!bs)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    if (!bs->count)
    {
        return (CCC_Count){.count = 0};
    }
    Block_count const end = block_count(bs->count);
    size_t cnt = 0;
    for (Block_count i = 0; i < end; cnt += popcount(bs->blocks[i++]))
    {}
    return (CCC_Count){.count = cnt};
}

CCC_Count
CCC_bitset_popcount_range(CCC_Bitset const *const bs, size_t const i,
                          size_t const count)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    size_t const end_i = i + count;
    size_t popped = 0;
    Block_count start_block = block_count_index(i);
    Bit_count const start_bit = bit_count_index(i);
    Bitblock first_block_on = BITBLOCK_ON << start_bit;
    if (start_bit + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ON << (start_bit + count));
    }
    popped += popcount(first_block_on & bs->blocks[start_block]);
    Block_count const end_block = block_count_index(end_i - 1);
    if (end_block == start_block)
    {
        return (CCC_Count){.count = popped};
    }
    for (++start_block; start_block < end_block;
         popped += popcount(bs->blocks[start_block++]))
    {}
    Bit_count const end_bit = bit_count_index(end_i - 1);
    Bitblock const last_block_on
        = BITBLOCK_ON >> ((BITBLOCK_BITS - end_bit) - 1);
    popped += popcount(last_block_on & bs->blocks[end_block]);
    return (CCC_Count){.count = popped};
}

CCC_Result
CCC_bitset_push_back(CCC_Bitset *const bs, CCC_Tribool const b)
{
    if (!bs || b > CCC_TRUE || b < CCC_FALSE)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    CCC_Result const check_resize = maybe_resize(bs, 1, bs->allocate);
    if (check_resize != CCC_RESULT_OK)
    {
        return check_resize;
    }
    ++bs->count;
    set(block_at(bs, bs->count - 1), bs->count - 1, b);
    fix_end(bs);
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_bitset_pop_back(CCC_Bitset *const bs)
{
    if (!bs || !bs->count)
    {
        return CCC_TRIBOOL_ERROR;
    }
    CCC_Tribool const was = status(block_at(bs, bs->count - 1), bs->count - 1);
    --bs->count;
    fix_end(bs);
    return was;
}

CCC_Tribool
CCC_bitset_any_range(CCC_Bitset const *const bs, size_t const i,
                     size_t const count)
{
    return any_or_none_range(bs, i, count, CCC_TRUE);
}

CCC_Tribool
CCC_bitset_any(CCC_Bitset const *const bs)
{
    return any_or_none_range(bs, 0, bs->count, CCC_TRUE);
}

CCC_Tribool
CCC_bitset_none_range(CCC_Bitset const *const bs, size_t const i,
                      size_t const count)
{
    return any_or_none_range(bs, i, count, CCC_FALSE);
}

CCC_Tribool
CCC_bitset_none(CCC_Bitset const *const bs)
{
    return any_or_none_range(bs, 0, bs->count, CCC_FALSE);
}

CCC_Tribool
CCC_bitset_all_range(CCC_Bitset const *const bs, size_t const i,
                     size_t const count)
{
    return all_range(bs, i, count);
}

CCC_Tribool
CCC_bitset_all(CCC_Bitset const *const bs)
{
    return all_range(bs, 0, bs->count);
}

CCC_Count
CCC_bitset_first_trailing_one_range(CCC_Bitset const *const bs, size_t const i,
                                    size_t const count)
{
    return first_trailing_one_range(bs, i, count);
}

CCC_Count
CCC_bitset_first_trailing_one(CCC_Bitset const *const bs)
{
    return first_trailing_one_range(bs, 0, bs->count);
}

CCC_Count
CCC_bitset_first_trailing_ones(CCC_Bitset const *const bs,
                               size_t const num_ones)
{
    return first_trailing_bits_range(bs, 0, bs->count, num_ones, CCC_TRUE);
}

CCC_Count
CCC_bitset_first_trailing_ones_range(CCC_Bitset const *const bs, size_t const i,
                                     size_t const count, size_t const num_ones)
{
    return first_trailing_bits_range(bs, i, count, num_ones, CCC_TRUE);
}

CCC_Count
CCC_bitset_first_trailing_zero_range(CCC_Bitset const *const bs, size_t const i,
                                     size_t const count)
{
    return first_trailing_zero_range(bs, i, count);
}

CCC_Count
CCC_bitset_first_trailing_zero(CCC_Bitset const *const bs)
{
    return first_trailing_zero_range(bs, 0, bs->count);
}

CCC_Count
CCC_bitset_first_trailing_zeros(CCC_Bitset const *const bs,
                                size_t const num_zeros)
{
    return first_trailing_bits_range(bs, 0, bs->count, num_zeros, CCC_FALSE);
}

CCC_Count
CCC_bitset_first_trailing_zeros_range(CCC_Bitset const *const bs,
                                      size_t const i, size_t const count,
                                      size_t const num_zeros)
{
    return first_trailing_bits_range(bs, i, count, num_zeros, CCC_FALSE);
}

CCC_Count
CCC_bitset_first_leading_one_range(CCC_Bitset const *const bs, size_t const i,
                                   size_t const count)
{
    return first_leading_one_range(bs, i, count);
}

CCC_Count
CCC_bitset_first_leading_one(CCC_Bitset const *const bs)
{
    return first_leading_one_range(bs, bs->count - 1, bs->count);
}

CCC_Count
CCC_bitset_first_leading_ones(CCC_Bitset const *const bs, size_t const num_ones)
{
    return first_leading_bits_range(bs, bs->count - 1, bs->count, num_ones,
                                    CCC_TRUE);
}

CCC_Count
CCC_bitset_first_leading_ones_range(CCC_Bitset const *const bs, size_t const i,
                                    size_t const count, size_t const num_ones)
{
    return first_leading_bits_range(bs, i, count, num_ones, CCC_TRUE);
}

CCC_Count
CCC_bitset_first_leading_zero_range(CCC_Bitset const *const bs, size_t const i,
                                    size_t const count)
{
    return first_leading_zero_range(bs, i, count);
}

CCC_Count
CCC_bitset_first_leading_zero(CCC_Bitset const *const bs)
{
    return first_leading_zero_range(bs, bs->count - 1, bs->count);
}

CCC_Count
CCC_bitset_first_leading_zeros(CCC_Bitset const *const bs,
                               size_t const num_zeros)
{
    return first_leading_bits_range(bs, bs->count - 1, bs->count, num_zeros,
                                    CCC_FALSE);
}

CCC_Count
CCC_bitset_first_leading_zeros_range(CCC_Bitset const *const bs, size_t const i,
                                     size_t const count, size_t const num_zeros)
{
    return first_leading_bits_range(bs, i, count, num_zeros, CCC_FALSE);
}

CCC_Result
CCC_bitset_clear(CCC_Bitset *const bs)
{
    if (!bs)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (bs->blocks)
    {
        assert(bs->capacity);
        (void)memset(bs->blocks, CCC_FALSE,
                     block_count(bs->capacity) * SIZEOF_BLOCK);
    }
    bs->count = 0;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_bitset_clear_and_free(CCC_Bitset *const bs)
{
    if (!bs)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!bs->allocate)
    {
        return CCC_RESULT_NO_ALLOCATION_FUNCTION;
    }
    if (bs->blocks)
    {
        (void)bs->allocate((CCC_Allocator_context){
            .input = bs->blocks,
            .bytes = 0,
            .context = bs->context,
        });
    }
    bs->count = 0;
    bs->capacity = 0;
    bs->blocks = NULL;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_bitset_clear_and_free_reserve(CCC_Bitset *const bs, CCC_Allocator *const fn)
{
    if (!bs || !fn)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (bs->blocks)
    {
        (void)fn((CCC_Allocator_context){
            .input = bs->blocks,
            .bytes = 0,
            .context = bs->context,
        });
    }
    bs->count = 0;
    bs->capacity = 0;
    bs->blocks = NULL;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_bitset_reserve(CCC_Bitset *const bs, size_t const to_add,
                   CCC_Allocator *const fn)
{
    if (!bs || !fn)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    return maybe_resize(bs, to_add, fn);
}

CCC_Result
CCC_bitset_copy(CCC_Bitset *const dst, CCC_Bitset const *const src,
                CCC_Allocator *const fn)
{
    if (!dst || !src || (dst->capacity < src->capacity && !fn))
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* Whatever future changes we make to bit set members should not fall out
       of sync with this code so save what we need to restore and then copy
       over everything else as a catch all. */
    Bitblock *const dst_mem = dst->blocks;
    size_t const dst_cap = dst->capacity;
    CCC_Allocator *const dst_allocate = dst->allocate;
    *dst = *src;
    dst->blocks = dst_mem;
    dst->capacity = dst_cap;
    dst->allocate = dst_allocate;
    if (!src->capacity)
    {
        return CCC_RESULT_OK;
    }
    if (dst->capacity < src->capacity)
    {
        Bitblock *const new_mem = fn((CCC_Allocator_context){
            .input = dst->blocks,
            .bytes = block_count(src->capacity) * SIZEOF_BLOCK,
            .context = dst->context,
        });
        if (!new_mem)
        {
            return CCC_RESULT_ALLOCATOR_ERROR;
        }
        dst->blocks = new_mem;
        dst->capacity = src->capacity;
    }
    if (!src->blocks || !dst->blocks)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    (void)memcpy(dst->blocks, src->blocks,
                 block_count(src->capacity) * SIZEOF_BLOCK);
    fix_end(dst);
    return CCC_RESULT_OK;
}

void *
CCC_bitset_data(CCC_Bitset const *const bs)
{
    if (!bs)
    {
        return NULL;
    }
    return bs->blocks;
}

CCC_Tribool
CCC_bitset_eq(CCC_Bitset const *const a, CCC_Bitset const *const b)
{
    if (!a || !b)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (a->count != b->count)
    {
        return CCC_FALSE;
    }
    return memcmp(a->blocks, b->blocks, block_count(a->count) * SIZEOF_BLOCK)
        == 0;
}

/*=========================     Private Interface   =========================*/

CCC_Result
CCC_private_bitset_reserve(struct CCC_Bitset *const bs, size_t const to_add,
                           CCC_Allocator *const fn)
{
    return CCC_bitset_reserve(bs, to_add, fn);
}

CCC_Tribool
CCC_private_bitset_set(struct CCC_Bitset *const bs, size_t const i,
                       CCC_Tribool const b)
{
    return CCC_bitset_set(bs, i, b);
}

/*=======================    Static Helpers    ==============================*/

/** Assumes set size is greater than or equal to subset size. */
static CCC_Tribool
is_subset_of(struct CCC_Bitset const *const set,
             struct CCC_Bitset const *const subset)
{
    assert(set->count >= subset->count);
    for (Block_count i = 0, end = block_count(subset->count); i < end; ++i)
    {
        /* Invariant: the last N unused bits in a set are zero so this works. */
        if ((set->blocks[i] & subset->blocks[i]) != subset->blocks[i])
        {
            return CCC_FALSE;
        }
    }
    return CCC_TRUE;
}

static CCC_Result
maybe_resize(struct CCC_Bitset *const bs, size_t const to_add,
             CCC_Allocator *const fn)
{
    size_t bits_needed = bs->count + to_add;
    if (bits_needed < bs->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (bits_needed <= bs->capacity)
    {
        return CCC_RESULT_OK;
    }
    if (!fn)
    {
        return CCC_RESULT_NO_ALLOCATION_FUNCTION;
    }
    if (!bs->count && to_add == 1)
    {
        bits_needed = BITBLOCK_BITS;
    }
    else if (to_add == 1)
    {
        bits_needed = bs->capacity * 2;
    }
    size_t const new_bytes
        = block_count(bits_needed - bs->count) * SIZEOF_BLOCK;
    size_t const old_bytes
        = bs->count ? block_count(bs->count) * SIZEOF_BLOCK : 0;
    Bitblock *const new_mem = fn((CCC_Allocator_context){
        .input = bs->blocks,
        .bytes = block_count(bits_needed) * SIZEOF_BLOCK,
        .context = bs->context,
    });
    if (!new_mem)
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    (void)memset((char *)new_mem + old_bytes, 0, new_bytes);
    bs->capacity = bits_needed;
    bs->blocks = new_mem;
    return CCC_RESULT_OK;
}

/** A trailing one in a range is the first bit set to one in any block within
range. The input i gives the starting bit of the search, meaning a bit within a
block that is the inclusive start of the range. The count gives us the end of
the search for an overall range of `[i, i + count)`. This means if the search
range is greater than a single block we will iterate in ascending order through
our blocks and from least to most significant bit within each block. */
static CCC_Count
first_trailing_one_range(struct CCC_Bitset const *const bs, size_t const i,
                         size_t const count)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    size_t const end_i = i + count;
    Block_count start_block = block_count_index(i);
    Bit_count const start_bit = bit_count_index(i);
    Bitblock first_block_on = BITBLOCK_ON << start_bit;
    if (start_bit + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ON << (start_bit + count));
    }
    Bit_count tz = ctz(first_block_on & bs->blocks[start_block]);
    if (tz != BITBLOCK_BITS)
    {
        return (CCC_Count){.count = (start_block * BITBLOCK_BITS) + tz};
    }
    Block_count const end_block = block_count_index(end_i - 1);
    if (end_block == start_block)
    {
        return (CCC_Count){.error = CCC_RESULT_FAIL};
    }
    /* Handle all values in between start and end in bulk. */
    for (++start_block; start_block < end_block; ++start_block)
    {
        tz = ctz(bs->blocks[start_block]);
        if (tz != BITBLOCK_BITS)
        {
            return (CCC_Count){
                .count = (start_block * BITBLOCK_BITS) + tz,
            };
        }
    }
    /* Handle last block. */
    Bit_count const end_bit = bit_count_index(end_i - 1);
    Bitblock const last_block_on
        = BITBLOCK_ON >> ((BITBLOCK_BITS - end_bit) - 1);
    tz = ctz(last_block_on & bs->blocks[end_block]);
    if (tz != BITBLOCK_BITS)
    {
        return (CCC_Count){.count = (end_block * BITBLOCK_BITS) + tz};
    }
    return (CCC_Count){.error = CCC_RESULT_FAIL};
}

/** Finds the starting index of a sequence of 1's or 0's of the num_bits size in
linear time. The algorithm aims to efficiently skip as many bits as possible
while searching for the desired group. This avoids both an O(N^2) runtime and
the use of any unnecessary modulo or division operations in a hot loop. */
static CCC_Count
first_trailing_bits_range(struct CCC_Bitset const *const bs, size_t const i,
                          size_t const count, size_t const num_bits,
                          CCC_Tribool const is_one)
{
    if (!bs || i >= bs->count || num_bits > count || i + count < i
        || i + count > bs->count)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    size_t const range_end = i + count;
    size_t num_found = 0;
    size_t bits_start = i;
    Block_count cur_block = block_count_index(i);
    size_t cur_end = (cur_block * BITBLOCK_BITS) + BITBLOCK_BITS;
    Bit_count bit_i = bit_count_index(i);
    do
    {
        /* Clean up some edge cases for the helper function because we allow
           the user to specify any range. What if our range ends before the
           end of this block? What if it starts after index 0 of the first
           block? Pretend out of range bits are zeros. */
        Bitblock bits = is_one
                          ? bs->blocks[cur_block] & (BITBLOCK_ON << bit_i)
                          : (~bs->blocks[cur_block]) & (BITBLOCK_ON << bit_i);
        if (cur_end > range_end)
        {
            bits &= ~(BITBLOCK_ON << bit_count_index(range_end));
        }
        struct Group_count const ones
            = max_trailing_ones(bits, bit_i, num_bits - num_found);
        if (ones.count >= num_bits)
        {
            /* Found the solution all at once within a block. */
            return (CCC_Count){
                .count = (cur_block * BITBLOCK_BITS) + ones.i,
            };
        }
        if (!ones.i)
        {
            if (num_found + ones.count >= num_bits)
            {
                /* Found solution crossing block boundary from prefix blocks. */
                return (CCC_Count){.count = bits_start};
            }
            /* Found a full block so keep on trucking. */
            num_found += ones.count;
        }
        else
        {
            /* Fail but we have largest skip possible to continue our search
               from in order to save double checking unnecessary prefixes. */
            bits_start = (cur_block * BITBLOCK_BITS) + ones.i;
            num_found = ones.count;
        }
        bit_i = 0;
        ++cur_block;
        cur_end += BITBLOCK_BITS;
    }
    while (bits_start + num_bits <= range_end);
    return (CCC_Count){.error = CCC_RESULT_FAIL};
}

/** Returns the maximum group of consecutive ones in the bit block given. If the
number of consecutive ones remaining cannot be found the function returns
where the next search should start from, a critical step to a linear search;
specifically, we seek any group of continuous ones that runs from some index
in the block to the end of the block.

If no continuous group of ones exist that runs to the end of the block, the
BLOCK_BITS index is returned with a group size of 0 meaning the search for ones
will need to continue in the next block. This is helpful for the main search
loop adding to its start index and number of ones found so far. */
static struct Group_count
max_trailing_ones(Bitblock const b, Bit_count const i_bit,
                  size_t const ones_remain)
{
    /* Easy exit skip to the next block. Helps with sparse sets. */
    if (!b)
    {
        return (struct Group_count){.i = BITBLOCK_BITS};
    }
    if (ones_remain <= BITBLOCK_BITS)
    {
        assert(i_bit < BITBLOCK_BITS);
        /* This branch must find a smaller group anywhere in this block which is
           the most work required in this algorithm. We have some tricks to tell
           when to give up on this as soon as possible. */
        Bitblock b_check = b >> i_bit;
        Bitblock const remain = BITBLOCK_ON >> (BITBLOCK_BITS - ones_remain);
        /* Because of power of 2 rules we can stop early when the shifted
           becomes impossible to match. */
        for (Bit_count shifts = 0; b_check >= remain; b_check >>= 1, ++shifts)
        {
            if ((remain & b_check) == remain)
            {
                return (struct Group_count){
                    .i = i_bit + shifts,
                    .count = ones_remain,
                };
            }
        }
    }
    /* 2 cases covered: the ones remaining are greater than this block could
       hold or we did not find a match by the masking we just did. In either
       case we need the maximum contiguous ones that run all the way to the
       MSB. The best we could have is a full block of 1's. Otherwise we need
       to find where to start our new search for contiguous 1's. This could be
       the next block if there are not 1's that continue all the way to MSB. */
    Bit_count const leading_ones = clz(~b);
    return (struct Group_count){
        .i = BITBLOCK_BITS - leading_ones,
        .count = leading_ones,
    };
}

/** A trailing zero in a range is the first bit set to zero in any block within
range. The input i gives the starting bit of the search, meaning a bit within a
block that is the inclusive start of the range. The count gives us the end of
the search for an overall range of `[i, i + count)`. This means if the search
range is greater than a single block we will iterate in ascending order through
our blocks and from least to most significant bit within each block. */
static CCC_Count
first_trailing_zero_range(struct CCC_Bitset const *const bs, size_t const i,
                          size_t const count)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    size_t const end_i = i + count;
    Block_count start_block = block_count_index(i);
    Bit_count const start_bit = bit_count_index(i);
    Bitblock first_block_on = BITBLOCK_ON << start_bit;
    if (start_bit + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ON << (start_bit + count));
    }
    Bit_count tz = ctz(first_block_on & ~bs->blocks[start_block]);
    if (tz != BITBLOCK_BITS)
    {
        return (CCC_Count){
            .count = (start_block * BITBLOCK_BITS) + tz,
        };
    }
    Block_count const end_block = block_count_index(end_i - 1);
    if (start_block == end_block)
    {
        return (CCC_Count){.error = CCC_RESULT_FAIL};
    }
    /* Handle all values in between start and end in bulk. */
    for (++start_block; start_block < end_block; ++start_block)
    {
        tz = ctz(~bs->blocks[start_block]);
        if (tz != BITBLOCK_BITS)
        {
            return (CCC_Count){
                .count = (start_block * BITBLOCK_BITS) + tz,
            };
        }
    }
    /* Handle last block. */
    Bit_count const end_bit = bit_count_index(end_i - 1);
    Bitblock const last_block_on
        = BITBLOCK_ON >> ((BITBLOCK_BITS - end_bit) - 1);
    tz = ctz(last_block_on & ~bs->blocks[end_block]);
    if (tz != BITBLOCK_BITS)
    {
        return (CCC_Count){.count = (end_block * BITBLOCK_BITS) + tz};
    }
    return (CCC_Count){.error = CCC_RESULT_FAIL};
}

/** A leading one is the first bit in the range to be set to one withing a block
starting the search from the Most Significant Bit of each block. This means that
if the range is larger than a single block we iterate in descending order
through the set of blocks starting at i for the range of `[i, i - count)`. The
search within a given block proceeds from Most Significant Bit toward Least
Significant Bit. */
static CCC_Count
first_leading_one_range(struct CCC_Bitset const *const bs, size_t const i,
                        size_t const count)
{
    if (!bs || i >= bs->count || count > bs->count || i + 1 < count)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    size_t const end_i = (i + 1 - count);
    Bit_count const start_bit = bit_count_index(i);
    Bit_count const end_bit = bit_count_index(end_i);
    Block_count start_block = block_count_index(i);
    Bitblock first_block_on = BITBLOCK_ON >> ((BITBLOCK_BITS - start_bit) - 1);
    if (i - end_i < BITBLOCK_BITS)
    {
        first_block_on &= (BITBLOCK_ON << end_bit);
    }
    Bit_count lz = clz(first_block_on & bs->blocks[start_block]);
    if (lz != BITBLOCK_BITS)
    {
        return (CCC_Count){
            .count = (start_block * BITBLOCK_BITS) + (BITBLOCK_BITS - lz - 1),
        };
    }
    Block_count const end_block = block_count_index(end_i);
    if (end_block == start_block)
    {
        return (CCC_Count){.error = CCC_RESULT_FAIL};
    }
    /* Handle all values in between start and end in bulk. */
    for (--start_block; start_block > end_block; --start_block)
    {
        lz = clz(bs->blocks[start_block]);
        if (lz != BITBLOCK_BITS)
        {
            return (CCC_Count){
                .count
                = (start_block * BITBLOCK_BITS) + (BITBLOCK_BITS - lz - 1),
            };
        }
    }
    /* Handle last block. */
    Bitblock const last_block_on
        = ~(BITBLOCK_ON >> ((BITBLOCK_BITS - end_bit) - 1));
    lz = clz(last_block_on & bs->blocks[end_block]);
    if (lz != BITBLOCK_BITS)
    {
        return (CCC_Count){
            .count = (end_block * BITBLOCK_BITS) + (BITBLOCK_BITS - lz - 1),
        };
    }
    return (CCC_Count){.error = CCC_RESULT_FAIL};
}

/* Overall I am not thrilled with the need for handling signed values and back
wards iteration in this version. However, I am having trouble finding a clean
way to do this unsigned. Signed simplifies the iteration and interaction with
the helper function finding leading ones because the algorithm is complex
enough as is. Candidate for refactor. */
static CCC_Count
first_leading_bits_range(struct CCC_Bitset const *const bs, size_t const i,
                         size_t const count, size_t const num_bits,
                         CCC_Tribool const is_one)
{
    /* The only risk is that i is out of range of `ptrdiff_t` which would mean
       that we cannot proceed with algorithm. However, this is unlikely on
       most platforms as they often bound object size by the max pointer
       difference possible, which this type provides. However, it is not
       required by the C standard so we are obligated to check. */
    ptrdiff_t const range_end = (ptrdiff_t)(i - count);
    if (!bs || num_bits > PTRDIFF_MAX || i > PTRDIFF_MAX || i >= bs->count
        || !num_bits || num_bits > count || range_end < -1)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    size_t num_found = 0;
    ptrdiff_t bits_start = (ptrdiff_t)i;
    /* If we passed the earlier signed range check this cast is safe because
       (i / block bits) for some block index must be less than i. */
    Block_signed_count cur_block = (Block_signed_count)block_count_index(i);
    ptrdiff_t cur_end = (ptrdiff_t)((cur_block * BITBLOCK_BITS) - 1);
    Bit_signed_count i_bit = bit_count_index(i);
    do
    {
        Bitblock bits
            = is_one ? bs->blocks[cur_block]
                           & (BITBLOCK_ON >> ((BITBLOCK_BITS - i_bit) - 1))
                     : ~bs->blocks[cur_block]
                           & (BITBLOCK_ON >> ((BITBLOCK_BITS - i_bit) - 1));
        if (cur_end < range_end)
        {
            bits &= (BITBLOCK_ON << bit_count_index(range_end + 1));
        }
        struct Group_signed_count const ones
            = max_leading_ones(bits, i_bit, num_bits - num_found);
        if (ones.count >= num_bits)
        {
            return (CCC_Count){
                .count = (cur_block * BITBLOCK_BITS) + ones.i,
            };
        }
        if (ones.i == BITBLOCK_BITS - 1)
        {
            if (num_found + ones.count >= num_bits)
            {
                return (CCC_Count){.count = bits_start};
            }
            num_found += ones.count;
        }
        else
        {
            /* If the new block start index is -1, then this addition bumps us
               to the next block's Most Significant Bit and is a simple
               subtraction by one to start search on next block.*/
            bits_start = (ptrdiff_t)((cur_block * BITBLOCK_BITS) + ones.i);
            num_found = ones.count;
        }
        i_bit = BITBLOCK_BITS - 1;
        --cur_block;
        cur_end -= BITBLOCK_BITS;
    }
    while (bits_start >= range_end + (ptrdiff_t)num_bits);
    return (CCC_Count){.error = CCC_RESULT_FAIL};
}

/** Returns the maximum group of consecutive ones in the bit block given. If the
number of consecutive ones remaining cannot be found the function returns
where the next search should start from, a critical step to a linear search;
specifically, we seek any group of continuous ones that runs from some index
in the block to the start of the block (0th index).

If no continuous group of ones exist that runs to the start of the block, a -1
index is returned with a group size of 0 meaning the search for ones will need
to continue in the next block lower block. This is helpful for the main search
loop adding to its start index and number of ones found so far. Adding -1 is
just subtraction so this will correctly drop us to the top bit of the next Least
Significant Block to continue the search. */
static struct Group_signed_count
max_leading_ones(Bitblock const b, Bit_signed_count const i_bit,
                 size_t const ones_remaining)
{
    if (!b)
    {
        return (struct Group_signed_count){.i = -1};
    }
    if (ones_remaining <= BITBLOCK_BITS)
    {
        assert(i_bit < BITBLOCK_BITS);
        Bitblock b_check = b << (BITBLOCK_BITS - i_bit - 1);
        Bitblock const required = BITBLOCK_ON
                               << (BITBLOCK_BITS - ones_remaining);
        for (Bit_signed_count shifts = 0; b_check; b_check <<= 1, ++shifts)
        {
            if ((required & b_check) == required)
            {
                return (struct Group_signed_count){
                    .i = (Bit_signed_count)(i_bit - shifts),
                    .count = ones_remaining,
                };
            }
        }
    }
    Bit_signed_count const trailing_ones = (Bit_signed_count)ctz(~b);
    return (struct Group_signed_count){
        /* May be -1 if no ones found. This make backward iteration easier. */
        .i = (Bit_signed_count)(trailing_ones - 1),
        .count = trailing_ones,
    };
}

/** A leading zero is the first bit in the range to be set to zero withing a
block starting the search from the Most Significant Bit of each block. This
means that if the range is larger than a single block we iterate in descending
order through the set of blocks starting at i for the range of `[i, i - count)`.
The search within a given block proceeds from Most Significant Bit toward Least
Significant Bit. */
static CCC_Count
first_leading_zero_range(struct CCC_Bitset const *const bs, size_t const i,
                         size_t const count)
{
    if (!bs || i >= bs->count || count > bs->count || i + 1 < count)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    size_t const end_i = i + 1 - count;
    Block_count start_block = block_count_index(i);
    Bit_count const end_bit = bit_count_index(end_i);
    Bit_count const start_bit = bit_count_index(i);
    Bitblock first_block_on = BITBLOCK_ON >> ((BITBLOCK_BITS - start_bit) - 1);
    if (i - end_i < BITBLOCK_BITS)
    {
        first_block_on &= (BITBLOCK_ON << end_bit);
    }
    Bit_count lz = clz(first_block_on & ~bs->blocks[start_block]);
    if (lz != BITBLOCK_BITS)
    {
        return (CCC_Count){
            .count = ((start_block * BITBLOCK_BITS) + (BITBLOCK_BITS - lz - 1)),
        };
    }
    Block_count const end_block = block_count_index(end_i);
    if (end_block == start_block)
    {
        return (CCC_Count){.error = CCC_RESULT_FAIL};
    }
    /* Handle all values in between start and end in bulk. */
    for (--start_block; start_block > end_block; --start_block)
    {
        lz = clz(~bs->blocks[start_block]);
        if (lz != BITBLOCK_BITS)
        {
            return (CCC_Count){
                .count
                = ((start_block * BITBLOCK_BITS) + (BITBLOCK_BITS - lz - 1)),
            };
        }
    }
    /* Handle last block. */
    Bitblock const last_block_on
        = ~(BITBLOCK_ON >> ((BITBLOCK_BITS - end_bit) - 1));
    lz = clz(last_block_on & ~bs->blocks[end_block]);
    if (lz != BITBLOCK_BITS)
    {
        return (CCC_Count){
            .count = ((end_block * BITBLOCK_BITS) + (BITBLOCK_BITS - lz - 1)),
        };
    }
    return (CCC_Count){.error = CCC_RESULT_FAIL};
}

/** Performs the any or none scan operation over the specified range. The only
difference between the operations is the return value. Specify the desired
Tribool value to return upon encountering an on bit. For any this is
CCC_TRUE. For none this is CCC_FALSE. Saves writing two identical fns. */
static CCC_Tribool
any_or_none_range(struct CCC_Bitset const *const bs, size_t const i,
                  size_t const count, CCC_Tribool const ret)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count
        || ret < CCC_FALSE || ret > CCC_TRUE)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const end_i = i + count;
    Block_count start_block = block_count_index(i);
    Bit_count const start_bit = bit_count_index(i);
    Bitblock first_block_on = BITBLOCK_ON << start_bit;
    if (start_bit + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ON << (start_bit + count));
    }
    if (first_block_on & bs->blocks[start_block])
    {
        return ret;
    }
    Block_count const end_block = block_count_index(end_i - 1);
    if (end_block == start_block)
    {
        return !ret;
    }
    /* If this is the any check we might get lucky by checking the last block
       before looping over everything. */
    Bit_count const end_bit = bit_count_index(end_i - 1);
    Bitblock const last_block_on
        = BITBLOCK_ON >> ((BITBLOCK_BITS - end_bit) - 1);
    if (last_block_on & bs->blocks[end_block])
    {
        return ret;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        if (bs->blocks[start_block] & BITBLOCK_ON)
        {
            return ret;
        }
    }
    return !ret;
}

/** Check for all on is slightly different from the any or none checks so we
need a painfully repetitive function. */
static CCC_Tribool
all_range(struct CCC_Bitset const *const bs, size_t const i, size_t const count)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const end = i + count;
    Block_count start_block = block_count_index(i);
    Bit_count const start_bit = bit_count_index(i);
    Bitblock first_block_on = BITBLOCK_ON << start_bit;
    if (start_bit + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ON << (start_bit + count));
    }
    if ((first_block_on & bs->blocks[start_block]) != first_block_on)
    {
        return CCC_FALSE;
    }
    Block_count const end_block = block_count_index(end - 1);
    if (end_block == start_block)
    {
        return CCC_TRUE;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        if (bs->blocks[start_block] != BITBLOCK_ON)
        {
            return CCC_FALSE;
        }
    }
    Bit_count const end_bit = bit_count_index(end - 1);
    Bitblock const last_block_on
        = BITBLOCK_ON >> ((BITBLOCK_BITS - end_bit) - 1);
    if ((last_block_on & bs->blocks[end_block]) != last_block_on)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

/** Given the 0 based index from `[0, count of used bits in set)` returns a
reference to the block that such a bit belongs to. This block reference will
point to some block at index [0, count of blocks used in the set). */
static inline Bitblock *
block_at(struct CCC_Bitset const *const bs, size_t const bitset_index)
{
    return &bs->blocks[block_count_index(bitset_index)];
}

/** Sets all bits in bulk to value b and fixes the end block to ensure all bits
in the final block that are not in use are zeroed out. */
static inline void
set_all(struct CCC_Bitset *const bs, CCC_Tribool const b)
{
    (void)memset(bs->blocks, b ? ~0 : 0, block_count(bs->count) * SIZEOF_BLOCK);
    fix_end(bs);
}

/** Given the appropriate block in which bit_i resides, sets the bits position
to 0 or 1 as specified by the CCC_Tribool argument b.

Assumes block has been retrieved correctly in range [0, count of blocks in set)
and that bit_i is in range [0, count of active bits in set). */
static inline void
set(Bitblock *const block, size_t const bitset_index, CCC_Tribool const b)
{
    if (b)
    {
        *block |= on(bitset_index);
    }
    else
    {
        *block &= ~on(bitset_index);
    }
}

/** Given the bit set and the set index--set index is allowed to be greater than
the size of one block--returns the status of the bit at that index. */
static inline CCC_Tribool
status(Bitblock const *const bs, size_t const bitset_index)
{
    /* Be careful. The & op does not promise to evaluate to 1 or 0. We often
       just use it where that conversion takes place implicitly for us. */
    return (*bs & on(bitset_index)) != 0;
}

/** Given the true bit index in the bit set, expected to be in the range
[0, count of active bits in set), returns a Bitblock mask with only this bit
on in block to which it belongs. This mask guarantees to have a bit on within
a bit block at index [0, BITBLOCK_BITS - 1). */
static inline Bitblock
on(size_t bitset_index)
{
    return (Bitblock)1 << bit_count_index(bitset_index);
}

/** Clears unused bits in the last block according to count. Sets the last block
to have only the used bits set to their given values and all bits after to zero.
This is used as a safety mechanism throughout the code after complex operations
on bit blocks to ensure any side effects on unused bits are deleted. */
static inline void
fix_end(struct CCC_Bitset *const bs)
{
    /* Remember, we fill from LSB to MSB so we want the mask to start at lower
       order bit which is why we do the second funky flip on the whole op. */
    bs->count ? (*block_at(bs, bs->count - 1)
                 &= ~(((Bitblock)~1) << bit_count_index(bs->count - 1)))
              : (bs->blocks[0] = 0);
}

/** Returns the 0-based index of the block in the block array allocation to
which the given index belongs. Assumes the given index is somewhere between [0,
count of bits set). The returned index then represents the block in which this
index resides which is in the range [0, block containing last in use bit). */
static inline Block_count
block_count_index(size_t const bitset_index)
{
    return bitset_index / BITBLOCK_BITS;
}

/** Returns the 0-based index within a block to which the given index belongs.
This index will always be between [0, BITBLOCK_BITS - 1). */
static inline Bit_count
bit_count_index(size_t const bitset_index)
{
    static_assert((BITBLOCK_BITS & (BITBLOCK_BITS - 1)) == 0,
                  "the number of bits in a block is always a power of two, "
                  "avoiding modulo operations when possible.");
    return bitset_index & (BITBLOCK_BITS - 1);
}

/** Returns the number of blocks required to store the given bits. Assumes bits
is non-zero. For any bits > 1 the block count is always less than bits.*/
static inline Block_count
block_count(size_t const set_bits)
{
    static_assert(BITBLOCK_BITS);
    assert(set_bits);
    return (set_bits + (BITBLOCK_BITS - 1)) / BITBLOCK_BITS;
}

/** Returns min of size_t arguments. Beware of conversions. */
static inline size_t
size_t_min(size_t const a, size_t const b)
{
    return a < b ? a : b;
}

/** The following asserts assure that whether portable or built in bit
operations are used in the coming section we are safe in our assumptions about
widths and counts. */
static_assert(BITBLOCK_MSB < BITBLOCK_ON);
static_assert(SIZEOF_BLOCK == sizeof(unsigned));

/** Built-ins are common on Clang and GCC but we have portable fallback. */
#if defined(__has_builtin) && __has_builtin(__builtin_ctz)                     \
    && __has_builtin(__builtin_clz) && __has_builtin(__builtin_popcount)

/** Counts the on bits in a bit block. */
static inline Bit_count
popcount(Bitblock const b)
{
    /* There are different pop counts for different integer widths. Be sure to
       catch the use of the wrong one by mistake here at compile time. */
    static_assert(__builtin_popcount((Bitblock)~0) <= U8BLOCK_MAX);
    return (Bit_count)__builtin_popcount(b);
}

/** Counts the number of trailing zeros in a bit block starting from least
significant bit. */
static inline Bit_count
ctz(Bitblock const b)
{
    static_assert(__builtin_ctz(BITBLOCK_MSB) <= U8BLOCK_MAX);
    return b ? (Bit_count)__builtin_ctz(b) : BITBLOCK_BITS;
}

/** Counts the leading zeros in a bit block starting from the most significant
bit. */
static inline Bit_count
clz(Bitblock const b)
{
    static_assert(__builtin_clz((Bitblock)1) <= U8BLOCK_MAX);
    return b ? (Bit_count)__builtin_clz(b) : BITBLOCK_BITS;
}

#else /* !defined(__has_builtin) || !__has_builtin(__builtin_ctz)              \
    || !__has_builtin(__builtin_clz) || !__has_builtin(__builtin_popcount) */

/** Counts the on bits in a bit block. */
static inline Bit_count
popcount(CCC_Bitblock b)
{
    Bit_count cnt = 0;
    for (; b; cnt += ((b & 1U) != 0), b >>= 1U)
    {}
    return cnt;
}

/** Counts the number of trailing zeros in a bit block starting from least
significant bit. */
static inline Bit_count
ctz(CCC_Bitblock b)
{
    if (!b)
    {
        return BLOCK_BITS;
    }
    Bit_count cnt = 0;
    for (; (b & 1U) == 0; ++cnt, b >>= 1U)
    {}
    return cnt;
}

/** Counts the leading zeros in a bit block starting from the most significant
bit. */
static inline Bit_count
clz(CCC_Bitblock b)
{
    if (!b)
    {
        return BLOCK_BITS;
    }
    Bit_count cnt = 0;
    for (; (b & BITBLOCK_MSB) == 0; ++cnt, b <<= 1U)
    {}
    return cnt;
}

#endif /* defined(__has_builtin) && __has_builtin(__builtin_ctz)               \
    && __has_builtin(__builtin_clz) && __has_builtin(__builtin_popcount) */
