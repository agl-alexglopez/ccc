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
#include "impl/impl_bitset.h"
#include "types.h"

/*=========================   Types/Prototypes   ============================*/

/** @private Used frequently so call the builtin just once. */
enum : size_t
{
    /** @private Bytes of a bit block to help with byte calculations. */
    SIZEOF_BLOCK = sizeof(ccc_bitblock),
};

/** @private Various constants to support bit block size bit ops. */
enum : ccc_bitblock
{
    /** @private How many total bits that fit in a bit block. */
    BITBLOCK_BITS = (SIZEOF_BLOCK * CHAR_BIT),
    /** @private A mask of a bit block with all bits on. */
    BITBLOCK_ALL_ON = ((ccc_bitblock)~0),
    /** @private The Most Significant Bit of a bit block turned on to 1. */
    BITBLOCK_MSB = (((ccc_bitblock)1) << (((SIZEOF_BLOCK * CHAR_BIT)) - 1)),
};

/** @private An index within a block. A block is bounded to some number of bits
as determined by the type used for each block. This type is intended to count
bits in a block and therefore cannot count up to arbitrary indices. Assume its
range is `[0, BITBLOCK_BITS]`, for ease of use and clarity.

There are many types of indexing and counting that take place in a bit set so
use this type specifically for counting bits in a block so the reader is clear
on intent. */
typedef uint8_t ublockwidth;
static_assert((ublockwidth)~0 >= 0, "blockwidth_t must be unsigned");
static_assert((ublockwidth)~0 >= BITBLOCK_BITS,
              "blockwidth_t must be able to count all bits in a block.");

/** @private A helper to allow for an efficient linear scan for groups of 0's
or 1's in the set. */
struct ugroup
{
    /** A index [0, bit block bits) indicating the status of a search. */
    ublockwidth block_start_i;
    /** The number of bits of same value found starting at block_start_i. */
    size_t count;
};

/** @private A signed helper to allow for an efficient linear scan for groups of
0's or 1's in the set. Created to support -1 index return values for cleaner
group scanning in reverse. */
struct igroup
{
    /** A index [-1, bit block bits - 1) indicating the status of a search. */
    ptrdiff_t block_start_i;
    /** The number of bits of same value found starting at block_start_i. */
    size_t count;
};

static size_t block_i(size_t bit_i);
static inline ccc_bitblock *block_at(ccc_bitset const *bs, size_t bit_i);
static void set(ccc_bitblock *, size_t bit_i, ccc_tribool);
static ccc_bitblock on(size_t bit_i);
static ccc_bitblock last_on(struct ccc_bitset const *);
static void fix_end(struct ccc_bitset *);
static ccc_tribool status(ccc_bitblock const *, size_t bit_i);
static size_t block_count(size_t bits);
static unsigned popcount(ccc_bitblock);
static ccc_tribool any_or_none_range(struct ccc_bitset const *, size_t i,
                                     size_t count, ccc_tribool);
static ccc_tribool all_range(struct ccc_bitset const *bs, size_t i,
                             size_t count);
static ccc_ucount first_trailing_one_range(struct ccc_bitset const *bs,
                                           size_t i, size_t count);
static ccc_ucount first_trailing_zero_range(struct ccc_bitset const *bs,
                                            size_t i, size_t count);
static ccc_ucount first_leading_one_range(struct ccc_bitset const *bs, size_t i,
                                          size_t count);
static ccc_ucount first_leading_zero_range(struct ccc_bitset const *bs,
                                           size_t i, size_t count);
static ccc_ucount first_trailing_bits_range(struct ccc_bitset const *bs,
                                            size_t i, size_t count,
                                            size_t num_bits,
                                            ccc_tribool is_one);
static ccc_ucount first_leading_bits_range(struct ccc_bitset const *bs,
                                           size_t i, size_t count,
                                           size_t num_bits, ccc_tribool is_one);
static struct ugroup max_trailing_ones(ccc_bitblock b, ublockwidth i_in_block,
                                       size_t num_ones_remaining);
static struct igroup max_leading_ones(ccc_bitblock b, ublockwidth i_in_block,
                                      size_t num_ones_remaining);
static ccc_result maybe_resize(struct ccc_bitset *bs, size_t to_add,
                               ccc_any_alloc_fn *);
static size_t size_t_min(size_t, size_t);
static void set_all(struct ccc_bitset *bs, ccc_tribool b);
static ublockwidth blockwidth_i(size_t bit_i);
static ccc_tribool is_subset_of(struct ccc_bitset const *set,
                                struct ccc_bitset const *subset);
static unsigned ctz(ccc_bitblock);
static unsigned clz(ccc_bitblock);

/*=======================   Public Interface   ==============================*/

ccc_tribool
ccc_bs_is_proper_subset(ccc_bitset const *const set,
                        ccc_bitset const *const subset)
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

ccc_tribool
ccc_bs_is_subset(ccc_bitset const *const set, ccc_bitset const *const subset)
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

