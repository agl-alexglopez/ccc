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
    CHECK(size(&sll), 3);
    CHECK(check_order(&sll, 3, (int[3]){2, 1, 0}), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(sll_push_and_splice)
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

CHECK_BEGIN_STATIC_FN(sll_push_and_splice_range)
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

CHECK_BEGIN_STATIC_FN(sll_push_and_splice_range_no_ops)
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

int
main()
{
    return CHECK_RUN(sll_test_insert_three(), sll_push_and_splice(),
                     sll_push_and_splice_range(),
                     sll_push_and_splice_range_no_ops());
}
