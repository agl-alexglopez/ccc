#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_priority_queue.h"
#include "flat_priority_queue_util.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_insert_remove_four_dups)
{
    struct Val three_vals[4 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(
            three_vals, struct Val, CCC_ORDER_LESSER, val_order, NULL, NULL,
            (sizeof(three_vals) / sizeof(three_vals[0])));
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        CHECK(push(&flat_priority_queue, &three_vals[i], &(struct Val){})
                  != NULL,
              true);
        CHECK(validate(&flat_priority_queue), true);
        size_t const size_check = i + 1;
        CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count,
              size_check);
    }
    CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count, (size_t)4);
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        (void)pop(&flat_priority_queue, &(struct Val){});
        CHECK(validate(&flat_priority_queue), true);
    }
    CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count, (size_t)0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_insert_erase_shuffled)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    CHECK(insert_shuffled(&flat_priority_queue, vals, size, prime), PASS);
    struct Val const *min = front(&flat_priority_queue);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &flat_priority_queue), PASS);
    /* Now let's delete everything with no errors. */
    while (!CCC_flat_priority_queue_is_empty(&flat_priority_queue))
    {
        size_t const rand_index = rand_range(
            0,
            (int)CCC_flat_priority_queue_count(&flat_priority_queue).count - 1);
        (void)CCC_flat_priority_queue_erase(&flat_priority_queue,
                                            &vals[rand_index], &(struct Val){});
        CHECK(validate(&flat_priority_queue), true);
    }
    CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count, (size_t)0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_pop_max)
{
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    CHECK(insert_shuffled(&flat_priority_queue, vals, size, prime), PASS);
    struct Val const *min = front(&flat_priority_queue);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &flat_priority_queue), PASS);
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct Val const *const front = front(&flat_priority_queue);
        CHECK(front->val, sorted_check[i]);
        (void)pop(&flat_priority_queue, &(struct Val){});
    }
    CHECK(CCC_flat_priority_queue_is_empty(&flat_priority_queue), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_pop_min)
{
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    CHECK(insert_shuffled(&flat_priority_queue, vals, size, prime), PASS);
    struct Val const *min = front(&flat_priority_queue);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &flat_priority_queue), PASS);
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct Val const *const front = front(&flat_priority_queue);
        CHECK(front->val, sorted_check[i]);
        (void)pop(&flat_priority_queue, &(struct Val){});
    }
    CHECK(CCC_flat_priority_queue_is_empty(&flat_priority_queue), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_delete_prime_shuffle_duplicates)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const size = 99;
    int const prime = 101;
    /* Make the prime shuffle shorter than size for many duplicates. */
    int const less = 77;
    struct Val vals[99 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    int shuffled_index = prime % (size - less);
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = i;
        CHECK(push(&flat_priority_queue, &vals[i], &(struct Val){}) != NULL,
              true);
        CHECK(validate(&flat_priority_queue), true);
        size_t const s = i + 1;
        CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count, s);
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    size_t cur_size = size;
    while (!CCC_flat_priority_queue_is_empty(&flat_priority_queue))
    {
        size_t const rand_index = rand_range(
            0,
            (int)CCC_flat_priority_queue_count(&flat_priority_queue).count - 1);
        (void)CCC_flat_priority_queue_erase(&flat_priority_queue,
                                            &vals[rand_index], &(struct Val){});
        CHECK(validate(&flat_priority_queue), true);
        --cur_size;
        CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count,
              cur_size);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_prime_shuffle)
{
    int const size = 50;
    int const prime = 53;
    int const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    int shuffled_index = prime % (size - less);
    struct Val vals[50 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = shuffled_index;
        CHECK(push(&flat_priority_queue, &vals[i], &(struct Val){}) != NULL,
              true);
        CHECK(validate(&flat_priority_queue), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    while (!CCC_flat_priority_queue_is_empty(&flat_priority_queue))
    {
        size_t const rand_index = rand_range(
            0,
            (int)CCC_flat_priority_queue_count(&flat_priority_queue).count - 1);
        CHECK(CCC_flat_priority_queue_erase(&flat_priority_queue,
                                            &vals[rand_index], &(struct Val){})
                  == CCC_RESULT_OK,
              true);
        CHECK(validate(&flat_priority_queue), true);
        --cur_size;
        CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count,
              cur_size);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_weak_srand)
{
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_stack_nodes = 1000;
    struct Val vals[1000 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    for (int i = 0; i < num_stack_nodes; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        CHECK(push(&flat_priority_queue, &vals[i], &(struct Val){}) != NULL,
              true);
        CHECK(validate(&flat_priority_queue), true);
    }
    while (!CCC_flat_priority_queue_is_empty(&flat_priority_queue))
    {
        size_t const rand_index = rand_range(
            0,
            (int)CCC_flat_priority_queue_count(&flat_priority_queue).count - 1);
        CHECK(CCC_flat_priority_queue_erase(&flat_priority_queue,
                                            &vals[rand_index], &(struct Val){})
                  == CCC_RESULT_OK,
              true);
        CHECK(validate(&flat_priority_queue), true);
    }
    CHECK(CCC_flat_priority_queue_is_empty(&flat_priority_queue), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(flat_priority_queue_test_insert_remove_four_dups(),
                     flat_priority_queue_test_insert_erase_shuffled(),
                     flat_priority_queue_test_pop_max(),
                     flat_priority_queue_test_pop_min(),
                     flat_priority_queue_test_delete_prime_shuffle_duplicates(),
                     flat_priority_queue_test_prime_shuffle(),
                     flat_priority_queue_test_weak_srand());
}
