#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_double_ended_queue.h"
#include "flat_double_ended_queue_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

check_static_begin(flat_double_ended_queue_test_insert_three)
{
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize((int[3]){}, int, NULL, NULL, 3);
    check(create_queue(&q, 3, (int[3]){0, 1, 2}), CHECK_PASS);
    check(count(&q).count, 3);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_insert_overwrite)
{
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize((int[2]){}, int, NULL, NULL, 2);
    (void)CCC_flat_double_ended_queue_push_back(&q, &(int){3});
    check(*(int *)CCC_flat_double_ended_queue_back(&q), 3);
    (void)CCC_flat_double_ended_queue_push_front(&q, &(int){2});
    check(*(int *)CCC_flat_double_ended_queue_front(&q), 2);
    check(*(int *)CCC_flat_double_ended_queue_back(&q), 3);
    (void)CCC_flat_double_ended_queue_push_back(&q, &(int){1});
    check(*(int *)CCC_flat_double_ended_queue_back(&q), 1);
    check(*(int *)CCC_flat_double_ended_queue_front(&q), 3);
    (void)CCC_flat_double_ended_queue_pop_back(&q);
    int *i = CCC_flat_double_ended_queue_back(&q);
    check(*i, 3);
    i = CCC_flat_double_ended_queue_front(&q);
    check(*i, 3);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_insert_overwrite_three)
{
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize((int[3]){}, int, NULL, NULL, 3);
    check(create_queue(&q, 3, (int[3]){0, 1, 2}), CHECK_PASS);
    check(count(&q).count, 3);
    (void)flat_double_ended_queue_push_back_range(&q, 3, (int[3]){3, 4, 5});
    check(validate(&q), true);
    check(check_order(&q, 3, (int[3]){3, 4, 5}), CHECK_PASS);
    check(count(&q).count, 3);
    int *v = front(&q);
    check(v == NULL, false);
    check(*v, 3);
    v = back(&q);
    check(v == NULL, false);
    check(*v, 5);
    (void)flat_double_ended_queue_push_front_range(&q, 3, (int[3]){6, 7, 8});
    check(validate(&q), true);
    check(check_order(&q, 3, (int[3]){6, 7, 8}), CHECK_PASS);
    v = front(&q);
    check(v == NULL, false);
    check(*v, 6);
    v = back(&q);
    check(v == NULL, false);
    check(*v, 8);
    check(count(&q).count, 3);
    (void)flat_double_ended_queue_push_back_range(&q, 2, (int[2]){9, 10});
    check(validate(&q), true);
    check(check_order(&q, 3, (int[3]){8, 9, 10}), CHECK_PASS);
    v = front(&q);
    check(v == NULL, false);
    check(*v, 8);
    v = back(&q);
    check(v == NULL, false);
    check(*v, 10);
    (void)flat_double_ended_queue_push_front_range(&q, 2, (int[2]){11, 12});
    check(validate(&q), true);
    check(check_order(&q, 3, (int[3]){11, 12, 8}), CHECK_PASS);
    v = front(&q);
    check(v == NULL, false);
    check(*v, 11);
    v = back(&q);
    check(v == NULL, false);
    check(*v, 8);
    check(count(&q).count, 3);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_push_back_ranges)
{
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize((int[6]){}, int, NULL, NULL, 6);
    check(create_queue(&q, 3, (int[3]){0, 1, 2}), CHECK_PASS);
    check(check_order(&q, 3, (int[3]){0, 1, 2}), CHECK_PASS);
    (void)flat_double_ended_queue_push_back_range(&q, 2, (int[2]){3, 4});
    check(check_order(&q, 5, (int[5]){0, 1, 2, 3, 4}), CHECK_PASS);
    (void)flat_double_ended_queue_push_back_range(&q, 3, (int[3]){5, 6, 7});
    check(check_order(&q, 6, (int[6]){2, 3, 4, 5, 6, 7}), CHECK_PASS);
    (void)flat_double_ended_queue_push_back_range(&q, 4,
                                                  (int[4]){9, 10, 11, 12});
    check(check_order(&q, 6, (int[6]){6, 7, 9, 10, 11, 12}), CHECK_PASS);
    (void)flat_double_ended_queue_push_back_range(&q, 5,
                                                  (int[5]){13, 14, 15, 16, 17});
    check(check_order(&q, 6, (int[6]){12, 13, 14, 15, 16, 17}), CHECK_PASS);
    (void)flat_double_ended_queue_push_back_range(
        &q, 6, (int[6]){18, 19, 20, 21, 22, 23});
    check(check_order(&q, 6, (int[6]){18, 19, 20, 21, 22, 23}), CHECK_PASS);
    (void)flat_double_ended_queue_push_back_range(
        &q, 7, (int[7]){24, 25, 26, 27, 28, 29, 30});
    check(check_order(&q, 6, (int[6]){25, 26, 27, 28, 29, 30}), CHECK_PASS);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_push_front_ranges)
{
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize((int[6]){}, int, NULL, NULL, 6);
    check(create_queue(&q, 3, (int[3]){0, 1, 2}), CHECK_PASS);
    check(check_order(&q, 3, (int[3]){0, 1, 2}), CHECK_PASS);
    (void)flat_double_ended_queue_push_front_range(&q, 2, (int[2]){3, 4});
    check(check_order(&q, 5, (int[5]){3, 4, 0, 1, 2}), CHECK_PASS);
    (void)flat_double_ended_queue_push_front_range(&q, 3, (int[3]){5, 6, 7});
    check(check_order(&q, 6, (int[6]){5, 6, 7, 3, 4, 0}), CHECK_PASS);
    (void)flat_double_ended_queue_push_front_range(&q, 4,
                                                   (int[4]){9, 10, 11, 12});
    check(check_order(&q, 6, (int[6]){9, 10, 11, 12, 5, 6}), CHECK_PASS);
    (void)flat_double_ended_queue_push_front_range(
        &q, 5, (int[5]){13, 14, 15, 16, 17});
    check(check_order(&q, 6, (int[6]){13, 14, 15, 16, 17, 9}), CHECK_PASS);
    (void)flat_double_ended_queue_push_front_range(
        &q, 6, (int[6]){18, 19, 20, 21, 22, 23});
    check(check_order(&q, 6, (int[6]){18, 19, 20, 21, 22, 23}), CHECK_PASS);
    (void)flat_double_ended_queue_push_front_range(
        &q, 7, (int[7]){24, 25, 26, 27, 28, 29, 30});
    check(check_order(&q, 6, (int[6]){25, 26, 27, 28, 29, 30}), CHECK_PASS);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_insert_ranges)
{
    Flat_double_ended_queue q = flat_double_ended_queue_initialize(
        ((int[6]){0, 1, 2}), int, NULL, NULL, 6, 3);
    check(check_order(&q, 3, (int[3]){0, 1, 2}), CHECK_PASS);
    (void)flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 1), 2, (int[2]){3, 4});
    check(check_order(&q, 5, (int[5]){0, 3, 4, 1, 2}), CHECK_PASS);
    (void)flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 1), 3, (int[3]){5, 6, 7});
    check(check_order(&q, 6, (int[6]){5, 6, 7, 3, 4, 1}), CHECK_PASS);
    (void)flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 2), 4, (int[4]){8, 9, 10, 11});
    check(check_order(&q, 6, (int[6]){8, 9, 10, 11, 7, 3}), CHECK_PASS);
    (void)flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 3), 5, (int[5]){12, 13, 14, 15, 16});
    check(check_order(&q, 6, (int[6]){12, 13, 14, 15, 16, 11}), CHECK_PASS);
    (void)flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 3), 6,
        (int[6]){17, 18, 19, 20, 21, 22});
    check(check_order(&q, 6, (int[6]){17, 18, 19, 20, 21, 22}), CHECK_PASS);
    (void)flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 3), 7,
        (int[7]){23, 24, 25, 26, 27, 28, 29});
    check(check_order(&q, 6, (int[6]){24, 25, 26, 27, 28, 29}), CHECK_PASS);
    check_end();
}

