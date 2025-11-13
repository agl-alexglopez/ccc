#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "ordered_map.h"
#include "ordered_map_util.h"
#include "traits.h"

CHECK_BEGIN_STATIC_FN(ordered_map_test_empty)
{
    CCC_Ordered_map s = CCC_ordered_map_initialize(s, struct Val, elem, key,
                                                   id_order, NULL, NULL);
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(ordered_map_test_empty());
}
