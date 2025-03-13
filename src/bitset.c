/** This file implements a Bit Set using blocks of platform defined integers
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

/* @private How many total bits that fit in a ccc_bitblock_. */
#define BLOCK_BITS ((ptrdiff_t)(sizeof(ccc_bitblock_) * CHAR_BIT))
/* @private A mask of a ccc_bitblock_ with all bits on. */
#define ALL_BITS_ON ((ccc_bitblock_)~0)
/* @private The Most Significant Bit of a ccc_bitblock_ turned on to 1. */
#define BITBLOCK_MSB                                                           \
    (((ccc_bitblock_)1) << (((sizeof(ccc_bitblock_) * CHAR_BIT)) - 1))

/** @private An index within a block. A block is bounded to some number of bits
as determined by the type used for each block. */
typedef ptrdiff_t blockwidth_t;

/** @private A helper to allow for an efficient linear scan for groups of 0's
or 1's in the set. */
struct group
{
    ptrdiff_t block_start_i;
    ptrdiff_t count;
};

/*=========================   Prototypes   ==================================*/

static ptrdiff_t set_block_i(ptrdiff_t bit_i);
static void set(ccc_bitblock_ *, ptrdiff_t bit_i, ccc_tribool);
static ccc_bitblock_ on(ptrdiff_t bit_i);
static ccc_bitblock_ last_on(struct ccc_bitset_ const *);
static ccc_tribool status(ccc_bitblock_ const *, ptrdiff_t bit_i);
static ptrdiff_t blocks(ptrdiff_t bits);
static ptrdiff_t popcount(ccc_bitblock_);
static ccc_tribool any_or_none_range(struct ccc_bitset_ const *, ptrdiff_t i,
                                     ptrdiff_t count, ccc_tribool);
static ccc_tribool all_range(struct ccc_bitset_ const *bs, ptrdiff_t i,
                             ptrdiff_t count);
static ptrdiff_t first_trailing_one_range(struct ccc_bitset_ const *bs,
                                          ptrdiff_t i, ptrdiff_t count);
static ptrdiff_t first_trailing_zero_range(struct ccc_bitset_ const *bs,
                                           ptrdiff_t i, ptrdiff_t count);
static ptrdiff_t first_leading_one_range(struct ccc_bitset_ const *bs,
                                         ptrdiff_t i, ptrdiff_t count);
static ptrdiff_t first_leading_zero_range(struct ccc_bitset_ const *bs,
                                          ptrdiff_t i, ptrdiff_t count);
static ptrdiff_t countr_0(ccc_bitblock_);
static ptrdiff_t countl_0(ccc_bitblock_);
static ptrdiff_t first_trailing_bits_range(struct ccc_bitset_ const *bs,
                                           ptrdiff_t i, ptrdiff_t count,
                                           ptrdiff_t num_bits,
                                           ccc_tribool is_one);
static ptrdiff_t first_leading_bits_range(struct ccc_bitset_ const *bs,
                                          ptrdiff_t i, ptrdiff_t count,
                                          ptrdiff_t num_bits,
                                          ccc_tribool is_one);
static struct group max_trailing_ones(ccc_bitblock_ b, ptrdiff_t i_in_block,
                                      ptrdiff_t num_ones_remaining);
static struct group max_leading_ones(ccc_bitblock_ b, ptrdiff_t i_in_block,
                                     ptrdiff_t num_ones_remaining);
static ccc_result maybe_resize(struct ccc_bitset_ *bs, ptrdiff_t to_add);
static ptrdiff_t min(ptrdiff_t, ptrdiff_t);
static void set_all(struct ccc_bitset_ *bs, ccc_tribool b);
static blockwidth_t blockwidth_i(ptrdiff_t bit_i);
static ccc_tribool is_subset_of(struct ccc_bitset_ const *set,
                                struct ccc_bitset_ const *subset);
static ccc_tribool add_overflow(ptrdiff_t a, ptrdiff_t b);

/*=======================   Public Interface   ==============================*/

