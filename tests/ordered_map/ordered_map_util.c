#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "ordered_map.h"
#include "ordered_map_util.h"
#include "traits.h"
#include "types.h"

CCC_Order
id_order(CCC_Key_comparator_context const order)
{
    struct Val const *const c = order.type_rhs;
    int const key = *((int *)order.key_lhs);
    return (key > c->key) - (key < c->key);
}

CHECK_BEGIN_FN(insert_shuffled, CCC_Ordered_map *m, struct Val vals[],
               size_t const size, int const larger_prime)
{
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        vals[shuffled_index].key = (int)shuffled_index;
        (void)swap_entry(m, &vals[shuffled_index].elem, &(struct Val){}.elem);
        CHECK(count(m).count, i + 1);
        CHECK(validate(m), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(count(m).count, size);
    CHECK_END_FN();
}

/* Iterative inorder traversal to check the heap is sorted. */
size_t
inorder_fill(int vals[], size_t size, CCC_Ordered_map *m)
{
    if (count(m).count != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct Val *e = begin(m); e; e = next(m, &e->elem))
    {
        vals[i++] = e->key;
    }
    return i;
}

void *
val_bump_alloc(CCC_Allocator_context const context)
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
    assert(!"Shouldn't attempt to realloc in bump allocator.");
    return NULL;
}
