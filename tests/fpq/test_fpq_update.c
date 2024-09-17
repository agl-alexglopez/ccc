#include "flat_priority_queue.h"
#include "fpq_util.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static enum test_result fpq_test_insert_iterate_pop(void);
static enum test_result fpq_test_priority_update(void);
static enum test_result fpq_test_priority_removal(void);

#define NUM_TESTS (size_t)3
test_fn const all_tests[NUM_TESTS] = {
    fpq_test_insert_iterate_pop,
    fpq_test_priority_update,
    fpq_test_priority_removal,
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
fpq_test_insert_iterate_pop(void)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(1);
    size_t const num_nodes = 1000;
    struct val vals[1000 + 1];
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(vals, (sizeof(vals) / sizeof(vals[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(ccc_fpq_push(&fpq, &vals[i]) != NULL, true);
        CHECK(ccc_fpq_validate(&fpq), true);
    }
    size_t pop_count = 0;
    while (!ccc_fpq_empty(&fpq))
    {
        ccc_fpq_pop(&fpq);
        ++pop_count;
        CHECK(ccc_fpq_validate(&fpq), true);
    }
    CHECK(pop_count, num_nodes);
    return PASS;
}

static enum test_result
fpq_test_priority_removal(void)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[1000 + 1];
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(vals, (sizeof(vals) / sizeof(vals[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        struct val const *res = ccc_fpq_emplace(
            &fpq, (struct val){
                      .val = rand() % (num_nodes + 1), /*NOLINT*/
                      .id = (int)i,
                  });
        CHECK(res != NULL, true);
        CHECK(ccc_fpq_validate(&fpq), true);
    }
    int const limit = 400;
    for (size_t seen = 0, remaining = num_nodes; seen < remaining; ++seen)
    {
        struct val *cur = &vals[seen];
        if (cur->val > limit)
        {
            (void)ccc_fpq_erase(&fpq, cur);
            CHECK(ccc_fpq_validate(&fpq), true);
            --remaining;
        }
    }
    return PASS;
}

static enum test_result
fpq_test_priority_update(void)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[1000 + 1];
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(vals, (sizeof(vals) / sizeof(vals[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        struct val const *res = ccc_fpq_emplace(
            &fpq, (struct val){
                      .val = rand() % (num_nodes + 1), /*NOLINT*/
                      .id = (int)i,
                  });
        CHECK(res != NULL, true);
        CHECK(ccc_fpq_validate(&fpq), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *cur = &vals[val];
        int backoff = cur->val / 2;
        if (cur->val > limit)
        {
            CHECK(ccc_fpq_update(&fpq, cur, val_update, &backoff), true);
            CHECK(ccc_fpq_validate(&fpq), true);
        }
    }
    CHECK(ccc_fpq_size(&fpq), num_nodes);
    return PASS;
}
