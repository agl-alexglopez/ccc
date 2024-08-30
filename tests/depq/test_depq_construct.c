#include "depq_util.h"
#include "double_ended_priority_queue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>

static enum test_result depq_test_empty(void);

#define NUM_TESTS (size_t)1
test_fn const all_tests[NUM_TESTS] = {depq_test_empty};

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
depq_test_empty(void)
{
    ccc_double_ended_priority_queue pq
        = CCC_DEPQ_INIT(struct val, elem, pq, val_cmp, NULL);
    CHECK(ccc_depq_empty(&pq), true, "%d");
    return PASS;
}
