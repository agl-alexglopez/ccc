#include "pqueue.h"
#include "test.h"

struct val
{
    int id;
    int val;
    struct pq_elem elem;
};

static enum test_result pq_test_empty(void);
static enum pq_threeway_cmp val_cmp(const struct pq_elem *,
                                    const struct pq_elem *, void *);

#define NUM_TESTS (size_t)1
const test_fn all_tests[NUM_TESTS] = {pq_test_empty};

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
pq_test_empty(void)
{
    struct pqueue pq = PQ_INIT(PQLES, val_cmp, NULL);
    CHECK(pq_empty(&pq), true, bool, "%d");
    return PASS;
}

static enum pq_threeway_cmp
val_cmp(const struct pq_elem *a, const struct pq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = PQ_ENTRY(a, struct val, elem);
    struct val *rhs = PQ_ENTRY(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
