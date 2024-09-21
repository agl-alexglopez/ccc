#ifndef SLL_UTIL_H
#define SLL_UTIL_H

#include "singly_linked_list.h"
#include "test.h"
#include "types.h"

#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_sll_elem e;
};

ccc_threeway_cmp val_cmp(ccc_cmp const *);
enum test_result check_order(ccc_singly_linked_list const *, size_t n,
                             int const order[]);

#endif /* SLL_UTIL_H */
