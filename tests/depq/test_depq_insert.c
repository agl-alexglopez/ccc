#include "depq_util.h"
#include "double_ended_priority_queue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static enum test_result depq_test_insert_one(void);
static enum test_result depq_test_insert_three(void);
static enum test_result depq_test_insert_shuffle(void);
static enum test_result depq_test_struct_getter(void);
static enum test_result depq_test_insert_three_dups(void);
static enum test_result depq_test_read_max_min(void);

#define NUM_TESTS (size_t)6
test_fn const all_tests[NUM_TESTS] = {
    depq_test_insert_one,     depq_test_insert_three,
    depq_test_struct_getter,  depq_test_insert_three_dups,
    depq_test_insert_shuffle, depq_test_read_max_min,
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
depq_test_insert_one(void)
{
    ccc_double_ended_priority_queue pq
        = CCC_DEPQ_INIT(struct val, elem, val, pq, NULL, val_cmp, NULL);
    struct val single;
    single.val = 0;
    ccc_depq_push(&pq, &single.elem);
    CHECK(ccc_depq_empty(&pq), false, "%d");
    CHECK(((struct val *)ccc_depq_root(&pq))->val == single.val, true, "%d");
    return PASS;
}

static enum test_result
depq_test_insert_three(void)
{
    ccc_double_ended_priority_queue pq
        = CCC_DEPQ_INIT(struct val, elem, val, pq, NULL, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        ccc_depq_push(&pq, &three_vals[i].elem);
        CHECK(ccc_depq_validate(&pq), true, "%d");
        CHECK(ccc_depq_size(&pq), (size_t)i + 1, "%zu");
    }
    CHECK(ccc_depq_size(&pq), (size_t)3, "%zu");
    return PASS;
}

static enum test_result
depq_test_struct_getter(void)
{
    ccc_double_ended_priority_queue pq
        = CCC_DEPQ_INIT(struct val, elem, val, pq, NULL, val_cmp, NULL);
    ccc_double_ended_priority_queue pq_tester_clone = CCC_DEPQ_INIT(
        struct val, elem, val, pq_tester_clone, NULL, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        ccc_depq_push(&pq, &vals[i].elem);
        ccc_depq_push(&pq_tester_clone, &tester_clone[i].elem);
        CHECK(ccc_depq_validate(&pq), true, "%d");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val, "%d");
    }
    CHECK(ccc_depq_size(&pq), (size_t)10, "%zu");
    return PASS;
}

static enum test_result
depq_test_insert_three_dups(void)
{
    ccc_double_ended_priority_queue pq
        = CCC_DEPQ_INIT(struct val, elem, val, pq, NULL, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        ccc_depq_push(&pq, &three_vals[i].elem);
        CHECK(ccc_depq_validate(&pq), true, "%d");
        CHECK(ccc_depq_size(&pq), (size_t)i + 1, "%zu");
    }
    CHECK(ccc_depq_size(&pq), (size_t)3, "%zu");
    return PASS;
}

static enum test_result
depq_test_insert_shuffle(void)
{
    ccc_double_ended_priority_queue pq
        = CCC_DEPQ_INIT(struct val, elem, val, pq, NULL, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, "%d");
    struct val const *max = ccc_depq_max(&pq);
    CHECK(max->val, (int)size - 1, "%d");
    struct val const *min = ccc_depq_min(&pq);
    CHECK(min->val, 0, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &pq), size, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], "%d");
    }
    return PASS;
}

static enum test_result
depq_test_read_max_min(void)
{
    ccc_double_ended_priority_queue pq
        = CCC_DEPQ_INIT(struct val, elem, val, pq, NULL, val_cmp, NULL);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        ccc_depq_push(&pq, &vals[i].elem);
        CHECK(ccc_depq_validate(&pq), true, "%d");
        CHECK(ccc_depq_size(&pq), (size_t)i + 1, "%zu");
    }
    CHECK(ccc_depq_size(&pq), (size_t)10, "%zu");
    struct val const *max = ccc_depq_max(&pq);
    CHECK(max->val, 9, "%d");
    struct val const *min = ccc_depq_min(&pq);
    CHECK(min->val, 0, "%d");
    return PASS;
}
