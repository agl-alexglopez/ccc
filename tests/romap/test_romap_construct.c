#include "realtime_ordered_map.h"
#include "romap_util.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>

BEGIN_STATIC_TEST(romap_test_empty)
{
    ccc_realtime_ordered_map s
        = ccc_rom_init(struct val, elem, val, s, NULL, val_cmp, NULL);
    CHECK(ccc_rom_is_empty(&s), true);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(romap_test_empty());
}
