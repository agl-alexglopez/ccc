#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_double_ended_queue.h"
#include "flat_double_ended_queue_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

check_static_begin(pop_front_n, Flat_double_ended_queue *const q, size_t n)
{
    for (; n-- && !is_empty(q); (void)pop_front(q))
    {
        check(validate(q), true);
    }
    check_end();
}

check_static_begin(pop_back_n, Flat_double_ended_queue *const q, size_t n)
{
    for (; n-- && !is_empty(q); (void)pop_back(q))
    {
        check(validate(q), true);
    }
    check_end();
}

check_static_begin(flat_double_ended_queue_test_push_pop_back_three)
{
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize((int[3]){}, int, NULL, NULL, 3);
    check(create_queue(&q, 3, (int[3]){0, 1, 2}), CHECK_PASS);
    while (!is_empty(&q))
    {
        (void)pop_back(&q);
        check(validate(&q), true);
    }
    check(is_empty(&q), true);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_push_pop_front_and_back_singles)
{
    /* Avoids Variable Length Arrays but nobody else needs this constant. */
    enum : size_t
    {
        SM_FIXED_Q = 64,
    };
    Flat_double_ended_queue q = flat_double_ended_queue_initialize(
        (int[SM_FIXED_Q]){}, int, NULL, NULL, SM_FIXED_Q);
    /* Move the front pointer back a bit so that pushing to both sides wraps. */
    (void)CCC_flat_double_ended_queue_push_back_range(
        &q, 20,
        (int[20]){7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7});
    while (!CCC_flat_double_ended_queue_is_empty(&q))
    {
        check(*((int *)CCC_flat_double_ended_queue_front(&q)), 7);
        check(CCC_flat_double_ended_queue_pop_front(&q), CCC_RESULT_OK);
    }
    for (size_t i = 0;
         CCC_flat_double_ended_queue_count(&q).count != SM_FIXED_Q; ++i)
    {
        if (i % 2)
        {
            check(CCC_flat_double_ended_queue_push_front(&q, &(int){1}) != NULL,
                  true);
        }
        else
        {
            check(CCC_flat_double_ended_queue_push_back(&q, &(int){0}) != NULL,
                  true);
        }
    }
    size_t i = 0;
    for (; !CCC_flat_double_ended_queue_is_empty(&q); ++i)
    {
        if (i % 2)
        {
            int const elem = *((int *)CCC_flat_double_ended_queue_front(&q));
            check(CCC_flat_double_ended_queue_pop_front(&q), CCC_RESULT_OK);
            check(elem, 1);
        }
        else
        {
            int const elem = *((int *)CCC_flat_double_ended_queue_back(&q));
            check(CCC_flat_double_ended_queue_pop_back(&q), CCC_RESULT_OK);
            check(elem, 0);
        }
    }
    check(i, SM_FIXED_Q);
    check_end();
}

check_static_begin(
    flat_double_ended_queue_test_push_pop_front_and_back_singles_dynamic)
{
    size_t const sm_dyn_q = 128;
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize(NULL, int, std_allocate, NULL, 0);
    /* Move the front pointer back a bit so that pushing to both sides wraps. */
    (void)CCC_flat_double_ended_queue_push_back_range(
        &q, 20,
        (int[20]){7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7});
    while (!CCC_flat_double_ended_queue_is_empty(&q))
    {
        check(*((int *)CCC_flat_double_ended_queue_front(&q)), 7);
        check(CCC_flat_double_ended_queue_pop_front(&q), CCC_RESULT_OK);
    }
    for (size_t i = 0; CCC_flat_double_ended_queue_count(&q).count != sm_dyn_q;
         ++i)
    {
        if (i % 2)
        {
            check(CCC_flat_double_ended_queue_push_front(&q, &(int){1}) != NULL,
                  true);
        }
        else
        {
            check(CCC_flat_double_ended_queue_push_back(&q, &(int){0}) != NULL,
                  true);
        }
    }
    size_t i = 0;
    for (; !CCC_flat_double_ended_queue_is_empty(&q); ++i)
    {
        if (i % 2)
        {
            int const elem = *((int *)CCC_flat_double_ended_queue_front(&q));
            check(CCC_flat_double_ended_queue_pop_front(&q), CCC_RESULT_OK);
            check(elem, 1);
        }
        else
        {
            int const elem = *((int *)CCC_flat_double_ended_queue_back(&q));
            check(CCC_flat_double_ended_queue_pop_back(&q), CCC_RESULT_OK);
            check(elem, 0);
        }
    }
    check(i, sm_dyn_q);
    check_end(CCC_flat_double_ended_queue_clear_and_free(&q, NULL););
}

