#include <stddef.h>

#include "alloc.h"
#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"

CHECK_BEGIN_STATIC_FN(bs_test_construct)
{
    ccc_bitset bs
        = ccc_bs_init((ccc_bitblock[ccc_bs_blocks(10)]){}, NULL, NULL, 10, 10);
    CHECK(ccc_bs_popcount(&bs).count, 0);
    for (size_t i = 0; i < ccc_bs_capacity(&bs).count; ++i)
    {
        CHECK(ccc_bs_test(&bs, i), CCC_FALSE);
        CHECK(ccc_bs_test(&bs, i), CCC_FALSE);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_copy_no_alloc)
{
    ccc_bitset src
        = ccc_bs_init((ccc_bitblock[ccc_bs_blocks(512)]){}, NULL, NULL, 512);
    CHECK(ccc_bs_capacity(&src).count, 512);
    CHECK(ccc_bs_size(&src).count, 0);
    ccc_result push_status = CCC_RESULT_OK;
    for (size_t i = 0; push_status == CCC_RESULT_OK; ++i)
    {
        if (i % 2)
        {
            push_status = ccc_bs_push_back(&src, CCC_TRUE);
        }
        else
        {
            push_status = ccc_bs_push_back(&src, CCC_FALSE);
        }
    }
    CHECK(push_status, CCC_RESULT_NO_ALLOC);
    ccc_bitset dst
        = ccc_bs_init((ccc_bitblock[ccc_bs_blocks(513)]){}, NULL, NULL, 513);
    ccc_result r = ccc_bs_copy(&dst, &src, NULL);
    CHECK(r, CCC_RESULT_OK);
    CHECK(ccc_bs_popcount(&src).count, ccc_bs_popcount(&dst).count);
    CHECK(ccc_bs_size(&src).count, ccc_bs_size(&dst).count);
    while (!ccc_bs_empty(&src) && !ccc_bs_empty(&dst))
    {
        ccc_tribool const src_msb = ccc_bs_pop_back(&src);
        ccc_tribool const dst_msb = ccc_bs_pop_back(&dst);
        if (ccc_bs_size(&src).count % 2)
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
    CHECK(ccc_bs_empty(&src), ccc_bs_empty(&dst));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_copy_alloc)
{
    ccc_bitset src = ccc_bs_init((ccc_bitblock *)NULL, std_alloc, NULL, 0);
    for (size_t i = 0; i < 512; ++i)
    {
        if (i % 2)
        {
            CHECK(ccc_bs_push_back(&src, CCC_TRUE), CCC_RESULT_OK);
        }
        else
        {
            CHECK(ccc_bs_push_back(&src, CCC_FALSE), CCC_RESULT_OK);
        }
    }
    ccc_bitset dst = ccc_bs_init((ccc_bitblock *)NULL, std_alloc, NULL, 0);
    ccc_result r = ccc_bs_copy(&dst, &src, std_alloc);
    CHECK(r, CCC_RESULT_OK);
    CHECK(ccc_bs_popcount(&src).count, ccc_bs_popcount(&dst).count);
    CHECK(ccc_bs_size(&src).count, ccc_bs_size(&dst).count);
    while (!ccc_bs_empty(&src) && !ccc_bs_empty(&dst))
    {
        ccc_tribool const src_msb = ccc_bs_pop_back(&src);
        ccc_tribool const dst_msb = ccc_bs_pop_back(&dst);
        if (ccc_bs_size(&src).count % 2)
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
    CHECK(ccc_bs_empty(&src), ccc_bs_empty(&dst));
    CHECK_END_FN({
        (void)ccc_bs_clear_and_free(&src);
        (void)ccc_bs_clear_and_free(&dst);
    });
}

int
main(void)
{
    return CHECK_RUN(bs_test_construct(), bs_test_copy_no_alloc(),
                     bs_test_copy_alloc());
}
