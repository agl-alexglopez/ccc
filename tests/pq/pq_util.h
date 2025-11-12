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
    CCC_pq_elem elem;
};

void val_update(CCC_any_type);
CCC_threeway_cmp val_cmp(CCC_any_type_cmp);
enum check_result insert_shuffled(CCC_priority_queue *, struct val[], size_t,
                                  int);
enum check_result inorder_fill(int[], size_t, CCC_priority_queue *);

#endif /* CCC_PQ_UTIL_H */
