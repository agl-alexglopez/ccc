#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_priority_queue.h"
#include "flat_priority_queue_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"
#include "utility/stack_allocator.h"

check_static_begin(flat_priority_queue_test_insert_one)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 8);
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_with_capacity(
            struct Val, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
            &allocator, 8);
    (void)push(&flat_priority_queue, &(struct Val){.val = 1}, &(struct Val){});
    check(CCC_flat_priority_queue_is_empty(&flat_priority_queue), false);
    check_end();
}

check_static_begin(flat_priority_queue_test_insert_three)
{
    size_t size = 3;
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 8);
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_with_capacity(
            struct Val, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
            &allocator, 8);
    for (size_t i = 0; i < size; ++i)
    {
        (void)push(&flat_priority_queue,
                   &(struct Val){
                       .val = (int)i,
                   },
                   &(struct Val){});
        check(validate(&flat_priority_queue), true);
        check(CCC_flat_priority_queue_count(&flat_priority_queue).count, i + 1);
    }
    check(CCC_flat_priority_queue_count(&flat_priority_queue).count, size);
    check_end();
}

check_static_begin(flat_priority_queue_test_insert_three_dups)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 8);
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_with_capacity(
            struct Val, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
            &allocator, 8);
    for (int i = 0; i < 3; ++i)
    {
        (void)push(&flat_priority_queue,
                   &(struct Val){
                       .val = 0,
                   },
                   &(struct Val){});
        check(validate(&flat_priority_queue), true);
        check(CCC_flat_priority_queue_count(&flat_priority_queue).count,
              (size_t)i + 1);
    }
    check(CCC_flat_priority_queue_count(&flat_priority_queue).count, (size_t)3);
    check_end();
}

check_static_begin(flat_priority_queue_test_insert_shuffle)
{
    size_t const size = 50;
    int const prime = 53;
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 50);
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_with_capacity(
            struct Val, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
            &allocator, 50);
    check(insert_shuffled(&flat_priority_queue, size, prime), CHECK_PASS);

    struct Val const *min = front(&flat_priority_queue);
    check(min->val, 0);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &flat_priority_queue), CHECK_PASS);
    int prev = sorted_check[0];
    for (size_t i = 0; i < size; ++i)
    {
        check(prev <= sorted_check[i], true);
        prev = sorted_check[i];
    }
    check_end();
}

/** This should remain dynamic allocator test to test resizing logic. */
check_static_begin(flat_priority_queue_test_insert_shuffle_grow)
{
    size_t const size = 50;
    int const prime = 53;
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(NULL, struct Val, CCC_ORDER_LESSER,
                                             val_order, std_allocate, NULL, 0);
    check(insert_shuffled(&flat_priority_queue, size, prime), CHECK_PASS);

    struct Val const *min = front(&flat_priority_queue);
    check(min->val, 0);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &flat_priority_queue), CHECK_PASS);
    int prev = sorted_check[0];
    for (size_t i = 0; i < size; ++i)
    {
        check(prev <= sorted_check[i], true);
        prev = sorted_check[i];
    }
    check_end((void)CCC_flat_priority_queue_clear_and_free(&flat_priority_queue,
                                                           NULL););
}

check_static_begin(flat_priority_queue_test_insert_shuffle_reserve)
{
    size_t const size = 50;
    int const prime = 53;
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 50);
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_with_capacity(struct Val, CCC_ORDER_LESSER,
                                                val_order, NULL, &allocator, 0);
    CCC_Result const r = CCC_flat_priority_queue_reserve(
        &flat_priority_queue, 50, stack_allocator_allocate);
    check(r, CCC_RESULT_OK);
    check(insert_shuffled(&flat_priority_queue, size, prime), CHECK_PASS);
    struct Val const *min = front(&flat_priority_queue);
    check(min->val, 0);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &flat_priority_queue), CHECK_PASS);
    int prev = sorted_check[0];
    for (size_t i = 0; i < size; ++i)
    {
        check(prev <= sorted_check[i], true);
        prev = sorted_check[i];
    }
    check_end(clear_and_free_reserve(&flat_priority_queue, NULL,
                                     stack_allocator_allocate););
}

check_static_begin(flat_priority_queue_test_read_max_min)
{
    size_t const size = 10;
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 10);
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_with_capacity(
            struct Val, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
            &allocator, 10);
    for (size_t i = 0; i < size; ++i)
    {
        (void)push(&flat_priority_queue,
                   &(struct Val){
                       .val = (int)size - (int)i,
                   },
                   &(struct Val){});
        check(validate(&flat_priority_queue), true);
        check(CCC_flat_priority_queue_count(&flat_priority_queue).count, i + 1);
    }
    check(CCC_flat_priority_queue_count(&flat_priority_queue).count,
          (size_t)10);
    struct Val const *min = front(&flat_priority_queue);
    check(min->val, size - (size - 1));
    check_end();
}

int
main()
{
    return check_run(flat_priority_queue_test_insert_one(),
                     flat_priority_queue_test_insert_three(),
                     flat_priority_queue_test_insert_three_dups(),
                     flat_priority_queue_test_insert_shuffle(),
                     flat_priority_queue_test_insert_shuffle_grow(),
                     flat_priority_queue_test_insert_shuffle_reserve(),
                     flat_priority_queue_test_read_max_min());
}
