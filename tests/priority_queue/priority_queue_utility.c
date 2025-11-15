#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "priority_queue.h"
#include "priority_queue_utility.h"
#include "traits.h"
#include "types.h"

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

check_begin(insert_shuffled, CCC_Priority_queue *ppriority_queue,
            struct Val vals[], size_t const size, int const larger_prime)
{
    /* Math magic ahead so that we iterate over every index
       eventually but in a shuffled order. Not necessarily
       random but a repeatable sequence that makes it
       easier to debug if something goes wrong. Think
       of the prime number as a random seed, kind of. */
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        vals[shuffled_index].val = (int)shuffled_index;
        (void)push(ppriority_queue, &vals[shuffled_index].elem);
        check(count(ppriority_queue).count, i + 1);
        check(validate(ppriority_queue), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    check(count(ppriority_queue).count, size);
    check_end();
}

/* Iterative inorder traversal to check the heap is sorted. */
check_begin(inorder_fill, int vals[], size_t size,
            CCC_Priority_queue *ppriority_queue)
{
    check(count(ppriority_queue).count, size);
    size_t i = 0;
    CCC_Priority_queue copy = CCC_priority_queue_initialize(
        struct Val, elem, CCC_priority_queue_order(ppriority_queue), val_order,
        NULL, NULL);
    while (!is_empty(ppriority_queue))
    {
        struct Val *const front = front(ppriority_queue);
        (void)pop(ppriority_queue);
        check(validate(ppriority_queue), true);
        check(validate(&copy), true);
        vals[i++] = front->val;
        (void)push(&copy, &front->elem);
    }
    i = 0;
    while (!is_empty(&copy))
    {
        struct Val *v = front(&copy);
        check(v->val, vals[i++]);
        (void)pop(&copy);
        (void)push(ppriority_queue, &v->elem);
        check(validate(ppriority_queue), true);
        check(validate(&copy), true);
    }
    check_end();
}
