#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "dll_util.h"
#include "doubly_linked_list.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdio.h>

ccc_threeway_cmp
val_cmp(ccc_cmp const *const c)
{
    struct val const *const a = c->container_a;
    struct val const *const b = c->container_b;
    return (a->val > b->val) - (a->val < b->val);
}

BEGIN_TEST(check_order, doubly_linked_list const *const dll, size_t const n,
           int const order[])
{
    if (!n)
    {
        return PASS;
    }
    size_t i = 0;
    struct val *v = begin(dll);
    for (; v != end(dll) && i < n; v = next(dll, &v->e), ++i)
    {
        CHECK(order[i], v->val);
    }
    i = n;
    for (v = rbegin(dll); v != rend(dll) && i--; v = rnext(dll, &v->e))
    {
        CHECK(order[i], v->val);
    }
    END_FAIL({
        (void)fprintf(stderr, "%sCHECK: (int[%zu]){", GREEN, n);
        for (size_t j = 0; j < n; ++j)
        {
            (void)fprintf(stderr, "%d, ", order[j]);
        }
        (void)fprintf(stderr, "}\n%s", NONE);
        (void)fprintf(stderr, "%sERROR:%s (int[%zu]){", RED, GREEN, n);
        v = begin(dll);
        for (size_t j = 0; j < n && v != end(dll); ++j, v = next(dll, &v->e))
        {
            if (order[j] == v->val)
            {
                (void)fprintf(stderr, "%s%d, %s", GREEN, order[j], NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", RED, v->val, NONE);
            }
        }
        (void)fprintf(stderr, "%s}\n%s", GREEN, NONE);
    });
}
