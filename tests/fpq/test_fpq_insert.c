#include "flat_priority_queue.h"
#include "fpq_util.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static enum test_result fpq_test_insert_one(void);
static enum test_result fpq_test_insert_three(void);
static enum test_result fpq_test_insert_shuffle(void);
static enum test_result fpq_test_struct_getter(void);
static enum test_result fpq_test_insert_three_dups(void);
static enum test_result fpq_test_read_max_min(void);

#define NUM_TESTS (size_t)6
test_fn const all_tests[NUM_TESTS] = {
    fpq_test_insert_one,        fpq_test_insert_three,   fpq_test_struct_getter,
    fpq_test_insert_three_dups, fpq_test_insert_shuffle, fpq_test_read_max_min,
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
fpq_test_insert_one(void)
{
    struct val single[2] = {};
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(single, (sizeof(single) / sizeof(single[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    single[0].val = 0;
    ccc_fpq_push(&fpq, &single[0]);
    CHECK(ccc_fpq_empty(&fpq), false);
    return PASS;
}

static enum test_result
fpq_test_insert_three(void)
{
    size_t size = 3;
    struct val three_vals[4] = {};
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(three_vals, (sizeof(three_vals) / sizeof(three_vals[0])),
                       struct val, CCC_LES, NULL, val_cmp, NULL);
    for (size_t i = 0; i < size; ++i)
    {
        three_vals[i].val = (int)i;
        ccc_fpq_push(&fpq, &three_vals[i]);
        CHECK(ccc_fpq_validate(&fpq), true);
        CHECK(ccc_fpq_size(&fpq), i + 1);
    }
    CHECK(ccc_fpq_size(&fpq), size);
    return PASS;
}

static enum test_result
fpq_test_struct_getter(void)
{
    size_t const size = 10;
    struct val vals[10 + 1];
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(vals, (sizeof(vals) / sizeof(vals[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    struct val tester_clone[10 + 1];
    ccc_flat_priority_queue fpq_clone
        = CCC_FPQ_INIT(tester_clone, (sizeof(vals) / sizeof(vals[0])),
                       struct val, CCC_LES, NULL, val_cmp, NULL);
    for (size_t i = 0; i < size; ++i)
    {
        struct val const *res1
            = CCC_FPQ_EMPLACE(&fpq, (struct val){.id = (int)i, .val = (int)i});
        CHECK(res1 != NULL, true);
        struct val const *res2 = CCC_FPQ_EMPLACE(
            &fpq_clone, (struct val){.id = (int)i, .val = (int)i});
        CHECK(res2 != NULL, true);
        CHECK(ccc_fpq_validate(&fpq), true);
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val);
    }
    CHECK(ccc_fpq_size(&fpq), (size_t)10);
    return PASS;
}

static enum test_result
fpq_test_insert_three_dups(void)
{
    struct val three_vals[3 + 1];
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(three_vals, (sizeof(three_vals) / sizeof(three_vals[0])),
                       struct val, CCC_LES, NULL, val_cmp, NULL);
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        ccc_fpq_push(&fpq, &three_vals[i]);
        CHECK(ccc_fpq_validate(&fpq), true);
        CHECK(ccc_fpq_size(&fpq), (size_t)i + 1);
    }
    CHECK(ccc_fpq_size(&fpq), (size_t)3);
    return PASS;
}

static enum test_result
fpq_test_insert_shuffle(void)
{
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50 + 1];
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(vals, (sizeof(vals) / sizeof(vals[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    CHECK(insert_shuffled(&fpq, vals, size, prime), PASS);

    /* Test the printing function at least once. */
    ccc_fpq_print(&fpq, 0, val_print);

    struct val const *min = ccc_fpq_front(&fpq);
    CHECK(min->val, 0);
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &fpq), PASS);
    int prev = sorted_check[0];
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(prev <= sorted_check[i], true);
        prev = sorted_check[i];
    }
    return PASS;
}

static enum test_result
fpq_test_read_max_min(void)
{
    size_t const size = 10;
    struct val vals[10 + 1];
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(vals, (sizeof(vals) / sizeof(vals[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    for (size_t i = 0; i < size; ++i)
    {
        vals[i].val = (int)i;
        (void)ccc_fpq_push(&fpq, &vals[i]);
        CHECK(ccc_fpq_validate(&fpq), true);
        CHECK(ccc_fpq_size(&fpq), i + 1);
    }
    CHECK(ccc_fpq_size(&fpq), (size_t)10);
    struct val const *min = ccc_fpq_front(&fpq);
    CHECK(min->val, 0);
    return PASS;
}
