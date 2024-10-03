#define TRAITS_USING_NAMESPACE_CCC

#include "fmap_util.h"
#include "flat_ordered_map.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdio.h>

ccc_threeway_cmp
val_cmp(ccc_key_cmp const *const cmp)
{
    struct val const *const c = cmp->container;
    int const key = *((int *)cmp->key);
    return (key > c->id) - (key < c->id);
}

void
map_printer_fn(void const *const container)
{
    struct val const *const v = container;
    printf("{id:%d,val:%d}", v->id, v->val);
}

BEGIN_TEST(insert_shuffled, ccc_flat_ordered_map *m, size_t const size,
           int const larger_prime)
{
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        (void)insert(
            m, &(struct val){.id = (int)shuffled_index, .val = (int)i}.elem,
            &(struct val){}.elem);
        CHECK(validate(m), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(size(m), size);
    END_TEST();
}

/* Iterative inorder traversal to check the heap is sorted. */
size_t
inorder_fill(int vals[], size_t size, ccc_flat_ordered_map const *const m)
{
    if (size(m) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct val *e = begin(m); e != end(m); e = next(m, &e->elem))
    {
        vals[i++] = e->id;
    }
    return i;
}
