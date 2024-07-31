#include "depqueue.h"
#include "test.h"
#include "tree.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int id;
    int val;
    struct depq_elem elem;
};

static enum test_result depq_test_insert_remove_four_dups(void);
static enum test_result depq_test_insert_erase_shuffled(void);
static enum test_result depq_test_pop_max(void);
static enum test_result depq_test_pop_min(void);
static enum test_result depq_test_max_round_robin(void);
static enum test_result depq_test_min_round_robin(void);
static enum test_result depq_test_delete_prime_shuffle_duplicates(void);
static enum test_result depq_test_prime_shuffle(void);
static enum test_result depq_test_weak_srand(void);
static enum test_result insert_shuffled(struct depqueue *, struct val[], size_t,
                                        int);
static size_t inorder_fill(int[], size_t, struct depqueue *);
static dpq_threeway_cmp val_cmp(struct depq_elem const *,
                                struct depq_elem const *, void *);
static void depq_printer_fn(struct depq_elem const *);

#define NUM_TESTS (size_t)9
test_fn const all_tests[NUM_TESTS] = {
    depq_test_insert_remove_four_dups,
    depq_test_insert_erase_shuffled,
    depq_test_pop_max,
    depq_test_pop_min,
    depq_test_max_round_robin,
    depq_test_min_round_robin,
    depq_test_delete_prime_shuffle_duplicates,
    depq_test_prime_shuffle,
    depq_test_weak_srand,
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
depq_test_insert_remove_four_dups(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
    struct val three_vals[4];
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        depq_push(&pq, &three_vals[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%d");
        size_t const size = i + 1;
        CHECK(depq_size(&pq), size, size_t, "%zu");
    }
    CHECK(depq_size(&pq), 4, size_t, "%zu");
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        depq_pop_max(&pq);
        CHECK(validate_tree(&pq.t), true, bool, "%d");
    }
    CHECK(depq_size(&pq), 0ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
depq_test_insert_erase_shuffled(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *max = DEPQ_ENTRY(depq_const_max(&pq), struct val, elem);
    CHECK(max->val, size - 1, int, "%d");
    struct val const *min = DEPQ_ENTRY(depq_const_min(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &pq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        (void)depq_erase(&pq, &vals[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%d");
    }
    CHECK(depq_size(&pq), 0ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
depq_test_pop_max(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *max = DEPQ_ENTRY(depq_const_max(&pq), struct val, elem);
    CHECK(max->val, size - 1, int, "%d");
    struct val const *min = DEPQ_ENTRY(depq_const_min(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &pq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = size - 1; i != (size_t)-1; --i)
    {
        struct val const *front
            = DEPQ_ENTRY(depq_pop_max(&pq), struct val, elem);
        CHECK(front->val, vals[i].val, int, "%d");
    }
    CHECK(depq_empty(&pq), true, bool, "%d");
    return PASS;
}

static enum test_result
depq_test_pop_min(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *max = DEPQ_ENTRY(depq_const_max(&pq), struct val, elem);
    CHECK(max->val, size - 1, int, "%d");
    struct val const *min = DEPQ_ENTRY(depq_const_min(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &pq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct val const *front
            = DEPQ_ENTRY(depq_pop_min(&pq), struct val, elem);
        CHECK(front->val, vals[i].val, int, "%d");
    }
    CHECK(depq_empty(&pq), true, bool, "%d");
    return PASS;
}

static enum test_result
depq_test_max_round_robin(void)
{
    struct depqueue depq = DEPQ_INIT(depq, val_cmp, NULL);
    int const size = 6;
    struct val vals[size];
    struct val const order[6] = {
        {.id = 0, .val = 99}, {.id = 2, .val = 99}, {.id = 4, .val = 99},
        {.id = 1, .val = 1},  {.id = 3, .val = 1},  {.id = 5, .val = 1},
    };
    for (int i = 0; i < size; ++i)
    {
        if (i % 2)
        {
            vals[i].val = 1;
        }
        else
        {
            vals[i].val = 99;
        }
        vals[i].id = i;
        depq_push(&depq, &vals[i].elem);
        CHECK(validate_tree(&depq.t), true, bool, "%d");
    }
    /* Now let's make sure we pop round robin. */
    size_t i = 0;
    while (!depq_empty(&depq))
    {
        struct val const *front
            = DEPQ_ENTRY(depq_pop_max(&depq), struct val, elem);
        CHECK(front->id, order[i].id, int, "%d");
        CHECK(front->val, order[i].val, int, "%d");
        ++i;
    }
    return PASS;
}

static enum test_result
depq_test_min_round_robin(void)
{
    struct depqueue depq = DEPQ_INIT(depq, val_cmp, NULL);
    int const size = 6;
    struct val vals[size];
    struct val const order[6] = {
        {.id = 0, .val = 1},  {.id = 2, .val = 1},  {.id = 4, .val = 1},
        {.id = 1, .val = 99}, {.id = 3, .val = 99}, {.id = 5, .val = 99},
    };
    for (int i = 0; i < size; ++i)
    {
        if (i % 2)
        {
            vals[i].val = 99;
        }
        else
        {
            vals[i].val = 1;
        }
        vals[i].id = i;
        depq_push(&depq, &vals[i].elem);
        CHECK(validate_tree(&depq.t), true, bool, "%d");
    }
    /* Now let's make sure we pop round robin. */
    size_t i = 0;
    while (!depq_empty(&depq))
    {
        struct val const *front
            = DEPQ_ENTRY(depq_pop_min(&depq), struct val, elem);
        CHECK(front->id, order[i].id, int, "%d");
        CHECK(front->val, order[i].val, int, "%d");
        ++i;
    }
    return PASS;
}

static enum test_result
depq_test_delete_prime_shuffle_duplicates(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
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
        depq_push(&pq, &vals[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%d");
        size_t const s = i + 1;
        CHECK(depq_size(&pq), s, size_t, "%zu");
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }

    shuffled_index = prime % (size - less);
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        (void)depq_erase(&pq, &vals[shuffled_index].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%d");
        --cur_size;
        CHECK(depq_size(&pq), cur_size, size_t, "%zu");
        /* Shuffle normally here so we only remove each elem once. */
        shuffled_index = (shuffled_index + prime) % size;
    }
    return PASS;
}

static enum test_result
depq_test_prime_shuffle(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
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
        depq_push(&pq, &vals[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%d");
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* One test can use our printer function as test output */
    depq_print(&pq, depq_root(&pq), depq_printer_fn);
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        CHECK(depq_erase(&pq, &vals[i].elem) != NULL, true, bool, "%d");
        CHECK(validate_tree(&pq.t), true, bool, "%d");
        --cur_size;
        CHECK(depq_size(&pq), cur_size, size_t, "%zu");
    }
    return PASS;
}

static enum test_result
depq_test_weak_srand(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_nodes = 1000;
    struct val vals[num_nodes];
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        depq_push(&pq, &vals[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%d");
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CHECK(depq_erase(&pq, &vals[i].elem) != NULL, true, bool, "%d");
        CHECK(validate_tree(&pq.t), true, bool, "%d");
    }
    CHECK(depq_empty(&pq), true, bool, "%d");
    return PASS;
}

static enum test_result
insert_shuffled(struct depqueue *pq, struct val vals[], size_t const size,
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
        depq_push(pq, &vals[shuffled_index].elem);
        CHECK(depq_size(pq), i + 1, size_t, "%zu");
        CHECK(validate_tree(&pq->t), true, bool, "%d");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(depq_size(pq), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, struct depqueue *pq)
{
    if (depq_size(pq) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct depq_elem *e = depq_rbegin(pq); e != depq_end(pq);
         e = depq_rnext(pq, e))
    {
        vals[i++] = DEPQ_ENTRY(e, struct val, elem)->val;
    }
    return i;
}

static void
depq_printer_fn(struct depq_elem const *const e)
{
    struct val const *const v = DEPQ_ENTRY(e, struct val, elem);
    printf("{id:%d,val:%d}", v->id, v->val);
}

static dpq_threeway_cmp
val_cmp(struct depq_elem const *a, struct depq_elem const *b, void *aux)
{
    (void)aux;
    struct val *lhs = DEPQ_ENTRY(a, struct val, elem);
    struct val *rhs = DEPQ_ENTRY(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
