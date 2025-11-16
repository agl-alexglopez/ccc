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

check_static_begin(handle_adaptive_map_test_init_from)
{
    Handle_adaptive_map map_from_list
        = handle_adaptive_map_from(id, id_order, std_allocate, NULL, 0,
                                   (struct Val[]){
                                       {.id = 0, .val = 0},
                                       {.id = 1, .val = 1},
                                       {.id = 2, .val = 2},
                                   });
    check(validate(&map_from_list), true);
    check(count(&map_from_list).count, 3);
    size_t seen = 0;
    for (struct Val const *i = begin(&map_from_list); i != end(&map_from_list);
         i = next(&map_from_list, i))
    {
        check((i->id == 0 && i->val == 0) || (i->id == 1 && i->val == 1)
                  || (i->id == 2 && i->val == 2),
              true);
        ++seen;
    }
    check(seen, 3);
    check_end(handle_adaptive_map_clear_and_free(&map_from_list, NULL););
}

check_static_begin(handle_adaptive_map_test_init_from_overwrite)
{
    Handle_adaptive_map map_from_list
        = handle_adaptive_map_from(id, id_order, std_allocate, NULL, 0,
                                   (struct Val[]){
                                       {.id = 0, .val = 0},
                                       {.id = 0, .val = 1},
                                       {.id = 0, .val = 2},
                                   });
    check(validate(&map_from_list), true);
    check(count(&map_from_list).count, 1);
    size_t seen = 0;
    for (struct Val const *i = begin(&map_from_list); i != end(&map_from_list);
         i = next(&map_from_list, i))
    {
        check(i->id, 0);
        check(i->val, 2);
        ++seen;
    }
    check(seen, 1);
    check_end(handle_adaptive_map_clear_and_free(&map_from_list, NULL););
}

check_static_begin(handle_adaptive_map_test_init_from_fail)
{
    // Whoops forgot an allocation function.
    Handle_adaptive_map map_from_list
        = handle_adaptive_map_from(id, id_order, NULL, NULL, 0,
                                   (struct Val[]){
                                       {.id = 0, .val = 0},
                                       {.id = 0, .val = 1},
                                       {.id = 0, .val = 2},
                                   });
    check(validate(&map_from_list), true);
    check(count(&map_from_list).count, 0);
    size_t seen = 0;
    for (struct Val const *i = begin(&map_from_list); i != end(&map_from_list);
         i = next(&map_from_list, i))
    {
        check(i->id, 0);
        check(i->val, 2);
        ++seen;
    }
    check(seen, 0);
    CCC_Handle h = CCC_handle_adaptive_map_insert_or_assign(
        &map_from_list, &(struct Val){.id = 1, .val = 1});
    check(CCC_handle_insert_error(&h), CCC_TRUE);
    check_end(handle_adaptive_map_clear_and_free(&map_from_list, NULL););
}

check_static_begin(handle_adaptive_map_test_init_with_capacity)
{
    Handle_adaptive_map map = handle_adaptive_map_with_capacity(
        struct Val, id, id_order, std_allocate, NULL, 32);
    check(validate(&map), true);
    check(handle_adaptive_map_capacity(&map).count >= 32, true);
    for (int i = 0; i < 10; ++i)
    {
        CCC_Handle const h = CCC_handle_adaptive_map_insert_or_assign(
            &map, &(struct Val){.id = i, .val = i});
        check(CCC_handle_insert_error(&h), CCC_FALSE);
        check(handle_adaptive_map_validate(&map), CCC_TRUE);
    }
    check(handle_adaptive_map_count(&map).count, 10);
    size_t seen = 0;
    for (struct Val const *i = begin(&map); i != end(&map); i = next(&map, i))
    {
        check(i->id >= 0 && i->id < 10, true);
        check(i->val >= 0 && i->val < 10, true);
        check(i->val, i->id);
        ++seen;
    }
    check(seen, 10);
    check_end(handle_adaptive_map_clear_and_free(&map, NULL););
}

check_static_begin(handle_adaptive_map_test_init_with_capacity_no_op)
{
    /* Initialize with 0 cap is OK just does nothing. */
    Handle_adaptive_map map = handle_adaptive_map_with_capacity(
        struct Val, id, id_order, std_allocate, NULL, 0);
    check(validate(&map), true);
    check(handle_adaptive_map_capacity(&map).count, 0);
    check(handle_adaptive_map_count(&map).count, 0);
    CCC_Handle const h = CCC_handle_adaptive_map_insert_or_assign(
        &map, &(struct Val){.id = 1, .val = 1});
    check(CCC_handle_insert_error(&h), CCC_FALSE);
    check(handle_adaptive_map_validate(&map), CCC_TRUE);
    check(handle_adaptive_map_count(&map).count, 1);
    size_t seen = 0;
    for (struct Val const *i = begin(&map); i != end(&map); i = next(&map, i))
    {
        check(i->id, i->val);
        ++seen;
    }
    check(handle_adaptive_map_count(&map).count, 1);
    check(handle_adaptive_map_capacity(&map).count > 0, true);
    check(seen, 1);
    check_end(handle_adaptive_map_clear_and_free(&map, NULL););
}

check_static_begin(handle_adaptive_map_test_init_with_capacity_fail)
{
    /* Forgot allocation function. */
    Handle_adaptive_map map = handle_adaptive_map_with_capacity(
        struct Val, id, id_order, NULL, NULL, 32);
    check(validate(&map), true);
    check(handle_adaptive_map_capacity(&map).count, 0);
    CCC_Handle const e = CCC_handle_adaptive_map_insert_or_assign(
        &map, &(struct Val){.id = 1, .val = 1});
    check(CCC_handle_insert_error(&e), CCC_TRUE);
    check(handle_adaptive_map_validate(&map), CCC_TRUE);
    check(handle_adaptive_map_count(&map).count, 0);
    size_t seen = 0;
    for (struct Val const *i = begin(&map); i != end(&map); i = next(&map, i))
    {
        check(i->id, i->val);
        ++seen;
    }
    check(handle_adaptive_map_count(&map).count, 0);
    check(seen, 0);
    check_end(handle_adaptive_map_clear_and_free(&map, NULL););
}

int
main()
{
    return check_run(handle_adaptive_map_test_empty(),
                     handle_adaptive_map_test_copy_no_allocate(),
                     handle_adaptive_map_test_copy_no_allocate_fail(),
                     handle_adaptive_map_test_copy_allocate(),
                     handle_adaptive_map_test_copy_allocate_fail(),
                     handle_adaptive_map_test_init_from(),
                     handle_adaptive_map_test_init_from_overwrite(),
                     handle_adaptive_map_test_init_from_fail(),
                     handle_adaptive_map_test_init_with_capacity(),
                     handle_adaptive_map_test_init_with_capacity_no_op(),
                     handle_adaptive_map_test_init_with_capacity_fail());
}