ccc_tribool
ccc_bs_is_proper_subset(ccc_bitset const *const set,
                        ccc_bitset const *const subset)
{
    if (!set || !subset)
    {
        return CCC_BOOL_ERR;
    }
    if (set->sz_ <= subset->sz_)
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
        return CCC_BOOL_ERR;
    }
    if (set->sz_ < subset->sz_)
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
    if (!dst->sz_ || !src->sz_)
    {
        return CCC_RESULT_OK;
    }
    ptrdiff_t const end_block = blocks(min(dst->sz_, src->sz_));
    for (ptrdiff_t b = 0; b < end_block; ++b)
    {
        dst->mem_[b] |= src->mem_[b];
    }
    dst->mem_[set_block_i(dst->sz_ - 1)] &= last_on(dst);
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_xor(ccc_bitset *const dst, ccc_bitset const *const src)
{
    if (!dst || !src)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!dst->sz_ || !src->sz_)
    {
        return CCC_RESULT_OK;
    }
    ptrdiff_t const end_block = blocks(min(dst->sz_, src->sz_));
    for (ptrdiff_t b = 0; b < end_block; ++b)
    {
        dst->mem_[b] ^= src->mem_[b];
    }
    dst->mem_[set_block_i(dst->sz_ - 1)] &= last_on(dst);
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_and(ccc_bitset *dst, ccc_bitset const *src)
{
    if (!dst || !src)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!src->sz_)
    {
        set_all(dst, CCC_FALSE);
        return CCC_RESULT_OK;
    }
    if (!dst->sz_)
    {
        return CCC_RESULT_OK;
    }
    ptrdiff_t smaller_end = blocks(min(dst->sz_, src->sz_));
    for (ptrdiff_t b = 0; b < smaller_end; ++b)
    {
        dst->mem_[b] &= src->mem_[b];
    }
    if (dst->sz_ <= src->sz_)
    {
        return CCC_RESULT_OK;
    }
    /* The src widens to align with dst as integers would; same consequences. */
    ptrdiff_t const dst_blocks = blocks(dst->sz_);
    ptrdiff_t const remaining_blocks = dst_blocks - smaller_end;
    (void)memset(dst->mem_ + smaller_end, CCC_FALSE,
                 remaining_blocks * sizeof(ccc_bitblock_));
    dst->mem_[set_block_i(dst->sz_ - 1)] &= last_on(dst);
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_shiftl(ccc_bitset *const bs, ptrdiff_t const left_shifts)
{
    if (!bs || left_shifts < 0)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!bs->sz_ || !left_shifts)
    {
        return CCC_RESULT_OK;
    }
    if (left_shifts >= bs->sz_)
    {
        set_all(bs, CCC_FALSE);
        return CCC_RESULT_OK;
    }
    ptrdiff_t const last_block = set_block_i(bs->sz_ - 1);
    ptrdiff_t const shifted_blocks = set_block_i(left_shifts);
    blockwidth_t const partial_shift = blockwidth_i(left_shifts);
    if (!partial_shift)
    {
        for (ptrdiff_t shifted = last_block - shifted_blocks + 1,
                       overwritten = last_block;
             shifted--; --overwritten)
        {
            bs->mem_[overwritten] = bs->mem_[shifted];
        }
    }
    else
    {
        blockwidth_t const remaining_shift = BLOCK_BITS - partial_shift;
        for (ptrdiff_t shifted = last_block - shifted_blocks,
                       overwritten = last_block;
             shifted > 0; --shifted, --overwritten)
        {
            bs->mem_[overwritten]
                = (bs->mem_[shifted] << partial_shift)
                  | (bs->mem_[shifted - 1] >> remaining_shift);
        }
        bs->mem_[shifted_blocks] = bs->mem_[0] << partial_shift;
    }
    for (ptrdiff_t i = 0; i < shifted_blocks; ++i)
    {
        bs->mem_[i] = 0;
    }
    bs->mem_[set_block_i(bs->sz_ - 1)] &= last_on(bs);
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_shiftr(ccc_bitset *const bs, ptrdiff_t const right_shifts)
{
    if (!bs || right_shifts < 0)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!bs->sz_ || !right_shifts)
    {
        return CCC_RESULT_OK;
    }
    if (right_shifts >= bs->sz_)
    {
        set_all(bs, CCC_FALSE);
        return CCC_RESULT_OK;
    }
    ptrdiff_t const last_block = set_block_i(bs->sz_ - 1);
    ptrdiff_t const shifted_blocks = set_block_i(right_shifts);
    blockwidth_t partial_shift = blockwidth_i(right_shifts);
    if (!partial_shift)
    {
        for (ptrdiff_t shifted = shifted_blocks, overwritten = 0;
             shifted < last_block + 1; ++shifted, ++overwritten)
        {
            bs->mem_[overwritten] = bs->mem_[shifted];
        }
    }
    else
    {
        blockwidth_t remaining_shift = BLOCK_BITS - partial_shift;
        for (ptrdiff_t shifted = shifted_blocks, overwritten = 0;
             shifted < last_block; ++shifted, ++overwritten)
        {
            bs->mem_[overwritten] = (bs->mem_[shifted + 1] << remaining_shift)
                                    | (bs->mem_[shifted] >> partial_shift);
        }
        bs->mem_[last_block - shifted_blocks]
            = bs->mem_[last_block] >> partial_shift;
    }
    for (ptrdiff_t i = last_block, end = (last_block - shifted_blocks); i > end;
         --i)
    {
        bs->mem_[i] = 0;
    }
    bs->mem_[set_block_i(bs->sz_ - 1)] &= last_on(bs);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_bs_test(ccc_bitset const *const bs, ptrdiff_t const i)
{
    if (!bs || i < 0)
    {
        return CCC_BOOL_ERR;
    }
    ptrdiff_t const b_i = set_block_i(i);
    if (b_i >= bs->sz_)
    {
        return CCC_BOOL_ERR;
    }
    return status(&bs->mem_[b_i], i);
}

ccc_tribool
ccc_bs_set(ccc_bitset *const bs, ptrdiff_t const i, ccc_tribool const b)
{
    if (!bs || i < 0)
    {
        return CCC_BOOL_ERR;
    }
    ptrdiff_t const b_i = set_block_i(i);
    if (b_i >= bs->sz_)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &bs->mem_[b_i];
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
    if (bs->sz_)
    {
        set_all(bs, b);
    }
    return CCC_RESULT_OK;
}

/* A naive implementation might just call set for every index between the
   start and start + count. However, calculating the block and index within
   each block for every call to set costs a division and a modulo operation. We
   can avoid this by handling the first and last block and then handling
   everything in between with a bulk memset. */
ccc_result
ccc_bs_set_range(ccc_bitset *const bs, ptrdiff_t const i, ptrdiff_t const count,
                 ccc_tribool const b)
{
    if (!bs || i < 0 || count < 0 || i >= bs->sz_ || add_overflow(i, count))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    ptrdiff_t const end = i + count;
    if (end > bs->sz_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    ptrdiff_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i;
    if (start_i + count < BLOCK_BITS)
    {
        first_block_on &= ~(ALL_BITS_ON << (start_i + count));
    }
    if (b)
    {
        bs->mem_[start_block] |= first_block_on;
    }
    else
    {
        bs->mem_[start_block] &= ~first_block_on;
    }
    ptrdiff_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        bs->mem_[set_block_i(bs->sz_ - 1)] &= last_on(bs);
        return CCC_RESULT_OK;
    }
    if (++start_block != end_block)
    {
        int const v = b ? ~0 : 0;
        (void)memset(&bs->mem_[start_block], v,
                     (end_block - start_block) * sizeof(ccc_bitblock_));
    }
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock_ const last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - last_i) - 1);
    if (b)
    {
        bs->mem_[end_block] |= last_block_on;
    }
    else
    {
        bs->mem_[end_block] &= ~last_block_on;
    }
    bs->mem_[set_block_i(bs->sz_ - 1)] &= last_on(bs);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_bs_reset(ccc_bitset *const bs, ptrdiff_t const i)
{
    if (!bs || i < 0)
    {
        return CCC_BOOL_ERR;
    }
    ptrdiff_t const b_i = set_block_i(i);
    if (b_i >= bs->sz_)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &bs->mem_[b_i];
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
    if (bs->sz_)
    {
        (void)memset(bs->mem_, CCC_FALSE,
                     blocks(bs->sz_) * sizeof(ccc_bitblock_));
    }
    return CCC_RESULT_OK;
}

