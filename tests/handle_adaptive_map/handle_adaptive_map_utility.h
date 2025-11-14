#ifndef CCC_HOMAP_UTIL_H
#define CCC_HOMAP_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "handle_adaptive_map.h"
#include "types.h"

struct Val
{
    int id;
    int val;
};

CCC_handle_adaptive_map_declare_fixed_map(small_fixed_map, struct Val, 64);
CCC_handle_adaptive_map_declare_fixed_map(standard_fixed_map, struct Val, 1024);

enum : size_t
{
    SMALL_FIXED_CAP = CCC_handle_adaptive_map_fixed_capacity(small_fixed_map),
    STANDARD_FIXED_CAP
        = CCC_handle_adaptive_map_fixed_capacity(standard_fixed_map),
};

CCC_Order id_order(CCC_Key_comparator_context);

enum Check_result insert_shuffled(CCC_Handle_adaptive_map *m, size_t size,
                                  int larger_prime);
size_t inorder_fill(int vals[], size_t size, CCC_Handle_adaptive_map const *m);

#endif /* CCC_HOMAP_UTIL_H */
