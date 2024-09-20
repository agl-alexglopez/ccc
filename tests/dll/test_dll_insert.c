#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "dll_util.h"
#include "doubly_linked_list.h"
#include "test.h"
#include "traits.h"

#include <stddef.h>

BEGIN_STATIC_TEST(dll_test_push_three_front)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    CHECK(dll_push_front(&dll, &(struct val){}.e) != NULL, true);
    CHECK(dll_validate(&dll), true);
    CHECK(dll_push_front(&dll, &(struct val){.id = 1, .val = 1}.e) != NULL,
          true);
    CHECK(dll_validate(&dll), true);
    CHECK(dll_push_front(&dll, &(struct val){.id = 2, .val = 2}.e) != NULL,
          true);
    CHECK(dll_validate(&dll), true);
    CHECK(size(&dll), 3);
    struct val *v = dll_front(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 2);
    v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 0);
    END_TEST();
}

BEGIN_STATIC_TEST(dll_test_push_three_back)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    CHECK(dll_push_back(&dll, &(struct val){}.e) != NULL, true);
    CHECK(dll_validate(&dll), true);
    CHECK(dll_push_back(&dll, &(struct val){.id = 1, .val = 1}.e) != NULL,
          true);
    CHECK(dll_validate(&dll), true);
    CHECK(dll_push_back(&dll, &(struct val){.id = 2, .val = 2}.e) != NULL,
          true);
    CHECK(dll_validate(&dll), true);
    CHECK(size(&dll), 3);
    struct val *v = dll_front(&dll);
    CHECK(v->id, 0);
    CHECK(v == NULL, false);
    v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 2);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(dll_test_push_three_front, dll_test_push_three_back);
}
