#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_BOUNDED_MAP_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_bounded_map.h"
#include "handle_bounded_map_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/stack_allocator.h"

check_static_begin(handle_bounded_map_test_empty)
{
    Handle_bounded_map s
        = handle_bounded_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    check(is_empty(&s), true);
    check_end();
}

check_static_begin(handle_bounded_map_test_copy_no_allocate)
{
    Handle_bounded_map source
        = handle_bounded_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    Handle_bounded_map destination
        = handle_bounded_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    (void)swap_handle(&source, &(struct Val){.id = 0});
    (void)swap_handle(&source, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&source, &(struct Val){.id = 2, .val = 2});
    check(count(&source).count, 3);
    check(is_empty(&destination), true);
    CCC_Result res = handle_bounded_map_copy(&destination, &source, NULL);
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

check_static_begin(handle_bounded_map_test_copy_no_allocate_fail)
{
    Handle_bounded_map source = handle_bounded_map_initialize(
        &(Standard_fixed_map){}, struct Val, id, id_order, NULL, NULL,
        STANDARD_FIXED_CAP);
    Handle_bounded_map destination
        = handle_bounded_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    (void)swap_handle(&source, &(struct Val){.id = 0});
    (void)swap_handle(&source, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&source, &(struct Val){.id = 2, .val = 2});
    check(count(&source).count, 3);
    check(is_empty(&destination), true);
    CCC_Result res = handle_bounded_map_copy(&destination, &source, NULL);
    check(res != CCC_RESULT_OK, true);
    check_end();
}

check_static_begin(handle_bounded_map_test_copy_allocate)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(Small_fixed_map, 2);
    Handle_bounded_map source = handle_bounded_map_with_capacity(
        struct Val, id, id_order, stack_allocator_allocate, &allocator,
        SMALL_FIXED_CAP - 1);
    Handle_bounded_map destination = handle_bounded_map_initialize(
        NULL, struct Val, id, id_order, stack_allocator_allocate, &allocator,
        0);
    (void)swap_handle(&source, &(struct Val){.id = 0});
    (void)swap_handle(&source, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&source, &(struct Val){.id = 2, .val = 2});
    check(count(&source).count, 3);
    check(is_empty(&destination), true);
    CCC_Result res = handle_bounded_map_copy(&destination, &source,
                                             stack_allocator_allocate);
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
        (void)handle_bounded_map_clear_and_free(&source, NULL);
        (void)handle_bounded_map_clear_and_free(&destination, NULL);
    });
}

check_static_begin(handle_bounded_map_test_copy_allocate_fail)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(Small_fixed_map, 2);
    Handle_bounded_map source = handle_bounded_map_with_capacity(
        struct Val, id, id_order, stack_allocator_allocate, &allocator,
        SMALL_FIXED_CAP - 1);
    Handle_bounded_map destination = handle_bounded_map_initialize(
        NULL, struct Val, id, id_order, stack_allocator_allocate, &allocator,
        0);
    (void)swap_handle(&source, &(struct Val){.id = 0});
    (void)swap_handle(&source, &(struct Val){.id = 1, .val = 1});
    (void)swap_handle(&source, &(struct Val){.id = 2, .val = 2});
    check(count(&source).count, 3);
    check(is_empty(&destination), true);
    CCC_Result res = handle_bounded_map_copy(&destination, &source, NULL);
    check(res != CCC_RESULT_OK, true);
    check_end({ (void)handle_bounded_map_clear_and_free(&source, NULL); });
}

check_static_begin(handle_bounded_map_test_init_from)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(Small_fixed_map, 1);
    Handle_bounded_map map_from_list = handle_bounded_map_from(
        id, id_order, stack_allocator_allocate, &allocator, SMALL_FIXED_CAP - 1,
        (struct Val[]){
            {.id = 0, .val = 0},
            {.id = 1, .val = 1},
            {.id = 2, .val = 2},
        });
    check(validate(&map_from_list), true);
    check(count(&map_from_list).count, 3);
    size_t seen = 0;
    for (CCC_Handle_index i = begin(&map_from_list); i != end(&map_from_list);
         i = next(&map_from_list, i))
    {
        struct Val const *const v = handle_bounded_map_at(&map_from_list, i);
        check((v->id == 0 && v->val == 0) || (v->id == 1 && v->val == 1)
                  || (v->id == 2 && v->val == 2),
              true);
        ++seen;
    }
    check(seen, 3);
    check_end(handle_bounded_map_clear_and_free(&map_from_list, NULL););
}

check_static_begin(handle_bounded_map_test_init_from_overwrite)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(Small_fixed_map, 1);
    Handle_bounded_map map_from_list = handle_bounded_map_from(
        id, id_order, stack_allocator_allocate, &allocator, SMALL_FIXED_CAP - 1,
        (struct Val[]){
            {.id = 0, .val = 0},
            {.id = 0, .val = 1},
            {.id = 0, .val = 2},
        });
    check(validate(&map_from_list), true);
    check(count(&map_from_list).count, 1);
    size_t seen = 0;
    for (CCC_Handle_index i = begin(&map_from_list); i != end(&map_from_list);
         i = next(&map_from_list, i))
    {
        struct Val const *const v = handle_bounded_map_at(&map_from_list, i);
        check(v->id, 0);
        check(v->val, 2);
        ++seen;
    }
    check(seen, 1);
    check_end(handle_bounded_map_clear_and_free(&map_from_list, NULL););
}

