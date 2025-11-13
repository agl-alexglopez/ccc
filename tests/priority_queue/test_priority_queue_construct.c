#include <stdbool.h>
#include <stddef.h>

#include "checkers.h"
#include "priority_queue.h"
#include "priority_queue_util.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(priority_queue_test_empty)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    CHECK(CCC_priority_queue_is_empty(&priority_queue), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(priority_queue_test_empty());
}
