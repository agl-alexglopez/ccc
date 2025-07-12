#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "alloc.h"
#include "buf_util.h"
#include "ccc/buffer.h"
#include "ccc/types.h"
#include "checkers.h"
#include "random.h"

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
    CHECK(buf_size(&b).count, sizeof(push) / sizeof(*push));
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
    CHECK(buf_size(&b).count, cap);
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
    rand_shuffle(buf_sizeof_type(&b).count, buf_begin(&b), buf_size(&b).count,
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

/** Returns an array corresponding to an array of temperatures. Each index in
the result should set to the number of days needed to see a warmer temperature
than the temperature at that index in the temps array. */
CHECK_BEGIN_STATIC_FN(buf_test_daily_temps)
{
    int const temps[] = {73, 74, 75, 71, 69, 72, 76, 73};
    enum : size_t
    {
        TEMPS_CAP = sizeof(temps) / sizeof(*temps),
    };
    buffer const correct = buf_init(((int[TEMPS_CAP]){1, 1, 4, 2, 1, 1, 0, 0}),
                                    int, NULL, NULL, TEMPS_CAP, TEMPS_CAP);
    buffer res = buf_init((int[TEMPS_CAP]){}, int, NULL, NULL, TEMPS_CAP);
    buffer max_idx_stack
        = buf_init((int[TEMPS_CAP]){}, int, NULL, NULL, TEMPS_CAP);
    for (int i = 0; i < (int)TEMPS_CAP; ++i)
    {
        while (!buf_is_empty(&max_idx_stack)
               && temps[i] > temps[*(int *)buf_back(&max_idx_stack)])
        {
            int const *const p
                = buf_emplace(&res, *(int *)buf_back(&max_idx_stack),
                              i - *(int *)buf_back(&max_idx_stack));
            CHECK(p != NULL, CCC_TRUE);
            ccc_result r = buf_pop_back(&max_idx_stack);
            CHECK(r, CCC_RESULT_OK);
        }
        CHECK(buf_push_back(&max_idx_stack, &i) != NULL, CCC_TRUE);
    }
    CHECK(
        memcmp(buf_begin(&res), buf_begin(&correct),
               buf_capacity(&correct).count * buf_sizeof_type(&correct).count),
        0);
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
    rand_shuffle(buf_sizeof_type(&b).count, buf_begin(&b), buf_size(&b).count,
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

int
main(void)
{
    /* NOLINTNEXTLINE */
    srand(time(NULL));
    return CHECK_RUN(buf_test_push_fixed(), buf_test_push_resize(),
                     buf_test_push_qsort(), buf_test_push_sort(),
                     buf_test_daily_temps());
}
