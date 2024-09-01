#include "map.h"
#include "map_util.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

static enum test_result map_test_forward_iter(void);
static enum test_result map_test_iterate_removal(void);
static enum test_result map_test_iterate_remove_reinsert(void);
static enum test_result map_test_valid_range(void);
static enum test_result map_test_invalid_range(void);
static enum test_result map_test_empty_range(void);
static enum test_result iterator_check(ccc_map *);

#define NUM_TESTS ((size_t)6)
test_fn const all_tests[NUM_TESTS] = {
    map_test_forward_iter, map_test_iterate_removal,
    map_test_valid_range,  map_test_invalid_range,
    map_test_empty_range,  map_test_iterate_remove_reinsert,
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
map_test_forward_iter(void)
{
    ccc_map s = CCC_M_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct val *e = ccc_m_begin(&s); e; e = ccc_m_next(&s, &e->elem), ++j)
    {}
    CHECK(j, 0, "%d");
    int const num_nodes = 33;
    int const prime = 37;
    struct val vals[num_nodes];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = (int)shuffled_index;
        vals[i].id = i;
        (void)ccc_m_insert(&s, &vals[i].elem);
        CHECK(ccc_m_validate(&s), true, "%d");
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[num_nodes];
    CHECK(inorder_fill(val_keys_inorder, num_nodes, &s), ccc_m_size(&s), "%zu");
    j = 0;
    for (struct val *e = ccc_m_begin(&s); e && j < num_nodes;
         e = ccc_m_next(&s, &e->elem), ++j)
    {
        CHECK(e->val, val_keys_inorder[j], "%d");
    }
    return PASS;
}

static enum test_result
map_test_iterate_removal(void)
{
    ccc_map s = CCC_M_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
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
        (void)ccc_m_insert(&s, &vals[i].elem);
        CHECK(ccc_m_validate(&s), true, "%d");
    }
    CHECK(iterator_check(&s), PASS, "%d");
    int const limit = 400;
    for (struct val *i = ccc_m_begin(&s), *next = NULL; i; i = next)
    {
        next = ccc_m_next(&s, &i->elem);
        if (i->val > limit)
        {
            (void)ccc_m_remove(&s, &i->elem);
            CHECK(ccc_m_validate(&s), true, "%d");
        }
    }
    return PASS;
}

static enum test_result
map_test_iterate_remove_reinsert(void)
{
    ccc_map s = CCC_M_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
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
        (void)ccc_m_insert(&s, &vals[i].elem);
        CHECK(ccc_m_validate(&s), true, "%d");
    }
    CHECK(iterator_check(&s), PASS, "%d");
    size_t const old_size = ccc_m_size(&s);
    int const limit = 400;
    int new_unique_entry_val = 1001;
    for (struct val *i = ccc_m_begin(&s), *next = NULL; i; i = next)
    {
        next = ccc_m_next(&s, &i->elem);
        if (i->val < limit)
        {
            (void)ccc_m_remove(&s, &i->elem);
            i->val = new_unique_entry_val;
            CHECK(ccc_m_insert_entry(ccc_m_entry(&s, &i->val), &i->elem)
                      != NULL,
                  true, "%d");
            CHECK(ccc_m_validate(&s), true, "%d");
            ++new_unique_entry_val;
        }
    }
    CHECK(ccc_m_size(&s), old_size, "%zu");
    return PASS;
}

