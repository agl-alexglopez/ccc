#ifndef CCC_TRAITS_H
#define CCC_TRAITS_H

#include "impl_traits.h"

/*======================     Entry API  =====================================*/

#define ccc_insert(container_ptr, insert_args...)                              \
    ccc_impl_insert(container_ptr, insert_args)

#define ccc_insert_vr(container_ptr, key_val_container_handle_ptr...)          \
    ccc_impl_insert_vr(container_ptr, key_val_container_handle_ptr)

#define ccc_try_insert(container_ptr, try_insert_args...)                      \
    ccc_impl_try_insert(container_ptr, try_insert_args)

#define ccc_insert_or_assign(container_ptr, insert_or_assign_args...)          \
    ccc_impl_insert_or_assign(container_ptr, insert_or_assign_args)

#define ccc_try_insert_vr(container_ptr, key_val_container_handle_ptr...)      \
    ccc_impl_try_insert_vr(container_ptr, key_val_container_handle_ptr)

#define ccc_remove(container_ptr, key_val_container_handle_ptr...)             \
    ccc_impl_remove(container_ptr, key_val_container_handle_ptr)

#define ccc_remove_vr(container_ptr, key_val_container_handle_ptr...)          \
    ccc_impl_remove_vr(container_ptr, key_val_container_handle_ptr)

#define ccc_remove_entry(container_entry_ptr)                                  \
    ccc_impl_remove_entry(container_entry_ptr)

#define ccc_remove_entry_vr(container_entry_ptr)                               \
    ccc_impl_remove_entry_vr(container_entry_ptr)

#define ccc_entry(container_ptr, key_ptr...)                                   \
    ccc_impl_entry(container_ptr, key_ptr)

#define ccc_entry_vr(container_ptr, key_ptr...)                                \
    ccc_impl_entry_vr(container_ptr, key_ptr)

#define ccc_and_modify(entry_ptr, mod_fn) ccc_impl_and_modify(entry_ptr, mod_fn)

#define ccc_and_modify_aux(entry_ptr, mod_fn, aux_data_ptr...)                 \
    ccc_impl_and_modify_aux(entry_ptr, mod_fn, aux_data_ptr)

#define ccc_insert_entry(entry_ptr, key_val_container_handle_ptr...)           \
    ccc_impl_insert_entry(entry_ptr, key_val_container_handle_ptr)

#define ccc_or_insert(entry_ptr, key_val_container_handle_ptr...)              \
    ccc_impl_or_insert(entry_ptr, key_val_container_handle_ptr)

#define ccc_unwrap(entry_ptr) ccc_impl_unwrap(entry_ptr)

#define ccc_occupied(entry_ptr) ccc_impl_occupied(entry_ptr)

#define ccc_insert_error(entry_ptr) ccc_impl_insert_error(entry_ptr)

/*======================    Misc Search API  ================================*/

#define ccc_get_key_val(container_ptr, key_ptr...)                             \
    ccc_impl_get_key_val(container_ptr, key_ptr)

#define ccc_contains(container_ptr, key_ptr...)                                \
    ccc_impl_contains(container_ptr, key_ptr)

/*================       Sequential Containers API      =====================*/

#define ccc_push(container_ptr, container_handle_ptr...)                       \
    ccc_impl_push(container_ptr, container_handle_ptr)

#define ccc_push_back(container_ptr, container_handle_ptr...)                  \
    ccc_impl_push_back(container_ptr, container_handle_ptr)

#define ccc_push_front(container_ptr, container_handle_ptr...)                 \
    ccc_impl_push_front(container_ptr, container_handle_ptr)

#define ccc_pop(container_ptr) ccc_impl_pop(container_ptr)

#define ccc_pop_front(container_ptr) ccc_impl_pop_front(container_ptr)

#define ccc_pop_back(container_ptr) ccc_impl_pop_back(container_ptr)

#define ccc_front(container_ptr) ccc_impl_front(container_ptr)

#define ccc_back(container_ptr) ccc_impl_back(container_ptr)

/*================       Priority Queue Update API      =====================*/

#define ccc_update(container_ptr, update_args...)                              \
    ccc_impl_update(container_ptr, update_args)

#define ccc_increase(container_ptr, increase_args...)                          \
    ccc_impl_increase(container_ptr, increase_args)

#define ccc_decrease(container_ptr, decrease_args...)                          \
    ccc_impl_decrease(container_ptr, decrease_args)

#define ccc_erase(container_ptr, container_handle_ptr...)                      \
    ccc_impl_erase(container_ptr, container_handle_ptr)

#define ccc_erase_range(container_ptr, container_handle_begin_end_ptrs...)     \
    ccc_impl_erase_range(container_ptr, container_handle_begin_end_ptrs)

/*===================       Iterators API      ==============================*/

