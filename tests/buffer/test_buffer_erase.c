#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "buffer_utility.h"
#include "ccc/buffer.h"
#include "ccc/types.h"
#include "checkers.h"
#include "utility/allocate.h"
#include "utility/random.h"

check_static_begin(buffer_test_push_pop_fixed)
{
    Buffer b = buffer_initialize((int[8]){}, int, NULL, NULL, 8);
    int const push[8] = {7, 6, 5, 4, 3, 2, 1, 0};
    size_t count = 0;
    for (size_t i = 0; i < sizeof(push) / sizeof(*push); ++i)
    {
        int const *const p = buffer_push_back(&b, &push[i]);
        check(p != NULL, CCC_TRUE);
        check(*p, push[i]);
        ++count;
    }
    check(buffer_count(&b).count, sizeof(push) / sizeof(*push));
    check(buffer_count(&b).count, count);
    check(buffer_push_back(&b, &(int){99}) == NULL, CCC_TRUE);
    while (!buffer_is_empty(&b))
    {
        int const v = *(int *)buffer_back(&b);
        check(buffer_pop_back(&b), CCC_RESULT_OK);
        --count;
        check(v, push[count]);
    }
    check(buffer_count(&b).count, count);
    check(count, 0);
    check_end();
}

check_static_begin(buffer_test_push_resize_pop)
{
    Buffer b = buffer_initialize(NULL, int, std_allocate, NULL, 0);
    size_t const cap = 32;
    int *const many = malloc(sizeof(int) * cap);
    iota(many, cap, 0);
    check(many != NULL, CCC_TRUE);
    size_t count = 0;
    for (size_t i = 0; i < cap; ++i)
    {
        int *p = buffer_push_back(&b, &many[i]);
        check(p != NULL, CCC_TRUE);
        check(*p, many[i]);
        ++count;
    }
    check(count, cap);
    check(buffer_count(&b).count, cap);
    check(buffer_capacity(&b).count >= cap, CCC_TRUE);
    while (!buffer_is_empty(&b))
    {
        int const v = *buffer_back_as(&b, int);
        check(buffer_pop_back(&b), CCC_RESULT_OK);
        --count;
        check(v, many[count]);
    }
    check(buffer_count(&b).count, count);
    check(count, 0);
    check_end({
        (void)buffer_clear_and_free(&b, NULL);
        free(many);
    });
}

check_static_begin(buffer_test_daily_temperatures)
{
    enum : size_t
    {
        TMPCAP = 8,
    };
    Buffer const temps
        = buffer_initialize(((int[TMPCAP]){73, 74, 75, 71, 69, 72, 76, 73}),
                            int, NULL, NULL, TMPCAP, TMPCAP);
    Buffer const correct
        = buffer_initialize(((int[TMPCAP]){1, 1, 4, 2, 1, 1, 0, 0}), int, NULL,
                            NULL, TMPCAP, TMPCAP);
    Buffer res
        = buffer_initialize((int[TMPCAP]){}, int, NULL, NULL, TMPCAP, TMPCAP);
    Buffer idx_stack
        = buffer_initialize((int[TMPCAP]){}, int, NULL, NULL, TMPCAP);
    for (int i = 0, end = (int)buffer_count(&temps).count; i < end; ++i)
    {
        while (!buffer_is_empty(&idx_stack)
               && *buffer_as(&temps, int, i) > *buffer_as(
                      &temps, int, *buffer_back_as(&idx_stack, int)))
        {
            int const *const ptr
                = buffer_emplace(&res, *buffer_back_as(&idx_stack, int),
                                 i - *buffer_back_as(&idx_stack, int));
            check(ptr != NULL, CCC_TRUE);
            CCC_Result const r = buffer_pop_back(&idx_stack);
            check(r, CCC_RESULT_OK);
        }
        int const *const ptr = buffer_push_back(&idx_stack, &i);
        check(ptr != NULL, CCC_TRUE);
    }
    check(memcmp(buffer_begin(&res), buffer_begin(&correct),
                 buffer_count_bytes(&correct).count),
          0);
    check_end();
}

static CCC_Order
order_car_idx(CCC_Type_comparator_context const order)
{
    Buffer const *const positions = order.context;
    int const *const lhs_pos = buffer_at(positions, *(int *)order.type_lhs);
    int const *const rhs_pos = buffer_at(positions, *(int *)order.type_rhs);
    /* Reversed sort. We want descending not ascending order. We ask how many
       car fleets there will be by starting at the cars furthest away that may
       catch up to those ahead. */
    return (*lhs_pos < *rhs_pos) - (*lhs_pos > *rhs_pos);
}

