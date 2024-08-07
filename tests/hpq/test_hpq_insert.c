#include "heap_pqueue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct val
{
    int id;
    int val;
    ccc_fpq_elem elem;
};

static enum test_result fpq_test_insert_one(void);
static enum test_result fpq_test_insert_three(void);
static enum test_result fpq_test_insert_shuffle(void);
static enum test_result fpq_test_struct_getter(void);
static enum test_result fpq_test_insert_three_dups(void);
static enum test_result fpq_test_read_max_min(void);
static enum test_result insert_shuffled(ccc_flat_pqueue *, struct val[], size_t,
                                        int);
static size_t inorder_fill(int[], size_t, ccc_flat_pqueue *);
static ccc_fpq_threeway_cmp val_cmp(ccc_fpq_elem const *, ccc_fpq_elem const *,
                                    void *);

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
    ccc_flat_pqueue pq;
    ccc_fpq_init(&pq, HPQLES, val_cmp, NULL);
    struct val single;
    single.val = 0;
    ccc_fpq_push(&pq, &single.elem);
    CHECK(ccc_fpq_empty(&pq), false, bool, "%d");
    return PASS;
}

static enum test_result
fpq_test_insert_three(void)
{
    ccc_flat_pqueue pq;
    ccc_fpq_init(&pq, HPQLES, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        ccc_fpq_push(&pq, &three_vals[i].elem);
        CHECK(ccc_fpq_validate(&pq), true, bool, "%d");
        CHECK(ccc_fpq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(ccc_fpq_size(&pq), 3, size_t, "%zu");
    return PASS;
}

static enum test_result
fpq_test_struct_getter(void)
{
    ccc_flat_pqueue pq;
    ccc_fpq_init(&pq, HPQLES, val_cmp, NULL);
    ccc_flat_pqueue fpq_tester_clone;
    ccc_fpq_init(&fpq_tester_clone, HPQLES, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        ccc_fpq_push(&pq, &vals[i].elem);
        ccc_fpq_push(&fpq_tester_clone, &tester_clone[i].elem);
        CHECK(ccc_fpq_validate(&pq), true, bool, "%d");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get
            = HPQ_ENTRY(&tester_clone[i].elem, struct val, elem);
        CHECK(get->val, vals[i].val, int, "%d");
    }
    CHECK(ccc_fpq_size(&pq), 10ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
fpq_test_insert_three_dups(void)
{
    ccc_flat_pqueue pq;
    ccc_fpq_init(&pq, HPQLES, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        ccc_fpq_push(&pq, &three_vals[i].elem);
        CHECK(ccc_fpq_validate(&pq), true, bool, "%d");
        CHECK(ccc_fpq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(ccc_fpq_size(&pq), 3ULL, size_t, "%zu");
    return PASS;
}

static ccc_fpq_threeway_cmp
val_cmp(ccc_fpq_elem const *a, ccc_fpq_elem const *b, void *aux)
{
    (void)aux;
    struct val *lhs = HPQ_ENTRY(a, struct val, elem);
    struct val *rhs = HPQ_ENTRY(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static enum test_result
fpq_test_insert_shuffle(void)
{
    ccc_flat_pqueue pq;
    ccc_fpq_init(&pq, HPQLES, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *min = HPQ_ENTRY(ccc_fpq_front(&pq), struct val, elem);
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
fpq_test_read_max_min(void)
{
    ccc_flat_pqueue pq;
    ccc_fpq_init(&pq, HPQLES, val_cmp, NULL);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        ccc_fpq_push(&pq, &vals[i].elem);
        CHECK(ccc_fpq_validate(&pq), true, bool, "%d");
        CHECK(ccc_fpq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(ccc_fpq_size(&pq), 10ULL, size_t, "%zu");
    struct val const *min = HPQ_ENTRY(ccc_fpq_front(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    return PASS;
}

static enum test_result
insert_shuffled(ccc_flat_pqueue *pq, struct val vals[], size_t const size,
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
        ccc_fpq_push(pq, &vals[shuffled_index].elem);
        CHECK(ccc_fpq_size(pq), i + 1, size_t, "%zu");
        CHECK(ccc_fpq_validate(pq), true, bool, "%d");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_fpq_size(pq), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, ccc_flat_pqueue *hpq)
{
    if (ccc_fpq_size(hpq) != size)
    {
        return 0;
    }
    size_t i = 0;
    ccc_flat_pqueue copy;
    ccc_fpq_init(&copy, ccc_fpq_order(hpq), val_cmp, NULL);
    while (!ccc_fpq_empty(hpq) && i < size)
    {
        ccc_fpq_elem *const front = ccc_fpq_pop(hpq);
        vals[i++] = HPQ_ENTRY(front, struct val, elem)->val;
        ccc_fpq_push(&copy, front);
    }
    while (!ccc_fpq_empty(&copy))
    {
        ccc_fpq_push(hpq, ccc_fpq_pop(&copy));
    }
    return i;
}
