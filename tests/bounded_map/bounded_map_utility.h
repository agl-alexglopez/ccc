#ifndef CCC_ROMAP_UTIL_H
#define CCC_ROMAP_UTIL_H

#include <stddef.h>

#include "bounded_map.h"
#include "checkers.h"
#include "types.h"

struct Val
{
    int key;
    int val;
    CCC_Bounded_map_node elem;
};

CCC_Order id_order(CCC_Key_comparator_context);

/** Runs a prime shuffle over the map using size as N and larger_prime as the
larger prime to run the shuffle. Expects the map to have allocation permission.
Use a heap or stack allocator. */
enum Check_result insert_shuffled(CCC_Bounded_map *m, size_t size,
                                  int larger_prime);
enum Check_result inorder_fill(int vals[], size_t size,
                               CCC_Bounded_map const *m);

#endif /* CCC_ROMAP_UTIL_H */
