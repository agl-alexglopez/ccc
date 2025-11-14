#include <stdbool.h>
#include <stddef.h>

#include "bounded_map.h"
#include "bounded_map_utility.h"
#include "checkers.h"

check_static_begin(romap_test_empty)
{
    CCC_Bounded_map s = CCC_bounded_map_initialize(s, struct Val, elem, key,
                                                   id_order, NULL, NULL);
    check(CCC_bounded_map_is_empty(&s), true);
    check_end();
}

int
main()
{
    return check_run(romap_test_empty());
}
