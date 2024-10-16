#define TRAITS_USING_NAMESPACE_CCC
#define ORDERED_MULTIMAP_USING_NAMESPACE_CCC

#include "omm_util.h"
#include "ordered_multimap.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

BEGIN_STATIC_TEST(omm_test_insert_one)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);
    struct val single;
    single.val = 0;
    CHECK(unwrap(insert_r(&omm, &single.elem)) != NULL, true);
    CHECK(is_empty(&omm), false);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_insert_three)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        CHECK(unwrap(insert_r(&omm, &three_vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
        CHECK(size(&omm), (size_t)i + 1);
    }
    CHECK(size(&omm), (size_t)3);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_insert_macros)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, val, realloc, val_cmp, NULL);
    struct val const *ins = ccc_omm_or_insert_w(
        entry_r(&omm, &(int){2}), (struct val){.val = 2, .id = 0});
    CHECK(ins != NULL, true);
    CHECK(validate(&omm), true);
    CHECK(size(&omm), 1);
    ins = omm_insert_entry_w(entry_r(&omm, &(int){2}),
                             (struct val){.val = 2, .id = 0});
    CHECK(ins != NULL, true);
    CHECK(validate(&omm), true);
    CHECK(size(&omm), 2);
    ins = omm_insert_entry_w(entry_r(&omm, &(int){9}),
                             (struct val){.val = 9, .id = 1});
    CHECK(ins != NULL, true);
    CHECK(validate(&omm), true);
    CHECK(size(&omm), 3);
    ins = ccc_entry_unwrap(
        omm_insert_or_assign_w(&omm, 3, (struct val){.id = 99}));
    CHECK(validate(&omm), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&omm), true);
    CHECK(ins->id, 99);
    CHECK(size(&omm), 4);
    ins = ccc_entry_unwrap(
        omm_insert_or_assign_w(&omm, 3, (struct val){.id = 98}));
    CHECK(validate(&omm), true);
    CHECK(ins == NULL, false);
    CHECK(ins->id, 98);
    CHECK(size(&omm), 4);
    ins = ccc_entry_unwrap(omm_try_insert_w(&omm, 3, (struct val){.id = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&omm), true);
    CHECK(ins->id, 98);
    CHECK(size(&omm), 4);
    ins = ccc_entry_unwrap(omm_try_insert_w(&omm, 4, (struct val){.id = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&omm), true);
    CHECK(ins->id, 100);
    CHECK(size(&omm), 5);
    END_TEST(omm_clear(&omm, NULL););
}

BEGIN_STATIC_TEST(omm_test_struct_getter)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);
    ccc_ordered_multimap omm_tester_clone = ccc_omm_init(
        omm_tester_clone, struct val, elem, val, NULL, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        CHECK(unwrap(insert_r(&omm, &vals[i].elem)) != NULL, true);
        CHECK(unwrap(insert_r(&omm_tester_clone, &tester_clone[i].elem))
                  != NULL,
              true);
        CHECK(validate(&omm), true);
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val);
    }
    CHECK(size(&omm), (size_t)10);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_insert_three_dups)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        CHECK(unwrap(insert_r(&omm, &three_vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
        CHECK(size(&omm), (size_t)i + 1);
    }
    CHECK(size(&omm), (size_t)3);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_insert_shuffle)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&omm, vals, size, prime), PASS);
    struct val const *max = ccc_omm_max(&omm);
    CHECK(max->val, (int)size - 1);
    struct val const *min = ccc_omm_min(&omm);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &omm), size);
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i]);
    }
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_read_max_min)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        CHECK(unwrap(insert_r(&omm, &vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
        CHECK(size(&omm), (size_t)i + 1);
    }
    CHECK(size(&omm), (size_t)10);
    struct val const *max = ccc_omm_max(&omm);
    CHECK(max->val, 9);
    struct val const *min = ccc_omm_min(&omm);
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
