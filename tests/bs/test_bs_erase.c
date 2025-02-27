#include <stdbool.h>
#include <stddef.h>

#include "alloc.h"
#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"

CHECK_BEGIN_STATIC_FN(bs_test_push_pop_back_no_realloc)
{
    ccc_bitset bs
        = ccc_bs_init((ccc_bitblock[ccc_bs_blocks(16)]){}, NULL, NULL, 16);
    CHECK(ccc_bs_capacity(&bs), 16);
    CHECK(ccc_bs_size(&bs), 0);
    ccc_result push_status = CCC_OK;
    for (size_t i = 0; push_status == CCC_OK; ++i)
    {
        if (i % 2)
        {
            push_status = ccc_bs_push_back(&bs, CCC_TRUE);
        }
        else
        {
            push_status = ccc_bs_push_back(&bs, CCC_FALSE);
        }
    }
    CHECK(push_status, CCC_NO_ALLOC);
    CHECK(ccc_bs_size(&bs), 16);
    CHECK(ccc_bs_popcount(&bs), 16 / 2);
    while (!ccc_bs_empty(&bs))
    {
        ccc_tribool const msb = ccc_bs_pop_back(&bs);
        if (ccc_bs_size(&bs) % 2)
        {
            CHECK(msb, CCC_TRUE);
        }
        else
        {
            CHECK(msb, CCC_FALSE);
        }
    }
    CHECK(ccc_bs_size(&bs), 0);
    CHECK(ccc_bs_popcount(&bs), 0);
    CHECK(ccc_bs_capacity(&bs), 16);
    CHECK(ccc_bs_clear(&bs), CCC_OK);
    CHECK(ccc_bs_capacity(&bs), 16);
    CHECK(ccc_bs_clear_and_free(&bs), CCC_NO_ALLOC);
    CHECK(ccc_bs_capacity(&bs), 16);
    CHECK(ccc_bs_size(&bs), 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_push_pop_back_alloc)
{
    ccc_bitset bs = ccc_bs_init((ccc_bitblock *)NULL, std_alloc, NULL, 0);
    CHECK(ccc_bs_capacity(&bs), 0);
    CHECK(ccc_bs_size(&bs), 0);
    for (size_t i = 0; ccc_bs_size(&bs) < 16ULL; ++i)
    {
        if (i % 2)
        {
            CHECK(ccc_bs_push_back(&bs, CCC_TRUE), CCC_OK);
        }
        else
        {
            CHECK(ccc_bs_push_back(&bs, CCC_FALSE), CCC_OK);
        }
    }
    CHECK(ccc_bs_size(&bs), 16);
    CHECK(ccc_bs_popcount(&bs), 16 / 2);
    while (!ccc_bs_empty(&bs))
    {
        ccc_tribool const msb_was = ccc_bs_pop_back(&bs);
        if (ccc_bs_size(&bs) % 2)
        {
            CHECK(msb_was, CCC_TRUE);
        }
        else
        {
            CHECK(msb_was, CCC_FALSE);
        }
    }
    CHECK(ccc_bs_pop_back(&bs), (ccc_tribool)CCC_BOOL_ERR);
    CHECK(ccc_bs_size(&bs), 0);
    CHECK(ccc_bs_popcount(&bs), 0);
    CHECK(ccc_bs_capacity(&bs) != 0, true);
    CHECK(ccc_bs_clear(&bs), CCC_OK);
    CHECK(ccc_bs_capacity(&bs) != 0, true);
    CHECK(ccc_bs_clear_and_free(&bs), CCC_OK);
    CHECK(ccc_bs_capacity(&bs), 0);
    CHECK(ccc_bs_size(&bs), 0);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(bs_test_push_pop_back_no_realloc(),
                     bs_test_push_pop_back_alloc());
}
