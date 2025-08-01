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
    ccc_sll_elem e;
};

ccc_threeway_cmp val_cmp(ccc_any_type_cmp);
enum check_result check_order(ccc_singly_linked_list const *, size_t n,
                              int const order[]);
enum check_result create_list(ccc_singly_linked_list *, size_t n,
                              struct val vals[]);

#endif /* SLL_UTIL_H */
