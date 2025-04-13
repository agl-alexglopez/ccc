#ifndef CCC_PQ_UTIL_H
#define CCC_PQ_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "priority_queue.h"
#include "types.h"

struct val
{
    int id;
    int val;
    ccc_pq_elem elem;
};

void val_update(ccc_any_type);
ccc_threeway_cmp val_cmp(ccc_any_cmp);
enum check_result insert_shuffled(ccc_priority_queue *, struct val[], size_t,
                                  int);
enum check_result inorder_fill(int[], size_t, ccc_priority_queue *);

#endif /* CCC_PQ_UTIL_H */
