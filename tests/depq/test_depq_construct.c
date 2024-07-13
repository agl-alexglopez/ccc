#include "depqueue.h"
#include "test.h"
#include "tree.h"

struct val
{
    int id;
    int val;
    struct depq_elem elem;
};

static enum test_result depq_test_empty(void);
static dpq_threeway_cmp val_cmp(const struct depq_elem *,
                                const struct depq_elem *, void *);

#define NUM_TESTS (size_t)1
const test_fn all_tests[NUM_TESTS] = {depq_test_empty};

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
depq_test_empty(void)
{
    struct depqueue pq = DEPQ_INIT(pq, val_cmp, NULL);
    CHECK(depq_empty(&pq), true, bool, "%d");
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
