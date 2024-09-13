#ifndef CCC_IMPL_TRAITS_H
#define CCC_IMPL_TRAITS_H

#include "flat_hash_map.h"
#include "ordered_map.h"
#include "realtime_ordered_map.h"
#include "types.h"

#define ccc_impl_insert(container_ptr, key_val_container_handle_ptr)           \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_insert,                                   \
        ccc_ordered_map *: ccc_om_insert,                                      \
        ccc_realtime_ordered_map *: ccc_rom_insert)(                           \
        (container_ptr), (key_val_container_handle_ptr))

#define ccc_impl_insert_lv(container_ptr, key_val_container_handle_ptr)        \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map                                                      \
            *: &(ccc_entry){ccc_fhm_insert(                                    \
                                (ccc_flat_hash_map *)(container_ptr),          \
                                (ccc_fh_map_elem                               \
                                     *)(key_val_container_handle_ptr))         \
                                .impl_},                                       \
        ccc_ordered_map                                                        \
            *: &(ccc_entry){ccc_om_insert(                                     \
                                (ccc_ordered_map *)(container_ptr),            \
                                (ccc_o_map_elem                                \
                                     *)(key_val_container_handle_ptr))         \
                                .impl_},                                       \
        ccc_realtime_ordered_map                                               \
            *: &(ccc_entry){                                                   \
                ccc_rom_insert(                                                \
                    (ccc_realtime_ordered_map *)(container_ptr),               \
                    (ccc_rtom_elem *)(key_val_container_handle_ptr))           \
                    .impl_})

#define ccc_impl_remove(container_ptr, key_val_container_handle_ptr)           \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_remove,                                   \
        ccc_ordered_map *: ccc_om_remove,                                      \
        ccc_realtime_ordered_map *: ccc_rom_remove)(                           \
        (container_ptr), (key_val_container_handle_ptr))

#define ccc_impl_remove_lv(container_ptr, key_val_container_handle_ptr)        \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map                                                      \
            *: &(ccc_entry){ccc_fhm_remove(                                    \
                                (ccc_flat_hash_map *)(container_ptr),          \
                                (ccc_fh_map_elem                               \
                                     *)(key_val_container_handle_ptr))         \
                                .impl_},                                       \
        ccc_ordered_map                                                        \
            *: &(ccc_entry){ccc_om_remove(                                     \
                                (ccc_ordered_map *)(container_ptr),            \
                                (ccc_o_map_elem                                \
                                     *)(key_val_container_handle_ptr))         \
                                .impl_},                                       \
        ccc_realtime_ordered_map                                               \
            *: &(ccc_entry){                                                   \
                ccc_rom_remove(                                                \
                    (ccc_realtime_ordered_map *)(container_ptr),               \
                    (ccc_rtom_elem *)(key_val_container_handle_ptr))           \
                    .impl_})

#define ccc_impl_remove_entry(container_entry_ptr)                             \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry *: ccc_fhm_remove_entry,                              \
        ccc_o_map_entry *: ccc_om_remove_entry,                                \
        ccc_rtom_entry *: ccc_rom_remove_entry)((container_entry_ptr))

#define ccc_impl_remove_entry_lv(container_entry_ptr)                          \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry                                                       \
            *: &(ccc_entry){ccc_fhm_remove_entry(                              \
                                (ccc_fh_map_entry *)(container_entry_ptr))     \
                                .impl_},                                       \
        ccc_o_map_entry                                                        \
            *: &(ccc_entry){ccc_om_remove_entry(                               \
                                (ccc_o_map_entry *)(container_entry_ptr))      \
                                .impl_},                                       \
        ccc_rtom_entry                                                         \
            *: &(ccc_entry){                                                   \
                ccc_rom_remove_entry((ccc_rtom_entry *)(container_entry_ptr))  \
                    .impl_})

#define ccc_impl_entry(container_ptr, key_ptr)                                 \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_entry,                                    \
        ccc_ordered_map *: ccc_om_entry,                                       \
        ccc_realtime_ordered_map *: ccc_rom_entry)((container_ptr), (key_ptr))

#define ccc_impl_entry_lv(container_ptr, key_ptr)                              \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map                                                      \
            *: &(ccc_fh_map_entry){ccc_fhm_entry(                              \
                                       (ccc_flat_hash_map *)(container_ptr),   \
                                       (key_ptr))                              \
                                       .impl_},                                \
        ccc_ordered_map                                                        \
            *: &(ccc_o_map_entry){ccc_om_entry(                                \
                                      (ccc_ordered_map *)(container_ptr),      \
                                      (key_ptr))                               \
                                      .impl_},                                 \
        ccc_realtime_ordered_map                                               \
            *: &(ccc_rtom_entry){                                              \
                ccc_rom_entry((ccc_realtime_ordered_map *)(container_ptr),     \
                              (key_ptr))                                       \
                    .impl_})

