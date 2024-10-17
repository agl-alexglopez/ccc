#ifndef CCC_SINGLY_LINKED_LIST_H
#define CCC_SINGLY_LINKED_LIST_H

#include "impl_singly_linked_list.h"
#include "types.h"

#include <stddef.h>

typedef struct ccc_sll_elem_ ccc_sll_elem;

typedef struct ccc_sll_ ccc_singly_linked_list;

#define ccc_sll_init(list_name, struct_name, list_elem_field, alloc_fn,        \
                     cmp_fn, aux_data)                                         \
    ccc_impl_sll_init(list_name, struct_name, list_elem_field, alloc_fn,       \
                      cmp_fn, aux_data)

#define ccc_sll_emplace_front(list_ptr, struct_initializer...)                 \
    ccc_impl_sll_emplace_front(list_ptr, struct_initializer)

void *ccc_sll_push_front(ccc_singly_linked_list *sll,
                         ccc_sll_elem *struct_handle);
void *ccc_sll_front(ccc_singly_linked_list const *sll);
ccc_result ccc_sll_pop_front(ccc_singly_linked_list *sll);
ccc_result ccc_sll_splice(ccc_singly_linked_list *pos_sll,
                          ccc_sll_elem *pos_before,
                          ccc_singly_linked_list *splice_sll,
                          ccc_sll_elem *splice);
ccc_result ccc_sll_splice_range(ccc_singly_linked_list *pos_sll,
                                ccc_sll_elem *pos_before,
                                ccc_singly_linked_list *splice_sll,
                                ccc_sll_elem *splice_begin,
                                ccc_sll_elem *splice_end);

void *ccc_sll_extract(ccc_singly_linked_list *extract_sll,
                      ccc_sll_elem *extract);
void *ccc_sll_extract_range(ccc_singly_linked_list *extract_sll,
                            ccc_sll_elem *extract_begin,
                            ccc_sll_elem *extract_end);

void *ccc_sll_begin(ccc_singly_linked_list const *sll);
void *ccc_sll_end(ccc_singly_linked_list const *sll);
void *ccc_sll_next(ccc_singly_linked_list const *sll,
                   ccc_sll_elem const *iter_handle);

ccc_sll_elem *ccc_sll_begin_elem(ccc_singly_linked_list const *sll);
ccc_sll_elem *ccc_sll_begin_sentinel(ccc_singly_linked_list const *sll);
size_t ccc_sll_size(ccc_singly_linked_list const *sll);
bool ccc_sll_is_empty(ccc_singly_linked_list const *sll);

bool ccc_sll_validate(ccc_singly_linked_list const *sll);
ccc_result ccc_sll_clear(ccc_singly_linked_list *sll, ccc_destructor_fn *fn);

#ifdef SINGLY_LINKED_LIST_USING_NAMESPACE_CCC
typedef ccc_sll_elem sll_elem;
typedef ccc_singly_linked_list singly_linked_list;
#    define sll_init(args...) ccc_sll_init(args)
#    define sll_emplace_front(args...) ccc_sll_emplace_front(args)
#    define sll_push_front(args...) ccc_sll_push_front(args)
#    define sll_front(args...) ccc_sll_front(args)
#    define sll_pop_front(args...) ccc_sll_pop_front(args)
#    define sll_splice(args...) ccc_sll_splice(args)
#    define sll_splice_range(args...) ccc_sll_splice_range(args)
#    define sll_extract(args...) ccc_sll_extract(args)
#    define sll_extract_range(args...) ccc_sll_extract_range(args)
#    define sll_begin(args...) ccc_sll_begin(args)
#    define sll_end(args...) ccc_sll_end(args)
#    define sll_next(args...) ccc_sll_next(args)
#    define sll_begin_elem(args...) ccc_sll_begin_elem(args)
#    define sll_begin_sentinel(args...) ccc_sll_begin_sentinel(args)
#    define sll_size(args...) ccc_sll_size(args)
#    define sll_is_empty(args...) ccc_sll_is_empty(args)
#    define sll_validate(args...) ccc_sll_validate(args)
#    define sll_clear(args...) ccc_sll_clear(args)
#endif /* SINGLY_LINKED_LIST_USING_NAMESPACE_CCC */

#endif /* CCC_FORWARD_LIST_H */
