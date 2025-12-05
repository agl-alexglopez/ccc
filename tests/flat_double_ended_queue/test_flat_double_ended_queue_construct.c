#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_double_ended_queue.h"
#include "traits.h"
#include "types.h"
#include "utility/stack_allocator.h"

check_static_begin(flat_double_ended_queue_test_construct)
{
    int vals[2];
    Flat_double_ended_queue q = flat_double_ended_queue_initialize(
        vals, int, NULL, NULL, sizeof(vals) / sizeof(int));
    check(is_empty(&q), true);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_copy_no_allocate)
{
    Flat_double_ended_queue q1 = flat_double_ended_queue_initialize(
        ((int[3]){0, 1, 2}), int, NULL, NULL, 3, 3);
    Flat_double_ended_queue q2 = CCC_flat_double_ended_queue_initialize(
        ((int[5]){}), int, NULL, NULL, 5);
    check(count(&q1).count, 3);
    check(*(int *)front(&q1), 0);
    check(is_empty(&q2), true);
    CCC_Result const res = flat_double_ended_queue_copy(&q2, &q1, NULL);
    check(res, CCC_RESULT_OK);
    check(count(&q2).count, 3);
    while (!is_empty(&q1) && !is_empty(&q2))
    {
        int f1 = *(int *)front(&q1);
        int f2 = *(int *)front(&q2);
        (void)pop_front(&q1);
        (void)pop_front(&q2);
        check(f1, f2);
    }
    check(is_empty(&q1), is_empty(&q2));
    check_end();
}

check_static_begin(flat_double_ended_queue_test_copy_no_allocate_fail)
{
    Flat_double_ended_queue q1 = flat_double_ended_queue_initialize(
        ((int[3]){0, 1, 2}), int, NULL, NULL, 3, 3);
    Flat_double_ended_queue q2 = CCC_flat_double_ended_queue_initialize(
        ((int[2]){}), int, NULL, NULL, 2);
    check(count(&q1).count, 3);
    check(*(int *)front(&q1), 0);
    check(is_empty(&q2), true);
    CCC_Result const res = flat_double_ended_queue_copy(&q2, &q1, NULL);
    check(res != CCC_RESULT_OK, true);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_copy_allocate)
{
    struct Stack_allocator allocator = stack_allocator_initialize(int, 16);
    Flat_double_ended_queue q1 = flat_double_ended_queue_with_capacity(
        int, stack_allocator_allocate, &allocator, 8);
    Flat_double_ended_queue q2 = CCC_flat_double_ended_queue_initialize(
        NULL, int, NULL, &allocator, 0);
    CCC_Result res = flat_double_ended_queue_push_back_range(
        &q1, 5, (int[5]){0, 1, 2, 3, 4});
    check(res, CCC_RESULT_OK);
    check(*(int *)front(&q1), 0);
    check(is_empty(&q2), true);
    res = flat_double_ended_queue_copy(&q2, &q1, stack_allocator_allocate);
    check(res, CCC_RESULT_OK);
    check(count(&q2).count, 5);
    while (!is_empty(&q1) && !is_empty(&q2))
    {
        int const f1 = *(int *)front(&q1);
        int const f2 = *(int *)front(&q2);
        (void)pop_front(&q1);
        (void)pop_front(&q2);
        check(f1, f2);
    }
    check(is_empty(&q1), is_empty(&q2));
    check_end({
        (void)flat_double_ended_queue_clear_and_free(&q1, NULL);
        (void)CCC_flat_double_ended_queue_clear_and_free_reserve(
            &q2, NULL, stack_allocator_allocate);
    });
}

check_static_begin(flat_double_ended_queue_test_copy_allocate_fail)
{
    struct Stack_allocator allocator = stack_allocator_initialize(int, 16);
    Flat_double_ended_queue q1 = flat_double_ended_queue_with_capacity(
        int, stack_allocator_allocate, &allocator, 8);
    Flat_double_ended_queue q2 = CCC_flat_double_ended_queue_initialize(
        NULL, int, NULL, &allocator, 0);
    CCC_Result res = flat_double_ended_queue_push_back_range(
        &q1, 5, (int[5]){0, 1, 2, 3, 4});
    check(res, CCC_RESULT_OK);
    check(*(int *)front(&q1), 0);
    check(is_empty(&q2), true);
    res = flat_double_ended_queue_copy(&q2, &q1, NULL);
    check(res != CCC_RESULT_OK, true);
    check_end({ (void)flat_double_ended_queue_clear_and_free(&q1, NULL); });
}

