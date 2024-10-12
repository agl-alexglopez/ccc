#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC

#include "flat_realtime_ordered_map.h"
#include "frtomap_util.h"
#include "test.h"
#include "traits.h"

#include <stdbool.h>
#include <stddef.h>

BEGIN_STATIC_TEST(frtomap_test_empty)
{
    flat_realtime_ordered_map s
        = frm_init((struct val[3]){}, 3, elem, id, NULL, val_cmp, NULL);
    CHECK(is_empty(&s), true);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(frtomap_test_empty());
}
