#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "singly_linked_list.h"
#include "singly_linked_list_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/stack_allocator.h"

check_static_begin(singly_linked_list_test_insert_three)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 3);
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        struct Val, e, val_order, stack_allocator_allocate, &allocator);
    check(push_front(&singly_linked_list, &(struct Val){}.e) != NULL, true);
    struct Val *v = front(&singly_linked_list);
    check(validate(&singly_linked_list), true);
    check(v == NULL, false);
    check(v->val, 0);
    check(push_front(&singly_linked_list, &(struct Val){.val = 1}.e) != NULL,
          true);
    check(validate(&singly_linked_list), true);
    v = front(&singly_linked_list);
    check(v == NULL, false);
    check(v->val, 1);
    check(push_front(&singly_linked_list, &(struct Val){.val = 2}.e) != NULL,
          true);
    check(validate(&singly_linked_list), true);
    v = front(&singly_linked_list);
    check(v == NULL, false);
    check(v->val, 2);
    check(count(&singly_linked_list).count, 3);
    check(check_order(&singly_linked_list, 3, (int[3]){2, 1, 0}), CHECK_PASS);
    check_end();
}

check_static_begin(singly_linked_list_test_push_and_splice)
{
    Singly_linked_list singly_linked_list
        = singly_linked_list_initialize(struct Val, e, val_order, NULL, NULL);
    struct Val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum Check_result const t = push_list(&singly_linked_list, 4, vals);
    check(t, CHECK_PASS);
    check(check_order(&singly_linked_list, 4, (int[4]){3, 2, 1, 0}),
          CHECK_PASS);
    check(splice(&singly_linked_list,
                 singly_linked_list_node_begin(&singly_linked_list),
                 &singly_linked_list, &vals[0].e),
          CCC_RESULT_OK);
    check(validate(&singly_linked_list), true);
    check(check_order(&singly_linked_list, 4, (int[4]){3, 0, 2, 1}),
          CHECK_PASS);
    check(splice(&singly_linked_list, &vals[0].e, &singly_linked_list,
                 &vals[3].e),
          CCC_RESULT_OK);
    check(validate(&singly_linked_list), true);
    check(check_order(&singly_linked_list, 4, (int[4]){0, 3, 2, 1}),
          CHECK_PASS);
    check(splice(&singly_linked_list, &vals[1].e, &singly_linked_list,
                 &vals[0].e),
          CCC_RESULT_OK);
    check(validate(&singly_linked_list), true);
    check(check_order(&singly_linked_list, 4, (int[4]){3, 2, 1, 0}),
          CHECK_PASS);
    check_end();
}

check_static_begin(singly_linked_list_test_push_and_splice_range)
{
    Singly_linked_list singly_linked_list
        = singly_linked_list_initialize(struct Val, e, val_order, NULL, NULL);
    struct Val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum Check_result const t = push_list(&singly_linked_list, 5, vals);
    check(t, CHECK_PASS);
    check(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}),
          CHECK_PASS);
    check(
        splice_range(&singly_linked_list,
                     singly_linked_list_node_before_begin(&singly_linked_list),
                     &singly_linked_list, &vals[2].e, &vals[0].e),
        CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 5, (int[5]){2, 1, 4, 3, 0}),
          CHECK_PASS);
    check(splice_range(&singly_linked_list, &vals[3].e, &singly_linked_list,
                       &vals[2].e, &vals[4].e),
          CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}),
          CHECK_PASS);
    check(
        splice_range(&singly_linked_list,
                     singly_linked_list_node_before_begin(&singly_linked_list),
                     &singly_linked_list, &vals[3].e, &vals[1].e),
        CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 5, (int[5]){3, 2, 4, 1, 0}),
          CHECK_PASS);
    check(splice_range(&singly_linked_list, &vals[0].e, &singly_linked_list,
                       &vals[2].e, &vals[4].e),
          CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 5, (int[5]){3, 4, 1, 0, 2}),
          CHECK_PASS);
    check(splice_range(&singly_linked_list, &vals[4].e, &singly_linked_list,
                       &vals[0].e, singly_linked_list_end(&singly_linked_list)),
          CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 5, (int[5]){3, 4, 0, 2, 1}),
          CHECK_PASS);
    check(splice_range(&singly_linked_list, &vals[3].e, &singly_linked_list,
                       &vals[0].e, &vals[1].e),
          CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 5, (int[5]){3, 0, 2, 4, 1}),
          CHECK_PASS);
    check_end();
}

