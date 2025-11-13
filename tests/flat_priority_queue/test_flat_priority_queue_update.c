#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_priority_queue.h"
#include "flat_priority_queue_utility.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_insert_iterate_pop)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(1);
    size_t const num_nodes = 1000;
    struct Val vals[1000 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(push(&flat_priority_queue, &vals[i], &(struct Val){}) != NULL,
              true);
        CHECK(validate(&flat_priority_queue), true);
    }
    size_t pop_count = 0;
    while (!is_empty(&flat_priority_queue))
    {
        (void)pop(&flat_priority_queue, &(struct Val){});
        ++pop_count;
        CHECK(validate(&flat_priority_queue), true);
    }
    CHECK(pop_count, num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_priority_removal)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        struct Val const *res = CCC_flat_priority_queue_emplace(
            &flat_priority_queue,
            (struct Val){
                .val = rand() % (num_nodes + 1), /*NOLINT*/
                .id = (int)i,
            });
        CHECK(res != NULL, true);
        CHECK(validate(&flat_priority_queue), true);
    }
    int const limit = 400;
    for (size_t seen = 0, remaining = num_nodes; seen < remaining; ++seen)
    {
        struct Val *cur = &vals[seen];
        if (cur->val > limit)
        {
            (void)erase(&flat_priority_queue, cur, &(struct Val){});
            CHECK(validate(&flat_priority_queue), true);
            --remaining;
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_priority_update)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        struct Val const *res = CCC_flat_priority_queue_emplace(
            &flat_priority_queue,
            (struct Val){
                .val = rand() % (num_nodes + 1), /*NOLINT*/
                .id = (int)i,
            });
        CHECK(res != NULL, true);
        CHECK(validate(&flat_priority_queue), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct Val *cur = &vals[val];
        int backoff = cur->val / 2;
        if (cur->val > limit)
        {
            struct Val const *const updated
                = update(&flat_priority_queue, cur, &(struct Val){}, val_update,
                         &backoff);
            CHECK(updated != NULL, true);
            CHECK(updated->val, backoff);
            CHECK(validate(&flat_priority_queue), true);
        }
    }
    CHECK(count(&flat_priority_queue).count, num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_priority_update_with)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct Val vals[1000 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        struct Val const *res = CCC_flat_priority_queue_emplace(
            &flat_priority_queue,
            (struct Val){
                .val = rand() % (num_nodes + 1), /*NOLINT*/
                .id = (int)i,
            });
        CHECK(res != NULL, true);
        CHECK(validate(&flat_priority_queue), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        int backoff = vals[val].val / 2;
        if (vals[val].val > limit)
        {
            struct Val const *const updated = CCC_flat_priority_queue_update_w(
                &flat_priority_queue, &vals[val],
                {
                    T->val = backoff;
                });
            CHECK(updated != NULL, true);
            CHECK(updated->val, backoff);
            CHECK(validate(&flat_priority_queue), true);
        }
    }
    CHECK(count(&flat_priority_queue).count, num_nodes);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(flat_priority_queue_test_insert_iterate_pop(),
                     flat_priority_queue_test_priority_update(),
                     flat_priority_queue_test_priority_update_with(),
                     flat_priority_queue_test_priority_removal());
}
