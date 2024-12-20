#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>

#include "bitset.h"
#include "impl/impl_bitset.h"
#include "types.h"

#define BLOCK_BITS (sizeof(ccc_bitblock_) * CHAR_BIT)
#define ALL_ON ((ccc_bitblock_)~0)
#define BITBLOCK_MSB                                                           \
    (((ccc_bitblock_)1) << (((sizeof(ccc_bitblock_) * CHAR_BIT)) - 1))

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
static ptrdiff_t first_trailing_1_range(struct ccc_bitset_ const *bs, size_t i,
                                        size_t count);
static ptrdiff_t first_trailing_0_range(struct ccc_bitset_ const *bs, size_t i,
                                        size_t count);
static ptrdiff_t countr_0(ccc_bitblock_);

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
    size_t const start_block = block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on = ALL_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on &= (ALL_ON >> (BLOCK_BITS - (start_i_in_block + count)));
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
    if (end_block != start_block)
    {
        size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
        ccc_bitblock_ last_block_on
            = ALL_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
        if (b)
        {
            bs->set_[end_block] |= last_block_on;
        }
        else
        {
            bs->set_[end_block] &= ~last_block_on;
        }
        if (end_block - start_block > 1)
        {
            int const v = b ? ~0 : 0;
            (void)memset(&bs->set_[start_block + 1], v,
                         (end_block - (start_block + 1))
                             * sizeof(ccc_bitblock_));
        }
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
    size_t const start_block = block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on = ALL_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on &= (ALL_ON >> (BLOCK_BITS - (start_i_in_block + count)));
    }
    bs->set_[start_block] &= ~first_block_on;
    size_t const end_block = block_i(end - 1);
    if (end_block != start_block)
    {
        size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
        ccc_bitblock_ last_block_on
            = ALL_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
        bs->set_[end_block] &= ~last_block_on;
        if (end_block - start_block > 1)
        {
            (void)memset(&bs->set_[start_block + 1], 0,
                         (end_block - (start_block + 1))
                             * sizeof(ccc_bitblock_));
        }
    }
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
    ccc_bitblock_ first_block_on = ALL_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on &= (ALL_ON >> (BLOCK_BITS - (start_i_in_block + count)));
    }
    bs->set_[start_block] ^= first_block_on;
    size_t const end_block = block_i(end - 1);
    if (end_block != start_block)
    {
        size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
        ccc_bitblock_ last_block_on
            = ALL_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
        bs->set_[end_block] ^= last_block_on;
        if (end_block - start_block > 1)
        {
            for (++start_block; start_block < end_block; ++start_block)
            {
                bs->set_[start_block] = ~bs->set_[start_block];
            }
        }
    }
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
    ccc_bitblock_ first_block_on = ALL_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on &= (ALL_ON >> (BLOCK_BITS - (start_i_in_block + count)));
    }
    popped += popcount(first_block_on & bs->set_[start_block]);
    size_t const end_block = block_i(end - 1);
    if (end_block != start_block)
    {
        size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
        ccc_bitblock_ last_block_on
            = ALL_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
        popped += popcount(last_block_on & bs->set_[end_block]);
        if (end_block - start_block > 1)
        {
            for (++start_block; start_block < end_block;
                 popped += popcount(bs->set_[start_block++]))
            {}
        }
    }
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
ccc_bs_first_trailing_1_range(ccc_bitset const *const bs, size_t const i,
                              size_t const count)
{
    return first_trailing_1_range(bs, i, count);
}

ptrdiff_t
ccc_bs_first_trailing_1(ccc_bitset const *const bs)
{
    return first_trailing_1_range(bs, 0, bs->cap_);
}

ptrdiff_t
ccc_bs_first_trailing_0_range(ccc_bitset const *const bs, size_t const i,
                              size_t const count)
{
    return first_trailing_0_range(bs, i, count);
}

ptrdiff_t
ccc_bs_first_trailing_0(ccc_bitset const *const bs)
{
    return first_trailing_0_range(bs, 0, bs->cap_);
}

/*=======================    Static Helpers    ==============================*/

static inline ptrdiff_t
first_trailing_1_range(struct ccc_bitset_ const *const bs, size_t const i,
                       size_t const count)
{
    size_t const end = i + count;
    if (!bs || i >= bs->cap_ || end > bs->cap_ || end < i)
    {
        return -1;
    }
    size_t start_block = block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on = ALL_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on &= (ALL_ON >> (BLOCK_BITS - (start_i_in_block + count)));
    }
    ptrdiff_t i_in_block = countr_0(first_block_on & bs->set_[start_block]);
    if (i_in_block != BLOCK_BITS)
    {
        return ((ptrdiff_t)(start_block * BLOCK_BITS)) + i_in_block;
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
            return ((ptrdiff_t)(start_block * BLOCK_BITS)) + i_in_block;
        }
    }
    /* Handle last block. */
    size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on = ALL_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
    i_in_block = countr_0(last_block_on & bs->set_[end_block]);
    if (i_in_block != BLOCK_BITS)
    {
        return ((ptrdiff_t)(end_block * BLOCK_BITS)) + i_in_block;
    }
    return -1;
}

static inline ptrdiff_t
first_trailing_0_range(struct ccc_bitset_ const *const bs, size_t const i,
                       size_t const count)
{
    size_t const end = i + count;
    if (!bs || i >= bs->cap_ || end > bs->cap_ || end < i)
    {
        return -1;
    }
    size_t start_block = block_i(i);
    size_t const start_i_in_block = i % BLOCK_BITS;
    ccc_bitblock_ first_block_on = ALL_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on &= (ALL_ON >> (BLOCK_BITS - (start_i_in_block + count)));
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
    ccc_bitblock_ last_block_on = ALL_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
    i_in_block = countr_0(last_block_on & ~bs->set_[end_block]);
    if (i_in_block != BLOCK_BITS)
    {
        return ((ptrdiff_t)(end_block * BLOCK_BITS)) + i_in_block;
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
    ccc_bitblock_ first_block_on = ALL_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on &= (ALL_ON >> (BLOCK_BITS - (start_i_in_block + count)));
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
    size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on = ALL_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
    if (last_block_on & bs->set_[end_block])
    {
        return ret;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        if (bs->set_[start_block] & ALL_ON)
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
    ccc_bitblock_ first_block_on = ALL_ON << start_i_in_block;
    if (start_i_in_block + count < BLOCK_BITS)
    {
        first_block_on &= (ALL_ON >> (BLOCK_BITS - (start_i_in_block + count)));
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
    size_t const end_i_in_block = (end - 1) % BLOCK_BITS;
    ccc_bitblock_ last_block_on = ALL_ON >> ((BLOCK_BITS - end_i_in_block) - 1);
    if ((last_block_on & bs->set_[end_block]) != last_block_on)
    {
        return CCC_FALSE;
    }
    for (++start_block; start_block < end_block; ++start_block)
    {
        if (bs->set_[start_block] != ALL_ON)
        {
            return CCC_FALSE;
        }
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
               : ALL_ON;
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
    static_assert(BITBLOCK_MSB < ALL_ON);
    static_assert(sizeof(ccc_bitblock_) == sizeof(unsigned));
    return b ? __builtin_ctz(b) : (ptrdiff_t)BLOCK_BITS;
#else
    static_assert(BITBLOCK_MSB < ALL_ON);
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
