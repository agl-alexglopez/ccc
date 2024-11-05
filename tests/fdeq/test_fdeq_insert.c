#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "fdeq_util.h"
#include "flat_double_ended_queue.h"
#include "test.h"
#include "traits.h"

#include <stddef.h>

BEGIN_STATIC_TEST(fdeq_test_insert_three)
{
    flat_double_ended_queue q = fdeq_init((int[3]){}, NULL, NULL, 3);
    CHECK(create_queue(&q, 3, (int[3]){0, 1, 2}), PASS);
    CHECK(size(&q), 3);
    END_TEST();
}

BEGIN_STATIC_TEST(fdeq_test_insert_overwrite_three)
{
    flat_double_ended_queue q = fdeq_init((int[3]){}, NULL, NULL, 3);
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
    (void)fdeq_push_front_range(&q, 2, (int[2]){11, 12});
    CHECK(validate(&q), true);
    CHECK(check_order(&q, 3, (int[3]){11, 12, 8}), PASS);
    v = front(&q);
    CHECK(v == NULL, false);
    CHECK(*v, 11);
    v = back(&q);
    CHECK(v == NULL, false);
    CHECK(*v, 8);
    CHECK(size(&q), 3);
    END_TEST();
}

BEGIN_STATIC_TEST(fdeq_test_push_back_ranges)
{
    flat_double_ended_queue q = fdeq_init((int[6]){}, NULL, NULL, 6);
    CHECK(create_queue(&q, 3, (int[3]){0, 1, 2}), PASS);
    CHECK(check_order(&q, 3, (int[3]){0, 1, 2}), PASS);
    (void)fdeq_push_back_range(&q, 2, (int[2]){3, 4});
    CHECK(check_order(&q, 5, (int[5]){0, 1, 2, 3, 4}), PASS);
    (void)fdeq_push_back_range(&q, 3, (int[3]){5, 6, 7});
    CHECK(check_order(&q, 6, (int[6]){2, 3, 4, 5, 6, 7}), PASS);
    (void)fdeq_push_back_range(&q, 4, (int[4]){9, 10, 11, 12});
    CHECK(check_order(&q, 6, (int[6]){6, 7, 9, 10, 11, 12}), PASS);
    (void)fdeq_push_back_range(&q, 5, (int[5]){13, 14, 15, 16, 17});
    CHECK(check_order(&q, 6, (int[6]){12, 13, 14, 15, 16, 17}), PASS);
    (void)fdeq_push_back_range(&q, 6, (int[6]){18, 19, 20, 21, 22, 23});
    CHECK(check_order(&q, 6, (int[6]){18, 19, 20, 21, 22, 23}), PASS);
    (void)fdeq_push_back_range(&q, 7, (int[7]){24, 25, 26, 27, 28, 29, 30});
    CHECK(check_order(&q, 6, (int[6]){25, 26, 27, 28, 29, 30}), PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(fdeq_test_push_front_ranges)
{
    flat_double_ended_queue q = fdeq_init((int[6]){}, NULL, NULL, 6);
    CHECK(create_queue(&q, 3, (int[3]){0, 1, 2}), PASS);
    CHECK(check_order(&q, 3, (int[3]){0, 1, 2}), PASS);
    (void)fdeq_push_front_range(&q, 2, (int[2]){3, 4});
    CHECK(check_order(&q, 5, (int[5]){3, 4, 0, 1, 2}), PASS);
    (void)fdeq_push_front_range(&q, 3, (int[3]){5, 6, 7});
    CHECK(check_order(&q, 6, (int[6]){5, 6, 7, 3, 4, 0}), PASS);
    (void)fdeq_push_front_range(&q, 4, (int[4]){9, 10, 11, 12});
    CHECK(check_order(&q, 6, (int[6]){9, 10, 11, 12, 5, 6}), PASS);
    (void)fdeq_push_front_range(&q, 5, (int[5]){13, 14, 15, 16, 17});
    CHECK(check_order(&q, 6, (int[6]){13, 14, 15, 16, 17, 9}), PASS);
    (void)fdeq_push_front_range(&q, 6, (int[6]){18, 19, 20, 21, 22, 23});
    CHECK(check_order(&q, 6, (int[6]){18, 19, 20, 21, 22, 23}), PASS);
    (void)fdeq_push_front_range(&q, 7, (int[7]){24, 25, 26, 27, 28, 29, 30});
    CHECK(check_order(&q, 6, (int[6]){25, 26, 27, 28, 29, 30}), PASS);
    END_TEST();
}

BEGIN_STATIC_TEST(fdeq_test_insert_ranges)
{
    flat_double_ended_queue q
        = fdeq_init(((int[6]){0, 1, 2}), NULL, NULL, 6, 3);
    CHECK(check_order(&q, 3, (int[3]){0, 1, 2}), PASS);
    (void)fdeq_insert_range(&q, fdeq_at(&q, 1), 2, (int[2]){3, 4});
    CHECK(check_order(&q, 5, (int[5]){0, 3, 4, 1, 2}), PASS);
    (void)fdeq_insert_range(&q, fdeq_at(&q, 1), 3, (int[3]){5, 6, 7});
    CHECK(check_order(&q, 6, (int[6]){5, 6, 7, 3, 4, 1}), PASS);
    (void)fdeq_insert_range(&q, fdeq_at(&q, 2), 4, (int[4]){8, 9, 10, 11});
    CHECK(check_order(&q, 6, (int[6]){8, 9, 10, 11, 7, 3}), PASS);
    (void)fdeq_insert_range(&q, fdeq_at(&q, 3), 5,
                            (int[5]){12, 13, 14, 15, 16});
    CHECK(check_order(&q, 6, (int[6]){12, 13, 14, 15, 16, 11}), PASS);
    (void)fdeq_insert_range(&q, fdeq_at(&q, 3), 6,
                            (int[6]){17, 18, 19, 20, 21, 22});
    CHECK(check_order(&q, 6, (int[6]){17, 18, 19, 20, 21, 22}), PASS);
    (void)fdeq_insert_range(&q, fdeq_at(&q, 3), 7,
                            (int[7]){23, 24, 25, 26, 27, 28, 29});
    CHECK(check_order(&q, 6, (int[6]){24, 25, 26, 27, 28, 29}), PASS);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(fdeq_test_insert_three(),
                     fdeq_test_insert_overwrite_three(),
                     fdeq_test_push_back_ranges(),
                     fdeq_test_push_front_ranges(), fdeq_test_insert_ranges());
}
