#include "set_util.h"
#include "test.h"

#include <stdio.h>

ccc_threeway_cmp
val_cmp(ccc_key_cmp const cmp)
{
    struct val const *const c = cmp.container;
    int const key = *((int *)cmp.key);
    return (key > c->val) - (key < c->val);
}

void
set_printer_fn(void const *const container)
{
    struct val const *const v = container;
    printf("{id:%d,val:%d}", v->id, v->val);
}

enum test_result
insert_shuffled(ccc_set *s, struct val vals[], size_t const size,
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
        ccc_s_insert(s, &vals[shuffled_index].elem);
        CHECK(ccc_s_size(s), i + 1, "%zu");
        CHECK(ccc_s_validate(s), true, "%d");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_s_size(s), size, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
size_t
inorder_fill(int vals[], size_t size, ccc_set *s)
{
    if (ccc_s_size(s) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct val *e = ccc_s_begin(s); e; e = ccc_s_next(s, &e->elem))
    {
        vals[i++] = e->val;
    }
    return i;
}
