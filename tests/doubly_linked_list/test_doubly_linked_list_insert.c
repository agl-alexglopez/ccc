#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "doubly_linked_list.h"
#include "doubly_linked_list_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/stack_allocator.h"

check_static_begin(doubly_linked_list_test_push_three_front)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 3);
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        struct Val, e, val_order, stack_allocator_allocate, &allocator);
    check(push_front(&doubly_linked_list, &(struct Val){}.e) != NULL, true);
    check(validate(&doubly_linked_list), true);
    check(push_front(&doubly_linked_list, &(struct Val){.id = 1, .val = 1}.e)
              != NULL,
          true);
    check(validate(&doubly_linked_list), true);
    check(push_front(&doubly_linked_list, &(struct Val){.id = 2, .val = 2}.e)
              != NULL,
          true);
    check(validate(&doubly_linked_list), true);
    check(count(&doubly_linked_list).count, 3);
    struct Val *v = doubly_linked_list_front(&doubly_linked_list);
    check(v == NULL, false);
    check(v->id, 2);
    v = doubly_linked_list_back(&doubly_linked_list);
    check(v == NULL, false);
    check(v->id, 0);
    check_end();
}

check_static_begin(doubly_linked_list_test_push_three_back)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 3);
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        struct Val, e, val_order, stack_allocator_allocate, &allocator);
    check(push_back(&doubly_linked_list, &(struct Val){}.e) != NULL, true);
    check(validate(&doubly_linked_list), true);
    check(push_back(&doubly_linked_list, &(struct Val){.id = 1, .val = 1}.e)
              != NULL,
          true);
    check(validate(&doubly_linked_list), true);
    check(push_back(&doubly_linked_list, &(struct Val){.id = 2, .val = 2}.e)
              != NULL,
          true);
    check(validate(&doubly_linked_list), true);
    check(count(&doubly_linked_list).count, 3);
    struct Val *v = doubly_linked_list_front(&doubly_linked_list);
    check(v->id, 0);
    check(v == NULL, false);
    v = doubly_linked_list_back(&doubly_linked_list);
    check(v == NULL, false);
    check(v->id, 2);
    check_end();
}

check_static_begin(doubly_linked_list_test_push_and_splice)
{
    Doubly_linked_list doubly_linked_list
        = doubly_linked_list_initialize(struct Val, e, val_order, NULL, NULL);
    struct Val vals[4] = {
        [0] = {.val = 0},
        [1] = {.val = 1},
        [2] = {.val = 2},
        [3] = {.val = 3},
    };
    enum Check_result const t
        = push_list(&doubly_linked_list, UTIL_PUSH_BACK, 4, vals);
    check(t, CHECK_PASS);
    check(splice(&doubly_linked_list,
                 doubly_linked_list_node_begin(&doubly_linked_list),
                 &doubly_linked_list, &vals[3].e),
          CCC_RESULT_OK);
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 4, (int[]){3, 0, 1, 2}), CHECK_PASS);
    check(splice(&doubly_linked_list, &vals[2].e, &doubly_linked_list,
                 &vals[3].e),
          CCC_RESULT_OK);
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 4, (int[]){0, 1, 3, 2}), CHECK_PASS);
    check_end();
}

check_static_begin(doubly_linked_list_test_push_and_splice_range)
{
    Doubly_linked_list doubly_linked_list
        = doubly_linked_list_initialize(struct Val, e, val_order, NULL, NULL);
    struct Val vals[4] = {
        [0] = {.val = 0},
        [1] = {.val = 1},
        [2] = {.val = 2},
        [3] = {.val = 3},
    };
    enum Check_result const t
        = push_list(&doubly_linked_list, UTIL_PUSH_BACK, 4, vals);
    check(t, CHECK_PASS);
    check(splice_range(&doubly_linked_list,
                       doubly_linked_list_node_begin(&doubly_linked_list),
                       &doubly_linked_list, &vals[1].e,
                       doubly_linked_list_end(&doubly_linked_list)),
          CCC_RESULT_OK);
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 4, (int[]){1, 2, 3, 0}), CHECK_PASS);
    check(splice_range(&doubly_linked_list,
                       doubly_linked_list_node_begin(&doubly_linked_list),
                       &doubly_linked_list, &vals[2].e,
                       doubly_linked_list_end(&doubly_linked_list)),
          CCC_RESULT_OK);
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 4, (int[]){2, 3, 0, 1}), CHECK_PASS);
    check(splice_range(&doubly_linked_list, &vals[2].e, &doubly_linked_list,
                       &vals[3].e, &vals[1].e),
          CCC_RESULT_OK);
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 4, (int[]){3, 0, 2, 1}), CHECK_PASS);
    check_end();
}