ccc_result
ccc_bs_or(ccc_bitset *const dst, ccc_bitset const *const src)
{
    if (!dst || !src)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!dst->count || !src->count)
    {
        return CCC_RESULT_OK;
    }
    size_t const end_block = block_count(size_t_min(dst->count, src->count));
    for (size_t b = 0; b < end_block; ++b)
    {
        dst->blocks[b] |= src->blocks[b];
    }
    fix_end(dst);
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_xor(ccc_bitset *const dst, ccc_bitset const *const src)
{
    if (!dst || !src)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!dst->count || !src->count)
    {
        return CCC_RESULT_OK;
    }
    size_t const end_block = block_count(size_t_min(dst->count, src->count));
    for (size_t b = 0; b < end_block; ++b)
    {
        dst->blocks[b] ^= src->blocks[b];
    }
    fix_end(dst);
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_and(ccc_bitset *dst, ccc_bitset const *src)
{
    if (!dst || !src)
    {
        return CCC_RESULT_ARG_ERROR;
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
    size_t const end_block = block_count(size_t_min(dst->count, src->count));
    for (size_t b = 0; b < end_block; ++b)
    {
        dst->blocks[b] &= src->blocks[b];
    }
    if (dst->count <= src->count)
    {
        return CCC_RESULT_OK;
    }
    /* The src widens to align with dst as integers would; same consequences. */
    size_t const dst_blocks = block_count(dst->count);
    size_t const remain = dst_blocks - end_block;
    (void)memset(dst->blocks + end_block, CCC_FALSE, remain * SIZEOF_BLOCK);
    fix_end(dst);
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_shiftl(ccc_bitset *const bs, size_t const left_shifts)
{
    if (!bs)
    {
        return CCC_RESULT_ARG_ERROR;
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
    size_t const last_block = block_i(bs->count - 1);
    size_t const shifted_blocks = block_i(left_shifts);
    ublockwidth const partial_shift = blockwidth_i(left_shifts);
    if (!partial_shift)
    {
        for (size_t shifted = last_block - shifted_blocks + 1,
                    overwritten = last_block;
             shifted--; --overwritten)
        {
            bs->blocks[overwritten] = bs->blocks[shifted];
        }
    }
    else
    {
        ublockwidth const remaining_shift = BITBLOCK_BITS - partial_shift;
        for (size_t shifted = last_block - shifted_blocks,
                    overwritten = last_block;
             shifted > 0; --shifted, --overwritten)
        {
            bs->blocks[overwritten]
                = (bs->blocks[shifted] << partial_shift)
                  | (bs->blocks[shifted - 1] >> remaining_shift);
        }
        bs->blocks[shifted_blocks] = bs->blocks[0] << partial_shift;
    }
    /* Zero fills in lower bits just as an integer shift would. */
    for (size_t i = 0; i < shifted_blocks; ++i)
    {
        bs->blocks[i] = 0;
    }
    fix_end(bs);
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_shiftr(ccc_bitset *const bs, size_t const right_shifts)
{
    if (!bs)
    {
        return CCC_RESULT_ARG_ERROR;
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
    size_t const last_block = block_i(bs->count - 1);
    size_t const shifted_blocks = block_i(right_shifts);
    ublockwidth partial_shift = blockwidth_i(right_shifts);
    if (!partial_shift)
    {
        for (size_t shifted = shifted_blocks, overwritten = 0;
             shifted < last_block + 1; ++shifted, ++overwritten)
        {
            bs->blocks[overwritten] = bs->blocks[shifted];
        }
    }
    else
    {
        ublockwidth remaining_shift = BITBLOCK_BITS - partial_shift;
        for (size_t shifted = shifted_blocks, overwritten = 0;
             shifted < last_block; ++shifted, ++overwritten)
        {
            bs->blocks[overwritten]
                = (bs->blocks[shifted + 1] << remaining_shift)
                  | (bs->blocks[shifted] >> partial_shift);
        }
        bs->blocks[last_block - shifted_blocks]
            = bs->blocks[last_block] >> partial_shift;
    }
    /* This is safe for a few reasons:
       - If shifts equals count we set all to 0 and returned early.
       - If we only have one block i will be equal to end and we are done.
       - If end is the 0th block we will stop after 1. A meaningful shift
         occurred in the 0th block so zeroing would be a mistake.
       - All other cases ensure it is safe to decrease i (no underflow).
       This operation emulates the zeroing of high bits on a right shift and
       a bit sign is considered unsigned so we don't sign bit fill. */
    for (size_t i = last_block, end = last_block - shifted_blocks; i > end; --i)
    {
        bs->blocks[i] = 0;
    }
    fix_end(bs);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_bs_test(ccc_bitset const *const bs, size_t const i)
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

ccc_tribool
ccc_bs_set(ccc_bitset *const bs, size_t const i, ccc_tribool const b)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (i >= bs->count)
    {
        return CCC_TRIBOOL_ERROR;
    }
    ccc_bitblock *const block = block_at(bs, i);
    ccc_tribool const was = status(block, i);
    set(block, i, b);
    return was;
}

ccc_result
ccc_bs_set_all(ccc_bitset *const bs, ccc_tribool const b)
{
    if (!bs)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (bs->count)
    {
        set_all(bs, b);
    }
    return CCC_RESULT_OK;
}

/** A naive implementation might just call set for every index between the
start and start + count. However, calculating the block and index within each
block for every call to set costs a division and a modulo operation. We can
avoid this by handling the first and last block and then handling everything in
between with a bulk memset. */
ccc_result
ccc_bs_set_range(ccc_bitset *const bs, size_t const i, size_t const count,
                 ccc_tribool const b)
{
    if (!bs || i >= bs->count || i + count < i)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    size_t const end = i + count;
    size_t start_block = block_i(i);
    ublockwidth const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    if (b)
    {
        bs->blocks[start_block] |= first_block_on;
    }
    else
    {
        bs->blocks[start_block] &= ~first_block_on;
    }
    size_t const end_block = block_i(end - 1);
    if (end_block == start_block)
    {
        fix_end(bs);
        return CCC_RESULT_OK;
    }
    if (++start_block != end_block)
    {
        int const v = b ? ~0 : 0;
        (void)memset(&bs->blocks[start_block], v,
                     (end_block - start_block) * SIZEOF_BLOCK);
    }
    ublockwidth const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    if (b)
    {
        bs->blocks[end_block] |= last_block_on;
    }
    else
    {
        bs->blocks[end_block] &= ~last_block_on;
    }
    fix_end(bs);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_bs_reset(ccc_bitset *const bs, size_t const i)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (i >= bs->count)
    {
        return CCC_TRIBOOL_ERROR;
    }
    ccc_bitblock *const block = block_at(bs, i);
    ccc_tribool const was = status(block, i);
    *block &= ~on(i);
    fix_end(bs);
    return was;
}

ccc_result
ccc_bs_reset_all(ccc_bitset *const bs)
{
    if (!bs)
    {
        return CCC_RESULT_ARG_ERROR;
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
ccc_result
ccc_bs_reset_range(ccc_bitset *const bs, size_t const i, size_t const count)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    size_t const end = i + count;
    size_t start_block = block_i(i);
    ublockwidth const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    bs->blocks[start_block] &= ~first_block_on;
    size_t const end_block = block_i(end - 1);
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
    ublockwidth const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    bs->blocks[end_block] &= ~last_block_on;
    fix_end(bs);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_bs_flip(ccc_bitset *const bs, size_t const i)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const b_i = block_i(i);
    if (b_i >= bs->count)
    {
        return CCC_TRIBOOL_ERROR;
    }
    ccc_bitblock *const block = &bs->blocks[b_i];
    ccc_tribool const was = status(block, i);
    *block ^= on(i);
    fix_end(bs);
    return was;
}

ccc_result
ccc_bs_flip_all(ccc_bitset *const bs)
{
    if (!bs)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!bs->count)
    {
        return CCC_RESULT_OK;
    }
    size_t const end = block_count(bs->count);
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
ccc_result
ccc_bs_flip_range(ccc_bitset *const bs, size_t const i, size_t const count)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    size_t const end = i + count;
    size_t start_block = block_i(i);
    ublockwidth const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    bs->blocks[start_block] ^= first_block_on;
    size_t const end_block = block_i(end - 1);
    if (end_block == start_block)
    {
        fix_end(bs);
        return CCC_RESULT_OK;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        bs->blocks[start_block] = ~bs->blocks[start_block];
    }
    ublockwidth const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    bs->blocks[end_block] ^= last_block_on;
    fix_end(bs);
    return CCC_RESULT_OK;
}

ccc_ucount
ccc_bs_capacity(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = bs->capacity};
}

ccc_ucount
ccc_bs_blocks_capacity(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = block_count(bs->capacity)};
}

ccc_ucount
ccc_bs_size(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = bs->count};
}

ccc_ucount
ccc_bs_blocks_size(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = block_count(bs->count)};
}

ccc_tribool
ccc_bs_empty(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !bs->count;
}

ccc_ucount
ccc_bs_popcount(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    if (!bs->count)
    {
        return (ccc_ucount){.count = 0};
    }
    size_t const end = block_count(bs->count);
    size_t cnt = 0;
    for (size_t i = 0; i < end; cnt += popcount(bs->blocks[i++]))
    {}
    return (ccc_ucount){.count = cnt};
}

ccc_ucount
ccc_bs_popcount_range(ccc_bitset const *const bs, size_t const i,
                      size_t const count)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    size_t const end = i + count;
    size_t popped = 0;
    size_t start_block = block_i(i);
    ublockwidth const start_i_in_block = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i_in_block;
    if (start_i_in_block + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i_in_block + count));
    }
    popped += popcount(first_block_on & bs->blocks[start_block]);
    size_t const end_block = block_i(end - 1);
    if (end_block == start_block)
    {
        return (ccc_ucount){.count = popped};
    }
    for (++start_block; start_block < end_block;
         popped += popcount(bs->blocks[start_block++]))
    {}
    ublockwidth const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    popped += popcount(last_block_on & bs->blocks[end_block]);
    return (ccc_ucount){.count = popped};
}

