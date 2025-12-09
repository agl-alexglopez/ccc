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
#include "utility/stack_allocator.h"

check_static_begin(adaptive_map_test_prime_shuffle)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 50);
    CCC_Adaptive_map s = CCC_adaptive_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);
    size_t const size = 50;
    size_t const prime = 53;
    size_t const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    size_t shuffled_index = prime % (size - less);
    bool repeats[50] = {};
    for (size_t i = 0; i < size; ++i)
    {
        if (occupied(insert_or_assign_wrap(&s,
                                           &(struct Val){
                                               .val = (int)shuffled_index,
                                               .key = (int)shuffled_index,
                                           }
                                                .elem)))
        {
            repeats[i] = true;
        }
        check(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    check(CCC_adaptive_map_count(&s).count < size, true);
    struct Val *const vals = allocator.blocks;
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
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 50);
    CCC_Adaptive_map s = CCC_adaptive_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);
    size_t const size = 50;
    int const prime = 53;
    check(insert_shuffled(&s, size, prime), CHECK_PASS);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &s), CHECK_PASS);
    /* Now let's delete everything with no errors. */
    struct Val *const vals = allocator.blocks;
    for (size_t i = 0; i < size; ++i)
    {
        struct Val *v = unwrap(remove_key_value_wrap(&s, &vals[i].elem));
        check(v != NULL, true);
        check(v->key, vals[i].key);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(adaptive_map_test_weak_srand)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 100);
    CCC_Adaptive_map s = CCC_adaptive_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_nodes = 100;
    bool repeats[100] = {};
    for (int i = 0; i < num_nodes; ++i)
    {
        if (occupied(insert_or_assign_wrap(&s,
                                           &(struct Val){
                                               .key = rand(), /* NOLINT */
                                               .val = i,
                                           }
                                                .elem)))
        {
            repeats[i] = true;
        }
        check(validate(&s), true);
    }
    struct Val *const vals = allocator.blocks;
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Entry entry = CCC_remove_key_value(&s, &vals[i].elem);
        check(occupied(&entry) || repeats[i], true);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(adaptive_map_test_insert_erase_cycles)
{
    /* Over allocate because we do more insertions near the end. */
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 200);
    CCC_Adaptive_map s = CCC_adaptive_map_initialize(
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
        CCC_Entry const *const entry = CCC_adaptive_map_insert_or_assign_with(
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
    return check_run(adaptive_map_test_insert_erase_shuffled(),
                     adaptive_map_test_prime_shuffle(),
                     adaptive_map_test_weak_srand(),
                     adaptive_map_test_insert_erase_cycles());
}
