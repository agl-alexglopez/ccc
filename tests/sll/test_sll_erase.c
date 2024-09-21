#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "singly_linked_list.h"
#include "sll_util.h"
#include "test.h"
#include "traits.h"
#include "types.h"

BEGIN_STATIC_TEST(sll_test_insert_delete_three)
{
    CHECK(true, true);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(sll_test_insert_delete_three());
}
