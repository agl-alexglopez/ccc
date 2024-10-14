#define TRAITS_USING_NAMESPACE_CCC

#include "omm_util.h"
#include "ordered_multimap.h"
#include "test.h"
#include "traits.h"

#include <stdbool.h>
#include <stddef.h>

BEGIN_STATIC_TEST(omm_test_empty)
{
    ccc_ordered_multimap pq
        = ccc_omm_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    CHECK(is_empty(&pq), true);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(omm_test_empty());
}
