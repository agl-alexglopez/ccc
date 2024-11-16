#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_realtime_ordered_map.h"
#include "fromap_util.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

CHECK_BEGIN_STATIC_FN(check_range, flat_realtime_ordered_map const *const frm,
                      range const *const r, size_t const n,
                      int const expect_range[])
{
    size_t index = 0;
    struct val *iter = begin_range(r);
    for (; iter != end_range(r) && index < n;
         iter = next(frm, &iter->elem), ++index)
    {
        int const cur_id = iter->id;
        CHECK(expect_range[index], cur_id);
    }
    CHECK(iter, end_range(r));
    if (iter != end(frm))
    {
        CHECK(((struct val *)iter)->id, expect_range[n - 1]);
    }
    CHECK_END_FN_FAIL({
        (void)fprintf(stderr, "%sCHECK: (int[%zu]){", GREEN, n);
        for (size_t j = 0; j < n; ++j)
        {
            (void)fprintf(stderr, "%d, ", expect_range[j]);
        }
        (void)fprintf(stderr, "}\n%s", NONE);
        (void)fprintf(stderr, "%sERROR:%s (int[%zu]){", RED, GREEN, n);
        iter = begin_range(r);
        for (size_t j = 0; j < n && iter != end_range(r);
             ++j, iter = next(frm, &iter->elem))
        {
            if (iter == end(frm) || !iter)
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
        for (; iter != end_range(r); iter = next(frm, &iter->elem))
        {
            (void)fprintf(stderr, "%s%d, %s", RED, iter->id, NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", GREEN, NONE);
    });
}

CHECK_BEGIN_STATIC_FN(check_rrange, flat_realtime_ordered_map const *const frm,
                      rrange const *const r, size_t const n,
                      int const expect_rrange[])
{
    struct val *iter = rbegin_rrange(r);
    size_t index = 0;
    for (; iter != rend_rrange(r); iter = rnext(frm, &iter->elem))
    {
        int const cur_id = iter->id;
        CHECK(expect_rrange[index], cur_id);
        ++index;
    }
    CHECK(iter, rend_rrange(r));
    if (iter != rend(frm))
    {
        CHECK(((struct val *)iter)->id, expect_rrange[n - 1]);
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
        iter = rbegin_rrange(r);
        for (j = 0; j < n && iter != rend_rrange(r);
             ++j, iter = rnext(frm, &iter->elem))
        {
            if (iter == rend(frm) || !iter)
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
        for (; iter != rend_rrange(r); iter = rnext(frm, &iter->elem))
        {
            (void)fprintf(stderr, "%s%d, %s", RED, iter->id, NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", GREEN, NONE);
    });
}

CHECK_BEGIN_STATIC_FN(iterator_check, flat_realtime_ordered_map *s)
{
    size_t const size = size(s);
    size_t iter_count = 0;
    for (struct val *e = begin(s); e != end(s); e = next(s, &e->elem))
    {
        ++iter_count;
        CHECK(iter_count <= size, true);
    }
    CHECK(iter_count, size);
    iter_count = 0;
    for (struct val *e = rbegin(s); e != end(s); e = rnext(s, &e->elem))
    {
        ++iter_count;
        CHECK(iter_count <= size, true);
    }
    CHECK(iter_count, size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_forward_iter)
{
    flat_realtime_ordered_map s
        = frm_init((struct val[34]){}, 34, elem, id, NULL, id_cmp, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct val *e = begin(&s); e != end(&s); e = next(&s, &e->elem), ++j)
    {}
    CHECK(j, 0);
    int const num_nodes = 33;
    int const prime = 37;
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        (void)insert(&s,
                     &(struct val){.id = (int)shuffled_index, .val = i}.elem);
        CHECK(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int keys_inorder[33];
    CHECK(inorder_fill(keys_inorder, num_nodes, &s), size(&s));
    j = 0;
    for (struct val *e = begin(&s); e != end(&s) && j < num_nodes;
         e = next(&s, &e->elem), ++j)
    {
        CHECK(e->id, keys_inorder[j]);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_iterate_removal)
{
    flat_realtime_ordered_map s
        = frm_init((struct val[1001]){}, 1001, elem, id, NULL, id_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. NOLINTNEXTLINE */
        (void)insert(
            &s,
            &(struct val){.id = rand() % (num_nodes + 1), .val = (int)i}.elem);
        CHECK(validate(&s), true);
    }
    CHECK(iterator_check(&s), PASS);
    int const limit = 400;
    for (struct val *i = begin(&s), *next = NULL; i != end(&s); i = next)
    {
        next = next(&s, &i->elem);
        if (i->id > limit)
        {
            (void)remove(&s, &(struct val){.id = i->id}.elem);
            CHECK(validate(&s), true);
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_iterate_remove_reinsert)
{
    flat_realtime_ordered_map s
        = frm_init((struct val[1001]){}, 1001, elem, id, NULL, id_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. NOLINTNEXTLINE */
        (void)insert(
            &s,
            &(struct val){.id = rand() % (num_nodes + 1), .val = (int)i}.elem);
        CHECK(validate(&s), true);
    }
    CHECK(iterator_check(&s), PASS);
    size_t const old_size = size(&s);
    int const limit = 400;
    int new_unique_entry_id = 1001;
    for (struct val *i = begin(&s), *next = NULL; i != end(&s); i = next)
    {
        next = next(&s, &i->elem);
        if (i->id < limit)
        {
            struct val new_val = {.id = i->id};
            (void)remove(&s, &new_val.elem);
            new_val.id = new_unique_entry_id;
            ccc_entry e = insert_or_assign(&s, &new_val.elem);
            CHECK(unwrap(&e) != NULL, true);
            CHECK(validate(&s), true);
            ++new_unique_entry_id;
        }
    }
    CHECK(size(&s), old_size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_valid_range)
{
    flat_realtime_ordered_map s
        = frm_init((struct val[26]){}, 26, elem, id, NULL, id_cmp, NULL);

    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct val){.id = id, .val = i}.elem);
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

CHECK_BEGIN_STATIC_FN(fromap_test_valid_range_equals)
{
    flat_realtime_ordered_map s
        = frm_init((struct val[26]){}, 26, elem, id, NULL, id_cmp, NULL);
    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct val){.id = id, .val = i}.elem);
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

CHECK_BEGIN_STATIC_FN(fromap_test_invalid_range)
{
    flat_realtime_ordered_map s
        = frm_init((struct val[26]){}, 26, elem, id, NULL, id_cmp, NULL);
    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct val){.id = id, .val = i}.elem);
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

CHECK_BEGIN_STATIC_FN(fromap_test_empty_range)
{
    flat_realtime_ordered_map s
        = frm_init((struct val[26]){}, 26, elem, id, NULL, id_cmp, NULL);
    int const num_nodes = 25;
    int const step = 5;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += step)
    {
        (void)insert_or_assign(&s, &(struct val){.id = id, .val = i}.elem);
        CHECK(validate(&s), true);
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    ccc_range const forward_range = equal_range(&s, &(int){-50}, &(int){-25});
    CHECK(((struct val *)begin_range(&forward_range))->id, 0);
    CHECK(((struct val *)end_range(&forward_range))->id, 0);
    CHECK(begin_range(&forward_range), end_range(&forward_range));
    ccc_rrange const rev_range = equal_rrange(&s, &(int){150}, &(int){999});
    CHECK(rbegin_rrange(&rev_range), rend_rrange(&rev_range));
    CHECK(((struct val *)rbegin_rrange(&rev_range))->id,
          (num_nodes * step) - step);
    CHECK(((struct val *)rend_rrange(&rev_range))->id,
          (num_nodes * step) - step);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(fromap_test_forward_iter(), fromap_test_iterate_removal(),
                     fromap_test_valid_range(),
                     fromap_test_valid_range_equals(),
                     fromap_test_invalid_range(), fromap_test_empty_range(),
                     fromap_test_iterate_remove_reinsert());
}
