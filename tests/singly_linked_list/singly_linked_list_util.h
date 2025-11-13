#ifndef SLL_UTIL_H
#define SLL_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "singly_linked_list.h"
#include "types.h"

struct val
{
    int id;
    int val;
    CCC_Singly_linked_list_node e;
};

CCC_Order val_cmp(CCC_Type_comparison_context);
enum check_result check_order(CCC_Singly_linked_list const *, size_t n,
                              int const order[]);
enum check_result create_list(CCC_Singly_linked_list *, size_t n,
                              struct val vals[]);

#endif /* SLL_UTIL_H */