ccc_result
ccc_bs_push_back(ccc_bitset *const bs, ccc_tribool const b)
{
    if (!bs || b > CCC_TRUE || b < CCC_FALSE)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    ccc_result const check_resize = maybe_resize(bs, 1, bs->alloc);
    if (check_resize != CCC_RESULT_OK)
    {
        return check_resize;
    }
    ++bs->count;
    set(block_at(bs, bs->count - 1), bs->count - 1, b);
    fix_end(bs);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_bs_pop_back(ccc_bitset *const bs)
{
    if (!bs || !bs->count)
    {
        return CCC_TRIBOOL_ERROR;
    }
    ccc_tribool const was = status(block_at(bs, bs->count - 1), bs->count - 1);
    --bs->count;
    if (bs->count)
    {
        fix_end(bs);
    }
    else
    {
        bs->blocks[0] = 0;
    }
    return was;
}

ccc_tribool
ccc_bs_any_range(ccc_bitset const *const bs, size_t const i, size_t const count)
{
    return any_or_none_range(bs, i, count, CCC_TRUE);
}

ccc_tribool
ccc_bs_any(ccc_bitset const *const bs)
{
    return any_or_none_range(bs, 0, bs->count, CCC_TRUE);
}

ccc_tribool
ccc_bs_none_range(ccc_bitset const *const bs, size_t const i,
                  size_t const count)
{
    return any_or_none_range(bs, i, count, CCC_FALSE);
}

ccc_tribool
ccc_bs_none(ccc_bitset const *const bs)
{
    return any_or_none_range(bs, 0, bs->count, CCC_FALSE);
}

ccc_tribool
ccc_bs_all_range(ccc_bitset const *const bs, size_t const i, size_t const count)
{
    return all_range(bs, i, count);
}

ccc_tribool
ccc_bs_all(ccc_bitset const *const bs)
{
    return all_range(bs, 0, bs->count);
}

ccc_ucount
ccc_bs_first_trailing_one_range(ccc_bitset const *const bs, size_t const i,
                                size_t const count)
{
    return first_trailing_one_range(bs, i, count);
}

ccc_ucount
ccc_bs_first_trailing_one(ccc_bitset const *const bs)
{
    return first_trailing_one_range(bs, 0, bs->count);
}

ccc_ucount
ccc_bs_first_trailing_ones(ccc_bitset const *const bs, size_t const num_ones)
{
    return first_trailing_bits_range(bs, 0, bs->count, num_ones, CCC_TRUE);
}

ccc_ucount
ccc_bs_first_trailing_ones_range(ccc_bitset const *const bs, size_t const i,
                                 size_t const count, size_t const num_ones)
{
    return first_trailing_bits_range(bs, i, count, num_ones, CCC_TRUE);
}

ccc_ucount
ccc_bs_first_trailing_zero_range(ccc_bitset const *const bs, size_t const i,
                                 size_t const count)
{
    return first_trailing_zero_range(bs, i, count);
}

ccc_ucount
ccc_bs_first_trailing_zero(ccc_bitset const *const bs)
{
    return first_trailing_zero_range(bs, 0, bs->count);
}

ccc_ucount
ccc_bs_first_trailing_zeros(ccc_bitset const *const bs, size_t const num_zeros)
{
    return first_trailing_bits_range(bs, 0, bs->count, num_zeros, CCC_FALSE);
}

ccc_ucount
ccc_bs_first_trailing_zeros_range(ccc_bitset const *const bs, size_t const i,
                                  size_t const count, size_t const num_zeros)
{
    return first_trailing_bits_range(bs, i, count, num_zeros, CCC_FALSE);
}

ccc_ucount
ccc_bs_first_leading_one_range(ccc_bitset const *const bs, size_t const i,
                               size_t const count)
{
    return first_leading_one_range(bs, i, count);
}

ccc_ucount
ccc_bs_first_leading_one(ccc_bitset const *const bs)
{
    return first_leading_one_range(bs, bs->count - 1, bs->count);
}

ccc_ucount
ccc_bs_first_leading_ones(ccc_bitset const *const bs, size_t const num_ones)
{
    return first_leading_bits_range(bs, bs->count - 1, bs->count, num_ones,
                                    CCC_TRUE);
}

ccc_ucount
ccc_bs_first_leading_ones_range(ccc_bitset const *const bs, size_t const i,
                                size_t const count, size_t const num_ones)
{
    return first_leading_bits_range(bs, i, count, num_ones, CCC_TRUE);
}

ccc_ucount
ccc_bs_first_leading_zero_range(ccc_bitset const *const bs, size_t const i,
                                size_t const count)
{
    return first_leading_zero_range(bs, i, count);
}

ccc_ucount
ccc_bs_first_leading_zero(ccc_bitset const *const bs)
{
    return first_leading_zero_range(bs, bs->count - 1, bs->count);
}

ccc_ucount
ccc_bs_first_leading_zeros(ccc_bitset const *const bs, size_t const num_zeros)
{
    return first_leading_bits_range(bs, bs->count - 1, bs->count, num_zeros,
                                    CCC_FALSE);
}

ccc_ucount
ccc_bs_first_leading_zeros_range(ccc_bitset const *const bs, size_t const i,
                                 size_t const count, size_t const num_zeros)
{
    return first_leading_bits_range(bs, i, count, num_zeros, CCC_FALSE);
}

ccc_result
ccc_bs_clear(ccc_bitset *const bs)
{
    if (!bs)
    {
        return CCC_RESULT_ARG_ERROR;
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

ccc_result
ccc_bs_clear_and_free(ccc_bitset *const bs)
{
    if (!bs)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!bs->alloc)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    if (bs->blocks)
    {
        (void)bs->alloc(bs->blocks, 0, bs->aux);
    }
    bs->count = 0;
    bs->capacity = 0;
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_clear_and_free_reserve(ccc_bitset *bs, ccc_any_alloc_fn *fn)
{
    if (!bs || !fn)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (bs->blocks)
    {
        (void)fn(bs->blocks, 0, bs->aux);
    }
    bs->count = 0;
    bs->capacity = 0;
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_reserve(ccc_bitset *const bs, size_t const to_add,
               ccc_any_alloc_fn *const fn)
{
    if (!bs || !fn)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    return maybe_resize(bs, to_add, fn);
}

ccc_result
ccc_bs_copy(ccc_bitset *const dst, ccc_bitset const *const src,
            ccc_any_alloc_fn *const fn)
{
    if (!dst || !src || (dst->capacity < src->capacity && !fn))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Whatever future changes we make to bit set members should not fall out
       of sync with this code so save what we need to restore and then copy
       over everything else as a catch all. */
    ccc_bitblock *const dst_mem = dst->blocks;
    size_t const dst_cap = dst->capacity;
    ccc_any_alloc_fn *const dst_alloc = dst->alloc;
    *dst = *src;
    dst->blocks = dst_mem;
    dst->capacity = dst_cap;
    dst->alloc = dst_alloc;
    if (!src->capacity)
    {
        return CCC_RESULT_OK;
    }
    if (dst->capacity < src->capacity)
    {
        ccc_bitblock *const new_mem = fn(
            dst->blocks, block_count(src->capacity) * SIZEOF_BLOCK, dst->aux);
        if (!new_mem)
        {
            return CCC_RESULT_MEM_ERROR;
        }
        dst->blocks = new_mem;
        dst->capacity = src->capacity;
    }
    if (!src->blocks || !dst->blocks)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    (void)memcpy(dst->blocks, src->blocks,
                 block_count(src->capacity) * SIZEOF_BLOCK);
    fix_end(dst);
    return CCC_RESULT_OK;
}

ccc_bitblock *
ccc_bs_data(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return NULL;
    }
    return bs->blocks;
}

ccc_tribool
ccc_bs_eq(ccc_bitset const *const a, ccc_bitset const *const b)
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

/*=======================    Static Helpers    ==============================*/

/** Assumes set size is greater than or equal to subset size. */
static ccc_tribool
is_subset_of(struct ccc_bitset const *const set,
             struct ccc_bitset const *const subset)
{
    assert(set->count >= subset->count);
    for (size_t i = 0, end = block_count(subset->count); i < end; ++i)
    {
        /* Invariant: the last N unused bits in a set are zero so this works. */
        if ((set->blocks[i] & subset->blocks[i]) != subset->blocks[i])
        {
            return CCC_FALSE;
        }
    }
    return CCC_TRUE;
}

static ccc_result
maybe_resize(struct ccc_bitset *const bs, size_t const to_add,
             ccc_any_alloc_fn *const fn)
{
    size_t required = bs->count + to_add;
    if (required < bs->count)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (required <= bs->capacity)
    {
        return CCC_RESULT_OK;
    }
    if (!fn)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    if (!bs->count && to_add == 1)
    {
        required = BITBLOCK_BITS;
    }
    else if (to_add == 1)
    {
        required = bs->capacity * 2;
    }
    ccc_bitblock *const new_mem
        = fn(bs->blocks, block_count(required) * SIZEOF_BLOCK, bs->aux);
    if (!new_mem)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    bs->capacity = required;
    bs->blocks = new_mem;
    return CCC_RESULT_OK;
}

static ccc_ucount
first_trailing_one_range(struct ccc_bitset const *const bs, size_t const i,
                         size_t const count)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    size_t const end = i + count;
    size_t start_block = block_i(i);
    ublockwidth const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    size_t i_in_block = ctz(first_block_on & bs->blocks[start_block]);
    if (i_in_block != BITBLOCK_BITS)
    {
        return (ccc_ucount){
            .count = (start_block * BITBLOCK_BITS) + i_in_block,
        };
    }
    size_t const end_block = block_i(end - 1);
    if (end_block == start_block)
    {
        return (ccc_ucount){.error = CCC_RESULT_FAIL};
    }
    /* Handle all values in between start and end in bulk. */
    for (++start_block; start_block < end_block; ++start_block)
    {
        i_in_block = ctz(bs->blocks[start_block]);
        if (i_in_block != BITBLOCK_BITS)
        {
            return (ccc_ucount){
                .count = (start_block * BITBLOCK_BITS) + i_in_block,
            };
        }
    }
    /* Handle last block. */
    ublockwidth const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    i_in_block = ctz(last_block_on & bs->blocks[end_block]);
    if (i_in_block != BITBLOCK_BITS)
    {
        return (ccc_ucount){.count = (end_block * BITBLOCK_BITS) + i_in_block};
    }
    return (ccc_ucount){.error = CCC_RESULT_FAIL};
}

/** Finds the starting index of a sequence of 1's or 0's of the num_bits size in
linear time. The algorithm aims to efficiently skip as many bits as possible
while searching for the desired group. This avoids both an O(N^2) runtime and
the use of any unnecessary modulo or division operations in a hot loop. */
static ccc_ucount
first_trailing_bits_range(struct ccc_bitset const *const bs, size_t const i,
                          size_t const count, size_t const num_bits,
                          ccc_tribool const is_one)
{
    if (!bs || i >= bs->count || num_bits > count || i + count < i
        || i + count > bs->count)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    size_t const range_end = i + count;
    size_t num_found = 0;
    size_t bits_start = i;
    size_t cur_block = block_i(i);
    size_t cur_end = (cur_block * BITBLOCK_BITS) + BITBLOCK_BITS;
    ublockwidth block_i = blockwidth_i(i);
    do
    {
        /* Clean up some edge cases for the helper function because we allow
           the user to specify any range. What if our range ends before the
           end of this block? What if it starts after index 0 of the first
           block? Pretend out of range bits are zeros. */
        ccc_bitblock bits
            = is_one ? bs->blocks[cur_block] & (BITBLOCK_ALL_ON << block_i)
                     : (~bs->blocks[cur_block]) & (BITBLOCK_ALL_ON << block_i);
        if (cur_end > range_end)
        {
            /* Modulo at most once entire function, not every loop cycle. */
            bits &= ~(BITBLOCK_ALL_ON << blockwidth_i(range_end));
        }
        struct ugroup const ones
            = max_trailing_ones(bits, block_i, num_bits - num_found);
        if (ones.count >= num_bits)
        {
            /* Found the solution all at once within a block. */
            return (ccc_ucount){
                .count = (cur_block * BITBLOCK_BITS) + ones.block_start_i,
            };
        }
        if (!ones.block_start_i)
        {
            if (num_found + ones.count >= num_bits)
            {
                /* Found solution crossing block boundary from prefix blocks. */
                return (ccc_ucount){.count = bits_start};
            }
            /* Found a full block so keep on trucking. */
            num_found += ones.count;
        }
        else
        {
            /* Fail but we have largest skip possible to continue our search
               from in order to save double checking unnecessary prefixes. */
            bits_start = (cur_block * BITBLOCK_BITS) + ones.block_start_i;
            num_found = ones.count;
        }
        block_i = 0;
        ++cur_block;
        cur_end += BITBLOCK_BITS;
    } while (bits_start + num_bits <= range_end);
    return (ccc_ucount){.error = CCC_RESULT_FAIL};
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
static struct ugroup
max_trailing_ones(ccc_bitblock const b, ublockwidth const i_in_block,
                  size_t const ones_remaining)
{
    /* Easy exit skip to the next block. Helps with sparse sets. */
    if (!b)
    {
        return (struct ugroup){.block_start_i = BITBLOCK_BITS};
    }
    if (ones_remaining <= BITBLOCK_BITS)
    {
        assert(i_in_block < BITBLOCK_BITS);
        /* This branch must find a smaller group anywhere in this block which is
           the most work required in this algorithm. We have some tricks to tell
           when to give up on this as soon as possible. */
        ccc_bitblock b_check = b >> i_in_block;
        ccc_bitblock const remain
            = BITBLOCK_ALL_ON >> (BITBLOCK_BITS - ones_remaining);
        /* Because of power of 2 rules we can stop early when the shifted
           becomes impossible to match. */
        for (size_t shifts = 0; b_check >= remain; b_check >>= 1, ++shifts)
        {
            if ((remain & b_check) == remain)
            {
                return (struct ugroup){
                    .block_start_i = i_in_block + shifts,
                    .count = ones_remaining,
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
    size_t const ones_found = clz(~b);
    return (struct ugroup){.block_start_i = BITBLOCK_BITS - ones_found,
                           .count = ones_found};
}

static ccc_ucount
first_trailing_zero_range(struct ccc_bitset const *const bs, size_t const i,
                          size_t const count)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    size_t const end = i + count;
    size_t start_block = block_i(i);
    ublockwidth const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    size_t i_in_block = ctz(first_block_on & ~bs->blocks[start_block]);
    if (i_in_block != BITBLOCK_BITS)
    {
        return (ccc_ucount){
            .count = (start_block * BITBLOCK_BITS) + i_in_block,
        };
    }
    size_t const end_block = block_i(end - 1);
    if (start_block == end_block)
    {
        return (ccc_ucount){.error = CCC_RESULT_FAIL};
    }
    /* Handle all values in between start and end in bulk. */
    for (++start_block; start_block < end_block; ++start_block)
    {
        i_in_block = ctz(~bs->blocks[start_block]);
        if (i_in_block != BITBLOCK_BITS)
        {
            return (ccc_ucount){
                .count = (start_block * BITBLOCK_BITS) + i_in_block,
            };
        }
    }
    /* Handle last block. */
    ublockwidth const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    i_in_block = ctz(last_block_on & ~bs->blocks[end_block]);
    if (i_in_block != BITBLOCK_BITS)
    {
        return (ccc_ucount){.count = (end_block * BITBLOCK_BITS) + i_in_block};
    }
    return (ccc_ucount){.error = CCC_RESULT_FAIL};
}

static ccc_ucount
first_leading_one_range(struct ccc_bitset const *const bs, size_t const i,
                        size_t const count)
{
    if (!bs || i >= bs->count || count > bs->count || i + 1 < count)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    size_t const end_i = (i + 1 - count);
    ublockwidth const width_end_i = blockwidth_i(i);
    ublockwidth const width_final_i = blockwidth_i(end_i);
    size_t start_block_i = block_i(i);
    ccc_bitblock first_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - width_end_i) - 1);
    if (i - end_i < BITBLOCK_BITS)
    {
        first_block_on &= (BITBLOCK_ALL_ON << width_final_i);
    }
    size_t lead_zeros = clz(first_block_on & bs->blocks[start_block_i]);
    if (lead_zeros != BITBLOCK_BITS)
    {
        return (ccc_ucount){
            .count = (start_block_i * BITBLOCK_BITS)
                     + (BITBLOCK_BITS - lead_zeros - 1),
        };
    }
    size_t const end_block_i = block_i(end_i);
    if (end_block_i == start_block_i)
    {
        return (ccc_ucount){.error = CCC_RESULT_FAIL};
    }
    /* Handle all values in between start and end in bulk. */
    for (--start_block_i; start_block_i > end_block_i; --start_block_i)
    {
        lead_zeros = clz(bs->blocks[start_block_i]);
        if (lead_zeros != BITBLOCK_BITS)
        {
            return (ccc_ucount){
                .count = (start_block_i * BITBLOCK_BITS)
                         + (BITBLOCK_BITS - lead_zeros - 1),
            };
        }
    }
    /* Handle last block. */
    ccc_bitblock const last_block_on
        = ~(BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - width_final_i) - 1));
    lead_zeros = clz(last_block_on & bs->blocks[end_block_i]);
    if (lead_zeros != BITBLOCK_BITS)
    {
        return (ccc_ucount){
            .count
            = (end_block_i * BITBLOCK_BITS) + (BITBLOCK_BITS - lead_zeros - 1),
        };
    }
    return (ccc_ucount){.error = CCC_RESULT_FAIL};
}

static ccc_ucount
first_leading_bits_range(struct ccc_bitset const *const bs, size_t const i,
                         size_t const count, size_t const num_bits,
                         ccc_tribool const is_one)
{
    ptrdiff_t const range_end = (ptrdiff_t)(i - count);
    if (!bs || i >= bs->count || !num_bits || num_bits > count
        || range_end < -1)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    size_t num_found = 0;
    ptrdiff_t bits_start = (ptrdiff_t)i;
    ptrdiff_t cur_block = (ptrdiff_t)block_i(i);
    ptrdiff_t cur_end = (ptrdiff_t)((cur_block * BITBLOCK_BITS) - 1);
    ublockwidth block_i = blockwidth_i(i);
    do
    {
        ccc_bitblock bits
            = is_one
                  ? bs->blocks[cur_block]
                        & (BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - block_i) - 1))
                  : ~bs->blocks[cur_block]
                        & (BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - block_i) - 1));
        if (cur_end < range_end)
        {
            bits &= (BITBLOCK_ALL_ON << blockwidth_i(range_end + 1));
        }
        struct igroup const ones
            = max_leading_ones(bits, block_i, num_bits - num_found);
        if (ones.count >= num_bits)
        {
            return (ccc_ucount){
                .count = (cur_block * BITBLOCK_BITS) + ones.block_start_i,
            };
        }
        if (ones.block_start_i == BITBLOCK_BITS - 1)
        {
            if (num_found + ones.count >= num_bits)
            {
                return (ccc_ucount){.count = bits_start};
            }
            num_found += ones.count;
        }
        else
        {
            /* If the new block start index is -1, then this addition bumps us
               to the next block's Most Significant Bit and is a simple
               subtraction by one to start search on next block.*/
            bits_start
                = (ptrdiff_t)((cur_block * BITBLOCK_BITS) + ones.block_start_i);
            num_found = ones.count;
        }
        block_i = BITBLOCK_BITS - 1;
        --cur_block;
        cur_end -= BITBLOCK_BITS;
    } while (bits_start >= range_end + (ptrdiff_t)num_bits);
    return (ccc_ucount){.error = CCC_RESULT_FAIL};
}

