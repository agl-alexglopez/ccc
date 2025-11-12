#include <stddef.h>

#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"
#include "util/alloc.h"

CHECK_BEGIN_STATIC_FN(bs_test_construct)
{
    CCC_bitset bs = CCC_bs_init(CCC_bs_blocks(10), NULL, NULL, 10);
    CHECK(CCC_bs_popcount(&bs).count, 0);
    for (size_t i = 0; i < CCC_bs_capacity(&bs).count; ++i)
    {
        CHECK(CCC_bs_test(&bs, i), CCC_FALSE);
        CHECK(CCC_bs_test(&bs, i), CCC_FALSE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_copy_no_alloc)
{
    CCC_bitset src = CCC_bs_init(CCC_bs_blocks(512), NULL, NULL, 512, 0);
    CHECK(CCC_bs_capacity(&src).count, 512);
    CHECK(CCC_bs_count(&src).count, 0);
    CCC_result push_status = CCC_RESULT_OK;
    for (size_t i = 0; push_status == CCC_RESULT_OK; ++i)
    {
        if (i % 2)
        {
            push_status = CCC_bs_push_back(&src, CCC_TRUE);
        }
        else
        {
            push_status = CCC_bs_push_back(&src, CCC_FALSE);
        }
    }
    CHECK(push_status, CCC_RESULT_NO_ALLOC);
    CCC_bitset dst = CCC_bs_init(CCC_bs_blocks(513), NULL, NULL, 513, 0);
    CCC_result r = CCC_bs_copy(&dst, &src, NULL);
    CHECK(r, CCC_RESULT_OK);
    CHECK(CCC_bs_popcount(&src).count, CCC_bs_popcount(&dst).count);
    CHECK(CCC_bs_count(&src).count, CCC_bs_count(&dst).count);
    while (!CCC_bs_empty(&src) && !CCC_bs_empty(&dst))
    {
        CCC_tribool const src_msb = CCC_bs_pop_back(&src);
        CCC_tribool const dst_msb = CCC_bs_pop_back(&dst);
        if (CCC_bs_count(&src).count % 2)
        {
            CHECK(src_msb, CCC_TRUE);
            CHECK(src_msb, dst_msb);
        }
        else
        {
            CHECK(src_msb, CCC_FALSE);
            CHECK(src_msb, dst_msb);
        }
    }
    CHECK(CCC_bs_empty(&src), CCC_bs_empty(&dst));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_copy_alloc)
{
    CCC_bitset src = CCC_bs_init(NULL, std_alloc, NULL, 0);
    for (size_t i = 0; i < 512; ++i)
    {
        if (i % 2)
        {
            CHECK(CCC_bs_push_back(&src, CCC_TRUE), CCC_RESULT_OK);
        }
        else
        {
            CHECK(CCC_bs_push_back(&src, CCC_FALSE), CCC_RESULT_OK);
        }
    }
    CCC_bitset dst = CCC_bs_init(NULL, std_alloc, NULL, 0);
    CCC_result r = CCC_bs_copy(&dst, &src, std_alloc);
    CHECK(r, CCC_RESULT_OK);
    CHECK(CCC_bs_popcount(&src).count, CCC_bs_popcount(&dst).count);
    CHECK(CCC_bs_count(&src).count, CCC_bs_count(&dst).count);
    while (!CCC_bs_empty(&src) && !CCC_bs_empty(&dst))
    {
        CCC_tribool const src_msb = CCC_bs_pop_back(&src);
        CCC_tribool const dst_msb = CCC_bs_pop_back(&dst);
        if (CCC_bs_count(&src).count % 2)
        {
            CHECK(src_msb, CCC_TRUE);
            CHECK(src_msb, dst_msb);
        }
        else
        {
            CHECK(src_msb, CCC_FALSE);
            CHECK(src_msb, dst_msb);
        }
    }
    CHECK(CCC_bs_empty(&src), CCC_bs_empty(&dst));
    CHECK_END_FN({
        (void)CCC_bs_clear_and_free(&src);
        (void)CCC_bs_clear_and_free(&dst);
    });
}

CHECK_BEGIN_STATIC_FN(bs_test_init_from)
{
    char input[] = {'1', '1', '0', '1', '1', '0', '\0'};
    CCC_bitset b
        = CCC_bs_from(std_alloc, NULL, 0, sizeof(input) - 1, '1', "110110");
    CHECK(CCC_bs_count(&b).count, sizeof(input) - 1);
    CHECK(CCC_bs_capacity(&b).count, sizeof(input) - 1);
    CHECK(CCC_bs_popcount(&b).count, 4);
    CHECK(CCC_bs_test(&b, 0), CCC_TRUE);
    CHECK(CCC_bs_test(&b, sizeof(input) - 2), CCC_FALSE);
    CHECK_END_FN(CCC_bs_clear_and_free(&b););
}

CHECK_BEGIN_STATIC_FN(bs_test_init_from_cap)
{
    char input[] = {'1', '1', '0', '1', '1', '0', '\0'};
    CCC_bitset b = CCC_bs_from(std_alloc, NULL, 0, sizeof(input), '1', input,
                               (sizeof(input) - 1) * 2);
    CHECK(CCC_bs_count(&b).count, sizeof(input) - 1);
    CHECK(CCC_bs_capacity(&b).count, (sizeof(input) - 1) * 2);
    CHECK(CCC_bs_popcount(&b).count, 4);
    CHECK(CCC_bs_test(&b, 0), CCC_TRUE);
    CHECK(CCC_bs_test(&b, sizeof(input) - 2), CCC_FALSE);
    CHECK(CCC_TRIBOOL_ERROR, CCC_bs_test(&b, sizeof(input) - 1));
    CHECK(CCC_bs_push_back(&b, CCC_TRUE), CCC_RESULT_OK);
    CHECK(CCC_TRUE, CCC_bs_test(&b, sizeof(input) - 1));
    CHECK(CCC_bs_capacity(&b).count, (sizeof(input) - 1) * 2);
    CHECK_END_FN(CCC_bs_clear_and_free(&b););
}

CHECK_BEGIN_STATIC_FN(bs_test_init_from_fail)
{
    char input[] = {'1', '1', '0', '1', '1', '0', '\0'};
    /* Forgot allocation function. */
    CCC_bitset b = CCC_bs_from(NULL, NULL, 0, sizeof(input) - 1, '1', input);
    CHECK(CCC_bs_count(&b).count, 0);
    CHECK(CCC_bs_capacity(&b).count, 0);
    CHECK(CCC_bs_popcount(&b).count, 0);
    CHECK(CCC_TRIBOOL_ERROR, CCC_bs_test(&b, 0));
    CHECK(CCC_TRIBOOL_ERROR, CCC_bs_test(&b, 99));
    CHECK_END_FN(CCC_bs_clear_and_free(&b););
}

CHECK_BEGIN_STATIC_FN(bs_test_init_from_cap_fail)
{
    char input[] = {'1', '1', '0', '1', '1', '0', '\0'};
    /* Forgot allocation function. */
    CCC_bitset b
        = CCC_bs_from(NULL, NULL, 0, sizeof(input) - 1, '1', input, 99);
    CHECK(CCC_bs_count(&b).count, 0);
    CHECK(CCC_bs_capacity(&b).count, 0);
    CHECK(CCC_bs_popcount(&b).count, 0);
    CHECK(CCC_TRIBOOL_ERROR, CCC_bs_test(&b, 0));
    CHECK(CCC_TRIBOOL_ERROR, CCC_bs_test(&b, 99));
    CHECK_END_FN(CCC_bs_clear_and_free(&b););
}

CHECK_BEGIN_STATIC_FN(bs_test_init_with_capacity)
{
    CCC_bitset b = CCC_bs_with_capacity(std_alloc, NULL, 10);
    CHECK(CCC_bs_popcount(&b).count, 0);
    CHECK(CCC_bs_set(&b, 0, CCC_TRUE), CCC_FALSE);
    CHECK(CCC_bs_set(&b, 9, CCC_TRUE), CCC_FALSE);
    CHECK(CCC_bs_test(&b, 0), CCC_TRUE);
    CHECK(CCC_bs_test(&b, 9), CCC_TRUE);
    CHECK_END_FN(CCC_bs_clear_and_free(&b););
}

CHECK_BEGIN_STATIC_FN(bs_test_init_with_capacity_fail)
{
    CCC_bitset b = CCC_bs_with_capacity(NULL, NULL, 10);
    CHECK(CCC_bs_popcount(&b).count, 0);
    CHECK(CCC_TRIBOOL_ERROR, CCC_bs_set(&b, 0, CCC_TRUE));
    CHECK(CCC_TRIBOOL_ERROR, CCC_bs_set(&b, 9, CCC_TRUE));
    CHECK(CCC_TRIBOOL_ERROR, CCC_bs_test(&b, 0));
    CHECK(CCC_TRIBOOL_ERROR, CCC_bs_test(&b, 9));
    CHECK_END_FN(CCC_bs_clear_and_free(&b););
}

int
main(void)
{
    return CHECK_RUN(bs_test_construct(), bs_test_copy_no_alloc(),
                     bs_test_copy_alloc(), bs_test_init_from(),
                     bs_test_init_from_cap(), bs_test_init_from_fail(),
                     bs_test_init_from_cap_fail(), bs_test_init_with_capacity(),
                     bs_test_init_with_capacity_fail());
}
