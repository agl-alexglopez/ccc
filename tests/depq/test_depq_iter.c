#include "depqueue.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int id;
    int val;
    ccc_depq_elem elem;
};

static enum test_result depq_test_forward_iter_unique_vals(void);
static enum test_result depq_test_forward_iter_all_vals(void);
static enum test_result depq_test_insert_iterate_pop(void);
static enum test_result depq_test_priority_update(void);
static enum test_result depq_test_priority_removal(void);
static enum test_result depq_test_priority_valid_range(void);
static enum test_result depq_test_priority_invalid_range(void);
static enum test_result depq_test_priority_empty_range(void);
static size_t inorder_fill(int[], size_t, ccc_depqueue *);
static enum test_result iterator_check(ccc_depqueue *);
static void val_update(void *, void *);
static ccc_threeway_cmp val_cmp(void const *, void const *, void *);

#define NUM_TESTS (size_t)8
test_fn const all_tests[NUM_TESTS] = {
    depq_test_forward_iter_unique_vals, depq_test_forward_iter_all_vals,
    depq_test_insert_iterate_pop,       depq_test_priority_update,
    depq_test_priority_removal,         depq_test_priority_valid_range,
    depq_test_priority_invalid_range,   depq_test_priority_empty_range,
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
depq_test_forward_iter_unique_vals(void)
{
    ccc_depqueue pq = CCC_DEPQ_INIT(struct val, elem, pq, val_cmp, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct val *e = ccc_depq_begin(&pq); e;
         e = ccc_depq_next(&pq, &e->elem), ++j)
    {}
    CHECK(j, 0, int, "%d");
    int const num_nodes = 33;
    int const prime = 37;
    struct val vals[num_nodes];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = shuffled_index; // NOLINT
        vals[i].id = i;
        ccc_depq_push(&pq, &vals[i].elem);
        CHECK(ccc_depq_validate(&pq), true, bool, "%d");
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[num_nodes];
    CHECK(inorder_fill(val_keys_inorder, num_nodes, &pq), ccc_depq_size(&pq),
          size_t, "%zu");
    j = num_nodes - 1;
    for (struct val *e = ccc_depq_begin(&pq); e && j >= 0;
         e = ccc_depq_next(&pq, &e->elem), --j)
    {
        CHECK(e->val, val_keys_inorder[j], int, "%d");
    }
    return PASS;
}

static enum test_result
depq_test_forward_iter_all_vals(void)
{
    ccc_depqueue pq = CCC_DEPQ_INIT(struct val, elem, pq, val_cmp, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct val *i = ccc_depq_begin(&pq); i;
         i = ccc_depq_next(&pq, &i->elem), ++j)
    {}
    CHECK(j, 0, int, "%d");
    int const num_nodes = 33;
    struct val vals[num_nodes];
    vals[0].val = 0; // NOLINT
    vals[0].id = 0;
    ccc_depq_push(&pq, &vals[0].elem);
    /* This will test iterating through every possible length list. */
    for (int i = 1, val = 1; i < num_nodes; i += i, ++val)
    {
        for (int repeats = 0, index = i; repeats < i && index < num_nodes;
             ++repeats, ++index)
        {
            vals[index].val = val; // NOLINT
            vals[index].id = index;
            ccc_depq_push(&pq, &vals[index].elem);
            CHECK(ccc_depq_validate(&pq), true, bool, "%d");
        }
    }
    int val_keys_inorder[num_nodes];
    (void)inorder_fill(val_keys_inorder, num_nodes, &pq);
    j = num_nodes - 1;
    for (struct val *i = ccc_depq_begin(&pq); i && j >= 0;
         i = ccc_depq_next(&pq, &i->elem), --j)
    {
        CHECK(i->val, val_keys_inorder[j], int, "%d");
    }
    return PASS;
}

static enum test_result
depq_test_insert_iterate_pop(void)
{
    ccc_depqueue pq = CCC_DEPQ_INIT(struct val, elem, pq, val_cmp, NULL);
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
        ccc_depq_push(&pq, &vals[i].elem);
        CHECK(ccc_depq_validate(&pq), true, bool, "%d");
    }
    CHECK(iterator_check(&pq), PASS, enum test_result, "%d");
    size_t pop_count = 0;
    while (!ccc_depq_empty(&pq))
    {
        ccc_depq_pop_max(&pq);
        ++pop_count;
        CHECK(ccc_depq_validate(&pq), true, bool, "%d");
        if (pop_count % 200)
        {
            CHECK(iterator_check(&pq), PASS, enum test_result, "%d");
        }
    }
    CHECK(pop_count, num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
depq_test_priority_removal(void)
{
    ccc_depqueue pq = CCC_DEPQ_INIT(struct val, elem, pq, val_cmp, NULL);
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
        ccc_depq_push(&pq, &vals[i].elem);
        CHECK(ccc_depq_validate(&pq), true, bool, "%d");
    }
    CHECK(iterator_check(&pq), PASS, enum test_result, "%d");
    int const limit = 400;
    for (struct val *i = ccc_depq_begin(&pq); i;)
    {
        if (i->val > limit)
        {
            i = ccc_depq_erase(&pq, &i->elem);
            CHECK(ccc_depq_validate(&pq), true, bool, "%d");
        }
        else
        {
            i = ccc_depq_next(&pq, &i->elem);
        }
    }
    return PASS;
}

static enum test_result
depq_test_priority_update(void)
{
    ccc_depqueue pq = CCC_DEPQ_INIT(struct val, elem, pq, val_cmp, NULL);
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
        ccc_depq_push(&pq, &vals[i].elem);
        CHECK(ccc_depq_validate(&pq), true, bool, "%d");
    }
    CHECK(iterator_check(&pq), PASS, enum test_result, "%d");
    int const limit = 400;
    for (struct val *i = ccc_depq_begin(&pq); i;)
    {
        int backoff = i->val / 2;
        if (i->val > limit)
        {
            struct val *next = ccc_depq_next(&pq, &i->elem);
            CHECK(ccc_depq_update(&pq, &i->elem, val_update, &backoff), true,
                  bool, "%d");
            CHECK(ccc_depq_validate(&pq), true, bool, "%d");
            i = next;
        }
        else
        {
            i = ccc_depq_next(&pq, &i->elem);
        }
    }
    CHECK(ccc_depq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
depq_test_priority_valid_range(void)
{
    ccc_depqueue pq = CCC_DEPQ_INIT(struct val, elem, pq, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        ccc_depq_push(&pq, &vals[i].elem);
        CHECK(ccc_depq_validate(&pq), true, bool, "%d");
    }
    struct val b = {.id = 0, .val = 6};
    struct val e = {.id = 0, .val = 44};
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    int const rev_range_vals[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    ccc_rrange const rev_range = ccc_depq_equal_rrange(&pq, &b.elem, &e.elem);
    CHECK(((struct val *)ccc_depq_begin_rrange(&rev_range))->val,
          rev_range_vals[0], int, "%d");
    CHECK(((struct val *)ccc_depq_end_rrange(&rev_range))->val,
          rev_range_vals[7], int, "%d");
    size_t index = 0;
    struct val *i1 = ccc_depq_begin_rrange(&rev_range);
    for (; i1 != ccc_depq_end_rrange(&rev_range);
         i1 = ccc_depq_rnext(&pq, &i1->elem))
    {
        int const cur_val = i1->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1 == ccc_depq_end_rrange(&rev_range), true, bool, "%d");
    CHECK(i1->val, rev_range_vals[7], int, "%d");
    b.val = 119;
    e.val = 84;
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    int const range_vals[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    ccc_range const range = ccc_depq_equal_range(&pq, &b.elem, &e.elem);
    CHECK(((struct val *)ccc_depq_begin_range(&range))->val, range_vals[0], int,
          "%d");
    CHECK(((struct val *)ccc_depq_end_range(&range))->val, range_vals[7], int,
          "%d");
    index = 0;
    struct val *i2 = ccc_depq_begin_range(&range);
    for (; i2 != ccc_depq_end_range(&range); i2 = ccc_depq_next(&pq, &i2->elem))
    {
        int const cur_val = i2->val;
        CHECK(range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2 == ccc_depq_end_range(&range), true, bool, "%d");
    CHECK(i2->val, range_vals[7], int, "%d");
    return PASS;
}

static enum test_result
depq_test_priority_invalid_range(void)
{
    ccc_depqueue pq = CCC_DEPQ_INIT(struct val, elem, pq, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        ccc_depq_push(&pq, &vals[i].elem);
        CHECK(ccc_depq_validate(&pq), true, bool, "%d");
    }
    struct val b = {.id = 0, .val = 95};
    struct val e = {.id = 0, .val = 999};
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    int const rev_range_vals[6] = {95, 100, 105, 110, 115, 120};
    ccc_rrange const rev_range = ccc_depq_equal_rrange(&pq, &b.elem, &e.elem);
    CHECK(((struct val *)ccc_depq_begin_rrange(&rev_range))->val,
          rev_range_vals[0], int, "%d");
    CHECK(ccc_depq_end_rrange(&rev_range), NULL, ccc_depq_elem *, "%p");
    size_t index = 0;
    struct val *i1 = ccc_depq_begin_rrange(&rev_range);
    for (; i1 != ccc_depq_end_rrange(&rev_range);
         i1 = ccc_depq_rnext(&pq, &i1->elem))
    {
        int const cur_val = i1->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1 == ccc_depq_end_rrange(&rev_range) && !i1, true, bool, "%d");
    b.val = 36;
    e.val = -999;
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    int const range_vals[8] = {35, 30, 25, 20, 15, 10, 5, 0};
    ccc_range const range = ccc_depq_equal_range(&pq, &b.elem, &e.elem);
    CHECK(((struct val *)ccc_depq_begin_range(&range))->val, range_vals[0], int,
          "%d");
    CHECK(ccc_depq_end_range(&range), NULL, ccc_depq_elem *, "%p");
    index = 0;
    struct val *i2 = ccc_depq_begin_range(&range);
    for (; i2 != ccc_depq_end_range(&range); i2 = ccc_depq_next(&pq, &i2->elem))
    {
        int const cur_val = i2->val;
        CHECK(range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2 == ccc_depq_end_range(&range) && !i2, true, bool, "%d");
    return PASS;
}

static enum test_result
depq_test_priority_empty_range(void)
{
    ccc_depqueue pq = CCC_DEPQ_INIT(struct val, elem, pq, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        ccc_depq_push(&pq, &vals[i].elem);
        CHECK(ccc_depq_validate(&pq), true, bool, "%d");
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    struct val b = {.id = 0, .val = -50};
    struct val e = {.id = 0, .val = -25};
    ccc_rrange const rev_range = ccc_depq_equal_rrange(&pq, &b.elem, &e.elem);
    CHECK(((struct val *)ccc_depq_begin_rrange(&rev_range))->val, vals[0].val,
          int, "%d");
    CHECK(((struct val *)ccc_depq_end_rrange(&rev_range))->val, vals[0].val,
          int, "%d");
    b.val = 150;
    e.val = 999;
    ccc_range const range = ccc_depq_equal_range(&pq, &b.elem, &e.elem);
    CHECK(((struct val *)ccc_depq_begin_range(&range))->val,
          vals[num_nodes - 1].val, int, "%d");
    CHECK(((struct val *)ccc_depq_end_range(&range))->val,
          vals[num_nodes - 1].val, int, "%d");
    return PASS;
}

static size_t
inorder_fill(int vals[], size_t size, ccc_depqueue *pq)
{
    if (ccc_depq_size(pq) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct val *e = ccc_depq_rbegin(pq); e;
         e = ccc_depq_rnext(pq, &e->elem))
    {
        vals[i++] = e->val;
    }
    return i;
}

static enum test_result
iterator_check(ccc_depqueue *pq)
{
    size_t const size = ccc_depq_size(pq);
    size_t iter_count = 0;
    for (struct val *e = ccc_depq_begin(pq); e; e = ccc_depq_next(pq, &e->elem))
    {
        ++iter_count;
        CHECK(iter_count != size || ccc_depq_is_min(pq, &e->elem), true, bool,
              "%d");
        CHECK(iter_count == size || !ccc_depq_is_min(pq, &e->elem), true, bool,
              "%d");
    }
    CHECK(iter_count, size, size_t, "%zu");
    iter_count = 0;
    for (struct val *e = ccc_depq_rbegin(pq); e;
         e = ccc_depq_rnext(pq, &e->elem))
    {
        ++iter_count;
        CHECK(iter_count != size || ccc_depq_is_max(pq, &e->elem), true, bool,
              "%d");
        CHECK(iter_count == size || !ccc_depq_is_max(pq, &e->elem), true, bool,
              "%d");
    }
    CHECK(iter_count, size, size_t, "%zu");
    return PASS;
}

static ccc_threeway_cmp
val_cmp(void const *a, void const *b, void *aux)
{
    (void)aux;
    struct val const *const lhs = a;
    struct val const *const rhs = b;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static void
val_update(void *const a, void *aux)
{
    struct val *old = a;
    old->val = *(int *)aux;
}
