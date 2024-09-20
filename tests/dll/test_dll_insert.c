#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "dll_util.h"
#include "doubly_linked_list.h"
#include "test.h"
#include "traits.h"

#include <stddef.h>

BEGIN_STATIC_TEST(dll_test_push_three_front)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    CHECK(push_front(&dll, &(struct val){}.e) != NULL, true);
    CHECK(dll_validate(&dll), true);
    CHECK(push_front(&dll, &(struct val){.id = 1, .val = 1}.e) != NULL, true);
    CHECK(dll_validate(&dll), true);
    CHECK(push_front(&dll, &(struct val){.id = 2, .val = 2}.e) != NULL, true);
    CHECK(dll_validate(&dll), true);
    CHECK(size(&dll), 3);
    struct val *v = dll_front(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 2);
    v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 0);
    END_TEST();
}

BEGIN_STATIC_TEST(dll_test_push_three_back)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    CHECK(push_back(&dll, &(struct val){}.e) != NULL, true);
    CHECK(dll_validate(&dll), true);
    CHECK(push_back(&dll, &(struct val){.id = 1, .val = 1}.e) != NULL, true);
    CHECK(dll_validate(&dll), true);
    CHECK(push_back(&dll, &(struct val){.id = 2, .val = 2}.e) != NULL, true);
    CHECK(dll_validate(&dll), true);
    CHECK(size(&dll), 3);
    struct val *v = dll_front(&dll);
    CHECK(v->id, 0);
    CHECK(v == NULL, false);
    v = dll_back(&dll);
    CHECK(v == NULL, false);
    CHECK(v->id, 2);
    END_TEST();
}

BEGIN_STATIC_TEST(dll_test_push_and_splice)
{
    doubly_linked_list dll = dll_init(dll, struct val, e, NULL, val_cmp, NULL);
    (void)push_back(&dll, &(struct val){}.e);
    (void)push_back(&dll, &(struct val){.id = 1, .val = 1}.e);
    (void)push_back(&dll, &(struct val){.id = 2, .val = 2}.e);
    (void)push_back(&dll, &(struct val){.id = 3, .val = 3}.e);
    struct val *v = back(&dll);
    dll_splice(dll_begin_elem(&dll), &v->e);
    CHECK(dll_validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[4]){3, 0, 1, 2}), PASS);
    dll_splice(&((struct val *)back(&dll))->e, &((struct val *)front(&dll))->e);
    CHECK(dll_validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[4]){0, 1, 3, 2}), PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(dll_test_push_and_splice_range)
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
    dll_splice_range(dll_begin_elem(&dll), &v1->e, dll_end_sentinel(&dll));
    CHECK(dll_validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[4]){1, 2, 3, 0}), PASS);
    dll_splice_range(dll_begin_elem(&dll), &v2->e, dll_end_sentinel(&dll));
    CHECK(dll_validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[4]){2, 3, 0, 1}), PASS);
    dll_splice_range(&v2->e, &v3->e, &v1->e);
    CHECK(dll_validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[4]){3, 0, 2, 1}), PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(dll_test_push_and_splice_no_ops)
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
    dll_splice_range(&v0->e, &v0->e, dll_end_sentinel(&dll));
    CHECK(dll_validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[4]){0, 1, 2, 3}), PASS);
    dll_splice_range(&v3->e, &v1->e, &v3->e);
    CHECK(dll_validate(&dll), true);
    CHECK(check_order(&dll, 4, (int[4]){0, 1, 2, 3}), PASS);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(dll_test_push_three_front, dll_test_push_three_back,
                     dll_test_push_and_splice, dll_test_push_and_splice_range,
                     dll_test_push_and_splice_no_ops);
}
