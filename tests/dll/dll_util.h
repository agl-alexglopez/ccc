#ifndef DLL_UTIL_H
#define DLL_UTIL_H

#include "doubly_linked_list.h"
#include "test.h"
#include "types.h"

struct val
{
    int id;
    int val;
    ccc_dll_elem e;
};

ccc_threeway_cmp val_cmp(ccc_cmp const *);

enum test_result check_order(ccc_doubly_linked_list const *, size_t n,
                             int const order[]);

#endif /* DLL_UTIL_H */
