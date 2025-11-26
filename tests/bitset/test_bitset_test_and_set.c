#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#define BITSET_USING_NAMESPACE_CCC
#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"

check_static_begin(bitset_test_set_one)
{
    Bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    check(bitset_capacity(&bs).count, 10);
    /* Was false before. */
    check(bitset_set(&bs, 5, CCC_TRUE), CCC_FALSE);
    check(bitset_set(&bs, 5, CCC_TRUE), CCC_TRUE);
    check(bitset_popcount(&bs).count, 1);
    check(bitset_set(&bs, 5, CCC_FALSE), CCC_TRUE);
    check(bitset_set(&bs, 5, CCC_FALSE), CCC_FALSE);
    check_end();
}

check_static_begin(bitset_test_set_shuffled)
{
    Bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    size_t const larger_prime = 11;
    for (size_t i = 0, shuf_i = larger_prime % 10; i < 10;
         ++i, shuf_i = (shuf_i + larger_prime) % 10)
    {
        check(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
        check(bitset_set(&bs, i, CCC_TRUE), CCC_TRUE);
    }
    check(bitset_popcount(&bs).count, 10);
    for (size_t i = 0; i < 10; ++i)
    {
        check(bitset_test(&bs, i), CCC_TRUE);
        check(bitset_test(&bs, i), CCC_TRUE);
    }
    check(bitset_capacity(&bs).count, 10);
    check_end();
}

check_static_begin(bitset_test_set_all)
{
    Bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 10);
    for (size_t i = 0; i < 10; ++i)
    {
        check(bitset_test(&bs, i), CCC_TRUE);
        check(bitset_test(&bs, i), CCC_TRUE);
    }
    check(bitset_capacity(&bs).count, 10);
    check_end();
}

check_static_begin(bitset_test_set_range)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Start with a full range and reduce from the end. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        check(bitset_set_range(&bs, 0, count, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, 0, count).count, count);
        check(bitset_popcount(&bs).count, count);
        check(bitset_set_range(&bs, 0, count, CCC_FALSE), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, 0, count).count, 0);
        check(bitset_popcount(&bs).count, 0);
    }
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        check(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, count);
        check(bitset_popcount(&bs).count, count);
        check(bitset_set_range(&bs, i, count, CCC_FALSE), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, 0);
        check(bitset_popcount(&bs).count, 0);
    }
    /* Shrink range from both ends. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        check(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, count);
        check(bitset_popcount(&bs).count, count);
        check(bitset_set_range(&bs, i, count, CCC_FALSE), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, 0);
        check(bitset_popcount(&bs).count, 0);
    }
    check_end();
}

check_static_begin(bitset_test_reset)
{
    Bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    size_t const larger_prime = 11;
    for (size_t i = 0, shuf_i = larger_prime % 10; i < 10;
         ++i, shuf_i = (shuf_i + larger_prime) % 10)
    {
        check(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
        check(bitset_set(&bs, i, CCC_TRUE), CCC_TRUE);
    }
    check(bitset_reset(&bs, 9), CCC_TRUE);
    check(bitset_reset(&bs, 9), CCC_FALSE);
    check(bitset_popcount(&bs).count, 9);
    check(bitset_capacity(&bs).count, 10);
    check_end();
}

check_static_begin(bitset_test_reset_all)
{
    Bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    check(bitset_capacity(&bs).count, 10);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 10);
    check(bitset_reset_all(&bs), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 0);
    check_end();
}

check_static_begin(bitset_test_reset_range)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Start with a full range and reduce from the end. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        check(bitset_set_range(&bs, 0, count, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, 0, count).count, count);
        check(bitset_popcount(&bs).count, count);
        check(bitset_reset_range(&bs, 0, count), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, 0, count).count, 0);
        check(bitset_popcount(&bs).count, 0);
    }
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        check(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, count);
        check(bitset_popcount(&bs).count, count);
        check(bitset_reset_range(&bs, i, count), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, 0);
        check(bitset_popcount(&bs).count, 0);
    }
    /* Shrink range from both ends. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        check(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, count);
        check(bitset_popcount(&bs).count, count);
        check(bitset_reset_range(&bs, i, count), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, 0);
        check(bitset_popcount(&bs).count, 0);
    }
    check_end();
}

check_static_begin(bitset_test_flip)
{
    Bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    check(bitset_capacity(&bs).count, 10);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 10);
    check(bitset_flip(&bs, 9), CCC_TRUE);
    check(bitset_popcount(&bs).count, 9);
    check(bitset_flip(&bs, 9), CCC_FALSE);
    check(bitset_popcount(&bs).count, 10);
    check_end();
}

check_static_begin(bitset_test_flip_all)
{
    Bitset bs = bitset_initialize(bitset_blocks(10), NULL, NULL, 10);
    check(bitset_capacity(&bs).count, 10);
    for (size_t i = 0; i < 10; i += 2)
    {
        check(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    check(bitset_popcount(&bs).count, 5);
    check(bitset_flip_all(&bs), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 5);
    for (size_t i = 0; i < 10; ++i)
    {
        if (i % 2)
        {
            check(bitset_test(&bs, i), CCC_TRUE);
            check(bitset_test(&bs, i), CCC_TRUE);
        }
        else
        {
            check(bitset_test(&bs, i), CCC_FALSE);
            check(bitset_test(&bs, i), CCC_FALSE);
        }
    }
    check_end();
}

check_static_begin(bitset_test_flip_range)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t const orignal_popcount = bitset_popcount(&bs).count;
    /* Start with a full range and reduce from the end. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        check(bitset_flip_range(&bs, 0, count), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, 0, count).count, 0);
        check(bitset_popcount(&bs).count, orignal_popcount - count);
        check(bitset_flip_range(&bs, 0, count), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, 0, count).count, count);
        check(bitset_popcount(&bs).count, orignal_popcount);
    }
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0; i < 512; ++i)
    {
        size_t const count = 512 - i;
        check(bitset_flip_range(&bs, i, count), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, 0);
        check(bitset_popcount(&bs).count, orignal_popcount - count);
        check(bitset_flip_range(&bs, i, count), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, count);
        check(bitset_popcount(&bs).count, orignal_popcount);
    }
    /* Shrink range from both ends. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        check(bitset_flip_range(&bs, i, count), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, 0);
        check(bitset_popcount(&bs).count, orignal_popcount - count);
        check(bitset_flip_range(&bs, i, count), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, count);
        check(bitset_popcount(&bs).count, orignal_popcount);
    }
    check_end();
}

check_static_begin(bitset_test_any)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t const cap = bitset_capacity(&bs).count;
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        check(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, count);
        check(bitset_popcount(&bs).count, count);
        check(bitset_any(&bs), CCC_TRUE);
        check(bitset_any_range(&bs, 0, cap), CCC_TRUE);
        check(bitset_reset_range(&bs, i, count), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, 0);
        check(bitset_popcount(&bs).count, 0);
        check(bitset_any(&bs), CCC_FALSE);
        check(bitset_any_range(&bs, 0, cap), CCC_FALSE);
    }
    check_end();
}

check_static_begin(bitset_test_none)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t const cap = bitset_capacity(&bs).count;
    /* Start with a full range and reduce by moving start forward. */
    for (size_t i = 0, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        check(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, count);
        check(bitset_popcount(&bs).count, count);
        check(bitset_none(&bs), CCC_FALSE);
        check(bitset_none_range(&bs, 0, cap), CCC_FALSE);
        check(bitset_reset_range(&bs, i, count), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, 0);
        check(bitset_popcount(&bs).count, 0);
        check(bitset_none(&bs), CCC_TRUE);
        check(bitset_none_range(&bs, 0, cap), CCC_TRUE);
    }
    check_end();
}

