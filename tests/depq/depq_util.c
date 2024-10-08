#define TRAITS_USING_NAMESPACE_CCC

#include "depq_util.h"
#include "double_ended_priority_queue.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdio.h>

ccc_threeway_cmp
val_cmp(ccc_key_cmp const cmp)
{
    struct val const *const c = cmp.user_type;
    int key = *((int *)cmp.key);
    return (key > c->val) - (key < c->val);
}

void
val_update(ccc_user_type_mut const u)
{
    struct val *old = u.user_type;
    old->val = *(int *)u.aux;
}

void
depq_printer_fn(ccc_user_type const e)
{
    struct val const *const v = e.user_type;
    printf("{id:%d,val:%d}", v->id, v->val);
}

BEGIN_TEST(insert_shuffled, ccc_double_ended_priority_queue *pq,
           struct val vals[], size_t const size, int const larger_prime)
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
        push(pq, &vals[shuffled_index].elem);
        CHECK(validate(pq), true);
        CHECK(size(pq), i + 1);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(size(pq), size);
    END_TEST();
}

/* Iterative inorder traversal to check the heap is sorted. */
size_t
inorder_fill(int vals[], size_t size, ccc_double_ended_priority_queue *pq)
{
    if (size(pq) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct val *e = rbegin(pq); e != rend(pq); e = rnext(pq, &e->elem))
    {
        vals[i++] = e->val;
    }
    return i;
}
