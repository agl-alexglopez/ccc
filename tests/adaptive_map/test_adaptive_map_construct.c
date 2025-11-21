#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "adaptive_map.h"
#include "adaptive_map_utility.h"
#include "checkers.h"
#include "traits.h"
#include "types.h"

static CCC_Adaptive_map
construct_empty(void)
{
    CCC_Adaptive_map map = CCC_adaptive_map_initialize(struct Val, elem, key,
                                                       id_order, NULL, NULL);
    return map;
}

check_static_begin(adaptive_map_test_empty)
{
    CCC_Adaptive_map s = CCC_adaptive_map_initialize(struct Val, elem, key,
                                                     id_order, NULL, NULL);
    check(is_empty(&s), true);
    check_end();
}

/** If the user constructs a node style map from a helper function, the map
cannot have any self referential fields, such as nil or sentinel nodes. If
the map is initialized on the stack those self referential fields will become
invalidated after the constructing function ends. This leads to a dangling
reference to stack memory that no longer exists. Disastrous. The solution is
to never implement sentinels that refer to a memory address on the map struct
itself. */
check_static_begin(adaptive_map_test_construct)
{
    struct Val push = {};
    CCC_Adaptive_map map = construct_empty();
    CCC_Entry entry = CCC_adaptive_map_insert_or_assign(&map, &push.elem);
    check(CCC_adaptive_map_validate(&map), true);
    check(CCC_entry_insert_error(&entry), false);
    check(CCC_entry_occupied(&entry), false);
    check(CCC_adaptive_map_count(&map).count, 1);
    check_end();
}
int
main()
{
    return check_run(adaptive_map_test_empty(), adaptive_map_test_construct());
}
