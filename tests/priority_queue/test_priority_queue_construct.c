#include <stdbool.h>
#include <stddef.h>

#include "checkers.h"
#include "pq_util.h"
#include "priority_queue.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(pq_test_empty)
{
    CCC_priority_queue pq
        = CCC_pq_initialize(struct val, elem, CCC_LES, val_cmp, NULL, NULL);
    CHECK(CCC_pq_is_empty(&pq), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(pq_test_empty());
}
