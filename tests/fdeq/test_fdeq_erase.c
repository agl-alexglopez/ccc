#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "checkers.h"
#include "fdeq_util.h"
#include "flat_double_ended_queue.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

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
    flat_double_ended_queue q = fdeq_init((int[3]){}, int, NULL, NULL, 3);
    CHECK(create_queue(&q, 3, (int[3]){0, 1, 2}), PASS);
    while (!is_empty(&q))
    {
        (void)pop_back(&q);
        CHECK(validate(&q), true);
    }
    CHECK(is_empty(&q), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fdeq_test_push_pop_front_and_back_singles)
{
    /* Avoids Variable Length Arrays but nobody else needs this constant. */
    enum : size_t
    {
        SM_FIXED_Q = 64,
    };
    flat_double_ended_queue q
        = fdeq_init((int[SM_FIXED_Q]){}, int, NULL, NULL, SM_FIXED_Q);
    /* Move the front pointer back a bit so that pushing to both sides wraps. */
    (void)CCC_fdeq_push_back_range(
        &q, 20,
        (int[20]){7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7});
    while (!CCC_fdeq_is_empty(&q))
    {
        CHECK(*((int *)CCC_fdeq_front(&q)), 7);
        CHECK(CCC_fdeq_pop_front(&q), CCC_RESULT_OK);
    }
    for (size_t i = 0; CCC_fdeq_count(&q).count != SM_FIXED_Q; ++i)
    {
        if (i % 2)
        {
            CHECK(CCC_fdeq_push_front(&q, &(int){1}) != NULL, true);
        }
        else
        {
            CHECK(CCC_fdeq_push_back(&q, &(int){0}) != NULL, true);
        }
    }
    size_t i = 0;
    for (; !CCC_fdeq_is_empty(&q); ++i)
    {
        if (i % 2)
        {
            int const elem = *((int *)CCC_fdeq_front(&q));
            CHECK(CCC_fdeq_pop_front(&q), CCC_RESULT_OK);
            CHECK(elem, 1);
        }
        else
        {
            int const elem = *((int *)CCC_fdeq_back(&q));
            CHECK(CCC_fdeq_pop_back(&q), CCC_RESULT_OK);
            CHECK(elem, 0);
        }
    }
    CHECK(i, SM_FIXED_Q);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fdeq_test_push_pop_front_and_back_singles_dynamic)
{
    size_t const sm_dyn_q = 128;
    flat_double_ended_queue q = fdeq_init(NULL, int, std_alloc, NULL, 0);
    /* Move the front pointer back a bit so that pushing to both sides wraps. */
    (void)CCC_fdeq_push_back_range(
        &q, 20,
        (int[20]){7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7});
    while (!CCC_fdeq_is_empty(&q))
    {
        CHECK(*((int *)CCC_fdeq_front(&q)), 7);
        CHECK(CCC_fdeq_pop_front(&q), CCC_RESULT_OK);
    }
    for (size_t i = 0; CCC_fdeq_count(&q).count != sm_dyn_q; ++i)
    {
        if (i % 2)
        {
            CHECK(CCC_fdeq_push_front(&q, &(int){1}) != NULL, true);
        }
        else
        {
            CHECK(CCC_fdeq_push_back(&q, &(int){0}) != NULL, true);
        }
    }
    size_t i = 0;
    for (; !CCC_fdeq_is_empty(&q); ++i)
    {
        if (i % 2)
        {
            int const elem = *((int *)CCC_fdeq_front(&q));
            CHECK(CCC_fdeq_pop_front(&q), CCC_RESULT_OK);
            CHECK(elem, 1);
        }
        else
        {
            int const elem = *((int *)CCC_fdeq_back(&q));
            CHECK(CCC_fdeq_pop_back(&q), CCC_RESULT_OK);
            CHECK(elem, 0);
        }
    }
    CHECK(i, sm_dyn_q);
    CHECK_END_FN(CCC_fdeq_clear_and_free(&q, NULL););
}

CHECK_BEGIN_STATIC_FN(fdeq_test_push_pop_front_three)
{
    flat_double_ended_queue q = fdeq_init((int[3]){}, int, NULL, NULL, 3);
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
    flat_double_ended_queue q = fdeq_init((int[6]){}, int, NULL, NULL, 6);
    CHECK(create_queue(&q, 6, (int[6]){0, 1, 2, 3, 4, 5}), PASS);
    while (!is_empty(&q))
    {
        if (count(&q).count % 2)
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
    flat_double_ended_queue q = fdeq_init((int[10]){}, int, NULL, NULL, 10);
    CHECK(create_queue(&q, 6, (int[6]){0, 1, 2, 3, 4, 5}), PASS);
    CHECK(pop_back_n(&q, 4), PASS);
    CCC_result res = fdeq_push_front_range(&q, 4, (int[4]){6, 7, 8, 9});
    CHECK(res, CCC_RESULT_OK);
    CHECK(check_order(&q, 6, (int[]){6, 7, 8, 9, 0, 1}), PASS);
    CHECK(pop_back_n(&q, 2), PASS);
    res = fdeq_push_front_range(&q, 6, (int[6]){10, 11, 12, 13, 14, 15});
    CHECK(res, CCC_RESULT_OK);
    CHECK(check_order(&q, 10, (int[10]){10, 11, 12, 13, 14, 15, 6, 7, 8, 9}),
          PASS);
    res = fdeq_push_front_range(&q, 4, (int[4]){16, 17, 18, 19});
    CHECK(res, CCC_RESULT_OK);
    CHECK(
        check_order(&q, 10, (int[10]){16, 17, 18, 19, 10, 11, 12, 13, 14, 15}),
        PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fdeq_test_push_pop_back_ranges)
{
    flat_double_ended_queue q = fdeq_init((int[10]){}, int, NULL, NULL, 10);
    CHECK(create_queue(&q, 6, (int[6]){0, 1, 2, 3, 4, 5}), PASS);
    CHECK(pop_front_n(&q, 4), PASS);
    CCC_result res = fdeq_push_back_range(&q, 4, (int[4]){6, 7, 8, 9});
    CHECK(res, CCC_RESULT_OK);
    CHECK(check_order(&q, 6, (int[6]){4, 5, 6, 7, 8, 9}), PASS);
    CHECK(pop_front_n(&q, 2), PASS);
    res = fdeq_push_back_range(&q, 6, (int[6]){10, 11, 12, 13, 14, 15});
    CHECK(res, CCC_RESULT_OK);
    CHECK(check_order(&q, 10, (int[10]){6, 7, 8, 9, 10, 11, 12, 13, 14, 15}),
          PASS);
    res = fdeq_push_back_range(&q, 4, (int[4]){16, 17, 18, 19});
    CHECK(res, CCC_RESULT_OK);
    CHECK(
        check_order(&q, 10, (int[10]){10, 11, 12, 13, 14, 15, 16, 17, 18, 19}),
        PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fdeq_test_push_pop_middle_ranges)
{
    flat_double_ended_queue q = fdeq_init((int[10]){}, int, NULL, NULL, 10);
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
        fdeq_test_push_pop_front_and_back_singles(),
        fdeq_test_push_pop_front_and_back_singles_dynamic(),
        fdeq_test_push_pop_front_back(), fdeq_test_push_pop_front_ranges(),
        fdeq_test_push_pop_back_ranges(), fdeq_test_push_pop_middle_ranges());
}
