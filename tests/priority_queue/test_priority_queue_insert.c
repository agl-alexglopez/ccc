#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "priority_queue.h"
#include "priority_queue_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/stack_allocator.h"

enum : int
{
    STANDARD_CAP = 50,
};

check_static_begin(priority_queue_test_insert_one)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    struct Val single;
    single.val = 0;
    check(push(&priority_queue, &single.elem) != NULL, true);
    check(CCC_priority_queue_is_empty(&priority_queue), false);
    check_end();
}

check_static_begin(priority_queue_test_insert_three)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 3);
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
        &allocator);
    for (int i = 0; i < 3; ++i)
    {
        check(push(&priority_queue, &(struct Val){.val = i}.elem) != NULL,
              true);
        check(validate(&priority_queue), true);
        check(CCC_priority_queue_count(&priority_queue).count, (size_t)i + 1);
    }
    check(CCC_priority_queue_count(&priority_queue).count, (size_t)3);
    struct Val const *const front = CCC_priority_queue_front(&priority_queue);
    check(front != NULL, true);
    check(front->val, 0);
    check_end();
}

check_static_begin(priority_queue_test_insert_three_dups)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 3);
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
        &allocator);
    for (int i = 0; i < 3; ++i)
    {
        check(push(&priority_queue, &(struct Val){.val = 0}.elem) != NULL,
              true);
        check(validate(&priority_queue), true);
        check(CCC_priority_queue_count(&priority_queue).count, (size_t)i + 1);
    }
    check(CCC_priority_queue_count(&priority_queue).count, (size_t)3);
    struct Val const *const front = CCC_priority_queue_front(&priority_queue);
    check(front != NULL, true);
    check(front->val, 0);
    check_end();
}

check_static_begin(priority_queue_test_insert_shuffle)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, STANDARD_CAP);
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, stack_allocator_allocate,
        &allocator);
    int const prime = 53;
    check(insert_shuffled(&priority_queue, STANDARD_CAP, prime), CHECK_PASS);
    struct Val const *min = front(&priority_queue);
    check(min->val, 0);
    check(check_inorder_fill(&priority_queue, STANDARD_CAP), CHECK_PASS);
    check_end();
}

check_static_begin(priority_queue_test_read_max_min)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    struct Val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        check(push(&priority_queue, &vals[i].elem) != NULL, true);
        check(validate(&priority_queue), true);
        check(CCC_priority_queue_count(&priority_queue).count, (size_t)i + 1);
    }
    check(CCC_priority_queue_count(&priority_queue).count, (size_t)10);
    struct Val const *min = front(&priority_queue);
    check(min->val, 0);
    check_end();
}

int
main()
{
    return check_run(priority_queue_test_insert_one(),
                     priority_queue_test_insert_three(),
                     priority_queue_test_insert_three_dups(),
                     priority_queue_test_insert_shuffle(),
                     priority_queue_test_read_max_min());
}