/* Same concept as set range but easier. Handle first and last then set
   everything in between to false with memset. */
ccc_result
ccc_bs_reset_range(ccc_bitset *const bs, ptrdiff_t const i,
                   ptrdiff_t const count)
{
    if (!bs || i < 0 || count < 0 || i >= bs->sz_ || add_overflow(i, count))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    ptrdiff_t const end = i + count;
    if (end > bs->sz_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    ptrdiff_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i;
    if (start_i + count < BLOCK_BITS)
    {
        first_block_on &= ~(ALL_BITS_ON << (start_i + count));
    }
    bs->mem_[start_block] &= ~first_block_on;
    ptrdiff_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        bs->mem_[set_block_i(bs->sz_ - 1)] &= last_on(bs);
        return CCC_RESULT_OK;
    }
    if (++start_block != end_block)
    {
        (void)memset(&bs->mem_[start_block], CCC_FALSE,
                     (end_block - start_block) * sizeof(ccc_bitblock_));
    }
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock_ const last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - last_i) - 1);
    bs->mem_[end_block] &= ~last_block_on;
    bs->mem_[set_block_i(bs->sz_ - 1)] &= last_on(bs);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_bs_flip(ccc_bitset *const bs, ptrdiff_t const i)
{
    if (!bs || i < 0)
    {
        return CCC_BOOL_ERR;
    }
    ptrdiff_t const b_i = set_block_i(i);
    if (b_i >= bs->sz_)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &bs->mem_[b_i];
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
    if (!bs->sz_)
    {
        return CCC_RESULT_OK;
    }
    ptrdiff_t const end = blocks(bs->sz_);
    for (ptrdiff_t i = 0; i < end; ++i)
    {
        bs->mem_[i] = ~bs->mem_[i];
    }
    bs->mem_[end - 1] &= last_on(bs);
    return CCC_RESULT_OK;
}

/* Maybe future SIMD vectorization could speed things up here because we use
   the same strat of handling first and last which just leaves a simpler bulk
   operation in the middle. But we don't benefit from memset here. */
ccc_result
ccc_bs_flip_range(ccc_bitset *const bs, ptrdiff_t const i,
                  ptrdiff_t const count)
{
    if (!bs || i < 0 || count < 0 || i >= bs->sz_ || add_overflow(i, count))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    ptrdiff_t const end = i + count;
    if (end > bs->sz_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    ptrdiff_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i;
    if (start_i + count < BLOCK_BITS)
    {
        first_block_on &= ~(ALL_BITS_ON << (start_i + count));
    }
    bs->mem_[start_block] ^= first_block_on;
    ptrdiff_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        bs->mem_[set_block_i(bs->sz_ - 1)] &= last_on(bs);
        return CCC_RESULT_OK;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        bs->mem_[start_block] = ~bs->mem_[start_block];
    }
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock_ const last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - last_i) - 1);
    bs->mem_[end_block] ^= last_block_on;
    bs->mem_[set_block_i(bs->sz_ - 1)] &= last_on(bs);
    return CCC_RESULT_OK;
}

ptrdiff_t
ccc_bs_capacity(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return -1;
    }
    return bs->cap_;
}

ptrdiff_t
ccc_bs_blocks_capacity(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return -1;
    }
    return blocks(bs->cap_);
}

ptrdiff_t
ccc_bs_size(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return -1;
    }
    return bs->sz_;
}

ptrdiff_t
ccc_bs_blocks_size(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return -1;
    }
    return blocks(bs->sz_);
}

ccc_tribool
ccc_bs_empty(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return CCC_BOOL_ERR;
    }
    return !bs->sz_;
}

ptrdiff_t
ccc_bs_popcount(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return -1;
    }
    if (!bs->sz_)
    {
        return 0;
    }
    ptrdiff_t const end = blocks(bs->sz_);
    ptrdiff_t cnt = 0;
    for (ptrdiff_t i = 0; i < end; cnt += popcount(bs->mem_[i++]))
    {}
    return cnt;
}

ptrdiff_t
ccc_bs_popcount_range(ccc_bitset const *const bs, ptrdiff_t const i,
                      ptrdiff_t const count)
{
    if (!bs || i < 0 || i >= bs->sz_ || count < 0 || add_overflow(i, count))
    {
        return -1;
    }
    ptrdiff_t const end = i + count;
    if (end > bs->sz_)
    {
        return -1;
    }
    ptrdiff_t popped = 0;
    ptrdiff_t start_block = set_block_i(i);
    blockwidth_t const start_i_in_block = blockwidth_i(i);
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on &= ~(ALL_BITS_ON << (start_i_in_block + count));
    }
    popped += popcount(first_block_on & bs->mem_[start_block]);
    ptrdiff_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        return popped;
    }
    for (++start_block; start_block < end_block;
         popped += popcount(bs->mem_[start_block++]))
    {}
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock_ const last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - last_i) - 1);
    popped += popcount(last_block_on & bs->mem_[end_block]);
    return popped;
}

