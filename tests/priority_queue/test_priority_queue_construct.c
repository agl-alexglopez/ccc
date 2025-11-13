#include <stdbool.h>
#include <stddef.h>

#include "checkers.h"
#include "priority_queue.h"
#include "priority_queue_utility.h"
#include "types.h"

check_static_begin(priority_queue_test_empty)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    check(CCC_priority_queue_is_empty(&priority_queue), true);
    check_end();
}

int
main()
{
    return check_run(priority_queue_test_empty());
}
