#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_priority_queue.h"
#include "fpq_util.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

CHECK_BEGIN_STATIC_FN(fpq_test_insert_iterate_pop)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(1);
    size_t const num_nodes = 1000;
    struct val vals[1000 + 1];
    ccc_flat_priority_queue fpq = ccc_fpq_init(
        vals, CCC_LES, val_cmp, NULL, NULL, (sizeof(vals) / sizeof(vals[0])));
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        CHECK(push(&fpq, &vals[i]) != NULL, true);
        CHECK(validate(&fpq), true);
    }
    size_t pop_count = 0;
    while (!is_empty(&fpq))
    {
        (void)pop(&fpq);
        ++pop_count;
        CHECK(validate(&fpq), true);
    }
    CHECK(pop_count, num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_priority_removal)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[1000 + 1];
    ccc_flat_priority_queue fpq = ccc_fpq_init(
        vals, CCC_LES, val_cmp, NULL, NULL, (sizeof(vals) / sizeof(vals[0])));
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        struct val const *res = ccc_fpq_emplace(
            &fpq, (struct val){
                      .val = rand() % (num_nodes + 1), /*NOLINT*/
                      .id = (int)i,
                  });
        CHECK(res != NULL, true);
        CHECK(validate(&fpq), true);
    }
    int const limit = 400;
    for (size_t seen = 0, remaining = num_nodes; seen < remaining; ++seen)
    {
        struct val *cur = &vals[seen];
        if (cur->val > limit)
        {
            (void)erase(&fpq, cur);
            CHECK(validate(&fpq), true);
            --remaining;
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_priority_update)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[1000 + 1];
    ccc_flat_priority_queue fpq = ccc_fpq_init(
        vals, CCC_LES, val_cmp, NULL, NULL, (sizeof(vals) / sizeof(vals[0])));
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        struct val const *res = ccc_fpq_emplace(
            &fpq, (struct val){
                      .val = rand() % (num_nodes + 1), /*NOLINT*/
                      .id = (int)i,
                  });
        CHECK(res != NULL, true);
        CHECK(validate(&fpq), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *cur = &vals[val];
        int backoff = cur->val / 2;
        if (cur->val > limit)
        {
            struct val const *const updated
                = update(&fpq, cur, val_update, &backoff);
            CHECK(updated != NULL, true);
            CHECK(updated->val, backoff);
            CHECK(validate(&fpq), true);
        }
    }
    CHECK(size(&fpq), num_nodes);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_priority_update_with)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[1000 + 1];
    ccc_flat_priority_queue fpq = ccc_fpq_init(
        vals, CCC_LES, val_cmp, NULL, NULL, (sizeof(vals) / sizeof(vals[0])));
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        struct val const *res = ccc_fpq_emplace(
            &fpq, (struct val){
                      .val = rand() % (num_nodes + 1), /*NOLINT*/
                      .id = (int)i,
                  });
        CHECK(res != NULL, true);
        CHECK(validate(&fpq), true);
    }
    int const limit = 400;
    for (size_t val = 0; val < num_nodes; ++val)
    {
        struct val *cur = &vals[val];
        int backoff = cur->val / 2;
        if (cur->val > limit)
        {
            struct val const *const updated
                = ccc_fpq_update_w(&fpq, cur,
                                   {
                                       cur->val = backoff;
                                   });
            CHECK(updated != NULL, true);
            CHECK(updated->val, backoff);
            CHECK(validate(&fpq), true);
        }
    }
    CHECK(size(&fpq), num_nodes);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(fpq_test_insert_iterate_pop(), fpq_test_priority_update(),
                     fpq_test_priority_update_with(),
                     fpq_test_priority_removal());
}
