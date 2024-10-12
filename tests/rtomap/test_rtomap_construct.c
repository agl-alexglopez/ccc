#include "realtime_ordered_map.h"
#include "rtomap_util.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>

BEGIN_STATIC_TEST(rtomap_test_empty)
{
    ccc_realtime_ordered_map s
        = ccc_rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    CHECK(ccc_rom_is_empty(&s), true);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(rtomap_test_empty());
}
