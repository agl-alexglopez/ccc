#ifndef CCC_HROMAP_UTIL_H
#define CCC_HROMAP_UTIL_H

#include <stddef.h>

#include "array_tree_map.h"
#include "checkers.h"
#include "types.h"

struct Val
{
    int id;
    int val;
};

CCC_array_tree_map_declare_fixed_map(Small_fixed_map, struct Val, 64);
CCC_array_tree_map_declare_fixed_map(Standard_fixed_map, struct Val, 1024);

enum : size_t
{
    SMALL_FIXED_CAP = CCC_array_tree_map_fixed_capacity(Small_fixed_map),
    STANDARD_FIXED_CAP = CCC_array_tree_map_fixed_capacity(Standard_fixed_map),
};

CCC_Order id_order(CCC_Key_comparator_context);

enum Check_result insert_shuffled(CCC_Array_tree_map *m, size_t size,
                                  int larger_prime);
size_t inorder_fill(int vals[], size_t size, CCC_Array_tree_map const *m);

#endif /* CCC_HROMAP_UTIL_H */
