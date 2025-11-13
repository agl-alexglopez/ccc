#ifndef CCC_PQ_UTIL_H
#define CCC_PQ_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "priority_queue.h"
#include "types.h"

struct Val
{
    int id;
    int val;
    CCC_Priority_queue_node elem;
};

void val_update(CCC_Type_context);
CCC_Order val_order(CCC_Type_comparator_context);
enum Check_result insert_shuffled(CCC_Priority_queue *, struct Val[], size_t,
                                  int);
enum Check_result inorder_fill(int[], size_t, CCC_Priority_queue *);

#endif /* CCC_PQ_UTIL_H */
