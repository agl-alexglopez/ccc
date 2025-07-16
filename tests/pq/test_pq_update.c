#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "pq_util.h"
#include "priority_queue.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(pq_test_insert_iterate_pop)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, NULL, NULL);
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
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    size_t pop_count = 0;
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
    size_t const num_nodes = 1000;
    struct val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
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
    size_t const num_nodes = 1000;
    struct val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *i = &vals[val];
        int backoff = i->val / 2;
        if (i->val > limit)
        {
            CHECK(ccc_pq_update(&pq, &i->elem, val_update, &backoff) != NULL,
                  true);
            CHECK(validate(&pq), true);
        }
    }
    CHECK(ccc_pq_count(&pq).count, num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_priority_update_with)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, NULL, NULL);
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
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        int backoff = vals[val].val / 2;
        if (vals[val].val > limit)
        {
            CHECK(ccc_pq_update_w(&pq, &vals[val], { T->val = backoff; })
                      != NULL,
                  true);
            CHECK(validate(&pq), true);
        }
    }
    CHECK(ccc_pq_count(&pq).count, num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_priority_increase)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, NULL, NULL);
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
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *const i = &vals[val];
        int inc = (limit * 2) + 1;
        int dec = (i->val / 2) - 1;
        if (i->val > limit && dec < i->val)
        {
            CHECK(ccc_pq_decrease(&pq, &i->elem, val_update, &dec) != NULL,
                  true);
            CHECK(validate(&pq), true);
        }
        else if (i->val < limit && inc > i->val)
        {
            CHECK(ccc_pq_increase(&pq, &i->elem, val_update, &inc) != NULL,
                  true);
            CHECK(validate(&pq), true);
        }
    }
    CHECK(ccc_pq_count(&pq).count, num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_priority_increase_with)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, NULL, NULL);
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
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        int inc = (limit * 2) + 1;
        int dec = (vals[val].val / 2) - 1;
        if (vals[val].val > limit && dec < vals[val].val)
        {
            CHECK(ccc_pq_decrease_w(&pq, &vals[val], { T->val = dec; }) != NULL,
                  true);
            CHECK(validate(&pq), true);
        }
        else if (vals[val].val < limit && inc > vals[val].val)
        {
            CHECK(ccc_pq_increase_w(&pq, &vals[val], { T->val = inc; }) != NULL,
                  true);
            CHECK(validate(&pq), true);
        }
    }
    CHECK(ccc_pq_count(&pq).count, num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_priority_decrease)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_GRT, val_cmp, NULL, NULL);
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
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *const i = &vals[val];
        int inc = (limit * 2) + 1;
        int dec = (i->val / 2) - 1;
        if (i->val < limit && inc > i->val)
        {
            CHECK(ccc_pq_increase(&pq, &i->elem, val_update, &inc) != NULL,
                  true);
            CHECK(validate(&pq), true);
        }
        else if (i->val > limit && dec < i->val)
        {
            CHECK(ccc_pq_decrease(&pq, &i->elem, val_update, &dec) != NULL,
                  true);
            CHECK(validate(&pq), true);
        }
    }
    CHECK(ccc_pq_count(&pq).count, num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_priority_decrease_with)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_GRT, val_cmp, NULL, NULL);
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
        CHECK(push(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        int inc = (limit * 2) + 1;
        int dec = (vals[val].val / 2) - 1;
        if (vals[val].val < limit && inc > vals[val].val)
        {
            CHECK(ccc_pq_increase_w(&pq, &vals[val], { T->val = inc; }) != NULL,
                  true);
            CHECK(validate(&pq), true);
        }
        else if (vals[val].val > limit && dec < vals[val].val)
        {
            CHECK(ccc_pq_decrease_w(&pq, &vals[val], { T->val = dec; }) != NULL,
                  true);
            CHECK(validate(&pq), true);
        }
    }
    CHECK(ccc_pq_count(&pq).count, num_nodes);
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