ccc_result
ccc_bs_push_back(ccc_bitset *const bs, ccc_tribool const b)
{
    if (!bs || b > CCC_TRUE || b < CCC_FALSE)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    ccc_result const check_resize = maybe_resize(bs, 1);
    if (check_resize != CCC_RESULT_OK)
    {
        return check_resize;
    }
    ++bs->sz_;
    set(&bs->mem_[set_block_i(bs->sz_ - 1)], bs->sz_ - 1, b);
    bs->mem_[set_block_i(bs->sz_ - 1)] &= last_on(bs);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_bs_pop_back(ccc_bitset *const bs)
{
    if (!bs || !bs->sz_)
    {
        return CCC_BOOL_ERR;
    }
    ccc_tribool const was
        = status(&bs->mem_[set_block_i(bs->sz_ - 1)], bs->sz_ - 1);
    --bs->sz_;
    if (bs->sz_)
    {
        bs->mem_[set_block_i(bs->sz_ - 1)] &= last_on(bs);
    }
    else
    {
        bs->mem_[0] = 0;
    }
    return was;
}

ccc_tribool
ccc_bs_any_range(ccc_bitset const *const bs, ptrdiff_t const i,
                 ptrdiff_t const count)
{
    return any_or_none_range(bs, i, count, CCC_TRUE);
}

ccc_tribool
ccc_bs_any(ccc_bitset const *const bs)
{
    return any_or_none_range(bs, 0, bs->sz_, CCC_TRUE);
}

ccc_tribool
ccc_bs_none_range(ccc_bitset const *const bs, ptrdiff_t const i,
                  ptrdiff_t const count)
{
    return any_or_none_range(bs, i, count, CCC_FALSE);
}

ccc_tribool
ccc_bs_none(ccc_bitset const *const bs)
{
    return any_or_none_range(bs, 0, bs->sz_, CCC_FALSE);
}

ccc_tribool
ccc_bs_all_range(ccc_bitset const *const bs, ptrdiff_t const i,
                 ptrdiff_t const count)
{
    return all_range(bs, i, count);
}

ccc_tribool
ccc_bs_all(ccc_bitset const *const bs)
{
    return all_range(bs, 0, bs->sz_);
}

ptrdiff_t
ccc_bs_first_trailing_one_range(ccc_bitset const *const bs, ptrdiff_t const i,
                                ptrdiff_t const count)
{
    return first_trailing_one_range(bs, i, count);
}

ptrdiff_t
ccc_bs_first_trailing_one(ccc_bitset const *const bs)
{
    return first_trailing_one_range(bs, 0, bs->sz_);
}

ptrdiff_t
ccc_bs_first_trailing_ones(ccc_bitset const *const bs, ptrdiff_t const num_ones)
{
    return first_trailing_bits_range(bs, 0, bs->sz_, num_ones, CCC_TRUE);
}

ptrdiff_t
ccc_bs_first_trailing_ones_range(ccc_bitset const *const bs, ptrdiff_t const i,
                                 ptrdiff_t const count,
                                 ptrdiff_t const num_ones)
{
    return first_trailing_bits_range(bs, i, count, num_ones, CCC_TRUE);
}

ptrdiff_t
ccc_bs_first_trailing_zero_range(ccc_bitset const *const bs, ptrdiff_t const i,
                                 ptrdiff_t const count)
{
    return first_trailing_zero_range(bs, i, count);
}

ptrdiff_t
ccc_bs_first_trailing_zero(ccc_bitset const *const bs)
{
    return first_trailing_zero_range(bs, 0, bs->sz_);
}

ptrdiff_t
ccc_bs_first_trailing_zeros(ccc_bitset const *const bs,
                            ptrdiff_t const num_zeros)
{
    return first_trailing_bits_range(bs, 0, bs->sz_, num_zeros, CCC_FALSE);
}

ptrdiff_t
ccc_bs_first_trailing_zeros_range(ccc_bitset const *const bs, ptrdiff_t const i,
                                  ptrdiff_t const count,
                                  ptrdiff_t const num_zeros)
{
    return first_trailing_bits_range(bs, i, count, num_zeros, CCC_FALSE);
}

ptrdiff_t
ccc_bs_first_leading_one_range(ccc_bitset const *const bs, ptrdiff_t const i,
                               ptrdiff_t const count)
{
    return first_leading_one_range(bs, i, count);
}

ptrdiff_t
ccc_bs_first_leading_one(ccc_bitset const *const bs)
{
    return first_leading_one_range(bs, bs->sz_ - 1, bs->sz_);
}

ptrdiff_t
ccc_bs_first_leading_ones(ccc_bitset const *const bs, ptrdiff_t const num_ones)
{
    return first_leading_bits_range(bs, bs->sz_ - 1, bs->sz_, num_ones,
                                    CCC_TRUE);
}

ptrdiff_t
ccc_bs_first_leading_ones_range(ccc_bitset const *const bs, ptrdiff_t const i,
                                ptrdiff_t const count, ptrdiff_t const num_ones)
{
    return first_leading_bits_range(bs, i, count, num_ones, CCC_TRUE);
}

ptrdiff_t
ccc_bs_first_leading_zero_range(ccc_bitset const *const bs, ptrdiff_t const i,
                                ptrdiff_t const count)
{
    return first_leading_zero_range(bs, i, count);
}

ptrdiff_t
ccc_bs_first_leading_zero(ccc_bitset const *const bs)
{
    return first_leading_zero_range(bs, bs->sz_ - 1, bs->sz_);
}

ptrdiff_t
ccc_bs_first_leading_zeros(ccc_bitset const *const bs,
                           ptrdiff_t const num_zeros)
{
    return first_leading_bits_range(bs, bs->sz_ - 1, bs->sz_, num_zeros,
                                    CCC_FALSE);
}

ptrdiff_t
ccc_bs_first_leading_zeros_range(ccc_bitset const *const bs, ptrdiff_t const i,
                                 ptrdiff_t const count,
                                 ptrdiff_t const num_zeros)
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
    if (bs->mem_)
    {
        (void)memset(bs->mem_, CCC_FALSE,
                     blocks(bs->sz_) * sizeof(ccc_bitblock_));
    }
    bs->sz_ = 0;
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_clear_and_free(ccc_bitset *const bs)
{
    if (!bs)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!bs->alloc_)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    if (bs->mem_)
    {
        (void)bs->alloc_(bs->mem_, 0, bs->aux_);
    }
    bs->sz_ = 0;
    bs->cap_ = 0;
    return CCC_RESULT_OK;
}

