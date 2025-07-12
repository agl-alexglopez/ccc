#include <stddef.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "alloc.h"
#include "ccc/buffer.h"
#include "ccc/types.h"
#include "checkers.h"

CHECK_BEGIN_STATIC_FN(buf_test_empty)
{
    buffer b = buf_init((int[5]){}, int, NULL, NULL, 5);
    CHECK(buf_size(&b).count, 0);
    CHECK(buf_capacity(&b).count, 5);
    int const *const i = buf_at(&b, 0);
    CHECK(i != NULL, CCC_TRUE);
    CHECK(*i, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buf_test_full)
{
    buffer b = buf_init(((int[5]){0, 1, 2, 3, 4}), int, NULL, NULL, 5, 5);
    CHECK(buf_size(&b).count, 5);
    CHECK(buf_capacity(&b).count, 5);
    int const *const i = buf_at(&b, 2);
    CHECK(i != NULL, CCC_TRUE);
    CHECK(*i, 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buf_test_reserve)
{
    buffer b = buf_init(NULL, int, std_alloc, NULL, 0);
    CHECK(buf_reserve(&b, 8, std_alloc), CCC_RESULT_OK);
    CHECK(buf_size(&b).count, 0);
    CHECK(buf_capacity(&b).count, 8);
    CHECK_END_FN(buf_clear_and_free(&b, NULL););
}

CHECK_BEGIN_STATIC_FN(buf_test_copy_no_alloc)
{
    buffer src = buf_init(((int[5]){0, 1, 2, 3, 4}), int, NULL, NULL, 5, 5);
    buffer dst = buf_init(((int[10]){}), int, NULL, NULL, 10);
    CHECK(buf_size(&dst).count, 0);
    CHECK(buf_capacity(&dst).count, 10);
    ccc_result const r = buf_copy(&dst, &src, NULL);
    CHECK(r, CCC_RESULT_OK);
    CHECK(buf_size(&dst).count, 5);
    CHECK(buf_capacity(&dst).count, 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buf_test_copy_no_alloc_fail)
{
    buffer src = buf_init(((int[3]){0, 1, 2}), int, NULL, NULL, 3, 3);
    buffer bad_dst = buf_init(((int[2]){}), int, NULL, NULL, 2);
    CHECK(buf_size(&src).count, 3);
    CHECK(buf_is_empty(&bad_dst), CCC_TRUE);
    ccc_result res = buf_copy(&bad_dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, CCC_TRUE);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buf_test_copy_alloc)
{
    buffer src = buf_init(NULL, int, std_alloc, NULL, 0);
    buffer dst = buf_init(NULL, int, NULL, NULL, 0);
    CHECK(buf_is_empty(&dst), CCC_TRUE);
    enum : size_t
    {
        PUSHCAP = 5,
    };
    int push[PUSHCAP] = {0, 1, 2, 3, 4};
    for (size_t i = 0; i < PUSHCAP; ++i)
    {
        CHECK(buf_push_back(&src, &push[i]) != NULL, CCC_TRUE);
    }
    ccc_result const res = buf_copy(&dst, &src, std_alloc);
    CHECK(res, CCC_RESULT_OK);
    CHECK(*(int *)buf_begin(&src), 0);
    CHECK(buf_size(&dst).count, 5);
    while (!buf_is_empty(&src) && !buf_is_empty(&dst))
    {
        int const a = *(int *)buf_back(&src);
        int const b = *(int *)buf_back(&dst);
        (void)buf_pop_back(&src);
        (void)buf_pop_back(&dst);
        CHECK(a, b);
    }
    CHECK(buf_is_empty(&src), buf_is_empty(&dst));
    CHECK_END_FN({
        (void)buf_clear_and_free(&src, NULL);
        (void)buf_clear_and_free_reserve(&dst, NULL, std_alloc);
    });
}

CHECK_BEGIN_STATIC_FN(buf_test_copy_alloc_fail)
{
    buffer src = buf_init(NULL, int, std_alloc, NULL, 0);
    buffer dst = buf_init(NULL, int, NULL, NULL, 0);
    CHECK(buf_push_back(&src, &(int){88}) != NULL, CCC_TRUE);
    ccc_result const res = buf_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, CCC_TRUE);
    CHECK_END_FN({ (void)buf_clear_and_free(&src, NULL); });
}

int
main(void)
{
    return CHECK_RUN(buf_test_empty(), buf_test_full(), buf_test_reserve(),
                     buf_test_copy_no_alloc(), buf_test_copy_no_alloc_fail(),
                     buf_test_copy_alloc(), buf_test_copy_alloc_fail());
}
