#include "set.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int id;
    int val;
    ccc_set_elem elem;
};

static enum test_result set_test_forward_iter(void);
static enum test_result set_test_iterate_removal(void);
static enum test_result set_test_iterate_remove_reinsert(void);
static enum test_result set_test_valid_range(void);
static enum test_result set_test_invalid_range(void);
static enum test_result set_test_empty_range(void);
static size_t inorder_fill(int[], size_t, ccc_set *);
static enum test_result iterator_check(ccc_set *);
static ccc_threeway_cmp val_cmp(void const *, void const *, void *);

#define NUM_TESTS ((size_t)6)
test_fn const all_tests[NUM_TESTS] = {
    set_test_forward_iter, set_test_iterate_removal,
    set_test_valid_range,  set_test_invalid_range,
    set_test_empty_range,  set_test_iterate_remove_reinsert,
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
set_test_forward_iter(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, s, val_cmp, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct val *e = ccc_set_begin(&s); e;
         e = ccc_set_next(&s, &e->elem), ++j)
    {}
    CHECK(j, 0, int, "%d");
    int const num_nodes = 33;
    int const prime = 37;
    struct val vals[num_nodes];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = (int)shuffled_index;
        vals[i].id = i;
        ccc_set_insert(&s, &vals[i].elem);
        CHECK(ccc_set_validate(&s), true, bool, "%d");
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[num_nodes];
    CHECK(inorder_fill(val_keys_inorder, num_nodes, &s), ccc_set_size(&s),
          size_t, "%zu");
    j = 0;
    for (struct val *e = ccc_set_begin(&s); e && j < num_nodes;
         e = ccc_set_next(&s, &e->elem), ++j)
    {
        CHECK(e->val, val_keys_inorder[j], int, "%d");
    }
    return PASS;
}

static enum test_result
set_test_iterate_removal(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, s, val_cmp, NULL);
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
        ccc_set_insert(&s, &vals[i].elem);
        CHECK(ccc_set_validate(&s), true, bool, "%d");
    }
    CHECK(iterator_check(&s), PASS, enum test_result, "%d");
    int const limit = 400;
    for (struct val *i = ccc_set_begin(&s), *next = NULL; i; i = next)
    {
        next = ccc_set_next(&s, &i->elem);
        if (i->val > limit)
        {
            (void)ccc_set_erase(&s, &i->elem);
            CHECK(ccc_set_validate(&s), true, bool, "%d");
        }
    }
    return PASS;
}

static enum test_result
set_test_iterate_remove_reinsert(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, s, val_cmp, NULL);
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
        ccc_set_insert(&s, &vals[i].elem);
        CHECK(ccc_set_validate(&s), true, bool, "%d");
    }
    CHECK(iterator_check(&s), PASS, enum test_result, "%d");
    size_t const old_size = ccc_set_size(&s);
    int const limit = 400;
    int new_unique_entry_val = 1001;
    for (struct val *i = ccc_set_begin(&s), *next = NULL; i; i = next)
    {
        next = ccc_set_next(&s, &i->elem);
        if (i->val < limit)
        {
            (void)ccc_set_erase(&s, &i->elem);
            i->val = new_unique_entry_val;
            CHECK(ccc_set_insert(&s, &i->elem), true, bool, "%d");
            CHECK(ccc_set_validate(&s), true, bool, "%d");
            ++new_unique_entry_val;
        }
    }
    CHECK(ccc_set_size(&s), old_size, size_t, "%zu");
    return PASS;
}

