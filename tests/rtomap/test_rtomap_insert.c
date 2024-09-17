#define TRAITS_USING_NAMESPACE_CCC
#define REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC

#include "realtime_ordered_map.h"
#include "rtomap_util.h"
#include "test.h"
#include "traits.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static enum test_result rtomap_test_insert_one(void);
static enum test_result rtomap_test_insert_macros(void);
static enum test_result rtomap_test_insert_shuffle(void);
static enum test_result rtomap_test_insert_weak_srand(void);

#define NUM_TESTS ((size_t)4)
test_fn const all_tests[NUM_TESTS] = {
    rtomap_test_insert_one,
    rtomap_test_insert_macros,
    rtomap_test_insert_shuffle,
    rtomap_test_insert_weak_srand,
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
rtomap_test_insert_one(void)
{
    ccc_realtime_ordered_map s
        = rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    CHECK(occupied(insert_vr(&s, &(struct val){}.elem, &(struct val){}.elem)),
          false);
    CHECK(rom_empty(&s), false);
    struct val *v = rom_root(&s);
    CHECK(v == NULL, false);
    CHECK(v->val, 0);
    return PASS;
}

static enum test_result
rtomap_test_insert_macros(void)
{
    ccc_realtime_ordered_map s
        = rom_init(struct val, elem, val, s, realloc, val_cmp, NULL);
    struct val *v = rom_or_insert_w(entry_vr(&s, &(int){0}), (struct val){});
    CHECK(v != NULL, true);
    CHECK(size(&s), 1);
    v = rom_insert_entry_w(ccc_rom_entry_vr(&s, &(int){0}),
                           (struct val){.val = 0, .id = 99});
    CHECK(v != NULL, true);
    CHECK(v->val, 0);
    CHECK(v->id, 99);
    CHECK(size(&s), 1);
    v = unwrap(rom_insert_or_assign_w(&s, 0, (struct val){.id = 100}));
    CHECK(v != NULL, true);
    v->id++;
    CHECK(v->id, 101);
    CHECK(size(&s), 1);
    v = unwrap(rom_try_insert_w(&s, 0, (struct val){.id = 3}));
    CHECK(v != NULL, true);
    CHECK(v->id, 101);
    CHECK(size(&s), 1);
    v = unwrap(rom_try_insert_w(&s, 1, (struct val){.id = 2}));
    CHECK(v != NULL, true);
    CHECK(size(&s), 2);
    CHECK(v->val, 1);
    CHECK(v->id, 2);
    v = unwrap(rom_insert_or_assign_w(&s, 2, (struct val){.id = 77}));
    CHECK(v != NULL, true);
    CHECK(size(&s), 3);
    CHECK(v->val, 2);
    CHECK(v->id, 77);
    v = rom_insert_entry_w(ccc_rom_entry_vr(&s, &(int){88}),
                           (struct val){.val = 88, .id = 65});
    CHECK(size(&s), 4);
    rom_clear_and_free(&s, NULL);
    return PASS;
}

static enum test_result
rtomap_test_insert_shuffle(void)
{
    ccc_realtime_ordered_map s
        = rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&s, vals, size, prime), PASS);

    rom_print(&s, map_printer_fn);
    printf("\n");

    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &s), size);
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i]);
    }
    return PASS;
}

static enum test_result
rtomap_test_insert_weak_srand(void)
{
    ccc_realtime_ordered_map s
        = rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_nodes = 1000;
    struct val vals[1000];
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        ccc_entry const e = insert(&s, &vals[i].elem, &(struct val){});
        CHECK(insert_error(&e), false);
        CHECK(rom_validate(&s), true);
    }
    CHECK(size(&s), (size_t)num_nodes);
    return PASS;
}
