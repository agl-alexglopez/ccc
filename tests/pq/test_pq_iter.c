#include "pqueue.h"
#include "test.h"
#include "tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int id;
    int val;
    struct pq_elem elem;
};

static enum test_result pq_test_forward_iter_unique_vals(void);
static enum test_result pq_test_forward_iter_all_vals(void);
static enum test_result pq_test_insert_iterate_pop(void);
static enum test_result pq_test_priority_update(void);
static enum test_result pq_test_priority_removal(void);
static enum test_result pq_test_priority_valid_range(void);
static enum test_result pq_test_priority_invalid_range(void);
static enum test_result pq_test_priority_empty_range(void);
static size_t inorder_fill(int[], size_t, struct pqueue *);
static enum test_result iterator_check(struct pqueue *);
static void val_update(struct pq_elem *, void *);
static node_threeway_cmp val_cmp(const struct pq_elem *, const struct pq_elem *,
                                 void *);

#define NUM_TESTS (size_t)8
const test_fn all_tests[NUM_TESTS] = {
    pq_test_forward_iter_unique_vals, pq_test_forward_iter_all_vals,
    pq_test_insert_iterate_pop,       pq_test_priority_update,
    pq_test_priority_removal,         pq_test_priority_valid_range,
    pq_test_priority_invalid_range,   pq_test_priority_empty_range,
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
pq_test_forward_iter_unique_vals(void)
{
    struct pqueue pq;
    pq_init(&pq);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct pq_elem *e = pq_begin(&pq); e != pq_end(&pq);
         e = pq_next(&pq, e), ++j)
    {}
    CHECK(j, 0, int, "%d");
    const int num_nodes = 33;
    const int prime = 37;
    struct val vals[num_nodes];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = shuffled_index; // NOLINT
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq.t, (tree_cmp_fn *)val_cmp), true, bool, "%b");
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[num_nodes];
    CHECK(inorder_fill(val_keys_inorder, num_nodes, &pq), pq_size(&pq), size_t,
          "%zu");
    j = num_nodes - 1;
    for (struct pq_elem *e = pq_begin(&pq); e != pq_end(&pq) && j >= 0;
         e = pq_next(&pq, e), --j)
    {
        const struct val *v = pq_entry(e, struct val, elem);
        CHECK(v->val, val_keys_inorder[j], int, "%d");
    }
    return PASS;
}

static enum test_result
pq_test_forward_iter_all_vals(void)
{
    struct pqueue pq;
    pq_init(&pq);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct pq_elem *i = pq_begin(&pq); i != pq_end(&pq);
         i = pq_next(&pq, i), ++j)
    {}
    CHECK(j, 0, int, "%d");
    const int num_nodes = 33;
    struct val vals[num_nodes];
    vals[0].val = 0; // NOLINT
    vals[0].id = 0;
    pq_insert(&pq, &vals[0].elem, val_cmp, NULL);
    /* This will test iterating through every possible length list. */
    for (int i = 1, val = 1; i < num_nodes; i += i, ++val)
    {
        for (int repeats = 0, index = i; repeats < i && index < num_nodes;
             ++repeats, ++index)
        {
            vals[index].val = val; // NOLINT
            vals[index].id = index;
            pq_insert(&pq, &vals[index].elem, val_cmp, NULL);
            CHECK(validate_tree(&pq.t, (tree_cmp_fn *)val_cmp), true, bool,
                  "%b");
        }
    }
    int val_keys_inorder[num_nodes];
    (void)inorder_fill(val_keys_inorder, num_nodes, &pq);
    j = num_nodes - 1;
    for (struct pq_elem *i = pq_begin(&pq); i != pq_end(&pq) && j >= 0;
         i = pq_next(&pq, i), --j)
    {
        const struct val *v = pq_entry(i, struct val, elem);
        CHECK(v->val, val_keys_inorder[j], int, "%d");
    }
    return PASS;
}

