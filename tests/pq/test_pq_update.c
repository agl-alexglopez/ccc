#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "pq_util.h"
#include "priority_queue.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

CHECK_BEGIN_STATIC_FN(pq_test_insert_iterate_pop)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    ptrdiff_t const num_nodes = 1000;
    struct val vals[1000];
    for (ptrdiff_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    ptrdiff_t pop_count = 0;
    while (!ccc_pq_is_empty(&pq))
    {
        CHECK(pop(&pq), CCC_RESULT_OK);
        ++pop_count;
        CHECK(validate(&pq), true);
    }
    CHECK(pop_count, num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_priority_removal)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    ptrdiff_t const num_nodes = 1000;
    struct val vals[1000];
    for (ptrdiff_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (ptrdiff_t val = 0; val < num_nodes; ++val)
    {
        struct val *i = &vals[val];
        if (i->val > limit)
        {
            (void)ccc_pq_extract(&pq, &i->elem);
            CHECK(validate(&pq), true);
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_priority_update)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    ptrdiff_t const num_nodes = 1000;
    struct val vals[1000];
    for (ptrdiff_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (ptrdiff_t val = 0; val < num_nodes; ++val)
    {
        struct val *i = &vals[val];
        int backoff = i->val / 2;
        if (i->val > limit)
        {
            CHECK(ccc_pq_update(&pq, &i->elem, val_update, &backoff), true);
            CHECK(validate(&pq), true);
        }
    }
    CHECK(ccc_pq_size(&pq), num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_priority_update_with)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    ptrdiff_t const num_nodes = 1000;
    struct val vals[1000];
    for (ptrdiff_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (ptrdiff_t val = 0; val < num_nodes; ++val)
    {
        struct val *i = &vals[val];
        int backoff = i->val / 2;
        if (i->val > limit)
        {
            CHECK(ccc_pq_update_w(&pq, &i->elem, { i->val = backoff; }), true);
            CHECK(validate(&pq), true);
        }
    }
    CHECK(ccc_pq_size(&pq), num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_priority_increase)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    ptrdiff_t const num_nodes = 1000;
    struct val vals[1000];
    for (ptrdiff_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (ptrdiff_t val = 0; val < num_nodes; ++val)
    {
        struct val *const i = &vals[val];
        int inc = limit * 2;
        int dec = i->val / 2;
        if (i->val >= limit)
        {
            CHECK(ccc_pq_decrease(&pq, &i->elem, val_update, &dec), true);
            CHECK(validate(&pq), true);
        }
        else
        {
            CHECK(ccc_pq_increase(&pq, &i->elem, val_update, &inc), true);
            CHECK(validate(&pq), true);
        }
    }
    CHECK(ccc_pq_size(&pq), num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_priority_increase_with)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    ptrdiff_t const num_nodes = 1000;
    struct val vals[1000];
    for (ptrdiff_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (ptrdiff_t val = 0; val < num_nodes; ++val)
    {
        struct val *const i = &vals[val];
        int inc = limit * 2;
        int dec = i->val / 2;
        if (i->val >= limit)
        {
            CHECK(ccc_pq_decrease_w(&pq, &i->elem, { i->val = dec; }), true);
            CHECK(validate(&pq), true);
        }
        else
        {
            CHECK(ccc_pq_increase_w(&pq, &i->elem, { i->val = inc; }), true);
            CHECK(validate(&pq), true);
        }
    }
    CHECK(ccc_pq_size(&pq), num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_priority_decrease)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_GRT, val_cmp, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    ptrdiff_t const num_nodes = 1000;
    struct val vals[1000];
    for (ptrdiff_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (ptrdiff_t val = 0; val < num_nodes; ++val)
    {
        struct val *const i = &vals[val];
        int inc = limit * 2;
        int dec = i->val / 2;
        if (i->val < limit)
        {
            CHECK(ccc_pq_increase(&pq, &i->elem, val_update, &inc), true);
            CHECK(validate(&pq), true);
        }
        else
        {
            CHECK(ccc_pq_decrease(&pq, &i->elem, val_update, &dec), true);
            CHECK(validate(&pq), true);
        }
    }
    CHECK(ccc_pq_size(&pq), num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_priority_decrease_with)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_GRT, val_cmp, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    ptrdiff_t const num_nodes = 1000;
    struct val vals[1000];
    for (ptrdiff_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (ptrdiff_t val = 0; val < num_nodes; ++val)
    {
        struct val *const i = &vals[val];
        int inc = limit * 2;
        int dec = i->val / 2;
        if (i->val < limit)
        {
            CHECK(ccc_pq_increase_w(&pq, &i->elem, { i->val = inc; }), true);
            CHECK(validate(&pq), true);
        }
        else
        {
            CHECK(ccc_pq_decrease_w(&pq, &i->elem, { i->val = dec; }), true);
            CHECK(validate(&pq), true);
        }
    }
    CHECK(ccc_pq_size(&pq), num_nodes);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(
        pq_test_insert_iterate_pop(), pq_test_priority_update(),
        pq_test_priority_update_with(), pq_test_priority_removal(),
        pq_test_priority_increase(), pq_test_priority_increase_with(),
        pq_test_priority_decrease(), pq_test_priority_decrease_with());
}