/** Returns the maximum group of consecutive ones in the bit block given. If the
number of consecutive ones remaining cannot be found the function returns
where the next search should start from, a critical step to a linear search;
specifically, we seek any group of continuous ones that runs from some index
in the block to the start of the block (0th index).

If no continuous group of ones exist that runs to the start of the block, an -1
index is returned with a group size of 0 meaning the search for ones will need
to continue in the next block lower block. This is helpful for the main search
loop adding to its start index and number of ones found so far. Adding -1 is
just subtraction so this will correctly drop us to the top bit of the next Least
Significant Block to continue the search. */
static struct igroup
max_leading_ones(ccc_bitblock const b, ublockwidth const i_in_block,
                 size_t const ones_remaining)
{
    if (!b)
    {
        return (struct igroup){.block_start_i = -1};
    }
    if (ones_remaining <= BITBLOCK_BITS)
    {
        assert(i_in_block < BITBLOCK_BITS);
        ccc_bitblock b_check = b << (BITBLOCK_BITS - i_in_block - 1);
        ccc_bitblock const required = BITBLOCK_ALL_ON
                                      << (BITBLOCK_BITS - ones_remaining);
        for (ublockwidth shifts = 0; b_check; b_check <<= 1, ++shifts)
        {
            if ((required & b_check) == required)
            {
                return (struct igroup){
                    .block_start_i = i_in_block - shifts,
                    .count = ones_remaining,
                };
            }
        }
    }
    ptrdiff_t const num_ones_found = ctz(~b);
    return (struct igroup){
        .block_start_i = num_ones_found - 1,
        .count = num_ones_found,
    };
}