static enum test_result
pq_test_insert_iterate_pop(void)
{
    struct pqueue pq;
    pq_init(&pq);
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
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq.t, (tree_cmp_fn *)val_cmp), true, bool, "%b");
    }
    CHECK(iterator_check(&pq), PASS, enum test_result, "%d");
    size_t pop_count = 0;
    while (!pq_empty(&pq))
    {
        pq_pop_max(&pq);
        ++pop_count;
        CHECK(validate_tree(&pq.t, (tree_cmp_fn *)val_cmp), true, bool, "%b");
        if (pop_count % 200)
        {
            CHECK(iterator_check(&pq), PASS, enum test_result, "%d");
        }
    }
    CHECK(pop_count, num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_removal(void)
{
    struct pqueue pq;
    pq_init(&pq);
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
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq.t, (tree_cmp_fn *)val_cmp), true, bool, "%b");
    }
    CHECK(iterator_check(&pq), PASS, enum test_result, "%d");
    const int limit = 400;
    for (struct pq_elem *i = pq_begin(&pq); i != pq_end(&pq);)
    {
        struct val *cur = pq_entry(i, struct val, elem);
        if (cur->val > limit)
        {
            i = pq_erase(&pq, i, val_cmp, NULL);
            CHECK(validate_tree(&pq.t, (tree_cmp_fn *)val_cmp), true, bool,
                  "%b");
        }
        else
        {
            i = pq_next(&pq, i);
        }
    }
    return PASS;
}

