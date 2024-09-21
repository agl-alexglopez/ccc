#include "pq_util.h"
#include "priority_queue.h"
#include "test.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

BEGIN_STATIC_TEST(pq_test_empty)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    CHECK(ccc_pq_empty(&pq), true);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(pq_test_empty());
}
