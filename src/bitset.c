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

enum : ccc_bitblock
{
    /* @private How many total bits that fit in a ccc_bitblock. */
    BITBLOCK_BITS = (sizeof(ccc_bitblock) * CHAR_BIT),
    /* @private A mask of a ccc_bitblockwith all bits on. */
    BITBLOCK_ALL_ON = ((ccc_bitblock)~0),
    /* @private The Most Significant Bit of a ccc_bitblockturned on to 1. */
    BITBLOCK_MSB
    = (((ccc_bitblock)1) << (((sizeof(ccc_bitblock) * CHAR_BIT)) - 1)),
};

/** @private An index within a block. A block is bounded to some number of bits
as determined by the type used for each block. */
typedef size_t blockwidth_t;

/** @private A helper to allow for an efficient linear scan for groups of 0's
or 1's in the set. */
struct group
{
    /** A index [0, bit block bits) indicating the status of a search. */
    blockwidth_t block_start_i;
    /** The number of bits of same value found starting at block_start_i. */
    size_t count;
};

/*=========================   Prototypes   ==================================*/

static size_t set_block_i(size_t bit_i);
static void set(ccc_bitblock *, size_t bit_i, ccc_tribool);
static ccc_bitblock on(size_t bit_i);
static ccc_bitblock last_on(struct ccc_bitset const *);
static ccc_tribool status(ccc_bitblock const *, size_t bit_i);
static size_t blocks(size_t bits);
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
static struct group max_trailing_ones(ccc_bitblock b, size_t i_in_block,
                                      size_t num_ones_remaining);
static struct group max_leading_ones(ccc_bitblock b, ptrdiff_t i_in_block,
                                     ptrdiff_t num_ones_remaining);
static ccc_result maybe_resize(struct ccc_bitset *bs, size_t to_add,
                               ccc_any_alloc_fn *);
