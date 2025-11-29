#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "bounded_map.h"
#include "bounded_map_utility.h"
#include "checkers.h"
#include "traits.h"
#include "types.h"

CCC_Order
id_order(CCC_Key_comparator_context const order)
{
    struct Val const *const c = order.type_right;
    int const key = *((int *)order.key_left);
    return (key > c->key) - (key < c->key);
}

check_begin(insert_shuffled, CCC_Bounded_map *m, size_t const size,
            int const larger_prime)
{
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        (void)CCC_bounded_map_swap_entry(m,
                                         &(struct Val){
                                             .key = (int)shuffled_index,
                                             .val = (int)i,
                                         }
                                              .elem,
                                         &(struct Val){}.elem);
        check(validate(m), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    check(CCC_bounded_map_count(m).count, size);
    check_end();
}

/* Iterative inorder traversal to check the heap is sorted. */
check_begin(inorder_fill, int vals[], size_t size,
            CCC_Bounded_map const *const m)
{
    check(CCC_bounded_map_count(m).count, size);
    struct Val const *prev = begin(m);
    struct Val const *e = end(m);
    if (prev)
    {
        vals[0] = prev->key;
        e = next(m, &prev->elem);
    }
    size_t i = 1;
    while (e != end(m))
    {
        check(prev->key < e->key, true);
        vals[i++] = e->key;
        prev = e;
        e = next(m, &e->elem);
    }
    check_end();
}
