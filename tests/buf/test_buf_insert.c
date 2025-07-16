#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "buf_util.h"
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

static ccc_threeway_cmp
ccc_cmp_ints(ccc_any_type_cmp const cmp)
{
    int const lhs_int = *(int *)cmp.any_type_lhs;
    int const rhs_int = *(int *)cmp.any_type_rhs;
    return (lhs_int > rhs_int) - (lhs_int < rhs_int);
}

CHECK_BEGIN_STATIC_FN(buf_test_push_fixed)
{
    buffer b = buf_init((int[8]){}, int, NULL, NULL, 8);
    int const push[8] = {7, 6, 5, 4, 3, 2, 1, 0};
    for (size_t i = 0; i < sizeof(push) / sizeof(*push); ++i)
    {
        int *p = buf_push_back(&b, &push[i]);
        CHECK(p != NULL, CCC_TRUE);
        CHECK(*p, push[i]);
    }
    CHECK(buf_count(&b).count, sizeof(push) / sizeof(*push));
    CHECK(buf_push_back(&b, &(int){99}) == NULL, CCC_TRUE);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buf_test_push_resize)
{
    buffer b = buf_init(NULL, int, std_alloc, NULL, 0);
    size_t const cap = 32;
    int *const many = malloc(sizeof(int) * cap);
    iota(many, cap, 0);
    CHECK(many != NULL, CCC_TRUE);
    for (size_t i = 0; i < cap; ++i)
    {
        int *p = buf_push_back(&b, &many[i]);
        CHECK(p != NULL, CCC_TRUE);
        CHECK(*p, many[i]);
    }
    CHECK(buf_count(&b).count, cap);
    CHECK(buf_capacity(&b).count >= cap, CCC_TRUE);
    CHECK_END_FN({
        (void)buf_clear_and_free(&b, NULL);
        free(many);
    });
}

CHECK_BEGIN_STATIC_FN(buf_test_push_qsort)
{
    enum : size_t
    {
        BUF_SORT_CAP = 32,
    };
    buffer b = buf_init((int[BUF_SORT_CAP]){}, int, NULL, NULL, BUF_SORT_CAP,
                        BUF_SORT_CAP);
    int ref[BUF_SORT_CAP] = {};
    iota(ref, BUF_SORT_CAP, 0);
    iota(buf_begin(&b), BUF_SORT_CAP, 0);
    CHECK(memcmp(ref, buf_begin(&b), BUF_SORT_CAP * sizeof(*ref)), CCC_EQL);
    rand_shuffle(sizeof(*ref), ref, BUF_SORT_CAP, &(int){0});
    rand_shuffle(buf_sizeof_type(&b).count, buf_begin(&b), buf_count(&b).count,
                 &(int){0});
    qsort(ref, BUF_SORT_CAP, sizeof(*ref), cmp_ints);
    qsort(buf_begin(&b), buf_capacity(&b).count, buf_sizeof_type(&b).count,
          cmp_ints);
    CHECK(memcmp(ref, buf_begin(&b), BUF_SORT_CAP * sizeof(*ref)), CCC_EQL);
    int prev = INT_MIN;
    size_t count = 0;
    for (int const *i = buf_begin(&b); i != buf_end(&b); i = buf_next(&b, i))
    {
        CHECK(i != NULL, CCC_TRUE);
        CHECK(*i >= prev, CCC_TRUE);
        prev = *i;
        ++count;
    }
    CHECK(count, BUF_SORT_CAP);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buf_test_push_sort)
{
    enum : size_t
    {
        BUF_SORT_CAP = 32,
    };
    buffer b = buf_init((int[BUF_SORT_CAP]){}, int, NULL, NULL, BUF_SORT_CAP,
                        BUF_SORT_CAP);
    iota(buf_begin(&b), BUF_SORT_CAP, 0);
    rand_shuffle(buf_sizeof_type(&b).count, buf_begin(&b), buf_count(&b).count,
                 &(int){0});
    (void)sort(&b, ccc_cmp_ints, &(int){0});
    int prev = INT_MIN;
    size_t count = 0;
    for (int const *i = buf_begin(&b); i != buf_end(&b); i = buf_next(&b, i))
    {
        CHECK(i != NULL, CCC_TRUE);
        CHECK(*i >= prev, CCC_TRUE);
        prev = *i;
        ++count;
    }
    CHECK(count, BUF_SORT_CAP);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buf_test_insert_no_alloc)
{
    enum : size_t
    {
        BUFINSCAP = 8,
    };
    buffer b = buf_init(((int[BUFINSCAP]){1, 2, 4, 5}), int, NULL, NULL,
                        BUFINSCAP, BUFINSCAP - 3);
    CHECK(buf_count(&b).count, BUFINSCAP - 3);
    int const *const three = buf_insert(&b, 2, &(int){3});
    CHECK(three != NULL, CCC_TRUE);
    CHECK(*three, 3);
    ccc_threeway_cmp cmp
        = bufcmp(&b, BUFINSCAP - 2, (int[BUFINSCAP - 2]){1, 2, 3, 4, 5});
    CHECK(cmp, CCC_EQL);
    CHECK(buf_count(&b).count, BUFINSCAP - 2);
    int const *const zero = buf_insert(&b, 0, &(int){0});
    CHECK(zero != NULL, CCC_TRUE);
    CHECK(*zero, 0);
    cmp = bufcmp(&b, BUFINSCAP - 1, (int[BUFINSCAP - 1]){0, 1, 2, 3, 4, 5});
    CHECK(cmp, CCC_EQL);
    CHECK(buf_count(&b).count, BUFINSCAP - 1);
    int const *const six = buf_insert(&b, 6, &(int){6});
    CHECK(six != NULL, CCC_TRUE);
    CHECK(*six, 6);
    cmp = bufcmp(&b, BUFINSCAP, (int[BUFINSCAP]){0, 1, 2, 3, 4, 5, 6});
    CHECK(cmp, CCC_EQL);
    CHECK(buf_count(&b).count, BUFINSCAP);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buf_test_insert_no_alloc_fail)
{
    enum : size_t
    {
        BUFINSCAP = 8,
    };
    buffer b = buf_init(((int[BUFINSCAP]){0, 1, 2, 3, 4, 5, 6}), int, NULL,
                        NULL, BUFINSCAP, BUFINSCAP);
    CHECK(buf_count(&b).count, BUFINSCAP);
    int const *const three = buf_insert(&b, 3, &(int){3});
    CHECK(three == NULL, CCC_TRUE);
    CHECK(buf_count(&b).count, BUFINSCAP);
    CHECK_END_FN();
}

