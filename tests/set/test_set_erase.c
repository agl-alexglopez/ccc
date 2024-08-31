#include "set.h"
#include "set_util.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static enum test_result set_test_insert_erase_shuffled(void);
static enum test_result set_test_prime_shuffle(void);
static enum test_result set_test_weak_srand(void);

#define NUM_TESTS ((size_t)3)
test_fn const all_tests[NUM_TESTS] = {
    set_test_insert_erase_shuffled,
    set_test_prime_shuffle,
    set_test_weak_srand,
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
set_test_prime_shuffle(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
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
        if (ccc_s_get(ccc_s_insert(&s, &vals[i].elem)))
        {
            repeats[i] = true;
        }
        CHECK(ccc_s_validate(&s), true, "%d");
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* One test can use our printer function as test output */
    ccc_s_print(&s, &((struct val *)ccc_s_root(&s))->elem, set_printer_fn);
    CHECK(ccc_s_size(&s) < size, true, "%d");
    for (size_t i = 0; i < size; ++i)
    {
        void *const elem = ccc_s_remove(&s, &vals[i].elem);
        CHECK(elem || repeats[i], true, "%d");
        CHECK(ccc_s_validate(&s), true, "%d");
    }
    return PASS;
}

static enum test_result
set_test_insert_erase_shuffled(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
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
        (void)ccc_s_remove(&s, &vals[i].elem);
        CHECK(ccc_s_validate(&s), true, "%d");
    }
    CHECK(ccc_s_empty(&s), true, "%d");
    return PASS;
}

static enum test_result
set_test_weak_srand(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_nodes = 1000;
    struct val vals[num_nodes];
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        (void)ccc_s_insert(&s, &vals[i].elem);
        CHECK(ccc_s_validate(&s), true, "%d");
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CHECK(ccc_s_contains(&s, &vals[i].val), true, "%d");
        (void)ccc_s_remove(&s, &vals[i].elem);
        CHECK(ccc_s_validate(&s), true, "%d");
    }
    CHECK(ccc_s_empty(&s), true, "%d");
    return PASS;
}
