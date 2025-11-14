#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_ADAPTIVE_MAP_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_adaptive_map.h"
#include "handle_adaptive_map_utility.h"
#include "traits.h"
#include "types.h"

check_static_begin(check_range,
                   Handle_adaptive_map const *const handle_adaptive_map,
                   Range const *const r, size_t const n,
                   int const expect_range[])
{
    size_t index = 0;
    struct Val *iterator = range_begin(r);
    for (; iterator != range_end(r) && index < n;
         iterator = next(handle_adaptive_map, iterator), ++index)
    {
        int const cur_id = iterator->id;
        check(expect_range[index], cur_id);
    }
    check(iterator, range_end(r));
    if (iterator != end(handle_adaptive_map))
    {
        check(((struct Val *)iterator)->id, expect_range[n - 1]);
    }
    check_fail_end({
        (void)fprintf(stderr, "%sCHECK: (int[%zu]){", CHECK_GREEN, n);
        for (size_t j = 0; j < n; ++j)
        {
            (void)fprintf(stderr, "%d, ", expect_range[j]);
        }
        (void)fprintf(stderr, "}\n%s", CHECK_NONE);
        (void)fprintf(stderr, "%sCHECK_ERROR:%s (int[%zu]){", CHECK_RED,
                      CHECK_GREEN, n);
        iterator = range_begin(r);
        for (size_t j = 0; j < n && iterator != range_end(r);
             ++j, iterator = next(handle_adaptive_map, iterator))
        {
            if (iterator == end(handle_adaptive_map) || !iterator)
            {
                return CHECK_STATUS;
            }
            if (expect_range[j] == iterator->id)
            {
                (void)fprintf(stderr, "%s%d, %s", CHECK_GREEN, expect_range[j],
                              CHECK_NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", CHECK_RED, iterator->id,
                              CHECK_NONE);
            }
        }
        for (; iterator != range_end(r);
             iterator = next(handle_adaptive_map, iterator))
        {
            (void)fprintf(stderr, "%s%d, %s", CHECK_RED, iterator->id,
                          CHECK_NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", CHECK_GREEN, CHECK_NONE);
    });
}

check_static_begin(check_range_reverse,
                   Handle_adaptive_map const *const handle_adaptive_map,
                   Range_reverse const *const r, size_t const n,
                   int const expect_range_reverse[])
{
    struct Val *iterator = range_reverse_begin(r);
    size_t index = 0;
    for (; iterator != range_reverse_end(r);
         iterator = reverse_next(handle_adaptive_map, iterator))
    {
        int const cur_id = iterator->id;
        check(expect_range_reverse[index], cur_id);
        ++index;
    }
    check(iterator, range_reverse_end(r));
    if (iterator != reverse_end(handle_adaptive_map))
    {
        check(((struct Val *)iterator)->id, expect_range_reverse[n - 1]);
    }
    check_fail_end({
        (void)fprintf(stderr, "%sCHECK: (int[%zu]){", CHECK_GREEN, n);
        size_t j = 0;
        for (; j < n; ++j)
        {
            (void)fprintf(stderr, "%d, ", expect_range_reverse[j]);
        }
        (void)fprintf(stderr, "}\n%s", CHECK_NONE);
        (void)fprintf(stderr, "%sCHECK_ERROR:%s (int[%zu]){", CHECK_RED,
                      CHECK_GREEN, n);
        iterator = range_reverse_begin(r);
        for (j = 0; j < n && iterator != range_reverse_end(r);
             ++j, iterator = reverse_next(handle_adaptive_map, iterator))
        {
            if (iterator == reverse_end(handle_adaptive_map) || !iterator)
            {
                return CHECK_STATUS;
            }
            if (expect_range_reverse[j] == iterator->id)
            {
                (void)fprintf(stderr, "%s%d, %s", CHECK_GREEN,
                              expect_range_reverse[j], CHECK_NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", CHECK_RED, iterator->id,
                              CHECK_NONE);
            }
        }
        for (; iterator != range_reverse_end(r);
             iterator = reverse_next(handle_adaptive_map, iterator))
        {
            (void)fprintf(stderr, "%s%d, %s", CHECK_RED, iterator->id,
                          CHECK_NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", CHECK_GREEN, CHECK_NONE);
    });
}

check_static_begin(iterator_check, Handle_adaptive_map *s)
{
    size_t const size = count(s).count;
    size_t iterator_count = 0;
    for (struct Val *e = begin(s); e != end(s); e = next(s, e))
    {
        ++iterator_count;
        check(iterator_count <= size, true);
    }
    check(iterator_count, size);
    iterator_count = 0;
    for (struct Val *e = reverse_begin(s); e != end(s); e = reverse_next(s, e))
    {
        ++iterator_count;
        check(iterator_count <= size, true);
    }
    check(iterator_count, size);
    check_end();
}

check_static_begin(handle_adaptive_map_test_forward_iterator)
{
    CCC_Handle_adaptive_map s
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct Val *e = begin(&s); e != end(&s); e = next(&s, e), ++j)
    {}
    check(j, 0);
    int const num_nodes = 33;
    int const prime = 37;
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        (void)swap_handle(&s,
                          &(struct Val){.id = (int)shuffled_index, .val = i});
        check(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int keys_inorder[33];
    check(inorder_fill(keys_inorder, num_nodes, &s), count(&s).count);
    j = 0;
    for (struct Val *e = begin(&s); e != end(&s) && j < num_nodes;
         e = next(&s, e), ++j)
    {
        check(e->id, keys_inorder[j]);
    }
    check_end();
}

check_static_begin(handle_adaptive_map_test_iterate_removal)
{
    CCC_Handle_adaptive_map s = handle_adaptive_map_initialize(
        &(standard_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        STANDARD_FIXED_CAP);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(1);
    size_t const num_nodes = 1000;
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. NOLINTNEXTLINE */
        (void)swap_handle(
            &s, &(struct Val){.id = rand() % (num_nodes + 1), .val = (int)i});
        check(validate(&s), true);
    }
    check(iterator_check(&s), CHECK_PASS);
    int const limit = 400;
    size_t cur_node = 0;
    for (struct Val *i = begin(&s), *next = NULL;
         i != end(&s) && cur_node < num_nodes; i = next, ++cur_node)
    {
        next = next(&s, i);
        if (i->id > limit)
        {
            (void)remove(&s, &(struct Val){.id = i->id});
            check(validate(&s), true);
        }
    }
    check_end();
}

check_static_begin(handle_adaptive_map_test_iterate_remove_reinsert)
{
    CCC_Handle_adaptive_map s = handle_adaptive_map_initialize(
        &(standard_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        STANDARD_FIXED_CAP);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. NOLINTNEXTLINE */
        (void)swap_handle(
            &s, &(struct Val){.id = rand() % (num_nodes + 1), .val = (int)i});
        check(validate(&s), true);
    }
    check(iterator_check(&s), CHECK_PASS);
    size_t const old_size = count(&s).count;
    int const limit = 400;
    int new_unique_handle_id = 1001;
    for (struct Val *i = begin(&s), *next = NULL; i != end(&s); i = next)
    {
        next = next(&s, i);
        if (i->id < limit)
        {
            struct Val new_val = {.id = i->id};
            (void)remove(&s, &new_val);
            new_val.id = new_unique_handle_id;
            CCC_Handle e = insert_or_assign(&s, &new_val);
            check(unwrap(&e) != 0, true);
            check(validate(&s), true);
            ++new_unique_handle_id;
        }
    }
    check(count(&s).count, old_size);
    check_end();
}

check_static_begin(handle_adaptive_map_test_valid_range)
{
    CCC_Handle_adaptive_map s
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);

    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct Val){.id = id, .val = i});
        check(validate(&s), true);
    }
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    check(check_range(&s, equal_range_r(&s, &(int){6}, &(int){44}), 8,
                      (int[8]){10, 15, 20, 25, 30, 35, 40, 45}),
          CHECK_PASS);
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    check(check_range_reverse(
              &s, equal_range_reverse_r(&s, &(int){119}, &(int){84}), 8,
              (int[8]){115, 110, 105, 100, 95, 90, 85, 80}),
          CHECK_PASS);
    check_end();
}

