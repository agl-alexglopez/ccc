#include "heap_pqueue.h"
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int id;
    int val;
    struct hpq_elem elem;
};

static enum test_result hpq_test_insert_remove_four_dups(void);
static enum test_result hpq_test_insert_erase_shuffled(void);
static enum test_result hpq_test_pop_max(void);
static enum test_result hpq_test_pop_min(void);
static enum test_result hpq_test_max_round_robin(void);
static enum test_result hpq_test_min_round_robin(void);
static enum test_result hpq_test_delete_prime_shuffle_duplicates(void);
static enum test_result hpq_test_prime_shuffle(void);
static enum test_result hpq_test_weak_srand(void);
static enum test_result insert_shuffled(struct heap_pqueue *, struct val[],
                                        size_t, int);
static size_t inorder_fill(int[], size_t, struct heap_pqueue *);
static enum heap_pq_threeway_cmp val_cmp(const struct hpq_elem *,
                                         const struct hpq_elem *, void *);

#define NUM_TESTS (size_t)9
const test_fn all_tests[NUM_TESTS] = {
    hpq_test_insert_remove_four_dups,
    hpq_test_insert_erase_shuffled,
    hpq_test_pop_max,
    hpq_test_pop_min,
    hpq_test_max_round_robin,
    hpq_test_min_round_robin,
    hpq_test_delete_prime_shuffle_duplicates,
    hpq_test_prime_shuffle,
    hpq_test_weak_srand,
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        const bool fail = all_tests[i]() == FAIL;
        if (fail)
        {
            res = FAIL;
        }
    }
    return res;
}

