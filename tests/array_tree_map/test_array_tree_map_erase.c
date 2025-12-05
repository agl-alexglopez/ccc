#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ARRAY_TREE_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "array_tree_map.h"
#include "array_tree_map_utility.h"
#include "checkers.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

check_static_begin(array_tree_map_test_insert_erase_shuffled)
{
    CCC_Array_tree_map s
        = array_tree_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                    id_order, NULL, NULL, SMALL_FIXED_CAP);
    size_t const size = 50;
    int const prime = 53;
    check(insert_shuffled(&s, size, prime), CHECK_PASS);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &s), size);
    for (size_t i = 0; i + 1 < size; ++i)
    {
        check(sorted_check[i] <= sorted_check[i + 1], true);
    }
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        CCC_Handle const *const h
            = remove_wrap(&s, &(struct Val){.id = (int)i});
        check(occupied(h), true);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(array_tree_map_test_prime_shuffle)
{
    CCC_Array_tree_map s
        = array_tree_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                    id_order, NULL, NULL, SMALL_FIXED_CAP);
    size_t const size = 50;
    size_t const prime = 53;
    size_t const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    size_t shuffled_index = prime % (size - less);
    bool repeats[50] = {};
    for (size_t i = 0; i < size; ++i)
    {
        if (occupied(
                try_insert_wrap(&s, &(struct Val){.id = (int)shuffled_index,
                                                  .val = (int)shuffled_index})))
        {
            repeats[i] = true;
        }
        check(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    check(array_tree_map_count(&s).count < size, true);
    for (size_t i = 0; i < size; ++i)
    {
        CCC_Handle const *const e = remove_handle_wrap(handle_wrap(&s, &i));
        check(occupied(e) || repeats[i], true);
        check(validate(&s), true);
    }
    check_end();
}

check_static_begin(array_tree_map_test_weak_srand)
{
    CCC_Array_tree_map s
        = array_tree_map_initialize(&(Standard_fixed_map){}, struct Val, id,
                                    id_order, NULL, NULL, STANDARD_FIXED_CAP);
    srand(time(NULL)); /* NOLINT */
    int const num_nodes = 1000;
    int id_keys[1000];
    bool repeats[1000] = {};
    for (int i = 0; i < num_nodes; ++i)
    {
        int const rand_i = rand(); /* NOLINT */
        if (occupied(try_insert_wrap(&s, &(struct Val){
                                             .id = rand_i,
                                             .val = i,
                                         })))
        {
            repeats[i] = true;
        }
        (void)swap_handle(&s, &(struct Val){.id = rand_i, .val = i});
        id_keys[i] = rand_i;
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Handle const h = CCC_remove(&s, &(struct Val){.id = id_keys[i]});
        check(occupied(&h) || repeats[i], true);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(array_tree_map_test_insert_erase_cycles_no_allocate)
{
    CCC_Array_tree_map s
        = array_tree_map_initialize(&(Standard_fixed_map){}, struct Val, id,
                                    id_order, NULL, NULL, STANDARD_FIXED_CAP);
    srand(time(NULL)); /* NOLINT */
    int const num_nodes = 1000;
    int id_keys[1000];
    bool repeats[1000] = {};
    for (int i = 0; i < num_nodes; ++i)
    {
        int const rand_i = rand(); /* NOLINT */
        if (occupied(insert_or_assign_wrap(&s, &(struct Val){
                                                   .id = rand_i,
                                                   .val = i,
                                               })))
        {
            repeats[i] = true;
        }
        id_keys[i] = rand_i;
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_Handle h = CCC_remove(&s, &(struct Val){.id = id_keys[i]});
        check(occupied(&h) || repeats[i], true);
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_Handle h = insert_or_assign(&s, &(struct Val){.id = id_keys[i]});
        check(occupied(&h), false);
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Handle h = CCC_remove(&s, &(struct Val){.id = id_keys[i]});
        check(occupied(&h) || repeats[i], true);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

/** Make sure this test uses standard library allocator. Resizing is important
to test for handle maps. Stack allocator does not allow resizing. */
check_static_begin(array_tree_map_test_insert_erase_cycles_allocate)
{
    CCC_Array_tree_map s = array_tree_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    srand(time(NULL)); /* NOLINT */
    int const num_nodes = 1000;
    int id_keys[1000];
    bool repeats[1000] = {};
    for (int i = 0; i < num_nodes; ++i)
    {
        int const rand_i = rand(); /* NOLINT */
        if (occupied(insert_or_assign_wrap(&s, &(struct Val){
                                                   .id = rand_i,
                                                   .val = i,
                                               })))
        {
            repeats[i] = true;
        }
        id_keys[i] = rand_i;
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_Handle h = CCC_remove(&s, &(struct Val){.id = id_keys[i]});
        check(occupied(&h) || repeats[i], true);
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_Handle h = insert_or_assign(&s, &(struct Val){.id = id_keys[i]});
        check(occupied(&h), false);
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Handle h = CCC_remove(&s, &(struct Val){.id = id_keys[i]});
        check(occupied(&h) || repeats[i], true);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end(array_tree_map_clear_and_free(&s, NULL););
}

int
main()
{
    return check_run(array_tree_map_test_insert_erase_shuffled(),
                     array_tree_map_test_prime_shuffle(),
                     array_tree_map_test_weak_srand(),
                     array_tree_map_test_insert_erase_cycles_no_allocate(),
                     array_tree_map_test_insert_erase_cycles_allocate());
}
