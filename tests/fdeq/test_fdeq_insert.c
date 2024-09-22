#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "fdeq_util.h"
#include "flat_double_ended_queue.h"
#include "test.h"
#include "traits.h"

#include <stddef.h>

BEGIN_STATIC_TEST(fdeq_test_insert_three)
{
    int vals[3] = {};
    flat_double_ended_queue q
        = fdeq_init(vals, sizeof(vals) / sizeof(int), int, NULL);
    CHECK(create_queue(&q, 3, (int[3]){0, 1, 2}), PASS);
    CHECK(size(&q), 3);
    END_TEST();
}

BEGIN_STATIC_TEST(fdeq_test_insert_overwrite_three)
{
    int vals[3] = {};
    flat_double_ended_queue q
        = fdeq_init(vals, sizeof(vals) / sizeof(int), int, NULL);
    CHECK(create_queue(&q, 3, (int[3]){0, 1, 2}), PASS);
    CHECK(size(&q), 3);
    (void)fdeq_push_back_range(&q, 3, (int[3]){3, 4, 5});
    CHECK(validate(&q), true);
    CHECK(check_order(&q, 3, (int[3]){3, 4, 5}), PASS);
    CHECK(size(&q), 3);
    int *v = front(&q);
    CHECK(v == NULL, false);
    CHECK(*v, 3);
    v = back(&q);
    CHECK(v == NULL, false);
    CHECK(*v, 5);
    (void)fdeq_push_front_range(&q, 3, (int[3]){6, 7, 8});
    CHECK(validate(&q), true);
    CHECK(check_order(&q, 3, (int[3]){6, 7, 8}), PASS);
    v = front(&q);
    CHECK(v == NULL, false);
    CHECK(*v, 6);
    v = back(&q);
    CHECK(v == NULL, false);
    CHECK(*v, 8);
    CHECK(size(&q), 3);
    (void)fdeq_push_back_range(&q, 2, (int[2]){9, 10});
    CHECK(validate(&q), true);
    CHECK(check_order(&q, 3, (int[3]){8, 9, 10}), PASS);
    v = front(&q);
    CHECK(v == NULL, false);
    CHECK(*v, 8);
    v = back(&q);
    CHECK(v == NULL, false);
    CHECK(*v, 10);
    CHECK(size(&q), 3);
    END_TEST();
}

BEGIN_STATIC_TEST(fdeq_test_insert_three_with_extra_capacity)
{
    int vals[5] = {};
    flat_double_ended_queue q
        = fdeq_init(vals, sizeof(vals) / sizeof(int), int, NULL);
    CHECK(create_queue(&q, 3, (int[3]){0, 1, 2}), PASS);
    CHECK(size(&q), 3);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(fdeq_test_insert_three(),
                     fdeq_test_insert_overwrite_three(),
                     fdeq_test_insert_three_with_extra_capacity());
}
