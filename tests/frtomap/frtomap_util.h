#ifndef CCC_FRTOMAP_UTIL_H
#define CCC_FRTOMAP_UTIL_H

#include "flat_realtime_ordered_map.h"
#include "test.h"
#include "types.h"

#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_frtm_elem elem;
};

ccc_threeway_cmp val_cmp(ccc_key_cmp const *);
void map_printer_fn(void const *);

enum test_result insert_shuffled(ccc_flat_realtime_ordered_map *m, size_t size,
                                 int larger_prime);
size_t inorder_fill(int vals[], size_t size,
                    ccc_flat_realtime_ordered_map const *m);

#endif /* CCC_FRTOMAP_UTIL_H */
