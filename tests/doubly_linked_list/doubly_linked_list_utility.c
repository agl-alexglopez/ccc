#include <stddef.h>
#include <stdio.h>

#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "doubly_linked_list.h"
#include "doubly_linked_list_utility.h"
#include "traits.h"
#include "types.h"

CCC_Order
val_order(CCC_Type_comparator_context const c)
{
    struct Val const *const a = c.type_lhs;
    struct Val const *const b = c.type_rhs;
    return (a->val > b->val) - (a->val < b->val);
}

CHECK_BEGIN_FN(check_order, Doubly_linked_list const *const doubly_linked_list,
               size_t const n, int const order[])
{
    if (!n)
    {
        return PASS;
    }
    size_t i = 0;
    struct Val *v = begin(doubly_linked_list);
    for (; v != end(doubly_linked_list) && i < n;
         v = next(doubly_linked_list, &v->e), ++i)
    {
        CHECK(v == NULL, false);
        CHECK(order[i], v->val);
    }
    i = n;
    for (v = rbegin(doubly_linked_list); v != rend(doubly_linked_list) && i--;
         v = rnext(doubly_linked_list, &v->e))
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
        v = begin(doubly_linked_list);
        for (size_t j = 0; j < n && v != end(doubly_linked_list);
             ++j, v = next(doubly_linked_list, &v->e))
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
        for (; v != end(doubly_linked_list);
             v = next(doubly_linked_list, &v->e))
        {
            (void)fprintf(stderr, "%s%d, %s", RED, v->val, NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", GREEN, NONE);
    });
}

CHECK_BEGIN_FN(create_list, CCC_Doubly_linked_list *const doubly_linked_list,
               enum Push_direction const dir, size_t const n, struct Val vals[])
{
    for (size_t i = 0; i < n; ++i)
    {
        if (dir == UTIL_PUSH_FRONT)
        {
            CHECK(push_front(doubly_linked_list, &vals[i].e) == NULL, false);
        }
        else
        {
            CHECK(push_back(doubly_linked_list, &vals[i].e) == NULL, false);
        }
    }
    CHECK(validate(doubly_linked_list), true);
    CHECK_END_FN();
}
