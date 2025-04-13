#ifndef CCC_HROMAP_UTIL_H
#define CCC_HROMAP_UTIL_H

#include "checkers.h"
#include "handle_realtime_ordered_map.h"
#include "types.h"

#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_hromap_elem elem;
};

ccc_threeway_cmp id_cmp(ccc_any_key_cmp);

enum check_result insert_shuffled(ccc_handle_realtime_ordered_map *m,
                                  size_t size, int larger_prime);
size_t inorder_fill(int vals[], size_t size,
                    ccc_handle_realtime_ordered_map const *m);

#endif /* CCC_HROMAP_UTIL_H */
