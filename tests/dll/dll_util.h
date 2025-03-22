#ifndef DLL_UTIL_H
#define DLL_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "doubly_linked_list.h"
#include "types.h"

struct val
{
    ccc_dll_elem e;
    int id;
    int val;
};

enum push_end
{
    UTIL_PUSH_FRONT,
    UTIL_PUSH_BACK,
};

ccc_threeway_cmp val_cmp(ccc_cmp);

enum check_result check_order(ccc_doubly_linked_list const *, size_t n,
                              int const order[]);
enum check_result create_list(ccc_doubly_linked_list *, enum push_end, size_t n,
                              struct val vals[]);

#endif /* DLL_UTIL_H */
