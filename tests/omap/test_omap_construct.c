#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "omap_util.h"
#include "ordered_map.h"
#include "traits.h"

#include <stdbool.h>
#include <stddef.h>

CHECK_BEGIN_STATIC_FN(omap_test_empty)
{
    ccc_ordered_map s
        = ccc_om_init(s, struct val, elem, key, NULL, id_cmp, NULL);
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(omap_test_empty());
}
