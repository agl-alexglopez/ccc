#ifndef CCC_TRAITS_H
#define CCC_TRAITS_H

#include "impl_traits.h"

#define ccc_insert(container_ptr, key_val_container_handle_ptr)                \
    ccc_impl_insert(container_ptr, key_val_container_handle_ptr)

#define ccc_insert_lv(container_ptr, key_val_container_handle_ptr)             \
    ccc_impl_insert_lv(container_ptr, key_val_container_handle_ptr)

#define ccc_remove(container_ptr, key_val_container_handle_ptr)                \
    ccc_impl_remove(container_ptr, key_val_container_handle_ptr)

#define ccc_remove_lv(container_ptr, key_val_container_handle_ptr)             \
    ccc_impl_remove_lv(container_ptr, key_val_container_handle_ptr)

#define ccc_remove_entry(container_entry_ptr)                                  \
    ccc_impl_remove_entry(container_entry_ptr)

#define ccc_remove_entry_lv(container_entry_ptr)                               \
    ccc_impl_remove_entry_lv(container_entry_ptr)

#define ccc_entry(container_ptr, key_ptr) ccc_impl_entry(container_ptr, key_ptr)

#define ccc_entry_lv(container_ptr, key_ptr)                                   \
    ccc_impl_entry_lv(container_ptr, key_ptr)

#define ccc_and_modify(entry_ptr, mod_fn) ccc_impl_and_modify(entry_ptr, mod_fn)

#define ccc_and_modify_lv(entry_ptr, mod_fn)                                   \
    ccc_impl_and_modify_lv(entry_ptr, mod_fn)

#define ccc_and_modify_with(entry_ptr, mod_fn, aux_data_ptr)                   \
    ccc_impl_and_modify_with(entry_ptr, mod_fn, aux_data_ptr)

#define ccc_and_modify_with_lv(entry_ptr, mod_fn, aux_data_ptr)                \
    ccc_impl_and_modify_with_lv(entry_ptr, mod_fn, aux_data_ptr)

#define ccc_insert_entry(entry_ptr, key_val_container_handle_ptr)              \
    ccc_impl_insert_entry(entry_ptr, key_val_container_handle_ptr)

#define ccc_or_insert(entry_ptr, key_val_container_handle_ptr)                 \
    ccc_impl_or_insert(entry_ptr, key_val_container_handle_ptr)

#define ccc_unwrap(entry_ptr) ccc_impl_unwrap(entry_ptr)

#define ccc_occupied(entry_ptr) ccc_impl_occupied(entry_ptr)

#define ccc_insert_error(entry_ptr) ccc_impl_insert_error(entry_ptr)

#ifdef TRAITS_USING_NAMESPACE_CCC

#    define insert(container_ptr, key_val_container_handle_ptr)                \
        ccc_insert(container_ptr, key_val_container_handle_ptr)
#    define insert_lv(container_ptr, key_val_container_handle_ptr)             \
        ccc_insert_lv(container_ptr, key_val_container_handle_ptr)
#    define remove(container_ptr, key_val_container_handle_ptr)                \
        ccc_remove(container_ptr, key_val_container_handle_ptr)
#    define remove_lv(container_ptr, key_val_container_handle_ptr)             \
        ccc_remove_lv(container_ptr, key_val_container_handle_ptr)
#    define remove_entry(container_entry_ptr)                                  \
        ccc_remove_entry(container_entry_ptr)
#    define remove_entry_lv(container_entry_ptr)                               \
        ccc_remove_entry_lv(container_entry_ptr)
#    define entry(container_ptr, key_ptr) ccc_entry(container_ptr, key_ptr)
#    define entry_lv(container_ptr, key_ptr)                                   \
        ccc_entry_lv(container_ptr, key_ptr)
#    define or_insert(container_entry_ptr, key_val_container_handle_ptr)       \
        ccc_or_insert(container_entry_ptr, key_val_container_handle_ptr)
#    define insert_entry(container_entry_ptr, key_val_container_handle_ptr)    \
        ccc_insert_entry(container_entry_ptr, key_val_container_handle_ptr)
#    define and_modify(container_entry_ptr, mod_fn)                            \
        ccc_and_modify(container_entry_ptr, mod_fn)
#    define and_modify_with(container_entry_ptr, mod_fn, aux_data_ptr)         \
        ccc_and_modify_with(container_entry_ptr, mod_fn, aux_data_ptr)
#    define and_modify_lv(container_entry_ptr, mod_fn)                         \
        ccc_and_modify_lv(container_entry_ptr, mod_fn)
#    define and_modify_with_lv(container_entry_ptr, mod_fn, aux_data_ptr)      \
        ccc_and_modify_with_lv(container_entry_ptr, mod_fn, aux_data_ptr)
#    define occupied(basic_entry_ptr) ccc_occupied(basic_entry_ptr)
#    define insert_error(basic_entry_ptr) ccc_insert_error(basic_entry_ptr)
#    define unwrap(basic_entry_ptr) ccc_unwrap(basic_entry_ptr)
#    define begin_range(container_ptr) ccc_begin_range(container_ptr)
#    define end_range(container_ptr) ccc_end_range(container_ptr)
#    define begin_rrange(container_ptr) ccc_begin_rrange(container_ptr)
#    define end_rrange(container_ptr) ccc_end_rrange(container_ptr)

#endif /* CCC_USING_NAMESPACE_CCC */

#endif /* CCC_TRAITS_H */
