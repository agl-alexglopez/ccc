#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BOUNDED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "bounded_map.h"
#include "bounded_map_utility.h"
#include "checkers.h"
#include "traits.h"
#include "types.h"

check_static_begin(bounded_map_test_insert_erase_shuffled)
{
    CCC_Bounded_map s = bounded_map_initialize(s, struct Val, elem, key,
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
        struct Val *v = unwrap(remove_r(&s, &vals[i].elem));
        check(v != NULL, true);
        check(v->key, vals[i].key);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(bounded_map_test_prime_shuffle)
{
    CCC_Bounded_map s = bounded_map_initialize(s, struct Val, elem, key,
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
        CCC_Entry e = swap_entry(&s, &vals[i].elem, &(struct Val){}.elem);
        if (unwrap(&e))
        {
            repeats[i] = true;
        }
        check(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* One test can use our printer function as test output */
    check(bounded_map_count(&s).count < size, true);
    for (size_t i = 0; i < size; ++i)
    {
        check(occupied(remove_entry_r(entry_r(&s, &vals[i].key))) || repeats[i],
              true);
        check(validate(&s), true);
    }
    check_end();
}

check_static_begin(bounded_map_test_weak_srand)
{
    CCC_Bounded_map s = bounded_map_initialize(s, struct Val, elem, key,
                                               id_order, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_nodes = 1000;
    struct Val vals[1000];
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].key = rand(); // NOLINT
        vals[i].val = i;
        (void)swap_entry(&s, &vals[i].elem, &(struct Val){}.elem);
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        check(bounded_map_contains(&s, &vals[i].key), true);
        (void)CCC_remove(&s, &vals[i].elem);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

int
main()
{
    return check_run(bounded_map_test_insert_erase_shuffled(),
                     bounded_map_test_prime_shuffle(),
                     bounded_map_test_weak_srand());
}