check_static_begin(handle_bounded_map_test_init_from_fail)
{
    // Whoops forgot an allocation function.
    Handle_bounded_map map_from_list
        = handle_bounded_map_from(id, id_order, NULL, NULL, 0,
                                  (struct Val[]){
                                      {.id = 0, .val = 0},
                                      {.id = 0, .val = 1},
                                      {.id = 0, .val = 2},
                                  });
    check(validate(&map_from_list), true);
    check(count(&map_from_list).count, 0);
    size_t seen = 0;
    for (CCC_Handle_index i = begin(&map_from_list); i != end(&map_from_list);
         i = next(&map_from_list, i))
    {
        struct Val const *const v = handle_bounded_map_at(&map_from_list, i);
        check(v->id, 0);
        check(v->val, 2);
        ++seen;
    }
    check(seen, 0);
    CCC_Handle h = CCC_handle_bounded_map_insert_or_assign(
        &map_from_list, &(struct Val){.id = 1, .val = 1});
    check(CCC_handle_insert_error(&h), CCC_TRUE);
    check_end(handle_bounded_map_clear_and_free(&map_from_list, NULL););
}

check_static_begin(handle_bounded_map_test_init_with_capacity)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(Small_fixed_map, 1);
    Handle_bounded_map map = handle_bounded_map_with_capacity(
        struct Val, id, id_order, stack_allocator_allocate, &allocator,
        SMALL_FIXED_CAP - 1);
    check(validate(&map), true);
    check(handle_bounded_map_capacity(&map).count >= SMALL_FIXED_CAP - 1, true);
    for (int i = 0; i < 10; ++i)
    {
        CCC_Handle const h = CCC_handle_bounded_map_insert_or_assign(
            &map, &(struct Val){.id = i, .val = i});
        check(CCC_handle_insert_error(&h), CCC_FALSE);
        check(handle_bounded_map_validate(&map), CCC_TRUE);
    }
    check(handle_bounded_map_count(&map).count, 10);
    size_t seen = 0;
    for (CCC_Handle_index i = begin(&map); i != end(&map); i = next(&map, i))
    {
        struct Val const *const v = handle_bounded_map_at(&map, i);
        check(v->id >= 0 && v->id < 10, true);
        check(v->val >= 0 && v->val < 10, true);
        check(v->val, v->id);
        ++seen;
    }
    check(seen, 10);
    check_end(handle_bounded_map_clear_and_free(&map, NULL););
}

check_static_begin(handle_bounded_map_test_init_with_capacity_no_op)
{
    /* Initialize with 0 cap is OK just does nothing. */
    struct Stack_allocator allocator
        = stack_allocator_initialize(Small_fixed_map, 1);
    Handle_bounded_map map = handle_bounded_map_with_capacity(
        struct Val, id, id_order, stack_allocator_allocate, &allocator, 0);
    check(validate(&map), true);
    check(handle_bounded_map_capacity(&map).count, 0);
    check(handle_bounded_map_count(&map).count, 0);
    check(handle_bounded_map_reserve(&map, SMALL_FIXED_CAP - 1,
                                     stack_allocator_allocate),
          CCC_RESULT_OK);
    CCC_Handle const h = CCC_handle_bounded_map_insert_or_assign(
        &map, &(struct Val){.id = 1, .val = 1});
    check(CCC_handle_insert_error(&h), CCC_FALSE);
    check(handle_bounded_map_validate(&map), CCC_TRUE);
    check(handle_bounded_map_count(&map).count, 1);
    size_t seen = 0;
    for (CCC_Handle_index i = begin(&map); i != end(&map); i = next(&map, i))
    {
        struct Val const *const v = handle_bounded_map_at(&map, i);
        check(v->id, v->val);
        ++seen;
    }
    check(handle_bounded_map_count(&map).count, 1);
    check(handle_bounded_map_capacity(&map).count > 0, true);
    check(seen, 1);
    check_end(handle_bounded_map_clear_and_free(&map, NULL););
}

check_static_begin(handle_bounded_map_test_init_with_capacity_fail)
{
    /* Forgot allocation function. */
    Handle_bounded_map map = handle_bounded_map_with_capacity(
        struct Val, id, id_order, NULL, NULL, 32);
    check(validate(&map), true);
    check(handle_bounded_map_capacity(&map).count, 0);
    CCC_Handle const e = CCC_handle_bounded_map_insert_or_assign(
        &map, &(struct Val){.id = 1, .val = 1});
    check(CCC_handle_insert_error(&e), CCC_TRUE);
    check(handle_bounded_map_validate(&map), CCC_TRUE);
    check(handle_bounded_map_count(&map).count, 0);
    size_t seen = 0;
    for (CCC_Handle_index i = begin(&map); i != end(&map); i = next(&map, i))
    {
        struct Val const *const v = handle_bounded_map_at(&map, i);
        check(v->id, v->val);
        ++seen;
    }
    check(handle_bounded_map_count(&map).count, 0);
    check(seen, 0);
    check_end(handle_bounded_map_clear_and_free(&map, NULL););
}

int
main()
{
    return check_run(handle_bounded_map_test_empty(),
                     handle_bounded_map_test_copy_no_allocate(),
                     handle_bounded_map_test_copy_no_allocate_fail(),
                     handle_bounded_map_test_copy_allocate(),
                     handle_bounded_map_test_copy_allocate_fail(),
                     handle_bounded_map_test_init_from(),
                     handle_bounded_map_test_init_from_overwrite(),
                     handle_bounded_map_test_init_from_fail(),
                     handle_bounded_map_test_init_with_capacity(),
                     handle_bounded_map_test_init_with_capacity_no_op(),
                     handle_bounded_map_test_init_with_capacity_fail());
}
