#include <stdbool.h>
#include <stddef.h>

#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"
#include "utility/allocate.h"

check_static_begin(bitset_test_push_pop_back_no_reallocate)
{
    CCC_Bitset bs
        = CCC_bitset_initialize(CCC_bitset_blocks(16), NULL, NULL, 16, 0);
    check(CCC_bitset_capacity(&bs).count, 16);
    check(CCC_bitset_count(&bs).count, 0);
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
    check(push_status, CCC_RESULT_NO_ALLOCATION_FUNCTION);
    check(CCC_bitset_count(&bs).count, 16);
    check(CCC_bitset_popcount(&bs).count, 16 / 2);
    while (!CCC_bitset_empty(&bs))
    {
        CCC_Tribool const msb = CCC_bitset_pop_back(&bs);
        if (CCC_bitset_count(&bs).count % 2)
        {
            check(msb, CCC_TRUE);
        }
        else
        {
            check(msb, CCC_FALSE);
        }
    }
    check(CCC_bitset_count(&bs).count, 0);
    check(CCC_bitset_popcount(&bs).count, 0);
    check(CCC_bitset_capacity(&bs).count, 16);
    check(CCC_bitset_clear(&bs), CCC_RESULT_OK);
    check(CCC_bitset_capacity(&bs).count, 16);
    check(CCC_bitset_clear_and_free(&bs), CCC_RESULT_NO_ALLOCATION_FUNCTION);
    check(CCC_bitset_capacity(&bs).count, 16);
    check(CCC_bitset_count(&bs).count, 0);
    check_end();
}

check_static_begin(bitset_test_push_pop_back_allocate)
{
    CCC_Bitset bs = CCC_bitset_initialize(NULL, std_allocate, NULL, 0);
    check(CCC_bitset_capacity(&bs).count, 0);
    check(CCC_bitset_count(&bs).count, 0);
    for (size_t i = 0; CCC_bitset_count(&bs).count < 16; ++i)
    {
        if (i % 2)
        {
            check(CCC_bitset_push_back(&bs, CCC_TRUE), CCC_RESULT_OK);
        }
        else
        {
            check(CCC_bitset_push_back(&bs, CCC_FALSE), CCC_RESULT_OK);
        }
    }
    check(CCC_bitset_count(&bs).count, 16);
    check(CCC_bitset_popcount(&bs).count, 16 / 2);
    while (!CCC_bitset_empty(&bs))
    {
        CCC_Tribool const msb_was = CCC_bitset_pop_back(&bs);
        if (CCC_bitset_count(&bs).count % 2)
        {
            check(msb_was, CCC_TRUE);
        }
        else
        {
            check(msb_was, CCC_FALSE);
        }
    }
    check(CCC_bitset_pop_back(&bs), (CCC_Tribool)CCC_TRIBOOL_ERROR);
    check(CCC_bitset_count(&bs).count, 0);
    check(CCC_bitset_popcount(&bs).count, 0);
    check(CCC_bitset_capacity(&bs).count != 0, true);
    check(CCC_bitset_clear(&bs), CCC_RESULT_OK);
    check(CCC_bitset_capacity(&bs).count != 0, true);
    check(CCC_bitset_clear_and_free(&bs), CCC_RESULT_OK);
    check(CCC_bitset_capacity(&bs).count, 0);
    check(CCC_bitset_count(&bs).count, 0);
    check_end();
}

int
main(void)
{
    return check_run(bitset_test_push_pop_back_no_reallocate(),
                     bitset_test_push_pop_back_allocate());
}
