#include <limits.h>
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
#include "utility/stack_allocator.h"

enum : int
{
    STANDARD_CAP = 50,
    LARGE_CAP = 99,
    WEAK_SRAND_HEAP_CAP = 1000,
};

check_static_begin(priority_queue_test_insert_remove_four_dups)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 4);
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
        &allocator);
    for (int i = 0; i < 4; ++i)
    {
        check(push(&priority_queue, &(struct Val){.val = 0}.elem) != NULL,
              true);
        check(validate(&priority_queue), true);
        size_t const size = i + 1;
        check(CCC_priority_queue_count(&priority_queue).count, size);
    }
    check(CCC_priority_queue_count(&priority_queue).count, (size_t)4);
    for (int i = 0; i < 4; ++i)
    {
        check(pop(&priority_queue), CCC_RESULT_OK);
        check(validate(&priority_queue), true);
    }
    check(CCC_priority_queue_count(&priority_queue).count, (size_t)0);
    check_end();
}

check_static_begin(priority_queue_test_insert_extract_shuffled)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, STANDARD_CAP);
    CCC_Priority_queue queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
        &allocator);
    int const prime = 53;
    check(insert_shuffled(&queue, STANDARD_CAP, prime), CHECK_PASS);
    struct Val const *min = front(&queue);
    check(min->val, 0);
    check(check_inorder_fill(&queue, STANDARD_CAP), CHECK_PASS);
    /* Now let's delete everything with no errors. */
    struct Val const *const end
        = (struct Val *)((char *)allocator.blocks
                         + (sizeof(struct Val) * STANDARD_CAP));
    for (struct Val *i = allocator.blocks; i != end; ++i)
    {
        (void)CCC_priority_queue_extract(&queue, &i->elem);
        check(validate(&queue), true);
    }
    check(CCC_priority_queue_count(&queue).count, (size_t)0);
    check_end();
}

check_static_begin(priority_queue_test_pop_max)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, STANDARD_CAP);
    CCC_Priority_queue queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
        &allocator);
    int const prime = 53;
    check(insert_shuffled(&queue, STANDARD_CAP, prime), CHECK_PASS);
    struct Val const *min = front(&queue);
    check(min->val, 0);
    check(check_inorder_fill(&queue, STANDARD_CAP), CHECK_PASS);
    /* Now let's pop from the front of the queue until empty. */
    int prev_val = INT_MIN;
    for (size_t i = 0; i < STANDARD_CAP; ++i)
    {
        struct Val const *front = front(&queue);
        check(front->val > prev_val, true);
        prev_val = front->val;
        check(pop(&queue), CCC_RESULT_OK);
    }
    check(CCC_priority_queue_is_empty(&queue), true);
    check_end();
}

check_static_begin(priority_queue_test_pop_min)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, STANDARD_CAP);
    CCC_Priority_queue queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
        &allocator);
    int const prime = 53;
    check(insert_shuffled(&queue, STANDARD_CAP, prime), CHECK_PASS);
    struct Val const *min = front(&queue);
    check(min->val, 0);
    check(check_inorder_fill(&queue, STANDARD_CAP), CHECK_PASS);
    /* Now let's pop from the front of the queue until empty. */
    int prev_val = INT_MIN;
    for (size_t i = 0; i < STANDARD_CAP; ++i)
    {
        struct Val const *front = front(&queue);
        check(front->val > prev_val, true);
        prev_val = front->val;
        check(pop(&queue), CCC_RESULT_OK);
    }
    check(CCC_priority_queue_is_empty(&queue), true);
    check_end();
}

check_static_begin(priority_queue_test_delete_prime_shuffle_duplicates)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, LARGE_CAP);
    CCC_Priority_queue queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
        &allocator);
    int const prime = 101;
    /* Make the prime shuffle shorter than size for many duplicates. */
    int const less = 77;
    int shuffled_index = prime % (LARGE_CAP - less);
    for (int i = 0; i < LARGE_CAP; ++i)
    {
        struct Val const *const pushed
            = push(&queue, &(struct Val){.val = shuffled_index, .id = i}.elem);
        check(pushed != NULL, true);
        check(validate(&queue), true);
        size_t const s = i + 1;
        check(CCC_priority_queue_count(&queue).count, s);
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (LARGE_CAP - less);
    }

    shuffled_index = prime % (LARGE_CAP - less);
    size_t cur_size = LARGE_CAP;
    struct Val *const val_array = allocator.blocks;
    for (int i = 0; i < LARGE_CAP; ++i)
    {
        (void)CCC_priority_queue_extract(&queue,
                                         &val_array[shuffled_index].elem);
        check(validate(&queue), true);
        --cur_size;
        check(CCC_priority_queue_count(&queue).count, cur_size);
        /* Shuffle normally here so we only remove each elem once. */
        shuffled_index = (shuffled_index + prime) % LARGE_CAP;
    }
    check_end();
}

check_static_begin(priority_queue_test_prime_shuffle)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, STANDARD_CAP);
    CCC_Priority_queue queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
        &allocator);
    int const prime = 53;
    int const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    int shuffled_index = prime % (STANDARD_CAP - less);
    for (int i = 0; i < STANDARD_CAP; ++i)
    {
        struct Val const *const pushed = push(
            &queue,
            &(struct Val){.val = shuffled_index, .id = shuffled_index}.elem);
        check(pushed != NULL, true);
        check(validate(&queue), true);
        shuffled_index = (shuffled_index + prime) % (STANDARD_CAP - less);
    }
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = STANDARD_CAP;
    struct Val *const val_array = allocator.blocks;
    for (int i = 0; i < STANDARD_CAP; ++i)
    {
        (void)CCC_priority_queue_extract(&queue, &val_array[i].elem);
        check(validate(&queue), true);
        --cur_size;
        check(CCC_priority_queue_count(&queue).count, cur_size);
    }
    check_end();
}

check_static_begin(priority_queue_test_weak_srand)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, WEAK_SRAND_HEAP_CAP);
    CCC_Priority_queue queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
        &allocator);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    for (int i = 0; i < WEAK_SRAND_HEAP_CAP; ++i)
    {
        struct Val const *const pushed = push(&queue,
                                              &(struct Val){
                                                  .val = rand(), /* NOLINT */
                                                  .id = i,
                                              }
                                                   .elem);
        check(pushed != NULL, true);
        check(validate(&queue), true);
    }
    struct Val *const val_array = allocator.blocks;
    for (int i = 0; i < WEAK_SRAND_HEAP_CAP; ++i)
    {
        (void)CCC_priority_queue_extract(&queue, &val_array[i].elem);
        check(validate(&queue), true);
    }
    check(CCC_priority_queue_is_empty(&queue), true);
    check_end();
}

check_static_begin(priority_queue_test_weak_srand_allocate)
{
    CCC_Priority_queue queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, std_allocate, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_heap_nodes = 100;
    for (int i = 0; i < num_heap_nodes; ++i)
    {
        check(push(&queue,
                   &(struct Val){
                       .id = i,
                       .val = rand() /*NOLINT*/,
                   }
                        .elem)
                  != NULL,
              true);
        check(validate(&queue), true);
    }
    check_end(CCC_priority_queue_clear(&queue, NULL););
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
