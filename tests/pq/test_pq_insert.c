#include "pqueue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct val
{
    int id;
    int val;
    ccc_pq_elem elem;
};

static enum test_result pq_test_insert_one(void);
static enum test_result pq_test_insert_three(void);
static enum test_result pq_test_insert_shuffle(void);
static enum test_result pq_test_struct_getter(void);
static enum test_result pq_test_insert_three_dups(void);
static enum test_result pq_test_read_max_min(void);
static enum test_result insert_shuffled(ccc_pqueue *, struct val[], size_t,
                                        int);
static enum test_result inorder_fill(int[], size_t, ccc_pqueue *);
static ccc_pq_threeway_cmp val_cmp(ccc_pq_elem const *, ccc_pq_elem const *,
                                   void *);

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
    ccc_pqueue pq = CCC_PQ_INIT(CCC_PQ_LES, val_cmp, NULL);
    struct val single;
    single.val = 0;
    ccc_pq_push(&pq, &single.elem);
    CHECK(ccc_pq_empty(&pq), false, bool, "%d");
    return PASS;
}

static enum test_result
pq_test_insert_three(void)
{
    ccc_pqueue pq = CCC_PQ_INIT(CCC_PQ_LES, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        ccc_pq_push(&pq, &three_vals[i].elem);
        CHECK(ccc_pq_validate(&pq), true, bool, "%d");
        CHECK(ccc_pq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(ccc_pq_size(&pq), 3, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_struct_getter(void)
{
    ccc_pqueue pq = CCC_PQ_INIT(CCC_PQ_LES, val_cmp, NULL);
    ccc_pqueue pq_tester_clone = CCC_PQ_INIT(CCC_PQ_LES, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        ccc_pq_push(&pq, &vals[i].elem);
        ccc_pq_push(&pq_tester_clone, &tester_clone[i].elem);
        CHECK(ccc_pq_validate(&pq), true, bool, "%d");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get
            = CCC_PQ_OF(struct val, elem, &tester_clone[i].elem);
        CHECK(get->val, vals[i].val, int, "%d");
    }
    CHECK(ccc_pq_size(&pq), 10ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_insert_three_dups(void)
{
    ccc_pqueue pq = CCC_PQ_INIT(CCC_PQ_LES, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        ccc_pq_push(&pq, &three_vals[i].elem);
        CHECK(ccc_pq_validate(&pq), true, bool, "%d");
        CHECK(ccc_pq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(ccc_pq_size(&pq), 3ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_insert_shuffle(void)
{
    ccc_pqueue pq = CCC_PQ_INIT(CCC_PQ_LES, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *min = CCC_PQ_OF(struct val, elem, ccc_pq_front(&pq));
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &pq), PASS, enum test_result, "%d");
    return PASS;
}

static enum test_result
pq_test_read_max_min(void)
{
    ccc_pqueue pq = CCC_PQ_INIT(CCC_PQ_LES, val_cmp, NULL);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        ccc_pq_push(&pq, &vals[i].elem);
        CHECK(ccc_pq_validate(&pq), true, bool, "%d");
        CHECK(ccc_pq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(ccc_pq_size(&pq), 10ULL, size_t, "%zu");
    struct val const *min = CCC_PQ_OF(struct val, elem, ccc_pq_front(&pq));
    CHECK(min->val, 0, int, "%d");
    return PASS;
}

static enum test_result
insert_shuffled(ccc_pqueue *pq, struct val vals[], size_t const size,
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
        ccc_pq_push(pq, &vals[shuffled_index].elem);
        CHECK(ccc_pq_size(pq), i + 1, size_t, "%zu");
        CHECK(ccc_pq_validate(pq), true, bool, "%d");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_pq_size(pq), size, size_t, "%zu");
    return PASS;
}

static enum test_result
inorder_fill(int vals[], size_t size, ccc_pqueue *ppq)
{
    if (ccc_pq_size(ppq) != size)
    {
        return FAIL;
    }
    size_t i = 0;
    ccc_pqueue copy = CCC_PQ_INIT(ccc_pq_order(ppq), val_cmp, NULL);
    while (!ccc_pq_empty(ppq))
    {
        ccc_pq_elem *const front = ccc_pq_pop(ppq);
        CHECK(ccc_pq_validate(ppq), true, bool, "%d");
        CHECK(ccc_pq_validate(&copy), true, bool, "%d");
        vals[i++] = CCC_PQ_OF(struct val, elem, front)->val;
        ccc_pq_push(&copy, front);
    }
    i = 0;
    while (!ccc_pq_empty(&copy))
    {
        struct val *v = CCC_PQ_OF(struct val, elem, ccc_pq_pop(&copy));
        CHECK(v->val, vals[i++], int, "%d");
        ccc_pq_push(ppq, &v->elem);
        CHECK(ccc_pq_validate(ppq), true, bool, "%d");
        CHECK(ccc_pq_validate(&copy), true, bool, "%d");
    }
    return PASS;
}

static ccc_pq_threeway_cmp
val_cmp(ccc_pq_elem const *a, ccc_pq_elem const *b, void *aux)
{
    (void)aux;
    struct val *lhs = CCC_PQ_OF(struct val, elem, a);
    struct val *rhs = CCC_PQ_OF(struct val, elem, b);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
