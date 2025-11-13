#include <stdbool.h>
#include <stddef.h>

#include "checkers.h"
#include "realtime_ordered_map.h"
#include "romap_util.h"

CHECK_BEGIN_STATIC_FN(romap_test_empty)
{
    CCC_Realtime_ordered_map s = CCC_realtime_ordered_map_initialize(
        s, struct val, elem, key, id_cmp, NULL, NULL);
    CHECK(CCC_realtime_ordered_map_is_empty(&s), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(romap_test_empty());
}
