#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "buffer_util.h"
#include "ccc/buffer.h"
#include "ccc/types.h"
#include "checkers.h"
#include "util/alloc.h"
#include "util/random.h"

static int
cmp_ints(void const *const lhs, void const *const rhs)
{
    int const lhs_int = *(int *)lhs;
    int const rhs_int = *(int *)rhs;
    return (lhs_int > rhs_int) - (lhs_int < rhs_int);
}

static CCC_Order
CCC_cmp_ints(CCC_Type_comparator_context const cmp)
{
    int const lhs_int = *(int *)cmp.any_type_lhs;
    int const rhs_int = *(int *)cmp.any_type_rhs;
    return (lhs_int > rhs_int) - (lhs_int < rhs_int);
}

CHECK_BEGIN_STATIC_FN(buffer_test_push_fixed)
{
    Buffer b = buffer_initialize((int[8]){}, int, NULL, NULL, 8);
    int const push[8] = {7, 6, 5, 4, 3, 2, 1, 0};
    for (size_t i = 0; i < sizeof(push) / sizeof(*push); ++i)
    {
        int *p = buffer_push_back(&b, &push[i]);
        CHECK(p != NULL, CCC_TRUE);
        CHECK(*p, push[i]);
    }
    CHECK(buffer_count(&b).count, sizeof(push) / sizeof(*push));
    CHECK(buffer_push_back(&b, &(int){99}) == NULL, CCC_TRUE);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buffer_test_push_resize)
{
    Buffer b = buffer_initialize(NULL, int, std_alloc, NULL, 0);
    size_t const cap = 32;
    int *const many = malloc(sizeof(int) * cap);
    iota(many, cap, 0);
    CHECK(many != NULL, CCC_TRUE);
    for (size_t i = 0; i < cap; ++i)
    {
        int *p = buffer_push_back(&b, &many[i]);
        CHECK(p != NULL, CCC_TRUE);
        CHECK(*p, many[i]);
    }
    CHECK(buffer_count(&b).count, cap);
    CHECK(buffer_capacity(&b).count >= cap, CCC_TRUE);
    CHECK_END_FN({
        (void)buffer_clear_and_free(&b, NULL);
        free(many);
    });
}

CHECK_BEGIN_STATIC_FN(buffer_test_push_qsort)
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
    CHECK(memcmp(ref, buffer_begin(&b), BUF_SORT_CAP * sizeof(*ref)),
          CCC_ORDER_EQUAL);
    rand_shuffle(sizeof(*ref), ref, BUF_SORT_CAP, &(int){0});
    rand_shuffle(buffer_sizeof_type(&b).count, buffer_begin(&b),
                 buffer_count(&b).count, &(int){0});
    qsort(ref, BUF_SORT_CAP, sizeof(*ref), cmp_ints);
    qsort(buffer_begin(&b), buffer_capacity(&b).count,
          buffer_sizeof_type(&b).count, cmp_ints);
    CHECK(memcmp(ref, buffer_begin(&b), BUF_SORT_CAP * sizeof(*ref)),
          CCC_ORDER_EQUAL);
    int prev = INT_MIN;
    size_t count = 0;
    for (int const *i = buffer_begin(&b); i != buffer_end(&b);
         i = buffer_next(&b, i))
    {
        CHECK(i != NULL, CCC_TRUE);
        CHECK(*i >= prev, CCC_TRUE);
        prev = *i;
        ++count;
    }
    CHECK(count, BUF_SORT_CAP);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buffer_test_push_sort)
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
    (void)sort(&b, CCC_cmp_ints, &(int){0});
    int prev = INT_MIN;
    size_t count = 0;
    for (int const *i = buffer_begin(&b); i != buffer_end(&b);
         i = buffer_next(&b, i))
    {
        CHECK(i != NULL, CCC_TRUE);
        CHECK(*i >= prev, CCC_TRUE);
        prev = *i;
        ++count;
    }
    CHECK(count, BUF_SORT_CAP);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buffer_test_insert_no_alloc)
{
    enum : size_t
    {
        BUFINSCAP = 8,
    };
    Buffer b = buffer_initialize(((int[BUFINSCAP]){1, 2, 4, 5}), int, NULL,
                                 NULL, BUFINSCAP, BUFINSCAP - 3);
    CHECK(buffer_count(&b).count, BUFINSCAP - 3);
    int const *const three = buffer_insert(&b, 2, &(int){3});
    CHECK(three != NULL, CCC_TRUE);
    CHECK(*three, 3);
    CCC_Order cmp
        = bufcmp(&b, BUFINSCAP - 2, (int[BUFINSCAP - 2]){1, 2, 3, 4, 5});
    CHECK(cmp, CCC_ORDER_EQUAL);
    CHECK(buffer_count(&b).count, BUFINSCAP - 2);
    int const *const zero = buffer_insert(&b, 0, &(int){0});
    CHECK(zero != NULL, CCC_TRUE);
    CHECK(*zero, 0);
    cmp = bufcmp(&b, BUFINSCAP - 1, (int[BUFINSCAP - 1]){0, 1, 2, 3, 4, 5});
    CHECK(cmp, CCC_ORDER_EQUAL);
    CHECK(buffer_count(&b).count, BUFINSCAP - 1);
    int const *const six = buffer_insert(&b, 6, &(int){6});
    CHECK(six != NULL, CCC_TRUE);
    CHECK(*six, 6);
    cmp = bufcmp(&b, BUFINSCAP, (int[BUFINSCAP]){0, 1, 2, 3, 4, 5, 6});
    CHECK(cmp, CCC_ORDER_EQUAL);
    CHECK(buffer_count(&b).count, BUFINSCAP);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buffer_test_insert_no_alloc_fail)
{
    enum : size_t
    {
        BUFINSCAP = 8,
    };
    Buffer b = buffer_initialize(((int[BUFINSCAP]){0, 1, 2, 3, 4, 5, 6}), int,
                                 NULL, NULL, BUFINSCAP, BUFINSCAP);
    CHECK(buffer_count(&b).count, BUFINSCAP);
    int const *const three = buffer_insert(&b, 3, &(int){3});
    CHECK(three == NULL, CCC_TRUE);
    CHECK(buffer_count(&b).count, BUFINSCAP);
    CHECK_END_FN();
}

