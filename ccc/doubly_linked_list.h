#ifndef CCC_DOUBLY_LINKED_LIST_H
#define CCC_DOUBLY_LINKED_LIST_H

#include "impl_doubly_linked_list.h"

typedef struct
{
    struct ccc_impl_dll_elem impl;
} ccc_dll_elem;

typedef struct
{
    struct ccc_impl_doubly_linked_list impl;
} ccc_doubly_linked_list;

#define CCC_DLL_INIT(list_ptr, list_name, struct_name, list_elem_field,        \
                     alloc_fn, cmp_fn, aux_data)                               \
    CCC_IMPL_DLL_INIT(list_ptr, list_name, struct_name, list_elem_field,       \
                      alloc_fn, cmp_fn, aux_data)

#define DLL_EMPLACE_BACK(list_ptr, struct_initializer...)                      \
    CCC_IMPL_DLL_EMPLACE_BACK(list_ptr, struct_initializer)

#define DLL_EMPLACE_FRONT(list_ptr, struct_initializer...)                     \
    CCC_IMPL_DLL_EMPLACE_FRONT(list_ptr, struct_initializer)

void *ccc_dll_push_front(ccc_doubly_linked_list *l,
                         ccc_dll_elem *struct_handle);
void *ccc_dll_push_back(ccc_doubly_linked_list *l, ccc_dll_elem *struct_handle);
void *ccc_dll_front(ccc_doubly_linked_list const *l);
void *ccc_dll_back(ccc_doubly_linked_list const *l);
void ccc_dll_pop_front(ccc_doubly_linked_list *l);
void ccc_dll_pop_back(ccc_doubly_linked_list *l);

/** @brief Repositions to_cut before pos. Only list pointers are modified.
@param [in] pos the position before which to_cut will be moved.
@param [in] to_cut the element to cut out of its current position. */
void ccc_dll_splice(ccc_dll_elem *pos, ccc_dll_elem *to_cut);

void ccc_dll_splice_range(ccc_dll_elem *pos, ccc_dll_elem *begin,
                          ccc_dll_elem *end);

void *ccc_dll_begin(ccc_doubly_linked_list const *);
void *ccc_dll_next(ccc_doubly_linked_list const *, ccc_dll_elem const *);

ccc_dll_elem *ccc_dll_head(ccc_doubly_linked_list const *);
ccc_dll_elem *ccc_dll_tail(ccc_doubly_linked_list const *);

size_t ccc_dll_size(ccc_doubly_linked_list const *);
bool ccc_dll_empty(ccc_doubly_linked_list const *);

void ccc_dll_clear(ccc_doubly_linked_list *, ccc_destructor_fn *);

bool ccc_dll_validate(ccc_doubly_linked_list const *);

#endif /* CCC_LIST_H */
