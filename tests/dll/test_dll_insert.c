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
    CHECK(push_front(&dll, &(struct val){}.e) != NULL, true);
    CHECK(validate(&dll), true);
    CHECK(push_front(&dll, &(struct val){.id = 1, .val = 1}.e) != NULL, true);
    CHECK(validate(&dll), true);
    CHECK(push_front(&dll, &(struct val){.id = 2, .val = 2}.e) != NULL, true);
    CHECK(validate(&dll), true);
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
    CHECK(push_back(&dll, &(struct val){}.e) != NULL, true);
    CHECK(validate(&dll), true);
    CHECK(push_back(&dll, &(struct val){.id = 1, .val = 1}.e) != NULL, true);
    CHECK(validate(&dll), true);
    CHECK(push_back(&dll, &(struct val){.id = 2, .val = 2}.e) != NULL, true);
    CHECK(validate(&dll), true);
    CHECK(size(&dll), 3);
    struct val *v = dll_front(&dll);
    CHECK(v->id, 0);
    CHECK(v == NULL, false);
    v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 2);
    END_TEST();
}

BEGIN_STATIC_TEST(dll_test_push_and_splice)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    struct val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum test_result const t = create_list(&dll, UTIL_PUSH_BACK, 4, vals);
    CHECK(t, PASS);
    splice(&dll, dll_begin_elem(&dll), &dll, &vals[3].e);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){3, 0, 1, 2}), PASS);
    splice(&dll, &vals[2].e, &dll, &vals[3].e);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){0, 1, 3, 2}), PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(dll_test_push_and_splice_range)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    struct val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum test_result const t = create_list(&dll, UTIL_PUSH_BACK, 4, vals);
    CHECK(t, PASS);
    splice_range(&dll, dll_begin_elem(&dll), &dll, &vals[1].e,
                 dll_end_sentinel(&dll));
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){1, 2, 3, 0}), PASS);
    splice_range(&dll, dll_begin_elem(&dll), &dll, &vals[2].e,
                 dll_end_sentinel(&dll));
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){2, 3, 0, 1}), PASS);
    splice_range(&dll, &vals[2].e, &dll, &vals[3].e, &vals[1].e);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){3, 0, 2, 1}), PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(dll_test_push_and_splice_no_ops)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    struct val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum test_result const t = create_list(&dll, UTIL_PUSH_BACK, 4, vals);
    CHECK(t, PASS);
    splice_range(&dll, &vals[0].e, &dll, &vals[0].e, dll_end_sentinel(&dll));
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){0, 1, 2, 3}), PASS);
    splice_range(&dll, &vals[3].e, &dll, &vals[1].e, &vals[3].e);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){0, 1, 2, 3}), PASS);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(dll_test_push_three_front(), dll_test_push_three_back(),
                     dll_test_push_and_splice(),
                     dll_test_push_and_splice_range(),
                     dll_test_push_and_splice_no_ops());
}
