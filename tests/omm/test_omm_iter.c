#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC
#define ORDERED_MULTIMAP_USING_NAMESPACE_CCC

#include "omm_util.h"
#include "ordered_multimap.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

BEGIN_STATIC_TEST(check_range, ordered_multimap const *const rom,
                  range const *const r, size_t const n,
                  int const expect_range[])
{
    if (begin_range(r))
    {
        CHECK(((struct val *)begin_range(r))->val, expect_range[0]);
    }
    if (end_range(r))
    {
        CHECK(((struct val *)end_range(r))->val, expect_range[n - 1]);
    }
    size_t index = 0;
    struct val *iter = begin_range(r);
    for (; iter != end_range(r) && index < n;
         iter = next(rom, &iter->elem), ++index)
    {
        int const cur_val = iter->val;
        CHECK(expect_range[index], cur_val);
    }
    CHECK(iter, end_range(r));
    if (iter)
    {
        CHECK(((struct val *)iter)->val, expect_range[n - 1]);
    }
    END_FAIL({
        (void)fprintf(stderr, "%sCHECK: (int[%zu]){", GREEN, n);
        for (size_t j = 0; j < n; ++j)
        {
            (void)fprintf(stderr, "%d, ", expect_range[j]);
        }
        (void)fprintf(stderr, "}\n%s", NONE);
        (void)fprintf(stderr, "%sERROR:%s (int[%zu]){", RED, GREEN, n);
        iter = begin_range(r);
        for (size_t j = 0; j < n && iter != end_range(r);
             ++j, iter = next(rom, &iter->elem))
        {
            if (!iter)
            {
                return TEST_STATUS;
            }
            if (expect_range[j] == iter->val)
            {
                (void)fprintf(stderr, "%s%d, %s", GREEN, expect_range[j], NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", RED, iter->val, NONE);
            }
        }
        for (; iter != end_range(r); iter = next(rom, &iter->elem))
        {
            (void)fprintf(stderr, "%s%d, %s", RED, iter->val, NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", GREEN, NONE);
    });
}

BEGIN_STATIC_TEST(check_rrange, ordered_multimap const *const rom,
                  rrange const *const r, size_t const n,
                  int const expect_rrange[])
{
    if (rbegin_rrange(r))
    {
        CHECK(((struct val *)rbegin_rrange(r))->val, expect_rrange[0]);
    }
    if (rend_rrange(r))
    {
        CHECK(((struct val *)rend_rrange(r))->val, expect_rrange[n - 1]);
    }
    struct val *iter = rbegin_rrange(r);
    size_t index = 0;
    for (; iter != rend_rrange(r); iter = rnext(rom, &iter->elem))
    {
        int const cur_val = iter->val;
        CHECK(expect_rrange[index], cur_val);
        ++index;
    }
    CHECK(iter, rend_rrange(r));
    if (iter)
    {
        CHECK(((struct val *)iter)->val, expect_rrange[n - 1]);
    }
    END_FAIL({
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
             ++j, iter = rnext(rom, &iter->elem))
        {
            if (!iter)
            {
                return TEST_STATUS;
            }
            if (expect_rrange[j] == iter->val)
            {
                (void)fprintf(stderr, "%s%d, %s", GREEN, expect_rrange[j],
                              NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", RED, iter->val, NONE);
            }
        }
        for (; iter != rend_rrange(r); iter = rnext(rom, &iter->elem))
        {
            (void)fprintf(stderr, "%s%d, %s", RED, iter->val, NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", GREEN, NONE);
    });
}

BEGIN_STATIC_TEST(iterator_check, ordered_multimap *const omm)
{
    size_t const size = size(omm);
    size_t iter_count = 0;
    for (struct val *e = begin(omm); e; e = next(omm, &e->elem))
    {
        ++iter_count;
    }
    CHECK(iter_count, size);
    iter_count = 0;
    for (struct val *e = omm_rbegin(omm); e; e = rnext(omm, &e->elem))
    {
        ++iter_count;
    }
    CHECK(iter_count, size);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_forward_iter_unique_vals)
{
    ordered_multimap omm
        = omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct val *e = begin(&omm); e; e = next(&omm, &e->elem), ++j)
    {}
    CHECK(j, 0);
    int const num_nodes = 33;
    int const prime = 37;
    struct val vals[33];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = shuffled_index; // NOLINT
        vals[i].id = i;
        CHECK(unwrap(insert_r(&omm, &vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[33];
    CHECK(inorder_fill(val_keys_inorder, num_nodes, &omm), size(&omm));
    j = num_nodes - 1;
    for (struct val *e = begin(&omm); e && j >= 0;
         e = next(&omm, &e->elem), --j)
    {
        CHECK(e->val, val_keys_inorder[j]);
    }
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_forward_iter_all_vals)
{
    ordered_multimap omm
        = omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct val *i = begin(&omm); i; i = next(&omm, &i->elem), ++j)
    {}
    CHECK(j, 0);
    int const num_nodes = 33;
    struct val vals[33];
    vals[0].val = 0; // NOLINT
    vals[0].id = 0;
    CHECK(unwrap(insert_r(&omm, &vals[0].elem)) != NULL, true);
    /* This will test iterating through every possible length list. */
    for (int i = 1, val = 1; i < num_nodes; i += i, ++val)
    {
        for (int repeats = 0, index = i; repeats < i && index < num_nodes;
             ++repeats, ++index)
        {
            vals[index].val = val; // NOLINT
            vals[index].id = index;
            CHECK(unwrap(insert_r(&omm, &vals[index].elem)) != NULL, true);
            CHECK(validate(&omm), true);
        }
    }
    int val_keys_inorder[33];
    (void)inorder_fill(val_keys_inorder, num_nodes, &omm);
    j = num_nodes - 1;
    for (struct val *i = begin(&omm); i && j >= 0;
         i = next(&omm, &i->elem), --j)
    {
        CHECK(i->val, val_keys_inorder[j]);
    }
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_insert_iterate_pop)
{
    ordered_multimap omm
        = omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(unwrap(insert_r(&omm, &vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
    }
    CHECK(iterator_check(&omm), PASS);
    size_t pop_count = 0;
    while (!is_empty(&omm))
    {
        CHECK(omm_pop_max(&omm), CCC_OK);
        ++pop_count;
        CHECK(validate(&omm), true);
        if (pop_count % 200)
        {
            CHECK(iterator_check(&omm), PASS);
        }
    }
    CHECK(pop_count, num_nodes);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_priority_removal)
{
    ordered_multimap omm
        = omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(unwrap(insert_r(&omm, &vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
    }
    CHECK(iterator_check(&omm), PASS);
    int const limit = 400;
    for (struct val *i = begin(&omm); i;)
    {
        if (i->val > limit)
        {
            i = omm_extract(&omm, &i->elem);
            CHECK(validate(&omm), true);
        }
        else
        {
            i = next(&omm, &i->elem);
        }
    }
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_priority_update)
{
    ordered_multimap omm
        = omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(unwrap(insert_r(&omm, &vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
    }
    CHECK(iterator_check(&omm), PASS);
    int const limit = 400;
    for (struct val *i = begin(&omm); i;)
    {
        if (i->val > limit)
        {
            struct val *next = next(&omm, &i->elem);
            CHECK(update(&omm, &i->elem, val_update, &(int){i->val / 2}), true);
            CHECK(validate(&omm), true);
            i = next;
        }
        else
        {
            i = next(&omm, &i->elem);
        }
    }
    CHECK(size(&omm), num_nodes);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_priority_valid_range)
{
    ordered_multimap omm
        = omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        CHECK(unwrap(insert_r(&omm, &vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
    }
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    CHECK(check_rrange(&omm, equal_rrange_r(&omm, &(int){6}, &(int){44}), 8,
                       (int[8]){10, 15, 20, 25, 30, 35, 40, 45}),
          PASS);
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    CHECK(check_range(&omm, equal_range_r(&omm, &(int){119}, &(int){84}), 8,
                      (int[8]){115, 110, 105, 100, 95, 90, 85, 80}),
          PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_priority_valid_range_equals)
{
    ordered_multimap omm
        = omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        CHECK(unwrap(insert_r(&omm, &vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
    }
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    CHECK(check_rrange(&omm, equal_rrange_r(&omm, &(int){10}, &(int){40}), 8,
                       (int[8]){10, 15, 20, 25, 30, 35, 40, 45}),
          PASS);
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    CHECK(check_range(&omm, equal_range_r(&omm, &(int){115}, &(int){85}), 8,
                      (int[8]){115, 110, 105, 100, 95, 90, 85, 80}),
          PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_priority_invalid_range)
{
    ordered_multimap omm
        = omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        CHECK(unwrap(insert_r(&omm, &vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
    }
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    CHECK(check_rrange(&omm, equal_rrange_r(&omm, &(int){95}, &(int){999}), 6,
                       (int[6]){95, 100, 105, 110, 115, 120}),
          PASS);
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    CHECK(check_range(&omm, equal_range_r(&omm, &(int){36}, &(int){-999}), 8,
                      (int[8]){35, 30, 25, 20, 15, 10, 5, 0}),
          PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(omm_test_priority_empty_range)
{
    ordered_multimap omm
        = omm_init(omm, struct val, elem, val, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        CHECK(unwrap(insert_r(&omm, &vals[i].elem)) != NULL, true);
        CHECK(validate(&omm), true);
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    rrange const rev_range = equal_rrange(&omm, &(int){-50}, &(int){-25});
    CHECK(((struct val *)rbegin_rrange(&rev_range))->val, vals[0].val);
    CHECK(((struct val *)rend_rrange(&rev_range))->val, vals[0].val);
    range const eq_range = equal_range(&omm, &(int){150}, &(int){999});
    CHECK(((struct val *)begin_range(&eq_range))->val, vals[num_nodes - 1].val);
    CHECK(((struct val *)end_range(&eq_range))->val, vals[num_nodes - 1].val);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(
        omm_test_forward_iter_unique_vals(), omm_test_forward_iter_all_vals(),
        omm_test_insert_iterate_pop(), omm_test_priority_update(),
        omm_test_priority_removal(), omm_test_priority_valid_range(),
        omm_test_priority_valid_range_equals(),
        omm_test_priority_invalid_range(), omm_test_priority_empty_range());
}
