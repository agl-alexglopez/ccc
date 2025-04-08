#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define TRAITS_USING_NAMESPACE_CCC
#include "checkers.h"
#include "ommap_util.h"
#include "ordered_multimap.h"
#include "traits.h"
#include "types.h"

ccc_threeway_cmp
id_cmp(ccc_key_cmp const cmp)
{
    struct val const *const c = cmp.any_type_rhs;
    int key = *((int *)cmp.key_lhs);
    return (key > c->key) - (key < c->key);
}

void
val_update(ccc_any_type const u)
{
    struct val *old = u.any_type;
    old->key = *(int *)u.aux;
}

CHECK_BEGIN_FN(insert_shuffled, ccc_ordered_multimap *pq, struct val vals[],
               size_t const size, int const larger_prime)
{
    /* Math magic ahead so that we iterate over every index
       eventually but in a shuffled order. Not necessarily
       random but a repeatable sequence that makes it
       easier to debug if something goes wrong. Think
       of the prime number as a random seed, kind of. */
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        vals[shuffled_index].key = (int)shuffled_index;
        CHECK(unwrap(swap_entry_r(pq, &vals[shuffled_index].elem)) != NULL,
              true);
        CHECK(validate(pq), true);
        CHECK(size(pq).count, i + 1);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(size(pq).count, size);
    CHECK_END_FN();
}

/* Iterative inorder traversal to check the heap is sorted. */
size_t
inorder_fill(int vals[], size_t size, ccc_ordered_multimap *pq)
{
    if (size(pq).count != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct val *e = rbegin(pq); e != rend(pq); e = rnext(pq, &e->elem))
    {
        vals[i++] = e->key;
    }
    return i;
}

void *
val_bump_alloc(void *const ptr, size_t const size, void *const aux)
{
    if (!ptr && !size)
    {
        return NULL;
    }
    if (!ptr)
    {
        assert(size == sizeof(struct val)
               && "stack allocator for struct val only.");
        struct val_pool *vals = aux;
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
