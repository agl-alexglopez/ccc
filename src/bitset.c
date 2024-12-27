#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "bitset.h"
#include "impl/impl_bitset.h"
#include "types.h"

/* How many total bits that fit in a ccc_bitblock_. */
#define BLOCK_BITS (sizeof(ccc_bitblock_) * CHAR_BIT)
/* A mask of a ccc_bitblock_ with all bits on. */
#define ALL_BITS_ON ((ccc_bitblock_)~0)
/* The Most Significant Bit of a ccc_bitblock_ turned on to 1. */
#define BITBLOCK_MSB                                                           \
    (((ccc_bitblock_)1) << (((sizeof(ccc_bitblock_) * CHAR_BIT)) - 1))

struct group
{
    size_t block_start_i;
    size_t count;
};

/*=========================   Prototypes   ==================================*/

static size_t block_i(size_t bit_i);
static void set(ccc_bitblock_ *, size_t bit_i, ccc_tribool);
static ccc_bitblock_ on(size_t bit_i);
static ccc_bitblock_ last_on(struct ccc_bitset_ const *);
static ccc_tribool status(ccc_bitblock_ const *, size_t bit_i);
static size_t blocks(size_t bits);
static unsigned popcount(ccc_bitblock_);
static ccc_tribool any_or_none_range(struct ccc_bitset_ const *, size_t i,
                                     size_t count, ccc_tribool);
static ccc_tribool all_range(struct ccc_bitset_ const *bs, size_t i,
                             size_t count);
static ptrdiff_t first_trailing_one_range(struct ccc_bitset_ const *bs,
                                          size_t i, size_t count);
static ptrdiff_t first_trailing_zero_range(struct ccc_bitset_ const *bs,
                                           size_t i, size_t count);
static ptrdiff_t first_leading_one_range(struct ccc_bitset_ const *bs, size_t i,
                                         size_t count);
static ptrdiff_t first_leading_zero_range(struct ccc_bitset_ const *bs,
                                          size_t i, size_t count);
static ptrdiff_t countr_0(ccc_bitblock_);
static ptrdiff_t countl_0(ccc_bitblock_);
static ptrdiff_t first_trailing_ones_range(struct ccc_bitset_ const *bs,
                                           size_t i, size_t count,
                                           size_t num_ones);
static ptrdiff_t first_leading_ones_range(struct ccc_bitset_ const *bs,
                                          size_t i, size_t count,
                                          size_t num_ones);
static struct group max_trailing_ones(ccc_bitblock_ b, size_t i_in_block,
                                      size_t num_ones_remaining);
static struct group max_leading_ones(ccc_bitblock_ b, ptrdiff_t i_in_block,
                                     ptrdiff_t num_ones_remaining);

/*=======================   Public Interface   ==============================*/

ccc_tribool
ccc_bs_test(ccc_bitset const *const bs, size_t const i)
{
    if (!bs)
    {
        return CCC_BOOL_ERR;
    }
    return status(&bs->set_[block_i(i)], i);
}

ccc_tribool
ccc_bs_test_at(ccc_bitset const *const bs, size_t const i)
{
    size_t const b_i = block_i(i);
    if (!bs || b_i >= bs->cap_)
    {
        return CCC_BOOL_ERR;
    }
    return status(&bs->set_[b_i], i);
}

ccc_tribool
ccc_bs_set(ccc_bitset *const bs, size_t const i, ccc_tribool const b)
{
    if (!bs)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &bs->set_[block_i(i)];
    ccc_tribool const was = status(block, i);
    set(block, i, b);
    return was;
}

ccc_tribool
ccc_bs_set_at(ccc_bitset *const bs, size_t const i, ccc_tribool const b)
{
    size_t const b_i = block_i(i);
    if (!bs || b_i >= bs->cap_)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &bs->set_[b_i];
    ccc_tribool const was = status(block, i);
    set(block, i, b);
    return was;
}

ccc_result
ccc_bs_set_all(ccc_bitset *const bs, ccc_tribool const b)
{
    if (!bs || b <= CCC_BOOL_ERR || b > CCC_TRUE)
    {
        return CCC_INPUT_ERR;
    }
    if (bs->cap_)
    {
        int const v = b ? ~0 : 0;
        (void)memset(bs->set_, v, blocks(bs->cap_) * sizeof(ccc_bitblock_));
        bs->set_[block_i(bs->cap_ - 1)] &= last_on(bs);
    }
    return CCC_OK;
}

