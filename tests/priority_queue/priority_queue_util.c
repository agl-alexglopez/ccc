#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "priority_queue.h"
#include "priority_queue_util.h"
#include "traits.h"
#include "types.h"

CCC_Order
val_cmp(CCC_Type_comparator_context const cmp)
{
    struct val const *const lhs = cmp.any_type_lhs;
    struct val const *const rhs = cmp.any_type_rhs;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

void
val_update(CCC_Type_context const u)
{
    struct val *const old = u.any_type;
    old->val = *(int *)u.context;
}

CHECK_BEGIN_FN(insert_shuffled, CCC_Priority_queue *ppriority_queue,
               struct val vals[], size_t const size, int const larger_prime)
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
        CHECK(count(ppriority_queue).count, i + 1);
        CHECK(validate(ppriority_queue), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(count(ppriority_queue).count, size);
    CHECK_END_FN();
}

/* Iterative inorder traversal to check the heap is sorted. */
CHECK_BEGIN_FN(inorder_fill, int vals[], size_t size,
               CCC_Priority_queue *ppriority_queue)
{
    CHECK(count(ppriority_queue).count, size);
    size_t i = 0;
    CCC_Priority_queue copy = CCC_priority_queue_initialize(
        struct val, elem, CCC_priority_queue_order(ppriority_queue), val_cmp,
        NULL, NULL);
    while (!is_empty(ppriority_queue))
    {
        struct val *const front = front(ppriority_queue);
        (void)pop(ppriority_queue);
        CHECK(validate(ppriority_queue), true);
        CHECK(validate(&copy), true);
        vals[i++] = front->val;
        (void)push(&copy, &front->elem);
    }
    i = 0;
    while (!is_empty(&copy))
    {
        struct val *v = front(&copy);
        CHECK(v->val, vals[i++]);
        (void)pop(&copy);
        (void)push(ppriority_queue, &v->elem);
        CHECK(validate(ppriority_queue), true);
        CHECK(validate(&copy), true);
    }
    CHECK_END_FN();
}
