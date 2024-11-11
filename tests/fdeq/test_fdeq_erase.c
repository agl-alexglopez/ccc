#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "checkers.h"
#include "fdeq_util.h"
#include "flat_double_ended_queue.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>

CHECK_BEGIN_STATIC_FN(pop_front_n, flat_double_ended_queue *const q, size_t n)
{
    for (; n-- && !is_empty(q); (void)pop_front(q))
    {
        CHECK(validate(q), true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pop_back_n, flat_double_ended_queue *const q, size_t n)
{
    for (; n-- && !is_empty(q); (void)pop_back(q))
    {
        CHECK(validate(q), true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fdeq_test_push_pop_back_three)
{
    flat_double_ended_queue q = fdeq_init((int[3]){}, NULL, NULL, 3);
    CHECK(create_queue(&q, 3, (int[3]){0, 1, 2}), PASS);
    while (!is_empty(&q))
    {
        (void)pop_back(&q);
        CHECK(validate(&q), true);
    }
    CHECK(is_empty(&q), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fdeq_test_push_pop_front_three)
{
    flat_double_ended_queue q = fdeq_init((int[3]){}, NULL, NULL, 3);
    CHECK(create_queue(&q, 3, (int[3]){0, 1, 2}), PASS);
    while (!is_empty(&q))
    {
        (void)pop_front(&q);
        CHECK(validate(&q), true);
    }
    CHECK(is_empty(&q), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fdeq_test_push_pop_front_back)
{
    flat_double_ended_queue q = fdeq_init((int[6]){}, NULL, NULL, 6);
    CHECK(create_queue(&q, 6, (int[6]){0, 1, 2, 3, 4, 5}), PASS);
    while (!is_empty(&q))
    {
        if (size(&q) % 2)
        {
            (void)pop_front(&q);
        }
        else
        {
            (void)pop_back(&q);
        }
        CHECK(validate(&q), true);
    }
    CHECK(is_empty(&q), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fdeq_test_push_pop_front_ranges)
{
    flat_double_ended_queue q = fdeq_init((int[10]){}, NULL, NULL, 10);
    CHECK(create_queue(&q, 6, (int[6]){0, 1, 2, 3, 4, 5}), PASS);
    CHECK(pop_back_n(&q, 4), PASS);
    ccc_result res = fdeq_push_front_range(&q, 4, (int[4]){6, 7, 8, 9});
    CHECK(res, CCC_OK);
    CHECK(check_order(&q, 6, (int[]){6, 7, 8, 9, 0, 1}), PASS);
    CHECK(pop_back_n(&q, 2), PASS);
    res = fdeq_push_front_range(&q, 6, (int[6]){10, 11, 12, 13, 14, 15});
    CHECK(res, CCC_OK);
    CHECK(check_order(&q, 10, (int[10]){10, 11, 12, 13, 14, 15, 6, 7, 8, 9}),
          PASS);
    res = fdeq_push_front_range(&q, 4, (int[4]){16, 17, 18, 19});
    CHECK(res, CCC_OK);
    CHECK(
        check_order(&q, 10, (int[10]){16, 17, 18, 19, 10, 11, 12, 13, 14, 15}),
        PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fdeq_test_push_pop_back_ranges)
{
    flat_double_ended_queue q = fdeq_init((int[10]){}, NULL, NULL, 10);
    CHECK(create_queue(&q, 6, (int[6]){0, 1, 2, 3, 4, 5}), PASS);
    CHECK(pop_front_n(&q, 4), PASS);
    ccc_result res = fdeq_push_back_range(&q, 4, (int[4]){6, 7, 8, 9});
    CHECK(res, CCC_OK);
    CHECK(check_order(&q, 6, (int[6]){4, 5, 6, 7, 8, 9}), PASS);
    CHECK(pop_front_n(&q, 2), PASS);
    res = fdeq_push_back_range(&q, 6, (int[6]){10, 11, 12, 13, 14, 15});
    CHECK(res, CCC_OK);
    CHECK(check_order(&q, 10, (int[10]){6, 7, 8, 9, 10, 11, 12, 13, 14, 15}),
          PASS);
    res = fdeq_push_back_range(&q, 4, (int[4]){16, 17, 18, 19});
    CHECK(res, CCC_OK);
    CHECK(
        check_order(&q, 10, (int[10]){10, 11, 12, 13, 14, 15, 16, 17, 18, 19}),
        PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fdeq_test_push_pop_middle_ranges)
{
    flat_double_ended_queue q = fdeq_init((int[10]){}, NULL, NULL, 10);
    CHECK(create_queue(&q, 6, (int[6]){0, 1, 2, 3, 4, 5}), PASS);
    CHECK(pop_front_n(&q, 3), PASS);
    int *ins = fdeq_insert_range(&q, fdeq_at(&q, 1), 4, (int[4]){6, 7, 8, 9});
    CHECK(ins == NULL, false);
    CHECK(*ins, 6);
    CHECK(check_order(&q, 7, (int[7]){3, 6, 7, 8, 9, 4, 5}), PASS);
    ins = fdeq_insert_range(&q, fdeq_at(&q, 5), 3, (int[3]){10, 11, 12});
    CHECK(ins == NULL, false);
    CHECK(*ins, 10);
    CHECK(check_order(&q, 10, (int[10]){3, 6, 7, 8, 9, 10, 11, 12, 4, 5}),
          PASS);
    ins = fdeq_insert_range(&q, fdeq_at(&q, 8), 3, (int[3]){13, 14, 15});
    CHECK(ins == NULL, false);
    CHECK(*ins, 13);
    CHECK(check_order(&q, 10, (int[10]){8, 9, 10, 11, 12, 13, 14, 15, 4, 5}),
          PASS);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(
        fdeq_test_push_pop_back_three(), fdeq_test_push_pop_front_three(),
        fdeq_test_push_pop_front_back(), fdeq_test_push_pop_front_ranges(),
        fdeq_test_push_pop_back_ranges(), fdeq_test_push_pop_middle_ranges());
}