static enum test_result
set_test_valid_range(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, s, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        ccc_set_insert(&s, &vals[i].elem);
        CHECK(ccc_set_validate(&s), true, bool, "%d");
    }
    struct val b = {.id = 0, .val = 6};
    struct val e = {.id = 0, .val = 44};
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    int const range_vals[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    ccc_range const range = ccc_set_equal_range(&s, &b.elem, &e.elem);
    CHECK(((struct val *)ccc_set_begin_range(&range))->val, range_vals[0], int,
          "%d");
    CHECK(((struct val *)ccc_set_end_range(&range))->val, range_vals[7], int,
          "%d");
    size_t index = 0;
    struct val *i1 = ccc_set_begin_range(&range);
    for (; i1 != ccc_set_end_range(&range); i1 = ccc_set_next(&s, &i1->elem))
    {
        int const cur_val = i1->val;
        CHECK(range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1, ccc_set_end_range(&range), struct val *, "%p");
    CHECK(((struct val *)i1)->val, range_vals[7], int, "%d");
    b.val = 119;
    e.val = 84;
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    int const rev_range_vals[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    ccc_rrange const rev_range = ccc_set_equal_rrange(&s, &b.elem, &e.elem);
    CHECK(((struct val *)ccc_set_begin_rrange(&rev_range))->val,
          rev_range_vals[0], int, "%d");
    CHECK(((struct val *)ccc_set_end_rrange(&rev_range))->val,
          rev_range_vals[7], int, "%d");
    index = 0;
    struct val *i2 = ccc_set_begin_rrange(&rev_range);
    for (; i2 != ccc_set_end_rrange(&rev_range);
         i2 = ccc_set_rnext(&s, &i2->elem))
    {
        int const cur_val = i2->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2, ccc_set_end_rrange(&rev_range), struct val *, "%p");
    CHECK(i2->val, rev_range_vals[7], int, "%d");
    return PASS;
}

static enum test_result
set_test_invalid_range(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, s, val_cmp, NULL);
    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        ccc_set_insert(&s, &vals[i].elem);
        CHECK(ccc_set_validate(&s), true, bool, "%d");
    }
    struct val b = {.id = 0, .val = 95};
    struct val e = {.id = 0, .val = 999};
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    int const forward_range_vals[6] = {95, 100, 105, 110, 115, 120};
    ccc_range const rev_range = ccc_set_equal_range(&s, &b.elem, &e.elem);
    CHECK(((struct val *)ccc_set_begin_range(&rev_range))->val
              == forward_range_vals[0],
          true, bool, "%d");
    CHECK(ccc_set_end_range(&rev_range), NULL, ccc_set_elem *, "%p");
    size_t index = 0;
    struct val *i1 = ccc_set_begin_range(&rev_range);
    for (; i1 != ccc_set_end_range(&rev_range);
         i1 = ccc_set_next(&s, &i1->elem))
    {
        int const cur_val = i1->val;
        CHECK(forward_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1, ccc_set_end_range(&rev_range), struct val *, "%p");
    CHECK(i1, NULL, struct val *, "%p");
    b.val = 36;
    e.val = -999;
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    int const rev_range_vals[8] = {35, 30, 25, 20, 15, 10, 5, 0};
    ccc_rrange const range = ccc_set_equal_rrange(&s, &b.elem, &e.elem);
    CHECK(((struct val *)ccc_set_begin_rrange(&range))->val, rev_range_vals[0],
          int, "%d");
    CHECK(ccc_set_end_rrange(&range), NULL, ccc_set_elem *, "%p");
    index = 0;
    struct val *i2 = ccc_set_begin_rrange(&range);
    for (; i2 != ccc_set_end_rrange(&range); i2 = ccc_set_rnext(&s, &i2->elem))
    {
        int const cur_val = i2->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2, ccc_set_end_rrange(&range), struct val *, "%p");
    CHECK(i2, NULL, struct val *, "%p");
    return PASS;
}

static enum test_result
set_test_empty_range(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, s, val_cmp, NULL);
    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        ccc_set_insert(&s, &vals[i].elem);
        CHECK(ccc_set_validate(&s), true, bool, "%d");
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    struct val b = {.id = 0, .val = -50};
    struct val e = {.id = 0, .val = -25};
    ccc_range const forward_range = ccc_set_equal_range(&s, &b.elem, &e.elem);
    CHECK(((struct val *)ccc_set_begin_range(&forward_range))->val, vals[0].val,
          int, "%d");
    CHECK(((struct val *)ccc_set_end_range(&forward_range))->val, vals[0].val,
          int, "%d");
    b.val = 150;
    e.val = 999;
    ccc_rrange const rev_range = ccc_set_equal_rrange(&s, &b.elem, &e.elem);
    CHECK(((struct val *)ccc_set_begin_rrange(&rev_range))->val,
          vals[num_nodes - 1].val, int, "%d");
    CHECK(((struct val *)ccc_set_end_rrange(&rev_range))->val,
          vals[num_nodes - 1].val, int, "%d");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, ccc_set *s)
{
    if (ccc_set_size(s) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct val *e = ccc_set_begin(s); e; e = ccc_set_next(s, &e->elem))
    {
        vals[i++] = e->val;
    }
    return i;
}

static enum test_result
iterator_check(ccc_set *s)
{
    size_t const size = ccc_set_size(s);
    size_t iter_count = 0;
    for (struct val *e = ccc_set_begin(s); e; e = ccc_set_next(s, &e->elem))
    {
        ++iter_count;
        CHECK(iter_count != size || ccc_set_is_max(s, &e->elem), true, bool,
              "%d");
        CHECK(iter_count == size || !ccc_set_is_max(s, &e->elem), true, bool,
              "%d");
    }
    CHECK(iter_count, size, size_t, "%zu");
    iter_count = 0;
    for (struct val *e = ccc_set_rbegin(s); e; e = ccc_set_rnext(s, &e->elem))
    {
        ++iter_count;
        CHECK(iter_count != size || ccc_set_is_min(s, &e->elem), true, bool,
              "%d");
        CHECK(iter_count == size || !ccc_set_is_min(s, &e->elem), true, bool,
              "%d");
    }
    CHECK(iter_count, size, size_t, "%zu");
    return PASS;
}

static ccc_threeway_cmp
val_cmp(void const *const a, void const *const b, void *aux)
{
    (void)aux;
    struct val const *const lhs = a;
    struct val const *const rhs = b;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
