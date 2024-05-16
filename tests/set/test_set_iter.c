#include "set.h"
#include "test.h"
#include "tree.h"

#include <stdlib.h>
#include <time.h>

struct val
{
    int id;
    int val;
    set_elem elem;
};

static enum test_result set_test_forward_iter(void);
static enum test_result set_test_iterate_removal(void);
static enum test_result set_test_iterate_remove_reinsert(void);
static enum test_result set_test_valid_range(void);
static enum test_result set_test_invalid_range(void);
static enum test_result set_test_empty_range(void);
static size_t inorder_fill(int[], size_t, set *);
static enum test_result iterator_check(set *);
static threeway_cmp val_cmp(const set_elem *, const set_elem *, void *);

#define NUM_TESTS ((size_t)6)
const test_fn all_tests[NUM_TESTS] = {
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
        const bool fail = all_tests[i]() == FAIL;
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
    set s;
    set_init(&s);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (set_elem *e = set_begin(&s); e != set_end(&s);
         e = set_next(&s, e), ++j)
    {}
    CHECK(j, 0, int, "%d");
    const int num_nodes = 33;
    const int prime = 37;
    struct val vals[num_nodes];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = (int)shuffled_index;
        vals[i].id = i;
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[num_nodes];
    CHECK(inorder_fill(val_keys_inorder, num_nodes, &s), set_size(&s), size_t,
          "%zu");
    j = 0;
    for (set_elem *e = set_begin(&s); e != set_end(&s) && j < num_nodes;
         e = set_next(&s, e), ++j)
    {
        const struct val *v = set_entry(e, struct val, elem);
        CHECK(v->val, val_keys_inorder[j], int, "%d");
    }
    return PASS;
}

static enum test_result
set_test_iterate_removal(void)
{
    set s;
    set_init(&s);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const size_t num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    CHECK(iterator_check(&s), PASS, enum test_result, "%d");
    const int limit = 400;
    for (set_elem *i = set_begin(&s), *next = NULL; i != set_end(&s); i = next)
    {
        next = set_next(&s, i);
        struct val *cur = set_entry(i, struct val, elem);
        if (cur->val > limit)
        {
            (void)set_erase(&s, i, val_cmp, NULL);
            CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
        }
    }
    return PASS;
}

static enum test_result
set_test_iterate_remove_reinsert(void)
{
    set s;
    set_init(&s);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const size_t num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    CHECK(iterator_check(&s), PASS, enum test_result, "%d");
    const size_t old_size = set_size(&s);
    const int limit = 400;
    int new_unique_entry_val = 1001;
    for (set_elem *i = set_begin(&s), *next = NULL; i != set_end(&s); i = next)
    {
        next = set_next(&s, i);
        struct val *cur = set_entry(i, struct val, elem);
        if (cur->val < limit)
        {
            (void)set_erase(&s, i, val_cmp, NULL);
            struct val *v = set_entry(i, struct val, elem);
            v->val = new_unique_entry_val;
            CHECK(set_insert(&s, i, val_cmp, NULL), true, bool, "%b");
            CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
            ++new_unique_entry_val;
        }
    }
    CHECK(set_size(&s), old_size, size_t, "%zu");
    return PASS;
}

