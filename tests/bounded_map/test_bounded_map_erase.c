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
#include "utility/stack_allocator.h"

check_static_begin(bounded_map_test_insert_erase_shuffled)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 50);
    CCC_Bounded_map s = bounded_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);
    size_t const size = 50;
    int const prime = 53;
    check(insert_shuffled(&s, size, prime), CHECK_PASS);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &s), CHECK_PASS);
    struct Val *const vals = allocator.blocks;
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

check_static_begin(bounded_map_test_prime_shuffle)
{
    CCC_Bounded_map s
        = bounded_map_initialize(struct Val, elem, key, id_order, NULL, NULL);
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
        check(occupied(remove_entry_wrap(entry_wrap(&s, &vals[i].key)))
                  || repeats[i],
              true);
        check(validate(&s), true);
    }
    check_end();
}

check_static_begin(bounded_map_test_weak_srand)
{
    CCC_Bounded_map s
        = bounded_map_initialize(struct Val, elem, key, id_order, NULL, NULL);
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

check_static_begin(bounded_map_test_insert_erase_cycles)
{
    /* Over allocate because we do more insertions near the end. */
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 200);
    CCC_Bounded_map s = CCC_bounded_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);
    srand(time(NULL)); /* NOLINT */
    int const num_nodes = 100;
    int keys[100] = {};
    bool repeats[100] = {};
    for (int i = 0; i < num_nodes; ++i)
    {
        keys[i] = rand(); /* NOLINT */
        if (occupied(insert_or_assign_wrap(&s,
                                           &(struct Val){
                                               .key = keys[i],
                                               .val = i,
                                           }
                                                .elem)))
        {
            repeats[i] = true;
        }
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_Entry h = remove_entry(entry_wrap(&s, &keys[i]));
        check(occupied(&h) || repeats[i], true);
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_Entry const *const entry = CCC_bounded_map_insert_or_assign_with(
            &s, keys[i], (struct Val){.val = i});
        check(occupied(entry), false);
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Entry const entry = remove_entry(entry_wrap(&s, &keys[i]));
        check(occupied(&entry) || repeats[i], true);
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
                     bounded_map_test_weak_srand(),
                     bounded_map_test_insert_erase_cycles());
}
