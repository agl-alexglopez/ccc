#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "alloc.h"
#include "buf_util.h"
#include "ccc/buffer.h"
#include "ccc/types.h"
#include "checkers.h"
#include "random.h"

CHECK_BEGIN_STATIC_FN(buf_test_push_pop_fixed)
{
    buffer b = buf_init((int[8]){}, int, NULL, NULL, 8);
    int const push[8] = {7, 6, 5, 4, 3, 2, 1, 0};
    size_t count = 0;
    for (size_t i = 0; i < sizeof(push) / sizeof(*push); ++i)
    {
        int const *const p = buf_push_back(&b, &push[i]);
        CHECK(p != NULL, CCC_TRUE);
        CHECK(*p, push[i]);
        ++count;
    }
    CHECK(buf_size(&b).count, sizeof(push) / sizeof(*push));
    CHECK(buf_size(&b).count, count);
    CHECK(buf_push_back(&b, &(int){99}) == NULL, CCC_TRUE);
    while (!buf_is_empty(&b))
    {
        int const v = *(int *)buf_back(&b);
        CHECK(buf_pop_back(&b), CCC_RESULT_OK);
        --count;
        CHECK(v, push[count]);
    }
    CHECK(buf_size(&b).count, count);
    CHECK(count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buf_test_push_resize_pop)
{
    buffer b = buf_init(NULL, int, std_alloc, NULL, 0);
    size_t const cap = 32;
    int *const many = malloc(sizeof(int) * cap);
    iota(many, cap, 0);
    CHECK(many != NULL, CCC_TRUE);
    size_t count = 0;
    for (size_t i = 0; i < cap; ++i)
    {
        int *p = buf_push_back(&b, &many[i]);
        CHECK(p != NULL, CCC_TRUE);
        CHECK(*p, many[i]);
        ++count;
    }
    CHECK(count, cap);
    CHECK(buf_size(&b).count, cap);
    CHECK(buf_capacity(&b).count >= cap, CCC_TRUE);
    while (!buf_is_empty(&b))
    {
        int const v = *(int *)buf_back(&b);
        CHECK(buf_pop_back(&b), CCC_RESULT_OK);
        --count;
        CHECK(v, many[count]);
    }
    CHECK(buf_size(&b).count, count);
    CHECK(count, 0);
    CHECK_END_FN((void)buf_clear_and_free(&b, NULL););
}

CHECK_BEGIN_STATIC_FN(buf_test_daily_temperatures)
{
    enum : size_t
    {
        TMPCAP = 8,
    };
    buffer const temps
        = buf_init(((int[TMPCAP]){73, 74, 75, 71, 69, 72, 76, 73}), int, NULL,
                   NULL, TMPCAP, TMPCAP);
    buffer const correct = buf_init(((int[TMPCAP]){1, 1, 4, 2, 1, 1, 0, 0}),
                                    int, NULL, NULL, TMPCAP, TMPCAP);
    buffer res = buf_init((int[TMPCAP]){}, int, NULL, NULL, TMPCAP, TMPCAP);
    buffer idx_stack = buf_init((int[TMPCAP]){}, int, NULL, NULL, TMPCAP);
    for (int i = 0, end = (int)buf_size(&temps).count; i < end; ++i)
    {
        while (!buf_is_empty(&idx_stack)
               && *(int *)buf_at(&temps, i)
                      > *(int *)buf_at(&temps, *(int *)buf_back(&idx_stack)))
        {
            int const *const ptr
                = buf_emplace(&res, *(int *)buf_back(&idx_stack),
                              i - *(int *)buf_back(&idx_stack));
            CHECK(ptr != NULL, CCC_TRUE);
            ccc_result const r = buf_pop_back(&idx_stack);
            CHECK(r, CCC_RESULT_OK);
        }
        int const *const ptr = buf_push_back(&idx_stack, &i);
        CHECK(ptr != NULL, CCC_TRUE);
    }
    CHECK(memcmp(buf_begin(&res), buf_begin(&correct),
                 buf_size_bytes(&correct).count),
          0);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(buf_test_push_pop_fixed(), buf_test_push_resize_pop(),
                     buf_test_daily_temperatures());
}
