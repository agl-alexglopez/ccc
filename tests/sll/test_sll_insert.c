#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "singly_linked_list.h"
#include "sll_util.h"
#include "test.h"
#include "traits.h"

BEGIN_STATIC_TEST(sll_test_insert_three)
{
    singly_linked_list sll = sll_init(sll, struct val, e, NULL, NULL);
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

int
main()
{
    return RUN_TESTS(sll_test_insert_three());
}
