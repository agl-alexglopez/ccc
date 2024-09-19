#ifndef CCC_DOUBLY_LINKED_LIST_H
#define CCC_DOUBLY_LINKED_LIST_H

#include "impl_doubly_linked_list.h"
#include "types.h"

typedef struct
{
    struct ccc_dll_elem_ impl_;
} ccc_dll_elem;

typedef struct
{
    struct ccc_dll_ impl_;
} ccc_doubly_linked_list;

#define ccc_dll_init(list_ptr, list_name, struct_name, list_elem_field,        \
                     alloc_fn, cmp_fn, aux_data)                               \
    ccc_impl_dll_init(list_ptr, list_name, struct_name, list_elem_field,       \
                      alloc_fn, cmp_fn, aux_data)

#define ccc_dll_emplace_back(list_ptr, struct_initializer...)                  \
    ccc_impl_dll_emplace_back(list_ptr, struct_initializer)

#define ccc_dll_emplace_front(list_ptr, struct_initializer...)                 \
    ccc_impl_dll_emplace_front(list_ptr, struct_initializer)

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

void *ccc_dll_rbegin(ccc_doubly_linked_list const *);
void *ccc_dll_rnext(ccc_doubly_linked_list const *, ccc_dll_elem const *);

void *ccc_dll_end(ccc_doubly_linked_list const *);
void *ccc_dll_rend(ccc_doubly_linked_list const *);

ccc_dll_elem *ccc_dll_head(ccc_doubly_linked_list const *);
ccc_dll_elem *ccc_dll_tail(ccc_doubly_linked_list const *);

size_t ccc_dll_size(ccc_doubly_linked_list const *);
bool ccc_dll_empty(ccc_doubly_linked_list const *);

void ccc_dll_clear_and_free(ccc_doubly_linked_list *, ccc_destructor_fn *);

bool ccc_dll_validate(ccc_doubly_linked_list const *);

#endif /* CCC_LIST_H */