/* A naive implementation might just call set for every index between the
   start and start + count. However, calculating the block and index within
   each block for every call to set costs a division and a modulo operation. We
   can avoid this by handling the first and last block and then handling
   everything in between with a bulk memset. */
ccc_result
ccc_bs_set_range(ccc_bitset *const bs, size_t const i, size_t const count,
                 ccc_tribool const b)
{
    size_t const end = i + count;
    if (!bs || b <= CCC_BOOL_ERR || b > CCC_TRUE || i >= bs->cap_
        || end > bs->cap_ || end < i)
    {
        return CCC_INPUT_ERR;
    }
    size_t start_block = block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on
            &= (ALL_BITS_ON >> (BLOCK_BITS - (start_i_in_block + count)));
    }
    if (b)
    {
        bs->set_[start_block] |= first_block_on;
    }
    else
    {
        bs->set_[start_block] &= ~first_block_on;
    }
    size_t const end_block = block_i(end - 1);
    if (end_block == start_block)
    {
        bs->set_[block_i(bs->cap_ - 1)] &= last_on(bs);
        return CCC_OK;
    }
    if (++start_block != end_block)
    {
        int const v = b ? ~0 : 0;
        (void)memset(&bs->set_[start_block], v,
                     (end_block - start_block) * sizeof(ccc_bitblock_));
    }
    size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
    if (b)
    {
        bs->set_[end_block] |= last_block_on;
    }
    else
    {
        bs->set_[end_block] &= ~last_block_on;
    }
    bs->set_[block_i(bs->cap_ - 1)] &= last_on(bs);
    return CCC_OK;
}

ccc_tribool
ccc_bs_reset(ccc_bitset *const bs, size_t const i)
{
    if (!bs)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &bs->set_[block_i(i)];
    ccc_tribool const was = status(block, i);
    *block &= ~on(i);
    return was;
}

ccc_tribool
ccc_bs_reset_at(ccc_bitset *const bs, size_t const i)
{
    size_t const b_i = block_i(i);
    if (!bs || b_i >= bs->cap_)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &bs->set_[b_i];
    ccc_tribool const was = status(block, i);
    *block &= ~on(i);
    return was;
}

ccc_result
ccc_bs_reset_all(ccc_bitset *const bs)
{
    if (!bs)
    {
        return CCC_INPUT_ERR;
    }
    if (bs->cap_)
    {
        (void)memset(bs->set_, CCC_FALSE,
                     blocks(bs->cap_) * sizeof(ccc_bitblock_));
    }
    return CCC_OK;
}

/* Same concept as set range but easier. Handle first and last then set
   everything in between to false with memset. */
ccc_result
ccc_bs_reset_range(ccc_bitset *const bs, size_t const i, size_t const count)
{
    size_t const end = i + count;
    if (!bs || i >= bs->cap_ || end > bs->cap_ || end < i)
    {
        return CCC_INPUT_ERR;
    }
    size_t start_block = block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on
            &= (ALL_BITS_ON >> (BLOCK_BITS - (start_i_in_block + count)));
    }
    bs->set_[start_block] &= ~first_block_on;
    size_t const end_block = block_i(end - 1);
    if (end_block == start_block)
    {
        bs->set_[block_i(bs->cap_ - 1)] &= last_on(bs);
        return CCC_OK;
    }
    if (++start_block != end_block)
    {
        (void)memset(&bs->set_[start_block], CCC_FALSE,
                     (end_block - start_block) * sizeof(ccc_bitblock_));
    }
    size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
    bs->set_[end_block] &= ~last_block_on;
    bs->set_[block_i(bs->cap_ - 1)] &= last_on(bs);
    return CCC_OK;
}

ccc_tribool
ccc_bs_flip(ccc_bitset *const bs, size_t const i)
{
    if (!bs)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &bs->set_[block_i(i)];
    ccc_tribool const was = status(block, i);
    *block ^= on(i);
    return was;
}

ccc_tribool
ccc_bs_flip_at(ccc_bitset *const bs, size_t const i)
{
    size_t const b_i = block_i(i);
    if (!bs || b_i >= bs->cap_)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &bs->set_[b_i];
    ccc_tribool const was = status(block, i);
    *block ^= on(i);
    return was;
}

