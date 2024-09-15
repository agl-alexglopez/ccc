#define TRAITS_USING_NAMESPACE_CCC

#include "realtime_ordered_map.h"
#include "rtomap_util.h"
#include "test.h"
#include "traits.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

static enum test_result rtom_test_forward_iter(void);
static enum test_result rtom_test_iterate_removal(void);
static enum test_result rtom_test_iterate_remove_reinsert(void);
static enum test_result rtom_test_valid_range(void);
static enum test_result rtom_test_valid_range_equals(void);
static enum test_result rtom_test_invalid_range(void);
static enum test_result rtom_test_empty_range(void);
static enum test_result iterator_check(ccc_realtime_ordered_map *);

#define NUM_TESTS ((size_t)7)
test_fn const all_tests[NUM_TESTS] = {
    rtom_test_forward_iter,
    rtom_test_iterate_removal,
    rtom_test_valid_range,
    rtom_test_valid_range_equals,
    rtom_test_invalid_range,
    rtom_test_empty_range,
    rtom_test_iterate_remove_reinsert,
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        bool const fail = all_tests[i]() == FAIL;
        if (fail)
        {
            res = FAIL;
        }
    }
    return res;
}

static enum test_result
rtom_test_forward_iter(void)
{
    ccc_realtime_ordered_map s
        = CCC_ROM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct val *e = begin(&s); e != end(&s); e = next(&s, &e->elem), ++j)
    {}
    CHECK(j, 0);
    int const num_nodes = 33;
    int const prime = 37;
    struct val vals[num_nodes];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = (int)shuffled_index;
        vals[i].id = i;
        (void)insert(&s, &vals[i].elem);
        CHECK(ccc_rom_validate(&s), true);
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[num_nodes];
    CHECK(inorder_fill(val_keys_inorder, num_nodes, &s), size(&s));
    j = 0;
    for (struct val *e = begin(&s); e && j < num_nodes;
         e = next(&s, &e->elem), ++j)
    {
        CHECK(e->val, val_keys_inorder[j]);
    }
    return PASS;
}

static enum test_result
rtom_test_iterate_removal(void)
{
    ccc_realtime_ordered_map s
        = CCC_ROM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        (void)insert(&s, &vals[i].elem);
        CHECK(ccc_rom_validate(&s), true);
    }
    CHECK(iterator_check(&s), PASS);
    int const limit = 400;
    for (struct val *i = begin(&s), *next = NULL; i; i = next)
    {
        next = next(&s, &i->elem);
        if (i->val > limit)
        {
            (void)remove(&s, &i->elem);
            CHECK(ccc_rom_validate(&s), true);
        }
    }
    return PASS;
}

static enum test_result
rtom_test_iterate_remove_reinsert(void)
{
    ccc_realtime_ordered_map s
        = CCC_ROM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        (void)insert(&s, &vals[i].elem);
        CHECK(ccc_rom_validate(&s), true);
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
            CHECK(ccc_rom_validate(&s), true);
            ++new_unique_entry_val;
        }
    }
    CHECK(size(&s), old_size);
    return PASS;
}

