#define TRAITS_USING_NAMESPACE_CCC

#include "ommap_util.h"
#include "ordered_multimap.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdio.h>

ccc_threeway_cmp
val_cmp(ccc_key_cmp const cmp)
{
    struct val const *const c = cmp.user_type_rhs;
    int key = *((int *)cmp.key_lhs);
    return (key > c->val) - (key < c->val);
}

void
val_update(ccc_user_type_mut const u)
{
    struct val *old = u.user_type;
    old->val = *(int *)u.aux;
}

BEGIN_TEST(insert_shuffled, ccc_ordered_multimap *pq, struct val vals[],
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
        CHECK(unwrap(insert_r(pq, &vals[shuffled_index].elem)) != NULL, true);
        CHECK(validate(pq), true);
        CHECK(size(pq), i + 1);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(size(pq), size);
    END_TEST();
}

/* Iterative inorder traversal to check the heap is sorted. */
size_t
inorder_fill(int vals[], size_t size, ccc_ordered_multimap *pq)
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
