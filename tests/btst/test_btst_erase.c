#include <stddef.h>

#include "ccc/bitset.h"
#include "checkers.h"

CHECK_BEGIN_STATIC_FN(btst_test_pop_back)
{
    ccc_bitset btst = ccc_btst_init(NULL, 0, NULL, NULL);
    CHECK(ccc_btst_capacity(&btst), 0);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(btst_test_pop_back());
}
