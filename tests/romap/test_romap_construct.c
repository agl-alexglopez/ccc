#include "checkers.h"
#include "realtime_ordered_map.h"
#include "romap_util.h"

#include <stdbool.h>
#include <stddef.h>

CHECK_BEGIN_STATIC_FN(romap_test_empty)
{
    ccc_realtime_ordered_map s
        = ccc_rom_init(s, struct val, elem, key, id_cmp, NULL, NULL);
    CHECK(ccc_rom_is_empty(&s), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(romap_test_empty());
}
