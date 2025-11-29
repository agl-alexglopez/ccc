#include <stdbool.h>
#include <stddef.h>

#include "checkers.h"
#include "priority_queue.h"
#include "priority_queue_utility.h"
#include "types.h"
#include "utility/stack_allocator.h"

static CCC_Priority_queue
construct_empty(void)
{
    CCC_Priority_queue result = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    return result;
}

check_static_begin(priority_queue_test_empty)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    check(CCC_priority_queue_is_empty(&priority_queue), true);
    check_end();
}

/** If the user constructs a node style priority queue from a helper function,
the priority queue cannot have any self referential fields, such as nil or
sentinel nodes. If the priority queue is initialized on the stack those self
referential fields will become invalidated after the constructing function ends.
This leads to a dangling reference to stack memory that no longer exists.
Disastrous. The solution is to never implement sentinels that refer to a memory
address on the priority queue struct itself. */
check_static_begin(priority_queue_test_construct)
{
    CCC_Priority_queue pq = construct_empty();
    struct Val v = {};
    check(CCC_priority_queue_push(&pq, &v.elem) != NULL, true);
    check(CCC_priority_queue_validate(&pq), true);
    check_end();
}

check_static_begin(priority_queue_test_construct_from)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 3);
    CCC_Priority_queue pq
        = CCC_priority_queue_from(elem, CCC_ORDER_LESSER, val_order,
                                  stack_allocator_allocate, NULL, &allocator,
                                  (struct Val[]){
                                      {.val = 0},
                                      {.val = 1},
                                      {.val = 2},
                                  });
    check(CCC_priority_queue_validate(&pq), true);
    check(CCC_priority_queue_count(&pq).count, 3);
    struct Val const *const v = CCC_priority_queue_front(&pq);
    check(v != NULL, true);
    check(v->val, 0);
    check_end((void)CCC_priority_queue_clear(&pq, NULL););
}

check_static_begin(priority_queue_test_construct_from_fail)
{
    CCC_Priority_queue pq = CCC_priority_queue_from(elem, CCC_ORDER_LESSER,
                                                    val_order, NULL, NULL, NULL,
                                                    (struct Val[]){
                                                        {.val = 0},
                                                        {.val = 1},
                                                        {.val = 2},
                                                    });
    check(CCC_priority_queue_validate(&pq), true);
    check(CCC_priority_queue_is_empty(&pq), true);
    check_end((void)CCC_priority_queue_clear(&pq, NULL););
}

int
main()
{
    return check_run(priority_queue_test_empty(),
                     priority_queue_test_construct(),
                     priority_queue_test_construct_from(),
                     priority_queue_test_construct_from_fail());
}
