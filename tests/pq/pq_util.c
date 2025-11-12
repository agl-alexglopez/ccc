#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "pq_util.h"
#include "priority_queue.h"
#include "traits.h"
#include "types.h"

CCC_threeway_cmp
val_cmp(CCC_any_type_cmp const cmp)
{
    struct val const *const lhs = cmp.any_type_lhs;
    struct val const *const rhs = cmp.any_type_rhs;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

void
val_update(CCC_any_type const u)
{
    struct val *const old = u.any_type;
    old->val = *(int *)u.aux;
}

CHECK_BEGIN_FN(insert_shuffled, CCC_priority_queue *ppq, struct val vals[],
               size_t const size, int const larger_prime)
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
        (void)push(ppq, &vals[shuffled_index].elem);
        CHECK(count(ppq).count, i + 1);
        CHECK(validate(ppq), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(count(ppq).count, size);
    CHECK_END_FN();
}

/* Iterative inorder traversal to check the heap is sorted. */
CHECK_BEGIN_FN(inorder_fill, int vals[], size_t size, CCC_priority_queue *ppq)
{
    CHECK(count(ppq).count, size);
    size_t i = 0;
    CCC_priority_queue copy
        = CCC_pq_init(struct val, elem, CCC_pq_order(ppq), val_cmp, NULL, NULL);
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