#define ccc_begin(container_ptr) ccc_impl_begin(container_ptr)

#define ccc_rbegin(container_ptr) ccc_impl_rbegin(container_ptr)

#define ccc_next(container_ptr, void_iterator_ptr)                             \
    ccc_impl_next(container_ptr, void_iterator_ptr)

#define ccc_rnext(container_ptr, void_iterator_ptr)                            \
    ccc_impl_rnext(container_ptr, void_iterator_ptr)

#define ccc_end(container_ptr) ccc_impl_end(container_ptr)

#define ccc_rend(container_ptr) ccc_impl_rend(container_ptr)

#define ccc_equal_range(container_ptr, begin_and_end_args...)                  \
    ccc_impl_equal_range(container_ptr, begin_and_end_args)

#define ccc_equal_range_vr(container_ptr, begin_and_end_args...)               \
    ccc_impl_equal_range_vr(container_ptr, begin_and_end_args)

#define ccc_equal_rrange(container_ptr, rbegin_and_rend_args...)               \
    ccc_impl_equal_rrange(container_ptr, rbegin_and_rend_args)

#define ccc_equal_rrange_vr(container_ptr, rbegin_and_rend_args...)            \
    ccc_impl_equal_rrange_vr(container_ptr, rbegin_and_rend_args)

#define ccc_splice(container_ptr, splice_args...)                              \
    ccc_impl_splice(container_ptr, splice_args)

#define ccc_splice_range(container_ptr, splice_args...)                        \
    ccc_impl_splice_range(container_ptr, splice_args)

/*===================    Standard Getters API  ==============================*/

#define ccc_size(container_entry_ptr) ccc_impl_size(container_entry_ptr)

#define ccc_empty(container_entry_ptr) ccc_impl_empty(container_entry_ptr)

#define ccc_validate(container_entry_ptr) ccc_impl_validate(container_entry_ptr)

#ifdef TRAITS_USING_NAMESPACE_CCC

#    define insert(args...) ccc_insert(args)
#    define try_insert(args...) ccc_try_insert(args)
#    define insert_or_assign(args...) ccc_insert_or_assign(args)
#    define try_insert_vr(args...) ccc_try_insert_vr(args)
#    define insert_vr(args...) ccc_insert_vr(args)
#    define remove(args...) ccc_remove(args)
#    define remove_vr(args...) ccc_remove_vr(args)
#    define remove_entry(args...) ccc_remove_entry(args)
#    define remove_entry_vr(args...) ccc_remove_entry_vr(args)
#    define entry(args...) ccc_entry(args)
#    define entry_vr(args...) ccc_entry_vr(args)
#    define or_insert(args...) ccc_or_insert(args)
#    define insert_entry(args...) ccc_insert_entry(args)
#    define and_modify(args...) ccc_and_modify(args)
#    define and_modify_aux(args...) ccc_and_modify_aux(args)
#    define occupied(args...) ccc_occupied(args)
#    define insert_error(args...) ccc_insert_error(args)
#    define unwrap(args...) ccc_unwrap(args)

#    define push(args...) ccc_push(args)
#    define push_back(args...) ccc_push_back(args)
#    define push_front(args...) ccc_push_front(args)
#    define pop(args...) ccc_pop(args)
#    define pop_front(args...) ccc_pop_front(args)
#    define pop_back(args...) ccc_pop_back(args)
#    define front(args...) ccc_front(args)
#    define back(args...) ccc_back(args)
#    define update(args...) ccc_update(args)
#    define increase(args...) ccc_increase(args)
#    define decrease(args...) ccc_decrease(args)
#    define erase(args...) ccc_erase(args)
#    define erase_range(args...) ccc_erase_range(args)

#    define get_key_val(args...) ccc_get_key_val(args)
#    define get_mut(args...) ccc_get_key_val_mut(args)
#    define contains(args...) ccc_contains(args)

#    define begin(args...) ccc_begin(args)
#    define rbegin(args...) ccc_rbegin(args)
#    define next(args...) ccc_next(args)
#    define rnext(args...) ccc_rnext(args)
#    define end(args...) ccc_end(args)
#    define rend(args...) ccc_rend(args)

#    define equal_range(args...) ccc_equal_range(args)
#    define equal_rrange(args...) ccc_equal_rrange(args)
#    define equal_range_vr(args...) ccc_equal_range_vr(args)
#    define equal_rrange_vr(args...) ccc_equal_rrange_vr(args)
#    define splice(args...) ccc_splice(args)
#    define splice_range(args...) ccc_splice_range(args)

#    define size(args...) ccc_size(args)
#    define empty(args...) ccc_empty(args)
#    define validate(args...) ccc_validate(args)

#endif /* CCC_USING_NAMESPACE_CCC */

#endif /* CCC_TRAITS_H */
