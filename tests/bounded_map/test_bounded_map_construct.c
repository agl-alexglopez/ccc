#include <stdbool.h>
#include <stddef.h>

#include "bounded_map.h"
#include "bounded_map_utility.h"
#include "checkers.h"
#include "types.h"
#include "utility/stack_allocator.h"

static CCC_Bounded_map
construct_empty(void)
{
    CCC_Bounded_map map = CCC_bounded_map_initialize(struct Val, elem, key,
                                                     id_order, NULL, NULL);
    return map;
}

check_static_begin(bounded_map_test_empty)
{
    CCC_Bounded_map s = CCC_bounded_map_initialize(struct Val, elem, key,
                                                   id_order, NULL, NULL);
    check(CCC_bounded_map_is_empty(&s), true);
    check_end();
}

/** If the user constructs a node style map from a helper function, the map
cannot have any self referential fields, such as nil or sentinel nodes. If
the map is initialized on the stack those self referential fields will become
invalidated after the constructing function ends. This leads to a dangling
reference to stack memory that no longer exists. Disastrous. The solution is
to never implement sentinels that refer to a memory address on the map struct
itself. */
check_static_begin(bounded_map_test_construct)
{
    struct Val push = {};
    CCC_Bounded_map map = construct_empty();
    CCC_Entry entry = CCC_bounded_map_insert_or_assign(&map, &push.elem);
    check(CCC_bounded_map_validate(&map), true);
    check(CCC_entry_insert_error(&entry), false);
    check(CCC_entry_occupied(&entry), false);
    check(CCC_bounded_map_count(&map).count, 1);
    check_end();
}

check_static_begin(bounded_map_test_construct_from)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 3);
    CCC_Bounded_map map = CCC_bounded_map_from(
        elem, key, id_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[]){
            {.key = 0, .val = 0},
            {.key = 1, .val = 1},
            {.key = 2, .val = 2},
        });
    check(CCC_bounded_map_validate(&map), true);
    check(CCC_bounded_map_count(&map).count, 3);
    check_end((void)CCC_bounded_map_clear(&map, NULL););
}

check_static_begin(bounded_map_test_construct_from_overwrite)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 3);
    CCC_Bounded_map map = CCC_bounded_map_from(
        elem, key, id_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[]){
            {.key = 0, .val = 0},
            {.key = 1, .val = 1},
            {.key = 1, .val = 2},
        });
    check(CCC_bounded_map_validate(&map), true);
    check(CCC_bounded_map_count(&map).count, 2);
    struct Val const *const v = CCC_bounded_map_reverse_begin(&map);
    check(v != NULL, true);
    check(v->key, 1);
    check(v->val, 2);
    check_end((void)CCC_bounded_map_clear(&map, NULL););
}

check_static_begin(bounded_map_test_construct_from_fail)
{
    CCC_Bounded_map map
        = CCC_bounded_map_from(elem, key, id_order, NULL, NULL, NULL,
                               (struct Val[]){
                                   {.key = 0, .val = 0},
                                   {.key = 1, .val = 1},
                                   {.key = 2, .val = 2},
                               });
    check(CCC_bounded_map_validate(&map), true);
    check(CCC_bounded_map_is_empty(&map), true);
    check_end((void)CCC_bounded_map_clear(&map, NULL););
}

int
main()
{
    return check_run(bounded_map_test_empty(), bounded_map_test_construct(),
                     bounded_map_test_construct_from(),
                     bounded_map_test_construct_from_overwrite(),
                     bounded_map_test_construct_from_fail());
}
