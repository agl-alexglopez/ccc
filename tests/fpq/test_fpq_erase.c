#include "buf.h"
#include "flat_pqueue.h"
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

static enum test_result fpq_test_insert_remove_four_dups(void);
static enum test_result fpq_test_insert_erase_shuffled(void);
static enum test_result fpq_test_pop_max(void);
static enum test_result fpq_test_pop_min(void);
static enum test_result fpq_test_delete_prime_shuffle_duplicates(void);
static enum test_result fpq_test_prime_shuffle(void);
static enum test_result fpq_test_weak_srand(void);
static enum test_result insert_shuffled(ccc_flat_pqueue *, struct val[], size_t,
                                        int);
static enum test_result inorder_fill(int[], size_t, ccc_flat_pqueue *);
static ccc_fpq_threeway_cmp val_cmp(ccc_fpq_elem const *, ccc_fpq_elem const *,
                                    void *);
static size_t rand_range(size_t, size_t);

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
    size_t const size = 4;
    struct val three_vals[size];
    ccc_buf buf = CCC_BUF_INIT(three_vals, struct val, size, NULL);
    ccc_flat_pqueue fpq
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_FPQ_LES, val_cmp, NULL);
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        CHECK(ccc_fpq_push(&fpq, &three_vals[i]), CCC_FPQ_OK, ccc_fpq_result,
              "%d");
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
        size_t const size_check = i + 1;
        CHECK(ccc_fpq_size(&fpq), size_check, size_t, "%zu");
    }
    CHECK(ccc_fpq_size(&fpq), 4, size_t, "%zu");
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        ccc_fpq_pop(&fpq);
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
    }
    CHECK(ccc_fpq_size(&fpq), 0ULL, size_t, "%zu");
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
    struct val vals[size];
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, size, NULL);
    ccc_flat_pqueue fpq
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_FPQ_LES, val_cmp, NULL);
    CHECK(insert_shuffled(&fpq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *min = CCC_FPQ_OF(struct val, elem, ccc_fpq_front(&fpq));
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &fpq), PASS, enum test_result, "%d");
    /* Now let's delete everything with no errors. */
    while (!ccc_fpq_empty(&fpq))
    {
        size_t const rand_index = rand_range(0, (int)ccc_fpq_size(&fpq) - 1);
        (void)ccc_fpq_erase(&fpq, &vals[rand_index].elem);
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
    }
    CHECK(ccc_fpq_size(&fpq), 0ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
fpq_test_pop_max(void)
{
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, size, NULL);
    ccc_flat_pqueue fpq
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_FPQ_LES, val_cmp, NULL);
    CHECK(insert_shuffled(&fpq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *min = CCC_FPQ_OF(struct val, elem, ccc_fpq_front(&fpq));
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &fpq), PASS, enum test_result, "%d");
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        ccc_fpq_elem const *const e = ccc_fpq_pop(&fpq);
        CHECK(e, true, bool, "%b");
        struct val const *const front = CCC_FPQ_OF(struct val, elem, e);
        CHECK(front->val, sorted_check[i], int, "%d");
    }
    CHECK(ccc_fpq_empty(&fpq), true, bool, "%d");
    return PASS;
}

static enum test_result
fpq_test_pop_min(void)
{
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, size, NULL);
    ccc_flat_pqueue fpq
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_FPQ_LES, val_cmp, NULL);
    CHECK(insert_shuffled(&fpq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *min = CCC_FPQ_OF(struct val, elem, ccc_fpq_front(&fpq));
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &fpq), PASS, enum test_result, "%d");
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        ccc_fpq_elem const *const e = ccc_fpq_pop(&fpq);
        CHECK(e, true, bool, "%b");
        struct val const *const front = CCC_FPQ_OF(struct val, elem, e);
        CHECK(front->val, sorted_check[i], int, "%d");
    }
    CHECK(ccc_fpq_empty(&fpq), true, bool, "%d");
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
    struct val vals[size];
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, size, NULL);
    ccc_flat_pqueue fpq
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_FPQ_LES, val_cmp, NULL);
    int shuffled_index = prime % (size - less);
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = i;
        CHECK(ccc_fpq_push(&fpq, &vals[i]), CCC_FPQ_OK, ccc_fpq_result, "%d");
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
        size_t const s = i + 1;
        CHECK(ccc_fpq_size(&fpq), s, size_t, "%zu");
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    size_t cur_size = size;
    while (!ccc_fpq_empty(&fpq))
    {
        size_t const rand_index = rand_range(0, (int)ccc_fpq_size(&fpq) - 1);
        (void)ccc_fpq_erase(&fpq, &vals[rand_index].elem);
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
        --cur_size;
        CHECK(ccc_fpq_size(&fpq), cur_size, size_t, "%zu");
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
    struct val vals[size];
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, size, NULL);
    ccc_flat_pqueue fpq
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_FPQ_LES, val_cmp, NULL);
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = shuffled_index;
        CHECK(ccc_fpq_push(&fpq, &vals[i]), CCC_FPQ_OK, ccc_fpq_result, "%d");
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    while (!ccc_fpq_empty(&fpq))
    {
        size_t const rand_index = rand_range(0, (int)ccc_fpq_size(&fpq) - 1);
        CHECK(ccc_fpq_erase(&fpq, &vals[rand_index].elem) != NULL, true, bool,
              "%d");
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
        --cur_size;
        CHECK(ccc_fpq_size(&fpq), cur_size, size_t, "%zu");
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
    struct val vals[num_stack_elems];
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, num_stack_elems, NULL);
    ccc_flat_pqueue fpq
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_FPQ_LES, val_cmp, NULL);
    for (int i = 0; i < num_stack_elems; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        CHECK(ccc_fpq_push(&fpq, &vals[i]), CCC_FPQ_OK, ccc_fpq_result, "%d");
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
    }
    while (!ccc_fpq_empty(&fpq))
    {
        size_t const rand_index = rand_range(0, (int)ccc_fpq_size(&fpq) - 1);
        CHECK(ccc_fpq_erase(&fpq, &vals[rand_index].elem) != NULL, true, bool,
              "%d");
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
    }
    CHECK(ccc_fpq_empty(&fpq), true, bool, "%d");
    return PASS;
}

