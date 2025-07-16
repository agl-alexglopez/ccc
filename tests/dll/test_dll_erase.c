#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "dll_util.h"
#include "doubly_linked_list.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(dll_test_pop_empty)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    CHECK(is_empty(&dll), true);
    CHECK(dll_pop_front(&dll), CCC_RESULT_ARG_ERROR);
    CHECK(dll_validate(&dll), true);
    CHECK(dll_pop_back(&dll), CCC_RESULT_ARG_ERROR);
    CHECK(dll_validate(&dll), true);
    CHECK(dll_front(&dll), NULL);
    CHECK(dll_back(&dll), NULL);
    CHECK(is_empty(&dll), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_push_pop_front)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[3] = {{.val = 0}, {.val = 1}, {.val = 2}};
    enum check_result const t = create_list(&dll, UTIL_PUSH_BACK, 3, vals);
    CHECK(t, PASS);
    CHECK(count(&dll).count, 3);
    struct val *v = dll_front(&dll);
    CHECK(v == NULL, false);
    CHECK(v->val, 0);
    CHECK(pop_front(&dll), CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    v = dll_front(&dll);
    CHECK(v == NULL, false);
    CHECK(v->val, 1);
    CHECK(pop_front(&dll), CCC_RESULT_OK);
    v = dll_front(&dll);
    CHECK(validate(&dll), true);
    CHECK(v == NULL, false);
    CHECK(v->val, 2);
    CHECK(pop_front(&dll), CCC_RESULT_OK);
    CHECK(is_empty(&dll), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_push_pop_back)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[3] = {{.val = 0}, {.val = 1}, {.val = 2}};
    enum check_result const t = create_list(&dll, UTIL_PUSH_BACK, 3, vals);
    CHECK(t, PASS);
    CHECK(count(&dll).count, 3);
    struct val *v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->val, 2);
    CHECK(pop_back(&dll), CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->val, 1);
    CHECK(pop_back(&dll), CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->val, 0);
    CHECK(pop_back(&dll), CCC_RESULT_OK);
    CHECK(is_empty(&dll), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_push_pop_middle)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum check_result const t = create_list(&dll, UTIL_PUSH_BACK, 4, vals);
    CHECK(t, PASS);
    CHECK(extract(&dll, &vals[2].e) != NULL, true);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 3, (int[3]){0, 1, 3}), PASS);
    (void)extract(&dll, &vals[1].e);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 2, (int[2]){0, 3}), PASS);
    (void)extract(&dll, &vals[3].e);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 1, (int[1]){0}), PASS);
    (void)extract(&dll, &vals[0].e);
    CHECK(validate(&dll), true);
    CHECK(is_empty(&dll), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_push_pop_middle_range)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum check_result const t = create_list(&dll, UTIL_PUSH_BACK, 5, vals);
    CHECK(t, PASS);
    (void)dll_extract_range(&dll, &vals[1].e, &vals[4].e);
    CHECK(validate(&dll), true);
    CHECK(count(&dll).count, 2);
    CHECK(check_order(&dll, 2, (int[2]){0, 4}), PASS);
    (void)dll_extract_range(&dll, &vals[0].e, dll_end_sentinel(&dll));
    CHECK(validate(&dll), true);
    CHECK(count(&dll).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_splice_two_lists)
{
    doubly_linked_list to_lose
        = dll_init(to_lose, struct val, e, val_cmp, NULL, NULL);
    struct val to_lose_vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum check_result t
        = create_list(&to_lose, UTIL_PUSH_BACK, 5, to_lose_vals);
    CHECK(t, PASS);
    doubly_linked_list to_gain
        = dll_init(to_gain, struct val, e, val_cmp, NULL, NULL);
    struct val to_gain_vals[2] = {{.val = 0}, {.val = 1}};
    t = create_list(&to_gain, UTIL_PUSH_BACK, 2, to_gain_vals);
    CHECK(t, PASS);
    CHECK(check_order(&to_lose, 5, (int[5]){0, 1, 2, 3, 4}), PASS);
    CHECK(dll_splice(&to_gain, dll_end_sentinel(&to_gain), &to_lose,
                     &to_lose_vals[0].e),
          CCC_RESULT_OK);
    CHECK(validate(&to_gain), true);
    CHECK(validate(&to_lose), true);
    CHECK(count(&to_gain).count, 3);
    CHECK(count(&to_lose).count, 4);
    CHECK(check_order(&to_gain, 3, (int[3]){0, 1, 0}), PASS);
    CHECK(check_order(&to_lose, 4, (int[4]){1, 2, 3, 4}), PASS);
    CHECK(dll_splice_range(&to_gain, dll_end_elem(&to_gain), &to_lose,
                           dll_begin_elem(&to_lose),
                           dll_end_sentinel(&to_lose)),
          CCC_RESULT_OK);
    CHECK(validate(&to_gain), true);
    CHECK(validate(&to_lose), true);
    CHECK(count(&to_gain).count, 7);
    CHECK(count(&to_lose).count, 0);
    CHECK(check_order(&to_gain, 7, (int[7]){0, 1, 1, 2, 3, 4, 0}), PASS);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(dll_test_pop_empty(), dll_test_push_pop_front(),
                     dll_test_push_pop_back(), dll_test_push_pop_middle(),
                     dll_test_push_pop_middle_range(),
                     dll_test_splice_two_lists());
}
