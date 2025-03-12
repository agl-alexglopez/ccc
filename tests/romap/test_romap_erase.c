#define REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "realtime_ordered_map.h"
#include "romap_util.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

CHECK_BEGIN_STATIC_FN(romap_test_insert_erase_shuffled)
{
    ccc_realtime_ordered_map s
        = rom_init(s, struct val, elem, key, id_cmp, NULL, NULL);
    ptrdiff_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&s, vals, size, prime), PASS);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &s), size);
    for (ptrdiff_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].key, sorted_check[i]);
    }
    /* Now let's delete everything with no errors. */
    for (ptrdiff_t i = 0; i < size; ++i)
    {
        struct val *v = unwrap(remove_r(&s, &vals[i].elem));
        CHECK(v != NULL, true);
        CHECK(v->key, vals[i].key);
        CHECK(validate(&s), true);
    }
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_prime_shuffle)
{
    ccc_realtime_ordered_map s
        = rom_init(s, struct val, elem, key, id_cmp, NULL, NULL);
    ptrdiff_t const size = 50;
    ptrdiff_t const prime = 53;
    ptrdiff_t const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    ptrdiff_t shuffled_index = prime % (size - less);
    struct val vals[50];
    bool repeats[50];
    memset(repeats, false, sizeof(bool) * size);
    for (ptrdiff_t i = 0; i < size; ++i)
    {
        vals[i].val = (int)shuffled_index;
        vals[i].key = (int)shuffled_index;
        ccc_entry e = swap_entry(&s, &vals[i].elem, &(struct val){}.elem);
        if (unwrap(&e))
        {
            repeats[i] = true;
        }
        CHECK(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* One test can use our printer function as test output */
    CHECK(rom_size(&s) < size, true);
    for (ptrdiff_t i = 0; i < size; ++i)
    {
        CHECK(occupied(remove_entry_r(entry_r(&s, &vals[i].key))) || repeats[i],
              true);
        CHECK(validate(&s), true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_weak_srand)
{
    ccc_realtime_ordered_map s
        = rom_init(s, struct val, elem, key, id_cmp, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_nodes = 1000;
    struct val vals[1000];
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].key = rand(); // NOLINT
        vals[i].val = i;
        (void)swap_entry(&s, &vals[i].elem, &(struct val){}.elem);
        CHECK(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CHECK(rom_contains(&s, &vals[i].key), true);
        (void)remove(&s, &vals[i].elem);
        CHECK(validate(&s), true);
    }
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(romap_test_insert_erase_shuffled(),
                     romap_test_prime_shuffle(), romap_test_weak_srand());
}
