#include <stddef.h>

#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"

CHECK_BEGIN_STATIC_FN(bs_test_construct)
{
    ccc_bitset bs
        = ccc_bs_init((ccc_bitblock[ccc_bs_blocks(10)]){}, 10, NULL, NULL);
    CHECK(ccc_bs_popcount(&bs), 0);
    for (size_t i = 0; i < ccc_bs_capacity(&bs); ++i)
    {
        CHECK(ccc_bs_test(&bs, i), CCC_FALSE);
        CHECK(ccc_bs_test_at(&bs, i), CCC_FALSE);
    }
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(bs_test_construct());
}