/* Force a resize when inserting in middle forces shuffle down. */
CHECK_BEGIN_STATIC_FN(buffer_test_insert_alloc)
{
    Buffer b = buffer_initialize(NULL, int, std_alloc, NULL, 0);
    CCC_Result r = buffer_reserve(&b, 6, std_alloc);
    CHECK(r, CCC_RESULT_OK);
    r = append_range(&b, 6, (int[6]){1, 2, 4, 5, 6, 7});
    CHECK(r, CCC_RESULT_OK);
    CHECK(buffer_count(&b).count, 6);
    int const *const three = buffer_insert(&b, 2, &(int){3});
    CHECK(three != NULL, CCC_TRUE);
    CHECK(*three, 3);
    CCC_Order cmp = bufcmp(&b, 7, (int[7]){1, 2, 3, 4, 5, 6, 7});
    CHECK(cmp, CCC_ORDER_EQUAL);
    CHECK(buffer_count(&b).count, 7);
    int const *const zero = buffer_insert(&b, 0, &(int){0});
    CHECK(zero != NULL, CCC_TRUE);
    CHECK(*zero, 0);
    cmp = bufcmp(&b, 8, (int[8]){0, 1, 2, 3, 4, 5, 6, 7});
    CHECK(cmp, CCC_ORDER_EQUAL);
    CHECK(buffer_count(&b).count, 8);
    int const *const eight = buffer_insert(&b, 8, &(int){8});
    CHECK(eight != NULL, CCC_TRUE);
    CHECK(*eight, 8);
    cmp = bufcmp(&b, 9, (int[9]){0, 1, 2, 3, 4, 5, 6, 7, 8});
    CHECK(cmp, CCC_ORDER_EQUAL);
    CHECK(buffer_count(&b).count, 9);
    CHECK_END_FN(buffer_clear_and_free(&b, NULL););
}

int
main(void)
{
    /* NOLINTNEXTLINE */
    srand(time(NULL));
    return CHECK_RUN(buffer_test_push_fixed(), buffer_test_push_resize(),
                     buffer_test_push_qsort(), buffer_test_push_sort(),
                     buffer_test_insert_no_alloc(),
                     buffer_test_insert_no_alloc_fail(),
                     buffer_test_insert_alloc());
}
