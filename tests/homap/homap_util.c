#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_ordered_map.h"
#include "homap_util.h"
#include "traits.h"
#include "types.h"

CCC_threeway_cmp
id_cmp(CCC_any_key_cmp const cmp)
{
    struct val const *const c = cmp.any_type_rhs;
    int const key = *((int *)cmp.any_key_lhs);
    return (key > c->id) - (key < c->id);
}

CHECK_BEGIN_FN(insert_shuffled, CCC_handle_ordered_map *m, size_t const size,
               int const larger_prime)
{
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        (void)insert_or_assign(
            m, &(struct val){.id = (int)shuffled_index, .val = (int)i});
        CHECK(validate(m), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(count(m).count, size);
    CHECK_END_FN();
}

/* Iterative inorder traversal to check the heap is sorted. */
size_t
inorder_fill(int vals[], size_t size, CCC_handle_ordered_map const *const m)
{
    if (count(m).count != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct val *e = begin(m); e != end(m); e = next(m, e))
    {
        vals[i++] = e->id;
    }
    return i;
}
