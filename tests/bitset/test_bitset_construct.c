#include <stddef.h>

#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"
#include "util/alloc.h"

CHECK_BEGIN_STATIC_FN(bitset_test_construct)
{
    CCC_Bitset bs
        = CCC_bitset_initialize(CCC_bitset_blocks(10), NULL, NULL, 10);
    CHECK(CCC_bitset_popcount(&bs).count, 0);
    for (size_t i = 0; i < CCC_bitset_capacity(&bs).count; ++i)
    {
        CHECK(CCC_bitset_test(&bs, i), CCC_FALSE);
        CHECK(CCC_bitset_test(&bs, i), CCC_FALSE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_copy_no_alloc)
{
    CCC_Bitset src
        = CCC_bitset_initialize(CCC_bitset_blocks(512), NULL, NULL, 512, 0);
    CHECK(CCC_bitset_capacity(&src).count, 512);
    CHECK(CCC_bitset_count(&src).count, 0);
    CCC_Result push_status = CCC_RESULT_OK;
    for (size_t i = 0; push_status == CCC_RESULT_OK; ++i)
    {
        if (i % 2)
        {
            push_status = CCC_bitset_push_back(&src, CCC_TRUE);
        }
        else
        {
            push_status = CCC_bitset_push_back(&src, CCC_FALSE);
        }
    }
    CHECK(push_status, CCC_RESULT_NO_ALLOC);
    CCC_Bitset dst
        = CCC_bitset_initialize(CCC_bitset_blocks(513), NULL, NULL, 513, 0);
    CCC_Result r = CCC_bitset_copy(&dst, &src, NULL);
    CHECK(r, CCC_RESULT_OK);
    CHECK(CCC_bitset_popcount(&src).count, CCC_bitset_popcount(&dst).count);
    CHECK(CCC_bitset_count(&src).count, CCC_bitset_count(&dst).count);
    while (!CCC_bitset_empty(&src) && !CCC_bitset_empty(&dst))
    {
        CCC_Tribool const src_msb = CCC_bitset_pop_back(&src);
        CCC_Tribool const dst_msb = CCC_bitset_pop_back(&dst);
        if (CCC_bitset_count(&src).count % 2)
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
    CHECK(CCC_bitset_empty(&src), CCC_bitset_empty(&dst));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_copy_alloc)
{
    CCC_Bitset src = CCC_bitset_initialize(NULL, std_alloc, NULL, 0);
    for (size_t i = 0; i < 512; ++i)
    {
        if (i % 2)
        {
            CHECK(CCC_bitset_push_back(&src, CCC_TRUE), CCC_RESULT_OK);
        }
        else
        {
            CHECK(CCC_bitset_push_back(&src, CCC_FALSE), CCC_RESULT_OK);
        }
    }
    CCC_Bitset dst = CCC_bitset_initialize(NULL, std_alloc, NULL, 0);
    CCC_Result r = CCC_bitset_copy(&dst, &src, std_alloc);
    CHECK(r, CCC_RESULT_OK);
    CHECK(CCC_bitset_popcount(&src).count, CCC_bitset_popcount(&dst).count);
    CHECK(CCC_bitset_count(&src).count, CCC_bitset_count(&dst).count);
    while (!CCC_bitset_empty(&src) && !CCC_bitset_empty(&dst))
    {
        CCC_Tribool const src_msb = CCC_bitset_pop_back(&src);
        CCC_Tribool const dst_msb = CCC_bitset_pop_back(&dst);
        if (CCC_bitset_count(&src).count % 2)
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
    CHECK(CCC_bitset_empty(&src), CCC_bitset_empty(&dst));
    CHECK_END_FN({
        (void)CCC_bitset_clear_and_free(&src);
        (void)CCC_bitset_clear_and_free(&dst);
    });
}

CHECK_BEGIN_STATIC_FN(bitset_test_init_from)
{
    char input[] = {'1', '1', '0', '1', '1', '0', '\0'};
    CCC_Bitset b
        = CCC_bitset_from(std_alloc, NULL, 0, sizeof(input) - 1, '1', "110110");
    CHECK(CCC_bitset_count(&b).count, sizeof(input) - 1);
    CHECK(CCC_bitset_capacity(&b).count, sizeof(input) - 1);
    CHECK(CCC_bitset_popcount(&b).count, 4);
    CHECK(CCC_bitset_test(&b, 0), CCC_TRUE);
    CHECK(CCC_bitset_test(&b, sizeof(input) - 2), CCC_FALSE);
    CHECK_END_FN(CCC_bitset_clear_and_free(&b););
}

CHECK_BEGIN_STATIC_FN(bitset_test_init_from_cap)
{
    char input[] = {'1', '1', '0', '1', '1', '0', '\0'};
    CCC_Bitset b = CCC_bitset_from(std_alloc, NULL, 0, sizeof(input), '1',
                                   input, (sizeof(input) - 1) * 2);
    CHECK(CCC_bitset_count(&b).count, sizeof(input) - 1);
    CHECK(CCC_bitset_capacity(&b).count, (sizeof(input) - 1) * 2);
    CHECK(CCC_bitset_popcount(&b).count, 4);
    CHECK(CCC_bitset_test(&b, 0), CCC_TRUE);
    CHECK(CCC_bitset_test(&b, sizeof(input) - 2), CCC_FALSE);
    CHECK(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, sizeof(input) - 1));
    CHECK(CCC_bitset_push_back(&b, CCC_TRUE), CCC_RESULT_OK);
    CHECK(CCC_TRUE, CCC_bitset_test(&b, sizeof(input) - 1));
    CHECK(CCC_bitset_capacity(&b).count, (sizeof(input) - 1) * 2);
    CHECK_END_FN(CCC_bitset_clear_and_free(&b););
}

CHECK_BEGIN_STATIC_FN(bitset_test_init_from_fail)
{
    char input[] = {'1', '1', '0', '1', '1', '0', '\0'};
    /* Forgot allocation function. */
    CCC_Bitset b
        = CCC_bitset_from(NULL, NULL, 0, sizeof(input) - 1, '1', input);
    CHECK(CCC_bitset_count(&b).count, 0);
    CHECK(CCC_bitset_capacity(&b).count, 0);
    CHECK(CCC_bitset_popcount(&b).count, 0);
    CHECK(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, 0));
    CHECK(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, 99));
    CHECK_END_FN(CCC_bitset_clear_and_free(&b););
}

CHECK_BEGIN_STATIC_FN(bitset_test_init_from_cap_fail)
{
    char input[] = {'1', '1', '0', '1', '1', '0', '\0'};
    /* Forgot allocation function. */
    CCC_Bitset b
        = CCC_bitset_from(NULL, NULL, 0, sizeof(input) - 1, '1', input, 99);
    CHECK(CCC_bitset_count(&b).count, 0);
    CHECK(CCC_bitset_capacity(&b).count, 0);
    CHECK(CCC_bitset_popcount(&b).count, 0);
    CHECK(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, 0));
    CHECK(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, 99));
    CHECK_END_FN(CCC_bitset_clear_and_free(&b););
}

