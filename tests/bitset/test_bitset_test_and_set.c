#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#define BITSET_USING_NAMESPACE_CCC
#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"

CHECK_BEGIN_STATIC_FN(bitset_test_set_one)
{
    bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    CHECK(bitset_capacity(&bs).count, 10);
    /* Was false before. */
    CHECK(bitset_set(&bs, 5, CCC_TRUE), CCC_FALSE);
    CHECK(bitset_set(&bs, 5, CCC_TRUE), CCC_TRUE);
    CHECK(bitset_popcount(&bs).count, 1);
    CHECK(bitset_set(&bs, 5, CCC_FALSE), CCC_TRUE);
    CHECK(bitset_set(&bs, 5, CCC_FALSE), CCC_FALSE);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_set_shuffled)
{
    bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    size_t const larger_prime = 11;
    for (size_t i = 0, shuf_i = larger_prime % 10; i < 10;
         ++i, shuf_i = (shuf_i + larger_prime) % 10)
    {
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_TRUE);
    }
    CHECK(bitset_popcount(&bs).count, 10);
    for (size_t i = 0; i < 10; ++i)
    {
        CHECK(bitset_test(&bs, i), CCC_TRUE);
        CHECK(bitset_test(&bs, i), CCC_TRUE);
    }
    CHECK(bitset_capacity(&bs).count, 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_set_all)
{
    bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 10);
    for (size_t i = 0; i < 10; ++i)
    {
        CHECK(bitset_test(&bs, i), CCC_TRUE);
        CHECK(bitset_test(&bs, i), CCC_TRUE);
    }
    CHECK(bitset_capacity(&bs).count, 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_set_range)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Start with a full range and reduce from the end. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        CHECK(bitset_set_range(&bs, 0, count, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, 0, count).count, count);
        CHECK(bitset_popcount(&bs).count, count);
        CHECK(bitset_set_range(&bs, 0, count, CCC_FALSE), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, 0, count).count, 0);
        CHECK(bitset_popcount(&bs).count, 0);
    }
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        CHECK(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, count);
        CHECK(bitset_popcount(&bs).count, count);
        CHECK(bitset_set_range(&bs, i, count, CCC_FALSE), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, 0);
        CHECK(bitset_popcount(&bs).count, 0);
    }
    /* Shrink range from both ends. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        CHECK(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, count);
        CHECK(bitset_popcount(&bs).count, count);
        CHECK(bitset_set_range(&bs, i, count, CCC_FALSE), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, 0);
        CHECK(bitset_popcount(&bs).count, 0);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_reset)
{
    bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    size_t const larger_prime = 11;
    for (size_t i = 0, shuf_i = larger_prime % 10; i < 10;
         ++i, shuf_i = (shuf_i + larger_prime) % 10)
    {
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_TRUE);
    }
    CHECK(bitset_reset(&bs, 9), CCC_TRUE);
    CHECK(bitset_reset(&bs, 9), CCC_FALSE);
    CHECK(bitset_popcount(&bs).count, 9);
    CHECK(bitset_capacity(&bs).count, 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_reset_all)
{
    bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    CHECK(bitset_capacity(&bs).count, 10);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 10);
    CHECK(bitset_reset_all(&bs), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_reset_range)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Start with a full range and reduce from the end. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        CHECK(bitset_set_range(&bs, 0, count, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, 0, count).count, count);
        CHECK(bitset_popcount(&bs).count, count);
        CHECK(bitset_reset_range(&bs, 0, count), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, 0, count).count, 0);
        CHECK(bitset_popcount(&bs).count, 0);
    }
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        CHECK(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, count);
        CHECK(bitset_popcount(&bs).count, count);
        CHECK(bitset_reset_range(&bs, i, count), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, 0);
        CHECK(bitset_popcount(&bs).count, 0);
    }
    /* Shrink range from both ends. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        CHECK(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, count);
        CHECK(bitset_popcount(&bs).count, count);
        CHECK(bitset_reset_range(&bs, i, count), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, 0);
        CHECK(bitset_popcount(&bs).count, 0);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_flip)
{
    bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    CHECK(bitset_capacity(&bs).count, 10);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 10);
    CHECK(bitset_flip(&bs, 9), CCC_TRUE);
    CHECK(bitset_popcount(&bs).count, 9);
    CHECK(bitset_flip(&bs, 9), CCC_FALSE);
    CHECK(bitset_popcount(&bs).count, 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_flip_all)
{
    bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    CHECK(bitset_capacity(&bs).count, 10);
    for (size_t i = 0; i < 10; i += 2)
    {
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bitset_popcount(&bs).count, 5);
    CHECK(bitset_flip_all(&bs), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 5);
    for (size_t i = 0; i < 10; ++i)
    {
        if (i % 2)
        {
            CHECK(bitset_test(&bs, i), CCC_TRUE);
            CHECK(bitset_test(&bs, i), CCC_TRUE);
        }
        else
        {
            CHECK(bitset_test(&bs, i), CCC_FALSE);
            CHECK(bitset_test(&bs, i), CCC_FALSE);
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_flip_range)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t const orignal_popcount = bitset_popcount(&bs).count;
    /* Start with a full range and reduce from the end. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        CHECK(bitset_flip_range(&bs, 0, count), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, 0, count).count, 0);
        CHECK(bitset_popcount(&bs).count, orignal_popcount - count);
        CHECK(bitset_flip_range(&bs, 0, count), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, 0, count).count, count);
        CHECK(bitset_popcount(&bs).count, orignal_popcount);
    }
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        CHECK(bitset_flip_range(&bs, i, count), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, 0);
        CHECK(bitset_popcount(&bs).count, orignal_popcount - count);
        CHECK(bitset_flip_range(&bs, i, count), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, count);
        CHECK(bitset_popcount(&bs).count, orignal_popcount);
    }
    /* Shrink range from both ends. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        CHECK(bitset_flip_range(&bs, i, count), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, 0);
        CHECK(bitset_popcount(&bs).count, orignal_popcount - count);
        CHECK(bitset_flip_range(&bs, i, count), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, count);
        CHECK(bitset_popcount(&bs).count, orignal_popcount);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_any)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t const cap = bitset_capacity(&bs).count;
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        CHECK(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, count);
        CHECK(bitset_popcount(&bs).count, count);
        CHECK(bitset_any(&bs), CCC_TRUE);
        CHECK(bitset_any_range(&bs, 0, cap), CCC_TRUE);
        CHECK(bitset_reset_range(&bs, i, count), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, 0);
        CHECK(bitset_popcount(&bs).count, 0);
        CHECK(bitset_any(&bs), CCC_FALSE);
        CHECK(bitset_any_range(&bs, 0, cap), CCC_FALSE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_none)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t const cap = bitset_capacity(&bs).count;
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        CHECK(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, count);
        CHECK(bitset_popcount(&bs).count, count);
        CHECK(bitset_none(&bs), CCC_FALSE);
        CHECK(bitset_none_range(&bs, 0, cap), CCC_FALSE);
        CHECK(bitset_reset_range(&bs, i, count), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, 0);
        CHECK(bitset_popcount(&bs).count, 0);
        CHECK(bitset_none(&bs), CCC_TRUE);
        CHECK(bitset_none_range(&bs, 0, cap), CCC_TRUE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_all)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const cap = bitset_capacity(&bs).count;
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_all(&bs), CCC_TRUE);
    CHECK(bitset_all_range(&bs, 0, cap), CCC_TRUE);
    CHECK(bitset_reset_all(&bs), CCC_RESULT_OK);
    /* Start with an almost full range and reduce by moving start forward. */
    for (size_t i = 1, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        CHECK(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, count);
        CHECK(bitset_popcount(&bs).count, count);
        CHECK(bitset_all(&bs), CCC_FALSE);
        CHECK(bitset_all_range(&bs, i, count), CCC_TRUE);
        CHECK(bitset_reset_range(&bs, i, count), CCC_RESULT_OK);
        CHECK(bitset_popcount_range(&bs, i, count).count, 0);
        CHECK(bitset_popcount(&bs).count, 0);
        CHECK(bitset_all(&bs), CCC_FALSE);
        CHECK(bitset_all_range(&bs, i, count), CCC_FALSE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_first_trailing_one)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    /* Start with an almost full range and reduce by moving start forward. */
    for (size_t i = 0, end = 512; i < end - 1; ++i)
    {
        CHECK(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
        CHECK(bitset_first_trailing_one(&bs).count, i + 1);
        CHECK(bitset_first_trailing_one_range(&bs, 0, i + 1).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_one_range(&bs, i, end - i).count, i + 1);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_first_trailing_ones)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t window = BITSET_BLOCK_BITS;
    /* Slide a group of int size as a window across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        CHECK(bitset_set_range(&bs, i, window, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_first_trailing_ones(&bs, window).count, i);
        CHECK(bitset_first_trailing_ones(&bs, window - 1).count, i);
        CHECK(bitset_first_trailing_ones(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_ones_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_ones_range(&bs, i, window, window).count,
              i);
        CHECK(bitset_first_trailing_ones_range(&bs, i + 1, window, window).error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        CHECK(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    CHECK(bitset_reset_all(&bs), CCC_RESULT_OK);
    window /= 4;
    /* Slide a very small group across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        CHECK(bitset_set_range(&bs, i, window, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_first_trailing_ones(&bs, window).count, i);
        CHECK(bitset_first_trailing_ones(&bs, window - 1).count, i);
        CHECK(bitset_first_trailing_ones(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_ones_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_ones_range(&bs, i, window, window).count,
              i);
        CHECK(bitset_first_trailing_ones_range(&bs, i + 1, window, window).error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        CHECK(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    CHECK(bitset_reset_all(&bs), CCC_RESULT_OK);
    window *= 8;
    /* Slide a very large group across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        CHECK(bitset_set_range(&bs, i, window, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_first_trailing_ones(&bs, window).count, i);
        CHECK(bitset_first_trailing_ones(&bs, window - 1).count, i);
        CHECK(bitset_first_trailing_ones(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_ones_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_ones_range(&bs, i, window, window).count,
              i);
        CHECK(bitset_first_trailing_ones_range(&bs, i + 1, window, window).error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        CHECK(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_first_trailing_ones_fail)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const end = bitset_block_count(512);
    size_t const first_half = BITSET_BLOCK_BITS / 2;
    size_t const second_half = first_half - 1;
    /* We are going to search for a group of 16 which we will be very close
       to finding every time but it will be broken by an off bit before the
       16th in a group in every block. */
    for (size_t block = 0, i = 0; block < end;
         ++block, i = block * BITSET_BLOCK_BITS)
    {
        CHECK(bitset_set_range(&bs, i, first_half, CCC_TRUE), CCC_RESULT_OK);
        CHECK(bitset_set_range(&bs, i + first_half + 1, second_half, CCC_TRUE),
              CCC_RESULT_OK);
        CHECK(bitset_first_trailing_ones_range(&bs, i, BITSET_BLOCK_BITS,
                                               first_half + 1)
                      .error
                  != CCC_RESULT_OK,
              true);
    }
    /* Then we will search for a full block worth which we will never find
       thanks to the off bit embedded in each block. */
    CHECK(bitset_first_trailing_ones(&bs, BITSET_BLOCK_BITS).error
              != CCC_RESULT_OK,
          true);
    /* Now fix the last group and we should pass. */
    CHECK(
        bitset_set(&bs, ((end - 1) * BITSET_BLOCK_BITS) + first_half, CCC_TRUE),
        CCC_FALSE);
    /* Now the solution crosses the block border from second to last to last
       block. */
    CHECK(bitset_first_trailing_ones(&bs, BITSET_BLOCK_BITS).count,
          (((end - 2) * BITSET_BLOCK_BITS) + first_half + 1));
    (void)bitset_reset_all(&bs);
    (void)bitset_set_range(&bs, 0, BITSET_BLOCK_BITS * 3, CCC_TRUE);
    CHECK(bitset_set(&bs, first_half, CCC_FALSE), CCC_TRUE);
    CHECK(bitset_first_trailing_ones_range(&bs, 0, BITSET_BLOCK_BITS,
                                           BITSET_BLOCK_BITS)
                  .error
              != CCC_RESULT_OK,
          true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_first_trailing_zero)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Start with an almost full range and reduce by moving start forward. */
    for (size_t i = 0, end = 512; i < end - 1; ++i)
    {
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
        CHECK(bitset_first_trailing_zero(&bs).count, i + 1);
        CHECK(bitset_first_trailing_zero_range(&bs, 0, i + 1).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_zero_range(&bs, i, end - i).count, i + 1);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_first_trailing_zeros)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t window = BITSET_BLOCK_BITS;
    /* Slide a group of int size as a window across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        CHECK(bitset_set_range(&bs, i, window, CCC_FALSE), CCC_RESULT_OK);
        CHECK(bitset_first_trailing_zeros(&bs, window).count, i);
        CHECK(bitset_first_trailing_zeros(&bs, window - 1).count, i);
        CHECK(bitset_first_trailing_zeros(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_zeros_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_zeros_range(&bs, i, window, window).count,
              i);
        CHECK(
            bitset_first_trailing_zeros_range(&bs, i + 1, window, window).error
                != CCC_RESULT_OK,
            true);
        /* Cleanup behind as we go. */
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    window /= 4;
    /* Slide a very small group across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        CHECK(bitset_set_range(&bs, i, window, CCC_FALSE), CCC_RESULT_OK);
        CHECK(bitset_first_trailing_zeros(&bs, window).count, i);
        CHECK(bitset_first_trailing_zeros(&bs, window - 1).count, i);
        CHECK(bitset_first_trailing_zeros(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_zeros_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_zeros_range(&bs, i, window, window).count,
              i);
        CHECK(
            bitset_first_trailing_zeros_range(&bs, i + 1, window, window).error
                != CCC_RESULT_OK,
            true);
        /* Cleanup behind as we go. */
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    window *= 8;
    /* Slide a very large group across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        CHECK(bitset_set_range(&bs, i, window, CCC_FALSE), CCC_RESULT_OK);
        CHECK(bitset_first_trailing_zeros(&bs, window).count, i);
        CHECK(bitset_first_trailing_zeros(&bs, window - 1).count, i);
        CHECK(bitset_first_trailing_zeros(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_zeros_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_trailing_zeros_range(&bs, i, window, window).count,
              i);
        CHECK(
            bitset_first_trailing_zeros_range(&bs, i + 1, window, window).error
                != CCC_RESULT_OK,
            true);
        /* Cleanup behind as we go. */
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_first_trailing_zeros_fail)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t const end = bitset_block_count(512);
    size_t const first_half = BITSET_BLOCK_BITS / 2;
    size_t const second_half = first_half - 1;
    /* We are going to search for a group of 16 which we will be very close
       to finding every time but it will be broken by an off bit before the
       16th in a group in every block. */
    for (size_t block = 0, i = 0; block < end;
         ++block, i = block * BITSET_BLOCK_BITS)
    {
        CHECK(bitset_set_range(&bs, i, first_half, CCC_FALSE), CCC_RESULT_OK);
        CHECK(bitset_set_range(&bs, i + first_half + 1, second_half, CCC_FALSE),
              CCC_RESULT_OK);
        CHECK(bitset_first_trailing_zeros_range(&bs, i, BITSET_BLOCK_BITS,
                                                first_half + 1)
                      .error
                  != CCC_RESULT_OK,
              true);
    }
    /* Then we will search for a full block worth which we will never find
       thanks to the off bit embedded in each block. */
    CHECK(bitset_first_trailing_zeros(&bs, BITSET_BLOCK_BITS).error
              != CCC_RESULT_OK,
          true);
    /* Now fix the last group and we should pass. */
    CHECK(bitset_set(&bs, ((end - 1) * BITSET_BLOCK_BITS) + first_half,
                     CCC_FALSE),
          CCC_TRUE);
    /* Now the solution crosses the block border from second to last to last
       block. */
    CHECK(bitset_first_trailing_zeros(&bs, BITSET_BLOCK_BITS).count,
          (((end - 2) * BITSET_BLOCK_BITS) + first_half + 1));
    (void)bitset_reset_all(&bs);
    (void)bitset_set_range(&bs, 0, BITSET_BLOCK_BITS * 3, CCC_FALSE);
    CHECK(bitset_set(&bs, first_half, CCC_TRUE), CCC_FALSE);
    CHECK(bitset_first_trailing_zeros_range(&bs, 0, BITSET_BLOCK_BITS,
                                            BITSET_BLOCK_BITS)
                  .error
              != CCC_RESULT_OK,
          true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_first_leading_one)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t const last_i = 511;
    /* Start with an almost full range and reduce by moving start backwards. */
    for (size_t i = last_i; i > 1; --i)
    {
        CHECK(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
        CHECK(bitset_first_leading_one(&bs).count, i - 1);
        CHECK(bitset_first_leading_one_range(&bs, last_i, 512 - i).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_one_range(&bs, i, i + 1).count, i - 1);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_first_leading_ones)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t window = BITSET_BLOCK_BITS;
    /* Slide a group of int size as a window across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        CHECK(bitset_set_range(&bs, i - window + 1, window, CCC_TRUE),
              CCC_RESULT_OK);
        CHECK(bitset_first_leading_ones(&bs, window).count, i);
        CHECK(bitset_first_leading_ones(&bs, window - 1).count, i);
        CHECK(bitset_first_leading_ones(&bs, window + 1).error != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_ones_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_ones_range(&bs, i, window, window).count, i);
        CHECK(bitset_first_leading_ones_range(&bs, i + 1, window, window).error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        CHECK(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    CHECK(bitset_reset_all(&bs), CCC_RESULT_OK);
    window /= 4;
    /* Slide a very small group across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        CHECK(bitset_set_range(&bs, i - window + 1, window, CCC_TRUE),
              CCC_RESULT_OK);
        CHECK(bitset_first_leading_ones(&bs, window).count, i);
        CHECK(bitset_first_leading_ones(&bs, window - 1).count, i);
        CHECK(bitset_first_leading_ones(&bs, window + 1).error != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_ones_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_ones_range(&bs, i, window, window).count, i);
        CHECK(bitset_first_leading_ones_range(&bs, i + 1, window, window).error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        CHECK(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    CHECK(bitset_reset_all(&bs), CCC_RESULT_OK);
    window *= 8;
    /* Slide a very large group across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        CHECK(bitset_set_range(&bs, i - window + 1, window, CCC_TRUE),
              CCC_RESULT_OK);
        CHECK(bitset_first_leading_ones(&bs, window).count, i);
        CHECK(bitset_first_leading_ones(&bs, window - 1).count, i);
        CHECK(bitset_first_leading_ones(&bs, window + 1).error != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_ones_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_ones_range(&bs, i, window, window).count, i);
        CHECK(bitset_first_leading_ones_range(&bs, i + 1, window, window).error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        CHECK(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_first_leading_ones_fail)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const first_half = BITSET_BLOCK_BITS / 2;
    size_t const second_half = first_half - 1;
    /* We are going to search for a group of 17 which we will be very close
       to finding every time but it will be broken by an off bit before the
       17th in a group in every block. */
    for (size_t block = bitset_block_count(512), i = 511; block--;
         i -= BITSET_BLOCK_BITS)
    {
        CHECK(bitset_set_range(&bs, (block * BITSET_BLOCK_BITS), first_half,
                               CCC_TRUE),
              CCC_RESULT_OK);
        CHECK(bitset_set_range(&bs,
                               (block * BITSET_BLOCK_BITS) + first_half + 1,
                               second_half, CCC_TRUE),
              CCC_RESULT_OK);
        CHECK(bitset_first_leading_ones_range(&bs, i, BITSET_BLOCK_BITS,
                                              first_half + 1)
                      .error
                  != CCC_RESULT_OK,
              true);
    }
    /* Then we will search for a full block worth which we will never find
       thanks to the off bit embedded in each block. */
    CHECK(bitset_first_leading_ones(&bs, BITSET_BLOCK_BITS).error
              != CCC_RESULT_OK,
          true);
    /* Now fix the last group and we should pass. */
    CHECK(bitset_set(&bs, first_half, CCC_TRUE), CCC_FALSE);
    /* Now the solution crosses the block border from second to last to last
       block. */
    CHECK(bitset_first_leading_ones(&bs, BITSET_BLOCK_BITS).count,
          BITSET_BLOCK_BITS + first_half - 1);
    (void)bitset_reset_all(&bs);
    (void)bitset_set_range(&bs, 512 - (BITSET_BLOCK_BITS * 3),
                           BITSET_BLOCK_BITS * 3, CCC_TRUE);
    CHECK(bitset_set(&bs, 512 - first_half, CCC_FALSE), CCC_TRUE);
    CHECK(bitset_first_leading_ones_range(&bs, 511, BITSET_BLOCK_BITS,
                                          BITSET_BLOCK_BITS)
                  .error
              != CCC_RESULT_OK,
          true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_first_leading_zero)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const last_i = 511;
    /* Start with an almost full range and reduce by moving start backwards. */
    for (size_t i = last_i; i > 1; --i)
    {
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
        CHECK(bitset_first_leading_zero(&bs).count, i - 1);
        CCC_Count const r
            = bitset_first_leading_zero_range(&bs, last_i, 512 - i);
        CHECK(r.error != CCC_RESULT_OK, true);
        CHECK(bitset_first_leading_zero_range(&bs, i, i + 1).count, i - 1);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_first_leading_zeros)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t window = BITSET_BLOCK_BITS;
    /* Slide a group of int size as a window across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        CHECK(bitset_set_range(&bs, i - window + 1, window, CCC_FALSE),
              CCC_RESULT_OK);
        CHECK(bitset_first_leading_zeros(&bs, window).count, i);
        CHECK(bitset_first_leading_zeros(&bs, window - 1).count, i);
        CHECK(bitset_first_leading_zeros(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_zeros_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_zeros_range(&bs, i, window, window).count,
              i);
        CHECK(bitset_first_leading_zeros_range(&bs, i + 1, window, window).error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    window /= 4;
    /* Slide a very small group across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        CHECK(bitset_set_range(&bs, i - window + 1, window, CCC_FALSE),
              CCC_RESULT_OK);
        CHECK(bitset_first_leading_zeros(&bs, window).count, i);
        CHECK(bitset_first_leading_zeros(&bs, window - 1).count, i);
        CHECK(bitset_first_leading_zeros(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_zeros_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_zeros_range(&bs, i, window, window).count,
              i);
        CHECK(bitset_first_leading_zeros_range(&bs, i + 1, window, window).error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    window *= 8;
    /* Slide a very large group across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        CHECK(bitset_set_range(&bs, i - window + 1, window, CCC_FALSE),
              CCC_RESULT_OK);
        CHECK(bitset_first_leading_zeros(&bs, window).count, i);
        CHECK(bitset_first_leading_zeros(&bs, window - 1).count, i);
        CHECK(bitset_first_leading_zeros(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_zeros_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        CHECK(bitset_first_leading_zeros_range(&bs, i, window, window).count,
              i);
        CHECK(bitset_first_leading_zeros_range(&bs, i + 1, window, window).error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        CHECK(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_first_leading_zeros_fail)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t const first_half = BITSET_BLOCK_BITS / 2;
    size_t const second_half = first_half - 1;
    /* We are going to search for a group of 17 which we will be very close
       to finding every time but it will be broken by an off bit before the
       17th in a group in every block. */
    for (size_t block = bitset_block_count(512), i = 511; block--;
         i -= BITSET_BLOCK_BITS)
    {
        CHECK(bitset_set_range(&bs, (block * BITSET_BLOCK_BITS), first_half,
                               CCC_FALSE),
              CCC_RESULT_OK);
        CHECK(bitset_set_range(&bs,
                               (block * BITSET_BLOCK_BITS) + first_half + 1,
                               second_half, CCC_FALSE),
              CCC_RESULT_OK);
        CHECK(bitset_first_leading_zeros_range(&bs, i, BITSET_BLOCK_BITS,
                                               first_half + 1)
                      .error
                  != CCC_RESULT_OK,
              true);
    }
    /* Then we will search for a full block worth which we will never find
       thanks to the off bit embedded in each block. */
    CHECK(bitset_first_leading_zeros(&bs, BITSET_BLOCK_BITS).error
              != CCC_RESULT_OK,
          true);
    /* Now fix the last group and we should pass. */
    CHECK(bitset_set(&bs, first_half, CCC_FALSE), CCC_TRUE);
    /* Now the solution crosses the block border from second to last to last
       block. */
    CHECK(bitset_first_leading_zeros(&bs, BITSET_BLOCK_BITS).count,
          BITSET_BLOCK_BITS + first_half - 1);
    (void)bitset_reset_all(&bs);
    (void)bitset_set_range(&bs, 512 - (BITSET_BLOCK_BITS * 3),
                           BITSET_BLOCK_BITS * 3, CCC_FALSE);
    CHECK(bitset_set(&bs, 512 - first_half, CCC_TRUE), CCC_FALSE);
    CHECK(bitset_first_leading_zeros_range(&bs, 511, BITSET_BLOCK_BITS,
                                           BITSET_BLOCK_BITS)
                  .error
              != CCC_RESULT_OK,
          true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_or_same_size)
{
    bitset src = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    bitset dst = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const size = 512;
    for (size_t i = 0; i < size; i += 2)
    {
        CHECK(bitset_set(&dst, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 1; i < size; i += 2)
    {
        CHECK(bitset_set(&src, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bitset_popcount(&src).count, size / 2);
    CHECK(bitset_popcount(&dst).count, size / 2);
    CHECK(bitset_or(&dst, &src), CCC_RESULT_OK);
    CHECK(bitset_popcount(&dst).count, size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_or_diff_size)
{
    bitset dst = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Make it slightly harder by not ending on a perfect block boundary. */
    bitset src = bitset_initialize(bitset_blocks(244), NULL, NULL, 244);
    CHECK(bitset_set_all(&src, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_popcount(&src).count, 244);
    CHECK(bitset_popcount(&dst).count, 0);
    CHECK(bitset_or(&dst, &src), CCC_RESULT_OK);
    CHECK(bitset_popcount(&dst).count, 244);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_and_same_size)
{
    bitset src = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    bitset dst = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const size = 512;
    for (size_t i = 0; i < size; i += 2)
    {
        CHECK(bitset_set(&dst, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 1; i < size; i += 2)
    {
        CHECK(bitset_set(&src, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bitset_popcount(&src).count, size / 2);
    CHECK(bitset_popcount(&dst).count, size / 2);
    CHECK(bitset_and(&dst, &src), CCC_RESULT_OK);
    CHECK(bitset_popcount(&dst).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_and_diff_size)
{
    bitset dst = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Make it slightly harder by not ending on a perfect block boundary. */
    bitset src = bitset_initialize(bitset_blocks(244), NULL, NULL, 244);
    CHECK(bitset_set_all(&dst, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_set_all(&src, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_popcount(&dst).count, 512);
    CHECK(bitset_popcount(&src).count, 244);
    CHECK(bitset_and(&dst, &src), CCC_RESULT_OK);
    CHECK(bitset_popcount(&dst).count, 244);
    CHECK(bitset_count(&dst).count, 512);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_xor_same_size)
{
    bitset src = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    bitset dst = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const size = 512;
    for (size_t i = 0; i < size; i += 2)
    {
        CHECK(bitset_set(&dst, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 1; i < size; i += 2)
    {
        CHECK(bitset_set(&src, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bitset_popcount(&src).count, size / 2);
    CHECK(bitset_popcount(&dst).count, size / 2);
    CHECK(bitset_xor(&dst, &src), CCC_RESULT_OK);
    CHECK(bitset_popcount(&dst).count, size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_xor_diff_size)
{
    bitset dst = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Make it slightly harder by not ending on a perfect block boundary. */
    bitset src = bitset_initialize(bitset_blocks(244), NULL, NULL, 244);
    CHECK(bitset_set_all(&dst, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_set_all(&src, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_popcount(&dst).count, 512);
    CHECK(bitset_popcount(&src).count, 244);
    CHECK(bitset_xor(&dst, &src), CCC_RESULT_OK);
    CHECK(bitset_popcount(&dst).count, 512 - 244);
    CHECK(bitset_count(&dst).count, 512);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_shiftl)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 512);
    CHECK(bitset_shiftl(&bs, 512), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 0);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t ones = 512;
    CHECK(bitset_shiftl(&bs, BITSET_BLOCK_BITS), CCC_RESULT_OK);
    CHECK(bitset_popcount_range(&bs, 0, BITSET_BLOCK_BITS).count, 0);
    ones -= BITSET_BLOCK_BITS;
    CHECK(bitset_popcount(&bs).count, ones);
    CHECK(bitset_shiftl(&bs, BITSET_BLOCK_BITS / 2), CCC_RESULT_OK);
    ones -= (BITSET_BLOCK_BITS / 2);
    CHECK(bitset_popcount(&bs).count, ones);
    CHECK(bitset_shiftl(&bs, BITSET_BLOCK_BITS * 2), CCC_RESULT_OK);
    ones -= (BITSET_BLOCK_BITS * 2);
    CHECK(bitset_popcount(&bs).count, ones);
    CHECK(bitset_shiftl(&bs, (BITSET_BLOCK_BITS - 3) * 3), CCC_RESULT_OK);
    ones -= ((BITSET_BLOCK_BITS - 3) * 3);
    CHECK(bitset_popcount(&bs).count, ones);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_shiftl_edgecase)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 512);
    CHECK(bitset_shiftl(&bs, 510), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_shiftl_edgecase_small)
{
    bitset bs = bitset_initialize(bitset_blocks(8), NULL, NULL, 8);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 8);
    CHECK(bitset_shiftl(&bs, 7), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_shiftr)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 512);
    CHECK(bitset_shiftr(&bs, 512), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 0);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t ones = 512;
    CHECK(bitset_shiftr(&bs, BITSET_BLOCK_BITS), CCC_RESULT_OK);
    CHECK(bitset_popcount_range(&bs, 512 - BITSET_BLOCK_BITS, BITSET_BLOCK_BITS)
              .count,
          0);
    ones -= BITSET_BLOCK_BITS;
    CHECK(bitset_popcount(&bs).count, ones);
    CHECK(bitset_shiftr(&bs, BITSET_BLOCK_BITS / 2), CCC_RESULT_OK);
    ones -= (BITSET_BLOCK_BITS / 2);
    CHECK(bitset_popcount(&bs).count, ones);
    CHECK(bitset_shiftr(&bs, BITSET_BLOCK_BITS * 2), CCC_RESULT_OK);
    ones -= (BITSET_BLOCK_BITS * 2);
    CHECK(bitset_popcount(&bs).count, ones);
    CHECK(bitset_shiftr(&bs, (BITSET_BLOCK_BITS - 3) * 3), CCC_RESULT_OK);
    ones -= ((BITSET_BLOCK_BITS - 3) * 3);
    CHECK(bitset_popcount(&bs).count, ones);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_shiftr_edgecase)
{
    bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 512);
    CHECK(bitset_shiftr(&bs, 510), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_shiftr_edgecase_small)
{
    bitset bs = bitset_initialize(bitset_blocks(8), NULL, NULL, 8);
    CHECK(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 8);
    CHECK(bitset_shiftr(&bs, 7), CCC_RESULT_OK);
    CHECK(bitset_popcount(&bs).count, 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_subset)
{
    bitset set = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    bitset subset1 = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    bitset subset2 = bitset_initialize(bitset_blocks(244), NULL, NULL, 244);
    for (size_t i = 0; i < 512; i += 2)
    {
        CHECK(bitset_set(&set, i, CCC_TRUE), CCC_FALSE);
        CHECK(bitset_set(&subset1, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 0; i < 244; i += 2)
    {
        CHECK(bitset_set(&subset2, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bitset_is_subset(&set, &subset1), CCC_TRUE);
    CHECK(bitset_is_subset(&set, &subset2), CCC_TRUE);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_proper_subset)
{
    bitset set = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    bitset subset1 = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    bitset subset2 = bitset_initialize(bitset_blocks(244), NULL, NULL, 244);
    for (size_t i = 0; i < 512; i += 2)
    {
        CHECK(bitset_set(&set, i, CCC_TRUE), CCC_FALSE);
        CHECK(bitset_set(&subset1, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 0; i < 244; i += 2)
    {
        CHECK(bitset_set(&subset2, i, CCC_TRUE), CCC_FALSE);
    }
    CHECK(bitset_is_proper_subset(&set, &subset1), CCC_FALSE);
    CHECK(bitset_is_subset(&set, &subset1), CCC_TRUE);
    CHECK(bitset_is_subset(&set, &subset2), CCC_TRUE);
    CHECK(bitset_is_proper_subset(&set, &subset2), CCC_TRUE);
    CHECK_END_FN();
}

/* Returns if the box is valid. 1 for valid, 0 for invalid, -1 for an error */
CCC_Tribool
validate_sudoku_box(int board[9][9], bitset *const row_check,
                    bitset *const col_check, size_t const row_start,
                    size_t const col_start)
{
    bitset box_check = bitset_initialize(bitset_blocks(9), NULL, NULL, 9);
    CCC_Tribool was_on = CCC_FALSE;
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
            was_on = bitset_set(&box_check, digit, CCC_TRUE);
            if (was_on != CCC_FALSE)
            {
                goto done;
            }
            was_on = bitset_set(row_check, (r * 9) + digit, CCC_TRUE);
            if (was_on != CCC_FALSE)
            {
                goto done;
            }
            was_on = bitset_set(col_check, (c * 9) + digit, CCC_TRUE);
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
        case CCC_TRIBOOL_ERROR:
            return CCC_TRIBOOL_ERROR;
            break;
        default:
            return CCC_TRIBOOL_ERROR;
    }
}

/* A small problem like this is a perfect use case for a stack based bit set.
   All sizes are known at compile time meaning we get memory management for
   free and the optimal space and time complexity for this problem. */

CHECK_BEGIN_STATIC_FN(bitset_test_valid_sudoku)
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
    bitset row_check
        = bitset_initialize(bitset_blocks(9L * 9L), NULL, NULL, 9L * 9L);
    bitset col_check
        = bitset_initialize(bitset_blocks(9L * 9L), NULL, NULL, 9L * 9L);
    size_t const box_step = 3;
    for (size_t row = 0; row < 9; row += box_step)
    {
        for (size_t col = 0; col < 9; col += box_step)
        {
            CCC_Tribool const valid = validate_sudoku_box(
                valid_board, &row_check, &col_check, row, col);
            CHECK(valid, CCC_TRUE);
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_invalid_sudoku)
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
    bitset row_check
        = bitset_initialize(bitset_blocks(9L * 9L), NULL, NULL, 9L * 9L);
    bitset col_check
        = bitset_initialize(bitset_blocks(9L * 9L), NULL, NULL, 9L * 9L);
    size_t const box_step = 3;
    CCC_Tribool pass = CCC_TRUE;
    for (size_t row = 0; row < 9; row += box_step)
    {
        for (size_t col = 0; col < 9; col += box_step)
        {
            pass = validate_sudoku_box(invalid_board, &row_check, &col_check,
                                       row, col);
            CHECK(pass != CCC_TRIBOOL_ERROR, true);
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
        bitset_test_set_one(), bitset_test_set_shuffled(),
        bitset_test_set_all(), bitset_test_set_range(), bitset_test_reset(),
        bitset_test_flip(), bitset_test_flip_all(), bitset_test_flip_range(),
        bitset_test_reset_all(), bitset_test_reset_range(), bitset_test_any(),
        bitset_test_all(), bitset_test_none(), bitset_test_first_trailing_one(),
        bitset_test_first_trailing_ones(),
        bitset_test_first_trailing_ones_fail(),
        bitset_test_first_trailing_zero(), bitset_test_first_trailing_zeros(),
        bitset_test_first_trailing_zeros_fail(),
        bitset_test_first_leading_one(), bitset_test_first_leading_ones(),
        bitset_test_first_leading_ones_fail(), bitset_test_first_leading_zero(),
        bitset_test_first_leading_zeros(),
        bitset_test_first_leading_zeros_fail(), bitset_test_or_same_size(),
        bitset_test_or_diff_size(), bitset_test_and_same_size(),
        bitset_test_and_diff_size(), bitset_test_xor_same_size(),
        bitset_test_xor_diff_size(), bitset_test_shiftl(), bitset_test_shiftr(),
        bitset_test_shiftl_edgecase(), bitset_test_shiftr_edgecase(),
        bitset_test_shiftl_edgecase_small(),
        bitset_test_shiftr_edgecase_small(), bitset_test_subset(),
        bitset_test_proper_subset(), bitset_test_valid_sudoku(),
        bitset_test_invalid_sudoku());
}
