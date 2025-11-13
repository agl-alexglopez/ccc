#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_double_ended_queue.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

CHECK_BEGIN_STATIC_FN(flat_double_ended_queue_test_construct)
{
    int vals[2];
    flat_double_ended_queue q = flat_double_ended_queue_initialize(
        vals, int, NULL, NULL, sizeof(vals) / sizeof(int));
    CHECK(is_empty(&q), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_double_ended_queue_test_copy_no_alloc)
{
    flat_double_ended_queue q1 = flat_double_ended_queue_initialize(
        ((int[3]){0, 1, 2}), int, NULL, NULL, 3, 3);
    flat_double_ended_queue q2 = CCC_flat_double_ended_queue_initialize(
        ((int[5]){}), int, NULL, NULL, 5);
    CHECK(count(&q1).count, 3);
    CHECK(*(int *)front(&q1), 0);
    CHECK(is_empty(&q2), true);
    CCC_Result const res = flat_double_ended_queue_copy(&q2, &q1, NULL);
    CHECK(res, CCC_RESULT_OK);
    CHECK(count(&q2).count, 3);
    while (!is_empty(&q1) && !is_empty(&q2))
    {
        int f1 = *(int *)front(&q1);
        int f2 = *(int *)front(&q2);
        (void)pop_front(&q1);
        (void)pop_front(&q2);
        CHECK(f1, f2);
    }
    CHECK(is_empty(&q1), is_empty(&q2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_double_ended_queue_test_copy_no_alloc_fail)
{
    flat_double_ended_queue q1 = flat_double_ended_queue_initialize(
        ((int[3]){0, 1, 2}), int, NULL, NULL, 3, 3);
    flat_double_ended_queue q2 = CCC_flat_double_ended_queue_initialize(
        ((int[2]){}), int, NULL, NULL, 2);
    CHECK(count(&q1).count, 3);
    CHECK(*(int *)front(&q1), 0);
    CHECK(is_empty(&q2), true);
    CCC_Result const res = flat_double_ended_queue_copy(&q2, &q1, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_double_ended_queue_test_copy_alloc)
{
    flat_double_ended_queue q1
        = flat_double_ended_queue_initialize(NULL, int, std_alloc, NULL, 0);
    flat_double_ended_queue q2
        = CCC_flat_double_ended_queue_initialize(NULL, int, NULL, NULL, 0);
    CCC_Result res = flat_double_ended_queue_push_back_range(
        &q1, 5, (int[5]){0, 1, 2, 3, 4});
    CHECK(res, CCC_RESULT_OK);
    CHECK(*(int *)front(&q1), 0);
    CHECK(is_empty(&q2), true);
    res = flat_double_ended_queue_copy(&q2, &q1, std_alloc);
    CHECK(res, CCC_RESULT_OK);
    CHECK(count(&q2).count, 5);
    while (!is_empty(&q1) && !is_empty(&q2))
    {
        int const f1 = *(int *)front(&q1);
        int const f2 = *(int *)front(&q2);
        (void)pop_front(&q1);
        (void)pop_front(&q2);
        CHECK(f1, f2);
    }
    CHECK(is_empty(&q1), is_empty(&q2));
    CHECK_END_FN({
        (void)flat_double_ended_queue_clear_and_free(&q1, NULL);
        (void)CCC_flat_double_ended_queue_clear_and_free_reserve(&q2, NULL,
                                                                 std_alloc);
    });
}

CHECK_BEGIN_STATIC_FN(flat_double_ended_queue_test_copy_alloc_fail)
{
    flat_double_ended_queue q1
        = flat_double_ended_queue_initialize(NULL, int, std_alloc, NULL, 0);
    flat_double_ended_queue q2
        = CCC_flat_double_ended_queue_initialize(NULL, int, NULL, NULL, 0);
    CCC_Result res = flat_double_ended_queue_push_back_range(
        &q1, 5, (int[5]){0, 1, 2, 3, 4});
    CHECK(res, CCC_RESULT_OK);
    CHECK(*(int *)front(&q1), 0);
    CHECK(is_empty(&q2), true);
    res = flat_double_ended_queue_copy(&q2, &q1, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN({ (void)flat_double_ended_queue_clear_and_free(&q1, NULL); });
}

int
main()
{
    return CHECK_RUN(flat_double_ended_queue_test_construct(),
                     flat_double_ended_queue_test_copy_no_alloc(),
                     flat_double_ended_queue_test_copy_no_alloc_fail(),
                     flat_double_ended_queue_test_copy_alloc(),
                     flat_double_ended_queue_test_copy_alloc_fail());
}