ccc_result
ccc_bs_copy(ccc_bitset *const dst, ccc_bitset const *const src,
            ccc_alloc_fn *const fn)
{
    if (!dst || !src || (dst->cap_ < src->cap_ && !fn))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Whatever future changes we make to bit set members should not fall out
       of sync with this code so save what we need to restore and then copy
       over everything else as a catch all. */
    ccc_bitblock_ *const dst_mem = dst->mem_;
    ptrdiff_t const dst_cap = dst->cap_;
    ccc_alloc_fn *const dst_alloc = dst->alloc_;
    *dst = *src;
    dst->mem_ = dst_mem;
    dst->cap_ = dst_cap;
    dst->alloc_ = dst_alloc;
    if (dst->cap_ < src->cap_)
    {
        ccc_bitblock *const new_mem = fn(
            dst->mem_, blocks(src->cap_) * sizeof(ccc_bitblock_), dst->aux_);
        if (!new_mem)
        {
            return CCC_RESULT_MEM_ERR;
        }
        dst->mem_ = new_mem;
        dst->cap_ = src->cap_;
    }
    (void)memcpy(dst->mem_, src->mem_,
                 blocks(src->cap_) * sizeof(ccc_bitblock_));
    dst->mem_[set_block_i(dst->sz_ - 1)] &= last_on(dst);
    return CCC_RESULT_OK;
}

ccc_bitblock *
ccc_bs_data(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return NULL;
    }
    return bs->mem_;
}

ccc_tribool
ccc_bs_eq(ccc_bitset const *const a, ccc_bitset const *const b)
{
    if (!a || !b)
    {
        return CCC_BOOL_ERR;
    }
    if (a->sz_ != b->sz_)
    {
        return CCC_FALSE;
    }
    return memcmp(a->mem_, b->mem_, blocks(a->sz_) * sizeof(ccc_bitblock_))
           == 0;
}

/*=======================    Static Helpers    ==============================*/

/* Assumes set size is greater than or equal to subset size. */
static ccc_tribool
is_subset_of(struct ccc_bitset_ const *const set,
             struct ccc_bitset_ const *const subset)
{
    assert(set->sz_ >= subset->sz_);
    for (ptrdiff_t i = 0, end = blocks(subset->sz_); i < end; ++i)
    {
        /* Invariant: the last N unused bits in a set are zero so this works. */
        if ((set->mem_[i] & subset->mem_[i]) != subset->mem_[i])
        {
            return CCC_FALSE;
        }
    }
    return CCC_TRUE;
}

static ccc_result
maybe_resize(struct ccc_bitset_ *const bs, ptrdiff_t const to_add)
{
    if (to_add < 0 || bs->sz_ < 0 || add_overflow(bs->sz_, to_add))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (bs->sz_ + to_add <= bs->cap_)
    {
        return CCC_RESULT_OK;
    }
    if (!bs->alloc_)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    ptrdiff_t const new_cap = bs->sz_ ? (bs->sz_ + to_add) * 2 : BLOCK_BITS;
    ccc_bitblock_ *const new_mem = bs->alloc_(
        bs->mem_, blocks(new_cap) * sizeof(ccc_bitblock_), bs->aux_);
    if (!new_mem)
    {
        return CCC_RESULT_MEM_ERR;
    }
    bs->cap_ = new_cap;
    bs->mem_ = new_mem;
    return CCC_RESULT_OK;
}

static ptrdiff_t
first_trailing_one_range(struct ccc_bitset_ const *const bs, ptrdiff_t const i,
                         ptrdiff_t const count)
{
    if (!bs || i < 0 || count < 0 || i >= bs->sz_ || add_overflow(i, count))
    {
        return -1;
    }
    ptrdiff_t const end = i + count;
    if (end > bs->sz_)
    {
        return -1;
    }
    ptrdiff_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i;
    if (start_i + count < BLOCK_BITS)
    {
        first_block_on &= ~(ALL_BITS_ON << (start_i + count));
    }
    ptrdiff_t i_in_block = countr_0(first_block_on & bs->mem_[start_block]);
    if (i_in_block != BLOCK_BITS)
    {
        return (start_block * BLOCK_BITS) + i_in_block;
    }
    ptrdiff_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        return -1;
    }
    /* Handle all values in between start and end in bulk. */
    for (++start_block; start_block < end_block; ++start_block)
    {
        i_in_block = countr_0(bs->mem_[start_block]);
        if (i_in_block != BLOCK_BITS)
        {
            return (start_block * BLOCK_BITS) + i_in_block;
        }
    }
    /* Handle last block. */
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock_ const last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - last_i) - 1);
    i_in_block = countr_0(last_block_on & bs->mem_[end_block]);
    if (i_in_block != BLOCK_BITS)
    {
        return (end_block * BLOCK_BITS) + i_in_block;
    }
    return -1;
}

/* Finds the starting index of a sequence of 1's or 0's of the num_bits size in
   linear time. The algorithm aims to efficiently skip as many bits as possible
   while searching for the desired group. This avoids both an O(N^2) runtime and
   the use of any unnecessary modulo or division operations in a hot loop. */
static ptrdiff_t
first_trailing_bits_range(struct ccc_bitset_ const *const bs, ptrdiff_t const i,
                          ptrdiff_t const count, ptrdiff_t const num_bits,
                          ccc_tribool const is_one)
{
    if (!bs || i < 0 || count < 0 || i >= bs->sz_ || add_overflow(i, count)
        || num_bits <= 0 || num_bits > count)
    {
        return -1;
    }
    ptrdiff_t const range_end = i + count;
    if (range_end > bs->sz_)
    {
        return -1;
    }
    ptrdiff_t num_found = 0;
    ptrdiff_t bits_start = i;
    ptrdiff_t cur_block = set_block_i(i);
    ptrdiff_t cur_end = (cur_block * BLOCK_BITS) + BLOCK_BITS;
    blockwidth_t block_i = blockwidth_i(i);
    do
    {
        /* Clean up some edge cases for the helper function because we allow
           the user to specify any range. What if our range ends before the
           end of this block? What if it starts after index 0 of the first
           block? Pretend out of range bits are zeros. */
        ccc_bitblock_ bits
            = is_one ? bs->mem_[cur_block] & (ALL_BITS_ON << block_i)
                     : (~bs->mem_[cur_block]) & (ALL_BITS_ON << block_i);
        if (cur_end > range_end)
        {
            /* Modulo at most once entire function, not every loop cycle. */
            bits &= ~(ALL_BITS_ON << blockwidth_i(range_end));
        }
        struct group const ones
            = max_trailing_ones(bits, block_i, num_bits - num_found);
        if (ones.count >= num_bits)
        {
            /* Found the solution all at once within a block. */
            return (cur_block * BLOCK_BITS) + ones.block_start_i;
        }
        if (!ones.block_start_i)
        {
            if (num_found + ones.count >= num_bits)
            {
                /* Found solution crossing block boundary from prefix blocks. */
                return bits_start;
            }
            /* Found a full block so keep on trucking. */
            num_found += ones.count;
        }
        else
        {
            /* Fail but we have largest skip possible to continue our search
               from in order to save double checking unnecessary prefixes. */
            bits_start = (cur_block * BLOCK_BITS) + ones.block_start_i;
            num_found = ones.count;
        }
        block_i = 0;
        ++cur_block;
        cur_end += BLOCK_BITS;
    } while (bits_start + num_bits <= range_end);
    return -1;
}

