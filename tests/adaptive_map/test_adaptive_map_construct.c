#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "adaptive_map.h"
#include "adaptive_map_utility.h"
#include "checkers.h"
#include "traits.h"

check_static_begin(adaptive_map_test_empty)
{
    CCC_Adaptive_map s = CCC_adaptive_map_initialize(struct Val, elem, key,
                                                     id_order, NULL, NULL);
    check(is_empty(&s), true);
    check_end();
}

int
main()
{
    return check_run(adaptive_map_test_empty());
}