check_static_begin(bitset_test_all)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const cap = bitset_capacity(&bs).count;
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_all(&bs), CCC_TRUE);
    check(bitset_all_range(&bs, 0, cap), CCC_TRUE);
    check(bitset_reset_all(&bs), CCC_RESULT_OK);
    /* Start with an almost full range and reduce by moving start forward. */
    for (size_t i = 1, end = 512; i < end; ++i, --end)
    {
        size_t const count = end - i;
        check(bitset_set_range(&bs, i, count, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, count);
        check(bitset_popcount(&bs).count, count);
        check(bitset_all(&bs), CCC_FALSE);
        check(bitset_all_range(&bs, i, count), CCC_TRUE);
        check(bitset_reset_range(&bs, i, count), CCC_RESULT_OK);
        check(bitset_popcount_range(&bs, i, count).count, 0);
        check(bitset_popcount(&bs).count, 0);
        check(bitset_all(&bs), CCC_FALSE);
        check(bitset_all_range(&bs, i, count), CCC_FALSE);
    }
    check_end();
}

check_static_begin(bitset_test_first_trailing_one)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    /* Start with an almost full range and reduce by moving start forward. */
    for (size_t i = 0, end = 512; i < end - 1; ++i)
    {
        check(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
        check(bitset_first_trailing_one(&bs).count, i + 1);
        check(bitset_first_trailing_one_range(&bs, 0, i + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_one_range(&bs, i, end - i).count, i + 1);
    }
    check_end();
}

check_static_begin(bitset_test_first_trailing_ones)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t window = BITSET_BLOCK_BITS;
    /* Slide a group of int size as a window across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        check(bitset_set_range(&bs, i, window, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_first_trailing_ones(&bs, window).count, i);
        check(bitset_first_trailing_ones(&bs, window - 1).count, i);
        check(bitset_first_trailing_ones(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_ones_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_ones_range(&bs, i, window, window).count,
              i);
        check(bitset_first_trailing_ones_range(&bs, i + 1, window, window).error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        check(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    check(bitset_reset_all(&bs), CCC_RESULT_OK);
    window /= 4;
    /* Slide a very small group across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        check(bitset_set_range(&bs, i, window, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_first_trailing_ones(&bs, window).count, i);
        check(bitset_first_trailing_ones(&bs, window - 1).count, i);
        check(bitset_first_trailing_ones(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_ones_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_ones_range(&bs, i, window, window).count,
              i);
        check(bitset_first_trailing_ones_range(&bs, i + 1, window, window).error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        check(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    check(bitset_reset_all(&bs), CCC_RESULT_OK);
    window *= 8;
    /* Slide a very large group across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        check(bitset_set_range(&bs, i, window, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_first_trailing_ones(&bs, window).count, i);
        check(bitset_first_trailing_ones(&bs, window - 1).count, i);
        check(bitset_first_trailing_ones(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_ones_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_ones_range(&bs, i, window, window).count,
              i);
        check(bitset_first_trailing_ones_range(&bs, i + 1, window, window).error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        check(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    check_end();
}

check_static_begin(bitset_test_first_trailing_ones_fail)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const end = bitset_block_count(512);
    size_t const first_half = BITSET_BLOCK_BITS / 2;
    size_t const second_half = first_half - 1;
    /* We are going to search for a group of 16 which we will be very close
       to finding every time but it will be broken by an off bit before the
       16th in a group in every block. */
    for (size_t block = 0, i = 0; block < end;
         ++block, i = block * BITSET_BLOCK_BITS)
    {
        check(bitset_set_range(&bs, i, first_half, CCC_TRUE), CCC_RESULT_OK);
        check(bitset_set_range(&bs, i + first_half + 1, second_half, CCC_TRUE),
              CCC_RESULT_OK);
        check(bitset_first_trailing_ones_range(&bs, i, BITSET_BLOCK_BITS,
                                               first_half + 1)
                      .error
                  != CCC_RESULT_OK,
              true);
    }
    /* Then we will search for a full block worth which we will never find
       thanks to the off bit embedded in each block. */
    check(bitset_first_trailing_ones(&bs, BITSET_BLOCK_BITS).error
              != CCC_RESULT_OK,
          true);
    /* Now fix the last group and we should pass. */
    check(
        bitset_set(&bs, ((end - 1) * BITSET_BLOCK_BITS) + first_half, CCC_TRUE),
        CCC_FALSE);
    /* Now the solution crosses the block border from second to last to last
       block. */
    check(bitset_first_trailing_ones(&bs, BITSET_BLOCK_BITS).count,
          (((end - 2) * BITSET_BLOCK_BITS) + first_half + 1));
    (void)bitset_reset_all(&bs);
    (void)bitset_set_range(&bs, 0, BITSET_BLOCK_BITS * 3, CCC_TRUE);
    check(bitset_set(&bs, first_half, CCC_FALSE), CCC_TRUE);
    check(bitset_first_trailing_ones_range(&bs, 0, BITSET_BLOCK_BITS,
                                           BITSET_BLOCK_BITS)
                  .error
              != CCC_RESULT_OK,
          true);
    check_end();
}

