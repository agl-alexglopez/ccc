#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "dll_util.h"
#include "doubly_linked_list.h"
#include "test.h"
#include "traits.h"
#include "types.h"

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
    CHECK(n > 0, true);
    size_t i = 0;
    for (struct val *v = begin(dll); v != end(dll) && i < n;
         v = next(dll, &v->e), ++i)
    {
        CHECK(order[i], v->val);
    }
    i = n;
    for (struct val *v = rbegin(dll); v != rend(dll) && i--;
         v = rnext(dll, &v->e))
    {
        CHECK(order[i], v->val);
    }
    END_TEST();
}
