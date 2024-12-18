#include <limits.h>
#include <string.h>

#include "bitset.h"
#include "impl/impl_bitset.h"
#include "types.h"

/*=========================   Prototypes   ==================================*/

static size_t block_i(size_t bit_i);
static void set(ccc_bitblock_ *, size_t bit_i, ccc_tribool);
static ccc_bitblock_ on(size_t bit_i);
static ccc_bitblock_ last_on(struct ccc_bitset_ const *);
static ccc_tribool status(ccc_bitblock_ const *, size_t bit_i);
static size_t blocks(size_t bits);

/*=======================   Public Interface   ==============================*/

ccc_tribool
ccc_btst_set(ccc_bitset *const btst, size_t const i, ccc_tribool const b)
{
    if (!btst)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &btst->set_[block_i(i)];
    ccc_tribool const res = status(block, i);
    set(block, i, b);
    return res;
}

ccc_tribool
ccc_btst_set_at(ccc_bitset *const btst, size_t const i, ccc_tribool const b)
{
    size_t const b_i = block_i(i);
    if (!btst || b_i >= btst->cap_)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &btst->set_[b_i];
    ccc_tribool const res = status(block, i);
    set(block, i, b);
    return res;
}

ccc_result
ccc_btst_set_all(ccc_bitset *const btst, ccc_tribool const b)
{
    if (!btst || b == CCC_BOOL_ERR)
    {
        return CCC_INPUT_ERR;
    }
    if (btst->cap_)
    {
        memset(btst->set_, b, blocks(btst->cap_) * sizeof(ccc_bitblock_));
        btst->set_[block_i(btst->cap_ - 1)] &= last_on(btst);
    }
    return CCC_OK;
}

ccc_tribool
ccc_btst_reset(ccc_bitset *const btst, size_t const i)
{
    if (!btst)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &btst->set_[block_i(i)];
    ccc_tribool const res = status(block, i);
    *block &= ~on(i);
    return res;
}

ccc_tribool
ccc_btst_reset_at(ccc_bitset *const btst, size_t const i)
{
    size_t const b_i = block_i(i);
    if (!btst || b_i >= btst->cap_)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &btst->set_[b_i];
    ccc_tribool const res = status(block, i);
    *block &= ~on(i);
    return res;
}

ccc_result
ccc_btst_reset_all(ccc_bitset *const btst)
{
    if (!btst)
    {
        return CCC_INPUT_ERR;
    }
    if (btst->cap_)
    {
        memset(btst->set_, CCC_FALSE,
               blocks(btst->cap_) * sizeof(ccc_bitblock_));
    }
    return CCC_OK;
}

ccc_tribool
ccc_btst_flip(ccc_bitset *const btst, size_t const i)
{
    if (!btst)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &btst->set_[block_i(i)];
    ccc_tribool const res = status(block, i);
    *block ^= on(i);
    return res;
}

ccc_tribool
ccc_btst_flip_at(ccc_bitset *const btst, size_t const i)
{
    size_t const b_i = block_i(i);
    if (!btst || b_i >= btst->cap_)
    {
        return CCC_BOOL_ERR;
    }
    ccc_bitblock_ *const block = &btst->set_[b_i];
    ccc_tribool const res = status(block, i);
    *block ^= on(i);
    return res;
}

ccc_result
ccc_btst_flip_all(ccc_bitset *const btst)
{
    if (!btst)
    {
        return CCC_INPUT_ERR;
    }
    if (!btst->cap_)
    {
        return CCC_OK;
    }
    size_t const end = blocks(btst->cap_);
    for (size_t i = 0; i < end; ++i)
    {
        btst->set_[i] = ~btst->set_[i];
    }
    btst->set_[end - 1] &= last_on(btst);
    return CCC_OK;
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
status(ccc_bitblock_ const *const btst, size_t const bit_i)
{
    return *btst & on(bit_i);
}

static inline ccc_bitblock_
on(size_t bit_i)
{
    return (ccc_bitblock_)1 << (bit_i % CCC_IMPL_BTST_BLOCK_BITS);
}

/* Returns a mask of all bits on in the final bit block that represent only
   those bits which are in use according to the bit set capacity. The remaining
   higher order bits in the last block will be set to 0 because they are not
   used. If the capacity is zero a block with all bits on is returned. */
static ccc_bitblock_
last_on(struct ccc_bitset_ const *const btst)
{
    /* Remember, we fill from LSB to MSB so we want the mask to start at lower
       order bit which is why we do the second funky flip on the whole op. */
    return btst->cap_
               ? ~(((ccc_bitblock_)~1)
                   << ((size_t)(btst->cap_ - 1) % CCC_IMPL_BTST_BLOCK_BITS))
               : ~0;
}

static inline size_t
block_i(size_t const bit_i)
{
    return bit_i / CCC_IMPL_BTST_BLOCK_BITS;
}

static inline size_t
blocks(size_t const bits)
{
    return ((
        size_t)((bits)
                + ((CCC_IMPL_BTST_BLOCK_BITS - 1) / CCC_IMPL_BTST_BLOCK_BITS)));
}
