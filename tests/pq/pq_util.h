#ifndef CCC_PQ_UTIL_H
#define CCC_PQ_UTIL_H

#include "priority_queue.h"
#include "test.h"
#include "types.h"

struct val
{
    int id;
    int val;
    ccc_pq_elem elem;
};

void val_update(ccc_update);
ccc_threeway_cmp val_cmp(ccc_cmp);
enum test_result insert_shuffled(ccc_priority_queue *, struct val[], size_t,
                                 int);
enum test_result inorder_fill(int[], size_t, ccc_priority_queue *);

#endif /* CCC_PQ_UTIL_H */