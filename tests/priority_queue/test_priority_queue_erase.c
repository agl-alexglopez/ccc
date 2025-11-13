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
#include "utility/allocate.h"

check_static_begin(priority_queue_test_insert_remove_four_dups)
{
    CCC_Priority_queue ppriority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    struct Val three_vals[4];
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        check(push(&ppriority_queue, &three_vals[i].elem) != NULL, true);
        check(validate(&ppriority_queue), true);
        size_t const size = i + 1;
        check(CCC_priority_queue_count(&ppriority_queue).count, size);
    }
    check(CCC_priority_queue_count(&ppriority_queue).count, (size_t)4);
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        check(pop(&ppriority_queue), CCC_RESULT_OK);
        check(validate(&ppriority_queue), true);
    }
    check(CCC_priority_queue_count(&ppriority_queue).count, (size_t)0);
    check_end();
}

check_static_begin(priority_queue_test_insert_extract_shuffled)
{
    CCC_Priority_queue ppriority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50];
    check(insert_shuffled(&ppriority_queue, vals, size, prime), CHECK_PASS);
    struct Val const *min = front(&ppriority_queue);
    check(min->val, 0);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &ppriority_queue), CHECK_PASS);
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        (void)CCC_priority_queue_extract(&ppriority_queue, &vals[i].elem);
        check(validate(&ppriority_queue), true);
    }
    check(CCC_priority_queue_count(&ppriority_queue).count, (size_t)0);
    check_end();
}

check_static_begin(priority_queue_test_pop_max)
{
    CCC_Priority_queue ppriority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50];
    check(insert_shuffled(&ppriority_queue, vals, size, prime), CHECK_PASS);
    struct Val const *min = front(&ppriority_queue);
    check(min->val, 0);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &ppriority_queue), CHECK_PASS);
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct Val const *front = front(&ppriority_queue);
        check(front->val, vals[i].val);
        check(pop(&ppriority_queue), CCC_RESULT_OK);
    }
    check(CCC_priority_queue_is_empty(&ppriority_queue), true);
    check_end();
}

check_static_begin(priority_queue_test_pop_min)
{
    CCC_Priority_queue ppriority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50];
    check(insert_shuffled(&ppriority_queue, vals, size, prime), CHECK_PASS);
    struct Val const *min = front(&ppriority_queue);
    check(min->val, 0);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &ppriority_queue), CHECK_PASS);
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct Val const *front = front(&ppriority_queue);
        check(front->val, vals[i].val);
        check(pop(&ppriority_queue), CCC_RESULT_OK);
    }
    check(CCC_priority_queue_is_empty(&ppriority_queue), true);
    check_end();
}

check_static_begin(priority_queue_test_delete_prime_shuffle_duplicates)
{
    CCC_Priority_queue ppriority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    int const size = 99;
    int const prime = 101;
    /* Make the prime shuffle shorter than size for many duplicates. */
    int const less = 77;
    struct Val vals[99];
    int shuffled_index = prime % (size - less);
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = i;
        check(push(&ppriority_queue, &vals[i].elem) != NULL, true);
        check(validate(&ppriority_queue), true);
        size_t const s = i + 1;
        check(CCC_priority_queue_count(&ppriority_queue).count, s);
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }

    shuffled_index = prime % (size - less);
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        (void)CCC_priority_queue_extract(&ppriority_queue,
                                         &vals[shuffled_index].elem);
        check(validate(&ppriority_queue), true);
        --cur_size;
        check(CCC_priority_queue_count(&ppriority_queue).count, cur_size);
        /* Shuffle normally here so we only remove each elem once. */
        shuffled_index = (shuffled_index + prime) % size;
    }
    check_end();
}

check_static_begin(priority_queue_test_prime_shuffle)
{
    CCC_Priority_queue ppriority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    int const size = 50;
    int const prime = 53;
    int const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    int shuffled_index = prime % (size - less);
    struct Val vals[50];
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = shuffled_index;
        check(push(&ppriority_queue, &vals[i].elem) != NULL, true);
        check(validate(&ppriority_queue), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        (void)CCC_priority_queue_extract(&ppriority_queue, &vals[i].elem);
        check(validate(&ppriority_queue), true);
        --cur_size;
        check(CCC_priority_queue_count(&ppriority_queue).count, cur_size);
    }
    check_end();
}

check_static_begin(priority_queue_test_weak_srand)
{
    CCC_Priority_queue ppriority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_heap_nodes = 1000;
    struct Val vals[1000];
    for (int i = 0; i < num_heap_nodes; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        check(push(&ppriority_queue, &vals[i].elem) != NULL, true);
        check(validate(&ppriority_queue), true);
    }
    for (int i = 0; i < num_heap_nodes; ++i)
    {
        (void)CCC_priority_queue_extract(&ppriority_queue, &vals[i].elem);
        check(validate(&ppriority_queue), true);
    }
    check(CCC_priority_queue_is_empty(&ppriority_queue), true);
    check_end();
}

check_static_begin(priority_queue_test_weak_srand_allocate)
{
    CCC_Priority_queue ppriority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, std_allocate, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_heap_nodes = 100;
    for (int i = 0; i < num_heap_nodes; ++i)
    {
        check(push(&ppriority_queue,
                   &(struct Val){
                       .id = i,
                       .val = rand() /*NOLINT*/,
                   }
                        .elem)
                  != NULL,
              true);
        check(validate(&ppriority_queue), true);
    }
    check_end(CCC_priority_queue_clear(&ppriority_queue, NULL););
}

int
main()
{
    return check_run(
        priority_queue_test_insert_remove_four_dups(),
        priority_queue_test_insert_extract_shuffled(),
        priority_queue_test_pop_max(), priority_queue_test_pop_min(),
        priority_queue_test_delete_prime_shuffle_duplicates(),
        priority_queue_test_prime_shuffle(), priority_queue_test_weak_srand(),
        priority_queue_test_weak_srand_allocate());
}
