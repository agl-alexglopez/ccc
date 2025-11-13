#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "omap_util.h"
#include "ordered_map.h"
#include "traits.h"

CHECK_BEGIN_STATIC_FN(omap_test_empty)
{
    CCC_ordered_map s
        = CCC_om_initialize(s, struct val, elem, key, id_cmp, NULL, NULL);
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(omap_test_empty());
}