check_static_begin(doubly_linked_list_test_push_and_splice_no_ops)
{
    Doubly_linked_list doubly_linked_list
        = doubly_linked_list_initialize(struct Val, e, val_order, NULL, NULL);
    struct Val vals[4] = {
        [0] = {.val = 0},
        [1] = {.val = 1},
        [2] = {.val = 2},
        [3] = {.val = 3},
    };
    enum Check_result const t
        = push_list(&doubly_linked_list, UTIL_PUSH_BACK, 4, vals);
    check(t, CHECK_PASS);
    check(splice_range(&doubly_linked_list, &vals[0].e, &doubly_linked_list,
                       &vals[0].e, doubly_linked_list_end(&doubly_linked_list)),
          CCC_RESULT_OK);
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 4, (int[]){0, 1, 2, 3}), CHECK_PASS);
    check(splice_range(&doubly_linked_list, &vals[3].e, &doubly_linked_list,
                       &vals[1].e, &vals[3].e),
          CCC_RESULT_OK);
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 4, (int[]){0, 1, 2, 3}), CHECK_PASS);
    check_end();
}

check_static_begin(doubly_linked_list_test_sort_even)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 8);
    Doubly_linked_list doubly_linked_list = doubly_linked_list_from(
        e, val_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[8]){
            {.val = 9},
            {.val = 4},
            {.val = 1},
            {.val = 1},
            {.val = 99},
            {.val = -55},
            {.val = 5},
            {.val = 2},
        });
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 8,
                      (int[8]){9, 4, 1, 1, 99, -55, 5, 2}),
          CHECK_PASS);
    check(doubly_linked_list_is_sorted(&doubly_linked_list), false);
    CCC_Result const r = CCC_doubly_linked_list_sort(&doubly_linked_list);
    check(doubly_linked_list_is_sorted(&doubly_linked_list), true);
    check(r, CCC_RESULT_OK);
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 8,
                      (int[8]){-55, 1, 1, 2, 4, 5, 9, 99}),
          CHECK_PASS);
    check_end();
}

check_static_begin(doubly_linked_list_test_sort_odd)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 9);
    Doubly_linked_list doubly_linked_list = doubly_linked_list_from(
        e, val_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[9]){
            {.val = 9},
            {.val = 4},
            {.val = 1},
            {.val = 1},
            {.val = 99},
            {.val = -55},
            {.val = 5},
            {.val = 2},
            {.val = -99},
        });
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 9,
                      (int[9]){9, 4, 1, 1, 99, -55, 5, 2, -99}),
          CHECK_PASS);
    check(doubly_linked_list_is_sorted(&doubly_linked_list), false);
    CCC_Result const r = CCC_doubly_linked_list_sort(&doubly_linked_list);
    check(doubly_linked_list_is_sorted(&doubly_linked_list), true);
    check(r, CCC_RESULT_OK);
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 9,
                      (int[9]){-99, -55, 1, 1, 2, 4, 5, 9, 99}),
          CHECK_PASS);
    check_end();
}

check_static_begin(doubly_linked_list_test_sort_reverse)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 8);
    Doubly_linked_list doubly_linked_list = doubly_linked_list_from(
        e, val_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[8]){
            {.val = 9},
            {.val = 8},
            {.val = 7},
            {.val = 6},
            {.val = 5},
            {.val = 4},
            {.val = 3},
            {.val = 2},
        });
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 8, (int[8]){9, 8, 7, 6, 5, 4, 3, 2}),
          CHECK_PASS);
    check(doubly_linked_list_is_sorted(&doubly_linked_list), false);
    CCC_Result const r = CCC_doubly_linked_list_sort(&doubly_linked_list);
    check(doubly_linked_list_is_sorted(&doubly_linked_list), true);
    check(r, CCC_RESULT_OK);
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 8, (int[8]){2, 3, 4, 5, 6, 7, 8, 9}),
          CHECK_PASS);
    check_end();
}

check_static_begin(doubly_linked_list_test_sort_runs)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 12);
    Doubly_linked_list doubly_linked_list = doubly_linked_list_from(
        e, val_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[12]){
            {.val = 99},
            {.val = 101},
            {.val = 103},
            {.val = 4},
            {.val = 8},
            {.val = 9},
            {.val = -99},
            {.val = -55},
            {.val = -55},
            {.val = 3},
            {.val = 7},
            {.val = 10},
        });
    check(validate(&doubly_linked_list), true);
    enum Check_result t = check_order(
        &doubly_linked_list, 12,
        (int[12]){99, 101, 103, 4, 8, 9, -99, -55, -55, 3, 7, 10});
    check(t, CHECK_PASS);
    check(doubly_linked_list_is_sorted(&doubly_linked_list), false);
    CCC_Result const r = CCC_doubly_linked_list_sort(&doubly_linked_list);
    check(doubly_linked_list_is_sorted(&doubly_linked_list), true);
    check(r, CCC_RESULT_OK);
    check(validate(&doubly_linked_list), true);
    t = check_order(&doubly_linked_list, 12,
                    (int[12]){-99, -55, -55, 3, 4, 7, 8, 9, 10, 99, 101, 103});
    check(t, CHECK_PASS);
    check_end();
}

