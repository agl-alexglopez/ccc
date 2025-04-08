#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "sll_util.h"
#include "checkers.h"
#include "singly_linked_list.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdio.h>

ccc_threeway_cmp
val_cmp(ccc_cmp const c)
{
    struct val const *const a = c.any_type_lhs;
    struct val const *const b = c.any_type_rhs;
    return (a->val > b->val) - (a->val < b->val);
}

CHECK_BEGIN_FN(check_order, singly_linked_list const *const sll, size_t const n,
               int const order[])
{
    size_t i = 0;
    struct val const *v = begin(sll);
    for (; v != end(sll) && i < n; v = next(sll, &v->e), ++i)
    {
        CHECK(v == NULL, false);
        CHECK(v->val, order[i]);
    }
    CHECK(i, n);
    CHECK_END_FN_FAIL({
        (void)fprintf(stderr, "%sCHECK: (int[%zu]){", GREEN, n);
        for (size_t j = 0; j < n; ++j)
        {
            (void)fprintf(stderr, "%d, ", order[j]);
        }
        (void)fprintf(stderr, "}\n%s", NONE);
        (void)fprintf(stderr, "%sERROR:%s (int[%zu]){", RED, GREEN, n);
        v = begin(sll);
        for (size_t j = 0; j < n && v != end(sll); ++j, v = next(sll, &v->e))
        {
            if (!v)
            {
                return CHECK_STATUS;
            }
            if (order[j] == v->val)
            {
                (void)fprintf(stderr, "%s%d, %s", GREEN, order[j], NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", RED, v->val, NONE);
            }
        }
        for (; v != end(sll); v = next(sll, &v->e))
        {
            (void)fprintf(stderr, "%s%d, %s", RED, v->val, NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", GREEN, NONE);
    });
}

CHECK_BEGIN_FN(create_list, ccc_singly_linked_list *const sll, size_t const n,
               struct val vals[])
{
    for (size_t i = 0; i < n; ++i)
    {
        CHECK(push_front(sll, &vals[i].e) == NULL, false);
    }
    CHECK(validate(sll), true);
    CHECK_END_FN();
}
