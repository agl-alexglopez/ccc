#include "pqueue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct val
{
    int id;
    int val;
    pq_elem elem;
};

static enum test_result pq_test_insert_one(void);
static enum test_result pq_test_insert_three(void);
static enum test_result pq_test_insert_shuffle(void);
static enum test_result pq_test_struct_getter(void);
static enum test_result pq_test_insert_three_dups(void);
static enum test_result pq_test_read_max_min(void);
static enum test_result insert_shuffled(pqueue *, struct val[], size_t, int);
static size_t inorder_fill(int[], size_t, pqueue *);
static pq_threeway_cmp val_cmp(pq_elem const *, pq_elem const *, void *);

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
    pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
    struct val single;
    single.val = 0;
    pq_push(&pq, &single.elem);
    CHECK(pq_empty(&pq), false, bool, "%d");
    return PASS;
}

static enum test_result
pq_test_insert_three(void)
{
    pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        pq_push(&pq, &three_vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%d");
        CHECK(pq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(pq_size(&pq), 3, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_struct_getter(void)
{
    pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
    pqueue pq_tester_clone = PQ_INIT(PQLES, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        pq_push(&pq, &vals[i].elem);
        pq_push(&pq_tester_clone, &tester_clone[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%d");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get
            = PQ_ENTRY(&tester_clone[i].elem, struct val, elem);
        CHECK(get->val, vals[i].val, int, "%d");
    }
    CHECK(pq_size(&pq), 10ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_insert_three_dups(void)
{
    pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        pq_push(&pq, &three_vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%d");
        CHECK(pq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(pq_size(&pq), 3ULL, size_t, "%zu");
    return PASS;
}

static pq_threeway_cmp
val_cmp(pq_elem const *a, pq_elem const *b, void *aux)
{
    (void)aux;
    struct val *lhs = PQ_ENTRY(a, struct val, elem);
    struct val *rhs = PQ_ENTRY(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static enum test_result
pq_test_insert_shuffle(void)
{
    pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *min = PQ_ENTRY(pq_front(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &pq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    return PASS;
}

static enum test_result
pq_test_read_max_min(void)
{
    pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        pq_push(&pq, &vals[i].elem);
        CHECK(pq_validate(&pq), true, bool, "%d");
        CHECK(pq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(pq_size(&pq), 10ULL, size_t, "%zu");
    struct val const *min = PQ_ENTRY(pq_front(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    return PASS;
}

static enum test_result
insert_shuffled(pqueue *pq, struct val vals[], size_t const size,
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
        pq_push(pq, &vals[shuffled_index].elem);
        CHECK(pq_size(pq), i + 1, size_t, "%zu");
        CHECK(pq_validate(pq), true, bool, "%d");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(pq_size(pq), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, pqueue *ppq)
{
    if (pq_size(ppq) != size)
    {
        return 0;
    }
    size_t i = 0;
    pqueue copy = PQ_INIT(pq_order(ppq), val_cmp, NULL);
    while (!pq_empty(ppq) && i < size)
    {
        pq_elem *const front = pq_pop(ppq);
        CHECK(pq_validate(ppq), true, bool, "%d");
        vals[i++] = PQ_ENTRY(front, struct val, elem)->val;
        pq_push(&copy, front);
    }
    while (!pq_empty(&copy))
    {
        pq_push(ppq, pq_pop(&copy));
    }
    return i;
}
