#ifndef SLL_UTIL_H
#define SLL_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "singly_linked_list.h"
#include "types.h"

struct Val
{
    int id;
    int val;
    CCC_Singly_linked_list_node e;
};

CCC_Order val_order(CCC_Type_comparator_context);
enum Check_result check_order(CCC_Singly_linked_list const *, size_t n,
                              int const order[]);
enum Check_result push_list(CCC_Singly_linked_list *, size_t n,
                            struct Val vals[]);

#endif /* SLL_UTIL_H */
