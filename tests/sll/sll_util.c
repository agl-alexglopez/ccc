#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "sll_util.h"
#include "singly_linked_list.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>

ccc_threeway_cmp
val_cmp(ccc_cmp const *const c)
{
    struct val const *const a = c->container_a;
    struct val const *const b = c->container_b;
    return (a->val > b->val) - (a->val < b->val);
}

BEGIN_TEST(check_order, singly_linked_list const *const sll, size_t const n,
           int const order[])
{
    size_t i = 0;
    for (struct val const *iter = begin(sll); iter != end(sll) && i < n;
         iter = next(sll, &iter->e), ++i)
    {
        CHECK(iter->val, order[i]);
    }
    END_TEST();
}
