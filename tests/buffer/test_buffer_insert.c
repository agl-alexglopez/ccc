#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "buffer_utility.h"
#include "ccc/buffer.h"
#include "ccc/types.h"
#include "checkers.h"
#include "utility/allocate.h"
#include "utility/random.h"

static int
std_order_ints(void const *const lhs, void const *const rhs)
{
    int const lhs_int = *(int *)lhs;
    int const rhs_int = *(int *)rhs;
    return (lhs_int > rhs_int) - (lhs_int < rhs_int);
}

static CCC_Order
ccc_order_ints(CCC_Type_comparator_context const order)
{
    int const lhs_int = *(int *)order.type_lhs;
    int const rhs_int = *(int *)order.type_rhs;
    return (lhs_int > rhs_int) - (lhs_int < rhs_int);
}

check_static_begin(buffer_test_push_fixed)
{
    Buffer b = buffer_initialize((int[8]){}, int, NULL, NULL, 8);
    int const push[8] = {7, 6, 5, 4, 3, 2, 1, 0};
    for (size_t i = 0; i < sizeof(push) / sizeof(*push); ++i)
    {
        int *p = buffer_push_back(&b, &push[i]);
        check(p != NULL, CCC_TRUE);
        check(*p, push[i]);
    }
    check(buffer_count(&b).count, sizeof(push) / sizeof(*push));
    check(buffer_push_back(&b, &(int){99}) == NULL, CCC_TRUE);
    check_end();
}

check_static_begin(buffer_test_push_resize)
{
    Buffer b = buffer_initialize(NULL, int, std_allocate, NULL, 0);
    size_t const cap = 32;
    int *const many = malloc(sizeof(int) * cap);
    iota(many, cap, 0);
    check(many != NULL, CCC_TRUE);
    for (size_t i = 0; i < cap; ++i)
    {
        int *p = buffer_push_back(&b, &many[i]);
        check(p != NULL, CCC_TRUE);
        check(*p, many[i]);
    }
    check(buffer_count(&b).count, cap);
    check(buffer_capacity(&b).count >= cap, CCC_TRUE);
    check_end({
        (void)buffer_clear_and_free(&b, NULL);
        free(many);
    });
}

check_static_begin(buffer_test_push_qsort)
{
    enum : size_t
    {
        BUF_SORT_CAP = 32,
    };
    Buffer b = buffer_initialize((int[BUF_SORT_CAP]){}, int, NULL, NULL,
                                 BUF_SORT_CAP, BUF_SORT_CAP);
    int ref[BUF_SORT_CAP] = {};
    iota(ref, BUF_SORT_CAP, 0);
    iota(buffer_begin(&b), BUF_SORT_CAP, 0);
    check(memcmp(ref, buffer_begin(&b), BUF_SORT_CAP * sizeof(*ref)),
          CCC_ORDER_EQUAL);
    rand_shuffle(sizeof(*ref), ref, BUF_SORT_CAP, &(int){0});
    rand_shuffle(buffer_sizeof_type(&b).count, buffer_begin(&b),
                 buffer_count(&b).count, &(int){0});
    qsort(ref, BUF_SORT_CAP, sizeof(*ref), std_order_ints);
    qsort(buffer_begin(&b), buffer_capacity(&b).count,
          buffer_sizeof_type(&b).count, std_order_ints);
    check(memcmp(ref, buffer_begin(&b), BUF_SORT_CAP * sizeof(*ref)),
          CCC_ORDER_EQUAL);
    int prev = INT_MIN;
    size_t count = 0;
    for (int const *i = buffer_begin(&b); i != buffer_end(&b);
         i = buffer_next(&b, i))
    {
        check(i != NULL, CCC_TRUE);
        check(*i >= prev, CCC_TRUE);
        prev = *i;
        ++count;
    }
    check(count, BUF_SORT_CAP);
    check_end();
}

check_static_begin(buffer_test_push_sort)
{
    enum : size_t
    {
        BUF_SORT_CAP = 32,
    };
    Buffer b = buffer_initialize((int[BUF_SORT_CAP]){}, int, NULL, NULL,
                                 BUF_SORT_CAP, BUF_SORT_CAP);
    iota(buffer_begin(&b), BUF_SORT_CAP, 0);
    rand_shuffle(buffer_sizeof_type(&b).count, buffer_begin(&b),
                 buffer_count(&b).count, &(int){0});
    (void)sort(&b, ccc_order_ints, &(int){0});
    int prev = INT_MIN;
    size_t count = 0;
    for (int const *i = buffer_begin(&b); i != buffer_end(&b);
         i = buffer_next(&b, i))
    {
        check(i != NULL, CCC_TRUE);
        check(*i >= prev, CCC_TRUE);
        prev = *i;
        ++count;
    }
    check(count, BUF_SORT_CAP);
    check_end();
}

