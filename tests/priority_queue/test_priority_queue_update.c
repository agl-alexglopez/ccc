#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "priority_queue.h"
#include "priority_queue_utility.h"
#include "traits.h"
#include "types.h"

check_static_begin(priority_queue_test_insert_iterate_pop)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        check(push(&priority_queue, &vals[i].elem) != NULL, true);
        check(validate(&priority_queue), true);
    }
    size_t pop_count = 0;
    while (!CCC_priority_queue_is_empty(&priority_queue))
    {
        check(pop(&priority_queue), CCC_RESULT_OK);
        ++pop_count;
        check(validate(&priority_queue), true);
    }
    check(pop_count, num_nodes);
    check_end();
}

check_static_begin(priority_queue_test_priority_removal)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        check(push(&priority_queue, &vals[i].elem) != NULL, true);
        check(validate(&priority_queue), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct Val *i = &vals[val];
        if (i->val > limit)
        {
            (void)CCC_priority_queue_extract(&priority_queue, &i->elem);
            check(validate(&priority_queue), true);
        }
    }
    check_end();
}

check_static_begin(priority_queue_test_priority_update)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        check(push(&priority_queue, &vals[i].elem) != NULL, true);
        check(validate(&priority_queue), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct Val *i = &vals[val];
        int backoff = i->val / 2;
        if (i->val > limit)
        {
            check(CCC_priority_queue_update(&priority_queue, &i->elem,
                                            val_update, &backoff)
                      != NULL,
                  true);
            check(validate(&priority_queue), true);
        }
    }
    check(CCC_priority_queue_count(&priority_queue).count, num_nodes);
    check_end();
}

check_static_begin(priority_queue_test_priority_update_with)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        check(push(&priority_queue, &vals[i].elem) != NULL, true);
        check(validate(&priority_queue), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        int backoff = vals[val].val / 2;
        if (vals[val].val > limit)
        {
            check(CCC_priority_queue_update_with(&priority_queue, &vals[val],
                                                 { T->val = backoff; })
                      != NULL,
                  true);
            check(validate(&priority_queue), true);
        }
    }
    check(CCC_priority_queue_count(&priority_queue).count, num_nodes);
    check_end();
}

check_static_begin(priority_queue_test_priority_increase)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        check(push(&priority_queue, &vals[i].elem) != NULL, true);
        check(validate(&priority_queue), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct Val *const i = &vals[val];
        int inc = (limit * 2) + 1;
        int dec = (i->val / 2) - 1;
        if (i->val > limit && dec < i->val)
        {
            check(CCC_priority_queue_decrease(&priority_queue, &i->elem,
                                              val_update, &dec)
                      != NULL,
                  true);
            check(validate(&priority_queue), true);
        }
        else if (i->val < limit && inc > i->val)
        {
            check(CCC_priority_queue_increase(&priority_queue, &i->elem,
                                              val_update, &inc)
                      != NULL,
                  true);
            check(validate(&priority_queue), true);
        }
    }
    check(CCC_priority_queue_count(&priority_queue).count, num_nodes);
    check_end();
}

check_static_begin(priority_queue_test_priority_increase_with)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        check(push(&priority_queue, &vals[i].elem) != NULL, true);
        check(validate(&priority_queue), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        int inc = (limit * 2) + 1;
        int dec = (vals[val].val / 2) - 1;
        if (vals[val].val > limit && dec < vals[val].val)
        {
            check(CCC_priority_queue_decrease_with(&priority_queue, &vals[val],
                                                   { T->val = dec; })
                      != NULL,
                  true);
            check(validate(&priority_queue), true);
        }
        else if (vals[val].val < limit && inc > vals[val].val)
        {
            check(CCC_priority_queue_increase_with(&priority_queue, &vals[val],
                                                   { T->val = inc; })
                      != NULL,
                  true);
            check(validate(&priority_queue), true);
        }
    }
    check(CCC_priority_queue_count(&priority_queue).count, num_nodes);
    check_end();
}

check_static_begin(priority_queue_test_priority_decrease)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_GREATER, val_order, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        check(push(&priority_queue, &vals[i].elem) != NULL, true);
        check(validate(&priority_queue), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct Val *const i = &vals[val];
        int inc = (limit * 2) + 1;
        int dec = (i->val / 2) - 1;
        if (i->val < limit && inc > i->val)
        {
            check(CCC_priority_queue_increase(&priority_queue, &i->elem,
                                              val_update, &inc)
                      != NULL,
                  true);
            check(validate(&priority_queue), true);
        }
        else if (i->val > limit && dec < i->val)
        {
            check(CCC_priority_queue_decrease(&priority_queue, &i->elem,
                                              val_update, &dec)
                      != NULL,
                  true);
            check(validate(&priority_queue), true);
        }
    }
    check(CCC_priority_queue_count(&priority_queue).count, num_nodes);
    check_end();
}

check_static_begin(priority_queue_test_priority_decrease_with)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_GREATER, val_order, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        check(push(&priority_queue, &vals[i].elem) != NULL, true);
        check(validate(&priority_queue), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        int inc = (limit * 2) + 1;
        int dec = (vals[val].val / 2) - 1;
        if (vals[val].val < limit && inc > vals[val].val)
        {
            check(CCC_priority_queue_increase_with(&priority_queue, &vals[val],
                                                   { T->val = inc; })
                      != NULL,
                  true);
            check(validate(&priority_queue), true);
        }
        else if (vals[val].val > limit && dec < vals[val].val)
        {
            check(CCC_priority_queue_decrease_with(&priority_queue, &vals[val],
                                                   { T->val = dec; })
                      != NULL,
                  true);
            check(validate(&priority_queue), true);
        }
    }
    check(CCC_priority_queue_count(&priority_queue).count, num_nodes);
    check_end();
}

int
main()
{
    return check_run(priority_queue_test_insert_iterate_pop(),
                     priority_queue_test_priority_update(),
                     priority_queue_test_priority_update_with(),
                     priority_queue_test_priority_removal(),
                     priority_queue_test_priority_increase(),
                     priority_queue_test_priority_increase_with(),
                     priority_queue_test_priority_decrease(),
                     priority_queue_test_priority_decrease_with());
}
