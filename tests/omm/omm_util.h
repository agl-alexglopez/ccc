#ifndef CCC_OMM_UTIL_H
#define CCC_OMM_UTIL_H

#include <stddef.h>

#include "ordered_multimap.h"
#include "types.h"

struct val
{
    int id;
    int val;
    ccc_omm_elem elem;
};

ccc_threeway_cmp val_cmp(ccc_key_cmp);
void val_update(ccc_user_type_mut);
void omm_printer_fn(ccc_user_type);

enum test_result insert_shuffled(ccc_ordered_multimap *, struct val[], size_t,
                                 int);
size_t inorder_fill(int[], size_t, ccc_ordered_multimap *);

#endif /* CCC_OMM_UTIL_H */
