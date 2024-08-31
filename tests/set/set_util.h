#ifndef CCC_SET_UTIL_H
#define CCC_SET_UTIL_H

#include "set.h"
#include "test.h"

#include <stddef.h>

struct val
{
    int id;
    int val;
    ccc_set_elem elem;
};

ccc_threeway_cmp val_cmp(ccc_key_cmp);
void set_printer_fn(void const *);

enum test_result insert_shuffled(ccc_set *s, struct val vals[], size_t size,
                                 int larger_prime);
size_t inorder_fill(int vals[], size_t size, ccc_set *s);

#endif /* CCC_SET_UTIL_H */
