#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "dll_util.h"
#include "doubly_linked_list.h"
#include "test.h"
#include "traits.h"

#include <stddef.h>
BEGIN_STATIC_TEST(dll_test_push_pop_front)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    CHECK(push_front(&dll, &(struct val){}.e) != NULL, true);
    CHECK(push_front(&dll, &(struct val){.id = 1, .val = 1}.e) != NULL, true);
    CHECK(push_front(&dll, &(struct val){.id = 2, .val = 2}.e) != NULL, true);
    CHECK(size(&dll), 3);
    struct val *v = dll_front(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 2);
    pop_front(&dll);
    CHECK(validate(&dll), true);
    v = dll_front(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 1);
    pop_front(&dll);
    v = dll_front(&dll);
    CHECK(validate(&dll), true);
    CHECK(v == NULL, false);
    CHECK(v->id, 0);
    pop_front(&dll);
    CHECK(empty(&dll), true);
    END_TEST();
}

BEGIN_STATIC_TEST(dll_test_push_pop_back)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    CHECK(push_back(&dll, &(struct val){}.e) != NULL, true);
    CHECK(validate(&dll), true);
    CHECK(push_back(&dll, &(struct val){.id = 1, .val = 1}.e) != NULL, true);
    CHECK(validate(&dll), true);
    CHECK(push_back(&dll, &(struct val){.id = 2, .val = 2}.e) != NULL, true);
    CHECK(validate(&dll), true);
    CHECK(size(&dll), 3);
    struct val *v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 2);
    pop_back(&dll);
    CHECK(validate(&dll), true);
    v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 1);
    pop_back(&dll);
    CHECK(validate(&dll), true);
    v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 0);
    pop_back(&dll);
    CHECK(empty(&dll), true);
    END_TEST();
}

BEGIN_STATIC_TEST(dll_test_push_pop_middle)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    struct val *v0 = &(struct val){};
    struct val *v1 = &(struct val){.id = 1, .val = 1};
    struct val *v2 = &(struct val){.id = 2, .val = 2};
    struct val *v3 = &(struct val){.id = 3, .val = 3};
    (void)push_back(&dll, &v0->e);
    (void)push_back(&dll, &v1->e);
    (void)push_back(&dll, &v2->e);
    (void)push_back(&dll, &v3->e);
    dll_erase(&dll, &v2->e);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 3, (int[3]){0, 1, 3}), PASS);
    dll_erase(&dll, &v1->e);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 2, (int[2]){0, 3}), PASS);
    dll_erase(&dll, &v3->e);
    CHECK(validate(&dll), true);
    CHECK(check_order(&dll, 1, (int[1]){0}), PASS);
    dll_erase(&dll, &v0->e);
    CHECK(validate(&dll), true);
    CHECK(empty(&dll), true);
    END_TEST();
}

BEGIN_STATIC_TEST(dll_test_push_pop_middle_range)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    struct val *v0 = &(struct val){};
    struct val *v1 = &(struct val){.id = 1, .val = 1};
    struct val *v2 = &(struct val){.id = 2, .val = 2};
    struct val *v3 = &(struct val){.id = 3, .val = 3};
    struct val *v4 = &(struct val){.id = 4, .val = 4};
    (void)push_back(&dll, &v0->e);
    (void)push_back(&dll, &v1->e);
    (void)push_back(&dll, &v2->e);
    (void)push_back(&dll, &v3->e);
    (void)push_back(&dll, &v4->e);
    dll_erase_range(&dll, &v1->e, &v4->e);
    CHECK(validate(&dll), true);
    CHECK(size(&dll), 2);
    CHECK(check_order(&dll, 2, (int[2]){0, 4}), PASS);
    CHECK(v1->e.n_ == NULL, true);
    CHECK(v1->e.p_ == NULL, true);
    CHECK(v2->e.n_ == NULL, true);
    CHECK(v2->e.p_ == NULL, true);
    CHECK(v3->e.n_ == NULL, true);
    CHECK(v3->e.p_ == NULL, true);
    dll_erase_range(&dll, &v0->e, dll_end_sentinel(&dll));
    CHECK(validate(&dll), true);
    CHECK(size(&dll), 0);
    CHECK(v0->e.n_ == NULL, true);
    CHECK(v0->e.p_ == NULL, true);
    CHECK(v4->e.n_ == NULL, true);
    CHECK(v4->e.p_ == NULL, true);
    END_TEST();
}

BEGIN_STATIC_TEST(dll_test_splice_two_lists)
{
    doubly_linked_list to_gain
        = dll_init(to_gain, struct val, e, NULL, val_cmp, NULL);
    doubly_linked_list to_lose
        = dll_init(to_lose, struct val, e, NULL, val_cmp, NULL);
    struct val to_gain_vals[2] = {{.val = 0}, {.val = 1}};
    struct val to_lose_vals[5]
        = {{.val = 0}, {.val = 1}, {.val = 2}, {.val = 3}, {.val = 4}};
    for (int i = 0; (size_t)i < sizeof(to_gain_vals) / sizeof(struct val); ++i)
    {
        CHECK(push_back(&to_gain, &to_gain_vals[i].e) == NULL, false);
    }
    CHECK(check_order(&to_gain, 2, (int[2]){0, 1}), PASS);
    for (int i = 0; (size_t)i < sizeof(to_lose_vals) / sizeof(struct val); ++i)
    {
        CHECK(push_back(&to_lose, &to_lose_vals[i].e) == NULL, false);
    }
    CHECK(check_order(&to_lose, 5, (int[5]){0, 1, 2, 3, 4}), PASS);
    dll_splice(&to_gain, dll_end_sentinel(&to_gain), &to_lose,
               &to_lose_vals[0].e);
    CHECK(validate(&to_gain), true);
    CHECK(validate(&to_lose), true);
    CHECK(size(&to_gain), 3);
    CHECK(size(&to_lose), 4);
    CHECK(check_order(&to_gain, 3, (int[3]){0, 1, 0}), PASS);
    CHECK(check_order(&to_lose, 4, (int[4]){1, 2, 3, 4}), PASS);
    dll_splice_range(&to_gain, dll_end_elem(&to_gain), &to_lose,
                     dll_begin_elem(&to_lose), dll_end_sentinel(&to_lose));
    CHECK(validate(&to_gain), true);
    CHECK(validate(&to_lose), true);
    CHECK(size(&to_gain), 7);
    CHECK(size(&to_lose), 0);
    CHECK(check_order(&to_gain, 7, (int[7]){0, 1, 1, 2, 3, 4, 0}), PASS);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(dll_test_push_pop_front(), dll_test_push_pop_back(),
                     dll_test_push_pop_middle(),
                     dll_test_push_pop_middle_range(),
                     dll_test_splice_two_lists());
}