check_static_begin(flat_double_ended_queue_test_insert_ranges_reserve)
{
    Flat_double_ended_queue q
        = flat_double_ended_queue_initialize(NULL, int, NULL, NULL, 0);
    CCC_Result const r = flat_double_ended_queue_reserve(&q, 6, std_allocate);
    check(r, CCC_RESULT_OK);
    (void)flat_double_ended_queue_push_back_range(&q, 3, (int[3]){0, 1, 2});
    check(check_order(&q, 3, (int[3]){0, 1, 2}), CHECK_PASS);
    (void)flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 1), 2, (int[2]){3, 4});
    check(check_order(&q, 5, (int[5]){0, 3, 4, 1, 2}), CHECK_PASS);
    (void)flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 1), 3, (int[3]){5, 6, 7});
    check(check_order(&q, 6, (int[6]){5, 6, 7, 3, 4, 1}), CHECK_PASS);
    (void)flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 2), 4, (int[4]){8, 9, 10, 11});
    check(check_order(&q, 6, (int[6]){8, 9, 10, 11, 7, 3}), CHECK_PASS);
    (void)flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 3), 5, (int[5]){12, 13, 14, 15, 16});
    check(check_order(&q, 6, (int[6]){12, 13, 14, 15, 16, 11}), CHECK_PASS);
    (void)flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 3), 6,
        (int[6]){17, 18, 19, 20, 21, 22});
    check(check_order(&q, 6, (int[6]){17, 18, 19, 20, 21, 22}), CHECK_PASS);
    (void)flat_double_ended_queue_insert_range(
        &q, flat_double_ended_queue_at(&q, 3), 7,
        (int[7]){23, 24, 25, 26, 27, 28, 29});
    check(check_order(&q, 6, (int[6]){24, 25, 26, 27, 28, 29}), CHECK_PASS);
    check_end(clear_and_free_reserve(&q, NULL, std_allocate););
}

int
main()
{
    return check_run(flat_double_ended_queue_test_insert_three(),
                     flat_double_ended_queue_test_insert_overwrite_three(),
                     flat_double_ended_queue_test_push_back_ranges(),
                     flat_double_ended_queue_test_push_front_ranges(),
                     flat_double_ended_queue_test_insert_ranges(),
                     flat_double_ended_queue_test_insert_overwrite(),
                     flat_double_ended_queue_test_insert_ranges_reserve());
}
