#include <stddef.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "ccc/buffer.h"
#include "ccc/types.h"
#include "checkers.h"
#include "utility/allocate.h"

check_static_begin(buffer_test_empty)
{
    Buffer b = buffer_initialize((int[5]){}, int, NULL, NULL, 5);
    check(buffer_count(&b).count, 0);
    check(buffer_capacity(&b).count, 5);
    int const *const i = buffer_at(&b, 0);
    check(i != NULL, CCC_TRUE);
    check(*i, 0);
    check_end();
}

check_static_begin(buffer_test_full)
{
    Buffer b
        = buffer_initialize(((int[5]){0, 1, 2, 3, 4}), int, NULL, NULL, 5, 5);
    check(buffer_count(&b).count, 5);
    check(buffer_capacity(&b).count, 5);
    int const *const i = buffer_at(&b, 2);
    check(i != NULL, CCC_TRUE);
    check(*i, 2);
    check_end();
}

check_static_begin(buffer_test_reserve)
{
    Buffer b = buffer_initialize(NULL, int, std_allocate, NULL, 0);
    check(buffer_reserve(&b, 8, std_allocate), CCC_RESULT_OK);
    check(buffer_count(&b).count, 0);
    check(buffer_capacity(&b).count, 8);
    check_end(buffer_clear_and_free(&b, NULL););
}

check_static_begin(buffer_test_copy_no_allocate)
{
    Buffer source
        = buffer_initialize(((int[5]){0, 1, 2, 3, 4}), int, NULL, NULL, 5, 5);
    Buffer destination = buffer_initialize(((int[10]){}), int, NULL, NULL, 10);
    check(buffer_count(&destination).count, 0);
    check(buffer_capacity(&destination).count, 10);
    CCC_Result const r = buffer_copy(&destination, &source, NULL);
    check(r, CCC_RESULT_OK);
    check(buffer_count(&destination).count, 5);
    check(buffer_capacity(&destination).count, 10);
    check_end();
}

check_static_begin(buffer_test_copy_no_allocate_fail)
{
    Buffer source
        = buffer_initialize(((int[3]){0, 1, 2}), int, NULL, NULL, 3, 3);
    Buffer bad_destination
        = buffer_initialize(((int[2]){}), int, NULL, NULL, 2);
    check(buffer_count(&source).count, 3);
    check(buffer_is_empty(&bad_destination), CCC_TRUE);
    CCC_Result res = buffer_copy(&bad_destination, &source, NULL);
    check(res != CCC_RESULT_OK, CCC_TRUE);
    check_end();
}

check_static_begin(buffer_test_copy_allocate)
{
    Buffer source = buffer_initialize(NULL, int, std_allocate, NULL, 0);
    Buffer destination = buffer_initialize(NULL, int, NULL, NULL, 0);
    check(buffer_is_empty(&destination), CCC_TRUE);
    enum : size_t
    {
        PUSHCAP = 5,
    };
    int push[PUSHCAP] = {0, 1, 2, 3, 4};
    for (size_t i = 0; i < PUSHCAP; ++i)
    {
        check(buffer_push_back(&source, &push[i]) != NULL, CCC_TRUE);
    }
    CCC_Result const res = buffer_copy(&destination, &source, std_allocate);
    check(res, CCC_RESULT_OK);
    check(*(int *)buffer_begin(&source), 0);
    check(buffer_count(&destination).count, 5);
    while (!buffer_is_empty(&source) && !buffer_is_empty(&destination))
    {
        int const a = *(int *)buffer_back(&source);
        int const b = *(int *)buffer_back(&destination);
        (void)buffer_pop_back(&source);
        (void)buffer_pop_back(&destination);
        check(a, b);
    }
    check(buffer_is_empty(&source), buffer_is_empty(&destination));
    check_end({
        (void)buffer_clear_and_free(&source, NULL);
        (void)buffer_clear_and_free_reserve(&destination, NULL, std_allocate);
    });
}

check_static_begin(buffer_test_copy_allocate_fail)
{
    Buffer source = buffer_initialize(NULL, int, std_allocate, NULL, 0);
    Buffer destination = buffer_initialize(NULL, int, NULL, NULL, 0);
    check(buffer_push_back(&source, &(int){88}) != NULL, CCC_TRUE);
    CCC_Result const res = buffer_copy(&destination, &source, NULL);
    check(res != CCC_RESULT_OK, CCC_TRUE);
    check_end({ (void)buffer_clear_and_free(&source, NULL); });
}

check_static_begin(buffer_test_init_from)
{
    CCC_Buffer b
        = CCC_buffer_from(std_allocate, NULL, 8, (int[]){1, 2, 3, 4, 5, 6, 7});
    int elem = 1;
    for (int const *i = CCC_buffer_begin(&b); i != CCC_buffer_end(&b);
         i = CCC_buffer_next(&b, i))
    {
        check(elem, *i);
        ++elem;
    }
    check(elem, 8);
    check(CCC_buffer_count(&b).count, elem - 1);
    check(CCC_buffer_capacity(&b).count, elem);
    check_end((void)CCC_buffer_clear_and_free(&b, NULL););
}

check_static_begin(buffer_test_init_from_fail)
{
    /* Whoops forgot allocation function. */
    CCC_Buffer b = CCC_buffer_from(NULL, NULL, 0, (int[]){1, 2, 3, 4, 5, 6, 7});
    int elem = 1;
    for (int const *i = CCC_buffer_begin(&b); i != CCC_buffer_end(&b);
         i = CCC_buffer_next(&b, i))
    {
        check(elem, *i);
        ++elem;
    }
    check(elem, 1);
    check(CCC_buffer_count(&b).count, 0);
    check(CCC_buffer_capacity(&b).count, 0);
    check(CCC_buffer_push_back(&b, &(int){}), NULL);
    check_end((void)CCC_buffer_clear_and_free(&b, NULL););
}

check_static_begin(buffer_test_init_with_capacity)
{
    CCC_Buffer b = CCC_buffer_with_capacity(int, std_allocate, NULL, 8);
    check(CCC_buffer_capacity(&b).count, 8);
    check(CCC_buffer_push_back(&b, &(int){9}) != NULL, CCC_TRUE);
    size_t count = 0;
    for (int const *i = CCC_buffer_begin(&b); i != CCC_buffer_capacity_end(&b);
         i = CCC_buffer_next(&b, i))
    {
        ++count;
    }
    check(count, 8);
    check_end(CCC_buffer_clear_and_free(&b, NULL););
}

check_static_begin(buffer_test_init_with_capacity_fail)
{
    /* Forgot allocation function. */
    CCC_Buffer b = CCC_buffer_with_capacity(int, NULL, NULL, 8);
    check(CCC_buffer_capacity(&b).count, 0);
    check(CCC_buffer_push_back(&b, &(int){9}), NULL);
    size_t count = 0;
    for (int const *i = CCC_buffer_begin(&b); i != CCC_buffer_capacity_end(&b);
         i = CCC_buffer_next(&b, i))
    {
        ++count;
    }
    check(count, 0);
    check_end(CCC_buffer_clear_and_free(&b, NULL););
}

int
main(void)
{
    return check_run(
        buffer_test_empty(), buffer_test_full(), buffer_test_reserve(),
        buffer_test_copy_no_allocate(), buffer_test_copy_no_allocate_fail(),
        buffer_test_copy_allocate(), buffer_test_copy_allocate_fail(),
        buffer_test_init_from(), buffer_test_init_from_fail(),
        buffer_test_init_with_capacity(),
        buffer_test_init_with_capacity_fail());
}
