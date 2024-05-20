#include "set.h"
#include "test.h"
#include "tree.h"

struct val
{
    int id;
    int val;
    struct set_elem elem;
};

static enum test_result set_test_empty(void);

#define NUM_TESTS ((size_t)1)
const test_fn all_tests[NUM_TESTS] = {
    set_test_empty,
};

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
set_test_empty(void)
{
    struct set s;
    set_init(&s);
    CHECK(set_empty(&s), true, bool, "%b");
    return PASS;
}