#ifndef CCC_ROMAP_UTIL_H
#define CCC_ROMAP_UTIL_H

#include "checkers.h"
#include "realtime_ordered_map.h"
#include "types.h"

#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_romap_elem elem;
};

ccc_threeway_cmp id_cmp(ccc_key_cmp);

enum check_result insert_shuffled(ccc_realtime_ordered_map *m,
                                  struct val vals[], size_t size,
                                  int larger_prime);
size_t inorder_fill(int vals[], size_t size, ccc_realtime_ordered_map const *m);

#endif /* CCC_ROMAP_UTIL_H */
