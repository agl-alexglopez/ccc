#include "pair_pqueue.h"
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int id;
    int val;
    struct ppq_elem elem;
};

static enum test_result ppq_test_insert_iterate_pop(void);
static enum test_result ppq_test_priority_update(void);
static enum test_result ppq_test_priority_increase(void);
static enum test_result ppq_test_priority_decrease(void);
static enum test_result ppq_test_priority_removal(void);
static void val_update(struct ppq_elem *, void *);
static enum ppq_threeway_cmp val_cmp(const struct ppq_elem *,
                                     const struct ppq_elem *, void *);

#define NUM_TESTS (size_t)5
const test_fn all_tests[NUM_TESTS] = {
    ppq_test_insert_iterate_pop, ppq_test_priority_update,
    ppq_test_priority_removal,   ppq_test_priority_increase,
    ppq_test_priority_decrease,
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
ppq_test_insert_iterate_pop(void)
{
    struct pair_pqueue pq;
    ppq_init(&pq, PPQLES, val_cmp, NULL);
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
        ppq_push(&pq, &vals[i].elem);
        CHECK(ppq_validate(&pq), true, bool, "%b");
    }
    size_t pop_count = 0;
    while (!ppq_empty(&pq))
    {
        ppq_pop(&pq);
        ++pop_count;
        CHECK(ppq_validate(&pq), true, bool, "%b");
    }
    CHECK(pop_count, num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
ppq_test_priority_removal(void)
{
    struct pair_pqueue pq;
    ppq_init(&pq, PPQLES, val_cmp, NULL);
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
        ppq_push(&pq, &vals[i].elem);
        CHECK(ppq_validate(&pq), true, bool, "%b");
    }
    const int limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct ppq_elem *i = &vals[val].elem;
        struct val *cur = ppq_entry(i, struct val, elem);
        if (cur->val > limit)
        {
            i = ppq_erase(&pq, i);
            CHECK(ppq_validate(&pq), true, bool, "%b");
        }
    }
    return PASS;
}

static enum test_result
ppq_test_priority_update(void)
{
    struct pair_pqueue pq;
    ppq_init(&pq, PPQLES, val_cmp, NULL);
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
        ppq_push(&pq, &vals[i].elem);
        CHECK(ppq_validate(&pq), true, bool, "%b");
    }
    const int limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct ppq_elem *i = &vals[val].elem;
        struct val *cur = ppq_entry(i, struct val, elem);
        int backoff = cur->val / 2;
        if (cur->val > limit)
        {
            CHECK(ppq_update(&pq, i, val_update, &backoff), true, bool, "%b");
            CHECK(ppq_validate(&pq), true, bool, "%b");
        }
    }
    CHECK(ppq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
ppq_test_priority_increase(void)
{
    struct pair_pqueue pq;
    ppq_init(&pq, PPQLES, val_cmp, NULL);
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
        ppq_push(&pq, &vals[i].elem);
        CHECK(ppq_validate(&pq), true, bool, "%b");
    }
    const int limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct ppq_elem *i = &vals[val].elem;
        struct val *cur = ppq_entry(i, struct val, elem);
        int inc = limit * 2;
        int dec = cur->val / 2;
        if (cur->val > limit)
        {
            CHECK(ppq_decrease(&pq, i, val_update, &dec), true, bool, "%b");
            CHECK(ppq_validate(&pq), true, bool, "%b");
        }
        else
        {
            CHECK(ppq_increase(&pq, i, val_update, &inc), true, bool, "%b");
            CHECK(ppq_validate(&pq), true, bool, "%b");
        }
    }
    CHECK(ppq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
ppq_test_priority_decrease(void)
{
    struct pair_pqueue pq;
    ppq_init(&pq, PPQGRT, val_cmp, NULL);
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
        ppq_push(&pq, &vals[i].elem);
        CHECK(ppq_validate(&pq), true, bool, "%b");
    }
    const int limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct ppq_elem *i = &vals[val].elem;
        struct val *cur = ppq_entry(i, struct val, elem);
        int inc = limit * 2;
        int dec = cur->val / 2;
        if (cur->val < limit)
        {
            CHECK(ppq_increase(&pq, i, val_update, &inc), true, bool, "%b");
            CHECK(ppq_validate(&pq), true, bool, "%b");
        }
        else
        {
            CHECK(ppq_decrease(&pq, i, val_update, &dec), true, bool, "%b");
            CHECK(ppq_validate(&pq), true, bool, "%b");
        }
    }
    CHECK(ppq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum ppq_threeway_cmp
val_cmp(const struct ppq_elem *a, const struct ppq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = ppq_entry(a, struct val, elem);
    struct val *rhs = ppq_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static void
val_update(struct ppq_elem *a, void *aux)
{
    struct val *old = ppq_entry(a, struct val, elem);
    old->val = *(int *)aux;
}
