#include "pqueue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_pq_elem elem;
};

static enum test_result pq_test_empty(void);
static ccc_threeway_cmp val_cmp(void const *, void const *, void *);

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
    ccc_pqueue pq = CCC_PQ_INIT(struct val, elem, CCC_LES, val_cmp, NULL);
    CHECK(ccc_pq_empty(&pq), true, "%d");
    return PASS;
}

static ccc_threeway_cmp
val_cmp(void const *const a, void const *const b, void *aux)
{
    (void)aux;
    struct val const *const lhs = a;
    struct val const *const rhs = b;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
