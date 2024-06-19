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
static node_threeway_cmp val_cmp(const struct set_elem *,
                                 const struct set_elem *, void *);

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
    set_init(&s, val_cmp, NULL);
    CHECK(set_empty(&s), true, bool, "%b");
    return PASS;
}

static node_threeway_cmp
val_cmp(const struct set_elem *a, const struct set_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = set_entry(a, struct val, elem);
    struct val *rhs = set_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
