#ifndef CCC_FROMAP_UTIL_H
#define CCC_FROMAP_UTIL_H

#include "checkers.h"
#include "flat_realtime_ordered_map.h"
#include "types.h"

#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_fromap_elem elem;
};

ccc_threeway_cmp id_cmp(ccc_key_cmp);

enum check_result insert_shuffled(ccc_flat_realtime_ordered_map *m, size_t size,
                                  int larger_prime);
size_t inorder_fill(int vals[], size_t size,
                    ccc_flat_realtime_ordered_map const *m);

#endif /* CCC_FROMAP_UTIL_H */
