#include <stdbool.h>
#include <stddef.h>

#include "checkers.h"
#include "tree_map.h"
#include "tree_map_utility.h"
#include "types.h"
#include "utility/stack_allocator.h"

static CCC_Tree_map
construct_empty(void)
{
    CCC_Tree_map map
        = CCC_tree_map_initialize(struct Val, elem, key, id_order, NULL, NULL);
    return map;
}

check_static_begin(tree_map_test_empty)
{
    CCC_Tree_map s
        = CCC_tree_map_initialize(struct Val, elem, key, id_order, NULL, NULL);
    check(CCC_tree_map_is_empty(&s), true);
    check_end();
}

/** If the user constructs a node style map from a helper function, the map
cannot have any self referential fields, such as nil or sentinel nodes. If
the map is initialized on the stack those self referential fields will become
invalidated after the constructing function ends. This leads to a dangling
reference to stack memory that no longer exists. Disastrous. The solution is
to never implement sentinels that refer to a memory address on the map struct
itself. */
check_static_begin(tree_map_test_construct)
{
    struct Val push = {};
    CCC_Tree_map map = construct_empty();
    CCC_Entry entry = CCC_tree_map_insert_or_assign(&map, &push.elem);
    check(CCC_tree_map_validate(&map), true);
    check(CCC_entry_insert_error(&entry), false);
    check(CCC_entry_occupied(&entry), false);
    check(CCC_tree_map_count(&map).count, 1);
    check_end();
}

check_static_begin(tree_map_test_construct_from)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 3);
    CCC_Tree_map map = CCC_tree_map_from(
        elem, key, id_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[]){
            {.key = 0, .val = 0},
            {.key = 1, .val = 1},
            {.key = 2, .val = 2},
        });
    check(CCC_tree_map_validate(&map), true);
    check(CCC_tree_map_count(&map).count, 3);
    check_end((void)CCC_tree_map_clear(&map, NULL););
}

check_static_begin(tree_map_test_construct_from_overwrite)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 3);
    CCC_Tree_map map = CCC_tree_map_from(
        elem, key, id_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[]){
            {.key = 0, .val = 0},
            {.key = 1, .val = 1},
            {.key = 1, .val = 2},
        });
    check(CCC_tree_map_validate(&map), true);
    check(CCC_tree_map_count(&map).count, 2);
    struct Val const *const v = CCC_tree_map_reverse_begin(&map);
    check(v != NULL, true);
    check(v->key, 1);
    check(v->val, 2);
    check_end((void)CCC_tree_map_clear(&map, NULL););
}

check_static_begin(tree_map_test_construct_from_fail)
{
    CCC_Tree_map map = CCC_tree_map_from(elem, key, id_order, NULL, NULL, NULL,
                                         (struct Val[]){
                                             {.key = 0, .val = 0},
                                             {.key = 1, .val = 1},
                                             {.key = 2, .val = 2},
                                         });
    check(CCC_tree_map_validate(&map), true);
    check(CCC_tree_map_is_empty(&map), true);
    check_end((void)CCC_tree_map_clear(&map, NULL););
}

int
main()
{
    return check_run(tree_map_test_empty(), tree_map_test_construct(),
                     tree_map_test_construct_from(),
                     tree_map_test_construct_from_overwrite(),
                     tree_map_test_construct_from_fail());
}
