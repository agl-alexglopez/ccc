#include "pqueue.h"
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
    ccc_pq_elem elem;
};

static enum test_result pq_test_insert_remove_four_dups(void);
static enum test_result pq_test_insert_erase_shuffled(void);
static enum test_result pq_test_pop_max(void);
static enum test_result pq_test_pop_min(void);
static enum test_result pq_test_delete_prime_shuffle_duplicates(void);
static enum test_result pq_test_prime_shuffle(void);
static enum test_result pq_test_weak_srand(void);
static enum test_result insert_shuffled(ccc_pqueue *, struct val[], size_t,
                                        int);
static enum test_result inorder_fill(int[], size_t, ccc_pqueue *);
static ccc_threeway_cmp val_cmp(void const *, void const *, void *);

#define NUM_TESTS (size_t)9
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
    ccc_pqueue ppq = CCC_PQ_INIT(struct val, elem, CCC_LES, val_cmp, NULL);
    struct val three_vals[4];
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        ccc_pq_push(&ppq, &three_vals[i].elem);
        CHECK(ccc_pq_validate(&ppq), true, bool, "%d");
        size_t const size = i + 1;
        CHECK(ccc_pq_size(&ppq), size, size_t, "%zu");
    }
    CHECK(ccc_pq_size(&ppq), 4, size_t, "%zu");
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        ccc_pq_pop(&ppq);
        CHECK(ccc_pq_validate(&ppq), true, bool, "%d");
    }
    CHECK(ccc_pq_size(&ppq), 0ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_insert_erase_shuffled(void)
{
    ccc_pqueue ppq = CCC_PQ_INIT(struct val, elem, CCC_LES, val_cmp, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&ppq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *min = ccc_pq_front(&ppq);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &ppq), PASS, enum test_result, "%d");
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        (void)ccc_pq_erase(&ppq, &vals[i].elem);
        CHECK(ccc_pq_validate(&ppq), true, bool, "%d");
    }
    CHECK(ccc_pq_size(&ppq), 0ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_pop_max(void)
{
    ccc_pqueue ppq = CCC_PQ_INIT(struct val, elem, CCC_LES, val_cmp, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&ppq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *min = ccc_pq_front(&ppq);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &ppq), PASS, enum test_result, "%d");
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct val const *front = ccc_pq_pop(&ppq);
        CHECK(front->val, vals[i].val, int, "%d");
    }
    CHECK(ccc_pq_empty(&ppq), true, bool, "%d");
    return PASS;
}

static enum test_result
pq_test_pop_min(void)
{
    ccc_pqueue ppq = CCC_PQ_INIT(struct val, elem, CCC_LES, val_cmp, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&ppq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *min = ccc_pq_front(&ppq);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &ppq), PASS, enum test_result, "%d");
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct val const *front = ccc_pq_pop(&ppq);
        CHECK(front->val, vals[i].val, int, "%d");
    }
    CHECK(ccc_pq_empty(&ppq), true, bool, "%d");
    return PASS;
}

static enum test_result
pq_test_delete_prime_shuffle_duplicates(void)
{
    ccc_pqueue ppq = CCC_PQ_INIT(struct val, elem, CCC_LES, val_cmp, NULL);
    int const size = 99;
    int const prime = 101;
    /* Make the prime shuffle shorter than size for many duplicates. */
    int const less = 77;
    struct val vals[size];
    int shuffled_index = prime % (size - less);
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = i;
        ccc_pq_push(&ppq, &vals[i].elem);
        CHECK(ccc_pq_validate(&ppq), true, bool, "%d");
        size_t const s = i + 1;
        CHECK(ccc_pq_size(&ppq), s, size_t, "%zu");
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }

    shuffled_index = prime % (size - less);
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        (void)ccc_pq_erase(&ppq, &vals[shuffled_index].elem);
        CHECK(ccc_pq_validate(&ppq), true, bool, "%d");
        --cur_size;
        CHECK(ccc_pq_size(&ppq), cur_size, size_t, "%zu");
        /* Shuffle normally here so we only remove each elem once. */
        shuffled_index = (shuffled_index + prime) % size;
    }
    return PASS;
}

static enum test_result
pq_test_prime_shuffle(void)
{
    ccc_pqueue ppq = CCC_PQ_INIT(struct val, elem, CCC_LES, val_cmp, NULL);
    int const size = 50;
    int const prime = 53;
    int const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    int shuffled_index = prime % (size - less);
    struct val vals[size];
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = shuffled_index;
        ccc_pq_push(&ppq, &vals[i].elem);
        CHECK(ccc_pq_validate(&ppq), true, bool, "%d");
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        CHECK(ccc_pq_erase(&ppq, &vals[i].elem) != NULL, true, bool, "%d");
        CHECK(ccc_pq_validate(&ppq), true, bool, "%d");
        --cur_size;
        CHECK(ccc_pq_size(&ppq), cur_size, size_t, "%zu");
    }
    return PASS;
}

static enum test_result
pq_test_weak_srand(void)
{
    ccc_pqueue ppq = CCC_PQ_INIT(struct val, elem, CCC_LES, val_cmp, NULL);
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_heap_elems = 1000;
    struct val vals[num_heap_elems];
    for (int i = 0; i < num_heap_elems; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        ccc_pq_push(&ppq, &vals[i].elem);
        CHECK(ccc_pq_validate(&ppq), true, bool, "%d");
    }
    for (int i = 0; i < num_heap_elems; ++i)
    {
        CHECK(ccc_pq_erase(&ppq, &vals[i].elem) != NULL, true, bool, "%d");
        CHECK(ccc_pq_validate(&ppq), true, bool, "%d");
    }
    CHECK(ccc_pq_empty(&ppq), true, bool, "%d");
    return PASS;
}

static enum test_result
insert_shuffled(ccc_pqueue *ppq, struct val vals[], size_t const size,
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
        vals[shuffled_index].val = (int)shuffled_index;
        ccc_pq_push(ppq, &vals[shuffled_index].elem);
        CHECK(ccc_pq_size(ppq), i + 1, size_t, "%zu");
        CHECK(ccc_pq_validate(ppq), true, bool, "%d");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_pq_size(ppq), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static enum test_result
inorder_fill(int vals[], size_t size, ccc_pqueue *ppq)
{
    if (ccc_pq_size(ppq) != size)
    {
        return FAIL;
    }
    size_t i = 0;
    ccc_pqueue copy
        = CCC_PQ_INIT(struct val, elem, ccc_pq_order(ppq), val_cmp, NULL);
    while (!ccc_pq_empty(ppq))
    {
        struct val *const front = ccc_pq_pop(ppq);
        CHECK(ccc_pq_validate(ppq), true, bool, "%d");
        CHECK(ccc_pq_validate(&copy), true, bool, "%d");
        vals[i++] = front->val;
        ccc_pq_push(&copy, &front->elem);
    }
    i = 0;
    while (!ccc_pq_empty(&copy))
    {
        struct val *v = ccc_pq_pop(&copy);
        CHECK(v->val, vals[i++], int, "%d");
        ccc_pq_push(ppq, &v->elem);
        CHECK(ccc_pq_validate(ppq), true, bool, "%d");
        CHECK(ccc_pq_validate(&copy), true, bool, "%d");
    }
    return PASS;
}

static ccc_threeway_cmp
val_cmp(void const *const a, void const *const b, void *aux)
{
    (void)aux;
    struct val const *const lhs = a;
    struct val const *const rhs = b;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
