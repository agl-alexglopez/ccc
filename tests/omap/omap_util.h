#ifndef CCC_OMAP_UTIL_H
#define CCC_OMAP_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "ordered_map.h"
#include "types.h"

struct val
{
    int key;
    int val;
    ccc_omap_elem elem;
};

/** Use this type to set up a simple bump allocator. The pool of values can
come from any source. Usually since tests are on a smaller scale we can have
the pool be managed with a stack array of vals as the pool source. However,
a heap allocated array of vals or a ccc_buffer would work too. I'm hesitant
to bring the buffer into another container test as a dependency for now. */
struct val_pool
{
    struct val *vals; /* Stack, heap, or data segment. */
    size_t next_free; /* Starts at 0, bumps up by one on each alloc. */
    size_t capacity;  /* Total. Exhausted when next_free == capacity. */
};

/** The bump allocator will point to the val pool as its auxiliary data. It
can only allocate. Freeing is a No Op. Reallocation will kill the program. */
void *val_bump_alloc(void *ptr, size_t size, void *aux);

ccc_threeway_cmp id_cmp(ccc_any_key_cmp);

enum check_result insert_shuffled(ccc_ordered_map *m, struct val vals[],
                                  size_t size, int larger_prime);
size_t inorder_fill(int vals[], size_t size, ccc_ordered_map *m);

#endif /* CCC_OMAP_UTIL_H */
