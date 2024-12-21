#include <stddef.h>

#include "ccc/bitset.h"
#include "checkers.h"

CHECK_BEGIN_STATIC_FN(bs_test_push_back)
{
    ccc_bitset bs = ccc_bs_init(NULL, 0, NULL, NULL);
    CHECK(ccc_bs_capacity(&bs), 0);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(bs_test_push_back());
}
