#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "ordered_map.h"
#include "ordered_map_util.h"
#include "traits.h"

CHECK_BEGIN_STATIC_FN(ordered_map_test_prime_shuffle)
{
    CCC_Ordered_map s = CCC_ordered_map_initialize(s, struct Val, elem, key,
                                                   id_order, NULL, NULL);
    size_t const size = 50;
    size_t const prime = 53;
    size_t const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    size_t shuffled_index = prime % (size - less);
    struct Val vals[50];
    bool repeats[50];
    memset(repeats, false, sizeof(bool) * size);
    for (size_t i = 0; i < size; ++i)
    {
        vals[i].val = (int)shuffled_index;
        vals[i].key = (int)shuffled_index;
        if (occupied(swap_entry_r(&s, &vals[i].elem, &(struct Val){}.elem)))
        {
            repeats[i] = true;
        }
        CHECK(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    CHECK(CCC_ordered_map_count(&s).count < size, true);
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(occupied(remove_entry_r(entry_r(&s, &vals[i].key))) || repeats[i],
              true);
        CHECK(validate(&s), true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_insert_erase_shuffled)
{
    CCC_Ordered_map s = CCC_ordered_map_initialize(s, struct Val, elem, key,
                                                   id_order, NULL, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50];
    CHECK(insert_shuffled(&s, vals, size, prime), PASS);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &s), size);
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].key, sorted_check[i]);
    }
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        struct Val *v = unwrap(remove_r(&s, &vals[i].elem));
        CHECK(v != NULL, true);
        CHECK(v->key, vals[i].key);
        CHECK(validate(&s), true);
    }
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_weak_srand)
{
    CCC_Ordered_map s = CCC_ordered_map_initialize(s, struct Val, elem, key,
                                                   id_order, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    struct Val vals[1000];
    int const num_nodes = 1000;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].key = rand(); /* NOLINT */
        vals[i].val = i;
        (void)swap_entry(&s, &vals[i].elem, &(struct Val){}.elem);
        CHECK(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CHECK(contains(&s, &vals[i].key), true);
        (void)CCC_remove(&s, &vals[i].elem);
        CHECK(validate(&s), true);
    }
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(ordered_map_test_insert_erase_shuffled(),
                     ordered_map_test_prime_shuffle(),
                     ordered_map_test_weak_srand());
}
