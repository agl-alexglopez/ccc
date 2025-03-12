#define TRAITS_USING_NAMESPACE_CCC

#include "pq_util.h"
#include "checkers.h"
#include "priority_queue.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>

ccc_threeway_cmp
val_cmp(ccc_cmp const cmp)
{
    struct val const *const lhs = cmp.user_type_lhs;
    struct val const *const rhs = cmp.user_type_rhs;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

void
val_update(ccc_user_type const u)
{
    struct val *const old = u.user_type;
    old->val = *(int *)u.aux;
}

CHECK_BEGIN_FN(insert_shuffled, ccc_priority_queue *ppq, struct val vals[],
               ptrdiff_t const size, int const larger_prime)
{
    /* Math magic ahead so that we iterate over every index
       eventually but in a shuffled order. Not necessarily
       random but a repeatable sequence that makes it
       easier to debug if something goes wrong. Think
       of the prime number as a random seed, kind of. */
    ptrdiff_t shuffled_index = larger_prime % size;
    for (ptrdiff_t i = 0; i < size; ++i)
    {
        vals[shuffled_index].val = (int)shuffled_index;
        (void)push(ppq, &vals[shuffled_index].elem);
        CHECK(size(ppq), i + 1);
        CHECK(validate(ppq), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(size(ppq), size);
    CHECK_END_FN();
}

/* Iterative inorder traversal to check the heap is sorted. */
CHECK_BEGIN_FN(inorder_fill, int vals[], ptrdiff_t size,
               ccc_priority_queue *ppq)
{
    CHECK(size(ppq), size);
    ptrdiff_t i = 0;
    ccc_priority_queue copy
        = ccc_pq_init(struct val, elem, ccc_pq_order(ppq), val_cmp, NULL, NULL);
    while (!is_empty(ppq))
    {
        struct val *const front = front(ppq);
        (void)pop(ppq);
        CHECK(validate(ppq), true);
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
        (void)push(ppq, &v->elem);
        CHECK(validate(ppq), true);
        CHECK(validate(&copy), true);
    }
    CHECK_END_FN();
}
