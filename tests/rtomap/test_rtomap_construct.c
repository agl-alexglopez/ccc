#include "realtime_ordered_map.h"
#include "rtomap_util.h"
#include "test.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

static enum test_result rtomap_test_empty(void);

#define NUM_TESTS ((size_t)1)
test_fn const all_tests[NUM_TESTS] = {
    rtomap_test_empty,
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        bool const fail = all_tests[i]() == FAIL;
        if (fail)
        {
            res = FAIL;
        }
    }
    return res;
}

static enum test_result
rtomap_test_empty(void)
{
    ccc_realtime_ordered_map s
        = ccc_rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    CHECK(ccc_rom_empty(&s), true);
    return PASS;
}
