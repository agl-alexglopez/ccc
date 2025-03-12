#ifndef CCC_HOMAP_UTIL_H
#define CCC_HOMAP_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "handle_ordered_map.h"
#include "types.h"

struct val
{
    int id;
    int val;
    ccc_homap_elem elem;
};

ccc_threeway_cmp id_cmp(ccc_key_cmp);

enum check_result insert_shuffled(ccc_handle_ordered_map *m, ptrdiff_t size,
                                  int larger_prime);
ptrdiff_t inorder_fill(int vals[], ptrdiff_t size,
                       ccc_handle_ordered_map const *m);

#endif /* CCC_HOMAP_UTIL_H */