check_static_begin(buffer_test_car_fleet)
{
    enum : size_t
    {
        CARCAP = 5,
    };
    Buffer positions = buffer_initialize(((int[CARCAP]){10, 8, 0, 5, 3}), int,
                                         NULL, NULL, CARCAP, CARCAP);
    Buffer const speeds = buffer_initialize(((int[CARCAP]){2, 4, 1, 1, 3}), int,
                                            NULL, NULL, CARCAP, CARCAP);
    int const correct_fleet_count = 3;
    Buffer car_idx = buffer_initialize((int[CARCAP]){}, int, NULL, &positions,
                                       CARCAP, CARCAP);
    iota(buffer_begin(&car_idx), CARCAP, 0);
    sort(&car_idx, order_car_idx, &(int){0});
    int target = 12;
    int fleets = 1;
    double slowest_time_to_target
        = ((double)(target - *buffer_as(&positions, int, 0)))
        / *buffer_as(&speeds, int, 0);
    for (int const *iter = buffer_begin(&car_idx); iter != buffer_end(&car_idx);
         iter = buffer_next(&car_idx, iter))
    {
        double const time_of_closer_car
            = ((double)(target - *buffer_as(&positions, int, *iter)))
            / *buffer_as(&speeds, int, *iter);
        if (time_of_closer_car > slowest_time_to_target)
        {
            ++fleets;
            slowest_time_to_target = time_of_closer_car;
        }
    }
    check(fleets, correct_fleet_count);
    check_end();
}

check_static_begin(buffer_test_largest_rectangle_in_histogram)
{
    enum : size_t
    {
        HCAP = 6,
    };
    Buffer const heights = buffer_initialize(((int[HCAP]){2, 1, 5, 6, 2, 3}),
                                             int, NULL, NULL, HCAP, HCAP);
    int const correct_max_rectangle = 10;
    int max_rectangle = 0;
    Buffer bar_indices
        = buffer_initialize((int[HCAP]){}, int, NULL, NULL, HCAP);
    for (int i = 0, end = buffer_count(&heights).count; i <= end; ++i)
    {
        while (!buffer_is_empty(&bar_indices)
               && (i == end
                   || *buffer_as(&heights, int, i) < *buffer_as(
                          &heights, int, *buffer_back_as(&bar_indices, int))))
        {
            int const stack_top_i = *buffer_back_as(&bar_indices, int);
            int const stack_top_height = *buffer_as(&heights, int, stack_top_i);
            CCC_Result const r = buffer_pop_back(&bar_indices);
            check(r, CCC_RESULT_OK);
            int const w = buffer_is_empty(&bar_indices)
                            ? i
                            : i - *buffer_back_as(&bar_indices, int) - 1;
            max_rectangle = maxint(max_rectangle, stack_top_height * w);
        }
        int const *const ptr = buffer_push_back(&bar_indices, &i);
        check(ptr != NULL, CCC_TRUE);
    }
    check(max_rectangle, correct_max_rectangle);
    check_end();
}

check_static_begin(buffer_test_erase)
{
    enum : size_t
    {
        BECAP = 8,
    };
    Buffer b = buffer_initialize(((int[BECAP]){0, 1, 2, 3, 4, 5, 6, 7}), int,
                                 NULL, NULL, BECAP, BECAP);
    check(buffer_count(&b).count, BECAP);
    CCC_Result r = buffer_erase(&b, 4);
    check(r, CCC_RESULT_OK);
    CCC_Order order
        = buforder(&b, BECAP - 1, (int[BECAP - 1]){0, 1, 2, 3, 5, 6, 7});
    check(order, CCC_ORDER_EQUAL);
    check(buffer_count(&b).count, BECAP - 1);
    r = buffer_erase(&b, 0);
    check(r, CCC_RESULT_OK);
    order = buforder(&b, BECAP - 2, (int[BECAP - 2]){1, 2, 3, 5, 6, 7});
    check(order, CCC_ORDER_EQUAL);
    check(buffer_count(&b).count, BECAP - 2);
    r = buffer_erase(&b, BECAP - 3);
    check(r, CCC_RESULT_OK);
    order = buforder(&b, BECAP - 3, (int[BECAP - 3]){1, 2, 3, 5, 6});
    check(order, CCC_ORDER_EQUAL);
    check(buffer_count(&b).count, BECAP - 3);
    check_end();
}

int
main(void)
{
    return check_run(
        buffer_test_push_pop_fixed(), buffer_test_push_resize_pop(),
        buffer_test_daily_temperatures(), buffer_test_car_fleet(),
        buffer_test_largest_rectangle_in_histogram(), buffer_test_erase());
}