check_static_begin(singly_linked_list_test_push_and_splice_range_no_ops)
{
    Singly_linked_list singly_linked_list
        = singly_linked_list_initialize(struct Val, e, val_order, NULL, NULL);
    struct Val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum Check_result const t = push_list(&singly_linked_list, 5, vals);
    check(t, CHECK_PASS);
    check(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}),
          CHECK_PASS);
    check(splice(&singly_linked_list, &vals[2].e, &singly_linked_list,
                 &vals[2].e),
          CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}),
          CHECK_PASS);
    check(splice(&singly_linked_list, &vals[3].e, &singly_linked_list,
                 &vals[2].e),
          CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}),
          CHECK_PASS);
    check(splice_range(&singly_linked_list, &vals[3].e, &singly_linked_list,
                       &vals[2].e, &vals[0].e),
          CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}),
          CHECK_PASS);
    check(splice_range(&singly_linked_list, &vals[3].e, &singly_linked_list,
                       &vals[3].e, &vals[0].e),
          CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}),
          CHECK_PASS);
    check_end();
}

check_static_begin(singly_linked_list_test_sort_reverse)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 6);
    Singly_linked_list singly_linked_list = singly_linked_list_from(
        e, val_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[6]){
            {.val = 5},
            {.val = 4},
            {.val = 3},
            {.val = 2},
            {.val = 1},
            {.val = 0},
        });
    check(check_order(&singly_linked_list, 6, (int[6]){5, 4, 3, 2, 1, 0}),
          CHECK_PASS);
    check(validate(&singly_linked_list), true);
    check(singly_linked_list_is_sorted(&singly_linked_list), false);
    CCC_Result const r = CCC_singly_linked_list_sort(&singly_linked_list);
    check(singly_linked_list_is_sorted(&singly_linked_list), true);
    check(r, CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 6, (int[6]){0, 1, 2, 3, 4, 5}),
          CHECK_PASS);
    check_end();
}

check_static_begin(singly_linked_list_test_sort_even)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 8);
    Singly_linked_list singly_linked_list = singly_linked_list_from(
        e, val_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[8]){
            {.val = 9},
            {.val = 4},
            {.val = 1},
            {.val = 3},
            {.val = 99},
            {.val = -55},
            {.val = 5},
            {.val = 2},
        });
    check(validate(&singly_linked_list), true);
    check(check_order(&singly_linked_list, 8,
                      (int[8]){9, 4, 1, 3, 99, -55, 5, 2}),
          CHECK_PASS);
    check(singly_linked_list_is_sorted(&singly_linked_list), false);
    CCC_Result const r = CCC_singly_linked_list_sort(&singly_linked_list);
    check(singly_linked_list_is_sorted(&singly_linked_list), true);
    check(r, CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 8,
                      (int[8]){-55, 1, 2, 3, 4, 5, 9, 99}),
          CHECK_PASS);
    check(validate(&singly_linked_list), true);
    check_end();
}

check_static_begin(singly_linked_list_test_sort_odd)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 9);
    Singly_linked_list singly_linked_list = singly_linked_list_from(
        e, val_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[9]){
            {.val = 10},
            {.val = 9},
            {.val = 4},
            {.val = 1},
            {.val = 1},
            {.val = 99},
            {.val = -55},
            {.val = 5},
            {.val = 2},
        });
    check(validate(&singly_linked_list), true);
    check(check_order(&singly_linked_list, 9,
                      (int[9]){10, 9, 4, 1, 1, 99, -55, 5, 2}),
          CHECK_PASS);
    check(singly_linked_list_is_sorted(&singly_linked_list), false);
    CCC_Result const r = CCC_singly_linked_list_sort(&singly_linked_list);
    check(singly_linked_list_is_sorted(&singly_linked_list), true);
    check(r, CCC_RESULT_OK);
    check(check_order(&singly_linked_list, 9,
                      (int[9]){-55, 1, 1, 2, 4, 5, 9, 10, 99}),
          CHECK_PASS);
    check(validate(&singly_linked_list), true);
    check_end();
}

check_static_begin(singly_linked_list_test_sort_runs)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 12);
    Singly_linked_list singly_linked_list = singly_linked_list_from(
        e, val_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[12]){
            {.val = 10},
            {.val = 7},
            {.val = 3},
            {.val = -55},
            {.val = -55},
            {.val = -99},
            {.val = 9},
            {.val = 8},
            {.val = 4},
            {.val = 103},
            {.val = 101},
            {.val = 99},
        });
    check(validate(&singly_linked_list), true);
    enum Check_result t = check_order(
        &singly_linked_list, 12,
        (int[12]){10, 7, 3, -55, -55, -99, 9, 8, 4, 103, 101, 99});
    check(t, CHECK_PASS);
    check(singly_linked_list_is_sorted(&singly_linked_list), false);
    CCC_Result const r = CCC_singly_linked_list_sort(&singly_linked_list);
    check(singly_linked_list_is_sorted(&singly_linked_list), true);
    check(r, CCC_RESULT_OK);
    t = check_order(&singly_linked_list, 12,
                    (int[12]){-99, -55, -55, 3, 4, 7, 8, 9, 10, 99, 101, 103});
    check(validate(&singly_linked_list), true);
    check(t, CHECK_PASS);
    check_end();
}