/* Returns the maximum group of consecutive ones in the bitblock given. If the
   number of consecutive ones remaining cannot be found the function returns
   where the next search should start from, a critical step to a linear search;
   specifically, we seek any group of continuous ones that runs from some index
   in the block to the end of the block. If no continuous group of ones exist
   that runs to the end of the block, the BLOCK_BITS index is retuned with a
   group size of 0 meaning the search for ones will need to continue in the
   next block. This is helpful for the main search loop adding to its start
   index and number of ones found so far. */
static struct group
max_trailing_ones(ccc_bitblock_ const b, ptrdiff_t const i_in_block,
                  ptrdiff_t const ones_remaining)
{
    /* Easy exit skip to the next block. Helps with sparse sets. */
    if (!b)
    {
        return (struct group){.block_start_i = BLOCK_BITS};
    }
    if (ones_remaining <= BLOCK_BITS)
    {
        assert(i_in_block < BLOCK_BITS);
        /* This branch must find a smaller group anywhere in this block which is
           the most work required in this algorithm. We have some tricks to tell
           when to give up on this as soon as possible. */
        ccc_bitblock_ b_check = b >> i_in_block;
        ccc_bitblock_ const remain
            = ALL_BITS_ON >> (BLOCK_BITS - ones_remaining);
        /* Because of power of 2 rules we can stop early when the shifted
           becomes impossible to match. */
        for (ptrdiff_t shifts = 0; b_check >= remain; b_check >>= 1, ++shifts)
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
    ptrdiff_t const ones_found = countl_0(~b);
    return (struct group){.block_start_i = BLOCK_BITS - ones_found,
                          .count = ones_found};
}

static ptrdiff_t
first_trailing_zero_range(struct ccc_bitset_ const *const bs, ptrdiff_t const i,
                          ptrdiff_t const count)
{
    if (!bs || i < 0 || count < 0 || i >= bs->sz_ || add_overflow(i, count))
    {
        return -1;
    }
    ptrdiff_t const end = i + count;
    if (end > bs->sz_)
    {
        return -1;
    }
    ptrdiff_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i;
    if (start_i + count < BLOCK_BITS)
    {
        first_block_on &= ~(ALL_BITS_ON << (start_i + count));
    }
    ptrdiff_t i_in_block = countr_0(first_block_on & ~bs->mem_[start_block]);
    if (i_in_block != BLOCK_BITS)
    {
        return (start_block * BLOCK_BITS) + i_in_block;
    }
    ptrdiff_t const end_block = set_block_i(end - 1);
    if (start_block == end_block)
    {
        return -1;
    }
    /* Handle all values in between start and end in bulk. */
    for (++start_block; start_block < end_block; ++start_block)
    {
        i_in_block = countr_0(~bs->mem_[start_block]);
        if (i_in_block != BLOCK_BITS)
        {
            return (start_block * BLOCK_BITS) + i_in_block;
        }
    }
    /* Handle last block. */
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock_ const last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - last_i) - 1);
    i_in_block = countr_0(last_block_on & ~bs->mem_[end_block]);
    if (i_in_block != BLOCK_BITS)
    {
        return (end_block * BLOCK_BITS) + i_in_block;
    }
    return -1;
}

static ptrdiff_t
first_leading_one_range(struct ccc_bitset_ const *const bs, ptrdiff_t const i,
                        ptrdiff_t const count)
{
    if (!bs || i < 0 || count < 0 || i >= bs->sz_ || count > bs->sz_)
    {
        return -1;
    }
    ptrdiff_t const end = (i - count);
    if (end < -1)
    {
        return -1;
    }
    blockwidth_t const final_block_i = blockwidth_i(i - count + 1);
    ptrdiff_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock_ first_block_on = ALL_BITS_ON >> ((BLOCK_BITS - start_i) - 1);
    if (end >= 0 && i - end < BLOCK_BITS)
    {
        first_block_on &= (ALL_BITS_ON << final_block_i);
    }
    ptrdiff_t lead_zeros = countl_0(first_block_on & bs->mem_[start_block]);
    if (lead_zeros != BLOCK_BITS)
    {
        return (start_block * BLOCK_BITS) + (BLOCK_BITS - lead_zeros - 1);
    }
    ptrdiff_t const end_block = set_block_i(end + 1);
    if (end_block == start_block)
    {
        return -1;
    }
    /* Handle all values in between start and end in bulk. */
    for (--start_block; start_block > end_block; --start_block)
    {
        lead_zeros = countl_0(bs->mem_[start_block]);
        if (lead_zeros != BLOCK_BITS)
        {
            return (start_block * BLOCK_BITS) + (BLOCK_BITS - lead_zeros - 1);
        }
    }
    /* Handle last block. */
    blockwidth_t const last_i = blockwidth_i(end + 1);
    ccc_bitblock_ const last_block_on
        = ~(ALL_BITS_ON >> ((BLOCK_BITS - last_i) - 1));
    lead_zeros = countl_0(last_block_on & bs->mem_[end_block]);
    if (lead_zeros != BLOCK_BITS)
    {
        return (end_block * BLOCK_BITS) + (BLOCK_BITS - lead_zeros - 1);
    }
    return -1;
}

