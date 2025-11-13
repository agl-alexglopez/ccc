#include <stdbool.h>
#include <stddef.h>

#include "checkers.h"
#include "realtime_ordered_map.h"
#include "realtime_ordered_map_utility.h"

check_static_begin(romap_test_empty)
{
    CCC_Realtime_ordered_map s = CCC_realtime_ordered_map_initialize(
        s, struct Val, elem, key, id_order, NULL, NULL);
    check(CCC_realtime_ordered_map_is_empty(&s), true);
    check_end();
}

int
main()
{
    return check_run(romap_test_empty());
}
