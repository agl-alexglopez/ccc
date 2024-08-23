#ifndef CCC_FPQ_UTIL_H
#define CCC_FPQ_UTIL_H

#include "flat_pqueue.h"
#include "test.h"

struct val
{
    int id;
    int val;
};

ccc_threeway_cmp val_cmp(ccc_cmp);
void val_print(void const *);
void val_update(ccc_update);
size_t rand_range(size_t min, size_t max);
enum test_result inorder_fill(int[], size_t, ccc_flat_pqueue *);
enum test_result insert_shuffled(ccc_flat_pqueue *pq, struct val vals[],
                                 size_t size, int larger_prime);

#endif /* CCC_FPQ_UTIL_H */