ccc_result
ccc_bs_flip_all(ccc_bitset *const bs)
{
    if (!bs)
    {
        return CCC_INPUT_ERR;
    }
    if (!bs->cap_)
    {
        return CCC_OK;
    }
    size_t const end = blocks(bs->cap_);
    for (size_t i = 0; i < end; ++i)
    {
        bs->set_[i] = ~bs->set_[i];
    }
    bs->set_[end - 1] &= last_on(bs);
    return CCC_OK;
}

/* Maybe future SIMD vectorization could speed things up here because we use
   the same strat of handling first and last which just leaves a simpler bulk
   operation in the middle. But we don't benefit from memset here. */
ccc_result
ccc_bs_flip_range(ccc_bitset *const bs, size_t const i, size_t const count)
{
    size_t const end = i + count;
    if (!bs || i >= bs->cap_ || end > bs->cap_ || end < i)
    {
        return CCC_INPUT_ERR;
    }
    size_t start_block = block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on
            &= (ALL_BITS_ON >> (BLOCK_BITS - (start_i_in_block + count)));
    }
    bs->set_[start_block] ^= first_block_on;
    size_t const end_block = block_i(end - 1);
    if (end_block == start_block)
    {
        bs->set_[block_i(bs->cap_ - 1)] &= last_on(bs);
        return CCC_OK;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        bs->set_[start_block] = ~bs->set_[start_block];
    }
    size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
    bs->set_[end_block] ^= last_block_on;
    bs->set_[block_i(bs->cap_ - 1)] &= last_on(bs);
    return CCC_OK;
}

size_t
ccc_bs_capacity(ccc_bitset const *const bs)
{
    if (!bs)
    {
        return 0;
    }
    return bs->cap_;
}

size_t
ccc_bs_popcount(ccc_bitset const *const bs)
{
    if (!bs || !bs->cap_)
    {
        return 0;
    }
    size_t const end = blocks(bs->cap_);
    size_t cnt = 0;
    for (size_t i = 0; i < end; cnt += popcount(bs->set_[i++]))
    {}
    return cnt;
}

ptrdiff_t
ccc_bs_popcount_range(ccc_bitset const *const bs, size_t const i,
                      size_t const count)
{
    size_t const end = i + count;
    if (!bs || i >= bs->cap_ || end > bs->cap_ || end < i)
    {
        return -1;
    }
    ptrdiff_t popped = 0;
    size_t start_block = block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on
            &= (ALL_BITS_ON >> (BLOCK_BITS - (start_i_in_block + count)));
    }
    popped += popcount(first_block_on & bs->set_[start_block]);
    size_t const end_block = block_i(end - 1);
    if (end_block == start_block)
    {
        return popped;
    }
    for (++start_block; start_block < end_block;
         popped += popcount(bs->set_[start_block++]))
    {}
    size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
    popped += popcount(last_block_on & bs->set_[end_block]);
    return popped;
}

ccc_tribool
ccc_bs_any_range(ccc_bitset const *const bs, size_t const i, size_t const count)
{
    return any_or_none_range(bs, i, count, CCC_TRUE);
}

ccc_tribool
ccc_bs_any(ccc_bitset const *const bs)
{
    return any_or_none_range(bs, 0, bs->cap_, CCC_TRUE);
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
    return any_or_none_range(bs, 0, bs->cap_, CCC_FALSE);
}

ccc_tribool
ccc_bs_all_range(ccc_bitset const *const bs, size_t const i, size_t const count)
{
    return all_range(bs, i, count);
}

ccc_tribool
ccc_bs_all(ccc_bitset const *const bs)
{
    return all_range(bs, 0, bs->cap_);
}

ptrdiff_t
ccc_bs_first_trailing_one_range(ccc_bitset const *const bs, size_t const i,
                                size_t const count)
{
    return first_trailing_one_range(bs, i, count);
}

ptrdiff_t
ccc_bs_first_trailing_one(ccc_bitset const *const bs)
{
    return first_trailing_one_range(bs, 0, bs->cap_);
}

ptrdiff_t
ccc_bs_first_trailing_ones(ccc_bitset const *const bs, size_t const num_ones)
{
    return first_trailing_ones_range(bs, 0, bs->cap_, num_ones);
}

ptrdiff_t
ccc_bs_first_trailing_ones_range(ccc_bitset const *const bs, size_t const i,
                                 size_t const count, size_t const num_ones)
{
    return first_trailing_ones_range(bs, i, count, num_ones);
}