check_static_begin(doubly_linked_list_test_sort_halves)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 12);
    Doubly_linked_list doubly_linked_list = doubly_linked_list_from(
        e, val_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[12]){
            {.val = 25},
            {.val = 20},
            {.val = 18},
            {.val = 15},
            {.val = 12},
            {.val = 8},
            {.val = 21},
            {.val = 19},
            {.val = 17},
            {.val = 13},
            {.val = 10},
            {.val = 7},
        });
    check(validate(&doubly_linked_list), true);
    enum Check_result t
        = check_order(&doubly_linked_list, 12,
                      (int[12]){25, 20, 18, 15, 12, 8, 21, 19, 17, 13, 10, 7});
    check(t, CHECK_PASS);
    check(doubly_linked_list_is_sorted(&doubly_linked_list), false);
    CCC_Result const r = CCC_doubly_linked_list_sort(&doubly_linked_list);
    check(doubly_linked_list_is_sorted(&doubly_linked_list), true);
    check(r, CCC_RESULT_OK);
    t = check_order(&doubly_linked_list, 12,
                    (int[12]){7, 8, 10, 12, 13, 15, 17, 18, 19, 20, 21, 25});
    check(validate(&doubly_linked_list), true);
    check(t, CHECK_PASS);
    check_end();
}

check_static_begin(doubly_linked_list_test_sort_insert)
{
    Doubly_linked_list doubly_linked_list
        = doubly_linked_list_initialize(struct Val, e, val_order, NULL, NULL);
    struct Val *inserted = doubly_linked_list_insert_sorted(
        &doubly_linked_list, &(struct Val){.val = -99999}.e);
    check(inserted->val, -99999);
    check(validate(&doubly_linked_list), true);
    (void)CCC_doubly_linked_list_pop_front(&doubly_linked_list);
    check(validate(&doubly_linked_list), true);
    struct Val vals[9] = {
        [0] = {.val = 9}, [1] = {.val = 4},  [2] = {.val = 1},
        [3] = {.val = 1}, [4] = {.val = 99}, [5] = {.val = -55},
        [6] = {.val = 5}, [7] = {.val = 2},  [8] = {.val = -99},
    };
    enum Check_result const t
        = push_list(&doubly_linked_list, UTIL_PUSH_BACK, 9, vals);
    check(t, CHECK_PASS);
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 9,
                      (int[9]){9, 4, 1, 1, 99, -55, 5, 2, -99}),
          CHECK_PASS);
    check(doubly_linked_list_is_sorted(&doubly_linked_list), false);
    CCC_Result const r = doubly_linked_list_sort(&doubly_linked_list);
    check(doubly_linked_list_is_sorted(&doubly_linked_list), true);
    check(r, CCC_RESULT_OK);
    check(validate(&doubly_linked_list), true);
    check(check_order(&doubly_linked_list, 9,
                      (int[9]){-99, -55, 1, 1, 2, 4, 5, 9, 99}),
          CHECK_PASS);
    struct Val to_insert[5] = {
        [0] = {.val = -101}, [1] = {.val = -65}, [2] = {.val = 3},
        [3] = {.val = 20},   [4] = {.val = 101},
    };
    /* Before -99. */
    inserted = doubly_linked_list_insert_sorted(&doubly_linked_list,
                                                &to_insert[0].e);
    check(inserted != NULL, true);
    check(validate(&doubly_linked_list), true);
    check(doubly_linked_list_reverse_next(&doubly_linked_list, &inserted->e),
          doubly_linked_list_reverse_end(&doubly_linked_list));
    check(doubly_linked_list_next(&doubly_linked_list, &inserted->e), &vals[8]);

    /* After -99. */
    inserted = doubly_linked_list_insert_sorted(&doubly_linked_list,
                                                &to_insert[1].e);
    check(inserted != NULL, true);
    check(validate(&doubly_linked_list), true);
    check(doubly_linked_list_reverse_next(&doubly_linked_list, &inserted->e),
          &vals[8]);
    check(doubly_linked_list_next(&doubly_linked_list, &inserted->e), &vals[5]);

    /* Before 4. */
    inserted = doubly_linked_list_insert_sorted(&doubly_linked_list,
                                                &to_insert[2].e);
    check(inserted != NULL, true);
    check(validate(&doubly_linked_list), true);
    check(doubly_linked_list_reverse_next(&doubly_linked_list, &inserted->e),
          &vals[7]);
    check(doubly_linked_list_next(&doubly_linked_list, &inserted->e), &vals[1]);

    /* Before 99. */
    inserted = doubly_linked_list_insert_sorted(&doubly_linked_list,
                                                &to_insert[3].e);
    check(inserted != NULL, true);
    check(validate(&doubly_linked_list), true);
    check(doubly_linked_list_reverse_next(&doubly_linked_list, &inserted->e),
          &vals[0]);
    check(doubly_linked_list_next(&doubly_linked_list, &inserted->e), &vals[4]);

    /* After 99. */
    inserted = doubly_linked_list_insert_sorted(&doubly_linked_list,
                                                &to_insert[4].e);
    check(inserted != NULL, true);
    check(validate(&doubly_linked_list), true);
    check(doubly_linked_list_reverse_next(&doubly_linked_list, &inserted->e),
          &vals[4]);
    check(doubly_linked_list_next(&doubly_linked_list, &inserted->e),
          doubly_linked_list_end(&doubly_linked_list));

    check_end();
}

int
main()
{
    return check_run(doubly_linked_list_test_push_three_front(),
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
