#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "bitset.h"
#include "checkers.h"
#include "traits.h"
#include "types.h"
#include "util/allocate.h"

CHECK_BEGIN_STATIC_FN(bitset_test_push_back_no_reallocate)
{
    CCC_Bitset bs
        = CCC_bitset_initialize(CCC_bitset_blocks(16), NULL, NULL, 16, 0);
    CHECK(CCC_bitset_capacity(&bs).count, 16);
    CHECK(CCC_bitset_count(&bs).count, 0);
    CCC_Result push_status = CCC_RESULT_OK;
    for (size_t i = 0; push_status == CCC_RESULT_OK; ++i)
    {
        if (i % 2)
        {
            push_status = CCC_bitset_push_back(&bs, CCC_TRUE);
        }
        else
        {
            push_status = CCC_bitset_push_back(&bs, CCC_FALSE);
        }
    }
    CHECK(push_status, CCC_RESULT_NO_ALLOCATION_FUNCTION);
    CHECK(CCC_bitset_count(&bs).count, 16);
    CHECK(CCC_bitset_popcount(&bs).count, 16 / 2);
    CHECK(CCC_bitset_clear(&bs), CCC_RESULT_OK);
    CHECK(CCC_bitset_count(&bs).count, 0);
    CHECK(CCC_bitset_popcount(&bs).count, 0);
    CHECK(CCC_bitset_capacity(&bs).count, 16);
    CHECK(CCC_bitset_clear_and_free(&bs), CCC_RESULT_NO_ALLOCATION_FUNCTION);
    CHECK(CCC_bitset_capacity(&bs).count, 16);
    CHECK(CCC_bitset_count(&bs).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_push_back_allocate)
{
    CCC_Bitset bs = CCC_bitset_initialize(NULL, std_allocate, NULL, 0);
    CHECK(CCC_bitset_capacity(&bs).count, 0);
    CHECK(CCC_bitset_count(&bs).count, 0);
    for (size_t i = 0; CCC_bitset_count(&bs).count < 16; ++i)
    {
        if (i % 2)
        {
            CHECK(CCC_bitset_push_back(&bs, CCC_TRUE), CCC_RESULT_OK);
        }
        else
        {
            CHECK(CCC_bitset_push_back(&bs, CCC_FALSE), CCC_RESULT_OK);
        }
    }
    CHECK(CCC_bitset_count(&bs).count, 16);
    CHECK(CCC_bitset_popcount(&bs).count, 16 / 2);
    CHECK(CCC_bitset_clear(&bs), CCC_RESULT_OK);
    CHECK(CCC_bitset_count(&bs).count, 0);
    CHECK(CCC_bitset_popcount(&bs).count, 0);
    CHECK(CCC_bitset_capacity(&bs).count != 0, true);
    CHECK(CCC_bitset_clear_and_free(&bs), CCC_RESULT_OK);
    CHECK(CCC_bitset_capacity(&bs).count, 0);
    CHECK(CCC_bitset_count(&bs).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(bitset_test_push_back_reserve)
{
    CCC_Bitset bs = CCC_bitset_initialize(NULL, NULL, NULL, 0);
    CCC_Result const r = reserve(&bs, 512, std_allocate);
    CHECK(r, CCC_RESULT_OK);
    CHECK(CCC_bitset_count(&bs).count, 0);
    CHECK(CCC_bitset_capacity(&bs).count != 0, true);
    for (size_t i = 0; CCC_bitset_count(&bs).count < 512; ++i)
    {
        if (i % 2)
        {
            CHECK(CCC_bitset_push_back(&bs, CCC_TRUE), CCC_RESULT_OK);
        }
        else
        {
            CHECK(CCC_bitset_push_back(&bs, CCC_FALSE), CCC_RESULT_OK);
        }
    }
    CHECK(CCC_bitset_count(&bs).count, 512);
    CHECK(CCC_bitset_popcount(&bs).count, 512 / 2);
    CHECK(clear(&bs), CCC_RESULT_OK);
    CHECK(CCC_bitset_count(&bs).count, 0);
    CHECK(CCC_bitset_popcount(&bs).count, 0);
    CHECK(CCC_bitset_capacity(&bs).count != 0, true);
    CHECK(clear_and_free_reserve(&bs, std_allocate), CCC_RESULT_OK);
    CHECK(CCC_bitset_capacity(&bs).count, 0);
    CHECK(CCC_bitset_count(&bs).count, 0);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(bitset_test_push_back_no_reallocate(),
                     bitset_test_push_back_allocate(),
                     bitset_test_push_back_reserve());
}
