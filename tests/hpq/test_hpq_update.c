#include "heap_pqueue.h"
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
    ccc_fpq_elem elem;
};

static enum test_result fpq_test_insert_iterate_pop(void);
static enum test_result fpq_test_priority_update(void);
static enum test_result fpq_test_priority_removal(void);
static void val_update(ccc_fpq_elem *, void *);
static ccc_fpq_threeway_cmp val_cmp(ccc_fpq_elem const *, ccc_fpq_elem const *,
                                    void *);

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
    ccc_flat_pqueue pq;
    ccc_fpq_init(&pq, HPQLES, val_cmp, NULL);
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
        ccc_fpq_push(&pq, &vals[i].elem);
        CHECK(ccc_fpq_validate(&pq), true, bool, "%d");
    }
    size_t pop_count = 0;
    while (!ccc_fpq_empty(&pq))
    {
        ccc_fpq_pop(&pq);
        ++pop_count;
        CHECK(ccc_fpq_validate(&pq), true, bool, "%d");
    }
    CHECK(pop_count, num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
fpq_test_priority_removal(void)
{
    ccc_flat_pqueue pq;
    ccc_fpq_init(&pq, HPQLES, val_cmp, NULL);
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
        ccc_fpq_push(&pq, &vals[i].elem);
        CHECK(ccc_fpq_validate(&pq), true, bool, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        ccc_fpq_elem *i = &vals[val].elem;
        struct val *cur = HPQ_ENTRY(i, struct val, elem);
        if (cur->val > limit)
        {
            (void)ccc_fpq_erase(&pq, i);
            CHECK(ccc_fpq_validate(&pq), true, bool, "%d");
        }
    }
    return PASS;
}

static enum test_result
fpq_test_priority_update(void)
{
    ccc_flat_pqueue pq;
    ccc_fpq_init(&pq, HPQLES, val_cmp, NULL);
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
        ccc_fpq_push(&pq, &vals[i].elem);
        CHECK(ccc_fpq_validate(&pq), true, bool, "%d");
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        ccc_fpq_elem *i = &vals[val].elem;
        struct val *cur = HPQ_ENTRY(i, struct val, elem);
        int backoff = cur->val / 2;
        if (cur->val > limit)
        {
            CHECK(ccc_fpq_update(&pq, i, val_update, &backoff), true, bool,
                  "%d");
            CHECK(ccc_fpq_validate(&pq), true, bool, "%d");
        }
    }
    CHECK(ccc_fpq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static ccc_fpq_threeway_cmp
val_cmp(ccc_fpq_elem const *a, ccc_fpq_elem const *b, void *aux)
{
    (void)aux;
    struct val *lhs = HPQ_ENTRY(a, struct val, elem);
    struct val *rhs = HPQ_ENTRY(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static void
val_update(ccc_fpq_elem *a, void *aux)
{
    struct val *old = HPQ_ENTRY(a, struct val, elem);
    old->val = *(int *)aux;
}
