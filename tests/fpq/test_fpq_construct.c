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
    struct val vals[2] = {};
    ccc_flat_priority_queue pq
        = CCC_FPQ_INIT(vals, (sizeof(vals) / sizeof(struct val)), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    CHECK(ccc_fpq_empty(&pq), true);
    return PASS;
}

static enum test_result
pq_test_macro(void)
{
    struct val vals[2] = {};
    ccc_flat_priority_queue pq
        = CCC_FPQ_INIT(&vals, (sizeof(vals) / sizeof(struct val)), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    struct val *res = CCC_FPQ_EMPLACE(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res != NULL, true);
    CHECK(ccc_fpq_empty(&pq), false);
    struct val *res2 = CCC_FPQ_EMPLACE(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res2 == NULL, true);
    return PASS;
}

static enum test_result
pq_test_push(void)
{
    struct val vals[3] = {};
    ccc_flat_priority_queue pq
        = CCC_FPQ_INIT(&vals, (sizeof(vals) / sizeof(struct val)), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    struct val *res = ccc_fpq_push(&pq, &vals[0]);
    CHECK(res != NULL, true);
    CHECK(ccc_fpq_empty(&pq), false);
    return PASS;
}

static enum test_result
pq_test_raw_type(void)
{
    int vals[4] = {};
    ccc_flat_priority_queue pq = CCC_FPQ_INIT(
        &vals, (sizeof(vals) / sizeof(int)), int, CCC_LES, NULL, int_cmp, NULL);
    int val = 1;
    int *res = ccc_fpq_push(&pq, &val);
    CHECK(res != NULL, true);
    CHECK(ccc_fpq_empty(&pq), false);
    res = CCC_FPQ_EMPLACE(&pq, -1);
    CHECK(res != NULL, true);
    CHECK(ccc_fpq_size(&pq), 2);
    int *popped = ccc_fpq_front(&pq);
    CHECK(*popped, -1);
    return PASS;
}

static ccc_threeway_cmp
int_cmp(ccc_cmp const cmp)
{
    int a = *((int *)cmp.container_a);
    int b = *((int *)cmp.container_b);
    return (a > b) - (a < b);
}