static size_t min(size_t, size_t);
static void set_all(struct ccc_bitset *bs, ccc_tribool b);
static blockwidth_t blockwidth_i(size_t bit_i);
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
    if (set->sz <= subset->sz)
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
    if (set->sz < subset->sz)
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
    if (!dst->sz || !src->sz)
    {
        return CCC_RESULT_OK;
    }
    size_t const end_block = blocks(min(dst->sz, src->sz));
    for (size_t b = 0; b < end_block; ++b)
    {
        dst->mem[b] |= src->mem[b];
    }
    dst->mem[set_block_i(dst->sz - 1)] &= last_on(dst);
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_xor(ccc_bitset *const dst, ccc_bitset const *const src)
{
    if (!dst || !src)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!dst->sz || !src->sz)
    {
        return CCC_RESULT_OK;
    }
    size_t const end_block = blocks(min(dst->sz, src->sz));
    for (size_t b = 0; b < end_block; ++b)
    {
        dst->mem[b] ^= src->mem[b];
    }
    dst->mem[set_block_i(dst->sz - 1)] &= last_on(dst);
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_and(ccc_bitset *dst, ccc_bitset const *src)
{
    if (!dst || !src)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!src->sz)
    {
        set_all(dst, CCC_FALSE);
        return CCC_RESULT_OK;
    }
    if (!dst->sz)
    {
        return CCC_RESULT_OK;
    }
    size_t smaller_end = blocks(min(dst->sz, src->sz));
    for (size_t b = 0; b < smaller_end; ++b)
    {
        dst->mem[b] &= src->mem[b];
    }
    if (dst->sz <= src->sz)
    {
        return CCC_RESULT_OK;
    }
    /* The src widens to align with dst as integers would; same consequences. */
    size_t const dst_blocks = blocks(dst->sz);
    size_t const remaining_blocks = dst_blocks - smaller_end;
    (void)memset(dst->mem + smaller_end, CCC_FALSE,
                 remaining_blocks * sizeof(ccc_bitblock));
    dst->mem[set_block_i(dst->sz - 1)] &= last_on(dst);
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_shiftl(ccc_bitset *const bs, size_t const left_shifts)
{
    if (!bs)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!bs->sz || !left_shifts)
    {
        return CCC_RESULT_OK;
    }
    if (left_shifts >= bs->sz)
    {
        set_all(bs, CCC_FALSE);
        return CCC_RESULT_OK;
    }
    size_t const last_block = set_block_i(bs->sz - 1);
    size_t const shifted_blocks = set_block_i(left_shifts);
    blockwidth_t const partial_shift = blockwidth_i(left_shifts);
    if (!partial_shift)
    {
        for (size_t shifted = last_block - shifted_blocks + 1,
                    overwritten = last_block;
             shifted--; --overwritten)
        {
            bs->mem[overwritten] = bs->mem[shifted];
        }
    }
    else
    {
        blockwidth_t const remaining_shift = BITBLOCK_BITS - partial_shift;
        for (size_t shifted = last_block - shifted_blocks,
                    overwritten = last_block;
             shifted > 0; --shifted, --overwritten)
        {
            bs->mem[overwritten] = (bs->mem[shifted] << partial_shift)
                                   | (bs->mem[shifted - 1] >> remaining_shift);
        }
        bs->mem[shifted_blocks] = bs->mem[0] << partial_shift;
    }
    for (size_t i = 0; i < shifted_blocks; ++i)
    {
        bs->mem[i] = 0;
    }
    bs->mem[set_block_i(bs->sz - 1)] &= last_on(bs);
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_shiftr(ccc_bitset *const bs, size_t const right_shifts)
{
    if (!bs)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!bs->sz || !right_shifts)
    {
        return CCC_RESULT_OK;
    }
    if (right_shifts >= bs->sz)
    {
        set_all(bs, CCC_FALSE);
        return CCC_RESULT_OK;
    }
    size_t const last_block = set_block_i(bs->sz - 1);
    size_t const shifted_blocks = set_block_i(right_shifts);
    blockwidth_t partial_shift = blockwidth_i(right_shifts);
    if (!partial_shift)
    {
        for (size_t shifted = shifted_blocks, overwritten = 0;
             shifted < last_block + 1; ++shifted, ++overwritten)
        {
            bs->mem[overwritten] = bs->mem[shifted];
        }
    }
    else
    {
        blockwidth_t remaining_shift = BITBLOCK_BITS - partial_shift;
        for (size_t shifted = shifted_blocks, overwritten = 0;
             shifted < last_block; ++shifted, ++overwritten)
        {
            bs->mem[overwritten] = (bs->mem[shifted + 1] << remaining_shift)
                                   | (bs->mem[shifted] >> partial_shift);
        }
        bs->mem[last_block - shifted_blocks]
            = bs->mem[last_block] >> partial_shift;
    }
    for (ptrdiff_t i = (ptrdiff_t)last_block,
                   end = (ptrdiff_t)(last_block - shifted_blocks);
         i > end; --i)
    {
        bs->mem[i] = 0;
    }
    bs->mem[set_block_i(bs->sz - 1)] &= last_on(bs);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_bs_test(ccc_bitset const *const bs, size_t const i)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const b_i = set_block_i(i);
    if (b_i >= bs->sz)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return status(&bs->mem[b_i], i);
}

ccc_tribool
ccc_bs_set(ccc_bitset *const bs, size_t const i, ccc_tribool const b)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const b_i = set_block_i(i);
    if (b_i >= bs->sz)
    {
        return CCC_TRIBOOL_ERROR;
    }
    ccc_bitblock *const block = &bs->mem[b_i];
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
    if (bs->sz)
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
    if (!bs || i >= bs->sz)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    size_t const end = i + count;
    if (end > bs->sz)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    size_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    if (b)
    {
        bs->mem[start_block] |= first_block_on;
    }
    else
    {
        bs->mem[start_block] &= ~first_block_on;
    }
    size_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        bs->mem[set_block_i(bs->sz - 1)] &= last_on(bs);
        return CCC_RESULT_OK;
    }
    if (++start_block != end_block)
    {
        int const v = b ? ~0 : 0;
        (void)memset(&bs->mem[start_block], v,
                     (end_block - start_block) * sizeof(ccc_bitblock));
    }
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    if (b)
    {
        bs->mem[end_block] |= last_block_on;
    }
    else
    {
        bs->mem[end_block] &= ~last_block_on;
    }
    bs->mem[set_block_i(bs->sz - 1)] &= last_on(bs);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_bs_reset(ccc_bitset *const bs, size_t const i)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const b_i = set_block_i(i);
    if (b_i >= bs->sz)
    {
        return CCC_TRIBOOL_ERROR;
    }
    ccc_bitblock *const block = &bs->mem[b_i];
    ccc_tribool const was = status(block, i);
    *block &= ~on(i);
    return was;
}

