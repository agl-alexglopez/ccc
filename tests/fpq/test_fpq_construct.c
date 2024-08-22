#include "flat_pqueue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>

struct val
{
    int id;
    int val;
};

static enum test_result pq_test_empty(void);
static enum test_result pq_test_macro(void);
static enum test_result pq_test_push(void);
static ccc_threeway_cmp val_cmp(void const *, void const *, void *);

#define NUM_TESTS (size_t)3
test_fn const all_tests[NUM_TESTS]
    = {pq_test_empty, pq_test_macro, pq_test_push};

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
pq_test_empty(void)
{
    struct val vals[1] = {{0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 1, NULL);
    ccc_flat_pqueue pq
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_LES, val_cmp, NULL);
    CHECK(ccc_fpq_empty(&pq), true, "%d");
    return PASS;
}

static enum test_result
pq_test_macro(void)
{
    struct val vals[1] = {{0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 1, NULL);
    ccc_flat_pqueue pq
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_LES, val_cmp, NULL);
    ccc_result res
        = CCC_FPQ_EMPLACE(&pq, struct val, (struct val){.val = 0, .id = 0});
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fpq_empty(&pq), false, "%d");
    ccc_result res2
        = CCC_FPQ_EMPLACE(&pq, struct val, (struct val){.val = 0, .id = 0});
    CHECK(res2, CCC_NO_REALLOC, "%d");
    return PASS;
}

static enum test_result
pq_test_push(void)
{
    struct val vals[1] = {{0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 1, NULL);
    ccc_flat_pqueue pq
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_LES, val_cmp, NULL);
    ccc_result res = ccc_fpq_push(&pq, &vals[0]);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fpq_empty(&pq), false, "%d");
    return PASS;
}

static ccc_threeway_cmp
val_cmp(void const *a, void const *b, void *aux)
{
    (void)aux;
    struct val const *const lhs = a;
    struct val const *const rhs = b;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