check_static_begin(handle_adaptive_map_test_valid_range_equals)
{
    CCC_Handle_adaptive_map s
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct Val){.id = id, .val = i});
        check(validate(&s), true);
    }
    /* This should be the following range [5,50). 5 should stay at the start,
       and 45 is equal to our end key so is bumped to next greater, 50. */
    check(check_range(&s, equal_range_r(&s, &(int){10}, &(int){40}), 8,
                      (int[8]){10, 15, 20, 25, 30, 35, 40, 45}),
          CHECK_PASS);
    /* This should be the following range [115,84). 115 should be
       is a valid start to the range and 85 is eqal to end key so must
       be dropped to first value less than 85, 80. */
    check(check_range_reverse(
              &s, equal_range_reverse_r(&s, &(int){115}, &(int){85}), 8,
              (int[8]){115, 110, 105, 100, 95, 90, 85, 80}),
          CHECK_PASS);
    check_end();
}

check_static_begin(handle_adaptive_map_test_invalid_range)
{
    CCC_Handle_adaptive_map s
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct Val){.id = id, .val = i});
        check(validate(&s), true);
    }
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    check(check_range(&s, equal_range_r(&s, &(int){95}, &(int){999}), 6,
                      (int[6]){95, 100, 105, 110, 115, 120}),
          CHECK_PASS);
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    check(check_range_reverse(
              &s, equal_range_reverse_r(&s, &(int){36}, &(int){-999}), 8,
              (int[8]){35, 30, 25, 20, 15, 10, 5, 0}),
          CHECK_PASS);
    check_end();
}

check_static_begin(handle_adaptive_map_test_empty_range)
{
    CCC_Handle_adaptive_map s
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int const num_nodes = 25;
    int const step = 5;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += step)
    {
        (void)insert_or_assign(&s, &(struct Val){.id = id, .val = i});
        check(validate(&s), true);
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    CCC_Range const forward_range = equal_range(&s, &(int){-50}, &(int){-25});
    check(((struct Val *)range_begin(&forward_range))->id, 0);
    check(((struct Val *)range_end(&forward_range))->id, 0);
    check(range_begin(&forward_range), range_end(&forward_range));
    CCC_Range_reverse const rev_range
        = equal_range_reverse(&s, &(int){150}, &(int){999});
    check(range_reverse_begin(&rev_range), range_reverse_end(&rev_range));
    check(((struct Val *)range_reverse_begin(&rev_range))->id,
          (num_nodes * step) - step);
    check(((struct Val *)range_reverse_end(&rev_range))->id,
          (num_nodes * step) - step);
    check_end();
}

int
main()
{
    return check_run(handle_adaptive_map_test_forward_iterator(),
                     handle_adaptive_map_test_iterate_removal(),
                     handle_adaptive_map_test_valid_range(),
                     handle_adaptive_map_test_valid_range_equals(),
                     handle_adaptive_map_test_invalid_range(),
                     handle_adaptive_map_test_empty_range(),
                     handle_adaptive_map_test_iterate_remove_reinsert());
}
