#include "pqueue.h"
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int id;
    int val;
    struct pq_elem elem;
};

static enum test_result pq_test_insert_iterate_pop(void);
static enum test_result pq_test_priority_update(void);
static enum test_result pq_test_priority_increase(void);
static enum test_result pq_test_priority_decrease(void);
static enum test_result pq_test_priority_removal(void);
static void val_update(struct pq_elem *, void *);
static enum pq_threeway_cmp val_cmp(const struct pq_elem *,
                                    const struct pq_elem *, void *);

#define NUM_TESTS (size_t)5
const test_fn all_tests[NUM_TESTS] = {
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
        const bool fail = all_tests[i]() == FAIL;
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
    struct pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const size_t num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        pq_push(&pq, &vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%b");
    }
    size_t pop_count = 0;
    while (!pq_empty(&pq))
    {
        pq_pop(&pq);
        ++pop_count;
        CHECK(pq_validate(&pq), true, bool, "%b");
    }
    CHECK(pop_count, num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_removal(void)
{
    struct pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const size_t num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        pq_push(&pq, &vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%b");
    }
    const int limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct pq_elem *i = &vals[val].elem;
        struct val *cur = PQ_ENTRY(i, struct val, elem);
        if (cur->val > limit)
        {
            i = pq_erase(&pq, i);
            CHECK(pq_validate(&pq), true, bool, "%b");
        }
    }
    return PASS;
}

static enum test_result
pq_test_priority_update(void)
{
    struct pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const size_t num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        pq_push(&pq, &vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%b");
    }
    const int limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct pq_elem *i = &vals[val].elem;
        struct val *cur = PQ_ENTRY(i, struct val, elem);
        int backoff = cur->val / 2;
        if (cur->val > limit)
        {
            CHECK(pq_update(&pq, i, val_update, &backoff), true, bool, "%b");
            CHECK(pq_validate(&pq), true, bool, "%b");
        }
    }
    CHECK(pq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_increase(void)
{
    struct pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const size_t num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        pq_push(&pq, &vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%b");
    }
    const int limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct pq_elem *i = &vals[val].elem;
        struct val *cur = PQ_ENTRY(i, struct val, elem);
        int inc = limit * 2;
        int dec = cur->val / 2;
        if (cur->val > limit)
        {
            CHECK(pq_decrease(&pq, i, val_update, &dec), true, bool, "%b");
            CHECK(pq_validate(&pq), true, bool, "%b");
        }
        else
        {
            CHECK(pq_increase(&pq, i, val_update, &inc), true, bool, "%b");
            CHECK(pq_validate(&pq), true, bool, "%b");
        }
    }
    CHECK(pq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_decrease(void)
{
    struct pqueue pq = PQ_INIT(PQGRT, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const size_t num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        pq_push(&pq, &vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%b");
    }
    const int limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct pq_elem *i = &vals[val].elem;
        struct val *cur = PQ_ENTRY(i, struct val, elem);
        int inc = limit * 2;
        int dec = cur->val / 2;
        if (cur->val < limit)
        {
            CHECK(pq_increase(&pq, i, val_update, &inc), true, bool, "%b");
            CHECK(pq_validate(&pq), true, bool, "%b");
        }
        else
        {
            CHECK(pq_decrease(&pq, i, val_update, &dec), true, bool, "%b");
            CHECK(pq_validate(&pq), true, bool, "%b");
        }
    }
    CHECK(pq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum pq_threeway_cmp
val_cmp(const struct pq_elem *a, const struct pq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = PQ_ENTRY(a, struct val, elem);
    struct val *rhs = PQ_ENTRY(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static void
val_update(struct pq_elem *a, void *aux)
{
    struct val *old = PQ_ENTRY(a, struct val, elem);
    old->val = *(int *)aux;
}
