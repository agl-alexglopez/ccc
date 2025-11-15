#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_bounded_map.h"
#include "handle_bounded_map_utility.h"
#include "traits.h"
#include "types.h"

CCC_Order
id_order(CCC_Key_comparator_context const order)
{
    struct Val const *const c = order.type_right;
    int const key = *((int *)order.key_left);
    return (key > c->id) - (key < c->id);
}

check_begin(insert_shuffled, CCC_Handle_bounded_map *m, size_t const size,
            int const larger_prime)
{
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        (void)insert_or_assign(
            m, &(struct Val){.id = (int)shuffled_index, .val = (int)i});
        check(validate(m), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    check(count(m).count, size);
    check_end();
}

/* Iterative inorder traversal to check the heap is sorted. */
size_t
inorder_fill(int vals[], size_t size, CCC_Handle_bounded_map const *const m)
{
    if (CCC_handle_bounded_map_count(m).count != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct Val *e = begin(m); e != end(m); e = next(m, e))
    {
        vals[i++] = e->id;
    }
    return i;
}