static enum test_result
map_test_valid_range(void)
{
    ccc_map s = CCC_M_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        (void)ccc_m_insert(&s, &vals[i].elem);
        CHECK(ccc_m_validate(&s), true, "%d");
    }
    struct val b = {.id = 0, .val = 6};
    struct val e = {.id = 0, .val = 44};
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    int const range_vals[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    ccc_range const range = ccc_m_equal_range(&s, &b.val, &e.val);
    CHECK(((struct val *)ccc_m_begin_range(&range))->val, range_vals[0], "%d");
    CHECK(((struct val *)ccc_m_end_range(&range))->val, range_vals[7], "%d");
    size_t index = 0;
    struct val *i1 = ccc_m_begin_range(&range);
    for (; i1 != ccc_m_end_range(&range); i1 = ccc_m_next(&s, &i1->elem))
    {
        int const cur_val = i1->val;
        CHECK(range_vals[index], cur_val, "%d");
        ++index;
    }
    CHECK(i1, ccc_m_end_range(&range), "%p");
    CHECK(((struct val *)i1)->val, range_vals[7], "%d");
    b.val = 119;
    e.val = 84;
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    int const rev_range_vals[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    ccc_rrange const rev_range = ccc_m_equal_rrange(&s, &b.val, &e.val);
    CHECK(((struct val *)ccc_m_begin_rrange(&rev_range))->val,
          rev_range_vals[0], "%d");
    CHECK(((struct val *)ccc_m_end_rrange(&rev_range))->val, rev_range_vals[7],
          "%d");
    index = 0;
    struct val *i2 = ccc_m_begin_rrange(&rev_range);
    for (; i2 != ccc_m_end_rrange(&rev_range); i2 = ccc_m_rnext(&s, &i2->elem))
    {
        int const cur_val = i2->val;
        CHECK(rev_range_vals[index], cur_val, "%d");
        ++index;
    }
    CHECK(i2, ccc_m_end_rrange(&rev_range), "%p");
    CHECK(i2->val, rev_range_vals[7], "%d");
    return PASS;
}

static enum test_result
map_test_invalid_range(void)
{
    ccc_map s = CCC_M_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        (void)ccc_m_insert(&s, &vals[i].elem);
        CHECK(ccc_m_validate(&s), true, "%d");
    }
    struct val b = {.id = 0, .val = 95};
    struct val e = {.id = 0, .val = 999};
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    int const forward_range_vals[6] = {95, 100, 105, 110, 115, 120};
    ccc_range const rev_range = ccc_m_equal_range(&s, &b.val, &e.val);
    CHECK(((struct val *)ccc_m_begin_range(&rev_range))->val
              == forward_range_vals[0],
          true, "%d");
    CHECK(ccc_m_end_range(&rev_range), NULL, "%p");
    size_t index = 0;
    struct val *i1 = ccc_m_begin_range(&rev_range);
    for (; i1 != ccc_m_end_range(&rev_range); i1 = ccc_m_next(&s, &i1->elem))
    {
        int const cur_val = i1->val;
        CHECK(forward_range_vals[index], cur_val, "%d");
        ++index;
    }
    CHECK(i1, ccc_m_end_range(&rev_range), "%p");
    CHECK(i1, NULL, "%p");
    b.val = 36;
    e.val = -999;
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    int const rev_range_vals[8] = {35, 30, 25, 20, 15, 10, 5, 0};
    ccc_rrange const range = ccc_m_equal_rrange(&s, &b.val, &e.val);
    CHECK(((struct val *)ccc_m_begin_rrange(&range))->val, rev_range_vals[0],
          "%d");
    CHECK(ccc_m_end_rrange(&range), NULL, "%p");
    index = 0;
    struct val *i2 = ccc_m_begin_rrange(&range);
    for (; i2 != ccc_m_end_rrange(&range); i2 = ccc_m_rnext(&s, &i2->elem))
    {
        int const cur_val = i2->val;
        CHECK(rev_range_vals[index], cur_val, "%d");
        ++index;
    }
    CHECK(i2, ccc_m_end_rrange(&range), "%p");
    CHECK(i2, NULL, "%p");
    return PASS;
}

static enum test_result
map_test_empty_range(void)
{
    ccc_map s = CCC_M_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        ccc_m_insert(&s, &vals[i].elem);
        CHECK(ccc_m_validate(&s), true, "%d");
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    struct val b = {.id = 0, .val = -50};
    struct val e = {.id = 0, .val = -25};
    ccc_range const forward_range = ccc_m_equal_range(&s, &b.val, &e.val);
    CHECK(((struct val *)ccc_m_begin_range(&forward_range))->val, vals[0].val,
          "%d");
    CHECK(((struct val *)ccc_m_end_range(&forward_range))->val, vals[0].val,
          "%d");
    b.val = 150;
    e.val = 999;
    ccc_rrange const rev_range = ccc_m_equal_rrange(&s, &b.val, &e.val);
    CHECK(((struct val *)ccc_m_begin_rrange(&rev_range))->val,
          vals[num_nodes - 1].val, "%d");
    CHECK(((struct val *)ccc_m_end_rrange(&rev_range))->val,
          vals[num_nodes - 1].val, "%d");
    return PASS;
}

static enum test_result
iterator_check(ccc_map *s)
{
    size_t const size = ccc_m_size(s);
    size_t iter_count = 0;
    for (struct val *e = ccc_m_begin(s); e; e = ccc_m_next(s, &e->elem))
    {
        ++iter_count;
        CHECK(iter_count <= size, true, "%d");
    }
    CHECK(iter_count, size, "%zu");
    iter_count = 0;
    for (struct val *e = ccc_m_rbegin(s); e; e = ccc_m_rnext(s, &e->elem))
    {
        ++iter_count;
        CHECK(iter_count <= size, true, "%d");
    }
    CHECK(iter_count, size, "%zu");
    return PASS;
}
