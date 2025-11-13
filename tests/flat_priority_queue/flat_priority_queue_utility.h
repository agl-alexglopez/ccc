#ifndef CCC_FLAT_PRIORITY_QUEUE_UTIL_H
#define CCC_FLAT_PRIORITY_QUEUE_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "flat_priority_queue.h"
#include "types.h"

struct Val
{
    int id;
    int val;
};

CCC_Order val_order(CCC_Type_comparator_context);
void val_update(CCC_Type_context);
size_t rand_range(size_t min, size_t max);
enum Check_result inorder_fill(int[], size_t, CCC_Flat_priority_queue const *);
enum Check_result insert_shuffled(CCC_Flat_priority_queue *priority_queue,
                                  struct Val vals[], size_t size,
                                  int larger_prime);

#endif /* CCC_FLAT_PRIORITY_QUEUE_UTIL_H */
