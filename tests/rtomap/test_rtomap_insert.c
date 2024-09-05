#include "realtime_ordered_map.h"
#include "rtomap_util.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static enum test_result rtomap_test_insert_one(void);
static enum test_result rtomap_test_insert_shuffle(void);

#define NUM_TESTS ((size_t)2)
test_fn const all_tests[NUM_TESTS] = {
    rtomap_test_insert_one,
    rtomap_test_insert_shuffle,
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
rtomap_test_insert_one(void)
{
    ccc_realtime_ordered_map s
        = CCC_ROM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    struct val single;
    single.val = 0;
    CHECK(ccc_rom_unwrap(ccc_rom_insert(&s, &single.elem)) == NULL, true, "%d");
    CHECK(ccc_rom_empty(&s), false, "%d");
    CHECK(((struct val *)ccc_rom_root(&s))->val == single.val, true, "%d");
    return PASS;
}

static enum test_result
rtomap_test_insert_shuffle(void)
{
    ccc_realtime_ordered_map s
        = CCC_ROM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&s, vals, size, prime), PASS, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &s), size, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], "%d");
    }
    return PASS;
}