check_static_begin(buffer_test_insert_no_allocate)
{
    enum : size_t
    {
        BUFINSCAP = 8,
    };
    Buffer b = buffer_initialize(((int[BUFINSCAP]){1, 2, 4, 5}), int, NULL,
                                 NULL, BUFINSCAP, BUFINSCAP - 3);
    check(buffer_count(&b).count, BUFINSCAP - 3);
    int const *const three = buffer_insert(&b, 2, &(int){3});
    check(three != NULL, CCC_TRUE);
    check(*three, 3);
    CCC_Order order
        = buforder(&b, BUFINSCAP - 2, (int[BUFINSCAP - 2]){1, 2, 3, 4, 5});
    check(order, CCC_ORDER_EQUAL);
    check(buffer_count(&b).count, BUFINSCAP - 2);
    int const *const zero = buffer_insert(&b, 0, &(int){0});
    check(zero != NULL, CCC_TRUE);
    check(*zero, 0);
    order = buforder(&b, BUFINSCAP - 1, (int[BUFINSCAP - 1]){0, 1, 2, 3, 4, 5});
    check(order, CCC_ORDER_EQUAL);
    check(buffer_count(&b).count, BUFINSCAP - 1);
    int const *const six = buffer_insert(&b, 6, &(int){6});
    check(six != NULL, CCC_TRUE);
    check(*six, 6);
    order = buforder(&b, BUFINSCAP, (int[BUFINSCAP]){0, 1, 2, 3, 4, 5, 6});
    check(order, CCC_ORDER_EQUAL);
    check(buffer_count(&b).count, BUFINSCAP);
    check_end();
}

check_static_begin(buffer_test_insert_no_allocate_fail)
{
    enum : size_t
    {
        BUFINSCAP = 8,
    };
    Buffer b = buffer_initialize(((int[BUFINSCAP]){0, 1, 2, 3, 4, 5, 6}), int,
                                 NULL, NULL, BUFINSCAP, BUFINSCAP);
    check(buffer_count(&b).count, BUFINSCAP);
    int const *const three = buffer_insert(&b, 3, &(int){3});
    check(three == NULL, CCC_TRUE);
    check(buffer_count(&b).count, BUFINSCAP);
    check_end();
}

/* Force a resize when inserting in middle forces shuffle down. */
check_static_begin(buffer_test_insert_allocate)
{
    Buffer b = buffer_initialize(NULL, int, std_allocate, NULL, 0);
    CCC_Result r = buffer_reserve(&b, 6, std_allocate);
    check(r, CCC_RESULT_OK);
    r = append_range(&b, 6, (int[6]){1, 2, 4, 5, 6, 7});
    check(r, CCC_RESULT_OK);
    check(buffer_count(&b).count, 6);
    int const *const three = buffer_insert(&b, 2, &(int){3});
    check(three != NULL, CCC_TRUE);
    check(*three, 3);
    CCC_Order order = buforder(&b, 7, (int[7]){1, 2, 3, 4, 5, 6, 7});
    check(order, CCC_ORDER_EQUAL);
    check(buffer_count(&b).count, 7);
    int const *const zero = buffer_insert(&b, 0, &(int){0});
    check(zero != NULL, CCC_TRUE);
    check(*zero, 0);
    order = buforder(&b, 8, (int[8]){0, 1, 2, 3, 4, 5, 6, 7});
    check(order, CCC_ORDER_EQUAL);
    check(buffer_count(&b).count, 8);
    int const *const eight = buffer_insert(&b, 8, &(int){8});
    check(eight != NULL, CCC_TRUE);
    check(*eight, 8);
    order = buforder(&b, 9, (int[9]){0, 1, 2, 3, 4, 5, 6, 7, 8});
    check(order, CCC_ORDER_EQUAL);
    check(buffer_count(&b).count, 9);
    check_end(buffer_clear_and_free(&b, NULL););
}

int
main(void)
{
    /* NOLINTNEXTLINE */
    srand(time(NULL));
    return check_run(buffer_test_push_fixed(), buffer_test_push_resize(),
                     buffer_test_push_qsort(), buffer_test_push_sort(),
                     buffer_test_insert_no_allocate(),
                     buffer_test_insert_no_allocate_fail(),
                     buffer_test_insert_allocate());
}
