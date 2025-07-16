#ifndef CCC_HROMAP_UTIL_H
#define CCC_HROMAP_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "handle_realtime_ordered_map.h"
#include "types.h"

struct val
{
    int id;
    int val;
};

ccc_hrm_declare_fixed_map(small_fixed_map, struct val, 64);
ccc_hrm_declare_fixed_map(standard_fixed_map, struct val, 1024);

enum : size_t
{
    SMALL_FIXED_CAP = ccc_hrm_fixed_capacity(small_fixed_map),
    STANDARD_FIXED_CAP = ccc_hrm_fixed_capacity(standard_fixed_map),
};

ccc_threeway_cmp id_cmp(ccc_any_key_cmp);

enum check_result insert_shuffled(ccc_handle_realtime_ordered_map *m,
                                  size_t size, int larger_prime);
size_t inorder_fill(int vals[], size_t size,
                    ccc_handle_realtime_ordered_map const *m);

#endif /* CCC_HROMAP_UTIL_H */
