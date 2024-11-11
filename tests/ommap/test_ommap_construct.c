#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "ommap_util.h"
#include "ordered_multimap.h"
#include "traits.h"

#include <stdbool.h>
#include <stddef.h>

CHECK_BEGIN_STATIC_FN(ommap_test_empty)
{
    ccc_ordered_multimap pq
        = ccc_omm_init(pq, struct val, elem, val, NULL, val_cmp, NULL);
    CHECK(is_empty(&pq), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(ommap_test_empty());
}
