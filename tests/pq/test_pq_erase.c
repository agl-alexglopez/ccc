#include "pq_util.h"
#include "priority_queue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static enum test_result pq_test_insert_remove_four_dups(void);
static enum test_result pq_test_insert_erase_shuffled(void);
static enum test_result pq_test_pop_max(void);
static enum test_result pq_test_pop_min(void);
static enum test_result pq_test_delete_prime_shuffle_duplicates(void);
static enum test_result pq_test_prime_shuffle(void);
static enum test_result pq_test_weak_srand(void);

#define NUM_TESTS (size_t)7
test_fn const all_tests[NUM_TESTS] = {
    pq_test_insert_remove_four_dups,
    pq_test_insert_erase_shuffled,
    pq_test_pop_max,
    pq_test_pop_min,
    pq_test_delete_prime_shuffle_duplicates,
    pq_test_prime_shuffle,
    pq_test_weak_srand,
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
pq_test_insert_remove_four_dups(void)
{
    ccc_priority_queue ppq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    struct val three_vals[4];
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        ccc_pq_push(&ppq, &three_vals[i].elem);
        CHECK(ccc_pq_validate(&ppq), true);
        size_t const size = i + 1;
        CHECK(ccc_pq_size(&ppq), size);
    }
    CHECK(ccc_pq_size(&ppq), (size_t)4);
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        ccc_pq_pop(&ppq);
        CHECK(ccc_pq_validate(&ppq), true);
    }
    CHECK(ccc_pq_size(&ppq), (size_t)0);
    return PASS;
}

static enum test_result
pq_test_insert_erase_shuffled(void)
{
    ccc_priority_queue ppq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&ppq, vals, size, prime), PASS);
    struct val const *min = ccc_pq_front(&ppq);
    CHECK(min->val, 0);
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &ppq), PASS);
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        (void)ccc_pq_erase(&ppq, &vals[i].elem);
        CHECK(ccc_pq_validate(&ppq), true);
    }
    CHECK(ccc_pq_size(&ppq), (size_t)0);
    return PASS;
}

static enum test_result
pq_test_pop_max(void)
{
    ccc_priority_queue ppq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&ppq, vals, size, prime), PASS);
    struct val const *min = ccc_pq_front(&ppq);
    CHECK(min->val, 0);
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &ppq), PASS);
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct val const *front = ccc_pq_front(&ppq);
        CHECK(front->val, vals[i].val);
        ccc_pq_pop(&ppq);
    }
    CHECK(ccc_pq_empty(&ppq), true);
    return PASS;
}

static enum test_result
pq_test_pop_min(void)
{
    ccc_priority_queue ppq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&ppq, vals, size, prime), PASS);
    struct val const *min = ccc_pq_front(&ppq);
    CHECK(min->val, 0);
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &ppq), PASS);
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct val const *front = ccc_pq_front(&ppq);
        CHECK(front->val, vals[i].val);
        ccc_pq_pop(&ppq);
    }
    CHECK(ccc_pq_empty(&ppq), true);
    return PASS;
}

static enum test_result
pq_test_delete_prime_shuffle_duplicates(void)
{
    ccc_priority_queue ppq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    int const size = 99;
    int const prime = 101;
    /* Make the prime shuffle shorter than size for many duplicates. */
    int const less = 77;
    struct val vals[99];
    int shuffled_index = prime % (size - less);
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = i;
        ccc_pq_push(&ppq, &vals[i].elem);
        CHECK(ccc_pq_validate(&ppq), true);
        size_t const s = i + 1;
        CHECK(ccc_pq_size(&ppq), s);
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }

    shuffled_index = prime % (size - less);
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        (void)ccc_pq_erase(&ppq, &vals[shuffled_index].elem);
        CHECK(ccc_pq_validate(&ppq), true);
        --cur_size;
        CHECK(ccc_pq_size(&ppq), cur_size);
        /* Shuffle normally here so we only remove each elem once. */
        shuffled_index = (shuffled_index + prime) % size;
    }
    return PASS;
}

static enum test_result
pq_test_prime_shuffle(void)
{
    ccc_priority_queue ppq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    int const size = 50;
    int const prime = 53;
    int const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    int shuffled_index = prime % (size - less);
    struct val vals[50];
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = shuffled_index;
        ccc_pq_push(&ppq, &vals[i].elem);
        CHECK(ccc_pq_validate(&ppq), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        ccc_pq_erase(&ppq, &vals[i].elem);
        CHECK(ccc_pq_validate(&ppq), true);
        --cur_size;
        CHECK(ccc_pq_size(&ppq), cur_size);
    }
    return PASS;
}

static enum test_result
pq_test_weak_srand(void)
{
    ccc_priority_queue ppq
        = CCC_PQ_INIT(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_heap_elems = 1000;
    struct val vals[1000];
    for (int i = 0; i < num_heap_elems; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        ccc_pq_push(&ppq, &vals[i].elem);
        CHECK(ccc_pq_validate(&ppq), true);
    }
    for (int i = 0; i < num_heap_elems; ++i)
    {
        ccc_pq_erase(&ppq, &vals[i].elem);
        CHECK(ccc_pq_validate(&ppq), true);
    }
    CHECK(ccc_pq_empty(&ppq), true);
    return PASS;
}
