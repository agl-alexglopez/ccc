#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "dll_util.h"
#include "doubly_linked_list.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>

CHECK_BEGIN_STATIC_FN(dll_test_push_three_front)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val v0 = {};
    CHECK(push_front(&dll, &v0.e) != NULL, true);
    CHECK(validate(&dll), true);
    struct val v1 = {.id = 1, .val = 1};
    CHECK(push_front(&dll, &v1.e) != NULL, true);
    CHECK(validate(&dll), true);
    struct val v2 = {.id = 2, .val = 2};
    CHECK(push_front(&dll, &v2.e) != NULL, true);
    CHECK(validate(&dll), true);
    CHECK(size(&dll).count, 3);
    struct val *v = dll_front(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 2);
    v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_push_three_back)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val v0 = {};
    CHECK(push_back(&dll, &v0.e) != NULL, true);
    CHECK(validate(&dll), true);
    struct val v1 = {.id = 1, .val = 1};
    CHECK(push_back(&dll, &v1.e) != NULL, true);
    CHECK(validate(&dll), true);
    struct val v2 = {.id = 2, .val = 2};
    CHECK(push_back(&dll, &v2.e) != NULL, true);
    CHECK(validate(&dll), true);
    CHECK(size(&dll).count, 3);
    struct val *v = dll_front(&dll);
    CHECK(v->id, 0);
    CHECK(v == NULL, false);
    v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_push_and_splice)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum check_result const t = create_list(&dll, UTIL_PUSH_BACK, 4, vals);
    CHECK(t, PASS);
    CHECK(splice(&dll, dll_begin_elem(&dll), &dll, &vals[3].e), CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){3, 0, 1, 2}), PASS);
    CHECK(splice(&dll, &vals[2].e, &dll, &vals[3].e), CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){0, 1, 3, 2}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_push_and_splice_range)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum check_result const t = create_list(&dll, UTIL_PUSH_BACK, 4, vals);
    CHECK(t, PASS);
    CHECK(splice_range(&dll, dll_begin_elem(&dll), &dll, &vals[1].e,
                       dll_end_sentinel(&dll)),
          CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){1, 2, 3, 0}), PASS);
    CHECK(splice_range(&dll, dll_begin_elem(&dll), &dll, &vals[2].e,
                       dll_end_sentinel(&dll)),
          CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){2, 3, 0, 1}), PASS);
    CHECK(splice_range(&dll, &vals[2].e, &dll, &vals[3].e, &vals[1].e),
          CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){3, 0, 2, 1}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_push_and_splice_no_ops)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum check_result const t = create_list(&dll, UTIL_PUSH_BACK, 4, vals);
    CHECK(t, PASS);
    CHECK(splice_range(&dll, &vals[0].e, &dll, &vals[0].e,
                       dll_end_sentinel(&dll)),
          CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){0, 1, 2, 3}), PASS);
    CHECK(splice_range(&dll, &vals[3].e, &dll, &vals[1].e, &vals[3].e),
          CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[]){0, 1, 2, 3}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_sort_even)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[8] = {{.val = 9},  {.val = 4},   {.val = 1}, {.val = 1},
                          {.val = 99}, {.val = -55}, {.val = 5}, {.val = 2}};
    enum check_result const t = create_list(&dll, UTIL_PUSH_BACK, 8, vals);
    CHECK(t, PASS);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 8, (int[8]){9, 4, 1, 1, 99, -55, 5, 2}), PASS);
    ccc_result const r = ccc_dll_sort(&dll);
    CHECK(r, CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 8, (int[8]){-55, 1, 1, 2, 4, 5, 9, 99}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_sort_odd)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[9]
        = {{.val = 9},   {.val = 4}, {.val = 1}, {.val = 1},  {.val = 99},
           {.val = -55}, {.val = 5}, {.val = 2}, {.val = -99}};
    enum check_result const t = create_list(&dll, UTIL_PUSH_BACK, 9, vals);
    CHECK(t, PASS);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 9, (int[9]){9, 4, 1, 1, 99, -55, 5, 2, -99}), PASS);
    ccc_result const r = ccc_dll_sort(&dll);
    CHECK(r, CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 9, (int[9]){-99, -55, 1, 1, 2, 4, 5, 9, 99}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_sort_reverse)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[8] = {{.val = 9}, {.val = 8}, {.val = 7}, {.val = 6},
                          {.val = 5}, {.val = 4}, {.val = 3}, {.val = 2}};
    enum check_result const t = create_list(&dll, UTIL_PUSH_BACK, 8, vals);
    CHECK(t, PASS);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 8, (int[8]){9, 8, 7, 6, 5, 4, 3, 2}), PASS);
    ccc_result const r = ccc_dll_sort(&dll);
    CHECK(r, CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 8, (int[8]){2, 3, 4, 5, 6, 7, 8, 9}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(dll_test_sort_runs)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[12]
        = {{.val = 99},  {.val = 101}, {.val = 103}, {.val = 4},
           {.val = 8},   {.val = 9},   {.val = -99}, {.val = -55},
           {.val = -55}, {.val = 3},   {.val = 7},   {.val = 10}};
    enum check_result t = create_list(&dll, UTIL_PUSH_BACK, 12, vals);
    CHECK(t, PASS);
    CHECK(validate(&dll), true);
    t = check_order(&dll, 12,
                    (int[12]){99, 101, 103, 4, 8, 9, -99, -55, -55, 3, 7, 10});
    CHECK(t, PASS);
    ccc_result const r = ccc_dll_sort(&dll);
    CHECK(r, CCC_RESULT_OK);
    CHECK(validate(&dll), true);
    t = check_order(&dll, 12,
                    (int[12]){-99, -55, -55, 3, 4, 7, 8, 9, 10, 99, 101, 103});
    CHECK(t, PASS);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(
        dll_test_push_three_front(), dll_test_push_three_back(),
        dll_test_push_and_splice(), dll_test_push_and_splice_range(),
        dll_test_push_and_splice_no_ops(), dll_test_sort_even(),
        dll_test_sort_odd(), dll_test_sort_reverse(), dll_test_sort_runs());
}
