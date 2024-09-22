#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "singly_linked_list.h"
#include "sll_util.h"
#include "test.h"
#include "traits.h"

BEGIN_STATIC_TEST(sll_test_insert_three)
{
    singly_linked_list sll = sll_init(sll, struct val, e, NULL, val_cmp, NULL);
    CHECK(push_front(&sll, &(struct val){}.e) != NULL, true);
    struct val *v = front(&sll);
    CHECK(validate(&sll), true);
    CHECK(v == NULL, false);
    CHECK(v->val, 0);
    CHECK(push_front(&sll, &(struct val){.val = 1}.e) != NULL, true);
    CHECK(validate(&sll), true);
    v = front(&sll);
    CHECK(v == NULL, false);
    CHECK(v->val, 1);
    CHECK(push_front(&sll, &(struct val){.val = 2}.e) != NULL, true);
    CHECK(validate(&sll), true);
    v = front(&sll);
    CHECK(v == NULL, false);
    CHECK(v->val, 2);
    CHECK(size(&sll), 3);
    CHECK(check_order(&sll, 3, (int[3]){2, 1, 0}), PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(sll_push_and_splice)
{
    singly_linked_list sll = sll_init(sll, struct val, e, NULL, val_cmp, NULL);
    struct val vals[4] = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}};
    enum test_result const t = create_list(&sll, 4, vals);
    CHECK(t, PASS);
    CHECK(check_order(&sll, 4, (int[4]){3, 2, 1, 0}), PASS);
    splice(&sll, sll_begin_elem(&sll), &sll, &vals[0].e);
    CHECK(validate(&sll), true);
    CHECK(check_order(&sll, 4, (int[4]){3, 0, 2, 1}), PASS);
    splice(&sll, &vals[0].e, &sll, &vals[3].e);
    CHECK(validate(&sll), true);
    CHECK(check_order(&sll, 4, (int[4]){0, 3, 2, 1}), PASS);
    splice(&sll, &vals[1].e, &sll, &vals[0].e);
    CHECK(validate(&sll), true);
    CHECK(check_order(&sll, 4, (int[4]){3, 2, 1, 0}), PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(sll_push_and_splice_range)
{
    singly_linked_list sll = sll_init(sll, struct val, e, NULL, val_cmp, NULL);
    struct val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum test_result const t = create_list(&sll, 5, vals);
    CHECK(t, PASS);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    splice_range(&sll, sll_begin_sentinel(&sll), &sll, &vals[2].e, &vals[0].e);
    CHECK(check_order(&sll, 5, (int[5]){2, 1, 0, 4, 3}), PASS);
    splice_range(&sll, &vals[3].e, &sll, &vals[2].e, &vals[0].e);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    splice_range(&sll, sll_begin_sentinel(&sll), &sll, &vals[3].e, &vals[1].e);
    CHECK(check_order(&sll, 5, (int[5]){3, 2, 1, 4, 0}), PASS);
    splice_range(&sll, &vals[0].e, &sll, &vals[2].e, &vals[4].e);
    CHECK(check_order(&sll, 5, (int[5]){3, 0, 2, 1, 4}), PASS);
    splice_range(&sll, &vals[1].e, &sll, &vals[0].e, &vals[2].e);
    CHECK(check_order(&sll, 5, (int[5]){3, 1, 0, 2, 4}), PASS);
    splice_range(&sll, &vals[3].e, &sll, &vals[0].e, &vals[2].e);
    CHECK(check_order(&sll, 5, (int[5]){3, 0, 2, 1, 4}), PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(sll_push_and_splice_range_no_ops)
{
    singly_linked_list sll = sll_init(sll, struct val, e, NULL, val_cmp, NULL);
    struct val vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    enum test_result const t = create_list(&sll, 5, vals);
    CHECK(t, PASS);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    splice(&sll, &vals[2].e, &sll, &vals[2].e);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    splice(&sll, &vals[3].e, &sll, &vals[2].e);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    splice_range(&sll, &vals[3].e, &sll, &vals[2].e, &vals[0].e);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    splice_range(&sll, &vals[3].e, &sll, &vals[3].e, &vals[0].e);
    CHECK(check_order(&sll, 5, (int[5]){4, 3, 2, 1, 0}), PASS);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(sll_test_insert_three(), sll_push_and_splice(),
                     sll_push_and_splice_range(),
                     sll_push_and_splice_range_no_ops());
}