static ccc_ucount
first_leading_zero_range(struct ccc_bitset const *const bs, size_t const i,
                         size_t const count)
{
    if (!bs || i >= bs->count || count > bs->count || i + 1 < count)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    size_t const end_i = i + 1 - count;
    size_t start_block_i = block_i(i);
    ublockwidth const width_end_i = blockwidth_i(end_i);
    ublockwidth const width_start_i = blockwidth_i(i);
    ccc_bitblock first_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - width_start_i) - 1);
    if (i - end_i < BITBLOCK_BITS)
    {
        first_block_on &= (BITBLOCK_ALL_ON << width_end_i);
    }
    size_t lead_ones = clz(first_block_on & ~bs->blocks[start_block_i]);
    if (lead_ones != BITBLOCK_BITS)
    {
        return (ccc_ucount){
            .count = ((start_block_i * BITBLOCK_BITS)
                      + (BITBLOCK_BITS - lead_ones - 1)),
        };
    }
    size_t const end_block = block_i(end_i);
    if (end_block == start_block_i)
    {
        return (ccc_ucount){.error = CCC_RESULT_FAIL};
    }
    /* Handle all values in between start and end in bulk. */
    for (--start_block_i; start_block_i > end_block; --start_block_i)
    {
        lead_ones = clz(~bs->blocks[start_block_i]);
        if (lead_ones != BITBLOCK_BITS)
        {
            return (ccc_ucount){
                .count = ((start_block_i * BITBLOCK_BITS)
                          + (BITBLOCK_BITS - lead_ones - 1)),
            };
        }
    }
    /* Handle last block. */
    ccc_bitblock const last_block_on
        = ~(BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - width_end_i) - 1));
    lead_ones = clz(last_block_on & ~bs->blocks[end_block]);
    if (lead_ones != BITBLOCK_BITS)
    {
        return (ccc_ucount){
            .count
            = ((end_block * BITBLOCK_BITS) + (BITBLOCK_BITS - lead_ones - 1)),
        };
    }
    return (ccc_ucount){.error = CCC_RESULT_FAIL};
}