static enum test_result
hpq_test_insert_remove_four_dups(void)
{
    struct heap_pqueue hpq;
    hpq_init(&hpq, HPQLES, val_cmp, NULL);
    struct val three_vals[4];
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        hpq_push(&hpq, &three_vals[i].elem);
        CHECK(hpq_validate(&hpq), true, bool, "%b");
        const size_t size = i + 1;
        CHECK(hpq_size(&hpq), size, size_t, "%zu");
    }
    CHECK(hpq_size(&hpq), 4, size_t, "%zu");
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        hpq_pop(&hpq);
        CHECK(hpq_validate(&hpq), true, bool, "%b");
    }
    CHECK(hpq_size(&hpq), 0ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
hpq_test_insert_erase_shuffled(void)
{
    struct heap_pqueue hpq;
    hpq_init(&hpq, HPQLES, val_cmp, NULL);
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&hpq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *min = hpq_entry(hpq_pop(&hpq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &hpq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        (void)hpq_erase(&hpq, &vals[i].elem);
        CHECK(hpq_validate(&hpq), true, bool, "%b");
    }
    CHECK(hpq_size(&hpq), 0ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
hpq_test_pop_max(void)
{
    struct heap_pqueue hpq;
    hpq_init(&hpq, HPQLES, val_cmp, NULL);
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&hpq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *min = hpq_entry(hpq_pop(&hpq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &hpq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = size - 1; i != (size_t)-1; --i)
    {
        const struct val *front = hpq_entry(hpq_pop(&hpq), struct val, elem);
        CHECK(front->val, vals[i].val, int, "%d");
    }
    CHECK(hpq_empty(&hpq), true, bool, "%b");
    return PASS;
}

static enum test_result
hpq_test_pop_min(void)
{
    struct heap_pqueue hpq;
    hpq_init(&hpq, HPQLES, val_cmp, NULL);
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&hpq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *min = hpq_entry(hpq_pop(&hpq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &hpq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        const struct val *front = hpq_entry(hpq_pop(&hpq), struct val, elem);
        CHECK(front->val, vals[i].val, int, "%d");
    }
    CHECK(hpq_empty(&hpq), true, bool, "%b");
    return PASS;
}

static enum test_result
hpq_test_max_round_robin(void)
{
    struct heap_pqueue hpq;
    hpq_init(&hpq, HPQLES, val_cmp, NULL);
    const int size = 50;
    struct val vals[size];
    vals[0].id = 99;
    vals[0].val = 0;
    hpq_push(&hpq, &vals[0].elem);
    for (int i = 1; i < size; ++i)
    {
        vals[i].val = 99;
        vals[i].id = i;
        hpq_push(&hpq, &vals[i].elem);
        CHECK(hpq_validate(&hpq), true, bool, "%b");
    }
    /* Now let's make sure we pop round robin. */
    int last_id = 0;
    while (!hpq_empty(&hpq))
    {
        const struct val *front = hpq_entry(hpq_pop(&hpq), struct val, elem);
        CHECK(last_id < front->id, true, bool, "%b");
        last_id = front->id;
    }
    return PASS;
}

static enum test_result
hpq_test_min_round_robin(void)
{
    struct heap_pqueue hpq;
    hpq_init(&hpq, HPQLES, val_cmp, NULL);
    const int size = 50;
    struct val vals[size];
    vals[0].id = 99;
    vals[0].val = 99;
    hpq_push(&hpq, &vals[0].elem);
    for (int i = 1; i < size; ++i)
    {
        vals[i].val = 1;
        vals[i].id = i;
        hpq_push(&hpq, &vals[i].elem);
        CHECK(hpq_validate(&hpq), true, bool, "%b");
    }
    /* Now let's make sure we pop round robin. */
    int last_id = 0;
    while (!hpq_empty(&hpq))
    {
        const struct val *front = hpq_entry(hpq_pop(&hpq), struct val, elem);
        CHECK(last_id < front->id, true, bool, "%b");
        last_id = front->id;
    }
    return PASS;
}

static enum test_result
hpq_test_delete_prime_shuffle_duplicates(void)
{
    struct heap_pqueue hpq;
    hpq_init(&hpq, HPQLES, val_cmp, NULL);
    const int size = 99;
    const int prime = 101;
    /* Make the prime shuffle shorter than size for many duplicates. */
    const int less = 77;
    struct val vals[size];
    int shuffled_index = prime % (size - less);
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = i;
        hpq_push(&hpq, &vals[i].elem);
        CHECK(hpq_validate(&hpq), true, bool, "%b");
        const size_t s = i + 1;
        CHECK(hpq_size(&hpq), s, size_t, "%zu");
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }

    shuffled_index = prime % (size - less);
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        (void)hpq_erase(&hpq, &vals[shuffled_index].elem);
        CHECK(hpq_validate(&hpq), true, bool, "%b");
        --cur_size;
        CHECK(hpq_size(&hpq), cur_size, size_t, "%zu");
        /* Shuffle normally here so we only remove each elem once. */
        shuffled_index = (shuffled_index + prime) % size;
    }
    return PASS;
}

static enum test_result
hpq_test_prime_shuffle(void)
{
    struct heap_pqueue hpq;
    hpq_init(&hpq, HPQLES, val_cmp, NULL);
    const int size = 50;
    const int prime = 53;
    const int less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    int shuffled_index = prime % (size - less);
    struct val vals[size];
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = shuffled_index;
        hpq_push(&hpq, &vals[i].elem);
        CHECK(hpq_validate(&hpq), true, bool, "%b");
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        CHECK(hpq_erase(&hpq, &vals[i].elem) != NULL, true, bool, "%b");
        CHECK(hpq_validate(&hpq), true, bool, "%b");
        --cur_size;
        CHECK(hpq_size(&hpq), cur_size, size_t, "%zu");
    }
    return PASS;
}

static enum test_result
hpq_test_weak_srand(void)
{
    struct heap_pqueue hpq;
    hpq_init(&hpq, HPQLES, val_cmp, NULL);
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const int num_heap_elems = 1000;
    struct val vals[num_heap_elems];
    for (int i = 0; i < num_heap_elems; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        hpq_push(&hpq, &vals[i].elem);
        CHECK(hpq_validate(&hpq), true, bool, "%b");
    }
    for (int i = 0; i < num_heap_elems; ++i)
    {
        CHECK(hpq_erase(&hpq, &vals[i].elem) != NULL, true, bool, "%b");
        CHECK(hpq_validate(&hpq), true, bool, "%b");
    }
    CHECK(hpq_empty(&hpq), true, bool, "%b");
    return PASS;
}

static enum test_result
insert_shuffled(struct heap_pqueue *hpq, struct val vals[], const size_t size,
                const int larger_prime)
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
        hpq_push(hpq, &vals[shuffled_index].elem);
        CHECK(hpq_size(hpq), i + 1, size_t, "%zu");
        CHECK(hpq_validate(hpq), true, bool, "%b");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(hpq_size(hpq), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, struct heap_pqueue *hpq)
{
    if (hpq_size(hpq) != size)
    {
        return 0;
    }
    size_t i = 0;
    struct heap_pqueue copy;
    hpq_init(&copy, hpq_order(hpq), val_cmp, NULL);
    while (!hpq_empty(hpq))
    {
        struct hpq_elem *const front = hpq_pop(hpq);
        vals[i++] = hpq_entry(front, struct val, elem)->val;
        hpq_push(&copy, front);
    }
    while (!hpq_empty(&copy))
    {
        hpq_push(hpq, hpq_pop(&copy));
    }
    return i;
}

static enum heap_pq_threeway_cmp
val_cmp(const struct hpq_elem *a, const struct hpq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = hpq_entry(a, struct val, elem);
    struct val *rhs = hpq_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
