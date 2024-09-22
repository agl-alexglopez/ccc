#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "singly_linked_list.h"
#include "sll_util.h"
#include "test.h"
#include "traits.h"

BEGIN_STATIC_TEST(sll_test_construct)
{
    singly_linked_list sll = sll_init(sll, struct val, e, NULL, val_cmp, NULL);
    CHECK(empty(&sll), true);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(sll_test_construct());
}