ptrdiff_t
ccc_bs_first_trailing_zero_range(ccc_bitset const *const bs, size_t const i,
                                 size_t const count)
{
    return first_trailing_zero_range(bs, i, count);
}

ptrdiff_t
ccc_bs_first_trailing_zero(ccc_bitset const *const bs)
{
    return first_trailing_zero_range(bs, 0, bs->cap_);
}

ptrdiff_t
ccc_bs_first_leading_one_range(ccc_bitset const *const bs, size_t const i,
                               size_t const count)
{
    return first_leading_one_range(bs, i, count);
}

ptrdiff_t
ccc_bs_first_leading_one(ccc_bitset const *const bs)
{
    return first_leading_one_range(bs, bs->cap_ - 1, bs->cap_);
}

ptrdiff_t
ccc_bs_first_leading_ones(ccc_bitset const *const bs, size_t const num_ones)
{
    return first_leading_ones_range(bs, bs->cap_ - 1, bs->cap_, num_ones);
}

ptrdiff_t
ccc_bs_first_leading_ones_range(ccc_bitset const *const bs, size_t const i,
                                size_t const count, size_t const num_ones)
{
    return first_leading_ones_range(bs, i, count, num_ones);
}

ptrdiff_t
ccc_bs_first_leading_zero_range(ccc_bitset const *const bs, size_t const i,
                                size_t const count)
{
    return first_leading_zero_range(bs, i, count);
}

ptrdiff_t
ccc_bs_first_leading_zero(ccc_bitset const *const bs)
{
    return first_leading_zero_range(bs, bs->cap_ - 1, bs->cap_);
}

/*=======================    Static Helpers    ==============================*/

static inline ptrdiff_t
first_trailing_one_range(struct ccc_bitset_ const *const bs, size_t const i,
                         size_t const count)
{
    size_t const end = i + count;
    if (!bs || i >= bs->cap_ || end > bs->cap_ || end < i)
    {
        return -1;
    }
    size_t start_block = block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on
            &= (ALL_BITS_ON >> (BLOCK_BITS - (start_i_in_block + count)));
    }
    ptrdiff_t i_in_block = countr_0(first_block_on & bs->set_[start_block]);
    if (i_in_block != BLOCK_BITS)
    {
        return (ptrdiff_t)((start_block * BLOCK_BITS) + i_in_block);
    }
    size_t const end_block = block_i(end - 1);
    if (end_block == start_block)
    {
        return -1;
    }
    /* Handle all values in between start and end in bulk. */
    for (++start_block; start_block < end_block; ++start_block)
    {
        i_in_block = countr_0(bs->set_[start_block]);
        if (i_in_block != BLOCK_BITS)
        {
            return (ptrdiff_t)((start_block * BLOCK_BITS) + i_in_block);
        }
    }
    /* Handle last block. */
    size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
    i_in_block = countr_0(last_block_on & bs->set_[end_block]);
    if (i_in_block != BLOCK_BITS)
    {
        return (ptrdiff_t)((end_block * BLOCK_BITS) + i_in_block);
    }
    return -1;
}

/* Finds the starting index of a sequence of 1's of the num_ones size in linear
   time. The algorithm aims to efficiently skip as many bits as possible while
   searching for the desired group. This avoids both an O(N^2) runtime and the
   use of any unnecessary modulo or division operations in a hot loop. */
