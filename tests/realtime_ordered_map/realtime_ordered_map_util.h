#ifndef CCC_ROMAP_UTIL_H
#define CCC_ROMAP_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "realtime_ordered_map.h"
#include "types.h"

struct Val
{
    int key;
    int val;
    CCC_Realtime_ordered_map_node elem;
};

/** Use this type to set up a simple bump allocator. The pool of values can
come from any source. Usually since tests are on a smaller scale we can have
the pool be managed with a stack array of vals as the pool source. However,
a heap allocated array of vals or a CCC_Buffer would work too. I'm hesitant
to bring the Buffer into another container test as a dependency for now. */
struct Val_pool
{
    struct Val *vals; /* Stack, heap, or data segment. */
    size_t next_free; /* Starts at 0, bumps up by one on each alloc. */
    size_t capacity;  /* Total. Exhausted when next_free == capacity. */
};

/** The bump allocator will point to the val pool as its context data. It
can only allocate. Freeing is a No Op. Reallocation will kill the program. */
void *val_bump_allocate(CCC_Allocator_context);

CCC_Order id_order(CCC_Key_comparator_context);

enum Check_result insert_shuffled(CCC_Realtime_ordered_map *m,
                                  struct Val vals[], size_t size,
                                  int larger_prime);
size_t inorder_fill(int vals[], size_t size, CCC_Realtime_ordered_map const *m);

#endif /* CCC_ROMAP_UTIL_H */
