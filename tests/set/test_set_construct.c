#include "set.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_set_elem elem;
};

static enum test_result set_test_empty(void);
static ccc_set_threeway_cmp val_cmp(ccc_set_elem const *, ccc_set_elem const *,
                                    void *);

#define NUM_TESTS ((size_t)1)
test_fn const all_tests[NUM_TESTS] = {
    set_test_empty,
};

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
set_test_empty(void)
{
    ccc_set s = CCC_SET_INIT(s, val_cmp, NULL);
    CHECK(ccc_set_empty(&s), true, bool, "%d");
    return PASS;
}

static ccc_set_threeway_cmp
val_cmp(ccc_set_elem const *a, ccc_set_elem const *b, void *aux)
{
    (void)aux;
    struct val *lhs = CCC_SET_IN(a, struct val, elem);
    struct val *rhs = CCC_SET_IN(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
