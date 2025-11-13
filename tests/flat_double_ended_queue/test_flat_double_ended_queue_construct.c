#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_double_ended_queue.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

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
    Flat_double_ended_queue q1
        = flat_double_ended_queue_initialize(NULL, int, std_allocate, NULL, 0);
    Flat_double_ended_queue q2
        = CCC_flat_double_ended_queue_initialize(NULL, int, NULL, NULL, 0);
    CCC_Result res = flat_double_ended_queue_push_back_range(
        &q1, 5, (int[5]){0, 1, 2, 3, 4});
    check(res, CCC_RESULT_OK);
    check(*(int *)front(&q1), 0);
    check(is_empty(&q2), true);
    res = flat_double_ended_queue_copy(&q2, &q1, std_allocate);
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
        (void)CCC_flat_double_ended_queue_clear_and_free_reserve(&q2, NULL,
                                                                 std_allocate);
    });
}

check_static_begin(flat_double_ended_queue_test_copy_allocate_fail)
{
    Flat_double_ended_queue q1
        = flat_double_ended_queue_initialize(NULL, int, std_allocate, NULL, 0);
    Flat_double_ended_queue q2
        = CCC_flat_double_ended_queue_initialize(NULL, int, NULL, NULL, 0);
    CCC_Result res = flat_double_ended_queue_push_back_range(
        &q1, 5, (int[5]){0, 1, 2, 3, 4});
    check(res, CCC_RESULT_OK);
    check(*(int *)front(&q1), 0);
    check(is_empty(&q2), true);
    res = flat_double_ended_queue_copy(&q2, &q1, NULL);
    check(res != CCC_RESULT_OK, true);
    check_end({ (void)flat_double_ended_queue_clear_and_free(&q1, NULL); });
}

int
main()
{
    return check_run(flat_double_ended_queue_test_construct(),
                     flat_double_ended_queue_test_copy_no_allocate(),
                     flat_double_ended_queue_test_copy_no_allocate_fail(),
                     flat_double_ended_queue_test_copy_allocate(),
                     flat_double_ended_queue_test_copy_allocate_fail());
}
