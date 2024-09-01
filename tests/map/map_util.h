#ifndef CCC_M_UTIL_H
#define CCC_M_UTIL_H

#include "map.h"
#include "test.h"

#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_mp_elem elem;
};

ccc_threeway_cmp val_cmp(ccc_key_cmp);
void map_printer_fn(void const *);

enum test_result insert_shuffled(ccc_map *m, struct val vals[], size_t size,
                                 int larger_prime);
size_t inorder_fill(int vals[], size_t size, ccc_map *m);

#endif /* CCC_M_UTIL_H */
