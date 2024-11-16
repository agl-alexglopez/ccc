#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_realtime_ordered_map.h"
#include "fromap_util.h"
#include "traits.h"

#include <stdbool.h>
#include <stddef.h>

CHECK_BEGIN_STATIC_FN(fromap_test_empty)
{
    flat_realtime_ordered_map s
        = frm_init((struct val[3]){}, 3, elem, id, NULL, id_cmp, NULL);
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(fromap_test_empty());
}
