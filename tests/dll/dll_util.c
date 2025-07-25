#include <stddef.h>
#include <stdio.h>

#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "dll_util.h"
#include "doubly_linked_list.h"
#include "traits.h"
#include "types.h"

ccc_threeway_cmp
val_cmp(ccc_any_type_cmp const c)
{
    struct val const *const a = c.any_type_lhs;
    struct val const *const b = c.any_type_rhs;
    return (a->val > b->val) - (a->val < b->val);
}

CHECK_BEGIN_FN(check_order, doubly_linked_list const *const dll, size_t const n,
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
        CHECK(v == NULL, false);
        CHECK(order[i], v->val);
    }
    i = n;
    for (v = rbegin(dll); v != rend(dll) && i--; v = rnext(dll, &v->e))
    {
        CHECK(v == NULL, false);
        CHECK(order[i], v->val);
    }
    CHECK_END_FN_FAIL({
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
        for (; v != end(dll); v = next(dll, &v->e))
        {
            (void)fprintf(stderr, "%s%d, %s", RED, v->val, NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", GREEN, NONE);
    });
}

CHECK_BEGIN_FN(create_list, ccc_doubly_linked_list *const dll,
               enum push_end const dir, size_t const n, struct val vals[])
{
    for (size_t i = 0; i < n; ++i)
    {
        if (dir == UTIL_PUSH_FRONT)
        {
            CHECK(push_front(dll, &vals[i].e) == NULL, false);
        }
        else
        {
            CHECK(push_back(dll, &vals[i].e) == NULL, false);
        }
    }
    CHECK(validate(dll), true);
    CHECK_END_FN();
}
