#include "heap_pqueue.h"
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int id;
    int val;
    struct hpq_elem elem;
};

static enum test_result hpq_test_insert_iterate_pop(void);
static enum test_result hpq_test_priority_update(void);
static enum test_result hpq_test_priority_removal(void);
static void val_update(struct hpq_elem *, void *);
static enum heap_pq_threeway_cmp val_cmp(const struct hpq_elem *,
                                         const struct hpq_elem *, void *);

#define NUM_TESTS (size_t)3
const test_fn all_tests[NUM_TESTS] = {
    hpq_test_insert_iterate_pop,
    hpq_test_priority_update,
    hpq_test_priority_removal,
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
hpq_test_insert_iterate_pop(void)
{
    struct heap_pqueue pq;
    hpq_init(&pq, HPQ_LES, val_cmp, NULL);
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
        hpq_push(&pq, &vals[i].elem);
        CHECK(hpq_validate(&pq), true, bool, "%b");
    }
    size_t pop_count = 0;
    while (!hpq_empty(&pq))
    {
        hpq_pop(&pq);
        ++pop_count;
        CHECK(hpq_validate(&pq), true, bool, "%b");
    }
    CHECK(pop_count, num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
hpq_test_priority_removal(void)
{
    struct heap_pqueue pq;
    hpq_init(&pq, HPQ_LES, val_cmp, NULL);
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
        hpq_push(&pq, &vals[i].elem);
        CHECK(hpq_validate(&pq), true, bool, "%b");
    }
    const int limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct hpq_elem *i = &vals[val].elem;
        struct val *cur = hpq_entry(i, struct val, elem);
        if (cur->val > limit)
        {
            i = hpq_erase(&pq, i);
            CHECK(hpq_validate(&pq), true, bool, "%b");
        }
    }
    return PASS;
}

static enum test_result
hpq_test_priority_update(void)
{
    struct heap_pqueue pq;
    hpq_init(&pq, HPQ_LES, val_cmp, NULL);
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
        hpq_push(&pq, &vals[i].elem);
        CHECK(hpq_validate(&pq), true, bool, "%b");
    }
    const int limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct hpq_elem *i = &vals[val].elem;
        struct val *cur = hpq_entry(i, struct val, elem);
        int backoff = cur->val / 2;
        if (cur->val > limit)
        {
            CHECK(hpq_update(&pq, i, val_update, &backoff), true, bool, "%b");
            CHECK(hpq_validate(&pq), true, bool, "%b");
        }
    }
    CHECK(hpq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum heap_pq_threeway_cmp
val_cmp(const struct hpq_elem *a, const struct hpq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = hpq_entry(a, struct val, elem);
    struct val *rhs = hpq_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static void
val_update(struct hpq_elem *a, void *aux)
{
    struct val *old = hpq_entry(a, struct val, elem);
    old->val = *(int *)aux;
}
