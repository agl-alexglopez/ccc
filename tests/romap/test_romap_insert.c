#define TRAITS_USING_NAMESPACE_CCC
#define REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "realtime_ordered_map.h"
#include "romap_util.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

CHECK_BEGIN_STATIC_FN(romap_test_insert_one)
{
    struct val one = {};
    ccc_realtime_ordered_map s
        = rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    CHECK(occupied(insert_r(&s, &one.elem, &one.elem)), false);
    CHECK(is_empty(&s), false);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_macros)
{
    ccc_realtime_ordered_map s
        = rom_init(struct val, elem, val, s, std_alloc, val_cmp, NULL);
    struct val *v = rom_or_insert_w(entry_r(&s, &(int){0}), (struct val){});
    CHECK(v != NULL, true);
    v = rom_insert_entry_w(entry_r(&s, &(int){0}),
                           (struct val){.val = 0, .id = 99});
    CHECK(validate(&s), true);
    CHECK(v == NULL, false);
    CHECK(v->id, 99);
    v = rom_insert_entry_w(entry_r(&s, &(int){9}),
                           (struct val){.val = 9, .id = 100});
    CHECK(v != NULL, true);
    CHECK(v->id, 100);
    v = unwrap(rom_insert_or_assign_w(&s, 1, (struct val){.id = 100}));
    CHECK(validate(&s), true);
    CHECK(v != NULL, true);
    CHECK(v->id, 100);
    CHECK(size(&s), 3);
    v = unwrap(rom_insert_or_assign_w(&s, 1, (struct val){.id = 99}));
    CHECK(validate(&s), true);
    CHECK(v != NULL, true);
    CHECK(v->id, 99);
    CHECK(size(&s), 3);
    v = unwrap(rom_try_insert_w(&s, 1, (struct val){.id = 2}));
    CHECK(validate(&s), true);
    CHECK(v != NULL, true);
    CHECK(v->id, 99);
    CHECK(size(&s), 3);
    v = unwrap(rom_try_insert_w(&s, 2, (struct val){.id = 2}));
    CHECK(validate(&s), true);
    CHECK(v != NULL, true);
    CHECK(v->id, 2);
    CHECK(size(&s), 4);
    CHECK_END_FN(rom_clear(&s, NULL););
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_shuffle)
{
    ccc_realtime_ordered_map s
        = rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
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
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_weak_srand)
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
        ccc_entry const e = insert(&s, &vals[i].elem, &(struct val){}.elem);
        CHECK(insert_error(&e), false);
        CHECK(validate(&s), true);
    }
    CHECK(size(&s), (size_t)num_nodes);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(romap_test_insert_one(), romap_test_insert_macros(),
                     romap_test_insert_shuffle(),
                     romap_test_insert_weak_srand());
}
