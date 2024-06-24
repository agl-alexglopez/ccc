#include "pair_pqueue.h"
#include "test.h"

#include <stdio.h>

struct val
{
    int id;
    int val;
    struct ppq_elem elem;
};

static enum test_result ppq_test_insert_one(void);
static enum test_result ppq_test_insert_three(void);
static enum test_result ppq_test_insert_shuffle(void);
static enum test_result ppq_test_struct_getter(void);
static enum test_result ppq_test_insert_three_dups(void);
static enum test_result ppq_test_read_max_min(void);
static enum test_result insert_shuffled(struct pair_pqueue *, struct val[],
                                        size_t, int);
static size_t inorder_fill(int[], size_t, struct pair_pqueue *);
static enum ppq_threeway_cmp val_cmp(const struct ppq_elem *,
                                     const struct ppq_elem *, void *);

#define NUM_TESTS (size_t)6
const test_fn all_tests[NUM_TESTS] = {
    ppq_test_insert_one,        ppq_test_insert_three,   ppq_test_struct_getter,
    ppq_test_insert_three_dups, ppq_test_insert_shuffle, ppq_test_read_max_min,
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
ppq_test_insert_one(void)
{
    struct pair_pqueue pq;
    ppq_init(&pq, PPQLES, val_cmp, NULL);
    struct val single;
    single.val = 0;
    ppq_push(&pq, &single.elem);
    CHECK(ppq_empty(&pq), false, bool, "%b");
    return PASS;
}

static enum test_result
ppq_test_insert_three(void)
{
    struct pair_pqueue pq;
    ppq_init(&pq, PPQLES, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        ppq_push(&pq, &three_vals[i].elem);
        CHECK(ppq_validate(&pq), true, bool, "%b");
        CHECK(ppq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(ppq_size(&pq), 3, size_t, "%zu");
    return PASS;
}

static enum test_result
ppq_test_struct_getter(void)
{
    struct pair_pqueue pq;
    ppq_init(&pq, PPQLES, val_cmp, NULL);
    struct pair_pqueue ppq_tester_clone;
    ppq_init(&ppq_tester_clone, PPQLES, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        ppq_push(&pq, &vals[i].elem);
        ppq_push(&ppq_tester_clone, &tester_clone[i].elem);
        CHECK(ppq_validate(&pq), true, bool, "%b");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        const struct val *get
            = ppq_entry(&tester_clone[i].elem, struct val, elem);
        CHECK(get->val, vals[i].val, int, "%d");
    }
    CHECK(ppq_size(&pq), 10ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
ppq_test_insert_three_dups(void)
{
    struct pair_pqueue pq;
    ppq_init(&pq, PPQLES, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        ppq_push(&pq, &three_vals[i].elem);
        CHECK(ppq_validate(&pq), true, bool, "%b");
        CHECK(ppq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(ppq_size(&pq), 3ULL, size_t, "%zu");
    return PASS;
}

static enum ppq_threeway_cmp
val_cmp(const struct ppq_elem *a, const struct ppq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = ppq_entry(a, struct val, elem);
    struct val *rhs = ppq_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static enum test_result
ppq_test_insert_shuffle(void)
{
    struct pair_pqueue pq;
    ppq_init(&pq, PPQLES, val_cmp, NULL);
    /* Math magic ahead... */
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *min = ppq_entry(ppq_front(&pq), struct val, elem);
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
ppq_test_read_max_min(void)
{
    struct pair_pqueue pq;
    ppq_init(&pq, PPQLES, val_cmp, NULL);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        ppq_push(&pq, &vals[i].elem);
        CHECK(ppq_validate(&pq), true, bool, "%b");
        CHECK(ppq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(ppq_size(&pq), 10ULL, size_t, "%zu");
    const struct val *min = ppq_entry(ppq_front(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    return PASS;
}

static enum test_result
insert_shuffled(struct pair_pqueue *pq, struct val vals[], const size_t size,
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
        ppq_push(pq, &vals[shuffled_index].elem);
        CHECK(ppq_size(pq), i + 1, size_t, "%zu");
        CHECK(ppq_validate(pq), true, bool, "%b");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ppq_size(pq), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, struct pair_pqueue *ppq)
{
    if (ppq_size(ppq) != size)
    {
        return 0;
    }
    size_t i = 0;
    struct pair_pqueue copy;
    ppq_init(&copy, ppq_order(ppq), val_cmp, NULL);
    while (!ppq_empty(ppq) && i < size)
    {
        struct ppq_elem *const front = ppq_pop(ppq);
        CHECK(ppq_validate(ppq), true, bool, "%b");
        vals[i++] = ppq_entry(front, struct val, elem)->val;
        ppq_push(&copy, front);
    }
    while (!ppq_empty(&copy))
    {
        ppq_push(ppq, ppq_pop(&copy));
    }
    return i;
}