static inline ptrdiff_t
first_trailing_ones_range(struct ccc_bitset_ const *const bs, size_t const i,
                          size_t const count, size_t const num_ones)
{
    size_t const range_end = i + count;
    if (!bs || i >= bs->cap_ || range_end > bs->cap_ || range_end < i
        || !num_ones || num_ones > count)
    {
        return -1;
    }
    size_t num_found = 0;
    size_t ones_start = i;
    size_t cur_block = block_i(i);
    size_t cur_end = (cur_block * BLOCK_BITS) + BLOCK_BITS;
    size_t block_i = i % BLOCK_BITS;
    while (ones_start + num_ones <= range_end)
    {
        /* Clean up some edge cases for the helper function because we allow
           the user to specify any range. What if our range ends before the
           end of this block? What if it starts after index 0 of the first
           block? Pretend out of range bits don't exist. */
        ccc_bitblock_ bits = bs->set_[cur_block] & (ALL_BITS_ON << block_i);
        if (cur_end > range_end)
        {
            bits &= (ALL_BITS_ON >> (cur_end - range_end));
        }
        struct group const ones
            = max_trailing_ones(bits, block_i, num_ones - num_found);
        if (ones.count >= num_ones)
        {
            /* Found the solution all at once within a block. */
            return (ptrdiff_t)((cur_block * BLOCK_BITS) + ones.block_start_i);
        }
        if (!ones.block_start_i)
        {
            if (num_found + ones.count >= num_ones)
            {
                /* Found solution crossing block boundary from prefix blocks. */
                return (ptrdiff_t)ones_start;
            }
            /* Found a full block so keep on trucking. */
            num_found += ones.count;
        }
        else
        {
            /* Fail but we have largest skip possible to continue our search
               from in order to save double checking unnecessary prefixes. */
            ones_start
                = (cur_block * BLOCK_BITS) + (ptrdiff_t)ones.block_start_i;
            num_found = ones.count;
        }
        block_i = 0;
        ++cur_block;
        cur_end += BLOCK_BITS;
    }
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
static inline struct group
max_trailing_ones(ccc_bitblock_ b, size_t const i_in_block,
                  size_t const ones_remaining)
{
    /* Easy exit skip to the next block. Helps with sparse sets. */
    if (!b)
    {
        return (struct group){.block_start_i = BLOCK_BITS};
    }
    if (ones_remaining > (ptrdiff_t)BLOCK_BITS)
    {
        /* The best we could do is find that we have a block of all 1's, which
           means we have reduced the search by one block worth. If we don't
           have a full block we would then need to find the next continuous
           sequence of 1's to start the search at. Counting zeros from the left
           on the inverted block achieves both of these cases in one. */
        ptrdiff_t const leading_ones = countl_0(~b);
        return (struct group){.block_start_i = BLOCK_BITS - leading_ones,
                              .count = leading_ones};
    }
    /* This branch must find a smaller group anywhere in this block which is
       the most work required in this algorithm. We have some tricks to tell
       when to give up on this as soon as possible. */
    ccc_bitblock_ shifted = b >> i_in_block;
    ccc_bitblock_ const required_ones
        = ALL_BITS_ON >> (BLOCK_BITS - ones_remaining);
    /* Because of power of 2 rules we can stop early when the shifted becomes
       impossible to match. */
    for (size_t shifts = 0; shifted >= required_ones; shifted >>= 1, ++shifts)
    {
        if ((required_ones & shifted) == required_ones)
        {
            return (struct group){.block_start_i = i_in_block + shifts,
                                  .count = ones_remaining};
        }
    }
    /* Most important step. If we can't satisfy ones remaining, this function
       must give the main search loop the correct position to start a new
       search from so it can reset its starting index and current group size
       correctly. */
    ptrdiff_t const num_ones_found = countl_0(~b);
    return (struct group){.block_start_i = BLOCK_BITS - num_ones_found,
                          .count = num_ones_found};
}

static inline ptrdiff_t
first_trailing_zero_range(struct ccc_bitset_ const *const bs, size_t const i,
                          size_t const count)
{
    size_t const end = i + count;
    if (!bs || i >= bs->cap_ || end > bs->cap_ || end < i)
    {
        return -1;
    }
    size_t start_block = block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on
            &= (ALL_BITS_ON >> (BLOCK_BITS - (start_i_in_block + count)));
    }
    ptrdiff_t i_in_block = countr_0(first_block_on & ~bs->set_[start_block]);
    if (i_in_block != BLOCK_BITS)
    {
        return ((ptrdiff_t)(start_block * BLOCK_BITS)) + i_in_block;
    }
    size_t const end_block = block_i(end - 1);
    if (start_block == end_block)
    {
        return -1;
    }
    /* Handle all values in between start and end in bulk. */
    for (++start_block; start_block < end_block; ++start_block)
    {
        i_in_block = countr_0(~bs->set_[start_block]);
        if (i_in_block != BLOCK_BITS)
        {
            return ((ptrdiff_t)(start_block * BLOCK_BITS)) + i_in_block;
        }
    }
    /* Handle last block. */
    size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
    i_in_block = countr_0(last_block_on & ~bs->set_[end_block]);
    if (i_in_block != BLOCK_BITS)
    {
        return ((ptrdiff_t)(end_block * BLOCK_BITS)) + i_in_block;
    }
    return -1;
}

