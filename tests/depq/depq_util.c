#include "depq_util.h"
#include "test.h"

#include <stdio.h>

ccc_threeway_cmp
val_cmp(ccc_key_cmp const cmp)
{
    struct val const *const c = cmp.container;
    int key = *((int *)cmp.key);
    return (key > c->val) - (key < c->val);
}

void
val_update(ccc_update const u)
{
    struct val *old = u.container;
    old->val = *(int *)u.aux;
}

void
depq_printer_fn(void const *const e)
{
    struct val const *const v = e;
    printf("{id:%d,val:%d}", v->id, v->val);
}

enum test_result
insert_shuffled(ccc_double_ended_priority_queue *pq, struct val vals[],
                size_t const size, int const larger_prime)
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
        ccc_depq_push(pq, &vals[shuffled_index].elem);
        CHECK(ccc_depq_size(pq), i + 1, "%zu");
        CHECK(ccc_depq_validate(pq), true, "%d");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_depq_size(pq), size, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
size_t
inorder_fill(int vals[], size_t size, ccc_double_ended_priority_queue *pq)
{
    if (ccc_depq_size(pq) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct val *e = ccc_depq_rbegin(pq); e != ccc_depq_rend(pq);
         e = ccc_depq_rnext(pq, &e->elem))
    {
        vals[i++] = e->val;
    }
    return i;
}