#define ccc_impl_and_modify(container_entry_ptr, mod_fn)                       \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry *: ccc_fhm_and_modify,                                \
        ccc_o_map_entry *: ccc_om_and_modify,                                  \
        ccc_rtom_entry *: ccc_rom_and_modify)((container_entry_ptr), (mod_fn))

#define ccc_impl_and_modify_lv(container_entry_ptr, mod_fn)                    \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry                                                       \
            *: &(ccc_fh_map_entry){ccc_fhm_and_modify(                         \
                                       (ccc_fh_map_entry                       \
                                            *)(container_entry_ptr),           \
                                       (mod_fn))                               \
                                       .impl_},                                \
        ccc_o_map_entry                                                        \
            *: &(                                                              \
                ccc_o_map_entry){ccc_om_and_modify(                            \
                                     (ccc_o_map_entry *)(container_entry_ptr), \
                                     (mod_fn))                                 \
                                     .impl_},                                  \
        ccc_rtom_entry                                                         \
            *: &(ccc_rtom_entry){                                              \
                ccc_rom_and_modify((ccc_rtom_entry *)(container_entry_ptr),    \
                                   (mod_fn))                                   \
                    .impl_})

#define ccc_impl_and_modify_with(container_entry_ptr, mod_fn, aux_data_ptr)    \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry *: ccc_fhm_and_modify_with,                           \
        ccc_o_map_entry *: ccc_om_and_modify_with,                             \
        ccc_rtom_entry *: ccc_rom_and_modify_with)((container_entry_ptr),      \
                                                   (mod_fn), (aux_data_ptr))

#define ccc_impl_and_modify_with_lv(container_entry_ptr, mod_fn, aux_data_ptr) \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry                                                       \
            *: &(ccc_fh_map_entry){ccc_fhm_and_modify_with(                    \
                                       (ccc_fh_map_entry                       \
                                            *)(container_entry_ptr),           \
                                       (mod_fn), (aux_data_ptr))               \
                                       .impl_},                                \
        ccc_o_map_entry                                                        \
            *: &(                                                              \
                ccc_o_map_entry){ccc_om_and_modify_with(                       \
                                     (ccc_o_map_entry *)(container_entry_ptr), \
                                     (mod_fn), (aux_data_ptr))                 \
                                     .impl_},                                  \
        ccc_rtom_entry                                                         \
            *: &(ccc_rtom_entry){ccc_rom_and_modify_with(                      \
                                     (ccc_rtom_entry *)(container_entry_ptr),  \
                                     (mod_fn), (aux_data_ptr))                 \
                                     .impl_})

#define ccc_impl_insert_entry(container_entry_ptr,                             \
                              key_val_container_handle_ptr)                    \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry *: ccc_fhm_insert_entry,                              \
        ccc_o_map_entry *: ccc_om_insert_entry,                                \
        ccc_rtom_entry *: ccc_rom_insert_entry)(                               \
        (container_entry_ptr), (key_val_container_handle_ptr))

#define ccc_impl_or_insert(container_entry_ptr, key_val_container_handle_ptr)  \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry *: ccc_fhm_or_insert,                                 \
        ccc_o_map_entry *: ccc_om_or_insert,                                   \
        ccc_rtom_entry *: ccc_rom_or_insert)((container_entry_ptr),            \
                                             (key_val_container_handle_ptr))

#define ccc_impl_unwrap(container_entry_ptr)                                   \
    _Generic((container_entry_ptr),                                            \
        ccc_entry *: ccc_entry_unwrap,                                         \
        ccc_fh_map_entry *: ccc_fhm_unwrap,                                    \
        ccc_o_map_entry *: ccc_om_unwrap,                                      \
        ccc_rtom_entry *: ccc_rom_unwrap)((container_entry_ptr))

#define ccc_impl_occupied(container_entry_ptr)                                 \
    _Generic((container_entry_ptr),                                            \
        ccc_entry *: ccc_entry_occupied,                                       \
        ccc_fh_map_entry *: ccc_fhm_occupied,                                  \
        ccc_o_map_entry *: ccc_om_occupied,                                    \
        ccc_rtom_entry *: ccc_rom_occupied)((container_entry_ptr))

#define ccc_impl_insert_error(container_entry_ptr)                             \
    _Generic((container_entry_ptr),                                            \
        ccc_entry *: ccc_entry_insert_error,                                   \
        ccc_fh_map_entry *: ccc_fhm_insert_error,                              \
        ccc_o_map_entry *: ccc_om_insert_error,                                \
        ccc_rtom_entry *: ccc_rom_insert_error)((container_entry_ptr))

#endif /* CCC_IMPL_TRAITS_H */
