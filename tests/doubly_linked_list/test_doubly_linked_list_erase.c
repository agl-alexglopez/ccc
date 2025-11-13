#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "doubly_linked_list.h"
#include "doubly_linked_list_utility.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_pop_empty)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    CHECK(is_empty(&doubly_linked_list), true);
    CHECK(doubly_linked_list_pop_front(&doubly_linked_list),
          CCC_RESULT_ARGUMENT_ERROR);
    CHECK(doubly_linked_list_validate(&doubly_linked_list), true);
    CHECK(doubly_linked_list_pop_back(&doubly_linked_list),
          CCC_RESULT_ARGUMENT_ERROR);
    CHECK(doubly_linked_list_validate(&doubly_linked_list), true);
    CHECK(doubly_linked_list_front(&doubly_linked_list), NULL);
    CHECK(doubly_linked_list_back(&doubly_linked_list), NULL);
    CHECK(is_empty(&doubly_linked_list), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_push_pop_front)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[3] = {{.val = 0}, {.val = 1}, {.val = 2}};
    enum Check_result const t
        = create_list(&doubly_linked_list, UTIL_PUSH_BACK, 3, vals);
    CHECK(t, PASS);
    CHECK(count(&doubly_linked_list).count, 3);
    struct Val *v = doubly_linked_list_front(&doubly_linked_list);
    CHECK(v == NULL, false);
    CHECK(v->val, 0);
    CHECK(pop_front(&doubly_linked_list), CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    v = doubly_linked_list_front(&doubly_linked_list);
    CHECK(v == NULL, false);
    CHECK(v->val, 1);
    CHECK(pop_front(&doubly_linked_list), CCC_RESULT_OK);
    v = doubly_linked_list_front(&doubly_linked_list);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(v == NULL, false);
    CHECK(v->val, 2);
    CHECK(pop_front(&doubly_linked_list), CCC_RESULT_OK);
    CHECK(is_empty(&doubly_linked_list), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_push_pop_back)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[3] = {{.val = 0}, {.val = 1}, {.val = 2}};
    enum Check_result const t
        = create_list(&doubly_linked_list, UTIL_PUSH_BACK, 3, vals);
    CHECK(t, PASS);
    CHECK(count(&doubly_linked_list).count, 3);
    struct Val *v = doubly_linked_list_back(&doubly_linked_list);
    CHECK(v == NULL, false);
    CHECK(v->val, 2);
    CHECK(pop_back(&doubly_linked_list), CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    v = doubly_linked_list_back(&doubly_linked_list);
    CHECK(v == NULL, false);
    CHECK(v->val, 1);
    CHECK(pop_back(&doubly_linked_list), CCC_RESULT_OK);
    CHECK(validate(&doubly_linked_list), true);
    v = doubly_linked_list_back(&doubly_linked_list);
    CHECK(v == NULL, false);
    CHECK(v->val, 0);
    CHECK(pop_back(&doubly_linked_list), CCC_RESULT_OK);
    CHECK(is_empty(&doubly_linked_list), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_push_pop_middle)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum Check_result const t
        = create_list(&doubly_linked_list, UTIL_PUSH_BACK, 4, vals);
    CHECK(t, PASS);
    CHECK(extract(&doubly_linked_list, &vals[2].e) != NULL, true);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 3, (int[3]){0, 1, 3}), PASS);
    (void)extract(&doubly_linked_list, &vals[1].e);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 2, (int[2]){0, 3}), PASS);
    (void)extract(&doubly_linked_list, &vals[3].e);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(check_order(&doubly_linked_list, 1, (int[1]){0}), PASS);
    (void)extract(&doubly_linked_list, &vals[0].e);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(is_empty(&doubly_linked_list), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_push_pop_middle_range)
{
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    struct Val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum Check_result const t
        = create_list(&doubly_linked_list, UTIL_PUSH_BACK, 5, vals);
    CHECK(t, PASS);
    (void)doubly_linked_list_extract_range(&doubly_linked_list, &vals[1].e,
                                           &vals[4].e);
    CHECK(validate(&doubly_linked_list), true);
    CHECK(count(&doubly_linked_list).count, 2);
    CHECK(check_order(&doubly_linked_list, 2, (int[2]){0, 4}), PASS);
    (void)doubly_linked_list_extract_range(
        &doubly_linked_list, &vals[0].e,
        doubly_linked_list_sentinel_end(&doubly_linked_list));
    CHECK(validate(&doubly_linked_list), true);
    CHECK(count(&doubly_linked_list).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_splice_two_lists)
{
    Doubly_linked_list to_lose = doubly_linked_list_initialize(
        to_lose, struct Val, e, val_order, NULL, NULL);
    struct Val to_lose_vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum Check_result t
        = create_list(&to_lose, UTIL_PUSH_BACK, 5, to_lose_vals);
    CHECK(t, PASS);
    Doubly_linked_list to_gain = doubly_linked_list_initialize(
        to_gain, struct Val, e, val_order, NULL, NULL);
    struct Val to_gain_vals[2] = {{.val = 0}, {.val = 1}};
    t = create_list(&to_gain, UTIL_PUSH_BACK, 2, to_gain_vals);
    CHECK(t, PASS);
    CHECK(check_order(&to_lose, 5, (int[5]){0, 1, 2, 3, 4}), PASS);
    CHECK(doubly_linked_list_splice(&to_gain,
                                    doubly_linked_list_sentinel_end(&to_gain),
                                    &to_lose, &to_lose_vals[0].e),
          CCC_RESULT_OK);
    CHECK(validate(&to_gain), true);
    CHECK(validate(&to_lose), true);
    CHECK(count(&to_gain).count, 3);
    CHECK(count(&to_lose).count, 4);
    CHECK(check_order(&to_gain, 3, (int[3]){0, 1, 0}), PASS);
    CHECK(check_order(&to_lose, 4, (int[4]){1, 2, 3, 4}), PASS);
    CHECK(doubly_linked_list_splice_range(
              &to_gain, doubly_linked_list_node_end(&to_gain), &to_lose,
              doubly_linked_list_node_begin(&to_lose),
              doubly_linked_list_sentinel_end(&to_lose)),
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
    return CHECK_RUN(doubly_linked_list_test_pop_empty(),
                     doubly_linked_list_test_push_pop_front(),
                     doubly_linked_list_test_push_pop_back(),
                     doubly_linked_list_test_push_pop_middle(),
                     doubly_linked_list_test_push_pop_middle_range(),
                     doubly_linked_list_test_splice_two_lists());
}
