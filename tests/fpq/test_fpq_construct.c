#include "flat_priority_queue.h"
#include "fpq_util.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>

static enum test_result pq_test_empty(void);
static enum test_result pq_test_macro(void);
static enum test_result pq_test_push(void);
static enum test_result pq_test_raw_type(void);

#define NUM_TESTS (size_t)4
test_fn const all_tests[NUM_TESTS] = {
    pq_test_empty,
    pq_test_macro,
    pq_test_push,
    pq_test_raw_type,
};

static ccc_threeway_cmp int_cmp(ccc_cmp);

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
pq_test_empty(void)
{
    struct val vals[1] = {{0}};
    ccc_flat_priority_queue pq
        = CCC_FPQ_INIT(vals, 1, struct val, CCC_LES, NULL, val_cmp, NULL);
    CHECK(ccc_fpq_empty(&pq), true, "%d");
    return PASS;
}

static enum test_result
pq_test_macro(void)
{
    struct val vals[1] = {{0}};
    ccc_flat_priority_queue pq
        = CCC_FPQ_INIT(&vals, 1, struct val, CCC_LES, NULL, val_cmp, NULL);
    struct val *res = FPQ_EMPLACE(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res != NULL, true, "%d");
    CHECK(ccc_fpq_empty(&pq), false, "%d");
    struct val *res2 = FPQ_EMPLACE(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res2 == NULL, true, "%d");
    return PASS;
}

static enum test_result
pq_test_push(void)
{
    struct val vals[1] = {{0}};
    ccc_flat_priority_queue pq
        = CCC_FPQ_INIT(&vals, 1, struct val, CCC_LES, NULL, val_cmp, NULL);
    struct val *res = ccc_fpq_push(&pq, &vals[0]);
    CHECK(res != NULL, true, "%d");
    CHECK(ccc_fpq_empty(&pq), false, "%d");
    return PASS;
}

static enum test_result
pq_test_raw_type(void)
{
    int vals[3] = {0};
    ccc_flat_priority_queue pq
        = CCC_FPQ_INIT(&vals, 3, int, CCC_LES, NULL, int_cmp, NULL);
    int val = 1;
    int *res = ccc_fpq_push(&pq, &val);
    CHECK(res != NULL, true, "%d");
    CHECK(ccc_fpq_empty(&pq), false, "%d");
    res = FPQ_EMPLACE(&pq, -1);
    CHECK(res != NULL, true, "%d");
    CHECK(ccc_fpq_size(&pq), 2, "%zu");
    int *popped = ccc_fpq_front(&pq);
    CHECK(*popped, -1, "%d");
    return PASS;
}

static ccc_threeway_cmp
int_cmp(ccc_cmp const cmp)
{
    int a = *((int *)cmp.container_a);
    int b = *((int *)cmp.container_b);
    return (a > b) - (a < b);
}
