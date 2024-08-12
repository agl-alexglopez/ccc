#include "buf.h"
#include "flat_pqueue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_fpq_elem elem;
};

static enum test_result pq_test_empty(void);
static enum test_result pq_test_macro(void);
static enum test_result pq_test_push(void);
static ccc_fpq_threeway_cmp val_cmp(ccc_fpq_elem const *, ccc_fpq_elem const *,
                                    void *);

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
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_FPQ_LES, val_cmp, NULL);
    CHECK(ccc_fpq_empty(&pq), true, bool, "%d");
    return PASS;
}

static enum test_result
pq_test_macro(void)
{
    struct val vals[1] = {{0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 1, NULL);
    ccc_flat_pqueue pq
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_FPQ_LES, val_cmp, NULL);
    ccc_buf_result res
        = CCC_FPQ_EMPLACE(&pq, struct val, (struct val){.val = 0, .id = 0});
    CHECK(res, CCC_BUF_OK, ccc_buf_result, "%d");
    CHECK(ccc_fpq_empty(&pq), false, bool, "%d");
    ccc_buf_result res2
        = CCC_FPQ_EMPLACE(&pq, struct val, (struct val){.val = 0, .id = 0});
    CHECK(res2, CCC_BUF_FULL, ccc_buf_result, "%d");
    return PASS;
}

static enum test_result
pq_test_push(void)
{
    struct val vals[1] = {{0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 1, NULL);
    ccc_flat_pqueue pq
        = CCC_FPQ_INIT(&buf, struct val, elem, CCC_FPQ_LES, val_cmp, NULL);
    ccc_buf_result res = ccc_fpq_push(&pq, &vals[0]);
    CHECK(res, CCC_BUF_OK, ccc_buf_result, "%d");
    CHECK(ccc_fpq_empty(&pq), false, bool, "%d");
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
