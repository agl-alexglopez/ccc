#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "bitset.h"
#include "checkers.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

CHECK_BEGIN_STATIC_FN(bs_test_push_back_no_realloc)
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
    CHECK(CCC_bs_clear(&bs), CCC_RESULT_OK);
    CHECK(CCC_bs_count(&bs).count, 0);
    CHECK(CCC_bs_popcount(&bs).count, 0);
    CHECK(CCC_bs_capacity(&bs).count, 16);
    CHECK(CCC_bs_clear_and_free(&bs), CCC_RESULT_NO_ALLOC);
    CHECK(CCC_bs_capacity(&bs).count, 16);
    CHECK(CCC_bs_count(&bs).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_push_back_alloc)
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
    CHECK(CCC_bs_clear(&bs), CCC_RESULT_OK);
    CHECK(CCC_bs_count(&bs).count, 0);
    CHECK(CCC_bs_popcount(&bs).count, 0);
    CHECK(CCC_bs_capacity(&bs).count != 0, true);
    CHECK(CCC_bs_clear_and_free(&bs), CCC_RESULT_OK);
    CHECK(CCC_bs_capacity(&bs).count, 0);
    CHECK(CCC_bs_count(&bs).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bs_test_push_back_reserve)
{
    CCC_bitset bs = CCC_bs_init(NULL, NULL, NULL, 0);
    CCC_result const r = reserve(&bs, 512, std_alloc);
    CHECK(r, CCC_RESULT_OK);
    CHECK(CCC_bs_count(&bs).count, 0);
    CHECK(CCC_bs_capacity(&bs).count != 0, true);
    for (size_t i = 0; CCC_bs_count(&bs).count < 512; ++i)
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
    CHECK(CCC_bs_count(&bs).count, 512);
    CHECK(CCC_bs_popcount(&bs).count, 512 / 2);
    CHECK(clear(&bs), CCC_RESULT_OK);
    CHECK(CCC_bs_count(&bs).count, 0);
    CHECK(CCC_bs_popcount(&bs).count, 0);
    CHECK(CCC_bs_capacity(&bs).count != 0, true);
    CHECK(clear_and_free_reserve(&bs, std_alloc), CCC_RESULT_OK);
    CHECK(CCC_bs_capacity(&bs).count, 0);
    CHECK(CCC_bs_count(&bs).count, 0);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(bs_test_push_back_no_realloc(), bs_test_push_back_alloc(),
                     bs_test_push_back_reserve());
}
