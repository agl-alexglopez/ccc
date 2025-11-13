#ifndef CCC_FPQ_UTIL_H
#define CCC_FPQ_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "flat_priority_queue.h"
#include "types.h"

struct val
{
    int id;
    int val;
};

CCC_Order val_cmp(CCC_Type_comparator_context);
void val_update(CCC_Type_context);
size_t rand_range(size_t min, size_t max);
enum check_result inorder_fill(int[], size_t, CCC_flat_priority_queue const *);
enum check_result insert_shuffled(CCC_flat_priority_queue *pq,
                                  struct val vals[], size_t size,
                                  int larger_prime);

#endif /* CCC_FPQ_UTIL_H */
