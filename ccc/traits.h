#ifndef CCC_TRAITS_H
#define CCC_TRAITS_H

#include "impl_traits.h"

/*======================     Entry API  =====================================*/

#define ccc_insert(container_ptr, key_val_container_handle_ptr)                \
    ccc_impl_insert(container_ptr, key_val_container_handle_ptr)

#define ccc_insert_vr(container_ptr, key_val_container_handle_ptr)             \
    ccc_impl_insert_vr(container_ptr, key_val_container_handle_ptr)

#define ccc_remove(container_ptr, key_val_container_handle_ptr)                \
    ccc_impl_remove(container_ptr, key_val_container_handle_ptr)

#define ccc_remove_vr(container_ptr, key_val_container_handle_ptr)             \
    ccc_impl_remove_vr(container_ptr, key_val_container_handle_ptr)

#define ccc_remove_entry(container_entry_ptr)                                  \
    ccc_impl_remove_entry(container_entry_ptr)

#define ccc_remove_entry_vr(container_entry_ptr)                               \
    ccc_impl_remove_entry_vr(container_entry_ptr)

#define ccc_entry(container_ptr, key_ptr) ccc_impl_entry(container_ptr, key_ptr)

#define ccc_entry_vr(container_ptr, key_ptr)                                   \
    ccc_impl_entry_vr(container_ptr, key_ptr)

#define ccc_and_modify(entry_ptr, mod_fn) ccc_impl_and_modify(entry_ptr, mod_fn)

#define ccc_and_modify_vr(entry_ptr, mod_fn)                                   \
    ccc_impl_and_modify_vr(entry_ptr, mod_fn)

#define ccc_and_modify_with(entry_ptr, mod_fn, aux_data_ptr)                   \
    ccc_impl_and_modify_with(entry_ptr, mod_fn, aux_data_ptr)

#define ccc_and_modify_with_vr(entry_ptr, mod_fn, aux_data_ptr)                \
    ccc_impl_and_modify_with_vr(entry_ptr, mod_fn, aux_data_ptr)

#define ccc_insert_entry(entry_ptr, key_val_container_handle_ptr)              \
    ccc_impl_insert_entry(entry_ptr, key_val_container_handle_ptr)

#define ccc_or_insert(entry_ptr, key_val_container_handle_ptr)                 \
    ccc_impl_or_insert(entry_ptr, key_val_container_handle_ptr)

#define ccc_unwrap(entry_ptr) ccc_impl_unwrap(entry_ptr)

#define ccc_occupied(entry_ptr) ccc_impl_occupied(entry_ptr)

#define ccc_insert_error(entry_ptr) ccc_impl_insert_error(entry_ptr)

/*======================    Misc Search API  ================================*/

#define ccc_get(container_ptr, key_ptr) ccc_impl_get(container_ptr, key_ptr)

#define ccc_get_mut(container_ptr, key_ptr)                                    \
    ccc_impl_get_mut(container_ptr, key_ptr)

#define ccc_contains(container_ptr, key_ptr)                                   \
    ccc_impl_contains(container_ptr, key_ptr)

/*================       Sequential Containers API      =====================*/

#define ccc_push(container_ptr, container_handle_ptr)                          \
    ccc_impl_push(container_ptr, container_handle_ptr)

#define ccc_push_back(container_ptr, container_handle_ptr)                     \
    ccc_impl_push_back(container_ptr, container_handle_ptr)

#define ccc_push_front(container_ptr, container_handle_ptr)                    \
    ccc_impl_push_front(container_ptr, container_handle_ptr)

#define ccc_pop(container_ptr) ccc_impl_pop(container_ptr)

#define ccc_pop_front(container_ptr) ccc_impl_pop_front(container_ptr)

#define ccc_pop_back(container_ptr) ccc_impl_pop_back(container_ptr)

#define ccc_front(container_ptr) ccc_impl_front(container_ptr)

#define ccc_back(container_ptr) ccc_impl_back(container_ptr)

/*================       Priority Queue Update API      =====================*/

#define ccc_update(container_ptr, container_handle_ptr, update_fn_ptr,         \
                   aux_data_ptr)                                               \
    ccc_impl_update(container_ptr, container_handle_ptr, update_fn_ptr,        \
                    aux_data_ptr)

#define ccc_increase(container_ptr, container_handle_ptr, update_fn_ptr,       \
                     aux_data_ptr)                                             \
    ccc_impl_increase(container_ptr, container_handle_ptr, update_fn_ptr,      \
                      aux_data_ptr)

#define ccc_decrease(container_ptr, container_handle_ptr, update_fn_ptr,       \
                     aux_data_ptr)                                             \
    ccc_impl_decrease(container_ptr, container_handle_ptr, update_fn_ptr,      \
                      aux_data_ptr)

#define ccc_erase(container_ptr, container_handle_ptr)                         \
    ccc_impl_erase(container_ptr, container_handle_ptr)

/*===================       Iterators API      ==============================*/

#define ccc_begin(container_ptr) ccc_impl_begin(container_ptr)

#define ccc_rbegin(container_ptr) ccc_impl_rbegin(container_ptr)

#define ccc_next(container_ptr, void_iterator_ptr)                             \
    ccc_impl_next(container_ptr, void_iterator_ptr)

#define ccc_rnext(container_ptr, void_iterator_ptr)                            \
    ccc_impl_rnext(container_ptr, void_iterator_ptr)

#define ccc_end(container_ptr) ccc_impl_end(container_ptr)

#define ccc_rend(container_ptr) ccc_impl_rend(container_ptr)

