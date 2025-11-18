#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "adaptive_map_utility.h"
#include "ccc/adaptive_map.h"
#include "ccc/traits.h"
#include "ccc/types.h"
#include "checkers.h"

check_static_begin(adaptive_map_test_prime_shuffle)
{
    CCC_Adaptive_map s = CCC_adaptive_map_initialize(struct Val, elem, key,
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
        if (occupied(swap_entry_wrap(&s, &vals[i].elem, &(struct Val){}.elem)))
        {
            repeats[i] = true;
        }
        check(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    check(CCC_adaptive_map_count(&s).count < size, true);
    for (size_t i = 0; i < size; ++i)
    {
        check(occupied(remove_entry_wrap(entry_wrap(&s, &vals[i].key)))
                  || repeats[i],
              true);
        check(validate(&s), true);
    }
    check_end();
}

check_static_begin(adaptive_map_test_insert_erase_shuffled)
{
    CCC_Adaptive_map s = CCC_adaptive_map_initialize(struct Val, elem, key,
                                                     id_order, NULL, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50];
    check(insert_shuffled(&s, vals, size, prime), CHECK_PASS);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &s), size);
    for (size_t i = 0; i < size; ++i)
    {
        check(vals[i].key, sorted_check[i]);
    }
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        struct Val *v = unwrap(remove_wrap(&s, &vals[i].elem));
        check(v != NULL, true);
        check(v->key, vals[i].key);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(adaptive_map_test_weak_srand)
{
    CCC_Adaptive_map s = CCC_adaptive_map_initialize(struct Val, elem, key,
                                                     id_order, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_nodes = 1000;
    struct Val vals[1000];
    bool repeats[1000] = {};
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].key = rand(); // NOLINT
        vals[i].val = i;
        if (occupied(swap_entry_wrap(&s, &vals[i].elem, &(struct Val){}.elem)))
        {
            repeats[i] = true;
        }
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Entry entry = CCC_remove(&s, &vals[i].elem);
        check(occupied(&entry) || repeats[i], true);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(adaptive_map_test_insert_erase_cycles)
{
    CCC_Adaptive_map s = CCC_adaptive_map_initialize(struct Val, elem, key,
                                                     id_order, NULL, NULL);
    srand(time(NULL)); /* NOLINT */
    int const num_nodes = 1000;
    struct Val vals[1000];
    bool repeats[1000] = {};
    for (int i = 0; i < num_nodes; ++i)
    {
        int const rand_i = rand(); /* NOLINT */
        vals[i].key = rand_i;
        vals[i].val = i;
        if (occupied(insert_or_assign_wrap(&s, &vals[i].elem)))
        {
            repeats[i] = true;
        }
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_Entry h = CCC_remove(&s, &vals[i].elem);
        check(occupied(&h) || repeats[i], true);
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_Entry entry = insert_or_assign(&s, &vals[i].elem);
        check(occupied(&entry), false);
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Entry entry = CCC_remove(&s, &vals[i].elem);
        check(occupied(&entry) || repeats[i], true);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

int
main()
{
    return check_run(adaptive_map_test_insert_erase_shuffled(),
                     adaptive_map_test_prime_shuffle(),
                     adaptive_map_test_weak_srand(),
                     adaptive_map_test_insert_erase_cycles());
}
