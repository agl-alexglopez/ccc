#ifndef CCC_RTOMAP_UTIL_H
#define CCC_RTOMAP_UTIL_H

#include "realtime_ordered_map.h"
#include "test.h"
#include "types.h"

#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_rtom_elem elem;
};

ccc_threeway_cmp val_cmp(ccc_key_cmp);
void map_printer_fn(void const *);

enum test_result insert_shuffled(ccc_realtime_ordered_map *m, struct val vals[],
                                 size_t size, int larger_prime);
size_t inorder_fill(int vals[], size_t size, ccc_realtime_ordered_map const *m);

#endif /* CCC_RTOMAP_UTIL_H */
