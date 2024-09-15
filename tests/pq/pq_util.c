#include "pq_util.h"

ccc_threeway_cmp
val_cmp(ccc_cmp const cmp)
{
    struct val const *const lhs = cmp.container_a;
    struct val const *const rhs = cmp.container_b;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

void
val_update(ccc_update const u)
{
    struct val *const old = u.container;
    old->val = *(int *)u.aux;
}

enum test_result
insert_shuffled(ccc_priority_queue *ppq, struct val vals[], size_t const size,
                int const larger_prime)
{
    /* Math magic ahead so that we iterate over every index
       eventually but in a shuffled order. Not necessarily
       randome but a repeatable sequence that makes it
       easier to debug if something goes wrong. Think
       of the prime number as a random seed, kind of. */
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        vals[shuffled_index].val = (int)shuffled_index;
        ccc_pq_push(ppq, &vals[shuffled_index].elem);
        CHECK(ccc_pq_size(ppq), i + 1);
        CHECK(ccc_pq_validate(ppq), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_pq_size(ppq), size);
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
enum test_result
inorder_fill(int vals[], size_t size, ccc_priority_queue *ppq)
{
    if (ccc_pq_size(ppq) != size)
    {
        return FAIL;
    }
    size_t i = 0;
    ccc_priority_queue copy
        = CCC_PQ_INIT(struct val, elem, ccc_pq_order(ppq), NULL, val_cmp, NULL);
    while (!ccc_pq_empty(ppq))
    {
        struct val *const front = ccc_pq_front(ppq);
        ccc_pq_pop(ppq);
        CHECK(ccc_pq_validate(ppq), true);
        CHECK(ccc_pq_validate(&copy), true);
        vals[i++] = front->val;
        ccc_pq_push(&copy, &front->elem);
    }
    i = 0;
    while (!ccc_pq_empty(&copy))
    {
        struct val *v = ccc_pq_front(&copy);
        CHECK(v->val, vals[i++]);
        ccc_pq_pop(&copy);
        ccc_pq_push(ppq, &v->elem);
        CHECK(ccc_pq_validate(ppq), true);
        CHECK(ccc_pq_validate(&copy), true);
    }
    return PASS;
}
