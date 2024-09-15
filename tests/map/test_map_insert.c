#define TRAITS_USING_NAMESPACE_CCC

#include "map_util.h"
#include "ordered_map.h"
#include "test.h"
#include "traits.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static enum test_result map_test_insert_one(void);
static enum test_result map_test_insert_three(void);
static enum test_result map_test_struct_getter(void);
static enum test_result map_test_insert_shuffle(void);

#define NUM_TESTS ((size_t)4)
test_fn const all_tests[NUM_TESTS] = {
    map_test_insert_one,
    map_test_insert_three,
    map_test_struct_getter,
    map_test_insert_shuffle,
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        bool const fail = all_tests[i]() == FAIL;
        if (fail)
        {
            res = FAIL;
        }
    }
    return res;
}

static enum test_result
map_test_insert_one(void)
{
    ccc_ordered_map s
        = CCC_OM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    struct val single;
    single.val = 0;
    CHECK(insert_entry(entry_vr(&s, &single.val), &single.elem) != NULL, true);
    CHECK(empty(&s), false);
    CHECK(((struct val *)ccc_om_root(&s))->val == single.val, true);
    return PASS;
}

static enum test_result
map_test_insert_three(void)
{
    ccc_ordered_map s
        = CCC_OM_INIT(struct val, elem, val, s, realloc, val_cmp, NULL);
    struct val swap_slot = {.val = 1, .id = 99};
    CHECK(occupied(insert_vr(&s, &swap_slot.elem, &(struct val){}.elem)),
          false);
    CHECK(ccc_om_validate(&s), true);
    CHECK(size(&s), (size_t)1);
    swap_slot = (struct val){.val = 1, .id = 137};
    struct val const *ins
        = unwrap(insert_vr(&s, &swap_slot.elem, &(struct val){}.elem));
    CHECK(ccc_om_validate(&s), true);
    CHECK(size(&s), (size_t)1);
    CHECK(ins != NULL, true);
    CHECK(ins->val, 1);
    CHECK(ins->id, 137);
    CHECK(swap_slot.val, 1);
    CHECK(swap_slot.id, 99);
    ins = CCC_OM_OR_INSERT(CCC_OM_ENTRY(&s, 2),
                           (struct val){.val = 2, .id = 0});
    CHECK(ins != NULL, true);
    CHECK(ins->id, 0);
    CHECK(ccc_om_validate(&s), true);
    CHECK(size(&s), (size_t)2);
    ins = CCC_OM_INSERT_ENTRY(CCC_OM_ENTRY(&s, 2),
                              (struct val){.val = 2, .id = 1});
    CHECK(ins != NULL, true);
    CHECK(ins->id, 1);
    CHECK(ccc_om_validate(&s), true);
    CHECK(size(&s), (size_t)2);
    ins = CCC_OM_INSERT_ENTRY(CCC_OM_ENTRY(&s, 3), (struct val){.val = 3});
    CHECK(ccc_om_validate(&s), true);
    CHECK(size(&s), (size_t)3);
    ccc_om_clear(&s, NULL);
    return PASS;
}

static enum test_result
map_test_struct_getter(void)
{
    ccc_ordered_map s
        = CCC_OM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    ccc_ordered_map map_tester_clone = CCC_OM_INIT(
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
        CHECK(ccc_om_validate(&s), true);
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our
           get to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val);
    }
    CHECK(size(&s), (size_t)10);
    return PASS;
}

static enum test_result
map_test_insert_shuffle(void)
{
    ccc_ordered_map s
        = CCC_OM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
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
    return PASS;
}
