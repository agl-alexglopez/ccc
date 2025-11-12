#include <stdbool.h>
#include <stddef.h>

#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"
#include "util/alloc.h"

CHECK_BEGIN_STATIC_FN(bs_test_push_pop_back_no_realloc)
{
    CCC_bitset bs = CCC_bs_init(CCC_bs_blocks(16), NULL, NULL, 16, 0);
    CHECK(CCC_bs_capacity(&bs).count, 16);
    CHECK(CCC_bs_count(&bs).count, 0);
    CCC_result push_status = CCC_RESULT_OK;
    for (size_t i = 0; push_status == CCC_RESULT_OK; ++i)
    {
        if (i % 2)
        {
            push_status = CCC_bs_push_back(&bs, CCC_TRUE);
        }
        else
        {
            push_status = CCC_bs_push_back(&bs, CCC_FALSE);
        }
    }
    CHECK(push_status, CCC_RESULT_NO_ALLOC);
    CHECK(CCC_bs_count(&bs).count, 16);
    CHECK(CCC_bs_popcount(&bs).count, 16 / 2);
    while (!CCC_bs_empty(&bs))
    {
        CCC_tribool const msb = CCC_bs_pop_back(&bs);
        if (CCC_bs_count(&bs).count % 2)
        {
            CHECK(msb, CCC_TRUE);
        }
        else
        {
            CHECK(msb, CCC_FALSE);
        }
    }
    CHECK(CCC_bs_count(&bs).count, 0);
    CHECK(CCC_bs_popcount(&bs).count, 0);
    CHECK(CCC_bs_capacity(&bs).count, 16);
    CHECK(CCC_bs_clear(&bs), CCC_RESULT_OK);
    CHECK(CCC_bs_capacity(&bs).count, 16);
    CHECK(CCC_bs_clear_and_free(&bs), CCC_RESULT_NO_ALLOC);
    CHECK(CCC_bs_capacity(&bs).count, 16);
    CHECK(CCC_bs_count(&bs).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_push_pop_back_alloc)
{
    CCC_bitset bs = CCC_bs_init(NULL, std_alloc, NULL, 0);
    CHECK(CCC_bs_capacity(&bs).count, 0);
    CHECK(CCC_bs_count(&bs).count, 0);
    for (size_t i = 0; CCC_bs_count(&bs).count < 16; ++i)
    {
        if (i % 2)
        {
            CHECK(CCC_bs_push_back(&bs, CCC_TRUE), CCC_RESULT_OK);
        }
        else
        {
            CHECK(CCC_bs_push_back(&bs, CCC_FALSE), CCC_RESULT_OK);
        }
    }
    CHECK(CCC_bs_count(&bs).count, 16);
    CHECK(CCC_bs_popcount(&bs).count, 16 / 2);
    while (!CCC_bs_empty(&bs))
    {
        CCC_tribool const msb_was = CCC_bs_pop_back(&bs);
        if (CCC_bs_count(&bs).count % 2)
        {
            CHECK(msb_was, CCC_TRUE);
        }
        else
        {
            CHECK(msb_was, CCC_FALSE);
        }
    }
    CHECK(CCC_bs_pop_back(&bs), (CCC_tribool)CCC_TRIBOOL_ERROR);
    CHECK(CCC_bs_count(&bs).count, 0);
    CHECK(CCC_bs_popcount(&bs).count, 0);
    CHECK(CCC_bs_capacity(&bs).count != 0, true);
    CHECK(CCC_bs_clear(&bs), CCC_RESULT_OK);
    CHECK(CCC_bs_capacity(&bs).count != 0, true);
    CHECK(CCC_bs_clear_and_free(&bs), CCC_RESULT_OK);
    CHECK(CCC_bs_capacity(&bs).count, 0);
    CHECK(CCC_bs_count(&bs).count, 0);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(bs_test_push_pop_back_no_realloc(),
                     bs_test_push_pop_back_alloc());
}