ccc_result
ccc_bs_reset_all(ccc_bitset *const bs)
{
    if (!bs)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (bs->sz)
    {
        (void)memset(bs->mem, CCC_FALSE, blocks(bs->sz) * sizeof(ccc_bitblock));
    }
    return CCC_RESULT_OK;
}

/** Same concept as set range but easier. Handle first and last then set
everything in between to false with memset. */
ccc_result
ccc_bs_reset_range(ccc_bitset *const bs, size_t const i, size_t const count)
{
    if (!bs || i >= bs->sz || i + count < i || i + count > bs->sz)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    size_t const end = i + count;
    size_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    bs->mem[start_block] &= ~first_block_on;
    size_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        bs->mem[set_block_i(bs->sz - 1)] &= last_on(bs);
        return CCC_RESULT_OK;
    }
    if (++start_block != end_block)
    {
        (void)memset(&bs->mem[start_block], CCC_FALSE,
                     (end_block - start_block) * sizeof(ccc_bitblock));
    }
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    bs->mem[end_block] &= ~last_block_on;
    bs->mem[set_block_i(bs->sz - 1)] &= last_on(bs);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_bs_flip(ccc_bitset *const bs, size_t const i)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const b_i = set_block_i(i);
    if (b_i >= bs->sz)
    {
        return CCC_TRIBOOL_ERROR;
    }
    ccc_bitblock *const block = &bs->mem[b_i];
    ccc_tribool const was = status(block, i);
    *block ^= on(i);
    return was;
}

ccc_result
ccc_bs_flip_all(ccc_bitset *const bs)
{
    if (!bs)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!bs->sz)
    {
        return CCC_RESULT_OK;
    }
    size_t const end = blocks(bs->sz);
    for (size_t i = 0; i < end; ++i)
    {
        bs->mem[i] = ~bs->mem[i];
    }
    bs->mem[end - 1] &= last_on(bs);
    return CCC_RESULT_OK;
}

/** Maybe future SIMD vectorization could speed things up here because we use
the same strat of handling first and last which just leaves a simpler bulk
operation in the middle. But we don't benefit from memset here. */
ccc_result
ccc_bs_flip_range(ccc_bitset *const bs, size_t const i, size_t const count)
{
    if (!bs || i >= bs->sz || i + count < i || i + count > bs->sz)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    size_t const end = i + count;
    size_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    bs->mem[start_block] ^= first_block_on;
    size_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        bs->mem[set_block_i(bs->sz - 1)] &= last_on(bs);
        return CCC_RESULT_OK;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        bs->mem[start_block] = ~bs->mem[start_block];
    }
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    bs->mem[end_block] ^= last_block_on;
    bs->mem[set_block_i(bs->sz - 1)] &= last_on(bs);
    return CCC_RESULT_OK;
}

ccc_ucount
ccc_bs_capacity(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = bs->cap};
}

ccc_ucount
ccc_bs_blocks_capacity(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = blocks(bs->cap)};
}

ccc_ucount
ccc_bs_size(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = bs->sz};
}

ccc_ucount
ccc_bs_blocks_size(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = blocks(bs->sz)};
}

ccc_tribool
ccc_bs_empty(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !bs->sz;
}

ccc_ucount
ccc_bs_popcount(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    if (!bs->sz)
    {
        return (ccc_ucount){.count = 0};
    }
    size_t const end = blocks(bs->sz);
    size_t cnt = 0;
    for (size_t i = 0; i < end; cnt += popcount(bs->mem[i++]))
    {}
    return (ccc_ucount){.count = cnt};
}

