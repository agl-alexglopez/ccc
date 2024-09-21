#define TRAITS_USING_NAMESPACE_CCC
#define ORDERED_MAP_USING_NAMESPACE_CCC

#include "map_util.h"
#include "ordered_map.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

BEGIN_STATIC_TEST(map_test_insert_one)
{
    ccc_ordered_map s
        = ccc_om_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    struct val single;
    single.val = 0;
    CHECK(insert_entry(entry_vr(&s, &single.val), &single.elem) != NULL, true);
    CHECK(empty(&s), false);
    CHECK(((struct val *)ccc_om_root(&s))->val == single.val, true);
    END_TEST();
}

BEGIN_STATIC_TEST(map_test_insert_three)
{
    ccc_ordered_map s
        = ccc_om_init(struct val, elem, val, s, realloc, val_cmp, NULL);
    struct val swap_slot = {.val = 1, .id = 99};
    CHECK(occupied(insert_vr(&s, &swap_slot.elem, &(struct val){}.elem)),
          false);
    CHECK(validate(&s), true);
    CHECK(size(&s), (size_t)1);
    swap_slot = (struct val){.val = 1, .id = 137};
    struct val const *ins
        = unwrap(insert_vr(&s, &swap_slot.elem, &(struct val){}.elem));
    CHECK(validate(&s), true);
    CHECK(size(&s), (size_t)1);
    CHECK(ins != NULL, true);
    CHECK(ins->val, 1);
    CHECK(ins->id, 137);
    CHECK(swap_slot.val, 1);
    CHECK(swap_slot.id, 99);
    ins = ccc_om_or_insert_w(entry_vr(&s, &(int){2}),
                             (struct val){.val = 2, .id = 0});
    CHECK(ins != NULL, true);
    CHECK(ins->id, 0);
    CHECK(validate(&s), true);
    CHECK(size(&s), (size_t)2);
    ins = insert_entry(entry_vr(&s, &(int){2}),
                       &(struct val){.val = 2, .id = 1}.elem);
    CHECK(ins != NULL, true);
    CHECK(ins->id, 1);
    CHECK(validate(&s), true);
    CHECK(size(&s), (size_t)2);
    ins = ccc_entry_unwrap(
        om_insert_or_assign_w(&s, 3, (struct val){.id = 99}));
    CHECK(ins == NULL, false);
    CHECK(validate(&s), true);
    CHECK(ins->id, 99);
    CHECK(ins->val, 3);
    CHECK(size(&s), 3);
    ccc_om_clear_and_free(&s, NULL);
    END_TEST();
}

BEGIN_STATIC_TEST(map_test_insert_macros)
{
    ccc_ordered_map s
        = ccc_om_init(struct val, elem, val, s, realloc, val_cmp, NULL);
    struct val const *ins = ccc_om_or_insert_w(entry_vr(&s, &(int){2}),
                                               (struct val){.val = 2, .id = 0});
    CHECK(ins != NULL, true);
    CHECK(validate(&s), true);
    CHECK(size(&s), 1);
    ins = om_insert_entry_w(entry_vr(&s, &(int){2}),
                            (struct val){.val = 2, .id = 0});
    CHECK(ins != NULL, true);
    CHECK(validate(&s), true);
    CHECK(size(&s), 1);
    ins = om_insert_entry_w(entry_vr(&s, &(int){9}),
                            (struct val){.val = 9, .id = 1});
    CHECK(ins != NULL, true);
    CHECK(validate(&s), true);
    CHECK(size(&s), 2);
    ins = ccc_entry_unwrap(
        om_insert_or_assign_w(&s, 3, (struct val){.id = 99}));
    CHECK(validate(&s), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&s), true);
    CHECK(ins->id, 99);
    CHECK(size(&s), 3);
    ins = ccc_entry_unwrap(
        om_insert_or_assign_w(&s, 3, (struct val){.id = 98}));
    CHECK(validate(&s), true);
    CHECK(ins == NULL, false);
    CHECK(ins->id, 98);
    CHECK(size(&s), 3);
    ins = ccc_entry_unwrap(om_try_insert_w(&s, 3, (struct val){.id = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&s), true);
    CHECK(ins->id, 98);
    CHECK(size(&s), 3);
    ins = ccc_entry_unwrap(om_try_insert_w(&s, 4, (struct val){.id = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&s), true);
    CHECK(ins->id, 100);
    CHECK(size(&s), 4);
    END_TEST(ccc_om_clear_and_free(&s, NULL););
}

BEGIN_STATIC_TEST(map_test_struct_getter)
{
    ccc_ordered_map s
        = ccc_om_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    ccc_ordered_map map_tester_clone = ccc_om_init(
        struct val, elem, val, map_tester_clone, NULL, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        CHECK(insert_entry(entry_vr(&s, &vals[i].val), &vals[i].elem) != NULL,
              true);
        CHECK(insert_entry(entry_vr(&map_tester_clone, &tester_clone[i].val),
                           &tester_clone[i].elem)
                  != NULL,
              true);
        CHECK(validate(&s), true);
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our
           get to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val);
    }
    CHECK(size(&s), (size_t)10);
    END_TEST();
}

BEGIN_STATIC_TEST(map_test_insert_shuffle)
{
    ccc_ordered_map s
        = ccc_om_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&s, vals, size, prime), PASS);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &s), size);
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i]);
    }
    END_TEST();
}

int
main()
{
    return RUN_TESTS(map_test_insert_one(), map_test_insert_three(),
                     map_test_insert_macros(), map_test_struct_getter(),
                     map_test_insert_shuffle());
}
