#ifndef CCC_OMAP_UTIL_H
#define CCC_OMAP_UTIL_H

#include <stddef.h>

#include "adaptive_map.h"
#include "checkers.h"
#include "types.h"

struct Val
{
    int key;
    int val;
    CCC_Adaptive_map_node elem;
};

CCC_Order id_order(CCC_Key_comparator_context);

enum Check_result insert_shuffled(CCC_Adaptive_map *m, size_t size,
                                  int larger_prime);
enum Check_result inorder_fill(int vals[], size_t size, CCC_Adaptive_map *m);

#endif /* CCC_OMAP_UTIL_H */
