#ifndef CCC_MAP_UTIL_H
#define CCC_MAP_UTIL_H

#include "ordered_map.h"
#include "test.h"
#include "types.h"

#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_o_map_elem elem;
};

ccc_threeway_cmp val_cmp(ccc_key_cmp);
void map_printer_fn(ccc_user_type);

enum test_result insert_shuffled(ccc_ordered_map *m, struct val vals[],
                                 size_t size, int larger_prime);
size_t inorder_fill(int vals[], size_t size, ccc_ordered_map *m);

#endif /* CCC_MAP_UTIL_H */
