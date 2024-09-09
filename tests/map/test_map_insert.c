#include "map_util.h"
#include "ordered_map.h"
#include "test.h"

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
    CHECK(ccc_om_insert_entry(ccc_om_entry(&s, &single.val), &single.elem)
              != NULL,
          true, "%d");
    CHECK(ccc_om_empty(&s), false, "%d");
    CHECK(((struct val *)ccc_om_root(&s))->val == single.val, true, "%d");
    return PASS;
}

static enum test_result
map_test_insert_three(void)
{
    ccc_ordered_map s
        = CCC_OM_INIT(struct val, elem, val, s, realloc, val_cmp, NULL);
    struct val swap_slot = {.val = 1, .id = 99};
    CHECK(ccc_entry_occupied(ccc_om_insert(&s, &swap_slot.elem)), false, "%d");
    CHECK(ccc_om_validate(&s), true, "%d");
    CHECK(ccc_om_size(&s), (size_t)1, "%zu");
    swap_slot = (struct val){.val = 1, .id = 137};
    struct val const *ins
        = ccc_entry_unwrap(ccc_om_insert(&s, &swap_slot.elem));
    CHECK(ccc_om_validate(&s), true, "%d");
    CHECK(ccc_om_size(&s), (size_t)1, "%zu");
    CHECK(ins != NULL, true, "%d");
    CHECK(ins->val, 1, "%d");
    CHECK(ins->id, 137, "%d");
    CHECK(swap_slot.val, 1, "%d");
    CHECK(swap_slot.id, 99, "%d");
    ins = CCC_OM_OR_INSERT(CCC_OM_ENTRY(&s, 2),
                           (struct val){.val = 2, .id = 0});
    CHECK(ins != NULL, true, "%d");
    CHECK(ins->id, 0, "%d");
    CHECK(ccc_om_validate(&s), true, "%d");
    CHECK(ccc_om_size(&s), (size_t)2, "%zu");
    ins = CCC_OM_INSERT_ENTRY(CCC_OM_ENTRY(&s, 2),
                              (struct val){.val = 2, .id = 1});
    CHECK(ins != NULL, true, "%d");
    CHECK(ins->id, 1, "%d");
    CHECK(ccc_om_validate(&s), true, "%d");
    CHECK(ccc_om_size(&s), (size_t)2, "%zu");
    ins = CCC_OM_INSERT_ENTRY(CCC_OM_ENTRY(&s, 3), (struct val){.val = 3});
    CHECK(ccc_om_validate(&s), true, "%d");
    CHECK(ccc_om_size(&s), (size_t)3, "%zu");
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
        CHECK(ccc_om_insert_entry(ccc_om_entry(&s, &vals[i].val), &vals[i].elem)
                  != NULL,
              true, "%d");
        CHECK(ccc_om_insert_entry(
                  ccc_om_entry(&map_tester_clone, &tester_clone[i].val),
                  &tester_clone[i].elem)
                  != NULL,
              true, "%d");
        CHECK(ccc_om_validate(&s), true, "%d");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our
           get to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val, "%d");
    }
    CHECK(ccc_om_size(&s), (size_t)10, "%zu");
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
    struct val vals[size];
    CHECK(insert_shuffled(&s, vals, size, prime), PASS, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &s), size, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], "%d");
    }
    return PASS;
}
