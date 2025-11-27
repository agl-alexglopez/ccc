#include <limits.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "priority_queue.h"
#include "priority_queue_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/stack_allocator.h"

CCC_Order
val_order(CCC_Type_comparator_context const order)
{
    struct Val const *const left = order.type_left;
    struct Val const *const right = order.type_right;
    return (left->val > right->val) - (left->val < right->val);
}

void
val_update(CCC_Type_context const u)
{
    struct Val *const old = u.type;
    old->val = *(int *)u.context;
}

check_begin(insert_shuffled, CCC_Priority_queue *queue, size_t const size,
            int const larger_prime)
{
    check(queue->allocate != NULL, true);
    /* Math magic ahead so that we iterate over every index
       eventually but in a shuffled order. Not necessarily
       random but a repeatable sequence that makes it
       easier to debug if something goes wrong. Think
       of the prime number as a random seed, kind of. */
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        (void)push(queue, &(struct Val){.val = shuffled_index}.elem);
        check(count(queue).count, i + 1);
        check(validate(queue), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    check(count(queue).count, size);
    check_end();
}

/* Iterative inorder traversal to check the heap is sorted. */
check_begin(private_inorder_fill, struct Stack_allocator *const allocator,
            int vals[const], size_t const size, CCC_Priority_queue *const queue)
{
    check(count(queue).count, size);
    size_t i = 0;
    CCC_Priority_queue copy = CCC_priority_queue_initialize(
        struct Val, elem, CCC_priority_queue_order(queue), val_order,
        stack_allocator_allocate, allocator);
    int prev_val = queue->order == CCC_ORDER_LESSER ? INT_MIN : INT_MAX;
    while (!is_empty(queue))
    {
        struct Val *const front = front(queue);
        if (queue->order == CCC_ORDER_LESSER)
        {
            check(front->val > prev_val, true);
        }
        else
        {
            check(front->val < prev_val, true);
        }
        (void)pop(queue);
        check(validate(queue), true);
        check(validate(&copy), true);
        vals[i++] = front->val;
        (void)push(&copy, &front->elem);
    }
    check(queue->context != NULL, true);
    stack_allocator_reset(queue->context);
    i = 0;
    while (!is_empty(&copy))
    {
        struct Val *v = front(&copy);
        check(v->val, vals[i++]);
        (void)pop(&copy);
        (void)push(queue, &v->elem);
        check(validate(queue), true);
        check(validate(&copy), true);
    }
    check_end();
}