/** Performs the any or none scan operation over the specified range. The only
difference between the operations is the return value. Specify the desired
tribool value to return upon encountering an on bit. For any this is
CCC_TRUE. For none this is CCC_FALSE. Saves writing two identical fns. */
static ccc_tribool
any_or_none_range(struct ccc_bitset const *const bs, size_t const i,
                  size_t const count, ccc_tribool const ret)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count
        || ret < CCC_FALSE || ret > CCC_TRUE)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const end = i + count;
    size_t start_block = block_i(i);
    ublockwidth const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    if (first_block_on & bs->blocks[start_block])
    {
        return ret;
    }
    size_t const end_block = block_i(end - 1);
    if (end_block == start_block)
    {
        return !ret;
    }
    /* If this is the any check we might get lucky by checking the last block
       before looping over everything. */
    ublockwidth const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    if (last_block_on & bs->blocks[end_block])
    {
        return ret;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        if (bs->blocks[start_block] & BITBLOCK_ALL_ON)
        {
            return ret;
        }
    }
    return !ret;
}

/** Check for all on is slightly different from the any or none checks so we
need a painfully repetitive function. */
static ccc_tribool
all_range(struct ccc_bitset const *const bs, size_t const i, size_t const count)
{
    if (!bs || i >= bs->count || i + count < i || i + count > bs->count)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const end = i + count;
    size_t start_block = block_i(i);
    ublockwidth const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    if ((first_block_on & bs->blocks[start_block]) != first_block_on)
    {
        return CCC_FALSE;
    }
    size_t const end_block = block_i(end - 1);
    if (end_block == start_block)
    {
        return CCC_TRUE;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        if (bs->blocks[start_block] != BITBLOCK_ALL_ON)
        {
            return CCC_FALSE;
        }
    }
    size_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    if ((last_block_on & bs->blocks[end_block]) != last_block_on)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

/** Given the 0 based index from `[0, count of used bits in set)` returns a
reference to the block that such a bit belongs to. This block reference will
point to some block at index [0, count of blocks used in the set). */
static inline ccc_bitblock *
block_at(ccc_bitset const *const bs, size_t const bit_i)
{
    return &bs->blocks[block_i(bit_i)];
}

static inline void
set_all(struct ccc_bitset *const bs, ccc_tribool const b)
{
    (void)memset(bs->blocks, b ? ~0 : 0, block_count(bs->count) * SIZEOF_BLOCK);
    fix_end(bs);
}

/** Given a reference to the correct block in the set, the true set index (may
be greater than size of a block), and the value to set, sets the index in the
block to the given tribool value. */
static inline void
set(ccc_bitblock *const block, size_t const bit_i, ccc_tribool const b)
{
    if (b)
    {
        *block |= on(bit_i);
    }
    else
    {
        *block &= ~on(bit_i);
    }
}

/** Given the bit set and the set index--set index is allowed to be greater than
the size of one block--returns the status of the bit at that index. */
static inline ccc_tribool
status(ccc_bitblock const *const bs, size_t const bit_i)
{
    /* Be careful. Bitwise & does not promise to evaluate to 1 or 0. We often
       just use it where that conversion takes place implicitly for us. */
    return (*bs & on(bit_i)) != 0;
}

/** Return a block with only the desired bit turned on to true. */
static inline ccc_bitblock
on(size_t bit_i)
{
    return (ccc_bitblock)1 << blockwidth_i(bit_i);
}

/** Clears unused bits in the last block according to count. Sets the last block
to have only the used bits set to their given values and all bits after to zero.
This is used as a safety mechanism throughout the code after complex operations
on bit blocks to ensure any side effects on unused bits are deleted. */
static inline void
fix_end(struct ccc_bitset *const bs)
{
    *block_at(bs, bs->count - 1) &= last_on(bs);
}

/** Returns a mask of all bits on in the final bit block that represent only
those bits which are in use according to the bit set capacity. The remaining
higher order bits in the last block will be set to 0 because they are not
used. If the capacity is zero a block with all bits on is returned. */
static inline ccc_bitblock
last_on(struct ccc_bitset const *const bs)
{
    /* Remember, we fill from LSB to MSB so we want the mask to start at lower
       order bit which is why we do the second funky flip on the whole op. */
    return bs->count ? ~(((ccc_bitblock)~1) << blockwidth_i(bs->count - 1))
                     : BITBLOCK_ALL_ON;
}

/** Returns the 0-based index of the block to which the given index belongs.
Assumes the given index is somewhere between [0, size of bit set). The returned
index then represents the block in which this index resides which is guaranteed
to be in the range [0, last block containing last bit in active set). */
static inline size_t
block_i(size_t const bit_i)
{
    return bit_i / BITBLOCK_BITS;
}

/** Returns the 0-based index within a block to which the given index belongs.
This index will always be between [0, BITBLOCK_BITS - 1). */
static inline ublockwidth
blockwidth_i(size_t const bit_i)
{
    return bit_i % BITBLOCK_BITS;
}

/** Returns the number of blocks required to store the given bits. Assumes bits
is non-zero. */
static inline size_t
block_count(size_t const bits)
{
    static_assert(BITBLOCK_BITS);
    assert(bits);
    return (bits + (BITBLOCK_BITS - 1)) / BITBLOCK_BITS;
}

/** Returns min of size_t arguments. Beware of conversions. */
static inline size_t
size_t_min(size_t a, size_t b)
{
    return a < b ? a : b;
}

#if defined(__has_builtin) && __has_builtin(__builtin_ctz)                     \
    && __has_builtin(__builtin_clz) && __has_builtin(__builtin_popcount)

/** Counts the on bits in a bit block. */
static inline unsigned
popcount(ccc_bitblock const b)
{
    /* There are different pop counts for different integer widths. Be sure to
       catch the use of the wrong one by mistake here at compile time. */
    static_assert(SIZEOF_BLOCK == sizeof(unsigned));
    return __builtin_popcount(b);
}

/** Counts the number of trailing zeros in a bit block starting from least
significant bit. */
static inline unsigned
ctz(ccc_bitblock const b)
{
    static_assert(BITBLOCK_MSB < BITBLOCK_ALL_ON);
    static_assert(SIZEOF_BLOCK == sizeof(unsigned));
    return b ? (unsigned)__builtin_ctz(b) : BITBLOCK_BITS;
}

/** Counts the leading zeros in a bit block starting from the most significant
bit. */
static inline unsigned
clz(ccc_bitblock const b)
{
    static_assert(BITBLOCK_MSB < BITBLOCK_ALL_ON);
    static_assert(SIZEOF_BLOCK == sizeof(unsigned));
    return b ? (unsigned)__builtin_clz(b) : BITBLOCK_BITS;
}

#else /* !defined(__has_builtin) || !__has_builtin(__builtin_ctz)              \
    || !__has_builtin(__builtin_clz) || !__has_builtin(__builtin_popcount) */

/** Counts the on bits in a bit block. */
static inline unsigned
popcount(ccc_bitblock b)
{
    unsigned cnt = 0;
    for (; b; cnt += ((b & 1U) != 0), b >>= 1U)
    {}
    return cnt;
}

/** Counts the number of trailing zeros in a bit block starting from least
significant bit. */
static inline unsigned
ctz(ccc_bitblock b)
{
    if (!b)
    {
        return BLOCK_BITS;
    }
    unsigned cnt = 0;
    for (; (b & 1U) == 0; ++cnt, b >>= 1U)
    {}
    return cnt;
}

/** Counts the leading zeros in a bit block starting from the most significant
bit. */
static inline unsigned
clz(ccc_bitblock b)
{
    if (!b)
    {
        return BLOCK_BITS;
    }
    unsigned cnt = 0;
    for (; (b & BITBLOCK_MSB) == 0; ++cnt, b <<= 1U)
    {}
    return cnt;
}

#endif /* defined(__has_builtin) && __has_builtin(__builtin_ctz)               \
    && __has_builtin(__builtin_clz) && __has_builtin(__builtin_popcount) */
