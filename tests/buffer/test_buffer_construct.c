#include <stddef.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "ccc/buffer.h"
#include "ccc/types.h"
#include "checkers.h"
#include "util/allocate.h"

CHECK_BEGIN_STATIC_FN(buffer_test_empty)
{
    Buffer b = buffer_initialize((int[5]){}, int, NULL, NULL, 5);
    CHECK(buffer_count(&b).count, 0);
    CHECK(buffer_capacity(&b).count, 5);
    int const *const i = buffer_at(&b, 0);
    CHECK(i != NULL, CCC_TRUE);
    CHECK(*i, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buffer_test_full)
{
    Buffer b
        = buffer_initialize(((int[5]){0, 1, 2, 3, 4}), int, NULL, NULL, 5, 5);
    CHECK(buffer_count(&b).count, 5);
    CHECK(buffer_capacity(&b).count, 5);
    int const *const i = buffer_at(&b, 2);
    CHECK(i != NULL, CCC_TRUE);
    CHECK(*i, 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buffer_test_reserve)
{
    Buffer b = buffer_initialize(NULL, int, std_allocate, NULL, 0);
    CHECK(buffer_reserve(&b, 8, std_allocate), CCC_RESULT_OK);
    CHECK(buffer_count(&b).count, 0);
    CHECK(buffer_capacity(&b).count, 8);
    CHECK_END_FN(buffer_clear_and_free(&b, NULL););
}

CHECK_BEGIN_STATIC_FN(buffer_test_copy_no_allocate)
{
    Buffer src
        = buffer_initialize(((int[5]){0, 1, 2, 3, 4}), int, NULL, NULL, 5, 5);
    Buffer dst = buffer_initialize(((int[10]){}), int, NULL, NULL, 10);
    CHECK(buffer_count(&dst).count, 0);
    CHECK(buffer_capacity(&dst).count, 10);
    CCC_Result const r = buffer_copy(&dst, &src, NULL);
    CHECK(r, CCC_RESULT_OK);
    CHECK(buffer_count(&dst).count, 5);
    CHECK(buffer_capacity(&dst).count, 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buffer_test_copy_no_allocate_fail)
{
    Buffer src = buffer_initialize(((int[3]){0, 1, 2}), int, NULL, NULL, 3, 3);
    Buffer bad_dst = buffer_initialize(((int[2]){}), int, NULL, NULL, 2);
    CHECK(buffer_count(&src).count, 3);
    CHECK(buffer_is_empty(&bad_dst), CCC_TRUE);
    CCC_Result res = buffer_copy(&bad_dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, CCC_TRUE);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buffer_test_copy_allocate)
{
    Buffer src = buffer_initialize(NULL, int, std_allocate, NULL, 0);
    Buffer dst = buffer_initialize(NULL, int, NULL, NULL, 0);
    CHECK(buffer_is_empty(&dst), CCC_TRUE);
    enum : size_t
    {
        PUSHCAP = 5,
    };
    int push[PUSHCAP] = {0, 1, 2, 3, 4};
    for (size_t i = 0; i < PUSHCAP; ++i)
    {
        CHECK(buffer_push_back(&src, &push[i]) != NULL, CCC_TRUE);
    }
    CCC_Result const res = buffer_copy(&dst, &src, std_allocate);
    CHECK(res, CCC_RESULT_OK);
    CHECK(*(int *)buffer_begin(&src), 0);
    CHECK(buffer_count(&dst).count, 5);
    while (!buffer_is_empty(&src) && !buffer_is_empty(&dst))
    {
        int const a = *(int *)buffer_back(&src);
        int const b = *(int *)buffer_back(&dst);
        (void)buffer_pop_back(&src);
        (void)buffer_pop_back(&dst);
        CHECK(a, b);
    }
    CHECK(buffer_is_empty(&src), buffer_is_empty(&dst));
    CHECK_END_FN({
        (void)buffer_clear_and_free(&src, NULL);
        (void)buffer_clear_and_free_reserve(&dst, NULL, std_allocate);
    });
}

CHECK_BEGIN_STATIC_FN(buffer_test_copy_allocate_fail)
{
    Buffer src = buffer_initialize(NULL, int, std_allocate, NULL, 0);
    Buffer dst = buffer_initialize(NULL, int, NULL, NULL, 0);
    CHECK(buffer_push_back(&src, &(int){88}) != NULL, CCC_TRUE);
    CCC_Result const res = buffer_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, CCC_TRUE);
    CHECK_END_FN({ (void)buffer_clear_and_free(&src, NULL); });
}

CHECK_BEGIN_STATIC_FN(buffer_test_init_from)
{
    CCC_Buffer b
        = CCC_buffer_from(std_allocate, NULL, 8, (int[]){1, 2, 3, 4, 5, 6, 7});
    int elem = 1;
    for (int const *i = CCC_buffer_begin(&b); i != CCC_buffer_end(&b);
         i = CCC_buffer_next(&b, i))
    {
        CHECK(elem, *i);
        ++elem;
    }
    CHECK(elem, 8);
    CHECK(CCC_buffer_count(&b).count, elem - 1);
    CHECK(CCC_buffer_capacity(&b).count, elem);
    CHECK_END_FN((void)CCC_buffer_clear_and_free(&b, NULL););
}

CHECK_BEGIN_STATIC_FN(buffer_test_init_from_fail)
{
    /* Whoops forgot allocation function. */
    CCC_Buffer b = CCC_buffer_from(NULL, NULL, 0, (int[]){1, 2, 3, 4, 5, 6, 7});
    int elem = 1;
    for (int const *i = CCC_buffer_begin(&b); i != CCC_buffer_end(&b);
         i = CCC_buffer_next(&b, i))
    {
        CHECK(elem, *i);
        ++elem;
    }
    CHECK(elem, 1);
    CHECK(CCC_buffer_count(&b).count, 0);
    CHECK(CCC_buffer_capacity(&b).count, 0);
    CHECK(CCC_buffer_push_back(&b, &(int){}), NULL);
    CHECK_END_FN((void)CCC_buffer_clear_and_free(&b, NULL););
}

CHECK_BEGIN_STATIC_FN(buffer_test_init_with_capacity)
{
    CCC_Buffer b = CCC_buffer_with_capacity(int, std_allocate, NULL, 8);
    CHECK(CCC_buffer_capacity(&b).count, 8);
    CHECK(CCC_buffer_push_back(&b, &(int){9}) != NULL, CCC_TRUE);
    size_t count = 0;
    for (int const *i = CCC_buffer_begin(&b); i != CCC_buffer_capacity_end(&b);
         i = CCC_buffer_next(&b, i))
    {
        ++count;
    }
    CHECK(count, 8);
    CHECK_END_FN(CCC_buffer_clear_and_free(&b, NULL););
}

CHECK_BEGIN_STATIC_FN(buffer_test_init_with_capacity_fail)
{
    /* Forgot allocation function. */
    CCC_Buffer b = CCC_buffer_with_capacity(int, NULL, NULL, 8);
    CHECK(CCC_buffer_capacity(&b).count, 0);
    CHECK(CCC_buffer_push_back(&b, &(int){9}), NULL);
    size_t count = 0;
    for (int const *i = CCC_buffer_begin(&b); i != CCC_buffer_capacity_end(&b);
         i = CCC_buffer_next(&b, i))
    {
        ++count;
    }
    CHECK(count, 0);
    CHECK_END_FN(CCC_buffer_clear_and_free(&b, NULL););
}

int
main(void)
{
    return CHECK_RUN(
        buffer_test_empty(), buffer_test_full(), buffer_test_reserve(),
        buffer_test_copy_no_allocate(), buffer_test_copy_no_allocate_fail(),
        buffer_test_copy_allocate(), buffer_test_copy_allocate_fail(),
        buffer_test_init_from(), buffer_test_init_from_fail(),
        buffer_test_init_with_capacity(),
        buffer_test_init_with_capacity_fail());
}
