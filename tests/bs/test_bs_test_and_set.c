#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#define BITSET_USING_NAMESPACE_CCC
#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"

CHECK_BEGIN_STATIC_FN(bs_test_set_one)
{
    bitset bs = bs_init((bitblock[bs_blocks(10)]){}, NULL, NULL, 10, 10);
    CHECK(bs_capacity(&bs), 10);
    /* Was false before. */
    CHECK(bs_set(&bs, 5, CCC_TRUE), CCC_FALSE);
    CHECK(bs_set_at(&bs, 5, CCC_TRUE), CCC_TRUE);
    CHECK(bs_popcount(&bs), 1);
    CHECK(bs_set(&bs, 5, CCC_FALSE), CCC_TRUE);
    CHECK(bs_set_at(&bs, 5, CCC_FALSE), CCC_FALSE);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_set_shuffled)
{
    bitset bs = bs_init((bitblock[bs_blocks(10)]){}, NULL, NULL, 10, 10);
    size_t const larger_prime = 11;
    for (size_t i = 0, shuf_i = larger_prime % 10; i < 10;
         ++i, shuf_i = (shuf_i + larger_prime) % 10)
    {
        CHECK(bs_set(&bs, i, CCC_TRUE), CCC_FALSE);
        CHECK(bs_set_at(&bs, i, CCC_TRUE), CCC_TRUE);
    }
    CHECK(bs_popcount(&bs), 10);
    for (size_t i = 0; i < 10; ++i)
    {
        CHECK(bs_test(&bs, i), CCC_TRUE);
        CHECK(bs_test_at(&bs, i), CCC_TRUE);
    }
    CHECK(bs_capacity(&bs), 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_set_all)
{
    bitset bs = bs_init((bitblock[bs_blocks(10)]){}, NULL, NULL, 10, 10);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    CHECK(bs_popcount(&bs), 10);
    for (size_t i = 0; i < 10; ++i)
    {
        CHECK(bs_test(&bs, i), CCC_TRUE);
        CHECK(bs_test_at(&bs, i), CCC_TRUE);
    }
    CHECK(bs_capacity(&bs), 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_set_range)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    /* Start with a full range and reduce from the end. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        CHECK(bs_set_range(&bs, 0, count, CCC_TRUE), CCC_OK);
        CHECK(bs_popcount_range(&bs, 0, count), count);
        CHECK(bs_popcount(&bs), count);
        CHECK(bs_set_range(&bs, 0, count, CCC_FALSE), CCC_OK);
        CHECK(bs_popcount_range(&bs, 0, count), 0);
        CHECK(bs_popcount(&bs), 0);
    }
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        CHECK(bs_set_range(&bs, i, count, CCC_TRUE), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), count);
        CHECK(bs_popcount(&bs), count);
        CHECK(bs_set_range(&bs, i, count, CCC_FALSE), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), 0);
        CHECK(bs_popcount(&bs), 0);
    }
    /* Shrink range from both ends. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        CHECK(bs_set_range(&bs, i, count, CCC_TRUE), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), count);
        CHECK(bs_popcount(&bs), count);
        CHECK(bs_set_range(&bs, i, count, CCC_FALSE), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), 0);
        CHECK(bs_popcount(&bs), 0);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_reset)
{
    bitset bs = bs_init((bitblock[bs_blocks(10)]){}, NULL, NULL, 10, 10);
    size_t const larger_prime = 11;
    for (size_t i = 0, shuf_i = larger_prime % 10; i < 10;
         ++i, shuf_i = (shuf_i + larger_prime) % 10)
    {
        CHECK(bs_set(&bs, i, CCC_TRUE), CCC_FALSE);
        CHECK(bs_set_at(&bs, i, CCC_TRUE), CCC_TRUE);
    }
    CHECK(bs_reset(&bs, 9), CCC_TRUE);
    CHECK(bs_reset(&bs, 9), CCC_FALSE);
    CHECK(bs_popcount(&bs), 9);
    CHECK(bs_capacity(&bs), 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_reset_all)
{
    bitset bs = bs_init((bitblock[bs_blocks(10)]){}, NULL, NULL, 10, 10);
    CHECK(bs_capacity(&bs), 10);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    CHECK(bs_popcount(&bs), 10);
    CHECK(bs_reset_all(&bs), CCC_OK);
    CHECK(bs_popcount(&bs), 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_reset_range)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    /* Start with a full range and reduce from the end. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        CHECK(bs_set_range(&bs, 0, count, CCC_TRUE), CCC_OK);
        CHECK(bs_popcount_range(&bs, 0, count), count);
        CHECK(bs_popcount(&bs), count);
        CHECK(bs_reset_range(&bs, 0, count), CCC_OK);
        CHECK(bs_popcount_range(&bs, 0, count), 0);
        CHECK(bs_popcount(&bs), 0);
    }
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        CHECK(bs_set_range(&bs, i, count, CCC_TRUE), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), count);
        CHECK(bs_popcount(&bs), count);
        CHECK(bs_reset_range(&bs, i, count), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), 0);
        CHECK(bs_popcount(&bs), 0);
    }
    /* Shrink range from both ends. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        CHECK(bs_set_range(&bs, i, count, CCC_TRUE), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), count);
        CHECK(bs_popcount(&bs), count);
        CHECK(bs_reset_range(&bs, i, count), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), 0);
        CHECK(bs_popcount(&bs), 0);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_flip)
{
    bitset bs = bs_init((bitblock[bs_blocks(10)]){}, NULL, NULL, 10, 10);
    CHECK(bs_capacity(&bs), 10);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    CHECK(bs_popcount(&bs), 10);
    CHECK(bs_flip(&bs, 9), CCC_TRUE);
    CHECK(bs_popcount(&bs), 9);
    CHECK(bs_flip_at(&bs, 9), CCC_FALSE);
    CHECK(bs_popcount(&bs), 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_flip_all)
{
    bitset bs = bs_init((bitblock[bs_blocks(10)]){}, NULL, NULL, 10, 10);
    CHECK(bs_capacity(&bs), 10);
    for (size_t i = 0; i < 10; i += 2)
    {
        CHECK(bs_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bs_popcount(&bs), 5);
    CHECK(bs_flip_all(&bs), CCC_OK);
    CHECK(bs_popcount(&bs), 5);
    for (size_t i = 0; i < 10; ++i)
    {
        if (i % 2)
        {
            CHECK(bs_test(&bs, i), CCC_TRUE);
            CHECK(bs_test_at(&bs, i), CCC_TRUE);
        }
        else
        {
            CHECK(bs_test(&bs, i), CCC_FALSE);
            CHECK(bs_test_at(&bs, i), CCC_FALSE);
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_flip_range)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    size_t const orignal_popcount = bs_popcount(&bs);
    /* Start with a full range and reduce from the end. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        CHECK(bs_flip_range(&bs, 0, count), CCC_OK);
        CHECK(bs_popcount_range(&bs, 0, count), 0);
        CHECK(bs_popcount(&bs), orignal_popcount - count);
        CHECK(bs_flip_range(&bs, 0, count), CCC_OK);
        CHECK(bs_popcount_range(&bs, 0, count), count);
        CHECK(bs_popcount(&bs), orignal_popcount);
    }
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        CHECK(bs_flip_range(&bs, i, count), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), 0);
        CHECK(bs_popcount(&bs), orignal_popcount - count);
        CHECK(bs_flip_range(&bs, i, count), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), count);
        CHECK(bs_popcount(&bs), orignal_popcount);
    }
    /* Shrink range from both ends. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        CHECK(bs_flip_range(&bs, i, count), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), 0);
        CHECK(bs_popcount(&bs), orignal_popcount - count);
        CHECK(bs_flip_range(&bs, i, count), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), count);
        CHECK(bs_popcount(&bs), orignal_popcount);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_any)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    size_t const cap = bs_capacity(&bs);
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        CHECK(bs_set_range(&bs, i, count, CCC_TRUE), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), count);
        CHECK(bs_popcount(&bs), count);
        CHECK(bs_any(&bs), CCC_TRUE);
        CHECK(bs_any_range(&bs, 0, cap), CCC_TRUE);
        CHECK(bs_reset_range(&bs, i, count), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), 0);
        CHECK(bs_popcount(&bs), 0);
        CHECK(bs_any(&bs), CCC_FALSE);
        CHECK(bs_any_range(&bs, 0, cap), CCC_FALSE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_none)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    size_t const cap = bs_capacity(&bs);
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        CHECK(bs_set_range(&bs, i, count, CCC_TRUE), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), count);
        CHECK(bs_popcount(&bs), count);
        CHECK(bs_none(&bs), CCC_FALSE);
        CHECK(bs_none_range(&bs, 0, cap), CCC_FALSE);
        CHECK(bs_reset_range(&bs, i, count), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), 0);
        CHECK(bs_popcount(&bs), 0);
        CHECK(bs_none(&bs), CCC_TRUE);
        CHECK(bs_none_range(&bs, 0, cap), CCC_TRUE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_all)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    size_t const cap = bs_capacity(&bs);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    CHECK(bs_all(&bs), CCC_TRUE);
    CHECK(bs_all_range(&bs, 0, cap), CCC_TRUE);
    CHECK(bs_reset_all(&bs), CCC_OK);
    /* Start with an almost full range and reduce by moving start forward. */
    for (size_t i = 1, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        CHECK(bs_set_range(&bs, i, count, CCC_TRUE), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), count);
        CHECK(bs_popcount(&bs), count);
        CHECK(bs_all(&bs), CCC_FALSE);
        CHECK(bs_all_range(&bs, i, count), CCC_TRUE);
        CHECK(bs_reset_range(&bs, i, count), CCC_OK);
        CHECK(bs_popcount_range(&bs, i, count), 0);
        CHECK(bs_popcount(&bs), 0);
        CHECK(bs_all(&bs), CCC_FALSE);
        CHECK(bs_all_range(&bs, i, count), CCC_FALSE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_first_trailing_one)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    /* Start with an almost full range and reduce by moving start forward. */
    for (size_t i = 0, end = 512; i < end - 1; ++i)
    {
        CHECK(bs_set(&bs, i, CCC_FALSE), CCC_TRUE);
        CHECK(bs_first_trailing_one(&bs), i + 1);
        CHECK(bs_first_trailing_one_range(&bs, 0, i + 1), -1);
        CHECK(bs_first_trailing_one_range(&bs, i, end - i), i + 1);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_first_trailing_ones)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    size_t window = sizeof(bitblock) * CHAR_BIT;
    /* Slide a group of int size as a window across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        CHECK(bs_set_range(&bs, i, window, CCC_TRUE), CCC_OK);
        CHECK(bs_first_trailing_ones(&bs, window), i);
        CHECK(bs_first_trailing_ones(&bs, window - 1), i);
        CHECK(bs_first_trailing_ones(&bs, window + 1), -1);
        CHECK(bs_first_trailing_ones_range(&bs, 0, i, window), -1);
        CHECK(bs_first_trailing_ones_range(&bs, i, window, window), i);
        CHECK(bs_first_trailing_ones_range(&bs, i + 1, window, window), -1);
        /* Cleanup behind as we go. */
        CHECK(bs_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    CHECK(bs_reset_all(&bs), CCC_OK);
    window /= 4;
    /* Slide a very small group across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        CHECK(bs_set_range(&bs, i, window, CCC_TRUE), CCC_OK);
        CHECK(bs_first_trailing_ones(&bs, window), i);
        CHECK(bs_first_trailing_ones(&bs, window - 1), i);
        CHECK(bs_first_trailing_ones(&bs, window + 1), -1);
        CHECK(bs_first_trailing_ones_range(&bs, 0, i, window), -1);
        CHECK(bs_first_trailing_ones_range(&bs, i, window, window), i);
        CHECK(bs_first_trailing_ones_range(&bs, i + 1, window, window), -1);
        /* Cleanup behind as we go. */
        CHECK(bs_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    CHECK(bs_reset_all(&bs), CCC_OK);
    window *= 8;
    /* Slide a very large group across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        CHECK(bs_set_range(&bs, i, window, CCC_TRUE), CCC_OK);
        CHECK(bs_first_trailing_ones(&bs, window), i);
        CHECK(bs_first_trailing_ones(&bs, window - 1), i);
        CHECK(bs_first_trailing_ones(&bs, window + 1), -1);
        CHECK(bs_first_trailing_ones_range(&bs, 0, i, window), -1);
        CHECK(bs_first_trailing_ones_range(&bs, i, window, window), i);
        CHECK(bs_first_trailing_ones_range(&bs, i + 1, window, window), -1);
        /* Cleanup behind as we go. */
        CHECK(bs_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_first_trailing_ones_fail)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    size_t const end = bs_blocks(512);
    size_t const bits_in_block = sizeof(bitblock) * CHAR_BIT;
    size_t const first_half = bits_in_block / 2;
    size_t const second_half = first_half - 1;
    /* We are going to search for a group of 16 which we will be very close
       to finding every time but it will be broken by an off bit before the
       16th in a group in every block. */
    for (size_t block = 0, i = 0; block < end;
         ++block, i = block * bits_in_block)
    {
        CHECK(bs_set_range(&bs, i, first_half, CCC_TRUE), CCC_OK);
        CHECK(bs_set_range(&bs, i + first_half + 1, second_half, CCC_TRUE),
              CCC_OK);
        CHECK(
            bs_first_trailing_ones_range(&bs, i, bits_in_block, first_half + 1),
            -1);
    }
    /* Then we will search for a full block worth which we will never find
       thanks to the off bit embedded in each block. */
    CHECK(bs_first_trailing_ones(&bs, bits_in_block), -1);
    /* Now fix the last group and we should pass. */
    CHECK(bs_set_at(&bs, ((end - 1) * bits_in_block) + first_half, CCC_TRUE),
          CCC_FALSE);
    /* Now the solution crosses the block border from second to last to last
       block. */
    CHECK(bs_first_trailing_ones(&bs, bits_in_block),
          (((end - 2) * bits_in_block) + first_half + 1));
    (void)bs_reset_all(&bs);
    (void)bs_set_range(&bs, 0, bits_in_block * 3, CCC_TRUE);
    CHECK(bs_set_at(&bs, first_half, CCC_FALSE), CCC_TRUE);
    CHECK(bs_first_trailing_ones_range(&bs, 0, bits_in_block, bits_in_block),
          -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_first_trailing_zero)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    /* Start with an almost full range and reduce by moving start forward. */
    for (size_t i = 0, end = 512; i < end - 1; ++i)
    {
        CHECK(bs_set(&bs, i, CCC_TRUE), CCC_FALSE);
        CHECK(bs_first_trailing_zero(&bs), i + 1);
        CHECK(bs_first_trailing_zero_range(&bs, 0, i + 1), -1);
        CHECK(bs_first_trailing_zero_range(&bs, i, end - i), i + 1);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_first_trailing_zeros)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    size_t window = sizeof(bitblock) * CHAR_BIT;
    /* Slide a group of int size as a window across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        CHECK(bs_set_range(&bs, i, window, CCC_FALSE), CCC_OK);
        CHECK(bs_first_trailing_zeros(&bs, window), i);
        CHECK(bs_first_trailing_zeros(&bs, window - 1), i);
        CHECK(bs_first_trailing_zeros(&bs, window + 1), -1);
        CHECK(bs_first_trailing_zeros_range(&bs, 0, i, window), -1);
        CHECK(bs_first_trailing_zeros_range(&bs, i, window, window), i);
        CHECK(bs_first_trailing_zeros_range(&bs, i + 1, window, window), -1);
        /* Cleanup behind as we go. */
        CHECK(bs_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    window /= 4;
    /* Slide a very small group across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        CHECK(bs_set_range(&bs, i, window, CCC_FALSE), CCC_OK);
        CHECK(bs_first_trailing_zeros(&bs, window), i);
        CHECK(bs_first_trailing_zeros(&bs, window - 1), i);
        CHECK(bs_first_trailing_zeros(&bs, window + 1), -1);
        CHECK(bs_first_trailing_zeros_range(&bs, 0, i, window), -1);
        CHECK(bs_first_trailing_zeros_range(&bs, i, window, window), i);
        CHECK(bs_first_trailing_zeros_range(&bs, i + 1, window, window), -1);
        /* Cleanup behind as we go. */
        CHECK(bs_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    window *= 8;
    /* Slide a very large group across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        CHECK(bs_set_range(&bs, i, window, CCC_FALSE), CCC_OK);
        CHECK(bs_first_trailing_zeros(&bs, window), i);
        CHECK(bs_first_trailing_zeros(&bs, window - 1), i);
        CHECK(bs_first_trailing_zeros(&bs, window + 1), -1);
        CHECK(bs_first_trailing_zeros_range(&bs, 0, i, window), -1);
        CHECK(bs_first_trailing_zeros_range(&bs, i, window, window), i);
        CHECK(bs_first_trailing_zeros_range(&bs, i + 1, window, window), -1);
        /* Cleanup behind as we go. */
        CHECK(bs_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_first_trailing_zeros_fail)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    size_t const end = bs_blocks(512);
    size_t const bits_in_block = sizeof(bitblock) * CHAR_BIT;
    size_t const first_half = bits_in_block / 2;
    size_t const second_half = first_half - 1;
    /* We are going to search for a group of 16 which we will be very close
       to finding every time but it will be broken by an off bit before the
       16th in a group in every block. */
    for (size_t block = 0, i = 0; block < end;
         ++block, i = block * bits_in_block)
    {
        CHECK(bs_set_range(&bs, i, first_half, CCC_FALSE), CCC_OK);
        CHECK(bs_set_range(&bs, i + first_half + 1, second_half, CCC_FALSE),
              CCC_OK);
        CHECK(bs_first_trailing_zeros_range(&bs, i, bits_in_block,
                                            first_half + 1),
              -1);
    }
    /* Then we will search for a full block worth which we will never find
       thanks to the off bit embedded in each block. */
    CHECK(bs_first_trailing_zeros(&bs, bits_in_block), -1);
    /* Now fix the last group and we should pass. */
    CHECK(bs_set_at(&bs, ((end - 1) * bits_in_block) + first_half, CCC_FALSE),
          CCC_TRUE);
    /* Now the solution crosses the block border from second to last to last
       block. */
    CHECK(bs_first_trailing_zeros(&bs, bits_in_block),
          (((end - 2) * bits_in_block) + first_half + 1));
    (void)bs_reset_all(&bs);
    (void)bs_set_range(&bs, 0, bits_in_block * 3, CCC_FALSE);
    CHECK(bs_set_at(&bs, first_half, CCC_TRUE), CCC_FALSE);
    CHECK(bs_first_trailing_zeros_range(&bs, 0, bits_in_block, bits_in_block),
          -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_first_leading_one)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    size_t const last_i = 511;
    /* Start with an almost full range and reduce by moving start backwards. */
    for (size_t i = last_i; i > 1; --i)
    {
        CHECK(bs_set(&bs, i, CCC_FALSE), CCC_TRUE);
        CHECK(bs_first_leading_one(&bs), i - 1);
        CHECK(bs_first_leading_one_range(&bs, last_i, 512 - i), -1);
        CHECK(bs_first_leading_one_range(&bs, i, i + 1), i - 1);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_first_leading_ones)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    size_t window = sizeof(bitblock) * CHAR_BIT;
    /* Slide a group of int size as a window across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        CHECK(bs_set_range(&bs, i - window + 1, window, CCC_TRUE), CCC_OK);
        CHECK(bs_first_leading_ones(&bs, window), i);
        CHECK(bs_first_leading_ones(&bs, window - 1), i);
        CHECK(bs_first_leading_ones(&bs, window + 1), -1);
        CHECK(bs_first_leading_ones_range(&bs, 0, i, window), -1);
        CHECK(bs_first_leading_ones_range(&bs, i, window, window), i);
        CHECK(bs_first_leading_ones_range(&bs, i + 1, window, window), -1);
        /* Cleanup behind as we go. */
        CHECK(bs_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    CHECK(bs_reset_all(&bs), CCC_OK);
    window /= 4;
    /* Slide a very small group across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        CHECK(bs_set_range(&bs, i - window + 1, window, CCC_TRUE), CCC_OK);
        CHECK(bs_first_leading_ones(&bs, window), i);
        CHECK(bs_first_leading_ones(&bs, window - 1), i);
        CHECK(bs_first_leading_ones(&bs, window + 1), -1);
        CHECK(bs_first_leading_ones_range(&bs, 0, i, window), -1);
        CHECK(bs_first_leading_ones_range(&bs, i, window, window), i);
        CHECK(bs_first_leading_ones_range(&bs, i + 1, window, window), -1);
        /* Cleanup behind as we go. */
        CHECK(bs_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    CHECK(bs_reset_all(&bs), CCC_OK);
    window *= 8;
    /* Slide a very large group across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        CHECK(bs_set_range(&bs, i - window + 1, window, CCC_TRUE), CCC_OK);
        CHECK(bs_first_leading_ones(&bs, window), i);
        CHECK(bs_first_leading_ones(&bs, window - 1), i);
        CHECK(bs_first_leading_ones(&bs, window + 1), -1);
        CHECK(bs_first_leading_ones_range(&bs, 0, i, window), -1);
        CHECK(bs_first_leading_ones_range(&bs, i, window, window), i);
        CHECK(bs_first_leading_ones_range(&bs, i + 1, window, window), -1);
        /* Cleanup behind as we go. */
        CHECK(bs_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_first_leading_ones_fail)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    ptrdiff_t const bits_in_block = sizeof(bitblock) * CHAR_BIT;
    size_t const first_half = bits_in_block / 2;
    size_t const second_half = first_half - 1;
    /* We are going to search for a group of 17 which we will be very close
       to finding every time but it will be broken by an off bit before the
       17th in a group in every block. */
    for (ptrdiff_t block = bs_blocks(512) - 1, i = 511; block >= 0;
         --block, i -= bits_in_block)
    {
        CHECK(bs_set_range(&bs, (block * bits_in_block), first_half, CCC_TRUE),
              CCC_OK);
        CHECK(bs_set_range(&bs, (block * bits_in_block) + first_half + 1,
                           second_half, CCC_TRUE),
              CCC_OK);
        CHECK(
            bs_first_leading_ones_range(&bs, i, bits_in_block, first_half + 1),
            -1);
    }
    /* Then we will search for a full block worth which we will never find
       thanks to the off bit embedded in each block. */
    CHECK(bs_first_leading_ones(&bs, bits_in_block), -1);
    /* Now fix the last group and we should pass. */
    CHECK(bs_set_at(&bs, first_half, CCC_TRUE), CCC_FALSE);
    /* Now the solution crosses the block border from second to last to last
       block. */
    CHECK(bs_first_leading_ones(&bs, bits_in_block),
          bits_in_block + first_half - 1);
    (void)bs_reset_all(&bs);
    (void)bs_set_range(&bs, 512 - (bits_in_block * 3), bits_in_block * 3,
                       CCC_TRUE);
    CHECK(bs_set_at(&bs, 512 - first_half, CCC_FALSE), CCC_TRUE);
    CHECK(bs_first_leading_ones_range(&bs, 511, bits_in_block, bits_in_block),
          -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_first_leading_zero)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    size_t const last_i = 511;
    /* Start with an almost full range and reduce by moving start backwards. */
    for (size_t i = last_i; i > 1; --i)
    {
        CHECK(bs_set(&bs, i, CCC_TRUE), CCC_FALSE);
        CHECK(bs_first_leading_zero(&bs), i - 1);
        CHECK(bs_first_leading_zero_range(&bs, last_i, 512 - i), -1);
        CHECK(bs_first_leading_zero_range(&bs, i, i + 1), i - 1);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_first_leading_zeros)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    size_t window = sizeof(bitblock) * CHAR_BIT;
    /* Slide a group of int size as a window across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        CHECK(bs_set_range(&bs, i - window + 1, window, CCC_FALSE), CCC_OK);
        CHECK(bs_first_leading_zeros(&bs, window), i);
        CHECK(bs_first_leading_zeros(&bs, window - 1), i);
        CHECK(bs_first_leading_zeros(&bs, window + 1), -1);
        CHECK(bs_first_leading_zeros_range(&bs, 0, i, window), -1);
        CHECK(bs_first_leading_zeros_range(&bs, i, window, window), i);
        CHECK(bs_first_leading_zeros_range(&bs, i + 1, window, window), -1);
        /* Cleanup behind as we go. */
        CHECK(bs_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    window /= 4;
    /* Slide a very small group across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        CHECK(bs_set_range(&bs, i - window + 1, window, CCC_FALSE), CCC_OK);
        CHECK(bs_first_leading_zeros(&bs, window), i);
        CHECK(bs_first_leading_zeros(&bs, window - 1), i);
        CHECK(bs_first_leading_zeros(&bs, window + 1), -1);
        CHECK(bs_first_leading_zeros_range(&bs, 0, i, window), -1);
        CHECK(bs_first_leading_zeros_range(&bs, i, window, window), i);
        CHECK(bs_first_leading_zeros_range(&bs, i + 1, window, window), -1);
        /* Cleanup behind as we go. */
        CHECK(bs_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    window *= 8;
    /* Slide a very large group across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        CHECK(bs_set_range(&bs, i - window + 1, window, CCC_FALSE), CCC_OK);
        CHECK(bs_first_leading_zeros(&bs, window), i);
        CHECK(bs_first_leading_zeros(&bs, window - 1), i);
        CHECK(bs_first_leading_zeros(&bs, window + 1), -1);
        CHECK(bs_first_leading_zeros_range(&bs, 0, i, window), -1);
        CHECK(bs_first_leading_zeros_range(&bs, i, window, window), i);
        CHECK(bs_first_leading_zeros_range(&bs, i + 1, window, window), -1);
        /* Cleanup behind as we go. */
        CHECK(bs_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_first_leading_zeros_fail)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    ptrdiff_t const bits_in_block = sizeof(bitblock) * CHAR_BIT;
    size_t const first_half = bits_in_block / 2;
    size_t const second_half = first_half - 1;
    /* We are going to search for a group of 17 which we will be very close
       to finding every time but it will be broken by an off bit before the
       17th in a group in every block. */
    for (ptrdiff_t block = bs_blocks(512) - 1, i = 511; block >= 0;
         --block, i -= bits_in_block)
    {
        CHECK(bs_set_range(&bs, (block * bits_in_block), first_half, CCC_FALSE),
              CCC_OK);
        CHECK(bs_set_range(&bs, (block * bits_in_block) + first_half + 1,
                           second_half, CCC_FALSE),
              CCC_OK);
        CHECK(
            bs_first_leading_zeros_range(&bs, i, bits_in_block, first_half + 1),
            -1);
    }
    /* Then we will search for a full block worth which we will never find
       thanks to the off bit embedded in each block. */
    CHECK(bs_first_leading_zeros(&bs, bits_in_block), -1);
    /* Now fix the last group and we should pass. */
    CHECK(bs_set_at(&bs, first_half, CCC_FALSE), CCC_TRUE);
    /* Now the solution crosses the block border from second to last to last
       block. */
    CHECK(bs_first_leading_zeros(&bs, bits_in_block),
          bits_in_block + first_half - 1);
    (void)bs_reset_all(&bs);
    (void)bs_set_range(&bs, 512 - (bits_in_block * 3), bits_in_block * 3,
                       CCC_FALSE);
    CHECK(bs_set_at(&bs, 512 - first_half, CCC_TRUE), CCC_FALSE);
    CHECK(bs_first_leading_zeros_range(&bs, 511, bits_in_block, bits_in_block),
          -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_or_same_size)
{
    bitset src = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    bitset dst = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    size_t const size = 512;
    for (size_t i = 0; i < size; i += 2)
    {
        CHECK(bs_set(&dst, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 1; i < size; i += 2)
    {
        CHECK(bs_set(&src, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bs_popcount(&src), size / 2);
    CHECK(bs_popcount(&dst), size / 2);
    CHECK(bs_or(&dst, &src), CCC_OK);
    CHECK(bs_popcount(&dst), size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_or_diff_size)
{
    bitset dst = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    /* Make it slightly harder by not ending on a perfect block boundary. */
    bitset src = bs_init((bitblock[bs_blocks(244)]){}, NULL, NULL, 244, 244);
    CHECK(bs_set_all(&src, CCC_TRUE), CCC_OK);
    CHECK(bs_popcount(&src), 244);
    CHECK(bs_popcount(&dst), 0);
    CHECK(bs_or(&dst, &src), CCC_OK);
    CHECK(bs_popcount(&dst), 244);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_and_same_size)
{
    bitset src = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    bitset dst = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    size_t const size = 512;
    for (size_t i = 0; i < size; i += 2)
    {
        CHECK(bs_set(&dst, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 1; i < size; i += 2)
    {
        CHECK(bs_set(&src, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bs_popcount(&src), size / 2);
    CHECK(bs_popcount(&dst), size / 2);
    CHECK(bs_and(&dst, &src), CCC_OK);
    CHECK(bs_popcount(&dst), 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_and_diff_size)
{
    bitset dst = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    /* Make it slightly harder by not ending on a perfect block boundary. */
    bitset src = bs_init((bitblock[bs_blocks(244)]){}, NULL, NULL, 244, 244);
    CHECK(bs_set_all(&dst, CCC_TRUE), CCC_OK);
    CHECK(bs_set_all(&src, CCC_TRUE), CCC_OK);
    CHECK(bs_popcount(&dst), 512);
    CHECK(bs_popcount(&src), 244);
    CHECK(bs_and(&dst, &src), CCC_OK);
    CHECK(bs_popcount(&dst), 244);
    CHECK(bs_size(&dst), 512);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_xor_same_size)
{
    bitset src = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    bitset dst = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    size_t const size = 512;
    for (size_t i = 0; i < size; i += 2)
    {
        CHECK(bs_set(&dst, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 1; i < size; i += 2)
    {
        CHECK(bs_set(&src, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bs_popcount(&src), size / 2);
    CHECK(bs_popcount(&dst), size / 2);
    CHECK(bs_xor(&dst, &src), CCC_OK);
    CHECK(bs_popcount(&dst), size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_xor_diff_size)
{
    bitset dst = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    /* Make it slightly harder by not ending on a perfect block boundary. */
    bitset src = bs_init((bitblock[bs_blocks(244)]){}, NULL, NULL, 244, 244);
    CHECK(bs_set_all(&dst, CCC_TRUE), CCC_OK);
    CHECK(bs_set_all(&src, CCC_TRUE), CCC_OK);
    CHECK(bs_popcount(&dst), 512);
    CHECK(bs_popcount(&src), 244);
    CHECK(bs_xor(&dst, &src), CCC_OK);
    CHECK(bs_popcount(&dst), 512 - 244);
    CHECK(bs_size(&dst), 512);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_shiftl)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    CHECK(bs_popcount(&bs), 512);
    CHECK(bs_shiftl(&bs, 512), CCC_OK);
    CHECK(bs_popcount(&bs), 0);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    size_t const bits_in_block = sizeof(bitblock) * CHAR_BIT;
    size_t ones = 512;
    CHECK(bs_shiftl(&bs, bits_in_block), CCC_OK);
    ones -= bits_in_block;
    CHECK(bs_popcount(&bs), ones);
    CHECK(bs_shiftl(&bs, bits_in_block / 2), CCC_OK);
    ones -= (bits_in_block / 2);
    CHECK(bs_popcount(&bs), ones);
    CHECK(bs_shiftl(&bs, bits_in_block * 2), CCC_OK);
    ones -= (bits_in_block * 2);
    CHECK(bs_popcount(&bs), ones);
    CHECK(bs_shiftl(&bs, (bits_in_block - 3) * 3), CCC_OK);
    ones -= ((bits_in_block - 3) * 3);
    CHECK(bs_popcount(&bs), ones);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_shiftr)
{
    bitset bs = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    CHECK(bs_popcount(&bs), 512);
    CHECK(bs_shiftr(&bs, 512), CCC_OK);
    CHECK(bs_popcount(&bs), 0);
    CHECK(bs_set_all(&bs, CCC_TRUE), CCC_OK);
    size_t const bits_in_block = sizeof(bitblock) * CHAR_BIT;
    size_t ones = 512;
    CHECK(bs_shiftr(&bs, bits_in_block), CCC_OK);
    ones -= bits_in_block;
    CHECK(bs_popcount(&bs), ones);
    CHECK(bs_shiftr(&bs, bits_in_block / 2), CCC_OK);
    ones -= (bits_in_block / 2);
    CHECK(bs_popcount(&bs), ones);
    CHECK(bs_shiftr(&bs, bits_in_block * 2), CCC_OK);
    ones -= (bits_in_block * 2);
    CHECK(bs_popcount(&bs), ones);
    CHECK(bs_shiftr(&bs, (bits_in_block - 3) * 3), CCC_OK);
    ones -= ((bits_in_block - 3) * 3);
    CHECK(bs_popcount(&bs), ones);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_subset)
{
    bitset set = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    bitset subset1
        = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    bitset subset2
        = bs_init((bitblock[bs_blocks(244)]){}, NULL, NULL, 244, 244);
    for (size_t i = 0; i < 512; i += 2)
    {
        CHECK(bs_set(&set, i, CCC_TRUE), CCC_FALSE);
        CHECK(bs_set(&subset1, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 0; i < 244; i += 2)
    {
        CHECK(bs_set(&subset2, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bs_is_subset(&set, &subset1), CCC_TRUE);
    CHECK(bs_is_subset(&set, &subset2), CCC_TRUE);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_proper_subset)
{
    bitset set = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    bitset subset1
        = bs_init((bitblock[bs_blocks(512)]){}, NULL, NULL, 512, 512);
    bitset subset2
        = bs_init((bitblock[bs_blocks(244)]){}, NULL, NULL, 244, 244);
    for (size_t i = 0; i < 512; i += 2)
    {
        CHECK(bs_set(&set, i, CCC_TRUE), CCC_FALSE);
        CHECK(bs_set(&subset1, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 0; i < 244; i += 2)
    {
        CHECK(bs_set(&subset2, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bs_is_proper_subset(&set, &subset1), CCC_FALSE);
    CHECK(bs_is_subset(&set, &subset1), CCC_TRUE);
    CHECK(bs_is_subset(&set, &subset2), CCC_TRUE);
    CHECK(bs_is_proper_subset(&set, &subset2), CCC_TRUE);
    CHECK_END_FN();
}

/* Returns if the box is valid. 1 for valid, 0 for invalid, -1 for an error */
ccc_tribool
validate_sudoku_box(int board[9][9], bitset *const row_check,
                    bitset *const col_check, size_t const row_start,
                    size_t const col_start)
{
    bitset box_check = bs_init((bitblock[bs_blocks(9)]){}, NULL, NULL, 9, 9);
    ccc_tribool was_on = CCC_FALSE;
    for (size_t r = row_start; r < row_start + 3; ++r)
    {
        for (size_t c = col_start; c < col_start + 3; ++c)
        {
            if (!board[r][c])
            {
                continue;
            }
            /* Need the zero based digit. */
            size_t const digit = board[r][c] - 1;
            was_on = bs_set(&box_check, digit, CCC_TRUE);
            if (was_on != CCC_FALSE)
            {
                goto done;
            }
            was_on = bs_set(row_check, (r * 9) + digit, CCC_TRUE);
            if (was_on != CCC_FALSE)
            {
                goto done;
            }
            was_on = bs_set(col_check, (c * 9) + digit, CCC_TRUE);
            if (was_on != CCC_FALSE)
            {
                goto done;
            }
        }
    }
done:
    switch (was_on)
    {
    case CCC_TRUE:
        return CCC_FALSE;
        break;
    case CCC_FALSE:
        return CCC_TRUE;
        break;
    case CCC_BOOL_ERR:
        return CCC_BOOL_ERR;
        break;
    default:
        return CCC_BOOL_ERR;
    }
}

/* A small problem like this is a perfect use case for a stack based bit set.
   All sizes are known at compile time meaning we get memory management for
   free and the optimal space and time complexity for this problem. */

CHECK_BEGIN_STATIC_FN(bs_test_valid_sudoku)
{
    /* clang-format off */
    int valid_board[9][9] =
    {{5,3,0, 0,7,0, 0,0,0}
    ,{6,0,0, 1,9,5, 0,0,0}
    ,{0,9,8, 0,0,0, 0,6,0}

    ,{8,0,0, 0,6,0, 0,0,3}
    ,{4,0,0, 8,0,3, 0,0,1}
    ,{7,0,0, 0,2,0, 0,0,6}

    ,{0,6,0, 0,0,0, 2,8,0}
    ,{0,0,0, 4,1,9, 0,0,5}
    ,{0,0,0, 0,8,0, 0,7,9}};
    /* clang-format on */
    bitset row_check = bs_init((bitblock[bs_blocks(9ULL * 9ULL)]){}, NULL, NULL,
                               9ULL * 9ULL, 9ULL * 9ULL);
    bitset col_check = bs_init((bitblock[bs_blocks(9ULL * 9ULL)]){}, NULL, NULL,
                               9ULL * 9ULL, 9ULL * 9ULL);
    size_t const box_step = 3;
    for (size_t row = 0; row < 9ULL; row += box_step)
    {
        for (size_t col = 0; col < 9ULL; col += box_step)
        {
            ccc_tribool const valid = validate_sudoku_box(
                valid_board, &row_check, &col_check, row, col);
            CHECK(valid, CCC_TRUE);
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_invalid_sudoku)
{
    /* clang-format off */
    int invalid_board[9][9] =
    {{8,3,0, 0,7,0, 0,0,0} /* 8 in first box top left. */
    ,{6,0,0, 1,9,5, 0,0,0}
    ,{0,9,8, 0,0,0, 0,6,0} /* 8 in first box bottom right. */

    ,{8,0,0, 0,6,0, 0,0,3} /* 8 also overlaps with 8 in top left by row. */
    ,{4,0,0, 8,0,3, 0,0,1}
    ,{7,0,0, 0,2,0, 0,0,6}

    ,{0,6,0, 0,0,0, 2,8,0}
    ,{0,0,0, 4,1,9, 0,0,5}
    ,{0,0,0, 0,8,0, 0,7,9}};
    /* clang-format on */
    bitset row_check = bs_init((bitblock[bs_blocks(9ULL * 9ULL)]){}, NULL, NULL,
                               9ULL * 9ULL, 9ULL * 9ULL);
    bitset col_check = bs_init((bitblock[bs_blocks(9ULL * 9ULL)]){}, NULL, NULL,
                               9ULL * 9ULL, 9ULL * 9ULL);
    size_t const box_step = 3;
    ccc_tribool pass = CCC_TRUE;
    for (size_t row = 0; row < 9ULL; row += box_step)
    {
        for (size_t col = 0; col < 9ULL; col += box_step)
        {
            pass = validate_sudoku_box(invalid_board, &row_check, &col_check,
                                       row, col);
            CHECK(pass != CCC_BOOL_ERR, true);
            if (!pass)
            {
                goto done;
            }
        }
    }
done:
    CHECK(pass, CCC_FALSE);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(
        bs_test_set_one(), bs_test_set_shuffled(), bs_test_set_all(),
        bs_test_set_range(), bs_test_reset(), bs_test_flip(),
        bs_test_flip_all(), bs_test_flip_range(), bs_test_reset_all(),
        bs_test_reset_range(), bs_test_any(), bs_test_all(), bs_test_none(),
        bs_test_first_trailing_one(), bs_test_first_trailing_ones(),
        bs_test_first_trailing_ones_fail(), bs_test_first_trailing_zero(),
        bs_test_first_trailing_zeros(), bs_test_first_trailing_zeros_fail(),
        bs_test_first_leading_one(), bs_test_first_leading_ones(),
        bs_test_first_leading_ones_fail(), bs_test_first_leading_zero(),
        bs_test_first_leading_zeros(), bs_test_first_leading_zeros_fail(),
        bs_test_or_same_size(), bs_test_or_diff_size(), bs_test_and_same_size(),
        bs_test_and_diff_size(), bs_test_xor_same_size(),
        bs_test_xor_diff_size(), bs_test_shiftl(), bs_test_shiftr(),
        bs_test_subset(), bs_test_proper_subset(), bs_test_valid_sudoku(),
        bs_test_invalid_sudoku());
}
