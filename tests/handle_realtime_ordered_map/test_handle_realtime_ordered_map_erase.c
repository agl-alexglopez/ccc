#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_realtime_ordered_map.h"
#include "handle_realtime_ordered_map_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

check_static_begin(handle_realtime_ordered_map_test_insert_erase_shuffled)
{
    CCC_Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
        &(small_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        SMALL_FIXED_CAP);
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
        CCC_Handle const *const h = remove_r(&s, &(struct Val){.id = (int)i});
        check(occupied(h), true);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(handle_realtime_ordered_map_test_prime_shuffle)
{
    CCC_Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
        &(small_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        SMALL_FIXED_CAP);
    size_t const size = 50;
    size_t const prime = 53;
    size_t const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    size_t shuffled_index = prime % (size - less);
    bool repeats[50];
    memset(repeats, false, sizeof(bool) * size);
    for (size_t i = 0; i < size; ++i)
    {
        if (occupied(
                try_insert_r(&s, &(struct Val){.id = (int)shuffled_index,
                                               .val = (int)shuffled_index})))
        {
            repeats[i] = true;
        }
        check(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    check(handle_realtime_ordered_map_count(&s).count < size, true);
    for (size_t i = 0; i < size; ++i)
    {
        CCC_Handle const *const e = remove_handle_r(handle_r(&s, &i));
        check(occupied(e) || repeats[i], true);
        check(validate(&s), true);
    }
    check_end();
}

check_static_begin(handle_realtime_ordered_map_test_weak_srand)
{
    CCC_Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
        &(standard_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        STANDARD_FIXED_CAP);
    srand(time(NULL)); /* NOLINT */
    int const num_nodes = 1000;
    int id_keys[1000];
    for (int i = 0; i < num_nodes; ++i)
    {
        int const rand_i = rand(); /* NOLINT */
        (void)swap_handle(&s, &(struct Val){.id = rand_i, .val = i});
        id_keys[i] = rand_i;
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Handle const h = CCC_remove(&s, &(struct Val){.id = id_keys[i]});
        check(occupied(&h), true);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(
    handle_realtime_ordered_map_test_insert_erase_cycles_no_allocate)
{
    CCC_Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
        &(standard_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        STANDARD_FIXED_CAP);
    srand(time(NULL)); /* NOLINT */
    int const num_nodes = 1000;
    int id_keys[1000];
    for (int i = 0; i < num_nodes; ++i)
    {
        int const rand_i = rand(); /* NOLINT */
        (void)insert_or_assign(&s, &(struct Val){.id = rand_i, .val = i});
        id_keys[i] = rand_i;
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_Handle h = CCC_remove(&s, &(struct Val){.id = id_keys[i]});
        check(occupied(&h), true);
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
        check(occupied(&h), true);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(
    handle_realtime_ordered_map_test_insert_erase_cycles_allocate)
{
    CCC_Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    srand(time(NULL)); /* NOLINT */
    int const num_nodes = 1000;
    int id_keys[1000];
    for (int i = 0; i < num_nodes; ++i)
    {
        int const rand_i = rand(); /* NOLINT */
        (void)insert_or_assign(&s, &(struct Val){.id = rand_i, .val = i});
        id_keys[i] = rand_i;
        check(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_Handle h = CCC_remove(&s, &(struct Val){.id = id_keys[i]});
        check(occupied(&h), true);
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
        check(occupied(&h), true);
        check(validate(&s), true);
    }
    check(is_empty(&s), true);
    check_end(handle_realtime_ordered_map_clear_and_free(&s, NULL););
}

int
main()
{
    return check_run(
        handle_realtime_ordered_map_test_insert_erase_shuffled(),
        handle_realtime_ordered_map_test_prime_shuffle(),
        handle_realtime_ordered_map_test_weak_srand(),
        handle_realtime_ordered_map_test_insert_erase_cycles_no_allocate(),
        handle_realtime_ordered_map_test_insert_erase_cycles_allocate());
}
