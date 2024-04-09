#include "pqueue.h"
#include "test.h"
#include "tree.h"

#include <stdio.h>

struct val
{
    int id;
    int val;
    pq_elem elem;
};

static enum test_result pq_test_empty(void);

#define NUM_TESTS (size_t)1
const struct fn_name all_tests[NUM_TESTS] = {
    {pq_test_empty, "pq_test_empty"},
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        const bool fail = all_tests[i].fn() == FAIL;
        if (fail)
        {
            res = FAIL;
            (void)fprintf(stderr, "failure in test_pq_construct.c: %s\n",
                          all_tests[i].name);
        }
    }
    return res;
}

static enum test_result
pq_test_empty(void)
{
    pqueue pq;
    pq_init(&pq);
    CHECK(pq_empty(&pq), true, bool, "%b");
    return PASS;
}
