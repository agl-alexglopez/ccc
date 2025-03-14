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

ccc_threeway_cmp val_cmp(ccc_cmp);
void val_update(ccc_user_type);
ptrdiff_t rand_range(ptrdiff_t min, ptrdiff_t max);
enum check_result inorder_fill(int[], ptrdiff_t, ccc_flat_priority_queue *);
enum check_result insert_shuffled(ccc_flat_priority_queue *pq,
                                  struct val vals[], ptrdiff_t size,
                                  int larger_prime);

#endif /* CCC_FPQ_UTIL_H */
