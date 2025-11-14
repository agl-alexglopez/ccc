#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define BOUNDED_MAP_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC

#include "bounded_map.h"
#include "bounded_map_utility.h"
#include "checkers.h"
#include "traits.h"
#include "types.h"

check_static_begin(check_range, Bounded_map const *const rom,
                   Range const *const r, size_t const n,
                   int const expect_range[])
{
    if (range_begin(r))
    {
        check(((struct Val *)range_begin(r))->key, expect_range[0]);
    }
    if (range_end(r))
    {
        check(((struct Val *)range_end(r))->key, expect_range[n - 1]);
    }
    size_t index = 0;
    struct Val *iter = range_begin(r);
    for (; iter != range_end(r) && index < n;
         iter = next(rom, &iter->elem), ++index)
    {
        int const cur_id = iter->key;
        check(expect_range[index], cur_id);
    }
    check(iter, range_end(r));
    if (iter)
    {
        check(((struct Val *)iter)->key, expect_range[n - 1]);
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
        iter = range_begin(r);
        for (size_t j = 0; j < n && iter != range_end(r);
             ++j, iter = next(rom, &iter->elem))
        {
            if (!iter)
            {
                return CHECK_STATUS;
            }
            if (expect_range[j] == iter->key)
            {
                (void)fprintf(stderr, "%s%d, %s", CHECK_GREEN, expect_range[j],
                              CHECK_NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", CHECK_RED, iter->key,
                              CHECK_NONE);
            }
        }
        for (; iter != range_end(r); iter = next(rom, &iter->elem))
        {
            (void)fprintf(stderr, "%s%d, %s", CHECK_RED, iter->key, CHECK_NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", CHECK_GREEN, CHECK_NONE);
    });
}

check_static_begin(check_range_reverse, Bounded_map const *const rom,
                   Range_reverse const *const r, size_t const n,
                   int const expect_range_reverse[])
{
    if (range_reverse_begin(r))
    {
        check(((struct Val *)range_reverse_begin(r))->key,
              expect_range_reverse[0]);
    }
    if (range_reverse_end(r))
    {
        check(((struct Val *)range_reverse_end(r))->key,
              expect_range_reverse[n - 1]);
    }
    struct Val *iter = range_reverse_begin(r);
    size_t index = 0;
    for (; iter != range_reverse_end(r); iter = reverse_next(rom, &iter->elem))
    {
        int const cur_val = iter->key;
        check(expect_range_reverse[index], cur_val);
        ++index;
    }
    check(iter, range_reverse_end(r));
    if (iter)
    {
        check(((struct Val *)iter)->key, expect_range_reverse[n - 1]);
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
        iter = range_reverse_begin(r);
        for (j = 0; j < n && iter != range_reverse_end(r);
             ++j, iter = reverse_next(rom, &iter->elem))
        {
            if (!iter)
            {
                return CHECK_STATUS;
            }
            if (expect_range_reverse[j] == iter->key)
            {
                (void)fprintf(stderr, "%s%d, %s", CHECK_GREEN,
                              expect_range_reverse[j], CHECK_NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", CHECK_RED, iter->key,
                              CHECK_NONE);
            }
        }
        for (; iter != range_reverse_end(r);
             iter = reverse_next(rom, &iter->elem))
        {
            (void)fprintf(stderr, "%s%d, %s", CHECK_RED, iter->key, CHECK_NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", CHECK_GREEN, CHECK_NONE);
    });
}

check_static_begin(iterator_check, Bounded_map *s)
{
    size_t const size = count(s).count;
    size_t iter_count = 0;
    for (struct Val *e = begin(s); e != end(s); e = next(s, &e->elem))
    {
        ++iter_count;
        check(iter_count <= size, true);
    }
    check(iter_count, size);
    iter_count = 0;
    for (struct Val *e = reverse_begin(s); e != end(s);
         e = reverse_next(s, &e->elem))
    {
        ++iter_count;
        check(iter_count <= size, true);
    }
    check(iter_count, size);
    check_end();
}

check_static_begin(bounded_map_test_forward_iter)
{
    Bounded_map s = bounded_map_initialize(s, struct Val, elem, key, id_order,
                                           NULL, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct Val *e = begin(&s); e != end(&s); e = next(&s, &e->elem), ++j)
    {}
    check(j, 0);
    int const num_nodes = 33;
    int const prime = 37;
    struct Val vals[33];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].key = (int)shuffled_index;
        vals[i].val = i;
        (void)swap_entry(&s, &vals[i].elem, &(struct Val){}.elem);
        check(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[33];
    check(inorder_fill(val_keys_inorder, num_nodes, &s), count(&s).count);
    j = 0;
    for (struct Val *e = begin(&s); e && j < num_nodes;
         e = next(&s, &e->elem), ++j)
    {
        check(e->key, val_keys_inorder[j]);
    }
    check_end();
}

check_static_begin(bounded_map_test_iterate_removal)
{
    Bounded_map s = bounded_map_initialize(s, struct Val, elem, key, id_order,
                                           NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].key = rand() % (num_nodes + 1); // NOLINT
        vals[i].val = (int)i;
        (void)swap_entry(&s, &vals[i].elem, &(struct Val){}.elem);
        check(validate(&s), true);
    }
    check(iterator_check(&s), CHECK_PASS);
    int const limit = 400;
    for (struct Val *i = begin(&s), *next = NULL; i; i = next)
    {
        next = next(&s, &i->elem);
        if (i->key > limit)
        {
            (void)remove(&s, &i->elem);
            check(validate(&s), true);
        }
    }
    check_end();
}

check_static_begin(bounded_map_test_iterate_remove_reinsert)
{
    Bounded_map s = bounded_map_initialize(s, struct Val, elem, key, id_order,
                                           NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].key = rand() % (num_nodes + 1); // NOLINT
        vals[i].val = (int)i;
        (void)swap_entry(&s, &vals[i].elem, &(struct Val){}.elem);
        check(validate(&s), true);
    }
    check(iterator_check(&s), CHECK_PASS);
    size_t const old_size = count(&s).count;
    int const limit = 400;
    int new_unique_entry_val = 1001;
    for (struct Val *i = begin(&s), *next = NULL; i; i = next)
    {
        next = next(&s, &i->elem);
        if (i->key < limit)
        {
            (void)remove(&s, &i->elem);
            i->key = new_unique_entry_val;
            check(insert_entry(entry_r(&s, &i->key), &i->elem) != NULL, true);
            check(validate(&s), true);
            ++new_unique_entry_val;
        }
    }
    check(count(&s).count, old_size);
    check_end();
}

