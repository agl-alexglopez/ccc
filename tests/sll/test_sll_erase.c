#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "singly_linked_list.h"
#include "sll_util.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>

CHECK_BEGIN_STATIC_FN(sll_test_push_pop_three)
{
    singly_linked_list sll = sll_init(sll, struct val, e, NULL, val_cmp, NULL);
    struct val vals[3] = {{.val = 0}, {.val = 1}, {.val = 2}};
    enum check_result const t = create_list(&sll, 3, vals);
    CHECK(t, PASS);
    size_t const end = size(&sll);
    for (size_t i = 0; i < end; ++i)
    {
        (void)pop_front(&sll);
        CHECK(validate(&sll), true);
    }
    CHECK(is_empty(&sll), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(sll_test_push_extract_middle)
{
    singly_linked_list sll = sll_init(sll, struct val, e, NULL, val_cmp, NULL);
    struct val vals[3] = {{.val = 0}, {.val = 1}, {.val = 2}};
    enum check_result const t = create_list(&sll, 3, vals);
    CHECK(t, PASS);
    CHECK(check_order(&sll, 3, (int[3]){2, 1, 0}), PASS);
    struct val *after_extract = extract(&sll, &vals[1].e);
    CHECK(validate(&sll), true);
    CHECK(after_extract == NULL, false);
    CHECK(after_extract->val, 0);
    CHECK(check_order(&sll, 2, (int[2]){2, 0}), PASS);
    after_extract = extract(&sll, &vals[0].e);
    CHECK(after_extract, end(&sll));
    CHECK(check_order(&sll, 1, (int[1]){2}), PASS);
    CHECK(size(&sll), 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(sll_test_push_extract_range)
{
    singly_linked_list sll = sll_init(sll, struct val, e, NULL, val_cmp, NULL);
    struct val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum check_result const t = create_list(&sll, 5, vals);
    CHECK(t, PASS);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    struct val *after_extract = extract_range(&sll, &vals[3].e, &vals[1].e);
    CHECK(size(&sll), 2);
    CHECK(validate(&sll), true);
    CHECK(after_extract == NULL, false);
    CHECK(after_extract->val, 0);
    CHECK(check_order(&sll, 2, (int[2]){4, 0}), PASS);
    after_extract = extract_range(&sll, sll_begin_elem(&sll), &vals[0].e);
    CHECK(after_extract, end(&sll));
    CHECK(is_empty(&sll), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(sll_test_splice_two_lists)
{
    singly_linked_list to_lose
        = sll_init(to_lose, struct val, e, NULL, val_cmp, NULL);
    struct val to_lose_vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum check_result t = create_list(&to_lose, 5, to_lose_vals);
    CHECK(t, PASS);
    singly_linked_list to_gain
        = sll_init(to_gain, struct val, e, NULL, val_cmp, NULL);
    struct val to_gain_vals[2] = {{.val = 0}, {.val = 1}};
    t = create_list(&to_gain, 2, to_gain_vals);
    CHECK(t, PASS);
    CHECK(check_order(&to_lose, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    CHECK(check_order(&to_gain, 2, (int[2]){1, 0}), PASS);
    CHECK(splice(&to_gain, sll_begin_elem(&to_gain), &to_lose,
                 sll_begin_elem(&to_lose)),
          CCC_OK);
    CHECK(size(&to_gain), 3);
    CHECK(size(&to_lose), 4);
    CHECK(check_order(&to_lose, 4, (int[4]){3, 2, 1, 0}), PASS);
    CHECK(check_order(&to_gain, 3, (int[3]){1, 4, 0}), PASS);
    CHECK(splice_range(&to_gain, sll_begin_elem(&to_gain), &to_lose,
                       sll_begin_elem(&to_lose), &to_lose_vals[0].e),
          CCC_OK);
    CHECK(size(&to_gain), 7);
    CHECK(is_empty(&to_lose), true);
    CHECK(check_order(&to_gain, 7, (int[7]){1, 3, 2, 1, 0, 4, 0}), PASS);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(sll_test_push_pop_three(), sll_test_push_extract_middle(),
                     sll_test_push_extract_range(),
                     sll_test_splice_two_lists());
}
