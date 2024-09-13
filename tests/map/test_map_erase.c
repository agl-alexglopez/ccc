#define TRAITS_USING_NAMESPACE_CCC

#include "map_util.h"
#include "ordered_map.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static enum test_result map_test_insert_erase_shuffled(void);
static enum test_result map_test_prime_shuffle(void);
static enum test_result map_test_weak_srand(void);

#define NUM_TESTS ((size_t)3)
test_fn const all_tests[NUM_TESTS] = {
    map_test_insert_erase_shuffled,
    map_test_prime_shuffle,
    map_test_weak_srand,
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
map_test_prime_shuffle(void)
{
    ccc_ordered_map s
        = CCC_OM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    size_t const size = 50;
    size_t const prime = 53;
    size_t const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    size_t shuffled_index = prime % (size - less);
    struct val vals[size];
    bool repeats[size];
    memset(repeats, false, sizeof(bool) * size);
    for (size_t i = 0; i < size; ++i)
    {
        vals[i].val = (int)shuffled_index;
        vals[i].id = (int)shuffled_index;
        if (occupied(insert_vr(&s, &vals[i].elem)))
        {
            repeats[i] = true;
        }
        CHECK(ccc_om_validate(&s), true, "%d");
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* One test can use our printer function as test output */
    ccc_om_print(&s, &((struct val *)ccc_om_root(&s))->elem, map_printer_fn);
    CHECK(ccc_om_size(&s) < size, true, "%d");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(occupied(remove_entry_vr(entry_vr(&s, &vals[i].val)))
                  || repeats[i],
              true, "%d");
        CHECK(ccc_om_validate(&s), true, "%d");
    }
    return PASS;
}

static enum test_result
map_test_insert_erase_shuffled(void)
{
    ccc_ordered_map s
        = CCC_OM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
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
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        struct val *v = unwrap(remove_vr(&s, &vals[i].elem));
        CHECK(v != NULL, true, "%d");
        CHECK(v->val, vals[i].val, "%d");
        CHECK(ccc_om_validate(&s), true, "%d");
    }
    CHECK(ccc_om_empty(&s), true, "%d");
    return PASS;
}

static enum test_result
map_test_weak_srand(void)
{
    ccc_ordered_map s
        = CCC_OM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_nodes = 1000;
    struct val vals[num_nodes];
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        (void)insert(&s, &vals[i].elem);
        CHECK(ccc_om_validate(&s), true, "%d");
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CHECK(ccc_om_contains(&s, &vals[i].val), true, "%d");
        (void)remove(&s, &vals[i].elem);
        CHECK(ccc_om_validate(&s), true, "%d");
    }
    CHECK(ccc_om_empty(&s), true, "%d");
    return PASS;
}
