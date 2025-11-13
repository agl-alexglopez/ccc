#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_realtime_ordered_map.h"
#include "handle_realtime_ordered_map_util.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(
    check_range,
    Handle_realtime_ordered_map const *const handle_realtime_ordered_map,
    Range const *const r, size_t const n, int const expect_range[])
{
    size_t index = 0;
    struct Val *iter = range_begin(r);
    for (; iter != range_end(r) && index < n;
         iter = next(handle_realtime_ordered_map, iter), ++index)
    {
        int const cur_id = iter->id;
        CHECK(expect_range[index], cur_id);
    }
    CHECK(iter, range_end(r));
    if (iter != end(handle_realtime_ordered_map))
    {
        CHECK(((struct Val *)iter)->id, expect_range[n - 1]);
    }
    CHECK_END_FN_FAIL({
        (void)fprintf(stderr, "%sCHECK: (int[%zu]){", GREEN, n);
        for (size_t j = 0; j < n; ++j)
        {
            (void)fprintf(stderr, "%d, ", expect_range[j]);
        }
        (void)fprintf(stderr, "}\n%s", NONE);
        (void)fprintf(stderr, "%sERROR:%s (int[%zu]){", RED, GREEN, n);
        iter = range_begin(r);
        for (size_t j = 0; j < n && iter != range_end(r);
             ++j, iter = next(handle_realtime_ordered_map, iter))
        {
            if (iter == end(handle_realtime_ordered_map) || !iter)
            {
                return CHECK_STATUS;
            }
            if (expect_range[j] == iter->id)
            {
                (void)fprintf(stderr, "%s%d, %s", GREEN, expect_range[j], NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", RED, iter->id, NONE);
            }
        }
        for (; iter != range_end(r);
             iter = next(handle_realtime_ordered_map, iter))
        {
            (void)fprintf(stderr, "%s%d, %s", RED, iter->id, NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", GREEN, NONE);
    });
}

CHECK_BEGIN_STATIC_FN(
    check_rrange,
    Handle_realtime_ordered_map const *const handle_realtime_ordered_map,
    Reverse_range const *const r, size_t const n, int const expect_rrange[])
{
    struct Val *iter = rrange_rbegin(r);
    size_t index = 0;
    for (; iter != rrange_rend(r);
         iter = rnext(handle_realtime_ordered_map, iter))
    {
        int const cur_id = iter->id;
        CHECK(expect_rrange[index], cur_id);
        ++index;
    }
    CHECK(iter, rrange_rend(r));
    if (iter != rend(handle_realtime_ordered_map))
    {
        CHECK(((struct Val *)iter)->id, expect_rrange[n - 1]);
    }
    CHECK_END_FN_FAIL({
        (void)fprintf(stderr, "%sCHECK: (int[%zu]){", GREEN, n);
        size_t j = 0;
        for (; j < n; ++j)
        {
            (void)fprintf(stderr, "%d, ", expect_rrange[j]);
        }
        (void)fprintf(stderr, "}\n%s", NONE);
        (void)fprintf(stderr, "%sERROR:%s (int[%zu]){", RED, GREEN, n);
        iter = rrange_rbegin(r);
        for (j = 0; j < n && iter != rrange_rend(r);
             ++j, iter = rnext(handle_realtime_ordered_map, iter))
        {
            if (iter == rend(handle_realtime_ordered_map) || !iter)
            {
                return CHECK_STATUS;
            }
            if (expect_rrange[j] == iter->id)
            {
                (void)fprintf(stderr, "%s%d, %s", GREEN, expect_rrange[j],
                              NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", RED, iter->id, NONE);
            }
        }
        for (; iter != rrange_rend(r);
             iter = rnext(handle_realtime_ordered_map, iter))
        {
            (void)fprintf(stderr, "%s%d, %s", RED, iter->id, NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", GREEN, NONE);
    });
}

CHECK_BEGIN_STATIC_FN(iterator_check, Handle_realtime_ordered_map *s)
{
    size_t const size = count(s).count;
    size_t iter_count = 0;
    for (struct Val *e = begin(s); e != end(s); e = next(s, e))
    {
        ++iter_count;
        CHECK(iter_count <= size, true);
    }
    CHECK(iter_count, size);
    iter_count = 0;
    for (struct Val *e = rbegin(s); e != end(s); e = rnext(s, e))
    {
        ++iter_count;
        CHECK(iter_count <= size, true);
    }
    CHECK(iter_count, size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_realtime_ordered_map_test_forward_iter)
{
    CCC_Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
        &(small_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        SMALL_FIXED_CAP);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct Val *e = begin(&s); e != end(&s); e = next(&s, e), ++j)
    {}
    CHECK(j, 0);
    int const num_nodes = 33;
    int const prime = 37;
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        (void)swap_handle(&s,
                          &(struct Val){.id = (int)shuffled_index, .val = i});
        CHECK(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int keys_inorder[33];
    CHECK(inorder_fill(keys_inorder, num_nodes, &s), count(&s).count);
    j = 0;
    for (struct Val *e = begin(&s); e != end(&s) && j < num_nodes;
         e = next(&s, e), ++j)
    {
        CHECK(e->id, keys_inorder[j]);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_realtime_ordered_map_test_iterate_removal)
{
    CCC_Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
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
        CHECK(validate(&s), true);
    }
    CHECK(iterator_check(&s), PASS);
    int const limit = 400;
    size_t cur_node = 0;
    for (struct Val *i = begin(&s), *next = NULL;
         i != end(&s) && cur_node < num_nodes; i = next, ++cur_node)
    {
        next = next(&s, i);
        if (i->id > limit)
        {
            (void)remove(&s, &(struct Val){.id = i->id});
            CHECK(validate(&s), true);
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_realtime_ordered_map_test_iterate_remove_reinsert)
{
    CCC_Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
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
        CHECK(validate(&s), true);
    }
    CHECK(iterator_check(&s), PASS);
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
            CHECK(unwrap(&e) != 0, true);
            CHECK(validate(&s), true);
            ++new_unique_handle_id;
        }
    }
    CHECK(count(&s).count, old_size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_realtime_ordered_map_test_valid_range)
{
    CCC_Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
        &(small_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        SMALL_FIXED_CAP);

    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct Val){.id = id, .val = i});
        CHECK(validate(&s), true);
    }
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    CHECK(check_range(&s, equal_range_r(&s, &(int){6}, &(int){44}), 8,
                      (int[8]){10, 15, 20, 25, 30, 35, 40, 45}),
          PASS);
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    CHECK(check_rrange(&s, equal_rrange_r(&s, &(int){119}, &(int){84}), 8,
                       (int[8]){115, 110, 105, 100, 95, 90, 85, 80}),
          PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_realtime_ordered_map_test_valid_range_equals)
{
    CCC_Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
        &(small_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        SMALL_FIXED_CAP);
    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct Val){.id = id, .val = i});
        CHECK(validate(&s), true);
    }
    /* This should be the following range [5,50). 5 should stay at the start,
       and 45 is equal to our end key so is bumped to next greater, 50. */
    CHECK(check_range(&s, equal_range_r(&s, &(int){10}, &(int){40}), 8,
                      (int[8]){10, 15, 20, 25, 30, 35, 40, 45}),
          PASS);
    /* This should be the following range [115,84). 115 should be
       is a valid start to the range and 85 is eqal to end key so must
       be dropped to first value less than 85, 80. */
    CHECK(check_rrange(&s, equal_rrange_r(&s, &(int){115}, &(int){85}), 8,
                       (int[8]){115, 110, 105, 100, 95, 90, 85, 80}),
          PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_realtime_ordered_map_test_invalid_range)
{
    CCC_Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
        &(small_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        SMALL_FIXED_CAP);
    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct Val){.id = id, .val = i});
        CHECK(validate(&s), true);
    }
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    CHECK(check_range(&s, equal_range_r(&s, &(int){95}, &(int){999}), 6,
                      (int[6]){95, 100, 105, 110, 115, 120}),
          PASS);
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    CHECK(check_rrange(&s, equal_rrange_r(&s, &(int){36}, &(int){-999}), 8,
                       (int[8]){35, 30, 25, 20, 15, 10, 5, 0}),
          PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_realtime_ordered_map_test_empty_range)
{
    CCC_Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
        &(small_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        SMALL_FIXED_CAP);
    int const num_nodes = 25;
    int const step = 5;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += step)
    {
        (void)insert_or_assign(&s, &(struct Val){.id = id, .val = i});
        CHECK(validate(&s), true);
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    CCC_Range const forward_range = equal_range(&s, &(int){-50}, &(int){-25});
    CHECK(((struct Val *)range_begin(&forward_range))->id, 0);
    CHECK(((struct Val *)range_end(&forward_range))->id, 0);
    CHECK(range_begin(&forward_range), range_end(&forward_range));
    CCC_Reverse_range const rev_range
        = equal_rrange(&s, &(int){150}, &(int){999});
    CHECK(rrange_rbegin(&rev_range), rrange_rend(&rev_range));
    CHECK(((struct Val *)rrange_rbegin(&rev_range))->id,
          (num_nodes * step) - step);
    CHECK(((struct Val *)rrange_rend(&rev_range))->id,
          (num_nodes * step) - step);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(
        handle_realtime_ordered_map_test_forward_iter(),
        handle_realtime_ordered_map_test_iterate_removal(),
        handle_realtime_ordered_map_test_valid_range(),
        handle_realtime_ordered_map_test_valid_range_equals(),
        handle_realtime_ordered_map_test_invalid_range(),
        handle_realtime_ordered_map_test_empty_range(),
        handle_realtime_ordered_map_test_iterate_remove_reinsert());
}
