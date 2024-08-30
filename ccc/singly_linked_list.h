#ifndef CCC_SINGLY_LINKED_LIST_H
#define CCC_SINGLY_LINKED_LIST_H

#include "impl_singly_linked_list.h"

typedef struct
{
    struct ccc_impl_sll_elem impl;
} ccc_sll_elem;

typedef struct
{
    struct ccc_impl_singly_linked_list impl;
} ccc_singly_linked_list;

#define CCC_SLL_INIT(list_ptr, list_name, struct_name, list_elem_field,        \
                     realloc_fn, aux_data)                                     \
    CCC_IMPL_SLL_INIT(list_ptr, list_name, struct_name, list_elem_field,       \
                      realloc_fn, aux_data)

#define SLL_EMPLACE_FRONT(list_ptr, struct_initializer...)                     \
    CCC_IMPL_SLL_EMPLACE_FRONT(list_ptr, struct_initializer)

void *ccc_sll_push_front(ccc_singly_linked_list *sll,
                         ccc_sll_elem *struct_handle);
void *ccc_sll_front(ccc_singly_linked_list const *sll);
void ccc_sll_pop_front(ccc_singly_linked_list *sll);

ccc_sll_elem *ccc_sll_head(ccc_singly_linked_list const *sll);

bool ccc_sll_validate(ccc_singly_linked_list const *sll);

#endif /* CCC_FORWARD_LIST_H */