CHECK_BEGIN_STATIC_FN(bitset_test_init_with_capacity)
{
    CCC_Bitset b = CCC_bitset_with_capacity(std_alloc, NULL, 10);
    CHECK(CCC_bitset_popcount(&b).count, 0);
    CHECK(CCC_bitset_set(&b, 0, CCC_TRUE), CCC_FALSE);
    CHECK(CCC_bitset_set(&b, 9, CCC_TRUE), CCC_FALSE);
    CHECK(CCC_bitset_test(&b, 0), CCC_TRUE);
    CHECK(CCC_bitset_test(&b, 9), CCC_TRUE);
    CHECK_END_FN(CCC_bitset_clear_and_free(&b););
}

CHECK_BEGIN_STATIC_FN(bitset_test_init_with_capacity_fail)
{
    CCC_Bitset b = CCC_bitset_with_capacity(NULL, NULL, 10);
    CHECK(CCC_bitset_popcount(&b).count, 0);
    CHECK(CCC_TRIBOOL_ERROR, CCC_bitset_set(&b, 0, CCC_TRUE));
    CHECK(CCC_TRIBOOL_ERROR, CCC_bitset_set(&b, 9, CCC_TRUE));
    CHECK(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, 0));
    CHECK(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, 9));
    CHECK_END_FN(CCC_bitset_clear_and_free(&b););
}

int
main(void)
{
    return CHECK_RUN(bitset_test_construct(), bitset_test_copy_no_alloc(),
                     bitset_test_copy_alloc(), bitset_test_init_from(),
                     bitset_test_init_from_cap(), bitset_test_init_from_fail(),
                     bitset_test_init_from_cap_fail(),
                     bitset_test_init_with_capacity(),
                     bitset_test_init_with_capacity_fail());
}