static enum test_result
set_test_valid_range(void)
{
    set s;
    set_init(&s);

    const int num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    struct val b = {.id = 0, .val = 6};
    struct val e = {.id = 0, .val = 44};
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    const int range_vals[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    const set_range range
        = set_equal_range(&s, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(set_entry(range.begin, struct val, elem)->val, range_vals[0], int,
          "%d");
    CHECK(set_entry(range.end, struct val, elem)->val, range_vals[7], int,
          "%d");
    size_t index = 0;
    set_elem *i1 = range.begin;
    for (; i1 != range.end; i1 = set_next(&s, i1))
    {
        const int cur_val = set_entry(i1, struct val, elem)->val;
        CHECK(range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1, range.end, set_elem *, "%p");
    CHECK(set_entry(i1, struct val, elem)->val, range_vals[7], int, "%d");
    b.val = 119;
    e.val = 84;
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    const int rev_range_vals[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    const set_rrange rev_range
        = set_equal_rrange(&s, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(set_entry(rev_range.rbegin, struct val, elem)->val, rev_range_vals[0],
          int, "%d");
    CHECK(set_entry(rev_range.end, struct val, elem)->val, rev_range_vals[7],
          int, "%d");
    index = 0;
    set_elem *i2 = rev_range.rbegin;
    for (; i2 != rev_range.end; i2 = set_rnext(&s, i2))
    {
        const int cur_val = set_entry(i2, struct val, elem)->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2, rev_range.end, set_elem *, "%p");
    CHECK(set_entry(i2, struct val, elem)->val, rev_range_vals[7], int, "%d");
    return PASS;
}

static enum test_result
set_test_invalid_range(void)
{
    set s;
    set_init(&s);

    const int num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    struct val b = {.id = 0, .val = 95};
    struct val e = {.id = 0, .val = 999};
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    const int forward_range_vals[6] = {95, 100, 105, 110, 115, 120};
    const set_range rev_range
        = set_equal_range(&s, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(set_entry(rev_range.begin, struct val, elem)->val
              == forward_range_vals[0],
          true, bool, "%b");
    CHECK(rev_range.end, set_end(&s), set_elem *, "%p");
    size_t index = 0;
    set_elem *i1 = rev_range.begin;
    for (; i1 != rev_range.end; i1 = set_next(&s, i1))
    {
        const int cur_val = set_entry(i1, struct val, elem)->val;
        CHECK(forward_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1, rev_range.end, set_elem *, "%p");
    CHECK(i1, set_end(&s), set_elem *, "%p");
    b.val = 36;
    e.val = -999;
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    const int rev_range_vals[8] = {35, 30, 25, 20, 15, 10, 5, 0};
    const set_rrange range
        = set_equal_rrange(&s, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(set_entry(range.rbegin, struct val, elem)->val, rev_range_vals[0],
          int, "%d");
    CHECK(range.end, set_end(&s), set_elem *, "%p");
    index = 0;
    set_elem *i2 = range.rbegin;
    for (; i2 != range.end; i2 = set_rnext(&s, i2))
    {
        const int cur_val = set_entry(i2, struct val, elem)->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2, range.end, set_elem *, "%p");
    CHECK(i2, set_end(&s), set_elem *, "%p");
    return PASS;
}

static enum test_result
set_test_empty_range(void)
{
    set s;
    set_init(&s);

    const int num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    struct val b = {.id = 0, .val = -50};
    struct val e = {.id = 0, .val = -25};
    const set_range forward_range
        = set_equal_range(&s, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(set_entry(forward_range.begin, struct val, elem)->val, vals[0].val,
          int, "%d");
    CHECK(set_entry(forward_range.end, struct val, elem)->val, vals[0].val, int,
          "%d");
    b.val = 150;
    e.val = 999;
    const set_rrange rev_range
        = set_equal_rrange(&s, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(set_entry(rev_range.rbegin, struct val, elem)->val,
          vals[num_nodes - 1].val, int, "%d");
    CHECK(set_entry(rev_range.end, struct val, elem)->val,
          vals[num_nodes - 1].val, int, "%d");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, set *s)
{
    if (set_size(s) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (set_elem *e = set_begin(s); e != set_end(s); e = set_next(s, e))
    {
        vals[i++] = set_entry(e, struct val, elem)->val;
    }
    return i;
}

static enum test_result
iterator_check(set *s)
{
    const size_t size = set_size(s);
    size_t iter_count = 0;
    for (set_elem *e = set_begin(s); e != set_end(s); e = set_next(s, e))
    {
        ++iter_count;
        CHECK(iter_count != size || set_is_max(s, e), true, bool, "%b");
        CHECK(iter_count == size || !set_is_max(s, e), true, bool, "%b");
    }
    CHECK(iter_count, size, size_t, "%zu");
    iter_count = 0;
    for (set_elem *e = set_rbegin(s); e != set_end(s); e = set_rnext(s, e))
    {
        ++iter_count;
        CHECK(iter_count != size || set_is_min(s, e), true, bool, "%b");
        CHECK(iter_count == size || !set_is_min(s, e), true, bool, "%b");
    }
    CHECK(iter_count, size, size_t, "%zu");
    return PASS;
}

static threeway_cmp
val_cmp(const set_elem *a, const set_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = set_entry(a, struct val, elem);
    struct val *rhs = set_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