static inline ptrdiff_t
first_leading_one_range(struct ccc_bitset_ const *const bs, size_t const i,
                        size_t const count)
{
    if (!bs || i >= bs->cap_ || count > bs->cap_ || (ptrdiff_t)(i - count) < -1)
    {
        return -1;
    }
    ptrdiff_t const end = (ptrdiff_t)(i - count);
    ptrdiff_t const final_block_i = (ptrdiff_t)((i - count + 1) % BLOCK_BITS);
    ptrdiff_t start_block = (ptrdiff_t)block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - start_i_in_block) - 1);
    if (end >= 0 && i - end < BLOCK_BITS)
    {
        first_block_on &= (ALL_BITS_ON << final_block_i);
    }
    ptrdiff_t lead_zeros = countl_0(first_block_on & bs->set_[start_block]);
    if (lead_zeros != BLOCK_BITS)
    {
        return (ptrdiff_t)((start_block * BLOCK_BITS)
                           + (BLOCK_BITS - lead_zeros - 1));
    }
    ptrdiff_t const end_block = (ptrdiff_t)block_i(end + 1);
    if (end_block == start_block)
    {
        return -1;
    }
    /* Handle all values in between start and end in bulk. */
    for (--start_block; start_block > end_block; --start_block)
    {
        lead_zeros = countl_0(bs->set_[start_block]);
        if (lead_zeros != BLOCK_BITS)
        {
            return (ptrdiff_t)((start_block * BLOCK_BITS)
                               + (BLOCK_BITS - lead_zeros - 1));
        }
    }
    /* Handle last block. */
    size_t const end_i_in_block = (end + 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on
        = ~(ALL_BITS_ON >> ((BLOCK_BITS - end_i_in_block) - 1));
    lead_zeros = countl_0(last_block_on & bs->set_[end_block]);
    if (lead_zeros != BLOCK_BITS)
    {
        return (ptrdiff_t)((end_block * BLOCK_BITS)
                           + (BLOCK_BITS - lead_zeros - 1));
    }
    return -1;
}

/* Finds the starting index of a sequence of 1's of the num_ones size in linear
   time. The algorithm aims to efficiently skip as many bits as possible while
   searching for the desired group. This avoids both an O(N^2) runtime and the
   use of any unnecessary modulo or division operations in a hot loop. */
static inline ptrdiff_t
first_leading_ones_range(struct ccc_bitset_ const *const bs, size_t const i,
                         size_t const count, size_t const num_ones)
{
    ptrdiff_t const range_end = (ptrdiff_t)(i - count);
    if (!bs || i >= bs->cap_ || range_end > (ptrdiff_t)bs->cap_
        || range_end < -1 || !num_ones || num_ones > count)
    {
        return -1;
    }
    ptrdiff_t const final_block_i = (ptrdiff_t)((range_end + 1) % BLOCK_BITS);
    size_t num_found = 0;
    ptrdiff_t ones_start = (ptrdiff_t)i;
    ptrdiff_t cur_block = (ptrdiff_t)block_i(i);
    ptrdiff_t cur_end = (ptrdiff_t)((cur_block * BLOCK_BITS) - 1);
    ptrdiff_t block_i = (ptrdiff_t)(i % BLOCK_BITS);
    while (ones_start - (ptrdiff_t)num_ones >= range_end)
    {
        ccc_bitblock_ bits = bs->set_[cur_block]
                             & (ALL_BITS_ON >> ((BLOCK_BITS - block_i) - 1));
        if (cur_end < range_end)
        {
            assert(final_block_i < (ptrdiff_t)BLOCK_BITS);
            bits &= (ALL_BITS_ON << final_block_i);
        }
        struct group const ones = max_leading_ones(
            bits, block_i, (ptrdiff_t)(num_ones - num_found));
        if (ones.count >= num_ones)
        {
            return (ptrdiff_t)((cur_block * BLOCK_BITS) + ones.block_start_i);
        }
        if (ones.block_start_i == BLOCK_BITS - 1)
        {
            if (num_found + ones.count >= num_ones)
            {
                return ones_start;
            }
            num_found += ones.count;
        }
        else
        {
            ones_start
                = (ptrdiff_t)((cur_block * BLOCK_BITS) + ones.block_start_i);
            num_found = ones.count;
        }
        block_i = BLOCK_BITS - 1;
        --cur_block;
        cur_end -= BLOCK_BITS;
    }
    return -1;
}

