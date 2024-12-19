#include <stddef.h>

#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"

CHECK_BEGIN_STATIC_FN(btst_test_construct)
{
    ccc_bitset btst
        = ccc_btst_init((ccc_bitblock[ccc_bitblocks(10)]){}, 10, NULL, NULL);
    CHECK(ccc_btst_popcount(&btst), 0);
    for (size_t i = 0; i < ccc_btst_capacity(&btst); ++i)
    {
        CHECK(ccc_btst_test(&btst, i), CCC_FALSE);
        CHECK(ccc_btst_test_at(&btst, i), CCC_FALSE);
    }
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(btst_test_construct());
}
