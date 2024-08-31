#ifndef CCC_DEPQ_UTIL_H
#define CCC_DEPQ_UTIL_H

#include "double_ended_priority_queue.h"
#include "test.h"
#include "types.h"

struct val
{
    int id;
    int val;
    ccc_depq_elem elem;
};

ccc_threeway_cmp val_cmp(ccc_key_cmp);
void val_update(ccc_update);
void depq_printer_fn(void const *);

enum test_result insert_shuffled(ccc_double_ended_priority_queue *,
                                 struct val[], size_t, int);
size_t inorder_fill(int[], size_t, ccc_double_ended_priority_queue *);

#endif /* CCC_DEPQ_UTIL_H */