static enum test_result
rtom_test_valid_range(void)
{
    ccc_realtime_ordered_map s
        = CCC_ROM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        (void)insert(&s, &vals[i].elem);
        CHECK(ccc_rom_validate(&s), true);
    }
    int b = 6;
    int e = 44;
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    int const range_vals[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    ccc_range const range = equal_range(&s, &b, &e);
    CHECK(((struct val *)begin_range(&range))->val, range_vals[0]);
    CHECK(((struct val *)end_range(&range))->val, range_vals[7]);
    size_t index = 0;
    struct val *i1 = begin_range(&range);
    for (; i1 != end_range(&range); i1 = next(&s, &i1->elem))
    {
        int const cur_val = i1->val;
        CHECK(range_vals[index], cur_val);
        ++index;
    }
    CHECK(i1, end_range(&range));
    CHECK(((struct val *)i1)->val, range_vals[7]);
    b = 119;
    e = 84;
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    int const rev_range_vals[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    ccc_rrange const rev_range = equal_rrange(&s, &b, &e);
    CHECK(((struct val *)rbegin_rrange(&rev_range))->val, rev_range_vals[0]);
    CHECK(((struct val *)rend_rrange(&rev_range))->val, rev_range_vals[7]);
    index = 0;
    struct val *i2 = rbegin_rrange(&rev_range);
    for (; i2 != rend_rrange(&rev_range); i2 = rnext(&s, &i2->elem))
    {
        int const cur_val = i2->val;
        CHECK(rev_range_vals[index], cur_val);
        ++index;
    }
    CHECK(i2, rend_rrange(&rev_range));
    CHECK(i2->val, rev_range_vals[7]);
    return PASS;
}

static enum test_result
rtom_test_valid_range_equals(void)
{
    ccc_realtime_ordered_map s
        = CCC_ROM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        (void)insert(&s, &vals[i].elem);
        CHECK(ccc_rom_validate(&s), true);
    }
    int b = 10;
    int e = 40;
    /* This should be the following range [5,50). 5 should stay at the start,
       and 45 is equal to our end key so is bumped to next greater, 50. */
    int const range_vals[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    ccc_range const range = equal_range(&s, &b, &e);
    CHECK(((struct val *)begin_range(&range))->val, range_vals[0]);
    CHECK(((struct val *)end_range(&range))->val, range_vals[7]);
    size_t index = 0;
    struct val *i1 = begin_range(&range);
    for (; i1 != end_range(&range); i1 = next(&s, &i1->elem))
    {
        int const cur_val = i1->val;
        CHECK(range_vals[index], cur_val);
        ++index;
    }
    CHECK(i1, end_range(&range));
    CHECK(((struct val *)i1)->val, range_vals[7]);
    b = 115;
    e = 85;
    /* This should be the following range [115,84). 115 should be
       is a valid start to the range and 85 is eqal to end key so must
       be dropped to first value less than 85, 80. */
    int const rev_range_vals[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    ccc_rrange const rev_range = equal_rrange(&s, &b, &e);
    CHECK(((struct val *)rbegin_rrange(&rev_range))->val, rev_range_vals[0]);
    CHECK(((struct val *)rend_rrange(&rev_range))->val, rev_range_vals[7]);
    index = 0;
    struct val *i2 = rbegin_rrange(&rev_range);
    for (; i2 != rend_rrange(&rev_range); i2 = rnext(&s, &i2->elem))
    {
        int const cur_val = i2->val;
        CHECK(rev_range_vals[index], cur_val);
        ++index;
    }
    CHECK(i2, rend_rrange(&rev_range));
    CHECK(i2->val, rev_range_vals[7]);
    return PASS;
}

static enum test_result
rtom_test_invalid_range(void)
{
    ccc_realtime_ordered_map s
        = CCC_ROM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        (void)insert(&s, &vals[i].elem);
        CHECK(ccc_rom_validate(&s), true);
    }
    int b = 95;
    int e = 999;
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    int const forward_range_vals[6] = {95, 100, 105, 110, 115, 120};
    ccc_range const rev_range = equal_range(&s, &b, &e);
    CHECK(((struct val *)begin_range(&rev_range))->val, forward_range_vals[0]);
    CHECK(end_range(&rev_range), NULL);
    size_t index = 0;
    struct val *i1 = begin_range(&rev_range);
    for (; i1 != end_range(&rev_range); i1 = next(&s, &i1->elem))
    {
        int const cur_val = i1->val;
        CHECK(forward_range_vals[index], cur_val);
        ++index;
    }
    CHECK(i1, end_range(&rev_range));
    CHECK(i1, NULL);
    b = 36;
    e = -999;
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    int const rev_range_vals[8] = {35, 30, 25, 20, 15, 10, 5, 0};
    ccc_rrange const range = equal_rrange(&s, &b, &e);
    CHECK(((struct val *)rbegin_rrange(&range))->val, rev_range_vals[0]);
    CHECK(rend_rrange(&range), NULL);
    index = 0;
    struct val *i2 = rbegin_rrange(&range);
    for (; i2 != rend_rrange(&range); i2 = rnext(&s, &i2->elem))
    {
        int const cur_val = i2->val;
        CHECK(rev_range_vals[index], cur_val);
        ++index;
    }
    CHECK(i2, rend_rrange(&range));
    CHECK(i2, NULL);
    return PASS;
}

static enum test_result
rtom_test_empty_range(void)
{
    ccc_realtime_ordered_map s
        = CCC_ROM_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        insert(&s, &vals[i].elem);
        CHECK(ccc_rom_validate(&s), true);
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    struct val b = {.id = 0, .val = -50};
    struct val e = {.id = 0, .val = -25};
    ccc_range const forward_range = equal_range(&s, &b.val, &e.val);
    CHECK(((struct val *)begin_range(&forward_range))->val, vals[0].val);
    CHECK(((struct val *)end_range(&forward_range))->val, vals[0].val);
    b.val = 150;
    e.val = 999;
    ccc_rrange const rev_range = equal_rrange(&s, &b.val, &e.val);
    CHECK(((struct val *)rbegin_rrange(&rev_range))->val,
          vals[num_nodes - 1].val);
    CHECK(((struct val *)rend_rrange(&rev_range))->val,
          vals[num_nodes - 1].val);
    return PASS;
}

static enum test_result
iterator_check(ccc_realtime_ordered_map *s)
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
    return PASS;
}
