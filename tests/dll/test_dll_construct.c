#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "dll_util.h"
#include "doubly_linked_list.h"
#include "traits.h"

CHECK_BEGIN_STATIC_FN(dll_test_construct)
{
    struct val val = {};
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    CHECK(is_empty(&dll), true);
    CHECK(dll_push_front(&dll, &val.e) != NULL, true);
    CHECK(is_empty(&dll), false);
    CHECK(count(&dll).count, 1);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(dll_test_construct());
}
