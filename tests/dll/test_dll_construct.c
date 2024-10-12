#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "dll_util.h"
#include "doubly_linked_list.h"
#include "test.h"
#include "traits.h"

#include <stddef.h>

BEGIN_STATIC_TEST(dll_test_construct)
{
    struct val val = {};
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    CHECK(is_empty(&dll), true);
    CHECK(dll_push_front(&dll, &val.e) != NULL, true);
    CHECK(is_empty(&dll), false);
    CHECK(size(&dll), 1);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(dll_test_construct());
}
