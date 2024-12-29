#include <stddef.h>

#include "alloc.h"
#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"

CHECK_BEGIN_STATIC_FN(bs_test_push_back_no_realloc)
{
    ccc_bitset bs
        = ccc_bs_init((ccc_bitblock[ccc_bs_blocks(16)]){}, 16, NULL, NULL);
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
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_push_back_alloc)
{
    ccc_bitset bs = ccc_bs_init((ccc_bitblock *)NULL, 0, std_alloc, NULL);
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
    CHECK_END_FN(ccc_bs_clear_and_free(&bs););
}

int
main(void)
{
    return CHECK_RUN(bs_test_push_back_no_realloc(), bs_test_push_back_alloc());
}
