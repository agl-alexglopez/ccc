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

check_begin(check_order, Singly_linked_list const *const singly_linked_list,
            size_t const n, int const order[])
{
    size_t i = 0;
    struct Val const *v = begin(singly_linked_list);
    for (; v != end(singly_linked_list) && i < n;
         v = next(singly_linked_list, &v->e), ++i)
    {
        check(v == NULL, false);
        check(v->val, order[i]);
    }
    check(i, n);
    check_fail_end({
        (void)fprintf(stderr, "%sCHECK: (int[%zu]){", CHECK_GREEN, n);
        for (size_t j = 0; j < n; ++j)
        {
            (void)fprintf(stderr, "%d, ", order[j]);
        }
        (void)fprintf(stderr, "}\n%s", CHECK_NONE);
        (void)fprintf(stderr, "%sCHECK_ERROR:%s (int[%zu]){", CHECK_RED,
                      CHECK_GREEN, n);
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
                (void)fprintf(stderr, "%s%d, %s", CHECK_GREEN, order[j],
                              CHECK_NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", CHECK_RED, v->val,
                              CHECK_NONE);
            }
        }
        for (; v != end(singly_linked_list);
             v = next(singly_linked_list, &v->e))
        {
            (void)fprintf(stderr, "%s%d, %s", CHECK_RED, v->val, CHECK_NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", CHECK_GREEN, CHECK_NONE);
    });
}

check_begin(create_list, CCC_Singly_linked_list *const singly_linked_list,
            size_t const n, struct Val vals[])
{
    for (size_t i = 0; i < n; ++i)
    {
        check(push_front(singly_linked_list, &vals[i].e) == NULL, false);
    }
    check(validate(singly_linked_list), true);
    check_end();
}