/* Force a resize when inserting in middle forces shuffle down. */
CHECK_BEGIN_STATIC_FN(buf_test_insert_alloc)
{
    buffer b = buf_init(NULL, int, std_alloc, NULL, 0);
    ccc_result r = buf_reserve(&b, 6, std_alloc);
    CHECK(r, CCC_RESULT_OK);
    r = append_range(&b, 6, (int[6]){1, 2, 4, 5, 6, 7});
    CHECK(r, CCC_RESULT_OK);
    CHECK(buf_count(&b).count, 6);
    int const *const three = buf_insert(&b, 2, &(int){3});
    CHECK(three != NULL, CCC_TRUE);
    CHECK(*three, 3);
    ccc_threeway_cmp cmp = bufcmp(&b, 7, (int[7]){1, 2, 3, 4, 5, 6, 7});
    CHECK(cmp, CCC_EQL);
    CHECK(buf_count(&b).count, 7);
    int const *const zero = buf_insert(&b, 0, &(int){0});
    CHECK(zero != NULL, CCC_TRUE);
    CHECK(*zero, 0);
    cmp = bufcmp(&b, 8, (int[8]){0, 1, 2, 3, 4, 5, 6, 7});
    CHECK(cmp, CCC_EQL);
    CHECK(buf_count(&b).count, 8);
    int const *const eight = buf_insert(&b, 8, &(int){8});
    CHECK(eight != NULL, CCC_TRUE);
    CHECK(*eight, 8);
    cmp = bufcmp(&b, 9, (int[9]){0, 1, 2, 3, 4, 5, 6, 7, 8});
    CHECK(cmp, CCC_EQL);
    CHECK(buf_count(&b).count, 9);
    CHECK_END_FN(buf_clear_and_free(&b, NULL););
}

int
main(void)
{
    /* NOLINTNEXTLINE */
    srand(time(NULL));
    return CHECK_RUN(buf_test_push_fixed(), buf_test_push_resize(),
                     buf_test_push_qsort(), buf_test_push_sort(),
                     buf_test_insert_no_alloc(),
                     buf_test_insert_no_alloc_fail(), buf_test_insert_alloc());
}
