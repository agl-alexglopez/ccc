#include "flat_priority_queue.h"
#include "fpq_util.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static enum test_result fpq_test_insert_remove_four_dups(void);
static enum test_result fpq_test_insert_erase_shuffled(void);
static enum test_result fpq_test_pop_max(void);
static enum test_result fpq_test_pop_min(void);
static enum test_result fpq_test_delete_prime_shuffle_duplicates(void);
static enum test_result fpq_test_prime_shuffle(void);
static enum test_result fpq_test_weak_srand(void);

#define NUM_TESTS (size_t)7
test_fn const all_tests[NUM_TESTS] = {
    fpq_test_insert_remove_four_dups,
    fpq_test_insert_erase_shuffled,
    fpq_test_pop_max,
    fpq_test_pop_min,
    fpq_test_delete_prime_shuffle_duplicates,
    fpq_test_prime_shuffle,
    fpq_test_weak_srand,
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
fpq_test_insert_remove_four_dups(void)
{
    struct val three_vals[4 + 1];
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(three_vals, (sizeof(three_vals) / sizeof(three_vals[0])),
                       struct val, CCC_LES, NULL, val_cmp, NULL);
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        CHECK(ccc_fpq_push(&fpq, &three_vals[i]) != NULL, true);
        CHECK(ccc_fpq_validate(&fpq), true);
        size_t const size_check = i + 1;
        CHECK(ccc_fpq_size(&fpq), size_check);
    }
    CHECK(ccc_fpq_size(&fpq), (size_t)4);
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        ccc_fpq_pop(&fpq);
        CHECK(ccc_fpq_validate(&fpq), true);
    }
    CHECK(ccc_fpq_size(&fpq), (size_t)0);
    return PASS;
}

static enum test_result
fpq_test_insert_erase_shuffled(void)
{
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50 + 1];
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(vals, (sizeof(vals) / sizeof(vals[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    CHECK(insert_shuffled(&fpq, vals, size, prime), PASS);
    struct val const *min = ccc_fpq_front(&fpq);
    CHECK(min->val, 0);
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &fpq), PASS);
    /* Now let's delete everything with no errors. */
    while (!ccc_fpq_empty(&fpq))
    {
        size_t const rand_index = rand_range(0, (int)ccc_fpq_size(&fpq) - 1);
        (void)ccc_fpq_erase(&fpq, &vals[rand_index]);
        CHECK(ccc_fpq_validate(&fpq), true);
    }
    CHECK(ccc_fpq_size(&fpq), (size_t)0);
    return PASS;
}

static enum test_result
fpq_test_pop_max(void)
{
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50 + 1];
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(vals, (sizeof(vals) / sizeof(vals[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    CHECK(insert_shuffled(&fpq, vals, size, prime), PASS);
    struct val const *min = ccc_fpq_front(&fpq);
    CHECK(min->val, 0);
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &fpq), PASS);
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct val const *const front = ccc_fpq_front(&fpq);
        CHECK(front->val, sorted_check[i]);
        ccc_fpq_pop(&fpq);
    }
    CHECK(ccc_fpq_empty(&fpq), true);
    return PASS;
}

static enum test_result
fpq_test_pop_min(void)
{
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50 + 1];
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(vals, (sizeof(vals) / sizeof(vals[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    CHECK(insert_shuffled(&fpq, vals, size, prime), PASS);
    struct val const *min = ccc_fpq_front(&fpq);
    CHECK(min->val, 0);
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &fpq), PASS);
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct val const *const front = ccc_fpq_front(&fpq);
        CHECK(front->val, sorted_check[i]);
        ccc_fpq_pop(&fpq);
    }
    CHECK(ccc_fpq_empty(&fpq), true);
    return PASS;
}

static enum test_result
fpq_test_delete_prime_shuffle_duplicates(void)
{
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const size = 99;
    int const prime = 101;
    /* Make the prime shuffle shorter than size for many duplicates. */
    int const less = 77;
    struct val vals[99 + 1];
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(vals, (sizeof(vals) / sizeof(vals[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    int shuffled_index = prime % (size - less);
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = i;
        CHECK(ccc_fpq_push(&fpq, &vals[i]) != NULL, true);
        CHECK(ccc_fpq_validate(&fpq), true);
        size_t const s = i + 1;
        CHECK(ccc_fpq_size(&fpq), s);
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    size_t cur_size = size;
    while (!ccc_fpq_empty(&fpq))
    {
        size_t const rand_index = rand_range(0, (int)ccc_fpq_size(&fpq) - 1);
        (void)ccc_fpq_erase(&fpq, &vals[rand_index]);
        CHECK(ccc_fpq_validate(&fpq), true);
        --cur_size;
        CHECK(ccc_fpq_size(&fpq), cur_size);
    }
    return PASS;
}

static enum test_result
fpq_test_prime_shuffle(void)
{
    int const size = 50;
    int const prime = 53;
    int const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    int shuffled_index = prime % (size - less);
    struct val vals[50 + 1];
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(vals, (sizeof(vals) / sizeof(vals[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = shuffled_index;
        CHECK(ccc_fpq_push(&fpq, &vals[i]) != NULL, true);
        CHECK(ccc_fpq_validate(&fpq), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    while (!ccc_fpq_empty(&fpq))
    {
        size_t const rand_index = rand_range(0, (int)ccc_fpq_size(&fpq) - 1);
        CHECK(ccc_fpq_erase(&fpq, &vals[rand_index]) != NULL, true);
        CHECK(ccc_fpq_validate(&fpq), true);
        --cur_size;
        CHECK(ccc_fpq_size(&fpq), cur_size);
    }
    return PASS;
}

static enum test_result
fpq_test_weak_srand(void)
{
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_stack_elems = 1000;
    struct val vals[1000 + 1];
    ccc_flat_priority_queue fpq
        = CCC_FPQ_INIT(vals, (sizeof(vals) / sizeof(vals[0])), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    for (int i = 0; i < num_stack_elems; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        CHECK(ccc_fpq_push(&fpq, &vals[i]) != NULL, true);
        CHECK(ccc_fpq_validate(&fpq), true);
    }
    while (!ccc_fpq_empty(&fpq))
    {
        size_t const rand_index = rand_range(0, (int)ccc_fpq_size(&fpq) - 1);
        CHECK(ccc_fpq_erase(&fpq, &vals[rand_index]) != NULL, true);
        CHECK(ccc_fpq_validate(&fpq), true);
    }
    CHECK(ccc_fpq_empty(&fpq), true);
    return PASS;
}
