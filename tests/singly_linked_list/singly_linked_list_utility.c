#include <stddef.h>
#include <stdio.h>

#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "singly_linked_list.h"
#include "singly_linked_list_utility.h"
#include "traits.h"
#include "types.h"

CCC_Order
val_order(CCC_Type_comparator_context const c)
{
    struct Val const *const a = c.type_lhs;
    struct Val const *const b = c.type_rhs;
    return (a->val > b->val) - (a->val < b->val);
}

CHECK_BEGIN_FN(check_order, Singly_linked_list const *const singly_linked_list,
               size_t const n, int const order[])
{
    size_t i = 0;
    struct Val const *v = begin(singly_linked_list);
    for (; v != end(singly_linked_list) && i < n;
         v = next(singly_linked_list, &v->e), ++i)
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
        v = begin(singly_linked_list);
        for (size_t j = 0; j < n && v != end(singly_linked_list);
             ++j, v = next(singly_linked_list, &v->e))
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
        for (; v != end(singly_linked_list);
             v = next(singly_linked_list, &v->e))
        {
            (void)fprintf(stderr, "%s%d, %s", RED, v->val, NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", GREEN, NONE);
    });
}

CHECK_BEGIN_FN(create_list, CCC_Singly_linked_list *const singly_linked_list,
               size_t const n, struct Val vals[])
{
    for (size_t i = 0; i < n; ++i)
    {
        CHECK(push_front(singly_linked_list, &vals[i].e) == NULL, false);
    }
    CHECK(validate(singly_linked_list), true);
    CHECK_END_FN();
}
