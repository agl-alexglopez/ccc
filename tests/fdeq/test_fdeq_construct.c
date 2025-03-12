#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
#include "alloc.h"
#include "checkers.h"
#include "flat_double_ended_queue.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(fdeq_test_construct)
{
    int vals[2];
    flat_double_ended_queue q
        = fdeq_init(vals, NULL, NULL, sizeof(vals) / sizeof(int));
    CHECK(is_empty(&q), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fdeq_test_copy_no_alloc)
{
    flat_double_ended_queue q1
        = fdeq_init(((int[3]){0, 1, 2}), NULL, NULL, 3, 3);
    flat_double_ended_queue q2 = ccc_fdeq_init(((int[5]){}), NULL, NULL, 5);
    CHECK(size(&q1), 3);
    CHECK(*(int *)front(&q1), 0);
    CHECK(is_empty(&q2), true);
    ccc_result res = fdeq_copy(&q2, &q1, NULL);
    CHECK(res, CCC_RESULT_OK);
    CHECK(size(&q2), 3);
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

CHECK_BEGIN_STATIC_FN(fdeq_test_copy_no_alloc_fail)
{
    flat_double_ended_queue q1
        = fdeq_init(((int[3]){0, 1, 2}), NULL, NULL, 3, 3);
    flat_double_ended_queue q2 = ccc_fdeq_init(((int[2]){}), NULL, NULL, 2);
    CHECK(size(&q1), 3);
    CHECK(*(int *)front(&q1), 0);
    CHECK(is_empty(&q2), true);
    ccc_result res = fdeq_copy(&q2, &q1, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fdeq_test_copy_alloc)
{
    flat_double_ended_queue q1 = fdeq_init((int *)NULL, std_alloc, NULL, 0);
    flat_double_ended_queue q2 = ccc_fdeq_init((int *)NULL, NULL, NULL, 0);
    ccc_result res = fdeq_push_back_range(&q1, 5, (int[5]){0, 1, 2, 3, 4});
    CHECK(res, CCC_RESULT_OK);
    CHECK(*(int *)front(&q1), 0);
    CHECK(is_empty(&q2), true);
    res = fdeq_copy(&q2, &q1, std_alloc);
    CHECK(res, CCC_RESULT_OK);
    CHECK(size(&q2), 5);
    while (!is_empty(&q1) && !is_empty(&q2))
    {
        int f1 = *(int *)front(&q1);
        int f2 = *(int *)front(&q2);
        (void)pop_front(&q1);
        (void)pop_front(&q2);
        CHECK(f1, f2);
    }
    CHECK(is_empty(&q1), is_empty(&q2));
    CHECK_END_FN({
        (void)fdeq_clear_and_free(&q1, NULL);
        std_alloc(fdeq_data(&q2), 0, NULL);
    });
}

CHECK_BEGIN_STATIC_FN(fdeq_test_copy_alloc_fail)
{
    flat_double_ended_queue q1 = fdeq_init((int *)NULL, std_alloc, NULL, 0);
    flat_double_ended_queue q2 = ccc_fdeq_init((int *)NULL, NULL, NULL, 0);
    ccc_result res = fdeq_push_back_range(&q1, 5, (int[5]){0, 1, 2, 3, 4});
    CHECK(res, CCC_RESULT_OK);
    CHECK(*(int *)front(&q1), 0);
    CHECK(is_empty(&q2), true);
    res = fdeq_copy(&q2, &q1, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN({ (void)fdeq_clear_and_free(&q1, NULL); });
}

int
main()
{
    return CHECK_RUN(fdeq_test_construct(), fdeq_test_copy_no_alloc(),
                     fdeq_test_copy_no_alloc_fail(), fdeq_test_copy_alloc(),
                     fdeq_test_copy_alloc_fail());
}
