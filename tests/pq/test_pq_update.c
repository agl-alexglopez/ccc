#include "pq_util.h"
#include "priority_queue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static enum test_result pq_test_insert_iterate_pop(void);
static enum test_result pq_test_priority_update(void);
static enum test_result pq_test_priority_increase(void);
static enum test_result pq_test_priority_decrease(void);
static enum test_result pq_test_priority_removal(void);

#define NUM_TESTS (size_t)5
test_fn const all_tests[NUM_TESTS] = {
    pq_test_insert_iterate_pop, pq_test_priority_update,
    pq_test_priority_removal,   pq_test_priority_increase,
    pq_test_priority_decrease,
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
pq_test_insert_iterate_pop(void)
{
    ccc_priority_queue pq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        ccc_pq_push(&pq, &vals[i].elem);
        CHECK(ccc_pq_validate(&pq), true, "%d");
    }
    size_t pop_count = 0;
    while (!ccc_pq_empty(&pq))
    {
        ccc_pq_pop(&pq);
        ++pop_count;
        CHECK(ccc_pq_validate(&pq), true, "%d");
    }
    CHECK(pop_count, num_nodes, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_removal(void)
{
    ccc_priority_queue pq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        ccc_pq_push(&pq, &vals[i].elem);
        CHECK(ccc_pq_validate(&pq), true, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *i = &vals[val];
        if (i->val > limit)
        {
            (void)ccc_pq_erase(&pq, &i->elem);
            CHECK(ccc_pq_validate(&pq), true, "%d");
        }
    }
    return PASS;
}

static enum test_result
pq_test_priority_update(void)
{
    ccc_priority_queue pq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        ccc_pq_push(&pq, &vals[i].elem);
        CHECK(ccc_pq_validate(&pq), true, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *i = &vals[val];
        int backoff = i->val / 2;
        if (i->val > limit)
        {
            CHECK(ccc_pq_update(&pq, &i->elem, val_update, &backoff), true,
                  "%d");
            CHECK(ccc_pq_validate(&pq), true, "%d");
        }
    }
    CHECK(ccc_pq_size(&pq), num_nodes, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_increase(void)
{
    ccc_priority_queue pq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        ccc_pq_push(&pq, &vals[i].elem);
        CHECK(ccc_pq_validate(&pq), true, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *const i = &vals[val];
        int inc = limit * 2;
        int dec = i->val / 2;
        if (i->val > limit)
        {
            CHECK(ccc_pq_decrease(&pq, &i->elem, val_update, &dec), true, "%d");
            CHECK(ccc_pq_validate(&pq), true, "%d");
        }
        else
        {
            CHECK(ccc_pq_increase(&pq, &i->elem, val_update, &inc), true, "%d");
            CHECK(ccc_pq_validate(&pq), true, "%d");
        }
    }
    CHECK(ccc_pq_size(&pq), num_nodes, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_decrease(void)
{
    ccc_priority_queue pq
        = CCC_PQ_INIT(struct val, elem, CCC_GRT, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        ccc_pq_push(&pq, &vals[i].elem);
        CHECK(ccc_pq_validate(&pq), true, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *const i = &vals[val];
        int inc = limit * 2;
        int dec = i->val / 2;
        if (i->val < limit)
        {
            CHECK(ccc_pq_increase(&pq, &i->elem, val_update, &inc), true, "%d");
            CHECK(ccc_pq_validate(&pq), true, "%d");
        }
        else
        {
            CHECK(ccc_pq_decrease(&pq, &i->elem, val_update, &dec), true, "%d");
            CHECK(ccc_pq_validate(&pq), true, "%d");
        }
    }
    CHECK(ccc_pq_size(&pq), num_nodes, "%zu");
    return PASS;
}
