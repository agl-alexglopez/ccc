#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "singly_linked_list.h"
#include "singly_linked_list_utility.h"
#include "traits.h"
#include "types.h"

check_static_begin(singly_linked_list_test_pop_empty)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct Val, e, val_order, NULL, NULL);
    check(is_empty(&singly_linked_list), true);
    check(singly_linked_list_pop_front(&singly_linked_list),
          CCC_RESULT_ARGUMENT_ERROR);
    check(singly_linked_list_validate(&singly_linked_list), true);
    check(singly_linked_list_front(&singly_linked_list), NULL);
    check(is_empty(&singly_linked_list), true);
    check_end();
}

check_static_begin(singly_linked_list_test_push_pop_three)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[3] = {{.val = 0}, {.val = 1}, {.val = 2}};
    enum Check_result const t = create_list(&singly_linked_list, 3, vals);
    check(t, CHECK_PASS);
    size_t const end = count(&singly_linked_list).count;
    for (size_t i = 0; i < end; ++i)
    {
        (void)pop_front(&singly_linked_list);
        check(validate(&singly_linked_list), true);
    }
    check(is_empty(&singly_linked_list), true);
    check_end();
}

check_static_begin(singly_linked_list_test_push_extract_middle)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[3] = {{.val = 0}, {.val = 1}, {.val = 2}};
    enum Check_result const t = create_list(&singly_linked_list, 3, vals);
    check(t, CHECK_PASS);
    check(check_order(&singly_linked_list, 3, (int[3]){2, 1, 0}), CHECK_PASS);
    struct Val *after_extract = extract(&singly_linked_list, &vals[1].e);
    check(validate(&singly_linked_list), true);
    check(after_extract == NULL, false);
    check(after_extract->val, 0);
    check(check_order(&singly_linked_list, 2, (int[2]){2, 0}), CHECK_PASS);
    after_extract = extract(&singly_linked_list, &vals[0].e);
    check(after_extract, end(&singly_linked_list));
    check(check_order(&singly_linked_list, 1, (int[1]){2}), CHECK_PASS);
    check(count(&singly_linked_list).count, 1);
    check_end();
}

check_static_begin(singly_linked_list_test_push_extract_range)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum Check_result const t = create_list(&singly_linked_list, 5, vals);
    check(t, CHECK_PASS);
    check(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}),
          CHECK_PASS);
    struct Val *after_extract
        = extract_range(&singly_linked_list, &vals[3].e, &vals[1].e);
    check(count(&singly_linked_list).count, 2);
    check(validate(&singly_linked_list), true);
    check(after_extract == NULL, false);
    check(after_extract->val, 0);
    check(check_order(&singly_linked_list, 2, (int[2]){4, 0}), CHECK_PASS);
    after_extract = extract_range(
        &singly_linked_list, singly_linked_list_node_begin(&singly_linked_list),
        &vals[0].e);
    check(after_extract, end(&singly_linked_list));
    check(is_empty(&singly_linked_list), true);
    check_end();
}

check_static_begin(singly_linked_list_test_splice_two_lists)
{
    Singly_linked_list to_lose = singly_linked_list_initialize(
        to_lose, struct Val, e, val_order, NULL, NULL);
    struct Val to_lose_vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum Check_result t = create_list(&to_lose, 5, to_lose_vals);
    check(t, CHECK_PASS);
    Singly_linked_list to_gain = singly_linked_list_initialize(
        to_gain, struct Val, e, val_order, NULL, NULL);
    struct Val to_gain_vals[2] = {{.val = 0}, {.val = 1}};
    t = create_list(&to_gain, 2, to_gain_vals);
    check(t, CHECK_PASS);
    check(check_order(&to_lose, 5, (int[5]){4, 3, 2, 1, 0}), CHECK_PASS);
    check(check_order(&to_gain, 2, (int[2]){1, 0}), CHECK_PASS);
    check(splice(&to_gain, singly_linked_list_node_begin(&to_gain), &to_lose,
                 singly_linked_list_node_begin(&to_lose)),
          CCC_RESULT_OK);
    check(count(&to_gain).count, 3);
    check(count(&to_lose).count, 4);
    check(check_order(&to_lose, 4, (int[4]){3, 2, 1, 0}), CHECK_PASS);
    check(check_order(&to_gain, 3, (int[3]){1, 4, 0}), CHECK_PASS);
    check(splice_range(&to_gain, singly_linked_list_node_begin(&to_gain),
                       &to_lose, singly_linked_list_node_begin(&to_lose),
                       &to_lose_vals[0].e),
          CCC_RESULT_OK);
    check(count(&to_gain).count, 7);
    check(is_empty(&to_lose), true);
    check(check_order(&to_gain, 7, (int[7]){1, 3, 2, 1, 0, 4, 0}), CHECK_PASS);
    check_end();
}

int
main()
{
    return check_run(singly_linked_list_test_pop_empty(),
                     singly_linked_list_test_push_pop_three(),
                     singly_linked_list_test_push_extract_middle(),
                     singly_linked_list_test_push_extract_range(),
                     singly_linked_list_test_splice_two_lists());
}
