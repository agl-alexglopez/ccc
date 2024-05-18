#include "pqueue.h"
#include "test.h"
#include "tree.h"

struct val
{
    int id;
    int val;
    struct pq_elem elem;
};

static enum test_result pq_test_empty(void);

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
    struct pqueue pq;
    pq_init(&pq);
    CHECK(pq_empty(&pq), true, bool, "%b");
    return PASS;
}
