#define TRAITS_USING_NAMESPACE_CCC
#define REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC

#include "realtime_ordered_map.h"
#include "rtomap_util.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

BEGIN_STATIC_TEST(check_range, realtime_ordered_map const *const rom,
                  range const *const r, size_t const n,
                  int const expect_range[])
{
    if (begin_range(r))
    {
        CHECK(((struct val *)begin_range(r))->val, expect_range[0]);
    }
    if (ccc_end_range(r))
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
    END_TEST();
}

BEGIN_STATIC_TEST(check_rrange, realtime_ordered_map const *const rom,
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
    END_TEST();
}

BEGIN_STATIC_TEST(iterator_check, realtime_ordered_map *s)
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
    END_TEST();
}

BEGIN_STATIC_TEST(rtom_test_forward_iter)
{
    realtime_ordered_map s
        = rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct val *e = begin(&s); e != end(&s); e = next(&s, &e->elem), ++j)
    {}
    CHECK(j, 0);
    int const num_nodes = 33;
    int const prime = 37;
    struct val vals[33];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = (int)shuffled_index;
        vals[i].id = i;
        (void)insert(&s, &vals[i].elem, &(struct val){});
        CHECK(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[33];
    CHECK(inorder_fill(val_keys_inorder, num_nodes, &s), size(&s));
    j = 0;
    for (struct val *e = begin(&s); e && j < num_nodes;
         e = next(&s, &e->elem), ++j)
    {
        CHECK(e->val, val_keys_inorder[j]);
    }
    END_TEST();
}

BEGIN_STATIC_TEST(rtom_test_iterate_removal)
{
    realtime_ordered_map s
        = rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
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
        (void)insert(&s, &vals[i].elem, &(struct val){});
        CHECK(validate(&s), true);
    }
    CHECK(iterator_check(&s), PASS);
    int const limit = 400;
    for (struct val *i = begin(&s), *next = NULL; i; i = next)
    {
        next = next(&s, &i->elem);
        if (i->val > limit)
        {
            (void)remove(&s, &i->elem);
            CHECK(validate(&s), true);
        }
    }
    END_TEST();
}

BEGIN_STATIC_TEST(rtom_test_iterate_remove_reinsert)
{
    realtime_ordered_map s
        = rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
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
        (void)insert(&s, &vals[i].elem, &(struct val){});
        CHECK(validate(&s), true);
    }
    CHECK(iterator_check(&s), PASS);
    size_t const old_size = size(&s);
    int const limit = 400;
    int new_unique_entry_val = 1001;
    for (struct val *i = begin(&s), *next = NULL; i; i = next)
    {
        next = next(&s, &i->elem);
        if (i->val < limit)
        {
            (void)remove(&s, &i->elem);
            i->val = new_unique_entry_val;
            CHECK(insert_entry(entry_vr(&s, &i->val), &i->elem) != NULL, true);
            CHECK(validate(&s), true);
            ++new_unique_entry_val;
        }
    }
    CHECK(size(&s), old_size);
    END_TEST();
}

BEGIN_STATIC_TEST(rtom_test_valid_range)
{
    realtime_ordered_map s
        = rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        (void)insert(&s, &vals[i].elem, &(struct val){});
        CHECK(validate(&s), true);
    }
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    CHECK(check_range(&s, equal_range_vr(&s, &(int){6}, &(int){44}), 8,
                      (int[8]){10, 15, 20, 25, 30, 35, 40, 45}),
          PASS);
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    CHECK(check_rrange(&s, equal_rrange_vr(&s, &(int){119}, &(int){84}), 8,
                       (int[8]){115, 110, 105, 100, 95, 90, 85, 80}),
          PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(rtom_test_valid_range_equals)
{
    realtime_ordered_map s
        = rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        (void)insert(&s, &vals[i].elem, &(struct val){});
        CHECK(validate(&s), true);
    }
    /* This should be the following range [5,50). 5 should stay at the start,
       and 45 is equal to our end key so is bumped to next greater, 50. */
    CHECK(check_range(&s, equal_range_vr(&s, &(int){10}, &(int){40}), 8,
                      (int[8]){10, 15, 20, 25, 30, 35, 40, 45}),
          PASS);
    /* This should be the following range [115,84). 115 should be
       is a valid start to the range and 85 is eqal to end key so must
       be dropped to first value less than 85, 80. */
    CHECK(check_rrange(&s, equal_rrange_vr(&s, &(int){115}, &(int){85}), 8,
                       (int[8]){115, 110, 105, 100, 95, 90, 85, 80}),
          PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(rtom_test_invalid_range)
{
    realtime_ordered_map s
        = rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    int const num_nodes = 25;
    struct val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        (void)insert(&s, &vals[i].elem, &(struct val){});
        CHECK(validate(&s), true);
    }
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    CHECK(check_range(&s, equal_range_vr(&s, &(int){95}, &(int){999}), 6,
                      (int[6]){95, 100, 105, 110, 115, 120}),
          PASS);
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    CHECK(check_rrange(&s, equal_rrange_vr(&s, &(int){36}, &(int){-999}), 8,
                       (int[8]){35, 30, 25, 20, 15, 10, 5, 0}),
          PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(rtom_test_empty_range)
{
    realtime_ordered_map s
        = rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    int const num_nodes = 25;
    struct val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        insert(&s, &vals[i].elem, &(struct val){});
        CHECK(validate(&s), true);
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    ccc_range const forward_range = equal_range(&s, &(int){-50}, &(int){-25});
    CHECK(((struct val *)begin_range(&forward_range))->val, vals[0].val);
    CHECK(((struct val *)end_range(&forward_range))->val, vals[0].val);
    ccc_rrange const rev_range = equal_rrange(&s, &(int){150}, &(int){999});
    CHECK(((struct val *)rbegin_rrange(&rev_range))->val,
          vals[num_nodes - 1].val);
    CHECK(((struct val *)rend_rrange(&rev_range))->val,
          vals[num_nodes - 1].val);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(rtom_test_forward_iter(), rtom_test_iterate_removal(),
                     rtom_test_valid_range(), rtom_test_valid_range_equals(),
                     rtom_test_invalid_range(), rtom_test_empty_range(),
                     rtom_test_iterate_remove_reinsert());
}
