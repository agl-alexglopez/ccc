#define TRAITS_USING_NAMESPACE_CCC

#include "depq_util.h"
#include "double_ended_priority_queue.h"
#include "test.h"
#include "traits.h"

#include <stdbool.h>
#include <stddef.h>

BEGIN_STATIC_TEST(depq_test_empty)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    CHECK(empty(&pq), true);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(depq_test_empty());
}
