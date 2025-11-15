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

check_begin(insert_shuffled, CCC_Bounded_map *m, struct Val vals[],
            size_t const size, int const larger_prime)
{
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        vals[shuffled_index].key = (int)shuffled_index;
        vals[shuffled_index].val = (int)i;
        (void)CCC_bounded_map_swap_entry(m, &vals[shuffled_index].elem,
                                         &(struct Val){}.elem);
        check(validate(m), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    check(CCC_bounded_map_count(m).count, size);
    check_end();
}

/* Iterative inorder traversal to check the heap is sorted. */
size_t
inorder_fill(int vals[], size_t size, CCC_Bounded_map const *const m)
{
    if (CCC_bounded_map_count(m).count != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct Val *e = begin(m); e != end(m); e = next(m, &e->elem))
    {
        vals[i++] = e->key;
    }
    return i;
}

void *
val_bump_allocate(CCC_Allocator_context const context)
{
    if (!context.input && !context.bytes)
    {
        return NULL;
    }
    if (!context.input)
    {
        assert(context.bytes == sizeof(struct Val)
               && "stack allocator for struct Val only.");
        struct Val_pool *vals = context.context;
        if (vals->next_free >= vals->capacity)
        {
            return NULL;
        }
        return &vals->vals[vals->next_free++];
    }
    if (!context.bytes)
    {
        /* Don't do anything fancy on free, just bump forward so no op here. */
        return NULL;
    }
    assert(!"Shouldn't attempt to reallocate in bump allocator.");
    return NULL;
}
