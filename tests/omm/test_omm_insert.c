#define TRAITS_USING_NAMESPACE_CCC
#define ORDERED_MULTIMAP_USING_NAMESPACE_CCC

#include "omm_util.h"
#include "ordered_multimap.h"
#include "test.h"
#include "traits.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

BEGIN_STATIC_TEST(omm_test_insert_one)
{
    ccc_ordered_multimap pq
        = ccc_omm_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    struct val single;
    single.val = 0;
    CHECK(unwrap(insert_vr(&pq, &single.elem)) != NULL, true);
    CHECK(is_empty(&pq), false);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_insert_three)
{
    ccc_ordered_multimap pq
        = ccc_omm_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        CHECK(unwrap(insert_vr(&pq, &three_vals[i].elem)) != NULL, true);
        CHECK(validate(&pq), true);
        CHECK(size(&pq), (size_t)i + 1);
    }
    CHECK(size(&pq), (size_t)3);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_insert_macros)
{
    ccc_ordered_multimap pq
        = ccc_omm_init(struct val, elem, val, pq, realloc, val_cmp, NULL);
    struct val const *ins = ccc_omm_or_insert_w(
        entry_vr(&pq, &(int){2}), (struct val){.val = 2, .id = 0});
    CHECK(ins != NULL, true);
    CHECK(validate(&pq), true);
    CHECK(size(&pq), 1);
    ins = omm_insert_entry_w(entry_vr(&pq, &(int){2}),
                             (struct val){.val = 2, .id = 0});
    CHECK(ins != NULL, true);
    CHECK(validate(&pq), true);
    CHECK(size(&pq), 2);
    ins = omm_insert_entry_w(entry_vr(&pq, &(int){9}),
                             (struct val){.val = 9, .id = 1});
    CHECK(ins != NULL, true);
    CHECK(validate(&pq), true);
    CHECK(size(&pq), 3);
    ins = ccc_entry_unwrap(
        omm_insert_or_assign_w(&pq, 3, (struct val){.id = 99}));
    CHECK(validate(&pq), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&pq), true);
    CHECK(ins->id, 99);
    CHECK(size(&pq), 4);
    ins = ccc_entry_unwrap(
        omm_insert_or_assign_w(&pq, 3, (struct val){.id = 98}));
    CHECK(validate(&pq), true);
    CHECK(ins == NULL, false);
    CHECK(ins->id, 98);
    CHECK(size(&pq), 4);
    ins = ccc_entry_unwrap(omm_try_insert_w(&pq, 3, (struct val){.id = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&pq), true);
    CHECK(ins->id, 98);
    CHECK(size(&pq), 4);
    ins = ccc_entry_unwrap(omm_try_insert_w(&pq, 4, (struct val){.id = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&pq), true);
    CHECK(ins->id, 100);
    CHECK(size(&pq), 5);
    END_TEST(omm_clear(&pq, NULL););
}

BEGIN_STATIC_TEST(omm_test_struct_getter)
{
    ccc_ordered_multimap pq
        = ccc_omm_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    ccc_ordered_multimap pq_tester_clone = ccc_omm_init(
        struct val, elem, val, pq_tester_clone, NULL, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        CHECK(unwrap(insert_vr(&pq, &vals[i].elem)) != NULL, true);
        CHECK(unwrap(insert_vr(&pq_tester_clone, &tester_clone[i].elem))
                  != NULL,
              true);
        CHECK(validate(&pq), true);
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val);
    }
    CHECK(size(&pq), (size_t)10);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_insert_three_dups)
{
    ccc_ordered_multimap pq
        = ccc_omm_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        CHECK(unwrap(insert_vr(&pq, &three_vals[i].elem)) != NULL, true);
        CHECK(validate(&pq), true);
        CHECK(size(&pq), (size_t)i + 1);
    }
    CHECK(size(&pq), (size_t)3);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_insert_shuffle)
{
    ccc_ordered_multimap pq
        = ccc_omm_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS);
    struct val const *max = ccc_omm_max(&pq);
    CHECK(max->val, (int)size - 1);
    struct val const *min = ccc_omm_min(&pq);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &pq), size);
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i]);
    }
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_read_max_min)
{
    ccc_ordered_multimap pq
        = ccc_omm_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        CHECK(unwrap(insert_vr(&pq, &vals[i].elem)) != NULL, true);
        CHECK(validate(&pq), true);
        CHECK(size(&pq), (size_t)i + 1);
    }
    CHECK(size(&pq), (size_t)10);
    struct val const *max = ccc_omm_max(&pq);
    CHECK(max->val, 9);
    struct val const *min = ccc_omm_min(&pq);
    CHECK(min->val, 0);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(omm_test_insert_one(), omm_test_insert_three(),
                     omm_test_insert_macros(), omm_test_struct_getter(),
                     omm_test_insert_three_dups(), omm_test_insert_shuffle(),
                     omm_test_read_max_min());
}
