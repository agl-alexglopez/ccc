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
    CCC_Priority_queue_node elem;
};

void val_update(CCC_Type_context);
CCC_Order val_cmp(CCC_Type_comparator_context);
enum check_result insert_shuffled(CCC_Priority_queue *, struct val[], size_t,
                                  int);
enum check_result inorder_fill(int[], size_t, CCC_Priority_queue *);

#endif /* CCC_PQ_UTIL_H */
