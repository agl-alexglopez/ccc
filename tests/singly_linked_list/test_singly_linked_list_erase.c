#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "singly_linked_list.h"
#include "singly_linked_list_util.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_pop_empty)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    CHECK(is_empty(&singly_linked_list), true);
    CHECK(singly_linked_list_pop_front(&singly_linked_list),
          CCC_RESULT_ARGUMENT_ERROR);
    CHECK(singly_linked_list_validate(&singly_linked_list), true);
    CHECK(singly_linked_list_front(&singly_linked_list), NULL);
    CHECK(is_empty(&singly_linked_list), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_push_pop_three)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val vals[3] = {{.val = 0}, {.val = 1}, {.val = 2}};
    enum check_result const t = create_list(&singly_linked_list, 3, vals);
    CHECK(t, PASS);
    size_t const end = count(&singly_linked_list).count;
    for (size_t i = 0; i < end; ++i)
    {
        (void)pop_front(&singly_linked_list);
        CHECK(validate(&singly_linked_list), true);
    }
    CHECK(is_empty(&singly_linked_list), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_push_extract_middle)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val vals[3] = {{.val = 0}, {.val = 1}, {.val = 2}};
    enum check_result const t = create_list(&singly_linked_list, 3, vals);
    CHECK(t, PASS);
    CHECK(check_order(&singly_linked_list, 3, (int[3]){2, 1, 0}), PASS);
    struct val *after_extract = extract(&singly_linked_list, &vals[1].e);
    CHECK(validate(&singly_linked_list), true);
    CHECK(after_extract == NULL, false);
    CHECK(after_extract->val, 0);
    CHECK(check_order(&singly_linked_list, 2, (int[2]){2, 0}), PASS);
    after_extract = extract(&singly_linked_list, &vals[0].e);
    CHECK(after_extract, end(&singly_linked_list));
    CHECK(check_order(&singly_linked_list, 1, (int[1]){2}), PASS);
    CHECK(count(&singly_linked_list).count, 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_push_extract_range)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct val, e, val_cmp, NULL, NULL);
    struct val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum check_result const t = create_list(&singly_linked_list, 5, vals);
    CHECK(t, PASS);
    CHECK(check_order(&singly_linked_list, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    struct val *after_extract
        = extract_range(&singly_linked_list, &vals[3].e, &vals[1].e);
    CHECK(count(&singly_linked_list).count, 2);
    CHECK(validate(&singly_linked_list), true);
    CHECK(after_extract == NULL, false);
    CHECK(after_extract->val, 0);
    CHECK(check_order(&singly_linked_list, 2, (int[2]){4, 0}), PASS);
    after_extract = extract_range(
        &singly_linked_list, singly_linked_list_begin_node(&singly_linked_list),
        &vals[0].e);
    CHECK(after_extract, end(&singly_linked_list));
    CHECK(is_empty(&singly_linked_list), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_splice_two_lists)
{
    Singly_linked_list to_lose = singly_linked_list_initialize(
        to_lose, struct val, e, val_cmp, NULL, NULL);
    struct val to_lose_vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum check_result t = create_list(&to_lose, 5, to_lose_vals);
    CHECK(t, PASS);
    Singly_linked_list to_gain = singly_linked_list_initialize(
        to_gain, struct val, e, val_cmp, NULL, NULL);
    struct val to_gain_vals[2] = {{.val = 0}, {.val = 1}};
    t = create_list(&to_gain, 2, to_gain_vals);
    CHECK(t, PASS);
    CHECK(check_order(&to_lose, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(check_order(&to_gain, 2, (int[2]){1, 0}), PASS);
    CHECK(splice(&to_gain, singly_linked_list_begin_node(&to_gain), &to_lose,
                 singly_linked_list_begin_node(&to_lose)),
          CCC_RESULT_OK);
    CHECK(count(&to_gain).count, 3);
    CHECK(count(&to_lose).count, 4);
    CHECK(check_order(&to_lose, 4, (int[4]){3, 2, 1, 0}), PASS);
    CHECK(check_order(&to_gain, 3, (int[3]){1, 4, 0}), PASS);
    CHECK(splice_range(&to_gain, singly_linked_list_begin_node(&to_gain),
                       &to_lose, singly_linked_list_begin_node(&to_lose),
                       &to_lose_vals[0].e),
          CCC_RESULT_OK);
    CHECK(count(&to_gain).count, 7);
    CHECK(is_empty(&to_lose), true);
    CHECK(check_order(&to_gain, 7, (int[7]){1, 3, 2, 1, 0, 4, 0}), PASS);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(singly_linked_list_test_pop_empty(),
                     singly_linked_list_test_push_pop_three(),
                     singly_linked_list_test_push_extract_middle(),
                     singly_linked_list_test_push_extract_range(),
                     singly_linked_list_test_splice_two_lists());
}
