#ifndef CCC_OMMAP_UTIL_H
#define CCC_OMMAP_UTIL_H

#include <stddef.h>

#include "ordered_multimap.h"
#include "types.h"

struct val
{
    int key;
    int val;
    ccc_ommap_elem elem;
};

/** Use this type to set up a simple bump allocator. The pool of values can
come from any source. Usually since tests are on a smaller scale we can have
the pool be managed with a stack array of vals as the pool source. However,
a heap allocated array of vals or a ccc_buffer would work too. I'm hesitant
to bring the buffer into another container test as a dependency for now. */
struct val_pool
{
    struct val *vals;    /* Stack, heap, or data segment. */
    ptrdiff_t next_free; /* Starts at 0, bumps up by one on each alloc. */
    ptrdiff_t capacity;  /* Total. Exhausted when next_free == capacity. */
};

/** The bump allocator will point to the val pool as its auxiliary data. It
can only allocate. Freeing is a No Op. Reallocation will kill the program. */
void *val_bump_alloc(void *ptr, size_t size, void *aux);

ccc_threeway_cmp id_cmp(ccc_key_cmp);
void val_update(ccc_user_type);

enum check_result insert_shuffled(ccc_ordered_multimap *, struct val[],
                                  ptrdiff_t, int);
ptrdiff_t inorder_fill(int[], ptrdiff_t, ccc_ordered_multimap *);

#endif /* CCC_OMMAP_UTIL_H */
