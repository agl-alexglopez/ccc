#include "set.h"
#include "test.h"
#include "tree.h"

struct val
{
    int id;
    int val;
    set_elem elem;
};

static enum test_result set_test_empty(void);

#define NUM_TESTS ((size_t)1)
const struct fn_name all_tests[NUM_TESTS] = {
    {set_test_empty, "set_test_empty"},
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
            (void)fprintf(stderr, "failure in tests_set.c: %s\n",
                          all_tests[i].name);
        }
    }
    return res;
}

static enum test_result
set_test_empty(void)
{
    set s;
    set_init(&s);
    CHECK(set_empty(&s), true, bool, "%b");
    return PASS;
}