static inline struct group
max_leading_ones(ccc_bitblock_ b, ptrdiff_t const i_in_block,
                 ptrdiff_t const ones_remaining)
{
    if (!b)
    {
        return (struct group){.block_start_i = -1};
    }
    if (ones_remaining > (ptrdiff_t)BLOCK_BITS)
    {
        ptrdiff_t const trailing_ones = countr_0(~b);
        return (struct group){.block_start_i = trailing_ones - 1,
                              .count = trailing_ones};
    }
    ccc_bitblock_ shifted = b << (BLOCK_BITS - i_in_block - 1);
    ccc_bitblock_ const required_ones = ALL_BITS_ON
                                        << (BLOCK_BITS - ones_remaining);
    for (size_t shifts = 0; shifted; shifted <<= 1, ++shifts)
    {
        if ((required_ones & shifted) == required_ones)
        {
            return (struct group){.block_start_i = i_in_block - shifts,
                                  .count = ones_remaining};
        }
    }
    ptrdiff_t const num_ones_found = countr_0(~b);
    return (struct group){.block_start_i = num_ones_found - 1,
                          .count = num_ones_found};
}

static inline ptrdiff_t
first_leading_zero_range(struct ccc_bitset_ const *const bs, size_t const i,
                         size_t const count)
{
    if (!bs || i >= bs->cap_ || count > bs->cap_ || (ptrdiff_t)(i - count) < -1)
    {
        return -1;
    }
    ptrdiff_t const end = (ptrdiff_t)(i - count);
    ptrdiff_t start_block = (ptrdiff_t)block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - start_i_in_block) - 1);
    if (end >= 0 && i - end < BLOCK_BITS)
    {
        first_block_on &= (ALL_BITS_ON << (BLOCK_BITS - (i - end)));
    }
    ptrdiff_t lead_ones = countl_0(first_block_on & ~bs->set_[start_block]);
    if (lead_ones != BLOCK_BITS)
    {
        return (ptrdiff_t)((start_block * BLOCK_BITS)
                           + (BLOCK_BITS - lead_ones - 1));
    }
    ptrdiff_t const end_block = (ptrdiff_t)block_i(end + 1);
    if (end_block == start_block)
    {
        return -1;
    }
    /* Handle all values in between start and end in bulk. */
    for (--start_block; start_block > end_block; --start_block)
    {
        lead_ones = countl_0(~bs->set_[start_block]);
        if (lead_ones != BLOCK_BITS)
        {
            return (ptrdiff_t)((start_block * BLOCK_BITS)
                               + (BLOCK_BITS - lead_ones - 1));
        }
    }
    /* Handle last block. */
    size_t const end_i_in_block = (end + 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on
        = ~(ALL_BITS_ON >> ((BLOCK_BITS - end_i_in_block) - 1));
    lead_ones = countl_0(last_block_on & ~bs->set_[end_block]);
    if (lead_ones != BLOCK_BITS)
    {
        return (ptrdiff_t)((end_block * BLOCK_BITS)
                           + (BLOCK_BITS - lead_ones - 1));
    }
    return -1;
}

/* Performs the any or none scan operation over the specified range. The only
   difference between the operations is the return value. Specify the desired
   tribool value to return upon encountering an on bit. For any this is
   CCC_TRUE. For none this is CCC_FALSE. Saves writing two identical fns. */
static inline ccc_tribool
any_or_none_range(struct ccc_bitset_ const *const bs, size_t const i,
                  size_t const count, ccc_tribool const ret)
{
    size_t const end = i + count;
    if (!bs || i >= bs->cap_ || end > bs->cap_ || end < i)
    {
        return CCC_BOOL_ERR;
    }
    size_t start_block = block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on
            &= (ALL_BITS_ON >> (BLOCK_BITS - (start_i_in_block + count)));
    }
    if (first_block_on & bs->set_[start_block])
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
    size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
    if (last_block_on & bs->set_[end_block])
    {
        return ret;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        if (bs->set_[start_block] & ALL_BITS_ON)
        {
            return ret;
        }
    }
    return !ret;
}

/* Check for all on is slightly different from the any or none checks so we
   need a painfully repetitive function. */