check_static_begin(flat_double_ended_queue_test_push_pop_front_three)
{
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize((int[3]){}, int, NULL, NULL, 3);
    check(create_queue(&q, 3, (int[3]){0, 1, 2}), CHECK_PASS);
    while (!is_empty(&q))
    {
        (void)pop_front(&q);
        check(validate(&q), true);
    }
    check(is_empty(&q), true);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_push_pop_front_back)
{
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize((int[6]){}, int, NULL, NULL, 6);
    check(create_queue(&q, 6, (int[6]){0, 1, 2, 3, 4, 5}), CHECK_PASS);
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
        check(validate(&q), true);
    }
    check(is_empty(&q), true);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_push_pop_front_ranges)
{
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize((int[10]){}, int, NULL, NULL, 10);
    check(create_queue(&q, 6, (int[6]){0, 1, 2, 3, 4, 5}), CHECK_PASS);
    check(pop_back_n(&q, 4), CHECK_PASS);
    CCC_Result res
        = flat_double_ended_queue_push_front_range(&q, 4, (int[4]){6, 7, 8, 9});
    check(res, CCC_RESULT_OK);
    check(check_order(&q, 6, (int[]){6, 7, 8, 9, 0, 1}), CHECK_PASS);
    check(pop_back_n(&q, 2), CHECK_PASS);
    res = flat_double_ended_queue_push_front_range(
        &q, 6, (int[6]){10, 11, 12, 13, 14, 15});
    check(res, CCC_RESULT_OK);
    check(check_order(&q, 10, (int[10]){10, 11, 12, 13, 14, 15, 6, 7, 8, 9}),
          CHECK_PASS);
    res = flat_double_ended_queue_push_front_range(&q, 4,
                                                   (int[4]){16, 17, 18, 19});
    check(res, CCC_RESULT_OK);
    check(
        check_order(&q, 10, (int[10]){16, 17, 18, 19, 10, 11, 12, 13, 14, 15}),
        CHECK_PASS);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_push_pop_back_ranges)
{
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize((int[10]){}, int, NULL, NULL, 10);
    check(create_queue(&q, 6, (int[6]){0, 1, 2, 3, 4, 5}), CHECK_PASS);
    check(pop_front_n(&q, 4), CHECK_PASS);
    CCC_Result res
        = flat_double_ended_queue_push_back_range(&q, 4, (int[4]){6, 7, 8, 9});
    check(res, CCC_RESULT_OK);
    check(check_order(&q, 6, (int[6]){4, 5, 6, 7, 8, 9}), CHECK_PASS);
    check(pop_front_n(&q, 2), CHECK_PASS);
    res = flat_double_ended_queue_push_back_range(
        &q, 6, (int[6]){10, 11, 12, 13, 14, 15});
    check(res, CCC_RESULT_OK);
    check(check_order(&q, 10, (int[10]){6, 7, 8, 9, 10, 11, 12, 13, 14, 15}),
          CHECK_PASS);
    res = flat_double_ended_queue_push_back_range(&q, 4,
                                                  (int[4]){16, 17, 18, 19});
    check(res, CCC_RESULT_OK);
    check(
        check_order(&q, 10, (int[10]){10, 11, 12, 13, 14, 15, 16, 17, 18, 19}),
        CHECK_PASS);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_push_pop_middle_ranges)
{
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize((int[10]){}, int, NULL, NULL, 10);
    check(create_queue(&q, 6, (int[6]){0, 1, 2, 3, 4, 5}), CHECK_PASS);
    check(pop_front_n(&q, 3), CHECK_PASS);
    int *ins = flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 1), 4, (int[4]){6, 7, 8, 9});
    check(ins == NULL, false);
    check(*ins, 6);
    check(check_order(&q, 7, (int[7]){3, 6, 7, 8, 9, 4, 5}), CHECK_PASS);
    ins = flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 5), 3, (int[3]){10, 11, 12});
    check(ins == NULL, false);
    check(*ins, 10);
    check(check_order(&q, 10, (int[10]){3, 6, 7, 8, 9, 10, 11, 12, 4, 5}),
          CHECK_PASS);
    ins = flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 8), 3, (int[3]){13, 14, 15});
    check(ins == NULL, false);
    check(*ins, 13);
    check(check_order(&q, 10, (int[10]){8, 9, 10, 11, 12, 13, 14, 15, 4, 5}),
          CHECK_PASS);
    check_end();
}

int
main()
{
    return check_run(
        flat_double_ended_queue_test_push_pop_back_three(),
        flat_double_ended_queue_test_push_pop_front_three(),
        flat_double_ended_queue_test_push_pop_front_and_back_singles(),
        flat_double_ended_queue_test_push_pop_front_and_back_singles_dynamic(),
        flat_double_ended_queue_test_push_pop_front_back(),
        flat_double_ended_queue_test_push_pop_front_ranges(),
        flat_double_ended_queue_test_push_pop_back_ranges(),
        flat_double_ended_queue_test_push_pop_middle_ranges());
}
