#include "checkers.h"
#include "pq_util.h"
#include "priority_queue.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

CHECK_BEGIN_STATIC_FN(pq_test_empty)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, NULL, NULL);
    CHECK(ccc_pq_is_empty(&pq), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(pq_test_empty());
}
