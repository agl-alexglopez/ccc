#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_BOUNDED_MAP_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_bounded_map.h"
#include "handle_bounded_map_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

check_static_begin(handle_bounded_map_test_empty)
{
    Handle_bounded_map s
        = handle_bounded_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(handle_bounded_map_test_copy_no_allocate)
{
    Handle_bounded_map src
        = handle_bounded_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    Handle_bounded_map dst
        = handle_bounded_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    (void)swap_handle(&src, &(struct Val){.id = 0});
    (void)swap_handle(&src, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct Val){.id = 2, .val = 2});
    check(count(&src).count, 3);
    check(is_empty(&dst), true);
    CCC_Result res = handle_bounded_map_copy(&dst, &src, NULL);
    check(res, CCC_RESULT_OK);
    check(count(&dst).count, count(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        struct Val src_v = {.id = i};
        struct Val dst_v = {.id = i};
        CCC_Handle src_e = CCC_remove(&src, &src_v);
        CCC_Handle dst_e = CCC_remove(&dst, &dst_v);
        check(occupied(&src_e), occupied(&dst_e));
        check(src_v.id, dst_v.id);
        check(src_v.val, dst_v.val);
    }
    check(is_empty(&src), is_empty(&dst));
    check(is_empty(&dst), true);
    check_end();
}

check_static_begin(handle_bounded_map_test_copy_no_allocate_fail)
{
    Handle_bounded_map src = handle_bounded_map_initialize(
        &(standard_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        STANDARD_FIXED_CAP);
    Handle_bounded_map dst
        = handle_bounded_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    (void)swap_handle(&src, &(struct Val){.id = 0});
    (void)swap_handle(&src, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct Val){.id = 2, .val = 2});
    check(count(&src).count, 3);
    check(is_empty(&dst), true);
    CCC_Result res = handle_bounded_map_copy(&dst, &src, NULL);
    check(res != CCC_RESULT_OK, true);
    check_end();
}

check_static_begin(handle_bounded_map_test_copy_allocate)
{
    Handle_bounded_map src = handle_bounded_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    Handle_bounded_map dst = handle_bounded_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    (void)swap_handle(&src, &(struct Val){.id = 0});
    (void)swap_handle(&src, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct Val){.id = 2, .val = 2});
    check(count(&src).count, 3);
    check(is_empty(&dst), true);
    CCC_Result res = handle_bounded_map_copy(&dst, &src, std_allocate);
    check(res, CCC_RESULT_OK);
    check(count(&dst).count, count(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        struct Val src_v = {.id = i};
        struct Val dst_v = {.id = i};
        CCC_Handle src_e = CCC_remove(&src, &src_v);
        CCC_Handle dst_e = CCC_remove(&dst, &dst_v);
        check(occupied(&src_e), occupied(&dst_e));
        check(src_v.id, dst_v.id);
        check(src_v.val, dst_v.val);
    }
    check(is_empty(&src), is_empty(&dst));
    check(is_empty(&dst), true);
    check_end({
        (void)handle_bounded_map_clear_and_free(&src, NULL);
        (void)handle_bounded_map_clear_and_free(&dst, NULL);
    });
}

check_static_begin(handle_bounded_map_test_copy_allocate_fail)
{
    Handle_bounded_map src = handle_bounded_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    Handle_bounded_map dst = handle_bounded_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    (void)swap_handle(&src, &(struct Val){.id = 0});
    (void)swap_handle(&src, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct Val){.id = 2, .val = 2});
    check(count(&src).count, 3);
    check(is_empty(&dst), true);
    CCC_Result res = handle_bounded_map_copy(&dst, &src, NULL);
    check(res != CCC_RESULT_OK, true);
    check_end({ (void)handle_bounded_map_clear_and_free(&src, NULL); });
}

int
main()
{
    return check_run(handle_bounded_map_test_empty(),
                     handle_bounded_map_test_copy_no_allocate(),
                     handle_bounded_map_test_copy_no_allocate_fail(),
                     handle_bounded_map_test_copy_allocate(),
                     handle_bounded_map_test_copy_allocate_fail());
}
