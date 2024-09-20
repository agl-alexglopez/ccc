#ifndef CCC_DOUBLY_LINKED_LIST_H
#define CCC_DOUBLY_LINKED_LIST_H

#include "impl_doubly_linked_list.h"
#include "types.h"

typedef struct ccc_dll_elem_ ccc_dll_elem;
typedef struct ccc_dll_ ccc_doubly_linked_list;

#define ccc_dll_init(list_name, struct_name, list_elem_field, alloc_fn,        \
                     cmp_fn, aux_data)                                         \
    ccc_impl_dll_init(list_name, struct_name, list_elem_field, alloc_fn,       \
                      cmp_fn, aux_data)

#define ccc_dll_emplace_back(list_ptr, struct_initializer...)                  \
    ccc_impl_dll_emplace_back(list_ptr, struct_initializer)

#define ccc_dll_emplace_front(list_ptr, struct_initializer...)                 \
    ccc_impl_dll_emplace_front(list_ptr, struct_initializer)

void *ccc_dll_push_front(ccc_doubly_linked_list *l,
                         ccc_dll_elem *struct_handle);
void *ccc_dll_push_back(ccc_doubly_linked_list *l, ccc_dll_elem *struct_handle);
void *ccc_dll_insert(ccc_doubly_linked_list *l, ccc_dll_elem *pos,
                     ccc_dll_elem *struct_handle);
void *ccc_dll_front(ccc_doubly_linked_list const *l);
void *ccc_dll_back(ccc_doubly_linked_list const *l);
void ccc_dll_pop_front(ccc_doubly_linked_list *l);
void ccc_dll_pop_back(ccc_doubly_linked_list *l);
void ccc_dll_erase(ccc_doubly_linked_list *l,
                   ccc_dll_elem *struct_handle_in_list);
void ccc_dll_erase_range(ccc_doubly_linked_list *l,
                         ccc_dll_elem *struct_handle_in_list_begin,
                         ccc_dll_elem *struct_handle_in_list_end);

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

ccc_dll_elem *ccc_dll_begin_elem(ccc_doubly_linked_list const *);
ccc_dll_elem *ccc_dll_end_elem(ccc_doubly_linked_list const *);
ccc_dll_elem *ccc_dll_end_sentinel(ccc_doubly_linked_list const *);

size_t ccc_dll_size(ccc_doubly_linked_list const *);
bool ccc_dll_empty(ccc_doubly_linked_list const *);

void ccc_dll_clear_and_free(ccc_doubly_linked_list *, ccc_destructor_fn *);

bool ccc_dll_validate(ccc_doubly_linked_list const *);

#ifdef DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC
typedef ccc_dll_elem dll_elem;
typedef ccc_doubly_linked_list doubly_linked_list;
#    define dll_init(args...) ccc_dll_init(args)
#    define dll_emplace_back(args...) ccc_dll_emplace_back(args)
#    define dll_emplace_front(args...) ccc_dll_emplace_front(args)
#    define dll_push_front(args...) ccc_dll_push_front(args)
#    define dll_push_back(args...) ccc_dll_push_back(args)
#    define dll_front(args...) ccc_dll_front(args)
#    define dll_back(args...) ccc_dll_back(args)
#    define dll_pop_front(args...) ccc_dll_pop_front(args)
#    define dll_pop_back(args...) ccc_dll_pop_back(args)
#    define dll_splice(args...) ccc_dll_splice(args)
#    define dll_splice_range(args...) ccc_dll_splice_range(args)
#    define dll_begin(args...) ccc_dll_begin(args)
#    define dll_next(args...) ccc_dll_next(args)
#    define dll_rbegin(args...) ccc_dll_rbegin(args)
#    define dll_rnext(args...) ccc_dll_rnext(args)
#    define dll_end(args...) ccc_dll_end(args)
#    define dll_end_elem(args...) ccc_dll_end_elem(args)
#    define dll_rend(args...) ccc_dll_rend(args)
#    define dll_begin_elem(args...) ccc_dll_begin_elem(args)
#    define dll_end_elem(args...) ccc_dll_end_elem(args)
#    define dll_end_sentinel(args...) ccc_dll_end_sentinel(args)
#    define dll_size(args...) ccc_dll_size(args)
#    define dll_empty(args...) ccc_dll_empty(args)
#    define dll_clear_and_free(args...) ccc_dll_clear_and_free(args)
#    define dll_validate(args...) ccc_dll_validate(args)
#endif /* DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC */

#endif /* CCC_LIST_H */