static ptrdiff_t
first_leading_bits_range(struct ccc_bitset_ const *const bs, ptrdiff_t const i,
                         ptrdiff_t const count, ptrdiff_t const num_bits,
                         ccc_tribool const is_one)
{
    if (!bs || i < 0 || count < 0 || i >= bs->sz_ || !num_bits
        || num_bits > count)
    {
        return -1;
    }
    ptrdiff_t const range_end = (i - count);
    if (range_end < -1)
    {
        return -1;
    }
    ptrdiff_t num_found = 0;
    ptrdiff_t bits_start = i;
    ptrdiff_t cur_block = set_block_i(i);
    ptrdiff_t cur_end = (cur_block * BLOCK_BITS) - 1;
    blockwidth_t block_i = blockwidth_i(i);
    do
    {
        ccc_bitblock_ bits
            = is_one ? bs->mem_[cur_block]
                           & (ALL_BITS_ON >> ((BLOCK_BITS - block_i) - 1))
                     : ~bs->mem_[cur_block]
                           & (ALL_BITS_ON >> ((BLOCK_BITS - block_i) - 1));
        if (cur_end < range_end)
        {
            bits &= (ALL_BITS_ON << blockwidth_i(range_end + 1));
        }
        struct group const ones
            = max_leading_ones(bits, block_i, (num_bits - num_found));
        if (ones.count >= num_bits)
        {
            return (cur_block * BLOCK_BITS) + ones.block_start_i;
        }
        if (ones.block_start_i == BLOCK_BITS - 1)
        {
            if (num_found + ones.count >= num_bits)
            {
                return bits_start;
            }
            num_found += ones.count;
        }
        else
        {
            bits_start = (cur_block * BLOCK_BITS) + ones.block_start_i;
            num_found = ones.count;
        }
        block_i = BLOCK_BITS - 1;
        --cur_block;
        cur_end -= BLOCK_BITS;
    } while (bits_start - num_bits >= range_end);
    return -1;
}

static struct group
max_leading_ones(ccc_bitblock_ const b, ptrdiff_t const i_in_block,
                 ptrdiff_t const ones_remaining)
{
    if (!b)
    {
        return (struct group){.block_start_i = -1};
    }
    if (ones_remaining <= BLOCK_BITS)
    {
        assert(i_in_block < BLOCK_BITS);
        ccc_bitblock_ b_check = b << (BLOCK_BITS - i_in_block - 1);
        ccc_bitblock_ const required = ALL_BITS_ON
                                       << (BLOCK_BITS - ones_remaining);
        for (ptrdiff_t shifts = 0; b_check; b_check <<= 1, ++shifts)
        {
            if ((required & b_check) == required)
            {
                return (struct group){.block_start_i = i_in_block - shifts,
                                      .count = ones_remaining};
            }
        }
    }
    ptrdiff_t const num_ones_found = countr_0(~b);
    return (struct group){.block_start_i = num_ones_found - 1,
                          .count = num_ones_found};
}

static ptrdiff_t
first_leading_zero_range(struct ccc_bitset_ const *const bs, ptrdiff_t const i,
                         ptrdiff_t const count)
{
    if (!bs || i < 0 || count < 0 || i >= bs->sz_ || count > bs->sz_)
    {
        return -1;
    }
    ptrdiff_t const end = (i - count);
    if (end < -1)
    {
        return -1;
    }
    ptrdiff_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock_ first_block_on = ALL_BITS_ON >> ((BLOCK_BITS - start_i) - 1);
    if (end >= 0 && i - end < BLOCK_BITS)
    {
        first_block_on &= ~(ALL_BITS_ON >> (i - end));
    }
    ptrdiff_t lead_ones = countl_0(first_block_on & ~bs->mem_[start_block]);
    if (lead_ones != BLOCK_BITS)
    {
        return ((start_block * BLOCK_BITS) + (BLOCK_BITS - lead_ones - 1));
    }
    ptrdiff_t const end_block = set_block_i(end + 1);
    if (end_block == start_block)
    {
        return -1;
    }
    /* Handle all values in between start and end in bulk. */
    for (--start_block; start_block > end_block; --start_block)
    {
        lead_ones = countl_0(~bs->mem_[start_block]);
        if (lead_ones != BLOCK_BITS)
        {
            return ((start_block * BLOCK_BITS) + (BLOCK_BITS - lead_ones - 1));
        }
    }
    /* Handle last block. */
    blockwidth_t const last_i = blockwidth_i(end + 1);
    ccc_bitblock_ const last_block_on
        = ~(ALL_BITS_ON >> ((BLOCK_BITS - last_i) - 1));
    lead_ones = countl_0(last_block_on & ~bs->mem_[end_block]);
    if (lead_ones != BLOCK_BITS)
    {
        return ((end_block * BLOCK_BITS) + (BLOCK_BITS - lead_ones - 1));
    }
    return -1;
}

/* Performs the any or none scan operation over the specified range. The only
   difference between the operations is the return value. Specify the desired
   tribool value to return upon encountering an on bit. For any this is
   CCC_TRUE. For none this is CCC_FALSE. Saves writing two identical fns. */
static ccc_tribool
any_or_none_range(struct ccc_bitset_ const *const bs, ptrdiff_t const i,
                  ptrdiff_t const count, ccc_tribool const ret)
{
    if (!bs || i < 0 || count < 0 || i >= bs->sz_ || ret < CCC_FALSE
        || ret > CCC_TRUE || add_overflow(i, count))
    {
        return CCC_BOOL_ERR;
    }
    ptrdiff_t const end = i + count;
    if (end > bs->sz_)
    {
        return CCC_BOOL_ERR;
    }
    ptrdiff_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i;
    if (start_i + count < BLOCK_BITS)
    {
        first_block_on &= ~(ALL_BITS_ON << (start_i + count));
    }
    if (first_block_on & bs->mem_[start_block])
    {
        return ret;
    }
    ptrdiff_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        return !ret;
    }
    /* If this is the any check we might get lucky by checking the last block
       before looping over everything. */
    blockwidth_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock_ const last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - last_i) - 1);
    if (last_block_on & bs->mem_[end_block])
    {
        return ret;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        if (bs->mem_[start_block] & ALL_BITS_ON)
        {
            return ret;
        }
    }
    return !ret;
}

