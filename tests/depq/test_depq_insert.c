#include "depqueue.h"
#include "test.h"
#include "tree.h"

#include <stdio.h>

struct val
{
    int id;
    int val;
    struct depq_elem elem;
};

static enum test_result depq_test_insert_one(void);
static enum test_result depq_test_insert_three(void);
static enum test_result depq_test_insert_shuffle(void);
static enum test_result depq_test_struct_getter(void);
static enum test_result depq_test_insert_three_dups(void);
static enum test_result depq_test_read_max_min(void);
static enum test_result insert_shuffled(struct depqueue *, struct val[], size_t,
                                        int);
static size_t inorder_fill(int[], size_t, struct depqueue *);
static dpq_threeway_cmp val_cmp(const struct depq_elem *,
                                const struct depq_elem *, void *);

#define NUM_TESTS (size_t)6
const test_fn all_tests[NUM_TESTS] = {
    depq_test_insert_one,     depq_test_insert_three,
    depq_test_struct_getter,  depq_test_insert_three_dups,
    depq_test_insert_shuffle, depq_test_read_max_min,
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
depq_test_insert_one(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
    struct val single;
    single.val = 0;
    depq_push(&pq, &single.elem);
    CHECK(depq_empty(&pq), false, bool, "%d");
    CHECK(DEPQ_ENTRY(depq_root(&pq), struct val, elem)->val == single.val, true,
          bool, "%d");
    return PASS;
}

static enum test_result
depq_test_insert_three(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        depq_push(&pq, &three_vals[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%d");
        CHECK(depq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(depq_size(&pq), 3, size_t, "%zu");
    return PASS;
}

static enum test_result
depq_test_struct_getter(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
    struct depqueue pq_tester_clone = DEPQ_INIT(pq_tester_clone, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        depq_push(&pq, &vals[i].elem);
        depq_push(&pq_tester_clone, &tester_clone[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%d");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        const struct val *get
            = DEPQ_ENTRY(&tester_clone[i].elem, struct val, elem);
        CHECK(get->val, vals[i].val, int, "%d");
    }
    CHECK(depq_size(&pq), 10ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
depq_test_insert_three_dups(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        depq_push(&pq, &three_vals[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%d");
        CHECK(depq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(depq_size(&pq), 3ULL, size_t, "%zu");
    return PASS;
}

static dpq_threeway_cmp
val_cmp(const struct depq_elem *a, const struct depq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = DEPQ_ENTRY(a, struct val, elem);
    struct val *rhs = DEPQ_ENTRY(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static enum test_result
depq_test_insert_shuffle(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
    /* Math magic ahead... */
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *max = DEPQ_ENTRY(depq_const_max(&pq), struct val, elem);
    CHECK(max->val, size - 1, int, "%d");
    const struct val *min = DEPQ_ENTRY(depq_const_min(&pq), struct val, elem);
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
depq_test_read_max_min(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        depq_push(&pq, &vals[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%d");
        CHECK(depq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(depq_size(&pq), 10ULL, size_t, "%zu");
    const struct val *max = DEPQ_ENTRY(depq_const_max(&pq), struct val, elem);
    CHECK(max->val, 9, int, "%d");
    const struct val *min = DEPQ_ENTRY(depq_const_min(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    return PASS;
}

static enum test_result
insert_shuffled(struct depqueue *pq, struct val vals[], const size_t size,
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