ccc_ucount
ccc_bs_popcount_range(ccc_bitset const *const bs, size_t const i,
                      size_t const count)
{
    if (!bs || i >= bs->sz || i + count < i || i + count > bs->sz)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    size_t const end = i + count;
    size_t popped = 0;
    size_t start_block = set_block_i(i);
    blockwidth_t const start_i_in_block = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i_in_block;
    if (start_i_in_block + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i_in_block + count));
    }
    popped += popcount(first_block_on & bs->mem[start_block]);
    size_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        return (ccc_ucount){.count = popped};
    }
    for (++start_block; start_block < end_block;
         popped += popcount(bs->mem[start_block++]))
    {}
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    popped += popcount(last_block_on & bs->mem[end_block]);
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
    ++bs->sz;
    set(&bs->mem[set_block_i(bs->sz - 1)], bs->sz - 1, b);
    bs->mem[set_block_i(bs->sz - 1)] &= last_on(bs);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_bs_pop_back(ccc_bitset *const bs)
{
    if (!bs || !bs->sz)
    {
        return CCC_TRIBOOL_ERROR;
    }
    ccc_tribool const was
        = status(&bs->mem[set_block_i(bs->sz - 1)], bs->sz - 1);
    --bs->sz;
    if (bs->sz)
    {
        bs->mem[set_block_i(bs->sz - 1)] &= last_on(bs);
    }
    else
    {
        bs->mem[0] = 0;
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
    return any_or_none_range(bs, 0, bs->sz, CCC_TRUE);
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
    return any_or_none_range(bs, 0, bs->sz, CCC_FALSE);
}

ccc_tribool
ccc_bs_all_range(ccc_bitset const *const bs, size_t const i, size_t const count)
{
    return all_range(bs, i, count);
}

ccc_tribool
ccc_bs_all(ccc_bitset const *const bs)
{
    return all_range(bs, 0, bs->sz);
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
    return first_trailing_one_range(bs, 0, bs->sz);
}

ccc_ucount
ccc_bs_first_trailing_ones(ccc_bitset const *const bs, size_t const num_ones)
{
    return first_trailing_bits_range(bs, 0, bs->sz, num_ones, CCC_TRUE);
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
    return first_trailing_zero_range(bs, 0, bs->sz);
}

ccc_ucount
ccc_bs_first_trailing_zeros(ccc_bitset const *const bs, size_t const num_zeros)
{
    return first_trailing_bits_range(bs, 0, bs->sz, num_zeros, CCC_FALSE);
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
    return first_leading_one_range(bs, bs->sz - 1, bs->sz);
}

ccc_ucount
ccc_bs_first_leading_ones(ccc_bitset const *const bs, size_t const num_ones)
{
    return first_leading_bits_range(bs, bs->sz - 1, bs->sz, num_ones, CCC_TRUE);
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
    return first_leading_zero_range(bs, bs->sz - 1, bs->sz);
}

ccc_ucount
ccc_bs_first_leading_zeros(ccc_bitset const *const bs, size_t const num_zeros)
{
    return first_leading_bits_range(bs, bs->sz - 1, bs->sz, num_zeros,
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
    if (bs->mem)
    {
        assert(bs->cap);
        (void)memset(bs->mem, CCC_FALSE,
                     blocks(bs->cap) * sizeof(ccc_bitblock));
    }
    bs->sz = 0;
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
    if (bs->mem)
    {
        (void)bs->alloc(bs->mem, 0, bs->aux);
    }
    bs->sz = 0;
    bs->cap = 0;
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_clear_and_free_reserve(ccc_bitset *bs, ccc_any_alloc_fn *fn)
{
    if (!bs || !fn)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (bs->mem)
    {
        (void)fn(bs->mem, 0, bs->aux);
    }
    bs->sz = 0;
    bs->cap = 0;
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
    if (!dst || !src || (dst->cap < src->cap && !fn))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Whatever future changes we make to bit set members should not fall out
       of sync with this code so save what we need to restore and then copy
       over everything else as a catch all. */
    ccc_bitblock *const dst_mem = dst->mem;
    size_t const dst_cap = dst->cap;
    ccc_any_alloc_fn *const dst_alloc = dst->alloc;
    *dst = *src;
    dst->mem = dst_mem;
    dst->cap = dst_cap;
    dst->alloc = dst_alloc;
    if (!src->cap)
    {
        return CCC_RESULT_OK;
    }
    if (dst->cap < src->cap)
    {
        ccc_bitblock *const new_mem
            = fn(dst->mem, blocks(src->cap) * sizeof(ccc_bitblock), dst->aux);
        if (!new_mem)
        {
            return CCC_RESULT_MEM_ERROR;
        }
        dst->mem = new_mem;
        dst->cap = src->cap;
    }
    if (!src->mem || !dst->mem)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    (void)memcpy(dst->mem, src->mem, blocks(src->cap) * sizeof(ccc_bitblock));
    dst->mem[set_block_i(dst->sz - 1)] &= last_on(dst);
    return CCC_RESULT_OK;
}

ccc_bitblock *
ccc_bs_data(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return NULL;
    }
    return bs->mem;
}

ccc_tribool
ccc_bs_eq(ccc_bitset const *const a, ccc_bitset const *const b)
{
    if (!a || !b)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (a->sz != b->sz)
    {
        return CCC_FALSE;
    }
    return memcmp(a->mem, b->mem, blocks(a->sz) * sizeof(ccc_bitblock)) == 0;
}

/*=======================    Static Helpers    ==============================*/

/** Assumes set size is greater than or equal to subset size. */
static ccc_tribool
is_subset_of(struct ccc_bitset const *const set,
             struct ccc_bitset const *const subset)
{
    assert(set->sz >= subset->sz);
    for (size_t i = 0, end = blocks(subset->sz); i < end; ++i)
    {
        /* Invariant: the last N unused bits in a set are zero so this works. */
        if ((set->mem[i] & subset->mem[i]) != subset->mem[i])
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
    size_t required = bs->sz + to_add;
    if (required < bs->sz)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (required <= bs->cap)
    {
        return CCC_RESULT_OK;
    }
    if (!fn)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    if (!bs->sz && to_add == 1)
    {
        required = BITBLOCK_BITS;
    }
    else if (to_add == 1)
    {
        required = bs->cap * 2;
    }
    ccc_bitblock *const new_mem
        = fn(bs->mem, blocks(required) * sizeof(ccc_bitblock), bs->aux);
    if (!new_mem)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    bs->cap = required;
    bs->mem = new_mem;
    return CCC_RESULT_OK;
}

static ccc_ucount
first_trailing_one_range(struct ccc_bitset const *const bs, size_t const i,
                         size_t const count)
{
    if (!bs || i >= bs->sz || i + count < i || i + count > bs->sz)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    size_t const end = i + count;
    size_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    size_t i_in_block = ctz(first_block_on & bs->mem[start_block]);
    if (i_in_block != BITBLOCK_BITS)
    {
        return (ccc_ucount){.count
                            = (start_block * BITBLOCK_BITS) + i_in_block};
    }
    size_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        return (ccc_ucount){.error = CCC_RESULT_FAIL};
    }
    /* Handle all values in between start and end in bulk. */
    for (++start_block; start_block < end_block; ++start_block)
    {
        i_in_block = ctz(bs->mem[start_block]);
        if (i_in_block != BITBLOCK_BITS)
        {
            return (ccc_ucount){.count
                                = (start_block * BITBLOCK_BITS) + i_in_block};
        }
    }
    /* Handle last block. */
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    i_in_block = ctz(last_block_on & bs->mem[end_block]);
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
    if (!bs || i >= bs->sz || num_bits > count || i + count < i
        || i + count > bs->sz)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    size_t const range_end = i + count;
    size_t num_found = 0;
    size_t bits_start = i;
    size_t cur_block = set_block_i(i);
    size_t cur_end = (cur_block * BITBLOCK_BITS) + BITBLOCK_BITS;
    blockwidth_t block_i = blockwidth_i(i);
    do
    {
        /* Clean up some edge cases for the helper function because we allow
           the user to specify any range. What if our range ends before the
           end of this block? What if it starts after index 0 of the first
           block? Pretend out of range bits are zeros. */
        ccc_bitblock bits
            = is_one ? bs->mem[cur_block] & (BITBLOCK_ALL_ON << block_i)
                     : (~bs->mem[cur_block]) & (BITBLOCK_ALL_ON << block_i);
        if (cur_end > range_end)
        {
            /* Modulo at most once entire function, not every loop cycle. */
            bits &= ~(BITBLOCK_ALL_ON << blockwidth_i(range_end));
        }
        struct group const ones
            = max_trailing_ones(bits, block_i, num_bits - num_found);
        if (ones.count >= num_bits)
        {
            /* Found the solution all at once within a block. */
            return (ccc_ucount){.count = (cur_block * BITBLOCK_BITS)
                                         + ones.block_start_i};
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

/** Returns the maximum group of consecutive ones in the bitblock given. If the
number of consecutive ones remaining cannot be found the function returns
where the next search should start from, a critical step to a linear search;
specifically, we seek any group of continuous ones that runs from some index
in the block to the end of the block. If no continuous group of ones exist
that runs to the end of the block, the BLOCK_BITS index is retuned with a
group size of 0 meaning the search for ones will need to continue in the
next block. This is helpful for the main search loop adding to its start
index and number of ones found so far. */
static struct group
max_trailing_ones(ccc_bitblock const b, size_t const i_in_block,
                  size_t const ones_remaining)
{
    /* Easy exit skip to the next block. Helps with sparse sets. */
    if (!b)
    {
        return (struct group){.block_start_i = BITBLOCK_BITS};
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
                return (struct group){.block_start_i = i_in_block + shifts,
                                      .count = ones_remaining};
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
    return (struct group){.block_start_i = BITBLOCK_BITS - ones_found,
                          .count = ones_found};
}

static ccc_ucount
first_trailing_zero_range(struct ccc_bitset const *const bs, size_t const i,
                          size_t const count)
{
    if (!bs || i >= bs->sz || i + count < i || i + count > bs->sz)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    size_t const end = i + count;
    size_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    size_t i_in_block = ctz(first_block_on & ~bs->mem[start_block]);
    if (i_in_block != BITBLOCK_BITS)
    {
        return (ccc_ucount){.count
                            = (start_block * BITBLOCK_BITS) + i_in_block};
    }
    size_t const end_block = set_block_i(end - 1);
    if (start_block == end_block)
    {
        return (ccc_ucount){.error = CCC_RESULT_FAIL};
    }
    /* Handle all values in between start and end in bulk. */
    for (++start_block; start_block < end_block; ++start_block)
    {
        i_in_block = ctz(~bs->mem[start_block]);
        if (i_in_block != BITBLOCK_BITS)
        {
            return (ccc_ucount){.count
                                = (start_block * BITBLOCK_BITS) + i_in_block};
        }
    }
    /* Handle last block. */
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    i_in_block = ctz(last_block_on & ~bs->mem[end_block]);
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
    if (!bs || i >= bs->sz || count > bs->sz || (ptrdiff_t)(i - count) < -1)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    ptrdiff_t const end = (ptrdiff_t)(i - count);
    blockwidth_t const final_block_i = blockwidth_i(i - count + 1);
    ptrdiff_t start_block = (ptrdiff_t)set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - start_i) - 1);
    if (end >= 0 && i - end < BITBLOCK_BITS)
    {
        first_block_on &= (BITBLOCK_ALL_ON << final_block_i);
    }
    ptrdiff_t lead_zeros
        = (ptrdiff_t)clz(first_block_on & bs->mem[start_block]);
    if (lead_zeros != BITBLOCK_BITS)
    {
        return (ccc_ucount){.count = (start_block * BITBLOCK_BITS)
                                     + (BITBLOCK_BITS - lead_zeros - 1)};
    }
    ptrdiff_t const end_block = (ptrdiff_t)set_block_i(end + 1);
    if (end_block == start_block)
    {
        return (ccc_ucount){.error = CCC_RESULT_FAIL};
    }
    /* Handle all values in between start and end in bulk. */
    for (--start_block; start_block > end_block; --start_block)
    {
        lead_zeros = (ptrdiff_t)clz(bs->mem[start_block]);
        if (lead_zeros != BITBLOCK_BITS)
        {
            return (ccc_ucount){.count = (start_block * BITBLOCK_BITS)
                                         + (BITBLOCK_BITS - lead_zeros - 1)};
        }
    }
    /* Handle last block. */
    blockwidth_t const last_i = blockwidth_i(end + 1);
    ccc_bitblock const last_block_on
        = ~(BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1));
    lead_zeros = clz(last_block_on & bs->mem[end_block]);
    if (lead_zeros != BITBLOCK_BITS)
    {
        return (ccc_ucount){.count = (end_block * BITBLOCK_BITS)
                                     + (BITBLOCK_BITS - lead_zeros - 1)};
    }
    return (ccc_ucount){.error = CCC_RESULT_FAIL};
}

static ccc_ucount
first_leading_bits_range(struct ccc_bitset const *const bs, size_t const i,
                         size_t const count, size_t const num_bits,
                         ccc_tribool const is_one)
{
    ptrdiff_t const range_end = (ptrdiff_t)(i - count);
    if (!bs || i >= bs->sz || !num_bits || num_bits > count || range_end < -1)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    size_t num_found = 0;
    ptrdiff_t bits_start = (ptrdiff_t)i;
    ptrdiff_t cur_block = (ptrdiff_t)set_block_i(i);
    ptrdiff_t cur_end = (ptrdiff_t)((cur_block * BITBLOCK_BITS) - 1);
    blockwidth_t block_i = blockwidth_i(i);
    do
    {
        ccc_bitblock bits
            = is_one
                  ? bs->mem[cur_block]
                        & (BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - block_i) - 1))
                  : ~bs->mem[cur_block]
                        & (BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - block_i) - 1));
        if (cur_end < range_end)
        {
            bits &= (BITBLOCK_ALL_ON << blockwidth_i(range_end + 1));
        }
        struct group const ones = max_leading_ones(
            bits, (ptrdiff_t)block_i, (ptrdiff_t)(num_bits - num_found));
        if (ones.count >= num_bits)
        {
            return (ccc_ucount){.count = (cur_block * BITBLOCK_BITS)
                                         + ones.block_start_i};
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
            bits_start
                = (ptrdiff_t)((cur_block * BITBLOCK_BITS) + ones.block_start_i);
            num_found = ones.count;
        }
        block_i = BITBLOCK_BITS - 1;
        --cur_block;
        cur_end -= BITBLOCK_BITS;
    } while (bits_start - (ptrdiff_t)num_bits >= range_end);
    return (ccc_ucount){.error = CCC_RESULT_FAIL};
}

