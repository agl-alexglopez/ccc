#define TRAITS_USING_NAMESPACE_CCC

#include "map_util.h"
#include "ordered_map.h"
#include "test.h"
#include "traits.h"

#include <stdbool.h>
#include <stddef.h>

BEGIN_STATIC_TEST(map_test_empty)
{
    ccc_ordered_map s
        = ccc_om_init(s, struct val, elem, val, NULL, val_cmp, NULL);
    CHECK(is_empty(&s), true);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(map_test_empty());
}
