#include "pqueue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int id;
    int val;
    ccc_pq_elem elem;
};

static enum test_result pq_test_insert_iterate_pop(void);
static enum test_result pq_test_priority_update(void);
static enum test_result pq_test_priority_increase(void);
static enum test_result pq_test_priority_decrease(void);
static enum test_result pq_test_priority_removal(void);
static void val_update(void *, void *);
static ccc_threeway_cmp val_cmp(void const *, void const *, void *);

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
    ccc_pqueue pq = CCC_PQ_INIT(struct val, elem, CCC_LES, val_cmp, NULL);
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
        ccc_pq_push(&pq, &vals[i]);
        CHECK(ccc_pq_validate(&pq), true, bool, "%d");
    }
    size_t pop_count = 0;
    while (!ccc_pq_empty(&pq))
    {
        ccc_pq_pop(&pq);
        ++pop_count;
        CHECK(ccc_pq_validate(&pq), true, bool, "%d");
    }
    CHECK(pop_count, num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_removal(void)
{
    ccc_pqueue pq = CCC_PQ_INIT(struct val, elem, CCC_LES, val_cmp, NULL);
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
        ccc_pq_push(&pq, &vals[i]);
        CHECK(ccc_pq_validate(&pq), true, bool, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *i = &vals[val];
        if (i->val > limit)
        {
            (void)ccc_pq_erase(&pq, i);
            CHECK(ccc_pq_validate(&pq), true, bool, "%d");
        }
    }
    return PASS;
}

static enum test_result
pq_test_priority_update(void)
{
    ccc_pqueue pq = CCC_PQ_INIT(struct val, elem, CCC_LES, val_cmp, NULL);
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
        ccc_pq_push(&pq, &vals[i]);
        CHECK(ccc_pq_validate(&pq), true, bool, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *i = &vals[val];
        int backoff = i->val / 2;
        if (i->val > limit)
        {
            CHECK(ccc_pq_update(&pq, i, val_update, &backoff), true, bool,
                  "%d");
            CHECK(ccc_pq_validate(&pq), true, bool, "%d");
        }
    }
    CHECK(ccc_pq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_increase(void)
{
    ccc_pqueue pq = CCC_PQ_INIT(struct val, elem, CCC_LES, val_cmp, NULL);
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
        ccc_pq_push(&pq, &vals[i]);
        CHECK(ccc_pq_validate(&pq), true, bool, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *const i = &vals[val];
        int inc = limit * 2;
        int dec = i->val / 2;
        if (i->val > limit)
        {
            CHECK(ccc_pq_decrease(&pq, i, val_update, &dec), true, bool, "%d");
            CHECK(ccc_pq_validate(&pq), true, bool, "%d");
        }
        else
        {
            CHECK(ccc_pq_increase(&pq, i, val_update, &inc), true, bool, "%d");
            CHECK(ccc_pq_validate(&pq), true, bool, "%d");
        }
    }
    CHECK(ccc_pq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_decrease(void)
{
    ccc_pqueue pq = CCC_PQ_INIT(struct val, elem, CCC_GRT, val_cmp, NULL);
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
        ccc_pq_push(&pq, &vals[i]);
        CHECK(ccc_pq_validate(&pq), true, bool, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *const i = &vals[val];
        int inc = limit * 2;
        int dec = i->val / 2;
        if (i->val < limit)
        {
            CHECK(ccc_pq_increase(&pq, i, val_update, &inc), true, bool, "%d");
            CHECK(ccc_pq_validate(&pq), true, bool, "%d");
        }
        else
        {
            CHECK(ccc_pq_decrease(&pq, i, val_update, &dec), true, bool, "%d");
            CHECK(ccc_pq_validate(&pq), true, bool, "%d");
        }
    }
    CHECK(ccc_pq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static ccc_threeway_cmp
val_cmp(void const *const a, void const *const b, void *aux)
{
    (void)aux;
    struct val const *const lhs = a;
    struct val const *const rhs = b;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static void
val_update(void *const a, void *aux)
{
    struct val *const old = a;
    old->val = *(int *)aux;
}
