#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC
#define DOUBLE_ENDED_PRIORITY_QUEUE_USING_NAMESPACE_CCC

#include "depq_util.h"
#include "double_ended_priority_queue.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

BEGIN_STATIC_TEST(check_range, double_ended_priority_queue const *const rom,
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

BEGIN_STATIC_TEST(check_rrange, double_ended_priority_queue const *const rom,
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

BEGIN_STATIC_TEST(iterator_check, ccc_double_ended_priority_queue *pq)
{
    size_t const size = size(pq);
    size_t iter_count = 0;
    for (struct val *e = begin(pq); e; e = next(pq, &e->elem))
    {
        ++iter_count;
        CHECK(iter_count != size || ccc_depq_is_min(pq, &e->elem), true);
        CHECK(iter_count == size || !ccc_depq_is_min(pq, &e->elem), true);
    }
    CHECK(iter_count, size);
    iter_count = 0;
    for (struct val *e = ccc_depq_rbegin(pq); e; e = rnext(pq, &e->elem))
    {
        ++iter_count;
        CHECK(iter_count != size || ccc_depq_is_max(pq, &e->elem), true);
        CHECK(iter_count == size || !ccc_depq_is_max(pq, &e->elem), true);
    }
    CHECK(iter_count, size);
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_forward_iter_unique_vals)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct val *e = begin(&pq); e; e = next(&pq, &e->elem), ++j)
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
        push(&pq, &vals[i].elem);
        CHECK(validate(&pq), true);
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[33];
    CHECK(inorder_fill(val_keys_inorder, num_nodes, &pq), size(&pq));
    j = num_nodes - 1;
    for (struct val *e = begin(&pq); e && j >= 0; e = next(&pq, &e->elem), --j)
    {
        CHECK(e->val, val_keys_inorder[j]);
    }
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_forward_iter_all_vals)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct val *i = begin(&pq); i; i = next(&pq, &i->elem), ++j)
    {}
    CHECK(j, 0);
    int const num_nodes = 33;
    struct val vals[33];
    vals[0].val = 0; // NOLINT
    vals[0].id = 0;
    push(&pq, &vals[0].elem);
    /* This will test iterating through every possible length list. */
    for (int i = 1, val = 1; i < num_nodes; i += i, ++val)
    {
        for (int repeats = 0, index = i; repeats < i && index < num_nodes;
             ++repeats, ++index)
        {
            vals[index].val = val; // NOLINT
            vals[index].id = index;
            push(&pq, &vals[index].elem);
            CHECK(validate(&pq), true);
        }
    }
    int val_keys_inorder[33];
    (void)inorder_fill(val_keys_inorder, num_nodes, &pq);
    j = num_nodes - 1;
    for (struct val *i = begin(&pq); i && j >= 0; i = next(&pq, &i->elem), --j)
    {
        CHECK(i->val, val_keys_inorder[j]);
    }
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_insert_iterate_pop)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
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
        push(&pq, &vals[i].elem);
        CHECK(validate(&pq), true);
    }
    CHECK(iterator_check(&pq), PASS);
    size_t pop_count = 0;
    while (!empty(&pq))
    {
        ccc_depq_pop_max(&pq);
        ++pop_count;
        CHECK(validate(&pq), true);
        if (pop_count % 200)
        {
            CHECK(iterator_check(&pq), PASS);
        }
    }
    CHECK(pop_count, num_nodes);
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_priority_removal)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
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
        push(&pq, &vals[i].elem);
        CHECK(validate(&pq), true);
    }
    CHECK(iterator_check(&pq), PASS);
    int const limit = 400;
    for (struct val *i = begin(&pq); i;)
    {
        if (i->val > limit)
        {
            i = ccc_depq_erase(&pq, &i->elem);
            CHECK(validate(&pq), true);
        }
        else
        {
            i = next(&pq, &i->elem);
        }
    }
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_priority_update)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
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
        push(&pq, &vals[i].elem);
        CHECK(validate(&pq), true);
    }
    CHECK(iterator_check(&pq), PASS);
    int const limit = 400;
    for (struct val *i = begin(&pq); i;)
    {
        if (i->val > limit)
        {
            struct val *next = next(&pq, &i->elem);
            CHECK(update(&pq, &i->elem, val_update, &(int){i->val / 2}), true);
            CHECK(validate(&pq), true);
            i = next;
        }
        else
        {
            i = next(&pq, &i->elem);
        }
    }
    CHECK(size(&pq), num_nodes);
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_priority_valid_range)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        push(&pq, &vals[i].elem);
        CHECK(validate(&pq), true);
    }
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    CHECK(check_rrange(&pq, equal_rrange_vr(&pq, &(int){6}, &(int){44}), 8,
                       (int[8]){10, 15, 20, 25, 30, 35, 40, 45}),
          PASS);
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    CHECK(check_range(&pq, equal_range_vr(&pq, &(int){119}, &(int){84}), 8,
                      (int[8]){115, 110, 105, 100, 95, 90, 85, 80}),
          PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_priority_valid_range_equals)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        push(&pq, &vals[i].elem);
        CHECK(validate(&pq), true);
    }
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    CHECK(check_rrange(&pq, equal_rrange_vr(&pq, &(int){10}, &(int){40}), 8,
                       (int[8]){10, 15, 20, 25, 30, 35, 40, 45}),
          PASS);
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    CHECK(check_range(&pq, equal_range_vr(&pq, &(int){115}, &(int){85}), 8,
                      (int[8]){115, 110, 105, 100, 95, 90, 85, 80}),
          PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_priority_invalid_range)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        push(&pq, &vals[i].elem);
        CHECK(validate(&pq), true);
    }
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    CHECK(check_rrange(&pq, equal_rrange_vr(&pq, &(int){95}, &(int){999}), 6,
                       (int[6]){95, 100, 105, 110, 115, 120}),
          PASS);
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    CHECK(check_range(&pq, equal_range_vr(&pq, &(int){36}, &(int){-999}), 8,
                      (int[8]){35, 30, 25, 20, 15, 10, 5, 0}),
          PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_priority_empty_range)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[25];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        push(&pq, &vals[i].elem);
        CHECK(validate(&pq), true);
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    ccc_rrange const rev_range = equal_rrange(&pq, &(int){-50}, &(int){-25});
    CHECK(((struct val *)rbegin_rrange(&rev_range))->val, vals[0].val);
    CHECK(((struct val *)rend_rrange(&rev_range))->val, vals[0].val);
    ccc_range const eq_range = equal_range(&pq, &(int){150}, &(int){999});
    CHECK(((struct val *)begin_range(&eq_range))->val, vals[num_nodes - 1].val);
    CHECK(((struct val *)end_range(&eq_range))->val, vals[num_nodes - 1].val);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(
        depq_test_forward_iter_unique_vals(), depq_test_forward_iter_all_vals(),
        depq_test_insert_iterate_pop(), depq_test_priority_update(),
        depq_test_priority_removal(), depq_test_priority_valid_range(),
        depq_test_priority_valid_range_equals(),
        depq_test_priority_invalid_range(), depq_test_priority_empty_range());
}
