#ifndef CCC_LIST_H
#define CCC_LIST_H

#include "emplace.h" /* NOLINT */
#include "impl_list.h"

typedef struct ccc_list_elem
{
    struct ccc_impl_list_elem impl;
} ccc_list_elem;

typedef struct ccc_list
{
    struct ccc_impl_list impl;
} ccc_list;

#define CCC_L_INIT(list_ptr, list_name, struct_name, list_elem_field,          \
                   realloc_fn, aux_data)                                       \
    CCC_IMPL_L_INIT(list_ptr, list_name, struct_name, list_elem_field,         \
                    realloc_fn, aux_data)

void *ccc_l_push_front(ccc_list *l, ccc_list_elem *struct_handle);
void *ccc_l_push_back(ccc_list *l, ccc_list_elem *struct_handle);
void *ccc_l_front(ccc_list const *l);
void *ccc_l_back(ccc_list const *l);
void ccc_l_pop_front(ccc_list *l);
void ccc_l_pop_back(ccc_list *l);

/** @brief Repositions to_cut before pos. Only list pointers are modified.
@param [in] pos the position before which to_cut will be moved.
@param [in] to_cut the element to cut out of its current position. */
void ccc_l_splice(ccc_list_elem *pos, ccc_list_elem *to_cut);

void *ccc_l_begin(ccc_list const *);
void *ccc_l_next(ccc_list const *, ccc_list_elem const *);

size_t ccc_l_size(ccc_list const *);

bool ccc_l_validate(ccc_list const *);

#endif /* CCC_LIST_H */
