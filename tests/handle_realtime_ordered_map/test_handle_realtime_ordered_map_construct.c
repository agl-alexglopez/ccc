#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_realtime_ordered_map.h"
#include "handle_realtime_ordered_map_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

CHECK_BEGIN_STATIC_FN(handle_realtime_ordered_map_test_empty)
{
    Handle_realtime_ordered_map s = handle_realtime_ordered_map_initialize(
        &(small_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        SMALL_FIXED_CAP);
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_realtime_ordered_map_test_copy_no_allocate)
{
    Handle_realtime_ordered_map src = handle_realtime_ordered_map_initialize(
        &(small_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        SMALL_FIXED_CAP);
    Handle_realtime_ordered_map dst = handle_realtime_ordered_map_initialize(
        &(small_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        SMALL_FIXED_CAP);
    (void)swap_handle(&src, &(struct Val){.id = 0});
    (void)swap_handle(&src, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct Val){.id = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    CCC_Result res = handle_realtime_ordered_map_copy(&dst, &src, NULL);
    CHECK(res, CCC_RESULT_OK);
    CHECK(count(&dst).count, count(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        struct Val src_v = {.id = i};
        struct Val dst_v = {.id = i};
        CCC_Handle src_e = CCC_remove(&src, &src_v);
        CCC_Handle dst_e = CCC_remove(&dst, &dst_v);
        CHECK(occupied(&src_e), occupied(&dst_e));
        CHECK(src_v.id, dst_v.id);
        CHECK(src_v.val, dst_v.val);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_realtime_ordered_map_test_copy_no_allocate_fail)
{
    Handle_realtime_ordered_map src = handle_realtime_ordered_map_initialize(
        &(standard_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        STANDARD_FIXED_CAP);
    Handle_realtime_ordered_map dst = handle_realtime_ordered_map_initialize(
        &(small_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        SMALL_FIXED_CAP);
    (void)swap_handle(&src, &(struct Val){.id = 0});
    (void)swap_handle(&src, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct Val){.id = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    CCC_Result res = handle_realtime_ordered_map_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_realtime_ordered_map_test_copy_allocate)
{
    Handle_realtime_ordered_map src = handle_realtime_ordered_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    Handle_realtime_ordered_map dst = handle_realtime_ordered_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    (void)swap_handle(&src, &(struct Val){.id = 0});
    (void)swap_handle(&src, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct Val){.id = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    CCC_Result res = handle_realtime_ordered_map_copy(&dst, &src, std_allocate);
    CHECK(res, CCC_RESULT_OK);
    CHECK(count(&dst).count, count(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        struct Val src_v = {.id = i};
        struct Val dst_v = {.id = i};
        CCC_Handle src_e = CCC_remove(&src, &src_v);
        CCC_Handle dst_e = CCC_remove(&dst, &dst_v);
        CHECK(occupied(&src_e), occupied(&dst_e));
        CHECK(src_v.id, dst_v.id);
        CHECK(src_v.val, dst_v.val);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN({
        (void)handle_realtime_ordered_map_clear_and_free(&src, NULL);
        (void)handle_realtime_ordered_map_clear_and_free(&dst, NULL);
    });
}

CHECK_BEGIN_STATIC_FN(handle_realtime_ordered_map_test_copy_allocate_fail)
{
    Handle_realtime_ordered_map src = handle_realtime_ordered_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    Handle_realtime_ordered_map dst = handle_realtime_ordered_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    (void)swap_handle(&src, &(struct Val){.id = 0});
    (void)swap_handle(&src, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct Val){.id = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    CCC_Result res = handle_realtime_ordered_map_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN(
        { (void)handle_realtime_ordered_map_clear_and_free(&src, NULL); });
}

int
main()
{
    return CHECK_RUN(handle_realtime_ordered_map_test_empty(),
                     handle_realtime_ordered_map_test_copy_no_allocate(),
                     handle_realtime_ordered_map_test_copy_no_allocate_fail(),
                     handle_realtime_ordered_map_test_copy_allocate(),
                     handle_realtime_ordered_map_test_copy_allocate_fail());
}