check_static_begin(bitset_test_first_trailing_zero)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Start with an almost full range and reduce by moving start forward. */
    for (size_t i = 0, end = 512; i < end - 1; ++i)
    {
        check(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
        check(bitset_first_trailing_zero(&bs).count, i + 1);
        check(bitset_first_trailing_zero_range(&bs, 0, i + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_zero_range(&bs, i, end - i).count, i + 1);
    }
    check_end();
}

check_static_begin(bitset_test_first_trailing_zeros)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t window = BITSET_BLOCK_BITS;
    /* Slide a group of int size as a window across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        check(bitset_set_range(&bs, i, window, CCC_FALSE), CCC_RESULT_OK);
        check(bitset_first_trailing_zeros(&bs, window).count, i);
        check(bitset_first_trailing_zeros(&bs, window - 1).count, i);
        check(bitset_first_trailing_zeros(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_zeros_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_zeros_range(&bs, i, window, window).count,
              i);
        check(
            bitset_first_trailing_zeros_range(&bs, i + 1, window, window).error
                != CCC_RESULT_OK,
            true);
        /* Cleanup behind as we go. */
        check(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    window /= 4;
    /* Slide a very small group across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        check(bitset_set_range(&bs, i, window, CCC_FALSE), CCC_RESULT_OK);
        check(bitset_first_trailing_zeros(&bs, window).count, i);
        check(bitset_first_trailing_zeros(&bs, window - 1).count, i);
        check(bitset_first_trailing_zeros(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_zeros_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_zeros_range(&bs, i, window, window).count,
              i);
        check(
            bitset_first_trailing_zeros_range(&bs, i + 1, window, window).error
                != CCC_RESULT_OK,
            true);
        /* Cleanup behind as we go. */
        check(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    window *= 8;
    /* Slide a very large group across the set. */
    for (size_t i = 0; i < (512 - window - 1); ++i)
    {
        check(bitset_set_range(&bs, i, window, CCC_FALSE), CCC_RESULT_OK);
        check(bitset_first_trailing_zeros(&bs, window).count, i);
        check(bitset_first_trailing_zeros(&bs, window - 1).count, i);
        check(bitset_first_trailing_zeros(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_zeros_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_trailing_zeros_range(&bs, i, window, window).count,
              i);
        check(
            bitset_first_trailing_zeros_range(&bs, i + 1, window, window).error
                != CCC_RESULT_OK,
            true);
        /* Cleanup behind as we go. */
        check(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    check_end();
}

check_static_begin(bitset_test_first_trailing_zeros_fail)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t const end = bitset_block_count(512);
    size_t const first_half = BITSET_BLOCK_BITS / 2;
    size_t const second_half = first_half - 1;
    /* We are going to search for a group of 16 which we will be very close
       to finding every time but it will be broken by an off bit before the
       16th in a group in every block. */
    for (size_t block = 0, i = 0; block < end;
         ++block, i = block * BITSET_BLOCK_BITS)
    {
        check(bitset_set_range(&bs, i, first_half, CCC_FALSE), CCC_RESULT_OK);
        check(bitset_set_range(&bs, i + first_half + 1, second_half, CCC_FALSE),
              CCC_RESULT_OK);
        check(bitset_first_trailing_zeros_range(&bs, i, BITSET_BLOCK_BITS,
                                                first_half + 1)
                      .error
                  != CCC_RESULT_OK,
              true);
    }
    /* Then we will search for a full block worth which we will never find
       thanks to the off bit embedded in each block. */
    check(bitset_first_trailing_zeros(&bs, BITSET_BLOCK_BITS).error
              != CCC_RESULT_OK,
          true);
    /* Now fix the last group and we should pass. */
    check(bitset_set(&bs, ((end - 1) * BITSET_BLOCK_BITS) + first_half,
                     CCC_FALSE),
          CCC_TRUE);
    /* Now the solution crosses the block border from second to last to last
       block. */
    check(bitset_first_trailing_zeros(&bs, BITSET_BLOCK_BITS).count,
          (((end - 2) * BITSET_BLOCK_BITS) + first_half + 1));
    (void)bitset_reset_all(&bs);
    (void)bitset_set_range(&bs, 0, BITSET_BLOCK_BITS * 3, CCC_FALSE);
    check(bitset_set(&bs, first_half, CCC_TRUE), CCC_FALSE);
    check(bitset_first_trailing_zeros_range(&bs, 0, BITSET_BLOCK_BITS,
                                            BITSET_BLOCK_BITS)
                  .error
              != CCC_RESULT_OK,
          true);
    check_end();
}

check_static_begin(bitset_test_first_leading_one)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    /* Start with an almost full range and reduce by moving start backwards. */
    for (size_t i = 512; i-- > 1;)
    {
        check(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
        check(bitset_first_leading_one(&bs).count, i - 1);
        check(bitset_first_leading_one_range(&bs, i, 512 - i + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_leading_one_range(&bs, 0, i + 1).count, i - 1);
    }
    check(bitset_first_leading_one(&bs).count, 0);
    check(bitset_first_leading_one_range(&bs, 0, 1).count, 0);
    check_end();
}

check_static_begin(bitset_test_first_leading_one_range)
{
    Bitset bs = bitset_initialize(bitset_blocks(32), NULL, NULL, 32);
    size_t const bit_of_interest = 3;
    check(bitset_set(&bs, bit_of_interest, CCC_TRUE), CCC_FALSE);
    for (size_t i = 0; i < bit_of_interest; ++i)
    {
        /* Testing our code paths that include only a single block to read. */
        check(bitset_first_leading_one_range(&bs, i, bit_of_interest - i).count,
              bit_of_interest);
    }
    /* It is important that our bit set not report a false positive here. No
       matter the block size, a single bit matching our query will be loaded
       with the block. But the implementation must ensure that bit is not
       a match if it is out of range. Here the bit is not in our searched range
       so we do not find any bits matching our query. */
    check(bitset_first_leading_one_range(&bs, bit_of_interest + 1,
                                         bitset_count(&bs).count
                                             - bit_of_interest + 1)
                  .error
              != CCC_RESULT_OK,
          true);
    check_end();
}

check_static_begin(bitset_test_first_leading_ones)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t window = BITSET_BLOCK_BITS;
    /* Slide a group of int size as a window across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        check(bitset_set_range(&bs, i - window + 1, window, CCC_TRUE),
              CCC_RESULT_OK);
        check(bitset_first_leading_ones(&bs, window).count, i);
        check(bitset_first_leading_ones(&bs, window - 1).count, i);
        check(bitset_first_leading_ones(&bs, window + 1).error != CCC_RESULT_OK,
              true);
        check(bitset_first_leading_ones_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        check(
            bitset_first_leading_ones_range(&bs, i - window + 1, window, window)
                .count,
            i);
        check(bitset_first_leading_ones_range(&bs, i - window, window, window)
                      .error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        check(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    check(bitset_reset_all(&bs), CCC_RESULT_OK);
    window /= 4;
    /* Slide a very small group across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        check(bitset_set_range(&bs, i - window + 1, window, CCC_TRUE),
              CCC_RESULT_OK);
        check(bitset_first_leading_ones(&bs, window).count, i);
        check(bitset_first_leading_ones(&bs, window - 1).count, i);
        check(bitset_first_leading_ones(&bs, window + 1).error != CCC_RESULT_OK,
              true);
        check(bitset_first_leading_ones_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        check(
            bitset_first_leading_ones_range(&bs, i - window + 1, window, window)
                .count,
            i);
        check(bitset_first_leading_ones_range(&bs, i - window, window, window)
                      .error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        check(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    check(bitset_reset_all(&bs), CCC_RESULT_OK);
    window *= 8;
    /* Slide a very large group across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        check(bitset_set_range(&bs, i - window + 1, window, CCC_TRUE),
              CCC_RESULT_OK);
        check(bitset_first_leading_ones(&bs, window).count, i);
        check(bitset_first_leading_ones(&bs, window - 1).count, i);
        check(bitset_first_leading_ones(&bs, window + 1).error != CCC_RESULT_OK,
              true);
        check(bitset_first_leading_ones_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        check(
            bitset_first_leading_ones_range(&bs, i - window + 1, window, window)
                .count,
            i);
        check(bitset_first_leading_ones_range(&bs, i - window, window, window)
                      .error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        check(bitset_set(&bs, i, CCC_FALSE), CCC_TRUE);
    }
    check_end();
}

check_static_begin(bitset_test_first_leading_ones_fail)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const first_half = BITSET_BLOCK_BITS / 2;
    size_t const second_half = first_half - 1;
    /* We are going to search for a group of 17 which we will be very close
       to finding every time but it will be broken by an off bit before the
       17th in a group in every block. */
    for (size_t block = bitset_block_count(512); block--;)
    {
        check(bitset_set_range(&bs, (block * BITSET_BLOCK_BITS), first_half,
                               CCC_TRUE),
              CCC_RESULT_OK);
        check(bitset_set_range(&bs,
                               (block * BITSET_BLOCK_BITS) + first_half + 1,
                               second_half, CCC_TRUE),
              CCC_RESULT_OK);
        check(bitset_first_leading_ones_range(&bs, (block * BITSET_BLOCK_BITS),
                                              BITSET_BLOCK_BITS, first_half + 1)
                      .error
                  != CCC_RESULT_OK,
              true);
    }
    /* Then we will search for a full block worth which we will never find
       thanks to the off bit embedded in each block. */
    check(bitset_first_leading_ones(&bs, BITSET_BLOCK_BITS).error
              != CCC_RESULT_OK,
          true);
    /* Now fix the last group and we should pass. */
    check(bitset_set(&bs, first_half, CCC_TRUE), CCC_FALSE);
    /* Now the solution crosses the block border from second to last to last
       block. */
    check(bitset_first_leading_ones(&bs, BITSET_BLOCK_BITS).count,
          BITSET_BLOCK_BITS + first_half - 1);
    (void)bitset_reset_all(&bs);
    (void)bitset_set_range(&bs, 512 - (BITSET_BLOCK_BITS * 3),
                           BITSET_BLOCK_BITS * 3, CCC_TRUE);
    check(bitset_set(&bs, 512 - first_half, CCC_FALSE), CCC_TRUE);
    check(bitset_first_leading_ones_range(&bs, 512 - BITSET_BLOCK_BITS,
                                          BITSET_BLOCK_BITS, BITSET_BLOCK_BITS)
                  .error
              != CCC_RESULT_OK,
          true);
    check_end();
}

check_static_begin(bitset_test_first_leading_zero)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Start with an almost full range and reduce by moving start backwards. */
    for (size_t i = 512; i-- > 1;)
    {
        check(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
        check(bitset_first_leading_zero(&bs).count, i - 1);
        check(bitset_first_leading_zero_range(&bs, i, 512 - i + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_leading_zero_range(&bs, 0, i + 1).count, i - 1);
    }
    check(bitset_first_leading_zero(&bs).count, 0);
    check(bitset_first_leading_zero_range(&bs, 0, 1).count, 0);
    check_end();
}

check_static_begin(bitset_test_first_leading_zero_range)
{
    Bitset bs = bitset_initialize(bitset_blocks(32), NULL, NULL, 32);
    (void)bitset_set_all(&bs, CCC_TRUE);
    size_t const bit_of_interest = 3;
    check(bitset_set(&bs, bit_of_interest, CCC_FALSE), CCC_TRUE);
    for (size_t i = 0; i < bit_of_interest; ++i)
    {
        /* Testing our code paths that include only a single block to read. */
        check(
            bitset_first_leading_zero_range(&bs, i, bit_of_interest - i).count,
            bit_of_interest);
    }
    /* It is important that our bit set not report a false positive here. No
       matter the block size, a single bit matching our query will be loaded
       with the block. But the implementation must ensure that bit is not
       a match if it is out of range. Here the bit is not in our searched range
       so we do not find any bits matching our query. */
    check(bitset_first_leading_zero_range(&bs, bit_of_interest + 1,
                                          bitset_count(&bs).count
                                              - bit_of_interest + 1)
                  .error
              != CCC_RESULT_OK,
          true);
    check_end();
}

check_static_begin(bitset_test_first_leading_zeros)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t window = BITSET_BLOCK_BITS;
    /* Slide a group of int size as a window across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        check(bitset_set_range(&bs, i - window + 1, window, CCC_FALSE),
              CCC_RESULT_OK);
        check(bitset_first_leading_zeros(&bs, window).count, i);
        check(bitset_first_leading_zeros(&bs, window - 1).count, i);
        check(bitset_first_leading_zeros(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_leading_zeros_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_leading_zeros_range(&bs, i - window + 1, window,
                                               window)
                  .count,
              i);
        check(bitset_first_leading_zeros_range(&bs, i - window, window, window)
                      .error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        check(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    window /= 4;
    /* Slide a very small group across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        check(bitset_set_range(&bs, i - window + 1, window, CCC_FALSE),
              CCC_RESULT_OK);
        check(bitset_first_leading_zeros(&bs, window).count, i);
        check(bitset_first_leading_zeros(&bs, window - 1).count, i);
        check(bitset_first_leading_zeros(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_leading_zeros_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_leading_zeros_range(&bs, i - window + 1, window,
                                               window)
                  .count,
              i);
        check(bitset_first_leading_zeros_range(&bs, i - window, window, window)
                      .error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        check(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    window *= 8;
    /* Slide a very large group across the set. */
    for (size_t i = 511; i > window + 1; --i)
    {
        check(bitset_set_range(&bs, i - window + 1, window, CCC_FALSE),
              CCC_RESULT_OK);
        check(bitset_first_leading_zeros(&bs, window).count, i);
        check(bitset_first_leading_zeros(&bs, window - 1).count, i);
        check(bitset_first_leading_zeros(&bs, window + 1).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_leading_zeros_range(&bs, 0, i, window).error
                  != CCC_RESULT_OK,
              true);
        check(bitset_first_leading_zeros_range(&bs, i - window + 1, window,
                                               window)
                  .count,
              i);
        check(bitset_first_leading_zeros_range(&bs, i - window, window, window)
                      .error
                  != CCC_RESULT_OK,
              true);
        /* Cleanup behind as we go. */
        check(bitset_set(&bs, i, CCC_TRUE), CCC_FALSE);
    }
    check_end();
}

check_static_begin(bitset_test_first_leading_zeros_fail)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t const first_half = BITSET_BLOCK_BITS / 2;
    size_t const second_half = first_half - 1;
    /* We are going to search for a group of 17 which we will be very close
       to finding every time but it will be broken by an off bit before the
       17th in a group in every block. */
    for (size_t block = bitset_block_count(512); block--;)
    {
        check(bitset_set_range(&bs, (block * BITSET_BLOCK_BITS), first_half,
                               CCC_FALSE),
              CCC_RESULT_OK);
        check(bitset_set_range(&bs,
                               (block * BITSET_BLOCK_BITS) + first_half + 1,
                               second_half, CCC_FALSE),
              CCC_RESULT_OK);
        check(bitset_first_leading_zeros_range(&bs, (block * BITSET_BLOCK_BITS),
                                               BITSET_BLOCK_BITS,
                                               first_half + 1)
                      .error
                  != CCC_RESULT_OK,
              true);
    }
    /* Then we will search for a full block worth which we will never find
       thanks to the off bit embedded in each block. */
    check(bitset_first_leading_zeros(&bs, BITSET_BLOCK_BITS).error
              != CCC_RESULT_OK,
          true);
    /* Now fix the last group and we should pass. */
    check(bitset_set(&bs, first_half, CCC_FALSE), CCC_TRUE);
    /* Now the solution crosses the block border from second to last to last
       block. */
    check(bitset_first_leading_zeros(&bs, BITSET_BLOCK_BITS).count,
          BITSET_BLOCK_BITS + first_half - 1);
    (void)bitset_reset_all(&bs);
    (void)bitset_set_range(&bs, 512 - (BITSET_BLOCK_BITS * 3),
                           BITSET_BLOCK_BITS * 3, CCC_FALSE);
    check(bitset_set(&bs, 512 - first_half, CCC_TRUE), CCC_FALSE);
    check(bitset_first_leading_zeros_range(&bs, 512 - BITSET_BLOCK_BITS,
                                           BITSET_BLOCK_BITS, BITSET_BLOCK_BITS)
                  .error
              != CCC_RESULT_OK,
          true);
    check_end();
}

check_static_begin(bitset_test_or_same_size)
{
    Bitset source = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    Bitset destination = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const size = 512;
    for (size_t i = 0; i < size; i += 2)
    {
        check(bitset_set(&destination, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 1; i < size; i += 2)
    {
        check(bitset_set(&source, i, CCC_TRUE), CCC_FALSE);
    }
    check(bitset_popcount(&source).count, size / 2);
    check(bitset_popcount(&destination).count, size / 2);
    check(bitset_or(&destination, &source), CCC_RESULT_OK);
    check(bitset_popcount(&destination).count, size);
    check_end();
}

check_static_begin(bitset_test_or_diff_size)
{
    Bitset destination = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Make it slightly harder by not ending on a perfect block boundary. */
    Bitset source = bitset_initialize(bitset_blocks(244), NULL, NULL, 244);
    check(bitset_set_all(&source, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_popcount(&source).count, 244);
    check(bitset_popcount(&destination).count, 0);
    check(bitset_or(&destination, &source), CCC_RESULT_OK);
    check(bitset_popcount(&destination).count, 244);
    check_end();
}

check_static_begin(bitset_test_and_same_size)
{
    Bitset source = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    Bitset destination = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const size = 512;
    for (size_t i = 0; i < size; i += 2)
    {
        check(bitset_set(&destination, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 1; i < size; i += 2)
    {
        check(bitset_set(&source, i, CCC_TRUE), CCC_FALSE);
    }
    check(bitset_popcount(&source).count, size / 2);
    check(bitset_popcount(&destination).count, size / 2);
    check(bitset_and(&destination, &source), CCC_RESULT_OK);
    check(bitset_popcount(&destination).count, 0);
    check_end();
}

check_static_begin(bitset_test_and_diff_size)
{
    Bitset destination = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Make it slightly harder by not ending on a perfect block boundary. */
    Bitset source = bitset_initialize(bitset_blocks(244), NULL, NULL, 244);
    check(bitset_set_all(&destination, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_set_all(&source, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_popcount(&destination).count, 512);
    check(bitset_popcount(&source).count, 244);
    check(bitset_and(&destination, &source), CCC_RESULT_OK);
    check(bitset_popcount(&destination).count, 244);
    check(bitset_count(&destination).count, 512);
    check_end();
}

check_static_begin(bitset_test_xor_same_size)
{
    Bitset source = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    Bitset destination = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    size_t const size = 512;
    for (size_t i = 0; i < size; i += 2)
    {
        check(bitset_set(&destination, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 1; i < size; i += 2)
    {
        check(bitset_set(&source, i, CCC_TRUE), CCC_FALSE);
    }
    check(bitset_popcount(&source).count, size / 2);
    check(bitset_popcount(&destination).count, size / 2);
    check(bitset_xor(&destination, &source), CCC_RESULT_OK);
    check(bitset_popcount(&destination).count, size);
    check_end();
}

check_static_begin(bitset_test_xor_diff_size)
{
    Bitset destination = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    /* Make it slightly harder by not ending on a perfect block boundary. */
    Bitset source = bitset_initialize(bitset_blocks(244), NULL, NULL, 244);
    check(bitset_set_all(&destination, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_set_all(&source, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_popcount(&destination).count, 512);
    check(bitset_popcount(&source).count, 244);
    check(bitset_xor(&destination, &source), CCC_RESULT_OK);
    check(bitset_popcount(&destination).count, 512 - 244);
    check(bitset_count(&destination).count, 512);
    check_end();
}

check_static_begin(bitset_test_shift_left)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 512);
    check(bitset_shift_left(&bs, 512), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 0);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t ones = 512;
    check(bitset_shift_left(&bs, BITSET_BLOCK_BITS), CCC_RESULT_OK);
    check(bitset_popcount_range(&bs, 0, BITSET_BLOCK_BITS).count, 0);
    ones -= BITSET_BLOCK_BITS;
    check(bitset_popcount(&bs).count, ones);
    check(bitset_shift_left(&bs, BITSET_BLOCK_BITS / 2), CCC_RESULT_OK);
    ones -= (BITSET_BLOCK_BITS / 2);
    check(bitset_popcount(&bs).count, ones);
    check(bitset_shift_left(&bs, BITSET_BLOCK_BITS * 2), CCC_RESULT_OK);
    ones -= (BITSET_BLOCK_BITS * 2);
    check(bitset_popcount(&bs).count, ones);
    check(bitset_shift_left(&bs, (BITSET_BLOCK_BITS - 3) * 3), CCC_RESULT_OK);
    ones -= ((BITSET_BLOCK_BITS - 3) * 3);
    check(bitset_popcount(&bs).count, ones);
    check_end();
}

check_static_begin(bitset_test_shift_left_edgecase)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 512);
    check(bitset_shift_left(&bs, 510), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 2);
    check_end();
}

check_static_begin(bitset_test_shift_left_edgecase_small)
{
    Bitset bs = bitset_initialize(bitset_blocks(8), NULL, NULL, 8);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 8);
    check(bitset_shift_left(&bs, 7), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 1);
    check_end();
}

check_static_begin(bitset_test_shift_right)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 512);
    check(bitset_shift_right(&bs, 512), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 0);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    size_t ones = 512;
    check(bitset_shift_right(&bs, BITSET_BLOCK_BITS), CCC_RESULT_OK);
    check(bitset_popcount_range(&bs, 512 - BITSET_BLOCK_BITS, BITSET_BLOCK_BITS)
              .count,
          0);
    ones -= BITSET_BLOCK_BITS;
    check(bitset_popcount(&bs).count, ones);
    check(bitset_shift_right(&bs, BITSET_BLOCK_BITS / 2), CCC_RESULT_OK);
    ones -= (BITSET_BLOCK_BITS / 2);
    check(bitset_popcount(&bs).count, ones);
    check(bitset_shift_right(&bs, BITSET_BLOCK_BITS * 2), CCC_RESULT_OK);
    ones -= (BITSET_BLOCK_BITS * 2);
    check(bitset_popcount(&bs).count, ones);
    check(bitset_shift_right(&bs, (BITSET_BLOCK_BITS - 3) * 3), CCC_RESULT_OK);
    ones -= ((BITSET_BLOCK_BITS - 3) * 3);
    check(bitset_popcount(&bs).count, ones);
    check_end();
}

check_static_begin(bitset_test_shift_right_edgecase)
{
    Bitset bs = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 512);
    check(bitset_shift_right(&bs, 510), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 2);
    check_end();
}

check_static_begin(bitset_test_shift_right_edgecase_small)
{
    Bitset bs = bitset_initialize(bitset_blocks(8), NULL, NULL, 8);
    check(bitset_set_all(&bs, CCC_TRUE), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 8);
    check(bitset_shift_right(&bs, 7), CCC_RESULT_OK);
    check(bitset_popcount(&bs).count, 1);
    check_end();
}

check_static_begin(bitset_test_subset)
{
    Bitset set = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    Bitset subset1 = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    Bitset subset2 = bitset_initialize(bitset_blocks(244), NULL, NULL, 244);
    for (size_t i = 0; i < 512; i += 2)
    {
        check(bitset_set(&set, i, CCC_TRUE), CCC_FALSE);
        check(bitset_set(&subset1, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 0; i < 244; i += 2)
    {
        check(bitset_set(&subset2, i, CCC_TRUE), CCC_FALSE);
    }
    check(bitset_is_subset(&subset1, &set), CCC_TRUE);
    check(bitset_is_subset(&subset2, &set), CCC_TRUE);
    check_end();
}

check_static_begin(bitset_test_proper_subset)
{
    Bitset set = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    Bitset subset1 = bitset_initialize(bitset_blocks(512), NULL, NULL, 512);
    Bitset subset2 = bitset_initialize(bitset_blocks(244), NULL, NULL, 244);
    for (size_t i = 0; i < 512; i += 2)
    {
        check(bitset_set(&set, i, CCC_TRUE), CCC_FALSE);
        check(bitset_set(&subset1, i, CCC_TRUE), CCC_FALSE);
    }
    for (size_t i = 0; i < 244; i += 2)
    {
        check(bitset_set(&subset2, i, CCC_TRUE), CCC_FALSE);
    }
    check(bitset_is_proper_subset(&subset1, &set), CCC_FALSE);
    check(bitset_is_subset(&subset1, &set), CCC_TRUE);
    check(bitset_is_subset(&subset2, &set), CCC_TRUE);
    check(bitset_is_proper_subset(&subset2, &set), CCC_TRUE);
    check_end();
}

/* Returns if the box is valid. 1 for valid, 0 for invalid, -1 for an error */
CCC_Tribool
validate_sudoku_box(int board[9][9], Bitset *const row_check,
                    Bitset *const col_check, size_t const row_start,
                    size_t const col_start)
{
    Bitset box_check = bitset_initialize(bitset_blocks(9), NULL, NULL, 9);
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

check_static_begin(bitset_test_valid_sudoku)
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
    Bitset row_check
        = bitset_initialize(bitset_blocks(9L * 9L), NULL, NULL, 9L * 9L);
    Bitset col_check
        = bitset_initialize(bitset_blocks(9L * 9L), NULL, NULL, 9L * 9L);
    size_t const box_step = 3;
    for (size_t row = 0; row < 9; row += box_step)
    {
        for (size_t col = 0; col < 9; col += box_step)
        {
            CCC_Tribool const valid = validate_sudoku_box(
                valid_board, &row_check, &col_check, row, col);
            check(valid, CCC_TRUE);
        }
    }
    check_end();
}

check_static_begin(bitset_test_invalid_sudoku)
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
    Bitset row_check
        = bitset_initialize(bitset_blocks(9L * 9L), NULL, NULL, 9L * 9L);
    Bitset col_check
        = bitset_initialize(bitset_blocks(9L * 9L), NULL, NULL, 9L * 9L);
    size_t const box_step = 3;
    CCC_Tribool pass = CCC_TRUE;
    for (size_t row = 0; row < 9; row += box_step)
    {
        for (size_t col = 0; col < 9; col += box_step)
        {
            pass = validate_sudoku_box(invalid_board, &row_check, &col_check,
                                       row, col);
            check(pass != CCC_TRIBOOL_ERROR, true);
            if (!pass)
            {
                goto done;
            }
        }
    }
done:
    check(pass, CCC_FALSE);
    check_end();
}

int
main(void)
{
    return check_run(
        bitset_test_set_one(), bitset_test_set_shuffled(),
        bitset_test_set_all(), bitset_test_set_range(), bitset_test_reset(),
        bitset_test_flip(), bitset_test_flip_all(), bitset_test_flip_range(),
        bitset_test_reset_all(), bitset_test_reset_range(), bitset_test_any(),
        bitset_test_all(), bitset_test_none(), bitset_test_first_trailing_one(),
        bitset_test_first_trailing_ones(),
        bitset_test_first_trailing_ones_fail(),
        bitset_test_first_trailing_zero(), bitset_test_first_trailing_zeros(),
        bitset_test_first_trailing_zeros_fail(),
        bitset_test_first_leading_one(), bitset_test_first_leading_one_range(),
        bitset_test_first_leading_ones(), bitset_test_first_leading_ones_fail(),
        bitset_test_first_leading_zero(),
        bitset_test_first_leading_zero_range(),
        bitset_test_first_leading_zeros(),
        bitset_test_first_leading_zeros_fail(), bitset_test_or_same_size(),
        bitset_test_or_diff_size(), bitset_test_and_same_size(),
        bitset_test_and_diff_size(), bitset_test_xor_same_size(),
        bitset_test_xor_diff_size(), bitset_test_shift_left(),
        bitset_test_shift_right(), bitset_test_shift_left_edgecase(),
        bitset_test_shift_right_edgecase(),
        bitset_test_shift_left_edgecase_small(),
        bitset_test_shift_right_edgecase_small(), bitset_test_subset(),
        bitset_test_proper_subset(), bitset_test_valid_sudoku(),
        bitset_test_invalid_sudoku());
}
