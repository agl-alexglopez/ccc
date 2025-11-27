#ifndef CCC_PQ_UTIL_H
#define CCC_PQ_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "priority_queue.h"
#include "types.h"
#include "utility/stack_allocator.h"

struct Val
{
    int id;
    int val;
    CCC_Priority_queue_node elem;
};

void val_update(CCC_Type_context);
CCC_Order val_order(CCC_Type_comparator_context);

/** Expects queue to have allocator permissions. */
enum Check_result insert_shuffled(CCC_Priority_queue *, size_t, int);

/** @brief Checks that the priority queue deterministically orders elements
in strictly increasing or decreasing order as determined by the initialization
order. Copies elements between priority queues to confirm this, checking the
keys remain in the same order.
@param[in] priority_queue_pointer the priority queue to test.
@param[in] priority_queue_size_integer_literal the direct integer liter, NOT
variable used to stack allocate the needed space to perform the check. This
must be equivalent to the priority queue size meaning both must be known at
compile time.
@return a passing check result if successful a failing check result if not.
@warning Buffers are allocated on the stack so only relatively small test cases
should be used. */
#define check_inorder_fill(priority_queue_pointer,                             \
                           priority_queue_size_integer_literal)                \
    (__extension__({                                                           \
        CCC_Priority_queue *const check_priority_queue_pointer                 \
            = (priority_queue_pointer);                                        \
        enum Check_result check_inorder_result = CHECK_FAIL;                   \
        if (check_priority_queue_pointer                                       \
            && CCC_priority_queue_count(check_priority_queue_pointer).count    \
                   == (priority_queue_size_integer_literal))                   \
        {                                                                      \
            check_inorder_result = private_inorder_fill(                       \
                &(struct Stack_allocator)stack_allocator_initialize(           \
                    struct Val, priority_queue_size_integer_literal),          \
                (int[priority_queue_size_integer_literal]){},                  \
                priority_queue_size_integer_literal, priority_queue_pointer);  \
        }                                                                      \
        check_inorder_result;                                                  \
    }))

/** Private for this header. Do not use directly. Use macro instead. */
enum Check_result private_inorder_fill(struct Stack_allocator *allocator, int[],
                                       size_t, CCC_Priority_queue *);

#endif /* CCC_PQ_UTIL_H */