static struct group
max_leading_ones(ccc_bitblock const b, ptrdiff_t const i_in_block,
                 ptrdiff_t const ones_remaining)
{
    if (!b)
    {
        return (struct group){.block_start_i = -1};
    }
    if (ones_remaining <= (ptrdiff_t)BITBLOCK_BITS)
    {
        assert(i_in_block < (ptrdiff_t)BITBLOCK_BITS);
        ccc_bitblock b_check = b << (BITBLOCK_BITS - i_in_block - 1);
        ccc_bitblock const required = BITBLOCK_ALL_ON
                                      << (BITBLOCK_BITS - ones_remaining);
        for (ptrdiff_t shifts = 0; b_check; b_check <<= 1, ++shifts)
        {
            if ((required & b_check) == required)
            {
                return (struct group){.block_start_i = i_in_block - shifts,
                                      .count = ones_remaining};
            }
        }
    }
    ptrdiff_t const num_ones_found = ctz(~b);
    return (struct group){.block_start_i = num_ones_found - 1,
                          .count = num_ones_found};
}

static ccc_ucount
first_leading_zero_range(struct ccc_bitset const *const bs, size_t const i,
                         size_t const count)
{
    ptrdiff_t const end = (ptrdiff_t)(i - count);
    if (!bs || i >= bs->sz || count > bs->sz || end < -1)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    ptrdiff_t start_block = (ptrdiff_t)set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - start_i) - 1);
    if (end >= 0 && i - end < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON >> (i - end));
    }
    ptrdiff_t lead_ones = clz(first_block_on & ~bs->mem[start_block]);
    if (lead_ones != BITBLOCK_BITS)
    {
        return (ccc_ucount){.count = ((start_block * BITBLOCK_BITS)
                                      + (BITBLOCK_BITS - lead_ones - 1))};
    }
    ptrdiff_t const end_block = (ptrdiff_t)set_block_i(end + 1);
    if (end_block == start_block)
    {
        return (ccc_ucount){.error = CCC_RESULT_FAIL};
    }
    /* Handle all values in between start and end in bulk. */
    for (--start_block; start_block > end_block; --start_block)
    {
        lead_ones = clz(~bs->mem[start_block]);
        if (lead_ones != BITBLOCK_BITS)
        {
            return (ccc_ucount){.count = ((start_block * BITBLOCK_BITS)
                                          + (BITBLOCK_BITS - lead_ones - 1))};
        }
    }
    /* Handle last block. */
    blockwidth_t const last_i = blockwidth_i(end + 1);
    ccc_bitblock const last_block_on
        = ~(BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1));
    lead_ones = clz(last_block_on & ~bs->mem[end_block]);
    if (lead_ones != BITBLOCK_BITS)
    {
        return (ccc_ucount){.count = ((end_block * BITBLOCK_BITS)
                                      + (BITBLOCK_BITS - lead_ones - 1))};
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
    if (!bs || i >= bs->sz || i + count < i || i + count > bs->sz
        || ret < CCC_FALSE || ret > CCC_TRUE)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const end = i + count;
    size_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    if (first_block_on & bs->mem[start_block])
    {
        return ret;
    }
    size_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        return !ret;
    }
    /* If this is the any check we might get lucky by checking the last block
       before looping over everything. */
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    if (last_block_on & bs->mem[end_block])
    {
        return ret;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        if (bs->mem[start_block] & BITBLOCK_ALL_ON)
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
    if (!bs || i >= bs->sz || i + count < i || i + count > bs->sz)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const end = i + count;
    size_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock first_block_on = BITBLOCK_ALL_ON << start_i;
    if (start_i + count < BITBLOCK_BITS)
    {
        first_block_on &= ~(BITBLOCK_ALL_ON << (start_i + count));
    }
    if ((first_block_on & bs->mem[start_block]) != first_block_on)
    {
        return CCC_FALSE;
    }
    size_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        return CCC_TRUE;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        if (bs->mem[start_block] != BITBLOCK_ALL_ON)
        {
            return CCC_FALSE;
        }
    }
    size_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock const last_block_on
        = BITBLOCK_ALL_ON >> ((BITBLOCK_BITS - last_i) - 1);
    if ((last_block_on & bs->mem[end_block]) != last_block_on)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