/* Check for all on is slightly different from the any or none checks so we
   need a painfully repetitive function. */
static ccc_tribool
all_range(struct ccc_bitset_ const *const bs, ptrdiff_t const i,
          ptrdiff_t const count)
{
    if (!bs || i < 0 || count < 0 || i >= bs->sz_ || add_overflow(i, count))
    {
        return CCC_BOOL_ERR;
    }
    ptrdiff_t const end = i + count;
    if (end > bs->sz_)
    {
        return CCC_BOOL_ERR;
    }
    ptrdiff_t start_block = set_block_i(i);
    blockwidth_t const start_i = blockwidth_i(i);
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i;
    if (start_i + count < BLOCK_BITS)
    {
        first_block_on &= ~(ALL_BITS_ON << (start_i + count));
    }
    if ((first_block_on & bs->mem_[start_block]) != first_block_on)
    {
        return CCC_FALSE;
    }
    ptrdiff_t const end_block = set_block_i(end - 1);
    if (end_block == start_block)
    {
        return CCC_TRUE;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        if (bs->mem_[start_block] != ALL_BITS_ON)
        {
            return CCC_FALSE;
        }
    }
    ptrdiff_t const last_i = blockwidth_i(end - 1);
    ccc_bitblock_ const last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - last_i) - 1);
    if ((last_block_on & bs->mem_[end_block]) != last_block_on)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

static inline void
set_all(struct ccc_bitset_ *const bs, ccc_tribool const b)
{
    (void)memset(bs->mem_, b ? ~0 : 0, blocks(bs->sz_) * sizeof(ccc_bitblock_));
    bs->mem_[set_block_i(bs->sz_ - 1)] &= last_on(bs);
}

/* Given a reference to the correct block in the set, the true set index (may be
   greater than size of a block), and the value to set, sets the index in the
   block to the given tribool value. */
static inline void
set(ccc_bitblock_ *const block, ptrdiff_t const bit_i, ccc_tribool const b)
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

/* Given the bit set and the set index--set index is allowed to be greater than
   the size of one block--returns the status of the bit at that index. */
static inline ccc_tribool
status(ccc_bitblock_ const *const bs, ptrdiff_t const bit_i)
{
    /* Be careful. Bitwise & does not promise to evaluate to 1 or 0. We often
       just use it where that conversion takes place implicitly for us. */
    return (*bs & on(bit_i)) != 0;
}

/* Return a block with only the desired bit turned on to true. */
static inline ccc_bitblock_
on(ptrdiff_t bit_i)
{
    return (ccc_bitblock_)1 << blockwidth_i(bit_i);
}

/* Returns a mask of all bits on in the final bit block that represent only
   those bits which are in use according to the bit set capacity. The remaining
   higher order bits in the last block will be set to 0 because they are not
   used. If the capacity is zero a block with all bits on is returned. */
static inline ccc_bitblock_
last_on(struct ccc_bitset_ const *const bs)
{
    /* Remember, we fill from LSB to MSB so we want the mask to start at lower
       order bit which is why we do the second funky flip on the whole op. */
    return bs->sz_ ? ~(((ccc_bitblock_)~1) << blockwidth_i(bs->sz_ - 1))
                   : ALL_BITS_ON;
}

static inline ptrdiff_t
set_block_i(ptrdiff_t const bit_i)
{
    return bit_i / BLOCK_BITS;
}

static inline blockwidth_t
blockwidth_i(ptrdiff_t const bit_i)
{
    return bit_i % BLOCK_BITS;
}

static inline ptrdiff_t
blocks(ptrdiff_t const bits)
{
    return (bits + (BLOCK_BITS - 1)) / BLOCK_BITS;
}

static inline ptrdiff_t
min(ptrdiff_t a, ptrdiff_t b)
{
    return a < b ? a : b;
}

static inline ccc_tribool
add_overflow(ptrdiff_t const a, ptrdiff_t const b)
{
    return PTRDIFF_MAX - b < a;
}

#if defined(__GNUC__) || defined(__clang__)

static inline ptrdiff_t
popcount(ccc_bitblock_ const b)
{
    /* There are different pop counts for different integer widths. Be sure to
       catch the use of the wrong one by mistake here at compile time. */
    static_assert(sizeof(ccc_bitblock_) == sizeof(unsigned));
    return __builtin_popcount(b);
}

static inline ptrdiff_t
countr_0(ccc_bitblock_ const b)
{
    static_assert(BITBLOCK_MSB < ALL_BITS_ON);
    static_assert(sizeof(ccc_bitblock_) == sizeof(unsigned));
    return b ? __builtin_ctz(b) : BLOCK_BITS;
}

static inline ptrdiff_t
countl_0(ccc_bitblock_ const b)
{
    static_assert(BITBLOCK_MSB < ALL_BITS_ON);
    static_assert(sizeof(ccc_bitblock_) == sizeof(unsigned));
    return b ? __builtin_clz(b) : BLOCK_BITS;
}

#else /* !defined(__GNUC__) && !defined(__clang__) */

static inline unsigned
popcount(ccc_bitblock_ const b)
{
    unsigned cnt = 0;
    for (; b; cnt += !!(b & 1U), b >>= 1U)
    {}
    return cnt;
}

static inline ptrdiff_t
countr_0(ccc_bitblock_ const b)
{
    if (!b)
    {
        return BLOCK_BITS;
    }
    ptrdiff_t cnt = 0;
    for (; !(b & 1U); ++cnt, b >>= 1U)
    {}
    return cnt;
}

static inline ptrdiff_t
countl_0(ccc_bitblock_ const b)
{
    if (!b)
    {
        return BLOCK_BITS;
    }
    ptrdiff_t cnt = 0;
    for (; !(b & BITBLOCK_MSB); ++cnt, b <<= 1U)
    {}
    return cnt;
}

#endif /* defined(__GNUC__) || defined(__clang__) */
