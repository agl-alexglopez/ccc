#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "singly_linked_list.h"
#include "singly_linked_list_util.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_insert_three)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val v0 = (struct val){};
    CHECK(push_front(&singly_linked_list, &v0.e) != NULL, true);
    struct val *v = front(&singly_linked_list);
    CHECK(validate(&singly_linked_list), true);
    CHECK(v == NULL, false);
    CHECK(v->val, 0);
    struct val v1 = (struct val){.val = 1};
    CHECK(push_front(&singly_linked_list, &v1.e) != NULL, true);
    CHECK(validate(&singly_linked_list), true);
    v = front(&singly_linked_list);
    CHECK(v == NULL, false);
    CHECK(v->val, 1);
    struct val v2 = (struct val){.val = 2};
    CHECK(push_front(&singly_linked_list, &v2.e) != NULL, true);
    CHECK(validate(&singly_linked_list), true);
    v = front(&singly_linked_list);
    CHECK(v == NULL, false);
    CHECK(v->val, 2);
    CHECK(count(&singly_linked_list).count, 3);
    CHECK(check_order(&singly_linked_list, 3, (int[3]){2, 1, 0}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_push_and_splice)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum check_result const t = create_list(&singly_linked_list, 4, vals);
    CHECK(t, PASS);
    CHECK(check_order(&singly_linked_list, 4, (int[4]){3, 2, 1, 0}), PASS);
    CHECK(splice(&singly_linked_list,
                 singly_linked_list_node_begin(&singly_linked_list),
                 &singly_linked_list, &vals[0].e),
          CCC_RESULT_OK);
    CHECK(validate(&singly_linked_list), true);
    CHECK(check_order(&singly_linked_list, 4, (int[4]){3, 0, 2, 1}), PASS);
    CHECK(splice(&singly_linked_list, &vals[0].e, &singly_linked_list,
                 &vals[3].e),
          CCC_RESULT_OK);
    CHECK(validate(&singly_linked_list), true);
    CHECK(check_order(&singly_linked_list, 4, (int[4]){0, 3, 2, 1}), PASS);
    CHECK(splice(&singly_linked_list, &vals[1].e, &singly_linked_list,
                 &vals[0].e),
          CCC_RESULT_OK);
    CHECK(validate(&singly_linked_list), true);
    CHECK(check_order(&singly_linked_list, 4, (int[4]){3, 2, 1, 0}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_push_and_splice_range)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum check_result const t = create_list(&singly_linked_list, 5, vals);
    CHECK(t, PASS);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(splice_range(&singly_linked_list,
                       singly_linked_list_sentinel_begin(&singly_linked_list),
                       &singly_linked_list, &vals[2].e, &vals[0].e),
          CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){2, 1, 0, 4, 3}), PASS);
    CHECK(splice_range(&singly_linked_list, &vals[3].e, &singly_linked_list,
                       &vals[2].e, &vals[0].e),
          CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(splice_range(&singly_linked_list,
                       singly_linked_list_sentinel_begin(&singly_linked_list),
                       &singly_linked_list, &vals[3].e, &vals[1].e),
          CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){3, 2, 1, 4, 0}), PASS);
    CHECK(splice_range(&singly_linked_list, &vals[0].e, &singly_linked_list,
                       &vals[2].e, &vals[4].e),
          CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){3, 0, 2, 1, 4}), PASS);
    CHECK(splice_range(&singly_linked_list, &vals[1].e, &singly_linked_list,
                       &vals[0].e, &vals[2].e),
          CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){3, 1, 0, 2, 4}), PASS);
    CHECK(splice_range(&singly_linked_list, &vals[3].e, &singly_linked_list,
                       &vals[0].e, &vals[2].e),
          CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){3, 0, 2, 1, 4}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_push_and_splice_range_no_ops)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum check_result const t = create_list(&singly_linked_list, 5, vals);
    CHECK(t, PASS);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(splice(&singly_linked_list, &vals[2].e, &singly_linked_list,
                 &vals[2].e),
          CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(splice(&singly_linked_list, &vals[3].e, &singly_linked_list,
                 &vals[2].e),
          CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(splice_range(&singly_linked_list, &vals[3].e, &singly_linked_list,
                       &vals[2].e, &vals[0].e),
          CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(splice_range(&singly_linked_list, &vals[3].e, &singly_linked_list,
                       &vals[3].e, &vals[0].e),
          CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_sort_reverse)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val vals[6] = {{.val = 0}, {.val = 1}, {.val = 2},
                          {.val = 3}, {.val = 4}, {.val = 5}};
    enum check_result const t = create_list(&singly_linked_list, 6, vals);
    CHECK(t, PASS);
    CHECK(check_order(&singly_linked_list, 6, (int[6]){5, 4, 3, 2, 1, 0}),
          PASS);
    CHECK(validate(&singly_linked_list), true);
    CHECK(singly_linked_list_is_sorted(&singly_linked_list), false);
    CCC_Result const r = CCC_singly_linked_list_sort(&singly_linked_list);
    CHECK(singly_linked_list_is_sorted(&singly_linked_list), true);
    CHECK(r, CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 6, (int[6]){0, 1, 2, 3, 4, 5}),
          PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_sort_even)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val vals[8] = {
        [7] = {.val = 9}, [6] = {.val = 4},  [5] = {.val = 1},
        [4] = {.val = 3}, [3] = {.val = 99}, [2] = {.val = -55},
        [1] = {.val = 5}, [0] = {.val = 2},
    };
    enum check_result const t = create_list(&singly_linked_list, 8, vals);
    CHECK(t, PASS);
    CHECK(validate(&singly_linked_list), true);
    CHECK(check_order(&singly_linked_list, 8,
                      (int[8]){9, 4, 1, 3, 99, -55, 5, 2}),
          PASS);
    CHECK(singly_linked_list_is_sorted(&singly_linked_list), false);
    CCC_Result const r = CCC_singly_linked_list_sort(&singly_linked_list);
    CHECK(singly_linked_list_is_sorted(&singly_linked_list), true);
    CHECK(r, CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 8,
                      (int[8]){-55, 1, 2, 3, 4, 5, 9, 99}),
          PASS);
    CHECK(validate(&singly_linked_list), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_sort_odd)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val vals[9] = {
        [8] = {.val = 10},  [7] = {.val = 9}, [6] = {.val = 4},
        [5] = {.val = 1},   [4] = {.val = 1}, [3] = {.val = 99},
        [2] = {.val = -55}, [1] = {.val = 5}, [0] = {.val = 2},
    };
    enum check_result const t = create_list(&singly_linked_list, 9, vals);
    CHECK(t, PASS);
    CHECK(validate(&singly_linked_list), true);
    CHECK(check_order(&singly_linked_list, 9,
                      (int[9]){10, 9, 4, 1, 1, 99, -55, 5, 2}),
          PASS);
    CHECK(singly_linked_list_is_sorted(&singly_linked_list), false);
    CCC_Result const r = CCC_singly_linked_list_sort(&singly_linked_list);
    CHECK(singly_linked_list_is_sorted(&singly_linked_list), true);
    CHECK(r, CCC_RESULT_OK);
    CHECK(check_order(&singly_linked_list, 9,
                      (int[9]){-55, 1, 1, 2, 4, 5, 9, 10, 99}),
          PASS);
    CHECK(validate(&singly_linked_list), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_sort_runs)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val vals[12]
        = {{.val = 99},  {.val = 101}, {.val = 103}, {.val = 4},
           {.val = 8},   {.val = 9},   {.val = -99}, {.val = -55},
           {.val = -55}, {.val = 3},   {.val = 7},   {.val = 10}};
    enum check_result t = create_list(&singly_linked_list, 12, vals);
    CHECK(t, PASS);
    CHECK(validate(&singly_linked_list), true);
    t = check_order(&singly_linked_list, 12,
                    (int[12]){10, 7, 3, -55, -55, -99, 9, 8, 4, 103, 101, 99});
    CHECK(t, PASS);
    CHECK(singly_linked_list_is_sorted(&singly_linked_list), false);
    CCC_Result const r = CCC_singly_linked_list_sort(&singly_linked_list);
    CHECK(singly_linked_list_is_sorted(&singly_linked_list), true);
    CHECK(r, CCC_RESULT_OK);
    t = check_order(&singly_linked_list, 12,
                    (int[12]){-99, -55, -55, 3, 4, 7, 8, 9, 10, 99, 101, 103});
    CHECK(validate(&singly_linked_list), true);
    CHECK(t, PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_sort_halves)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val vals[12] = {{.val = 7},  {.val = 10}, {.val = 13}, {.val = 17},
                           {.val = 19}, {.val = 21}, {.val = 8},  {.val = 12},
                           {.val = 15}, {.val = 18}, {.val = 20}, {.val = 25}};
    enum check_result t = create_list(&singly_linked_list, 12, vals);
    CHECK(t, PASS);
    CHECK(validate(&singly_linked_list), true);
    t = check_order(&singly_linked_list, 12,
                    (int[12]){25, 20, 18, 15, 12, 8, 21, 19, 17, 13, 10, 7});
    CHECK(t, PASS);
    CHECK(singly_linked_list_is_sorted(&singly_linked_list), false);
    CCC_Result const r = CCC_singly_linked_list_sort(&singly_linked_list);
    CHECK(singly_linked_list_is_sorted(&singly_linked_list), true);
    CHECK(r, CCC_RESULT_OK);
    t = check_order(&singly_linked_list, 12,
                    (int[12]){7, 8, 10, 12, 13, 15, 17, 18, 19, 20, 21, 25});
    CHECK(validate(&singly_linked_list), true);
    CHECK(t, PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_sort_insert)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val *inserted = singly_linked_list_insert_sorted(
        &singly_linked_list, &(struct val){.val = -99999}.e);
    CHECK(inserted->val, -99999);
    CHECK(validate(&singly_linked_list), true);
    (void)CCC_singly_linked_list_pop_front(&singly_linked_list);
    CHECK(validate(&singly_linked_list), true);
    struct val vals[9] = {
        [8] = {.val = 9}, [7] = {.val = 4},  [6] = {.val = 1},
        [5] = {.val = 1}, [4] = {.val = 99}, [3] = {.val = -55},
        [2] = {.val = 5}, [1] = {.val = 2},  [0] = {.val = -99},
    };
    enum check_result const t = create_list(&singly_linked_list, 9, vals);
    CHECK(t, PASS);
    CHECK(validate(&singly_linked_list), true);
    CHECK(check_order(&singly_linked_list, 9,
                      (int[9]){9, 4, 1, 1, 99, -55, 5, 2, -99}),
          PASS);
    CHECK(singly_linked_list_is_sorted(&singly_linked_list), false);
    CCC_Result const r = singly_linked_list_sort(&singly_linked_list);
    CHECK(singly_linked_list_is_sorted(&singly_linked_list), true);
    CHECK(r, CCC_RESULT_OK);
    CHECK(validate(&singly_linked_list), true);
    CHECK(check_order(&singly_linked_list, 9,
                      (int[9]){-99, -55, 1, 1, 2, 4, 5, 9, 99}),
          PASS);
    struct val to_insert[5] = {
        [0] = {.val = -101}, [1] = {.val = -65}, [2] = {.val = 3},
        [3] = {.val = 20},   [4] = {.val = 101},
    };
    /* Before -99. */
    inserted = singly_linked_list_insert_sorted(&singly_linked_list,
                                                &to_insert[0].e);
    CHECK(inserted != NULL, true);
    CHECK(validate(&singly_linked_list), true);
    CHECK(singly_linked_list_next(&singly_linked_list, &inserted->e), &vals[0]);

    /* After -99. */
    inserted = singly_linked_list_insert_sorted(&singly_linked_list,
                                                &to_insert[1].e);
    CHECK(inserted != NULL, true);
    CHECK(validate(&singly_linked_list), true);
    CHECK(singly_linked_list_next(&singly_linked_list, &inserted->e), &vals[3]);

    /* Before 4. */
    inserted = singly_linked_list_insert_sorted(&singly_linked_list,
                                                &to_insert[2].e);
    CHECK(inserted != NULL, true);
    CHECK(validate(&singly_linked_list), true);
    CHECK(singly_linked_list_next(&singly_linked_list, &inserted->e), &vals[7]);

    /* Before 99. */
    inserted = singly_linked_list_insert_sorted(&singly_linked_list,
                                                &to_insert[3].e);
    CHECK(inserted != NULL, true);
    CHECK(validate(&singly_linked_list), true);
    CHECK(singly_linked_list_next(&singly_linked_list, &inserted->e), &vals[4]);

    /* After 99. */
    inserted = singly_linked_list_insert_sorted(&singly_linked_list,
                                                &to_insert[4].e);
    CHECK(inserted != NULL, true);
    CHECK(validate(&singly_linked_list), true);
    CHECK(singly_linked_list_next(&singly_linked_list, &inserted->e),
          singly_linked_list_end(&singly_linked_list));

    CHECK_END_FN();
}
int
main()
{
    return CHECK_RUN(singly_linked_list_test_insert_three(),
                     singly_linked_list_test_push_and_splice(),
                     singly_linked_list_test_push_and_splice_range(),
                     singly_linked_list_test_push_and_splice_range_no_ops(),
                     singly_linked_list_test_sort_even(),
                     singly_linked_list_test_sort_reverse(),
                     singly_linked_list_test_sort_odd(),
                     singly_linked_list_test_sort_runs(),
                     singly_linked_list_test_sort_halves(),
                     singly_linked_list_test_sort_insert());
}
