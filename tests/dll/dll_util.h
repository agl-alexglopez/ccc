#ifndef DLL_UTIL_H
#define DLL_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "doubly_linked_list.h"
#include "types.h"

struct val
{
    CCC_dll_elem e;
    int id;
    int val;
};

enum push_end
{
    UTIL_PUSH_FRONT,
    UTIL_PUSH_BACK,
};

CCC_threeway_cmp val_cmp(CCC_any_type_cmp);

enum check_result check_order(CCC_doubly_linked_list const *, size_t n,
                              int const order[]);
enum check_result create_list(CCC_doubly_linked_list *, enum push_end, size_t n,
                              struct val vals[]);

#endif /* DLL_UTIL_H */