static enum test_result
insert_shuffled(ccc_flat_pqueue *fpq, struct val vals[], size_t const size,
                int const larger_prime)
{
    /* Math magic ahead so that we iterate over every index
       eventually but in a shuffled order. Not necessarily
       randome but a repeatable sequence that makes it
       easier to debug if something goes wrong. Think
       of the prime number as a random seed, kind of. */
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        vals[i].val = (int)shuffled_index;
        CHECK(ccc_fpq_push(fpq, &vals[i]), CCC_FPQ_OK, ccc_fpq_result, "%d");
        CHECK(ccc_fpq_size(fpq), i + 1, size_t, "%zu");
        CHECK(ccc_fpq_validate(fpq), true, bool, "%d");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_fpq_size(fpq), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static enum test_result
inorder_fill(int vals[], size_t size, ccc_flat_pqueue *fpq)
{
    if (ccc_fpq_size(fpq) != size)
    {
        return FAIL;
    }
    size_t i = 0;
    struct val copy_buf[sizeof(struct val) * ccc_fpq_size(fpq)];
    ccc_buf buf = CCC_BUF_INIT(copy_buf, struct val, ccc_fpq_size(fpq), NULL);
    ccc_flat_pqueue fpq_copy
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_FPQ_LES, val_cmp, NULL);
    while (!ccc_fpq_empty(fpq) && i < size)
    {
        ccc_fpq_elem *const front = ccc_fpq_pop(fpq);
        vals[i++] = CCC_FPQ_OF(struct val, elem, front)->val;
        struct val *const v = CCC_FPQ_OF(struct val, elem, front);
        size_t const prev = ccc_fpq_size(&fpq_copy);
        ccc_fpq_result const res = CCC_FPQ_EMPLACE(
            &fpq_copy, struct val, {.id = v->id, .val = v->val});
        CHECK(res, CCC_FPQ_OK, ccc_fpq_result, "%d");
        CHECK(prev < ccc_fpq_size(&fpq_copy), true, bool, "%d");
    }
    i = 0;
    while (!ccc_fpq_empty(&fpq_copy) && i < size)
    {
        struct val *const v
            = CCC_FPQ_OF(struct val, elem, ccc_fpq_pop(&fpq_copy));
        size_t const prev = ccc_fpq_size(fpq);
        ccc_fpq_result const res
            = CCC_FPQ_EMPLACE(fpq, struct val, {.id = v->id, .val = v->val});
        CHECK(res, CCC_FPQ_OK, ccc_fpq_result, "%d");
        CHECK(prev < ccc_fpq_size(fpq), true, bool, "%d");
        CHECK(vals[i++], v->val, int, "%d");
    }
    return PASS;
}

static ccc_fpq_threeway_cmp
val_cmp(ccc_fpq_elem const *a, ccc_fpq_elem const *b, void *aux)
{
    (void)aux;
    struct val *lhs = CCC_FPQ_OF(struct val, elem, a);
    struct val *rhs = CCC_FPQ_OF(struct val, elem, b);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static size_t
rand_range(size_t const min, size_t const max)
{
    /* NOLINTNEXTLINE(cert-msc30-c, cert-msc50-cpp) */
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}
