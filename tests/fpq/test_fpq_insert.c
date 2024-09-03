#include "flat_pqueue.h"
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
    struct val single[1];
    ccc_buf buf = CCC_BUF_INIT(single, sizeof(struct val), 1, NULL);
    ccc_flat_pqueue fpq = CCC_FPQ_INIT(&buf, offsetof(struct val, elem),
                                       CCC_FPQ_LES, val_cmp, NULL);
    single[0].val = 0;
    struct val *const v = ccc_buf_alloc(&buf);
    ccc_fpq_push(&fpq, &v->elem);
    CHECK(ccc_fpq_empty(&fpq), false, bool, "%d");
    return PASS;
}

static enum test_result
fpq_test_insert_three(void)
{
    size_t const size = 3;
    struct val three_vals[size];
    ccc_buf buf = CCC_BUF_INIT(three_vals, sizeof(struct val), size, NULL);
    ccc_flat_pqueue fpq = CCC_FPQ_INIT(&buf, offsetof(struct val, elem),
                                       CCC_FPQ_LES, val_cmp, NULL);
    for (size_t i = 0; i < size; ++i)
    {
        three_vals[i].val = (int)i;
        struct val *const v = ccc_buf_alloc(&buf);
        ccc_fpq_push(&fpq, &v->elem);
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
        CHECK(ccc_fpq_size(&fpq), i + 1, size_t, "%zu");
    }
    CHECK(ccc_fpq_size(&fpq), size, size_t, "%zu");
    return PASS;
}

static enum test_result
fpq_test_struct_getter(void)
{
    size_t const size = 10;
    struct val vals[size];
    ccc_buf buf = CCC_BUF_INIT(vals, sizeof(struct val), size, NULL);
    ccc_flat_pqueue fpq = CCC_FPQ_INIT(&buf, offsetof(struct val, elem),
                                       CCC_FPQ_LES, val_cmp, NULL);
    struct val tester_clone[size];
    ccc_buf buf_clone
        = CCC_BUF_INIT(tester_clone, sizeof(struct val), size, NULL);
    ccc_flat_pqueue fpq_clone = CCC_FPQ_INIT(
        &buf_clone, offsetof(struct val, elem), CCC_FPQ_LES, val_cmp, NULL);
    for (size_t i = 0; i < size; ++i)
    {
        vals[i].val = (int)i;
        tester_clone[i].val = (int)i;
        struct val *const v = ccc_buf_alloc(&buf);
        ccc_fpq_push(&fpq, &v->elem);
        struct val *const w = ccc_buf_alloc(&buf_clone);
        ccc_fpq_push(&fpq_clone, &w->elem);
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get
            = CCC_FPQ_OF(struct val, elem, &tester_clone[i].elem);
        CHECK(get->val, vals[i].val, int, "%d");
    }
    CHECK(ccc_fpq_size(&fpq), 10ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
fpq_test_insert_three_dups(void)
{
    size_t const size = 3;
    struct val three_vals[size];
    ccc_buf buf = CCC_BUF_INIT(three_vals, sizeof(struct val), size, NULL);
    ccc_flat_pqueue fpq = CCC_FPQ_INIT(&buf, offsetof(struct val, elem),
                                       CCC_FPQ_LES, val_cmp, NULL);
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        struct val *const v = ccc_buf_alloc(&buf);
        ccc_fpq_push(&fpq, &v->elem);
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
        CHECK(ccc_fpq_size(&fpq), i + 1, size_t, "%zu");
    }
    CHECK(ccc_fpq_size(&fpq), 3ULL, size_t, "%zu");
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

static enum test_result
fpq_test_insert_shuffle(void)
{
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    ccc_buf buf = CCC_BUF_INIT(vals, sizeof(struct val), size, NULL);
    ccc_flat_pqueue fpq = CCC_FPQ_INIT(&buf, offsetof(struct val, elem),
                                       CCC_FPQ_LES, val_cmp, NULL);
    CHECK(insert_shuffled(&fpq, vals, size, prime), PASS, enum test_result,
          "%d");
    struct val const *min = CCC_FPQ_OF(struct val, elem, ccc_fpq_front(&fpq));
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &fpq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    return PASS;
}

static enum test_result
fpq_test_read_max_min(void)
{
    size_t const size = 10;
    struct val vals[size];
    ccc_buf buf = CCC_BUF_INIT(vals, sizeof(struct val), size, NULL);
    ccc_flat_pqueue fpq = CCC_FPQ_INIT(&buf, offsetof(struct val, elem),
                                       CCC_FPQ_LES, val_cmp, NULL);
    for (size_t i = 0; i < size; ++i)
    {
        vals[i].val = (int)i;
        struct val *const v = ccc_buf_alloc(&buf);
        ccc_fpq_push(&fpq, &v->elem);
        CHECK(ccc_fpq_validate(&fpq), true, bool, "%d");
        CHECK(ccc_fpq_size(&fpq), i + 1, size_t, "%zu");
    }
    CHECK(ccc_fpq_size(&fpq), 10ULL, size_t, "%zu");
    struct val const *min = CCC_FPQ_OF(struct val, elem, ccc_fpq_front(&fpq));
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
        vals[i].val = (int)shuffled_index;
        struct val *const v = ccc_buf_alloc(ccc_fpq_buf(pq));
        ccc_fpq_push(pq, &v->elem);
        CHECK(ccc_fpq_size(pq), i + 1, size_t, "%zu");
        CHECK(ccc_fpq_validate(pq), true, bool, "%d");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_fpq_size(pq), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, ccc_flat_pqueue *fpq)
{
    if (ccc_fpq_size(fpq) != size)
    {
        return 0;
    }
    size_t i = 0;
    struct val *copy_buf = malloc(sizeof(struct val) * ccc_fpq_size(fpq));
    if (!copy_buf)
    {
        return 0;
    }
    ccc_buf buf
        = CCC_BUF_INIT(vals, sizeof(struct val), ccc_fpq_size(fpq), NULL);
    ccc_flat_pqueue fpq_copy = CCC_FPQ_INIT(&buf, offsetof(struct val, elem),
                                            CCC_FPQ_LES, val_cmp, NULL);
    while (!ccc_fpq_empty(fpq))
    {
        ccc_fpq_elem *const front = ccc_fpq_pop(fpq);
        vals[i++] = CCC_FPQ_OF(struct val, elem, front)->val;
        struct val *const v = ccc_buf_alloc(ccc_fpq_buf(&fpq_copy));
        *v = *CCC_FPQ_OF(struct val, elem, front);
        ccc_fpq_push(&fpq_copy, &v->elem);
    }
    while (!ccc_fpq_empty(&fpq_copy))
    {
        struct val *const v = ccc_buf_alloc(ccc_fpq_buf(fpq));
        *v = *CCC_FPQ_OF(struct val, elem, ccc_fpq_pop(&fpq_copy));
        ccc_fpq_push(fpq, &v->elem);
    }
    return i;
}
