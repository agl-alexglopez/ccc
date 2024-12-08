#define TRAITS_USING_NAMESPACE_CCC
#define ORDERED_MAP_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "omap_util.h"
#include "ordered_map.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

CHECK_BEGIN_STATIC_FN(omap_test_insert_one)
{
    ccc_ordered_map s
        = ccc_om_init(s, struct val, elem, key, NULL, id_cmp, NULL);
    struct val single;
    single.key = 0;
    CHECK(insert_entry(entry_r(&s, &single.key), &single.elem) != NULL, true);
    CHECK(is_empty(&s), false);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_three)
{
    ccc_ordered_map s
        = ccc_om_init(s, struct val, elem, key, std_alloc, id_cmp, NULL);
    struct val swap_slot = {.val = 99, .key = 1};
    CHECK(occupied(insert_r(&s, &swap_slot.elem, &(struct val){}.elem)), false);
    CHECK(validate(&s), true);
    CHECK(size(&s), (size_t)1);
    swap_slot = (struct val){.val = 137, .key = 1};
    ccc_entry *const e = insert_r(&s, &swap_slot.elem, &(struct val){}.elem);
    struct val const *ins = unwrap(e);
    CHECK(occupied(e), true);
    CHECK(validate(&s), true);
    CHECK(size(&s), (size_t)1);
    CHECK(ins != NULL, true);
    CHECK(ins->val, 99);
    CHECK(ins->key, 1);
    CHECK(swap_slot.key, 1);
    CHECK(swap_slot.val, 99);
    ins = ccc_om_or_insert_w(entry_r(&s, &(int){2}),
                             (struct val){.val = 0, .key = 2});
    CHECK(ins != NULL, true);
    CHECK(ins->val, 0);
    CHECK(validate(&s), true);
    CHECK(size(&s), (size_t)2);
    ins = insert_entry(entry_r(&s, &(int){2}),
                       &(struct val){.val = 1, .key = 2}.elem);
    CHECK(ins != NULL, true);
    CHECK(ins->val, 1);
    CHECK(validate(&s), true);
    CHECK(size(&s), (size_t)2);
    ins = ccc_entry_unwrap(
        om_insert_or_assign_w(&s, 3, (struct val){.val = 99}));
    CHECK(ins == NULL, false);
    CHECK(validate(&s), true);
    CHECK(ins->val, 99);
    CHECK(ins->key, 3);
    CHECK(size(&s), 3);
    CHECK_END_FN((void)ccc_om_clear(&s, NULL););
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_macros)
{
    ccc_ordered_map s
        = ccc_om_init(s, struct val, elem, key, std_alloc, id_cmp, NULL);
    struct val const *ins = ccc_om_or_insert_w(
        entry_r(&s, &(int){2}), (struct val){.val = 0, .key = 2});
    CHECK(ins != NULL, true);
    CHECK(validate(&s), true);
    CHECK(size(&s), 1);
    ins = om_insert_entry_w(entry_r(&s, &(int){2}),
                            (struct val){.val = 0, .key = 2});
    CHECK(ins != NULL, true);
    CHECK(validate(&s), true);
    CHECK(size(&s), 1);
    ins = om_insert_entry_w(entry_r(&s, &(int){9}),
                            (struct val){.val = 1, .key = 9});
    CHECK(ins != NULL, true);
    CHECK(validate(&s), true);
    CHECK(size(&s), 2);
    ins = ccc_entry_unwrap(
        om_insert_or_assign_w(&s, 3, (struct val){.val = 99}));
    CHECK(validate(&s), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&s), true);
    CHECK(ins->val, 99);
    CHECK(size(&s), 3);
    ins = ccc_entry_unwrap(
        om_insert_or_assign_w(&s, 3, (struct val){.val = 98}));
    CHECK(validate(&s), true);
    CHECK(ins == NULL, false);
    CHECK(ins->val, 98);
    CHECK(size(&s), 3);
    ins = ccc_entry_unwrap(om_try_insert_w(&s, 3, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&s), true);
    CHECK(ins->val, 98);
    CHECK(size(&s), 3);
    ins = ccc_entry_unwrap(om_try_insert_w(&s, 4, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&s), true);
    CHECK(ins->val, 100);
    CHECK(size(&s), 4);
    CHECK_END_FN((void)ccc_om_clear(&s, NULL););
}

CHECK_BEGIN_STATIC_FN(omap_test_struct_getter)
{
    ccc_ordered_map s
        = ccc_om_init(s, struct val, elem, key, NULL, id_cmp, NULL);
    ccc_ordered_map map_tester_clone = ccc_om_init(
        map_tester_clone, struct val, elem, key, NULL, id_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].key = i;
        tester_clone[i].key = i;
        CHECK(insert_entry(entry_r(&s, &vals[i].key), &vals[i].elem) != NULL,
              true);
        CHECK(insert_entry(entry_r(&map_tester_clone, &tester_clone[i].key),
                           &tester_clone[i].elem)
                  != NULL,
              true);
        CHECK(validate(&s), true);
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our
           get to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->key, vals[i].key);
    }
    CHECK(size(&s), (size_t)10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_shuffle)
{
    ccc_ordered_map s
        = ccc_om_init(s, struct val, elem, key, NULL, id_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&s, vals, size, prime), PASS);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &s), size);
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].key, sorted_check[i]);
    }
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(omap_test_insert_one(), omap_test_insert_three(),
                     omap_test_insert_macros(), omap_test_struct_getter(),
                     omap_test_insert_shuffle());
}