static inline void
set_all(struct ccc_bitset *const bs, ccc_tribool const b)
{
    (void)memset(bs->mem, b ? ~0 : 0, blocks(bs->sz) * sizeof(ccc_bitblock));
    bs->mem[set_block_i(bs->sz - 1)] &= last_on(bs);
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

/** Returns a mask of all bits on in the final bit block that represent only
those bits which are in use according to the bit set capacity. The remaining
higher order bits in the last block will be set to 0 because they are not
used. If the capacity is zero a block with all bits on is returned. */
static inline ccc_bitblock
last_on(struct ccc_bitset const *const bs)
{
    /* Remember, we fill from LSB to MSB so we want the mask to start at lower
       order bit which is why we do the second funky flip on the whole op. */
    return bs->sz ? ~(((ccc_bitblock)~1) << blockwidth_i(bs->sz - 1))
                  : BITBLOCK_ALL_ON;
}

/** Returns the 0-based index of the block to which the given index belongs.
Assumes the given index is somewhere between [0, size of bit set). The returned
index then represents the block in which this index resides which is guaranteed
to be in the range [0, last block containing last bit in active set). */
static inline size_t
set_block_i(size_t const bit_i)
{
    return bit_i / BITBLOCK_BITS;
}

/** Returns the 0-based index within a block to which the given index belongs.
This index will always be between [0, BITBLOCK_BITS - 1). */
static inline blockwidth_t
blockwidth_i(size_t const bit_i)
{
    return bit_i % BITBLOCK_BITS;
}

/** Returns the number of blocks required to store the given bits. Assumes bits
is non-zero. */
static inline size_t
blocks(size_t const bits)
{
    static_assert(BITBLOCK_BITS);
    assert(bits);
    return (bits + (BITBLOCK_BITS - 1)) / BITBLOCK_BITS;
}

/** Returns min of size_t arguments. Beware of conversions. */
static inline size_t
min(size_t a, size_t b)
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
    static_assert(sizeof(ccc_bitblock) == sizeof(unsigned));
    return __builtin_popcount(b);
}

/** Counts the number of trailing zeros in a bit block starting from least
significant bit. */
static inline unsigned
ctz(ccc_bitblock const b)
{
    static_assert(BITBLOCK_MSB < BITBLOCK_ALL_ON);
    static_assert(sizeof(ccc_bitblock) == sizeof(unsigned));
    return b ? (unsigned)__builtin_ctz(b) : BITBLOCK_BITS;
}

/** Counts the leading zeros in a bit block starting from the most significant
bit. */
static inline unsigned
clz(ccc_bitblock const b)
{
    static_assert(BITBLOCK_MSB < BITBLOCK_ALL_ON);
    static_assert(sizeof(ccc_bitblock) == sizeof(unsigned));
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
