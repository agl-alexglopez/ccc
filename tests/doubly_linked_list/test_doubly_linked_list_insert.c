#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "doubly_linked_list.h"
#include "doubly_linked_list_utility.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_push_three_front)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val v0 = {};
    CHECK(push_front(&doubly_linked_list, &v0.e) != NULL, true);
    CHECK(validate(&doubly_linked_list), true);
    struct Val v1 = {.id = 1, .val = 1};
    CHECK(push_front(&doubly_linked_list, &v1.e) != NULL, true);
    CHECK(validate(&doubly_linked_list), true);
    struct Val v2 = {.id = 2, .val = 2};
    CHECK(push_front(&doubly_linked_list, &v2.e) != NULL, true);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(count(&doubly_linked_list).count, 3);
    struct Val *v = doubly_linked_list_front(&doubly_linked_list);
    CHECK(v == NULL, false);
    CHECK(v->id, 2);
    v = doubly_linked_list_back(&doubly_linked_list);
    CHECK(v == NULL, false);
    CHECK(v->id, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_push_three_back)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val v0 = {};
    CHECK(push_back(&doubly_linked_list, &v0.e) != NULL, true);
    CHECK(validate(&doubly_linked_list), true);
    struct Val v1 = {.id = 1, .val = 1};
    CHECK(push_back(&doubly_linked_list, &v1.e) != NULL, true);
    CHECK(validate(&doubly_linked_list), true);
    struct Val v2 = {.id = 2, .val = 2};
    CHECK(push_back(&doubly_linked_list, &v2.e) != NULL, true);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(count(&doubly_linked_list).count, 3);
    struct Val *v = doubly_linked_list_front(&doubly_linked_list);
    CHECK(v->id, 0);
    CHECK(v == NULL, false);
    v = doubly_linked_list_back(&doubly_linked_list);
    CHECK(v == NULL, false);
    CHECK(v->id, 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_push_and_splice)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum Check_result const t
        = create_list(&doubly_linked_list, UTIL_PUSH_BACK, 4, vals);
    CHECK(t, PASS);
    CHECK(splice(&doubly_linked_list,
                 doubly_linked_list_node_begin(&doubly_linked_list),
                 &doubly_linked_list, &vals[3].e),
          CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 4, (int[]){3, 0, 1, 2}), PASS);
    CHECK(splice(&doubly_linked_list, &vals[2].e, &doubly_linked_list,
                 &vals[3].e),
          CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 4, (int[]){0, 1, 3, 2}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_push_and_splice_range)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum Check_result const t
        = create_list(&doubly_linked_list, UTIL_PUSH_BACK, 4, vals);
    CHECK(t, PASS);
    CHECK(splice_range(&doubly_linked_list,
                       doubly_linked_list_node_begin(&doubly_linked_list),
                       &doubly_linked_list, &vals[1].e,
                       doubly_linked_list_sentinel_end(&doubly_linked_list)),
          CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 4, (int[]){1, 2, 3, 0}), PASS);
    CHECK(splice_range(&doubly_linked_list,
                       doubly_linked_list_node_begin(&doubly_linked_list),
                       &doubly_linked_list, &vals[2].e,
                       doubly_linked_list_sentinel_end(&doubly_linked_list)),
          CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 4, (int[]){2, 3, 0, 1}), PASS);
    CHECK(splice_range(&doubly_linked_list, &vals[2].e, &doubly_linked_list,
                       &vals[3].e, &vals[1].e),
          CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 4, (int[]){3, 0, 2, 1}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_push_and_splice_no_ops)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum Check_result const t
        = create_list(&doubly_linked_list, UTIL_PUSH_BACK, 4, vals);
    CHECK(t, PASS);
    CHECK(splice_range(&doubly_linked_list, &vals[0].e, &doubly_linked_list,
                       &vals[0].e,
                       doubly_linked_list_sentinel_end(&doubly_linked_list)),
          CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 4, (int[]){0, 1, 2, 3}), PASS);
    CHECK(splice_range(&doubly_linked_list, &vals[3].e, &doubly_linked_list,
                       &vals[1].e, &vals[3].e),
          CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 4, (int[]){0, 1, 2, 3}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_sort_even)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[8] = {{.val = 9},  {.val = 4},   {.val = 1}, {.val = 1},
                          {.val = 99}, {.val = -55}, {.val = 5}, {.val = 2}};
    enum Check_result const t
        = create_list(&doubly_linked_list, UTIL_PUSH_BACK, 8, vals);
    CHECK(t, PASS);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 8,
                      (int[8]){9, 4, 1, 1, 99, -55, 5, 2}),
          PASS);
    CHECK(doubly_linked_list_is_sorted(&doubly_linked_list), false);
    CCC_Result const r = CCC_doubly_linked_list_sort(&doubly_linked_list);
    CHECK(doubly_linked_list_is_sorted(&doubly_linked_list), true);
    CHECK(r, CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 8,
                      (int[8]){-55, 1, 1, 2, 4, 5, 9, 99}),
          PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_sort_odd)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[9]
        = {{.val = 9},   {.val = 4}, {.val = 1}, {.val = 1},  {.val = 99},
           {.val = -55}, {.val = 5}, {.val = 2}, {.val = -99}};
    enum Check_result const t
        = create_list(&doubly_linked_list, UTIL_PUSH_BACK, 9, vals);
    CHECK(t, PASS);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 9,
                      (int[9]){9, 4, 1, 1, 99, -55, 5, 2, -99}),
          PASS);
    CHECK(doubly_linked_list_is_sorted(&doubly_linked_list), false);
    CCC_Result const r = CCC_doubly_linked_list_sort(&doubly_linked_list);
    CHECK(doubly_linked_list_is_sorted(&doubly_linked_list), true);
    CHECK(r, CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 9,
                      (int[9]){-99, -55, 1, 1, 2, 4, 5, 9, 99}),
          PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_sort_reverse)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[8] = {{.val = 9}, {.val = 8}, {.val = 7}, {.val = 6},
                          {.val = 5}, {.val = 4}, {.val = 3}, {.val = 2}};
    enum Check_result const t
        = create_list(&doubly_linked_list, UTIL_PUSH_BACK, 8, vals);
    CHECK(t, PASS);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 8, (int[8]){9, 8, 7, 6, 5, 4, 3, 2}),
          PASS);
    CHECK(doubly_linked_list_is_sorted(&doubly_linked_list), false);
    CCC_Result const r = CCC_doubly_linked_list_sort(&doubly_linked_list);
    CHECK(doubly_linked_list_is_sorted(&doubly_linked_list), true);
    CHECK(r, CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 8, (int[8]){2, 3, 4, 5, 6, 7, 8, 9}),
          PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_sort_runs)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[12]
        = {{.val = 99},  {.val = 101}, {.val = 103}, {.val = 4},
           {.val = 8},   {.val = 9},   {.val = -99}, {.val = -55},
           {.val = -55}, {.val = 3},   {.val = 7},   {.val = 10}};
    enum Check_result t
        = create_list(&doubly_linked_list, UTIL_PUSH_BACK, 12, vals);
    CHECK(t, PASS);
    CHECK(validate(&doubly_linked_list), true);
    t = check_order(&doubly_linked_list, 12,
                    (int[12]){99, 101, 103, 4, 8, 9, -99, -55, -55, 3, 7, 10});
    CHECK(t, PASS);
    CHECK(doubly_linked_list_is_sorted(&doubly_linked_list), false);
    CCC_Result const r = CCC_doubly_linked_list_sort(&doubly_linked_list);
    CHECK(doubly_linked_list_is_sorted(&doubly_linked_list), true);
    CHECK(r, CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    t = check_order(&doubly_linked_list, 12,
                    (int[12]){-99, -55, -55, 3, 4, 7, 8, 9, 10, 99, 101, 103});
    CHECK(t, PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_sort_halves)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[12] = {{.val = 7},  {.val = 10}, {.val = 13}, {.val = 17},
                           {.val = 19}, {.val = 21}, {.val = 8},  {.val = 12},
                           {.val = 15}, {.val = 18}, {.val = 20}, {.val = 25}};
    enum Check_result t
        = create_list(&doubly_linked_list, UTIL_PUSH_FRONT, 12, vals);
    CHECK(t, PASS);
    CHECK(validate(&doubly_linked_list), true);
    t = check_order(&doubly_linked_list, 12,
                    (int[12]){25, 20, 18, 15, 12, 8, 21, 19, 17, 13, 10, 7});
    CHECK(t, PASS);
    CHECK(doubly_linked_list_is_sorted(&doubly_linked_list), false);
    CCC_Result const r = CCC_doubly_linked_list_sort(&doubly_linked_list);
    CHECK(doubly_linked_list_is_sorted(&doubly_linked_list), true);
    CHECK(r, CCC_RESULT_OK);
    t = check_order(&doubly_linked_list, 12,
                    (int[12]){7, 8, 10, 12, 13, 15, 17, 18, 19, 20, 21, 25});
    CHECK(validate(&doubly_linked_list), true);
    CHECK(t, PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_sort_insert)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val *inserted = doubly_linked_list_insert_sorted(
        &doubly_linked_list, &(struct Val){.val = -99999}.e);
    CHECK(inserted->val, -99999);
    CHECK(validate(&doubly_linked_list), true);
    (void)CCC_doubly_linked_list_pop_front(&doubly_linked_list);
    CHECK(validate(&doubly_linked_list), true);
    struct Val vals[9] = {
        [0] = {.val = 9}, [1] = {.val = 4},  [2] = {.val = 1},
        [3] = {.val = 1}, [4] = {.val = 99}, [5] = {.val = -55},
        [6] = {.val = 5}, [7] = {.val = 2},  [8] = {.val = -99},
    };
    enum Check_result const t
        = create_list(&doubly_linked_list, UTIL_PUSH_BACK, 9, vals);
    CHECK(t, PASS);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 9,
                      (int[9]){9, 4, 1, 1, 99, -55, 5, 2, -99}),
          PASS);
    CHECK(doubly_linked_list_is_sorted(&doubly_linked_list), false);
    CCC_Result const r = doubly_linked_list_sort(&doubly_linked_list);
    CHECK(doubly_linked_list_is_sorted(&doubly_linked_list), true);
    CHECK(r, CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 9,
                      (int[9]){-99, -55, 1, 1, 2, 4, 5, 9, 99}),
          PASS);
    struct Val to_insert[5] = {
        [0] = {.val = -101}, [1] = {.val = -65}, [2] = {.val = 3},
        [3] = {.val = 20},   [4] = {.val = 101},
    };
    /* Before -99. */
    inserted = doubly_linked_list_insert_sorted(&doubly_linked_list,
                                                &to_insert[0].e);
    CHECK(inserted != NULL, true);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(doubly_linked_list_rnext(&doubly_linked_list, &inserted->e),
          doubly_linked_list_rend(&doubly_linked_list));
    CHECK(doubly_linked_list_next(&doubly_linked_list, &inserted->e), &vals[8]);

    /* After -99. */
    inserted = doubly_linked_list_insert_sorted(&doubly_linked_list,
                                                &to_insert[1].e);
    CHECK(inserted != NULL, true);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(doubly_linked_list_rnext(&doubly_linked_list, &inserted->e),
          &vals[8]);
    CHECK(doubly_linked_list_next(&doubly_linked_list, &inserted->e), &vals[5]);

    /* Before 4. */
    inserted = doubly_linked_list_insert_sorted(&doubly_linked_list,
                                                &to_insert[2].e);
    CHECK(inserted != NULL, true);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(doubly_linked_list_rnext(&doubly_linked_list, &inserted->e),
          &vals[7]);
    CHECK(doubly_linked_list_next(&doubly_linked_list, &inserted->e), &vals[1]);

    /* Before 99. */
    inserted = doubly_linked_list_insert_sorted(&doubly_linked_list,
                                                &to_insert[3].e);
    CHECK(inserted != NULL, true);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(doubly_linked_list_rnext(&doubly_linked_list, &inserted->e),
          &vals[0]);
    CHECK(doubly_linked_list_next(&doubly_linked_list, &inserted->e), &vals[4]);

    /* After 99. */
    inserted = doubly_linked_list_insert_sorted(&doubly_linked_list,
                                                &to_insert[4].e);
    CHECK(inserted != NULL, true);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(doubly_linked_list_rnext(&doubly_linked_list, &inserted->e),
          &vals[4]);
    CHECK(doubly_linked_list_next(&doubly_linked_list, &inserted->e),
          doubly_linked_list_end(&doubly_linked_list));

    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(doubly_linked_list_test_push_three_front(),
                     doubly_linked_list_test_push_three_back(),
                     doubly_linked_list_test_push_and_splice(),
                     doubly_linked_list_test_push_and_splice_range(),
                     doubly_linked_list_test_push_and_splice_no_ops(),
                     doubly_linked_list_test_sort_even(),
                     doubly_linked_list_test_sort_odd(),
                     doubly_linked_list_test_sort_reverse(),
                     doubly_linked_list_test_sort_runs(),
                     doubly_linked_list_test_sort_halves(),
                     doubly_linked_list_test_sort_insert());
}
