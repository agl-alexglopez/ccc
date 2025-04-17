#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "singly_linked_list.h"
#include "sll_util.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>

CHECK_BEGIN_STATIC_FN(sll_test_insert_three)
{
    singly_linked_list sll = sll_init(sll, struct val, e, val_cmp, NULL, NULL);
    struct val v0 = (struct val){};
    CHECK(push_front(&sll, &v0.e) != NULL, true);
    struct val *v = front(&sll);
    CHECK(validate(&sll), true);
    CHECK(v == NULL, false);
    CHECK(v->val, 0);
    struct val v1 = (struct val){.val = 1};
    CHECK(push_front(&sll, &v1.e) != NULL, true);
    CHECK(validate(&sll), true);
    v = front(&sll);
    CHECK(v == NULL, false);
    CHECK(v->val, 1);
    struct val v2 = (struct val){.val = 2};
    CHECK(push_front(&sll, &v2.e) != NULL, true);
    CHECK(validate(&sll), true);
    v = front(&sll);
    CHECK(v == NULL, false);
    CHECK(v->val, 2);
    CHECK(size(&sll).count, 3);
    CHECK(check_order(&sll, 3, (int[3]){2, 1, 0}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(sll_test_push_and_splice)
{
    singly_linked_list sll = sll_init(sll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum check_result const t = create_list(&sll, 4, vals);
    CHECK(t, PASS);
    CHECK(check_order(&sll, 4, (int[4]){3, 2, 1, 0}), PASS);
    CHECK(splice(&sll, sll_begin_elem(&sll), &sll, &vals[0].e), CCC_RESULT_OK);
    CHECK(validate(&sll), true);
    CHECK(check_order(&sll, 4, (int[4]){3, 0, 2, 1}), PASS);
    CHECK(splice(&sll, &vals[0].e, &sll, &vals[3].e), CCC_RESULT_OK);
    CHECK(validate(&sll), true);
    CHECK(check_order(&sll, 4, (int[4]){0, 3, 2, 1}), PASS);
    CHECK(splice(&sll, &vals[1].e, &sll, &vals[0].e), CCC_RESULT_OK);
    CHECK(validate(&sll), true);
    CHECK(check_order(&sll, 4, (int[4]){3, 2, 1, 0}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(sll_test_push_and_splice_range)
{
    singly_linked_list sll = sll_init(sll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum check_result const t = create_list(&sll, 5, vals);
    CHECK(t, PASS);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(splice_range(&sll, sll_begin_sentinel(&sll), &sll, &vals[2].e,
                       &vals[0].e),
          CCC_RESULT_OK);
    CHECK(check_order(&sll, 5, (int[5]){2, 1, 0, 4, 3}), PASS);
    CHECK(splice_range(&sll, &vals[3].e, &sll, &vals[2].e, &vals[0].e),
          CCC_RESULT_OK);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(splice_range(&sll, sll_begin_sentinel(&sll), &sll, &vals[3].e,
                       &vals[1].e),
          CCC_RESULT_OK);
    CHECK(check_order(&sll, 5, (int[5]){3, 2, 1, 4, 0}), PASS);
    CHECK(splice_range(&sll, &vals[0].e, &sll, &vals[2].e, &vals[4].e),
          CCC_RESULT_OK);
    CHECK(check_order(&sll, 5, (int[5]){3, 0, 2, 1, 4}), PASS);
    CHECK(splice_range(&sll, &vals[1].e, &sll, &vals[0].e, &vals[2].e),
          CCC_RESULT_OK);
    CHECK(check_order(&sll, 5, (int[5]){3, 1, 0, 2, 4}), PASS);
    CHECK(splice_range(&sll, &vals[3].e, &sll, &vals[0].e, &vals[2].e),
          CCC_RESULT_OK);
    CHECK(check_order(&sll, 5, (int[5]){3, 0, 2, 1, 4}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(sll_test_push_and_splice_range_no_ops)
{
    singly_linked_list sll = sll_init(sll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum check_result const t = create_list(&sll, 5, vals);
    CHECK(t, PASS);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(splice(&sll, &vals[2].e, &sll, &vals[2].e), CCC_RESULT_OK);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(splice(&sll, &vals[3].e, &sll, &vals[2].e), CCC_RESULT_OK);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(splice_range(&sll, &vals[3].e, &sll, &vals[2].e, &vals[0].e),
          CCC_RESULT_OK);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(splice_range(&sll, &vals[3].e, &sll, &vals[3].e, &vals[0].e),
          CCC_RESULT_OK);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(sll_test_sort_reverse)
{
    singly_linked_list sll = sll_init(sll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[6] = {{.val = 0}, {.val = 1}, {.val = 2},
                          {.val = 3}, {.val = 4}, {.val = 5}};
    enum check_result const t = create_list(&sll, 6, vals);
    CHECK(t, PASS);
    CHECK(check_order(&sll, 6, (int[6]){5, 4, 3, 2, 1, 0}), PASS);
    CHECK(validate(&sll), true);
    ccc_result const r = ccc_sll_sort(&sll);
    CHECK(r, CCC_RESULT_OK);
    CHECK(check_order(&sll, 6, (int[6]){0, 1, 2, 3, 4, 5}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(sll_test_sort_even)
{
    singly_linked_list sll = sll_init(sll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[8] = {
        [7] = {.val = 9}, [6] = {.val = 4},  [5] = {.val = 1},
        [4] = {.val = 3}, [3] = {.val = 99}, [2] = {.val = -55},
        [1] = {.val = 5}, [0] = {.val = 2},
    };
    enum check_result const t = create_list(&sll, 8, vals);
    CHECK(t, PASS);
    CHECK(validate(&sll), true);
    CHECK(check_order(&sll, 8, (int[8]){9, 4, 1, 3, 99, -55, 5, 2}), PASS);
    ccc_result const r = ccc_sll_sort(&sll);
    CHECK(r, CCC_RESULT_OK);
    CHECK(check_order(&sll, 8, (int[8]){-55, 1, 2, 3, 4, 5, 9, 99}), PASS);
    CHECK(validate(&sll), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(sll_test_sort_odd)
{
    singly_linked_list sll = sll_init(sll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[9] = {
        [8] = {.val = 10},  [7] = {.val = 9}, [6] = {.val = 4},
        [5] = {.val = 1},   [4] = {.val = 1}, [3] = {.val = 99},
        [2] = {.val = -55}, [1] = {.val = 5}, [0] = {.val = 2},
    };
    enum check_result const t = create_list(&sll, 9, vals);
    CHECK(t, PASS);
    CHECK(validate(&sll), true);
    CHECK(check_order(&sll, 9, (int[9]){10, 9, 4, 1, 1, 99, -55, 5, 2}), PASS);
    ccc_result const r = ccc_sll_sort(&sll);
    CHECK(r, CCC_RESULT_OK);
    CHECK(check_order(&sll, 9, (int[9]){-55, 1, 1, 2, 4, 5, 9, 10, 99}), PASS);
    CHECK(validate(&sll), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(sll_test_sort_runs)
{
    singly_linked_list sll = sll_init(sll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[12]
        = {{.val = 99},  {.val = 101}, {.val = 103}, {.val = 4},
           {.val = 8},   {.val = 9},   {.val = -99}, {.val = -55},
           {.val = -55}, {.val = 3},   {.val = 7},   {.val = 10}};
    enum check_result t = create_list(&sll, 12, vals);
    CHECK(t, PASS);
    CHECK(validate(&sll), true);
    t = check_order(&sll, 12,
                    (int[12]){10, 7, 3, -55, -55, -99, 9, 8, 4, 103, 101, 99});
    CHECK(t, PASS);
    ccc_result const r = ccc_sll_sort(&sll);
    CHECK(r, CCC_RESULT_OK);
    t = check_order(&sll, 12,
                    (int[12]){-99, -55, -55, 3, 4, 7, 8, 9, 10, 99, 101, 103});
    CHECK(validate(&sll), true);
    CHECK(t, PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(sll_test_sort_halves)
{
    singly_linked_list sll = sll_init(sll, struct val, e, val_cmp, NULL, NULL);
    struct val vals[12] = {{.val = 7},  {.val = 10}, {.val = 13}, {.val = 17},
                           {.val = 19}, {.val = 21}, {.val = 8},  {.val = 12},
                           {.val = 15}, {.val = 18}, {.val = 20}, {.val = 25}};
    enum check_result t = create_list(&sll, 12, vals);
    CHECK(t, PASS);
    CHECK(validate(&sll), true);
    t = check_order(&sll, 12,
                    (int[12]){25, 20, 18, 15, 12, 8, 21, 19, 17, 13, 10, 7});
    CHECK(t, PASS);
    ccc_result const r = ccc_sll_sort(&sll);
    CHECK(r, CCC_RESULT_OK);
    t = check_order(&sll, 12,
                    (int[12]){7, 8, 10, 12, 13, 15, 17, 18, 19, 20, 21, 25});
    CHECK(validate(&sll), true);
    CHECK(t, PASS);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(sll_test_insert_three(), sll_test_push_and_splice(),
                     sll_test_push_and_splice_range(),
                     sll_test_push_and_splice_range_no_ops(),
                     sll_test_sort_even(), sll_test_sort_reverse(),
                     sll_test_sort_odd(), sll_test_sort_runs(),
                     sll_test_sort_halves());
}