static enum test_result
pq_test_priority_update(void)
{
    struct pqueue pq;
    pq_init(&pq);
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
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq.t, (tree_cmp_fn *)val_cmp), true, bool, "%b");
    }
    CHECK(iterator_check(&pq), PASS, enum test_result, "%d");
    const int limit = 400;
    for (struct pq_elem *i = pq_begin(&pq); i != pq_end(&pq);)
    {
        struct val *cur = pq_entry(i, struct val, elem);
        int backoff = cur->val / 2;
        if (cur->val > limit)
        {
            struct pq_elem *next = pq_next(&pq, i);
            CHECK(pq_update(&pq, i, val_cmp, val_update, &backoff), true, bool,
                  "%b");
            CHECK(validate_tree(&pq.t, (tree_cmp_fn *)val_cmp), true, bool,
                  "%b");
            i = next;
        }
        else
        {
            i = pq_next(&pq, i);
        }
    }
    CHECK(pq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_valid_range(void)
{
    struct pqueue pq;
    pq_init(&pq);

    const int num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq.t, (tree_cmp_fn *)val_cmp), true, bool, "%b");
    }
    struct val b = {.id = 0, .val = 6};
    struct val e = {.id = 0, .val = 44};
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    const int rev_range_vals[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    const struct pq_rrange rev_range
        = pq_equal_rrange(&pq, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(pq_entry(rev_range.rbegin, struct val, elem)->val == rev_range_vals[0]
              && pq_entry(rev_range.end, struct val, elem)->val
                     == rev_range_vals[7],
          true, bool, "%b");
    size_t index = 0;
    struct pq_elem *i1 = rev_range.rbegin;
    for (; i1 != rev_range.end; i1 = pq_rnext(&pq, i1))
    {
        const int cur_val = pq_entry(i1, struct val, elem)->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1 == rev_range.end
              && pq_entry(i1, struct val, elem)->val == rev_range_vals[7],
          true, bool, "%b");
    b.val = 119;
    e.val = 84;
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    const int range_vals[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    const struct pq_range range
        = pq_equal_range(&pq, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(pq_entry(range.begin, struct val, elem)->val == range_vals[0]
              && pq_entry(range.end, struct val, elem)->val == range_vals[7],
          true, bool, "%b");
    index = 0;
    struct pq_elem *i2 = range.begin;
    for (; i2 != range.end; i2 = pq_next(&pq, i2))
    {
        const int cur_val = pq_entry(i2, struct val, elem)->val;
        CHECK(range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2 == range.end
              && pq_entry(i2, struct val, elem)->val == range_vals[7],
          true, bool, "%b");
    return PASS;
}

static enum test_result
pq_test_priority_invalid_range(void)
{
    struct pqueue pq;
    pq_init(&pq);

    const int num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq.t, (tree_cmp_fn *)val_cmp), true, bool, "%b");
    }
    struct val b = {.id = 0, .val = 95};
    struct val e = {.id = 0, .val = 999};
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    const int rev_range_vals[6] = {95, 100, 105, 110, 115, 120};
    const struct pq_rrange rev_range
        = pq_equal_rrange(&pq, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(pq_entry(rev_range.rbegin, struct val, elem)->val == rev_range_vals[0]
              && rev_range.end == pq_end(&pq),
          true, bool, "%b");
    size_t index = 0;
    struct pq_elem *i1 = rev_range.rbegin;
    for (; i1 != rev_range.end; i1 = pq_rnext(&pq, i1))
    {
        const int cur_val = pq_entry(i1, struct val, elem)->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1 == rev_range.end && i1 == pq_end(&pq), true, bool, "%b");
    b.val = 36;
    e.val = -999;
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    const int range_vals[8] = {35, 30, 25, 20, 15, 10, 5, 0};
    const struct pq_range range
        = pq_equal_range(&pq, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(pq_entry(range.begin, struct val, elem)->val == range_vals[0]
              && range.end == pq_end(&pq),
          true, bool, "%b");
    index = 0;
    struct pq_elem *i2 = range.begin;
    for (; i2 != range.end; i2 = pq_next(&pq, i2))
    {
        const int cur_val = pq_entry(i2, struct val, elem)->val;
        CHECK(range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2 == range.end && i2 == pq_end(&pq), true, bool, "%b");
    return PASS;
}

static enum test_result
pq_test_priority_empty_range(void)
{
    struct pqueue pq;
    pq_init(&pq);

    const int num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq.t, (tree_cmp_fn *)val_cmp), true, bool, "%b");
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    struct val b = {.id = 0, .val = -50};
    struct val e = {.id = 0, .val = -25};
    const struct pq_rrange rev_range
        = pq_equal_rrange(&pq, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(pq_entry(rev_range.rbegin, struct val, elem)->val == vals[0].val
              && pq_entry(rev_range.end, struct val, elem)->val == vals[0].val,
          true, bool, "%b");
    b.val = 150;
    e.val = 999;
    const struct pq_range range
        = pq_equal_range(&pq, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(pq_entry(range.begin, struct val, elem)->val
                  == vals[num_nodes - 1].val
              && pq_entry(range.end, struct val, elem)->val
                     == vals[num_nodes - 1].val,
          true, bool, "%b");
    return PASS;
}

static size_t
inorder_fill(int vals[], size_t size, struct pqueue *pq)
{
    if (pq_size(pq) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct pq_elem *e = pq_rbegin(pq); e != pq_end(pq);
         e = pq_rnext(pq, e))
    {
        vals[i++] = pq_entry(e, struct val, elem)->val;
    }
    return i;
}

static enum test_result
iterator_check(struct pqueue *pq)
{
    const size_t size = pq_size(pq);
    size_t iter_count = 0;
    for (struct pq_elem *e = pq_begin(pq); e != pq_end(pq); e = pq_next(pq, e))
    {
        ++iter_count;
        CHECK(iter_count != size || pq_is_min(pq, e), true, bool, "%b");
        CHECK(iter_count == size || !pq_is_min(pq, e), true, bool, "%b");
    }
    CHECK(iter_count, size, size_t, "%zu");
    iter_count = 0;
    for (struct pq_elem *e = pq_rbegin(pq); e != pq_end(pq);
         e = pq_rnext(pq, e))
    {
        ++iter_count;
        CHECK(iter_count != size || pq_is_max(pq, e), true, bool, "%b");
        CHECK(iter_count == size || !pq_is_max(pq, e), true, bool, "%b");
    }
    CHECK(iter_count, size, size_t, "%zu");
    return PASS;
}

static node_threeway_cmp
val_cmp(const struct pq_elem *a, const struct pq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = pq_entry(a, struct val, elem);
    struct val *rhs = pq_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static void
val_update(struct pq_elem *a, void *aux)
{
    struct val *old = pq_entry(a, struct val, elem);
    old->val = *(int *)aux;
}
