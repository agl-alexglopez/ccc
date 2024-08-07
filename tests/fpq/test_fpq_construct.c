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
static ccc_fpq_threeway_cmp val_cmp(ccc_fpq_elem const *, ccc_fpq_elem const *,
                                    void *);

#define NUM_TESTS (size_t)1
test_fn const all_tests[NUM_TESTS] = {pq_test_empty};

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
    ccc_flat_pqueue pq;
    ccc_fpq_init(&pq, CCC_FPQ_LES, val_cmp, NULL);
    CHECK(ccc_fpq_empty(&pq), true, bool, "%d");
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
