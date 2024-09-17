#ifndef CCC_SINGLY_LINKED_LIST_H
#define CCC_SINGLY_LINKED_LIST_H

#include "impl_singly_linked_list.h"

#include <stddef.h>

typedef struct
{
    struct ccc_sll_elem_ impl_;
} ccc_sll_elem;

typedef struct
{
    struct ccc_sll_ impl_;
} ccc_singly_linked_list;

#define ccc_sll_init(list_ptr, list_name, struct_name, list_elem_field,        \
                     alloc_fn, aux_data)                                       \
    ccc_impl_sll_init(list_ptr, list_name, struct_name, list_elem_field,       \
                      alloc_fn, aux_data)

#define ccc_sll_emplace_front(list_ptr, struct_initializer...)                 \
    ccc_impl_sll_emplace_front(list_ptr, struct_initializer)

void *ccc_sll_push_front(ccc_singly_linked_list *sll,
                         ccc_sll_elem *struct_handle);
void *ccc_sll_front(ccc_singly_linked_list const *sll);
void ccc_sll_pop_front(ccc_singly_linked_list *sll);

void *ccc_sll_begin(ccc_singly_linked_list const *sll);
void *ccc_sll_end(ccc_singly_linked_list const *sll);
void *ccc_sll_next(ccc_singly_linked_list const *sll,
                   ccc_sll_elem const *iter_handle);

ccc_sll_elem *ccc_sll_head(ccc_singly_linked_list const *sll);
size_t ccc_sll_size(ccc_singly_linked_list const *sll);
bool ccc_sll_empty(ccc_singly_linked_list const *sll);

bool ccc_sll_validate(ccc_singly_linked_list const *sll);

#endif /* CCC_FORWARD_LIST_H */