static inline ccc_tribool
all_range(struct ccc_bitset_ const *const bs, size_t const i,
          size_t const count)
{
    size_t const end = i + count;
    if (!bs || i >= bs->cap_ || end > bs->cap_ || end < i)
    {
        return CCC_BOOL_ERR;
    }
    size_t start_block = block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on = ALL_BITS_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on
            &= (ALL_BITS_ON >> (BLOCK_BITS - (start_i_in_block + count)));
    }
    if ((first_block_on & bs->set_[start_block]) != first_block_on)
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
        if (bs->set_[start_block] != ALL_BITS_ON)
        {
            return CCC_FALSE;
        }
    }
    size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on
        = ALL_BITS_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
    if ((last_block_on & bs->set_[end_block]) != last_block_on)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

static inline void
set(ccc_bitblock_ *const block, size_t const bit_i, ccc_tribool const b)
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

static inline ccc_tribool
status(ccc_bitblock_ const *const bs, size_t const bit_i)
{
    /* Be careful. Bitwise & does not promise to evaluate to 1 or 0. We often
       just use it where that conversion takes place implicitly for us. */
    return (*bs & on(bit_i)) != 0;
}

/* Return a block with only the desired bit turned on to true. */
static inline ccc_bitblock_
on(size_t bit_i)
{
    return (ccc_bitblock_)1 << (bit_i % BLOCK_BITS);
}

/* Returns a mask of all bits on in the final bit block that represent only
   those bits which are in use according to the bit set capacity. The remaining
   higher order bits in the last block will be set to 0 because they are not
   used. If the capacity is zero a block with all bits on is returned. */
static ccc_bitblock_
last_on(struct ccc_bitset_ const *const bs)
{
    /* Remember, we fill from LSB to MSB so we want the mask to start at lower
       order bit which is why we do the second funky flip on the whole op. */
    return bs->cap_
               ? ~(((ccc_bitblock_)~1) << ((size_t)(bs->cap_ - 1) % BLOCK_BITS))
               : ALL_BITS_ON;
}

static inline size_t
block_i(size_t const bit_i)
{
    return bit_i / BLOCK_BITS;
}

static inline size_t
blocks(size_t const bits)
{
    return (bits + (BLOCK_BITS - 1)) / BLOCK_BITS;
}

static inline unsigned
popcount(ccc_bitblock_ const b)
{
#if defined(__GNUC__) || defined(__clang__)
    /* There are different pop counts for different integer widths. Be sure to
       catch the use of the wrong one by mistake here at compile time. */
    static_assert(sizeof(ccc_bitblock_) == sizeof(unsigned));
    return __builtin_popcount(b);
#else
    unsigned cnt = 0;
    for (; b; cnt += ((b & 1U) != 0), b >>= 1U)
    {}
    return cnt;
#endif
}

static inline ptrdiff_t
countr_0(ccc_bitblock_ const b)
{
#if defined(__GNUC__) || defined(__clang__)
    static_assert(BITBLOCK_MSB < ALL_BITS_ON);
    static_assert(sizeof(ccc_bitblock_) == sizeof(unsigned));
    return b ? __builtin_ctz(b) : (ptrdiff_t)BLOCK_BITS;
#else
    static_assert(BITBLOCK_MSB < ALL_BITS_ON);
    static_assert(sizeof(ccc_bitblock_) == sizeof(unsigned));
    if (!b)
    {
        return (ptrdiff_t)BLOCK_BITS;
    }
    ptrdiff_t cnt = 0;
    for (; !(b & 1U); ++cnt, b >>= 1U)
    {}
    return cnt;
#endif
}

static inline ptrdiff_t
countl_0(ccc_bitblock_ const b)
{
#if defined(__GNUC__) || defined(__clang__)
    static_assert(BITBLOCK_MSB < ALL_BITS_ON);
    static_assert(sizeof(ccc_bitblock_) == sizeof(unsigned));
    return b ? __builtin_clz(b) : (ptrdiff_t)BLOCK_BITS;
#else
    static_assert(BITBLOCK_MSB < ALL_BITS_ON);
    static_assert(sizeof(ccc_bitblock_) == sizeof(unsigned));
    if (!b)
    {
        return (ptrdiff_t)BLOCK_BITS;
    }
    ptrdiff_t cnt = 0;
    for (; !(b & BITBLOCK_MSB); ++cnt, b <<= 1U)
    {}
    return cnt;
#endif
}