check_static_begin(bounded_map_test_valid_range)
{
    Bounded_map s = bounded_map_initialize(s, struct Val, elem, key, id_order,
                                           NULL, NULL);

    int const num_nodes = 25;
    struct Val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        vals[i].key = id; // NOLINT
        vals[i].val = i;
        (void)swap_entry(&s, &vals[i].elem, &(struct Val){}.elem);
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

check_static_begin(bounded_map_test_valid_range_equals)
{
    Bounded_map s = bounded_map_initialize(s, struct Val, elem, key, id_order,
                                           NULL, NULL);

    int const num_nodes = 25;
    struct Val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        vals[i].key = id; // NOLINT
        vals[i].val = i;
        (void)swap_entry(&s, &vals[i].elem, &(struct Val){}.elem);
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

check_static_begin(bounded_map_test_invalid_range)
{
    Bounded_map s = bounded_map_initialize(s, struct Val, elem, key, id_order,
                                           NULL, NULL);
    int const num_nodes = 25;
    struct Val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        vals[i].key = id; // NOLINT
        vals[i].val = i;
        (void)swap_entry(&s, &vals[i].elem, &(struct Val){}.elem);
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

check_static_begin(bounded_map_test_empty_range)
{
    Bounded_map s = bounded_map_initialize(s, struct Val, elem, key, id_order,
                                           NULL, NULL);
    int const num_nodes = 25;
    struct Val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        vals[i].key = id; // NOLINT
        vals[i].val = i;
        (void)swap_entry(&s, &vals[i].elem, &(struct Val){}.elem);
        check(validate(&s), true);
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    CCC_Range const forward_range = equal_range(&s, &(int){-50}, &(int){-25});
    check(((struct Val *)range_begin(&forward_range))->key, vals[0].key);
    check(((struct Val *)range_end(&forward_range))->key, vals[0].key);
    CCC_Range_reverse const rev_range
        = equal_range_reverse(&s, &(int){150}, &(int){999});
    check(((struct Val *)range_reverse_begin(&rev_range))->key,
          vals[num_nodes - 1].key);
    check(((struct Val *)range_reverse_end(&rev_range))->key,
          vals[num_nodes - 1].key);
    check_end();
}

int
main()
{
    return check_run(
        bounded_map_test_forward_iter(), bounded_map_test_iterate_removal(),
        bounded_map_test_valid_range(), bounded_map_test_valid_range_equals(),
        bounded_map_test_invalid_range(), bounded_map_test_empty_range(),
        bounded_map_test_iterate_remove_reinsert());
}