check_static_begin(singly_linked_list_test_sort_halves)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 12);
    Singly_linked_list singly_linked_list = singly_linked_list_from(
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
    check(validate(&singly_linked_list), true);
    enum Check_result t
        = check_order(&singly_linked_list, 12,
                      (int[12]){25, 20, 18, 15, 12, 8, 21, 19, 17, 13, 10, 7});
    check(t, CHECK_PASS);
    check(singly_linked_list_is_sorted(&singly_linked_list), false);
    CCC_Result const r = CCC_singly_linked_list_sort(&singly_linked_list);
    check(singly_linked_list_is_sorted(&singly_linked_list), true);
    check(r, CCC_RESULT_OK);
    t = check_order(&singly_linked_list, 12,
                    (int[12]){7, 8, 10, 12, 13, 15, 17, 18, 19, 20, 21, 25});
    check(validate(&singly_linked_list), true);
    check(t, CHECK_PASS);
    check_end();
}

check_static_begin(singly_linked_list_test_sort_insert)
{
    Singly_linked_list singly_linked_list
        = singly_linked_list_initialize(struct Val, e, val_order, NULL, NULL);
    struct Val *inserted = singly_linked_list_insert_sorted(
        &singly_linked_list, &(struct Val){.val = -99999}.e);
    check(inserted->val, -99999);
    check(validate(&singly_linked_list), true);
    (void)CCC_singly_linked_list_pop_front(&singly_linked_list);
    check(validate(&singly_linked_list), true);
    struct Val vals[9] = {
        [8] = {.val = 9}, [7] = {.val = 4},  [6] = {.val = 1},
        [5] = {.val = 1}, [4] = {.val = 99}, [3] = {.val = -55},
        [2] = {.val = 5}, [1] = {.val = 2},  [0] = {.val = -99},
    };
    enum Check_result const t = push_list(&singly_linked_list, 9, vals);
    check(t, CHECK_PASS);
    check(validate(&singly_linked_list), true);
    check(check_order(&singly_linked_list, 9,
                      (int[9]){9, 4, 1, 1, 99, -55, 5, 2, -99}),
          CHECK_PASS);
    check(singly_linked_list_is_sorted(&singly_linked_list), false);
    CCC_Result const r = singly_linked_list_sort(&singly_linked_list);
    check(singly_linked_list_is_sorted(&singly_linked_list), true);
    check(r, CCC_RESULT_OK);
    check(validate(&singly_linked_list), true);
    check(check_order(&singly_linked_list, 9,
                      (int[9]){-99, -55, 1, 1, 2, 4, 5, 9, 99}),
          CHECK_PASS);
    struct Val to_insert[5] = {
        [0] = {.val = -101}, [1] = {.val = -65}, [2] = {.val = 3},
        [3] = {.val = 20},   [4] = {.val = 101},
    };
    /* Before -99. */
    inserted = singly_linked_list_insert_sorted(&singly_linked_list,
                                                &to_insert[0].e);
    check(inserted != NULL, true);
    check(validate(&singly_linked_list), true);
    check(singly_linked_list_next(&singly_linked_list, &inserted->e), &vals[0]);

    /* After -99. */
    inserted = singly_linked_list_insert_sorted(&singly_linked_list,
                                                &to_insert[1].e);
    check(inserted != NULL, true);
    check(validate(&singly_linked_list), true);
    check(singly_linked_list_next(&singly_linked_list, &inserted->e), &vals[3]);

    /* Before 4. */
    inserted = singly_linked_list_insert_sorted(&singly_linked_list,
                                                &to_insert[2].e);
    check(inserted != NULL, true);
    check(validate(&singly_linked_list), true);
    check(singly_linked_list_next(&singly_linked_list, &inserted->e), &vals[7]);

    /* Before 99. */
    inserted = singly_linked_list_insert_sorted(&singly_linked_list,
                                                &to_insert[3].e);
    check(inserted != NULL, true);
    check(validate(&singly_linked_list), true);
    check(singly_linked_list_next(&singly_linked_list, &inserted->e), &vals[4]);

    /* After 99. */
    inserted = singly_linked_list_insert_sorted(&singly_linked_list,
                                                &to_insert[4].e);
    check(inserted != NULL, true);
    check(validate(&singly_linked_list), true);
    check(singly_linked_list_next(&singly_linked_list, &inserted->e),
          singly_linked_list_end(&singly_linked_list));

    check_end();
}
int
main()
{
    return check_run(singly_linked_list_test_insert_three(),
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
