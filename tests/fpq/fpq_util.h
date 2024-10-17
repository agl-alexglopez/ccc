#ifndef CCC_FPQ_UTIL_H
#define CCC_FPQ_UTIL_H

#include <stddef.h>

#include "flat_priority_queue.h"
#include "test.h"
#include "types.h"

struct val
{
    int id;
    int val;
};

ccc_threeway_cmp val_cmp(ccc_cmp);
void val_update(ccc_user_type_mut);
size_t rand_range(size_t min, size_t max);
enum test_result inorder_fill(int[], size_t, ccc_flat_priority_queue *);
enum test_result insert_shuffled(ccc_flat_priority_queue *pq, struct val vals[],
                                 size_t size, int larger_prime);

#endif /* CCC_FPQ_UTIL_H */
