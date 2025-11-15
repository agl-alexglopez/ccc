#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_ADAPTIVE_MAP_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_adaptive_map.h"
#include "handle_adaptive_map_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

check_static_begin(handle_adaptive_map_test_empty)
{
    Handle_adaptive_map s
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(handle_adaptive_map_test_copy_no_allocate)
{
    Handle_adaptive_map source
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    Handle_adaptive_map destination
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    (void)swap_handle(&source, &(struct Val){.id = 0});
    (void)swap_handle(&source, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&source, &(struct Val){.id = 2, .val = 2});
    check(count(&source).count, 3);
    check(is_empty(&destination), true);
    CCC_Result res = handle_adaptive_map_copy(&destination, &source, NULL);
    check(res, CCC_RESULT_OK);
    check(count(&destination).count, count(&source).count);
    for (int i = 0; i < 3; ++i)
    {
        struct Val source_v = {.id = i};
        struct Val destination_v = {.id = i};
        CCC_Handle source_e = CCC_remove(&source, &source_v);
        CCC_Handle destination_e = CCC_remove(&destination, &destination_v);
        check(occupied(&source_e), occupied(&destination_e));
        check(source_v.id, destination_v.id);
        check(source_v.val, destination_v.val);
    }
    check(is_empty(&source), is_empty(&destination));
    check(is_empty(&destination), true);
    check_end();
}

check_static_begin(handle_adaptive_map_test_copy_no_allocate_fail)
{
    Handle_adaptive_map source = handle_adaptive_map_initialize(
        &(standard_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        STANDARD_FIXED_CAP);
    Handle_adaptive_map destination
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    (void)swap_handle(&source, &(struct Val){.id = 0});
    (void)swap_handle(&source, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&source, &(struct Val){.id = 2, .val = 2});
    check(count(&source).count, 3);
    check(is_empty(&destination), true);
    CCC_Result res = handle_adaptive_map_copy(&destination, &source, NULL);
    check(res != CCC_RESULT_OK, true);
    check_end();
}

check_static_begin(handle_adaptive_map_test_copy_allocate)
{
    Handle_adaptive_map source = handle_adaptive_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    Handle_adaptive_map destination = handle_adaptive_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    (void)swap_handle(&source, &(struct Val){.id = 0});
    (void)swap_handle(&source, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&source, &(struct Val){.id = 2, .val = 2});
    check(count(&source).count, 3);
    check(is_empty(&destination), true);
    CCC_Result res
        = handle_adaptive_map_copy(&destination, &source, std_allocate);
    check(res, CCC_RESULT_OK);
    check(count(&destination).count, count(&source).count);
    for (int i = 0; i < 3; ++i)
    {
        struct Val source_v = {.id = i};
        struct Val destination_v = {.id = i};
        CCC_Handle source_e = CCC_remove(&source, &source_v);
        CCC_Handle destination_e = CCC_remove(&destination, &destination_v);
        check(occupied(&source_e), occupied(&destination_e));
        check(source_v.id, destination_v.id);
        check(source_v.val, destination_v.val);
    }
    check(is_empty(&source), is_empty(&destination));
    check(is_empty(&destination), true);
    check_end({
        (void)handle_adaptive_map_clear_and_free(&source, NULL);
        (void)handle_adaptive_map_clear_and_free(&destination, NULL);
    });
}

check_static_begin(handle_adaptive_map_test_copy_allocate_fail)
{
    Handle_adaptive_map source = handle_adaptive_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    Handle_adaptive_map destination = handle_adaptive_map_initialize(
        NULL, struct Val, id, id_order, std_allocate, NULL, 0);
    (void)swap_handle(&source, &(struct Val){.id = 0});
    (void)swap_handle(&source, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&source, &(struct Val){.id = 2, .val = 2});
    check(count(&source).count, 3);
    check(is_empty(&destination), true);
    CCC_Result res = handle_adaptive_map_copy(&destination, &source, NULL);
    check(res != CCC_RESULT_OK, true);
    check_end({ (void)handle_adaptive_map_clear_and_free(&source, NULL); });
}

int
main()
{
    return check_run(handle_adaptive_map_test_empty(),
                     handle_adaptive_map_test_copy_no_allocate(),
                     handle_adaptive_map_test_copy_no_allocate_fail(),
                     handle_adaptive_map_test_copy_allocate(),
                     handle_adaptive_map_test_copy_allocate_fail());
}
