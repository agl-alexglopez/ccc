#ifndef CCC_LIST_H
#define CCC_LIST_H

#include "impl_list.h"

typedef struct ccc_list_elem
{
    struct ccc_impl_list_elem impl;
} ccc_list_elem;

typedef struct ccc_list
{
    struct ccc_impl_list impl;
} ccc_list;

#define CCC_LIST_INIT(list_ptr, list_name, struct_name, list_elem_field,       \
                      realloc_fn, aux_data)                                    \
    CCC_IMPL_LIST_INIT(list_ptr, list_name, struct_name, list_elem_field,      \
                       realloc_fn, aux_data)

#define LIST_EMPLACE_BACK(list_ptr, struct_initializer...)                     \
    CCC_IMPL_LIST_EMPLACE_BACK(list_ptr, struct_initializer)

#define LIST_EMPLACE_FRONT(list_ptr, struct_initializer...)                    \
    CCC_IMPL_LIST_EMPLACE_FRONT(list_ptr, struct_initializer)

void *ccc_list_push_front(ccc_list *l, ccc_list_elem *struct_handle);
void *ccc_list_push_back(ccc_list *l, ccc_list_elem *struct_handle);
void *ccc_list_front(ccc_list const *l);
void *ccc_list_back(ccc_list const *l);
void ccc_list_pop_front(ccc_list *l);
void ccc_list_pop_back(ccc_list *l);

/** @brief Repositions to_cut before pos. Only list pointers are modified.
@param [in] pos the position before which to_cut will be moved.
@param [in] to_cut the element to cut out of its current position. */
void ccc_list_splice(ccc_list_elem *pos, ccc_list_elem *to_cut);

void *ccc_list_begin(ccc_list const *);
void *ccc_list_next(ccc_list const *, ccc_list_elem const *);

ccc_list_elem *ccc_list_head(ccc_list const *);
ccc_list_elem *ccc_list_tail(ccc_list const *);

size_t ccc_list_size(ccc_list const *);
bool ccc_list_empty(ccc_list const *);

void ccc_list_clear(ccc_list *, ccc_destructor_fn *);

bool ccc_list_validate(ccc_list const *);

#endif /* CCC_LIST_H */
