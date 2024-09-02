#include "pq_util.h"
#include "priority_queue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static enum test_result pq_test_insert_one(void);
static enum test_result pq_test_insert_three(void);
static enum test_result pq_test_insert_shuffle(void);
static enum test_result pq_test_struct_getter(void);
static enum test_result pq_test_insert_three_dups(void);
static enum test_result pq_test_read_max_min(void);

#define NUM_TESTS (size_t)6
test_fn const all_tests[NUM_TESTS] = {
    pq_test_insert_one,        pq_test_insert_three,   pq_test_struct_getter,
    pq_test_insert_three_dups, pq_test_insert_shuffle, pq_test_read_max_min,
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
pq_test_insert_one(void)
{
    ccc_priority_queue pq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    struct val single;
    single.val = 0;
    ccc_pq_push(&pq, &single.elem);
    CHECK(ccc_pq_empty(&pq), false, "%d");
    return PASS;
}

static enum test_result
pq_test_insert_three(void)
{
    ccc_priority_queue pq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        ccc_pq_push(&pq, &three_vals[i].elem);
        CHECK(ccc_pq_validate(&pq), true, "%d");
        CHECK(ccc_pq_size(&pq), (size_t)i + 1, "%zu");
    }
    CHECK(ccc_pq_size(&pq), (size_t)3, "%zu");
    return PASS;
}

static enum test_result
pq_test_struct_getter(void)
{
    ccc_priority_queue pq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    ccc_priority_queue pq_tester_clone
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        ccc_pq_push(&pq, &vals[i].elem);
        ccc_pq_push(&pq_tester_clone, &tester_clone[i].elem);
        CHECK(ccc_pq_validate(&pq), true, "%d");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val, "%d");
    }
    CHECK(ccc_pq_size(&pq), (size_t)10, "%zu");
    return PASS;
}

static enum test_result
pq_test_insert_three_dups(void)
{
    ccc_priority_queue pq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        ccc_pq_push(&pq, &three_vals[i].elem);
        CHECK(ccc_pq_validate(&pq), true, "%d");
        CHECK(ccc_pq_size(&pq), (size_t)i + 1, "%zu");
    }
    CHECK(ccc_pq_size(&pq), (size_t)3, "%zu");
    return PASS;
}

static enum test_result
pq_test_insert_shuffle(void)
{
    ccc_priority_queue pq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, "%d");
    struct val const *min = ccc_pq_front(&pq);
    CHECK(min->val, 0, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &pq), PASS, "%d");
    return PASS;
}

static enum test_result
pq_test_read_max_min(void)
{
    ccc_priority_queue pq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        ccc_pq_push(&pq, &vals[i].elem);
        CHECK(ccc_pq_validate(&pq), true, "%d");
        CHECK(ccc_pq_size(&pq), (size_t)i + 1, "%zu");
    }
    CHECK(ccc_pq_size(&pq), (size_t)10, "%zu");
    struct val const *min = ccc_pq_front(&pq);
    CHECK(min->val, 0, "%d");
    return PASS;
}
