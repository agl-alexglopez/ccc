#define TRAITS_USING_NAMESPACE_CCC

#include "map_util.h"
#include "ordered_map.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdio.h>

ccc_threeway_cmp
val_cmp(ccc_key_cmp const cmp)
{
    struct val const *const c = cmp.user_type_rhs;
    int const key = *((int *)cmp.key_lhs);
    return (key > c->val) - (key < c->val);
}

void
map_printer_fn(ccc_user_type const container)
{
    struct val const *const v = container.user_type;
    printf("{id:%d,val:%d}", v->id, v->val);
}

BEGIN_TEST(insert_shuffled, ccc_ordered_map *m, struct val vals[],
           size_t const size, int const larger_prime)
{
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        vals[shuffled_index].val = (int)shuffled_index;
        (void)insert(m, &vals[shuffled_index].elem, &(struct val){});
        CHECK(size(m), i + 1);
        CHECK(validate(m), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(size(m), size);
    END_TEST();
}

/* Iterative inorder traversal to check the heap is sorted. */
size_t
inorder_fill(int vals[], size_t size, ccc_ordered_map *m)
{
    if (size(m) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct val *e = begin(m); e; e = next(m, &e->elem))
    {
        vals[i++] = e->val;
    }
    return i;
}