check_static_begin(flat_double_ended_queue_test_init_from)
{
    struct Stack_allocator allocator = stack_allocator_initialize(int, 8);
    CCC_Flat_double_ended_queue queue = CCC_flat_double_ended_queue_from(
        stack_allocator_allocate, &allocator, 8,
        (int[7]){
            1,
            2,
            3,
            4,
            5,
            6,
            7,
        });
    int elem = 1;
    for (int const *i = CCC_flat_double_ended_queue_begin(&queue);
         i != CCC_flat_double_ended_queue_end(&queue);
         i = CCC_flat_double_ended_queue_next(&queue, i))
    {
        check(*i, elem);
        ++elem;
    }
    check(elem, 8);
    check(CCC_flat_double_ended_queue_count(&queue).count, elem - 1);
    check(CCC_flat_double_ended_queue_capacity(&queue).count, elem);
    check_end((void)CCC_flat_double_ended_queue_clear_and_free(&queue, NULL););
}

check_static_begin(flat_double_ended_queue_test_init_from_fail)
{
    /* Whoops forgot allocation function. */
    CCC_Flat_double_ended_queue queue = CCC_flat_double_ended_queue_from(
        NULL, NULL, 0, (int[]){1, 2, 3, 4, 5, 6, 7});
    int elem = 1;
    for (int const *i = CCC_flat_double_ended_queue_begin(&queue);
         i != CCC_flat_double_ended_queue_end(&queue);
         i = CCC_flat_double_ended_queue_next(&queue, i))
    {
        check(elem, *i);
        ++elem;
    }
    check(elem, 1);
    check(CCC_flat_double_ended_queue_count(&queue).count, 0);
    check(CCC_flat_double_ended_queue_capacity(&queue).count, 0);
    check(CCC_flat_double_ended_queue_push_back(&queue, &(int){}), NULL);
    check_end((void)CCC_flat_double_ended_queue_clear_and_free(&queue, NULL););
}

check_static_begin(flat_double_ended_queue_test_init_with_capacity)
{
    struct Stack_allocator allocator = stack_allocator_initialize(int, 8);
    CCC_Flat_double_ended_queue queue
        = CCC_flat_double_ended_queue_with_capacity(
            int, stack_allocator_allocate, &allocator, 8);
    check(CCC_flat_double_ended_queue_capacity(&queue).count, 8);
    check(CCC_flat_double_ended_queue_push_back(&queue, &(int){9}) != NULL,
          CCC_TRUE);
    check_end(CCC_flat_double_ended_queue_clear_and_free(&queue, NULL););
}

check_static_begin(flat_double_ended_queue_test_init_with_capacity_fail)
{
    /* Forgot allocation function. */
    CCC_Flat_double_ended_queue queue
        = CCC_flat_double_ended_queue_with_capacity(int, NULL, NULL, 8);
    check(CCC_flat_double_ended_queue_capacity(&queue).count, 0);
    check(CCC_flat_double_ended_queue_push_back(&queue, &(int){9}), NULL);
    check_end(CCC_flat_double_ended_queue_clear_and_free(&queue, NULL););
}

int
main()
{
    return check_run(flat_double_ended_queue_test_construct(),
                     flat_double_ended_queue_test_copy_no_allocate(),
                     flat_double_ended_queue_test_copy_no_allocate_fail(),
                     flat_double_ended_queue_test_copy_allocate(),
                     flat_double_ended_queue_test_copy_allocate_fail(),
                     flat_double_ended_queue_test_init_from(),
                     flat_double_ended_queue_test_init_from_fail(),
                     flat_double_ended_queue_test_init_with_capacity(),
                     flat_double_ended_queue_test_init_with_capacity_fail());
}
