#ifndef CCC_OMAP_UTIL_H
#define CCC_OMAP_UTIL_H

#include "checkers.h"
#include "ordered_map.h"
#include "types.h"

#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_omap_elem elem;
};

ccc_threeway_cmp val_cmp(ccc_key_cmp);

enum check_result insert_shuffled(ccc_ordered_map *m, struct val vals[],
                                  size_t size, int larger_prime);
size_t inorder_fill(int vals[], size_t size, ccc_ordered_map *m);

#endif /* CCC_OMAP_UTIL_H */
