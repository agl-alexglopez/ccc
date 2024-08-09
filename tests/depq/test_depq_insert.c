#include "depqueue.h"
#include "test.h"
#include "tree.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct val
{
    int id;
    int val;
    ccc_depq_elem elem;
};

static enum test_result depq_test_insert_one(void);
static enum test_result depq_test_insert_three(void);
static enum test_result depq_test_insert_shuffle(void);
static enum test_result depq_test_struct_getter(void);
static enum test_result depq_test_insert_three_dups(void);
static enum test_result depq_test_read_max_min(void);
static enum test_result insert_shuffled(ccc_depqueue *, struct val[], size_t,
                                        int);
static size_t inorder_fill(int[], size_t, ccc_depqueue *);
static ccc_depq_threeway_cmp val_cmp(ccc_depq_elem const *,
                                     ccc_depq_elem const *, void *);

#define NUM_TESTS (size_t)6
test_fn const all_tests[NUM_TESTS] = {
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
        bool const fail = all_tests[i]() == FAIL;
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
    ccc_depqueue pq = CCC_DEPQ_INIT(pq, val_cmp, NULL);
    struct val single;
    single.val = 0;
    ccc_depq_push(&pq, &single.elem);
    CHECK(ccc_depq_empty(&pq), false, bool, "%d");
    CHECK(CCC_DEPQ_OF(struct val, elem, ccc_depq_root(&pq))->val == single.val,
          true, bool, "%d");
    return PASS;
}

static enum test_result
depq_test_insert_three(void)
{
    ccc_depqueue pq = CCC_DEPQ_INIT(pq, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        ccc_depq_push(&pq, &three_vals[i].elem);
        CHECK(ccc_tree_validate(&pq.t), true, bool, "%d");
        CHECK(ccc_depq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(ccc_depq_size(&pq), 3, size_t, "%zu");
    return PASS;
}

static enum test_result
depq_test_struct_getter(void)
{
    ccc_depqueue pq = CCC_DEPQ_INIT(pq, val_cmp, NULL);
    ccc_depqueue pq_tester_clone
        = CCC_DEPQ_INIT(pq_tester_clone, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        ccc_depq_push(&pq, &vals[i].elem);
        ccc_depq_push(&pq_tester_clone, &tester_clone[i].elem);
        CHECK(ccc_tree_validate(&pq.t), true, bool, "%d");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get
            = CCC_DEPQ_OF(struct val, elem, &tester_clone[i].elem);
        CHECK(get->val, vals[i].val, int, "%d");
    }
    CHECK(ccc_depq_size(&pq), 10ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
depq_test_insert_three_dups(void)
{
    ccc_depqueue pq = CCC_DEPQ_INIT(pq, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        ccc_depq_push(&pq, &three_vals[i].elem);
        CHECK(ccc_tree_validate(&pq.t), true, bool, "%d");
        CHECK(ccc_depq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(ccc_depq_size(&pq), 3ULL, size_t, "%zu");
    return PASS;
}

static ccc_depq_threeway_cmp
val_cmp(ccc_depq_elem const *a, ccc_depq_elem const *b, void *aux)
{
    (void)aux;
    struct val *lhs = CCC_DEPQ_OF(struct val, elem, a);
    struct val *rhs = CCC_DEPQ_OF(struct val, elem, b);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static enum test_result
depq_test_insert_shuffle(void)
{
    ccc_depqueue pq = CCC_DEPQ_INIT(pq, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *max
        = CCC_DEPQ_OF(struct val, elem, ccc_depq_const_max(&pq));
    CHECK(max->val, size - 1, int, "%d");
    struct val const *min
        = CCC_DEPQ_OF(struct val, elem, ccc_depq_const_min(&pq));
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
    ccc_depqueue pq = CCC_DEPQ_INIT(pq, val_cmp, NULL);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        ccc_depq_push(&pq, &vals[i].elem);
        CHECK(ccc_tree_validate(&pq.t), true, bool, "%d");
        CHECK(ccc_depq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(ccc_depq_size(&pq), 10ULL, size_t, "%zu");
    struct val const *max
        = CCC_DEPQ_OF(struct val, elem, ccc_depq_const_max(&pq));
    CHECK(max->val, 9, int, "%d");
    struct val const *min
        = CCC_DEPQ_OF(struct val, elem, ccc_depq_const_min(&pq));
    CHECK(min->val, 0, int, "%d");
    return PASS;
}

static enum test_result
insert_shuffled(ccc_depqueue *pq, struct val vals[], size_t const size,
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
        ccc_depq_push(pq, &vals[shuffled_index].elem);
        CHECK(ccc_depq_size(pq), i + 1, size_t, "%zu");
        CHECK(ccc_tree_validate(&pq->t), true, bool, "%d");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_depq_size(pq), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, ccc_depqueue *pq)
{
    if (ccc_depq_size(pq) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (ccc_depq_elem *e = ccc_depq_rbegin(pq); e; e = ccc_depq_rnext(pq, e))
    {
        vals[i++] = CCC_DEPQ_OF(struct val, elem, e)->val;
    }
    return i;
}
