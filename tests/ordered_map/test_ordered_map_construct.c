#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "ordered_map.h"
#include "ordered_map_utility.h"
#include "traits.h"

check_static_begin(ordered_map_test_empty)
{
    CCC_Ordered_map s = CCC_ordered_map_initialize(s, struct Val, elem, key,
                                                   id_order, NULL, NULL);
    check(is_empty(&s), true);
    check_end();
}

int
main()
{
    return check_run(ordered_map_test_empty());
}
