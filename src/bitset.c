#include <assert.h>
#include <limits.h>
#include <string.h>

#include "bitset.h"
#include "impl/impl_bitset.h"
#include "types.h"

#define BLOCK_BITS (sizeof(ccc_bitblock_) * CHAR_BIT)

/*=========================   Prototypes   ==================================*/

static size_t block_i(size_t bit_i);
static void set(ccc_bitblock_ *, size_t bit_i, ccc_tribool);
static ccc_bitblock_ on(size_t bit_i);
static ccc_bitblock_ last_on(struct ccc_bitset_ const *);
static ccc_tribool status(ccc_bitblock_ const *, size_t bit_i);
static size_t blocks(size_t bits);
static unsigned popcount(ccc_bitblock_);

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
ccc_bs_set_range(ccc_bitset *const bs, size_t i, size_t const count,
                 ccc_tribool const b)
{
    size_t const end = i + count;
    if (!bs || b <= CCC_BOOL_ERR || b > CCC_TRUE || i >= bs->cap_
        || end > bs->cap_ || end < i)
    {
        return CCC_INPUT_ERR;
    }
    size_t const start_block = block_i(i);
    size_t const start_block_index = i % BLOCK_BITS;
    ccc_bitblock_ first_mask = (ccc_bitblock_)~0 << start_block_index;
    if (start_block_index + count < BLOCK_BITS)
    {
        first_mask &= (((ccc_bitblock_)~0)
                       >> (BLOCK_BITS - (start_block_index + count)));
    }
    if (b)
    {
        bs->set_[start_block] |= first_mask;
    }
    else
    {
        bs->set_[start_block] &= ~first_mask;
    }
    size_t const end_block = block_i(end - 1);
    if (end_block != start_block)
    {
        size_t const end_block_index = (end - 1) % BLOCK_BITS;
        ccc_bitblock_ last_mask
            = (((ccc_bitblock_)~0)) >> ((BLOCK_BITS - end_block_index) - 1);
        if (b)
        {
            bs->set_[end_block] |= last_mask;
        }
        else
        {
            bs->set_[end_block] &= ~last_mask;
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
ccc_bs_reset_range(ccc_bitset *const bs, size_t i, size_t const count)
{
    size_t const end = i + count;
    if (!bs || i >= bs->cap_ || end > bs->cap_ || end < i)
    {
        return CCC_INPUT_ERR;
    }
    size_t const start_block = block_i(i);
    size_t const start_block_index = i % BLOCK_BITS;
    ccc_bitblock_ first_mask = (ccc_bitblock_)~0 << start_block_index;
    if (start_block_index + count < BLOCK_BITS)
    {
        first_mask &= (((ccc_bitblock_)~0)
                       >> (BLOCK_BITS - (start_block_index + count)));
    }
    bs->set_[start_block] &= ~first_mask;
    size_t const end_block = block_i(end - 1);
    if (end_block != start_block)
    {
        size_t const end_block_index = (end - 1) % BLOCK_BITS;
        ccc_bitblock_ last_mask
            = (((ccc_bitblock_)~0)) >> ((BLOCK_BITS - end_block_index) - 1);
        bs->set_[end_block] &= ~last_mask;
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
ccc_bs_flip_range(ccc_bitset *const bs, size_t i, size_t const count)
{
    size_t const end = i + count;
    if (!bs || i >= bs->cap_ || end > bs->cap_ || end < i)
    {
        return CCC_INPUT_ERR;
    }
    size_t const start_block = block_i(i);
    size_t const start_block_index = i % BLOCK_BITS;
    ccc_bitblock_ first_mask = (ccc_bitblock_)~0 << start_block_index;
    if (start_block_index + count < BLOCK_BITS)
    {
        first_mask &= (((ccc_bitblock_)~0)
                       >> (BLOCK_BITS - (start_block_index + count)));
    }
    bs->set_[start_block] ^= first_mask;
    size_t const end_block = block_i(end - 1);
    if (end_block != start_block)
    {
        size_t const end_block_index = (end - 1) % BLOCK_BITS;
        ccc_bitblock_ last_mask
            = (((ccc_bitblock_)~0)) >> ((BLOCK_BITS - end_block_index) - 1);
        bs->set_[end_block] ^= last_mask;
        if (end_block - start_block > 1)
        {
            for (size_t block = start_block + 1; block < end_block; ++block)
            {
                bs->set_[block] = ~bs->set_[block];
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

/*=======================    Static Helpers    ==============================*/

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
               : ~0;
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
