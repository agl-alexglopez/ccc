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
    pq_elem elem;
};

static enum test_result pq_test_insert_iterate_pop(void);
static enum test_result pq_test_priority_update(void);
static enum test_result pq_test_priority_increase(void);
static enum test_result pq_test_priority_decrease(void);
static enum test_result pq_test_priority_removal(void);
static void val_update(pq_elem *, void *);
static pq_threeway_cmp val_cmp(pq_elem const *, pq_elem const *, void *);

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
    pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
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
        pq_push(&pq, &vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%d");
    }
    size_t pop_count = 0;
    while (!pq_empty(&pq))
    {
        pq_pop(&pq);
        ++pop_count;
        CHECK(pq_validate(&pq), true, bool, "%d");
    }
    CHECK(pop_count, num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_removal(void)
{
    pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
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
        pq_push(&pq, &vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        pq_elem *i = &vals[val].elem;
        struct val *cur = PQ_ENTRY(i, struct val, elem);
        if (cur->val > limit)
        {
            (void)pq_erase(&pq, i);
            CHECK(pq_validate(&pq), true, bool, "%d");
        }
    }
    return PASS;
}

static enum test_result
pq_test_priority_update(void)
{
    pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
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
        pq_push(&pq, &vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        pq_elem *i = &vals[val].elem;
        struct val *cur = PQ_ENTRY(i, struct val, elem);
        int backoff = cur->val / 2;
        if (cur->val > limit)
        {
            CHECK(pq_update(&pq, i, val_update, &backoff), true, bool, "%d");
            CHECK(pq_validate(&pq), true, bool, "%d");
        }
    }
    CHECK(pq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_increase(void)
{
    pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
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
        pq_push(&pq, &vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        pq_elem *i = &vals[val].elem;
        struct val *cur = PQ_ENTRY(i, struct val, elem);
        int inc = limit * 2;
        int dec = cur->val / 2;
        if (cur->val > limit)
        {
            CHECK(pq_decrease(&pq, i, val_update, &dec), true, bool, "%d");
            CHECK(pq_validate(&pq), true, bool, "%d");
        }
        else
        {
            CHECK(pq_increase(&pq, i, val_update, &inc), true, bool, "%d");
            CHECK(pq_validate(&pq), true, bool, "%d");
        }
    }
    CHECK(pq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_decrease(void)
{
    pqueue pq = PQ_INIT(PQGRT, val_cmp, NULL);
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
        pq_push(&pq, &vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        pq_elem *i = &vals[val].elem;
        struct val *cur = PQ_ENTRY(i, struct val, elem);
        int inc = limit * 2;
        int dec = cur->val / 2;
        if (cur->val < limit)
        {
            CHECK(pq_increase(&pq, i, val_update, &inc), true, bool, "%d");
            CHECK(pq_validate(&pq), true, bool, "%d");
        }
        else
        {
            CHECK(pq_decrease(&pq, i, val_update, &dec), true, bool, "%d");
            CHECK(pq_validate(&pq), true, bool, "%d");
        }
    }
    CHECK(pq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static pq_threeway_cmp
val_cmp(pq_elem const *a, pq_elem const *b, void *aux)
{
    (void)aux;
    struct val *lhs = PQ_ENTRY(a, struct val, elem);
    struct val *rhs = PQ_ENTRY(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static void
val_update(pq_elem *a, void *aux)
{
    struct val *old = PQ_ENTRY(a, struct val, elem);
    old->val = *(int *)aux;
}
