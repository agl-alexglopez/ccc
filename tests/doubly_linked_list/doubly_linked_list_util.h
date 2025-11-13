#ifndef DLL_UTIL_H
#define DLL_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "doubly_linked_list.h"
#include "types.h"

struct Val
{
    CCC_Doubly_linked_list_node e;
    int id;
    int val;
};

enum Push_direction
{
    UTIL_PUSH_FRONT,
    UTIL_PUSH_BACK,
};

CCC_Order val_order(CCC_Type_comparator_context);

enum Check_result check_order(CCC_Doubly_linked_list const *, size_t n,
                              int const order[]);
enum Check_result create_list(CCC_Doubly_linked_list *, enum Push_direction,
                              size_t n, struct Val vals[]);

#endif /* DLL_UTIL_H */
