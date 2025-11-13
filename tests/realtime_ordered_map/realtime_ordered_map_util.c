#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "realtime_ordered_map.h"
#include "romap_util.h"
#include "traits.h"
#include "types.h"

CCC_Order
id_cmp(CCC_Key_comparator_context const cmp)
{
    struct val const *const c = cmp.any_type_rhs;
    int const key = *((int *)cmp.any_key_lhs);
    return (key > c->key) - (key < c->key);
}

CHECK_BEGIN_FN(insert_shuffled, CCC_Realtime_ordered_map *m, struct val vals[],
               size_t const size, int const larger_prime)
{
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        vals[shuffled_index].key = (int)shuffled_index;
        vals[shuffled_index].val = (int)i;
        (void)CCC_realtime_ordered_map_swap_entry(m, &vals[shuffled_index].elem,
                                                  &(struct val){}.elem);
        CHECK(validate(m), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(CCC_realtime_ordered_map_count(m).count, size);
    CHECK_END_FN();
}

/* Iterative inorder traversal to check the heap is sorted. */
size_t
inorder_fill(int vals[], size_t size, CCC_Realtime_ordered_map const *const m)
{
    if (CCC_realtime_ordered_map_count(m).count != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct val *e = begin(m); e != end(m); e = next(m, &e->elem))
    {
        vals[i++] = e->key;
    }
    return i;
}

void *
val_bump_alloc(void *const ptr, size_t const size, void *const context)
{
    if (!ptr && !size)
    {
        return NULL;
    }
    if (!ptr)
    {
        assert(size == sizeof(struct val)
               && "stack allocator for struct val only.");
        struct val_pool *vals = context;
        if (vals->next_free >= vals->capacity)
        {
            return NULL;
        }
        return &vals->vals[vals->next_free++];
    }
    if (!size)
    {
        /* Don't do anything fancy on free, just bump forward so no op here. */
        return NULL;
    }
    assert(!"Shouldn't attempt to realloc in bump allocator.");
    return NULL;
}
