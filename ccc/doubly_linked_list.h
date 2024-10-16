#ifndef CCC_DOUBLY_LINKED_LIST_H
#define CCC_DOUBLY_LINKED_LIST_H

#include <stddef.h>

#include "impl_doubly_linked_list.h"
#include "types.h"

typedef struct ccc_dll_elem_ ccc_dll_elem;
typedef struct ccc_dll_ ccc_doubly_linked_list;

/** @brief Initialize a doubly linked list with its l-value name, type
containing the dll elems, the field of the dll elem, allocation function,
compare function and any auxilliary data needed for comparison, printing, or
destructors.
@param [in] list_name the name of the list being initialized.
@param [in] struct_name the type containing the intrusive dll element.
@param [in] list_elem_field name of the dll element in the containing type.
@param [in] alloc_fn the optional allocation function or NULL.
@param [in] cmp_fn the ccc_cmp_fn used to compare list elements.
@param [in] aux_data any auxilliary data that will be needed for comparison,
printing, or destruction of elements.
@return the initialized list. Assign to the list directly on the right hand
side of an equality operator. Initialization can occur at runtime or compile
time (e.g. ccc_doubly_linked l = ccc_dll_init(...);). */
#define ccc_dll_init(list_name, struct_name, list_elem_field, alloc_fn,        \
                     cmp_fn, aux_data)                                         \
    ccc_impl_dll_init(list_name, struct_name, list_elem_field, alloc_fn,       \
                      cmp_fn, aux_data)

/** @brief  writes contents of type initializer directly to allocated memory at
the back of the list.
@param [in] list_ptr the address of the doubly linked list.
@param [in] type_initializer the r-value initializer of the type to be inserted
in the list. This should match the type containing dll elements as a struct
member for this list.
@return a reference to the inserted element or NULL if allocation is not
allowed or fails.

Note that it does not make sense to use this method if the list has been
initialized without an allocation function. If the user does not allow
allocation, the contents of new elements to be inserted has been determined by
the user prior to any inserts into the list. */
#define ccc_dll_emplace_back(list_ptr, type_initializer...)                    \
    ccc_impl_dll_emplace_back(list_ptr, type_initializer)

/** @brief  writes contents of type initializer directly to allocated memory at
the front of the list.
@param [in] list_ptr the address of the doubly linked list.
@param [in] type_initializer the r-value initializer of the type to be inserted
in the list. This should match the type containing dll elements as a struct
member for this list.
@return a reference to the inserted element or NULL if allocation is not
allowed or fails.

Note that it does not make sense to use this method if the list has been
initialized without an allocation function. If the user does not allow
allocation, the contents of new elements to be inserted has been determined by
the user prior to any inserts into the list. */
#define ccc_dll_emplace_front(list_ptr, type_initializer...)                   \
    ccc_impl_dll_emplace_front(list_ptr, type_initializer)

void *ccc_dll_push_front(ccc_doubly_linked_list *l,
                         ccc_dll_elem *struct_handle);
void *ccc_dll_push_back(ccc_doubly_linked_list *l, ccc_dll_elem *struct_handle);
void *ccc_dll_insert(ccc_doubly_linked_list *l, ccc_dll_elem *pos,
                     ccc_dll_elem *struct_handle);
void *ccc_dll_front(ccc_doubly_linked_list const *l);
void *ccc_dll_back(ccc_doubly_linked_list const *l);
ccc_result ccc_dll_pop_front(ccc_doubly_linked_list *l);
ccc_result ccc_dll_pop_back(ccc_doubly_linked_list *l);
void *ccc_dll_extract(ccc_doubly_linked_list *l,
                      ccc_dll_elem *struct_handle_in_list);
void *ccc_dll_extract_range(ccc_doubly_linked_list *l,
                            ccc_dll_elem *struct_handle_in_list_begin,
                            ccc_dll_elem *struct_handle_in_list_end);

/** @brief Repositions to_cut before pos. Only list pointers are modified.
@param [in] pos_sll the list to which pos belongs.
@param [in] pos the position before which to_cut will be moved.
@param [in] to_cut_sll the list to which to_cut belongs. */
ccc_result ccc_dll_splice(ccc_doubly_linked_list *pos_sll, ccc_dll_elem *pos,
                          ccc_doubly_linked_list *to_cut_sll,
                          ccc_dll_elem *to_cut);

ccc_result ccc_dll_splice_range(ccc_doubly_linked_list *pos_sll,
                                ccc_dll_elem *pos,
                                ccc_doubly_linked_list *to_cut_sll,
                                ccc_dll_elem *begin, ccc_dll_elem *end);
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
bool ccc_dll_is_empty(ccc_doubly_linked_list const *);

ccc_result ccc_dll_clear(ccc_doubly_linked_list *, ccc_destructor_fn *);

bool ccc_dll_validate(ccc_doubly_linked_list const *);
ccc_result ccc_dll_print(ccc_doubly_linked_list const *, ccc_print_fn *);

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
#    define dll_extract(args...) ccc_dll_extract(args)
#    define dll_extract_range(args...) ccc_dll_extract_range(args)
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
#    define dll_is_empty(args...) ccc_dll_is_empty(args)
#    define dll_clear(args...) ccc_dll_clear(args)
#    define dll_validate(args...) ccc_dll_validate(args)
#    define dll_print(args...) ccc_dll_print(args)
#endif /* DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC */

#endif /* CCC_LIST_H */