/*===================    Standard Getters API  ==============================*/

#define ccc_size(container_entry_ptr) ccc_impl_size(container_entry_ptr)

#define ccc_empty(container_entry_ptr) ccc_impl_empty(container_entry_ptr)

#ifdef TRAITS_USING_NAMESPACE_CCC

#    define insert(container_ptr, key_val_container_handle_ptr)                \
        ccc_insert(container_ptr, key_val_container_handle_ptr)
#    define insert_vr(container_ptr, key_val_container_handle_ptr)             \
        ccc_insert_vr(container_ptr, key_val_container_handle_ptr)
#    define remove(container_ptr, key_val_container_handle_ptr)                \
        ccc_remove(container_ptr, key_val_container_handle_ptr)
#    define remove_vr(container_ptr, key_val_container_handle_ptr)             \
        ccc_remove_vr(container_ptr, key_val_container_handle_ptr)
#    define remove_entry(container_entry_ptr)                                  \
        ccc_remove_entry(container_entry_ptr)
#    define remove_entry_vr(container_entry_ptr)                               \
        ccc_remove_entry_vr(container_entry_ptr)
#    define entry(container_ptr, key_ptr) ccc_entry(container_ptr, key_ptr)
#    define entry_vr(container_ptr, key_ptr)                                   \
        ccc_entry_vr(container_ptr, key_ptr)
#    define or_insert(container_entry_ptr, key_val_container_handle_ptr)       \
        ccc_or_insert(container_entry_ptr, key_val_container_handle_ptr)
#    define insert_entry(container_entry_ptr, key_val_container_handle_ptr)    \
        ccc_insert_entry(container_entry_ptr, key_val_container_handle_ptr)
#    define and_modify(container_entry_ptr, mod_fn)                            \
        ccc_and_modify(container_entry_ptr, mod_fn)
#    define and_modify_with(container_entry_ptr, mod_fn, aux_data_ptr)         \
        ccc_and_modify_with(container_entry_ptr, mod_fn, aux_data_ptr)
#    define and_modify_vr(container_entry_ptr, mod_fn)                         \
        ccc_and_modify_vr(container_entry_ptr, mod_fn)
#    define and_modify_with_vr(container_entry_ptr, mod_fn, aux_data_ptr)      \
        ccc_and_modify_with_vr(container_entry_ptr, mod_fn, aux_data_ptr)
#    define occupied(basic_entry_ptr) ccc_occupied(basic_entry_ptr)
#    define insert_error(basic_entry_ptr) ccc_insert_error(basic_entry_ptr)
#    define unwrap(basic_entry_ptr) ccc_unwrap(basic_entry_ptr)
#    define begin_range(range_ptr) ccc_begin_range(range_ptr)
#    define end_range(range_ptr) ccc_end_range(range_ptr)
#    define rbegin_rrange(range_ptr) ccc_rbegin_rrange(range_ptr)
#    define rend_rrange(range_ptr) ccc_rend_rrange(range_ptr)

#    define push(container_ptr, container_handle_ptr)                          \
        ccc_push(container_ptr, container_handle_ptr)
#    define push_back(container_ptr, container_handle_ptr)                     \
        ccc_push_back(container_ptr, container_handle_ptr)
#    define push_front(container_ptr, container_handle_ptr)                    \
        ccc_push_front(container_ptr, container_handle_ptr)
#    define pop(container_ptr) ccc_pop(container_ptr)
#    define pop_front(container_ptr) ccc_pop_front(container_ptr)
#    define pop_back(container_ptr) ccc_pop_back(container_ptr)
#    define front(container_ptr) ccc_front(container_ptr)
#    define back(container_ptr) ccc_back(container_ptr)
#    define update(container_ptr, container_handle_ptr, update_fn_ptr,         \
                   aux_data_ptr)                                               \
        ccc_update(container_ptr, container_handle_ptr, update_fn_ptr,         \
                   aux_data_ptr)
#    define increase(container_ptr, container_handle_ptr, update_fn_ptr,       \
                     aux_data_ptr)                                             \
        ccc_increase(container_ptr, container_handle_ptr, update_fn_ptr,       \
                     aux_data_ptr)
#    define decrease(container_ptr, container_handle_ptr, update_fn_ptr,       \
                     aux_data_ptr)                                             \
        ccc_decrease(container_ptr, container_handle_ptr, update_fn_ptr,       \
                     aux_data_ptr)
#    define erase(container_ptr, container_handle_ptr)                         \
        ccc_erase(container_ptr, container_handle_ptr)

#    define get(container_ptr, key_ptr) ccc_get(container_ptr, key_ptr)
#    define get_mut(container_ptr, key_ptr) ccc_get_mut(container_ptr, key_ptr)
#    define contains(container_ptr, key_ptr)                                   \
        ccc_contains(container_ptr, key_ptr)

#    define begin(container_ptr) ccc_begin(container_ptr)
#    define rbegin(container_ptr) ccc_rbegin(container_ptr)
#    define next(container_ptr, void_iter_ptr)                                 \
        ccc_next(container_ptr, void_iter_ptr)
#    define rnext(container_ptr, void_iter_ptr)                                \
        ccc_rnext(container_ptr, void_iter_ptr)
#    define end(container_ptr) ccc_end(container_ptr)
#    define rend(container_ptr) ccc_rend(container_ptr)

#    define size(container_ptr) ccc_size(container_ptr)
#    define empty(container_ptr) ccc_empty(container_ptr)

#endif /* CCC_USING_NAMESPACE_CCC */

#endif /* CCC_TRAITS_H */
