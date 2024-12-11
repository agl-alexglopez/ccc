#define TRAITS_USING_NAMESPACE_CCC
#define ORDERED_MULTIMAP_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "ommap_util.h"
#include "ordered_multimap.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

CHECK_BEGIN_STATIC_FN(ommap_test_insert_one)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, key, NULL, id_cmp, NULL);
    struct val single;
    single.key = 0;
    CHECK(unwrap(insert_r(&omm, &single.elem)) != NULL, true);
    CHECK(is_empty(&omm), false);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ommap_test_insert_three)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, key, NULL, id_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].key = i;
        CHECK(unwrap(insert_r(&omm, &three_vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
        CHECK(size(&omm), (size_t)i + 1);
    }
    CHECK(size(&omm), (size_t)3);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ommap_test_insert_macros)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, key, std_alloc, id_cmp, NULL);
    struct val const *ins = ccc_omm_or_insert_w(
        entry_r(&omm, &(int){2}), (struct val){.val = 0, .key = 2});
    CHECK(ins != NULL, true);
    CHECK(validate(&omm), true);
    CHECK(size(&omm), 1);
    ins = omm_insert_entry_w(entry_r(&omm, &(int){2}),
                             (struct val){.val = 0, .key = 2});
    CHECK(ins != NULL, true);
    CHECK(validate(&omm), true);
    CHECK(size(&omm), 2);
    ins = omm_insert_entry_w(entry_r(&omm, &(int){9}),
                             (struct val){.val = 1, .key = 9});
    CHECK(ins != NULL, true);
    CHECK(validate(&omm), true);
    CHECK(size(&omm), 3);
    ins = ccc_entry_unwrap(
        omm_insert_or_assign_w(&omm, 3, (struct val){.val = 99}));
    CHECK(validate(&omm), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&omm), true);
    CHECK(ins->val, 99);
    CHECK(size(&omm), 4);
    ins = ccc_entry_unwrap(
        omm_insert_or_assign_w(&omm, 3, (struct val){.val = 98}));
    CHECK(validate(&omm), true);
    CHECK(ins == NULL, false);
    CHECK(ins->val, 98);
    CHECK(size(&omm), 4);
    ins = ccc_entry_unwrap(omm_try_insert_w(&omm, 3, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&omm), true);
    CHECK(ins->val, 98);
    CHECK(size(&omm), 4);
    ins = ccc_entry_unwrap(omm_try_insert_w(&omm, 4, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&omm), true);
    CHECK(ins->val, 100);
    CHECK(size(&omm), 5);
    CHECK_END_FN(omm_clear(&omm, NULL););
}

CHECK_BEGIN_STATIC_FN(ommap_test_struct_getter)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, key, NULL, id_cmp, NULL);
    ccc_ordered_multimap omm_tester_clone = ccc_omm_init(
        omm_tester_clone, struct val, elem, key, NULL, id_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].key = i;
        tester_clone[i].key = i;
        CHECK(unwrap(insert_r(&omm, &vals[i].elem)) != NULL, true);
        CHECK(unwrap(insert_r(&omm_tester_clone, &tester_clone[i].elem))
                  != NULL,
              true);
        CHECK(validate(&omm), true);
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->key, vals[i].key);
    }
    CHECK(size(&omm), (size_t)10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ommap_test_insert_three_dups)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, key, NULL, id_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].key = 0;
        CHECK(unwrap(insert_r(&omm, &three_vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
        CHECK(size(&omm), (size_t)i + 1);
    }
    CHECK(size(&omm), (size_t)3);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ommap_test_insert_shuffle)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, key, NULL, id_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&omm, vals, size, prime), PASS);
    struct val const *max = ccc_omm_max(&omm);
    CHECK(max->key, (int)size - 1);
    struct val const *min = ccc_omm_min(&omm);
    CHECK(min->key, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &omm), size);
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].key, sorted_check[i]);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ommap_test_read_max_min)
{
    ccc_ordered_multimap omm
        = ccc_omm_init(omm, struct val, elem, key, NULL, id_cmp, NULL);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].key = i;
        CHECK(unwrap(insert_r(&omm, &vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
        CHECK(size(&omm), (size_t)i + 1);
    }
    CHECK(size(&omm), (size_t)10);
    struct val const *max = ccc_omm_max(&omm);
    CHECK(max->key, 9);
    struct val const *min = ccc_omm_min(&omm);
    CHECK(min->key, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ommap_test_insert_and_find)
{
    int const size = 100;
    ordered_multimap s = omm_init(s, struct val, elem, key, NULL, id_cmp, NULL);
    struct val vals[101];
    for (int i = 0, curval = 0; i < size; i += 2, ++curval)
    {
        vals[curval] = (struct val){.key = i, .val = i};
        ccc_entry e = try_insert(&s, &vals[curval].elem);
        CHECK(occupied(&e), false);
        e = try_insert(&s, &vals[curval].elem);
        CHECK(occupied(&e), true);
        struct val const *const v = unwrap(&e);
        CHECK(v == NULL, false);
        CHECK(v->key, i);
        CHECK(v->val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        CHECK(contains(&s, &i), true);
        CHECK(occupied(entry_r(&s, &i)), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        CHECK(contains(&s, &i), false);
        CHECK(occupied(entry_r(&s, &i)), false);
    }
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(ommap_test_insert_one(), ommap_test_insert_three(),
                     ommap_test_insert_and_find(), ommap_test_insert_macros(),
                     ommap_test_struct_getter(), ommap_test_insert_three_dups(),
                     ommap_test_insert_shuffle(), ommap_test_read_max_min());
}
