#ifndef CCC_IMPL_TRAITS_H
#define CCC_IMPL_TRAITS_H

/* NOLINTBEGIN */
#include "buffer.h"
#include "doubly_linked_list.h"
#include "flat_double_ended_queue.h"
#include "flat_hash_map.h"
#include "flat_ordered_map.h"
#include "flat_priority_queue.h"
#include "flat_realtime_ordered_map.h"
#include "ordered_map.h"
#include "ordered_multimap.h"
#include "priority_queue.h"
#include "realtime_ordered_map.h"
#include "singly_linked_list.h"
#include "types.h"
/* NOLINTEND */

/*======================     Entry Interface
 * =====================================*/

#define ccc_impl_insert(container_ptr, insert_args...)                         \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_insert,                                   \
        ccc_ordered_map *: ccc_om_insert,                                      \
        ccc_ordered_multimap *: ccc_omm_insert,                                \
        ccc_flat_ordered_map *: ccc_fom_insert,                                \
        ccc_flat_realtime_ordered_map *: ccc_frm_insert,                       \
        ccc_realtime_ordered_map *: ccc_rom_insert)((container_ptr),           \
                                                    insert_args)

#define ccc_impl_insert_r(container_ptr, key_val_container_handle_ptr...)      \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_insert(container_ptr, key_val_container_handle_ptr).impl_     \
    }

#define ccc_impl_try_insert(container_ptr, try_insert_args...)                 \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_try_insert,                               \
        ccc_ordered_map *: ccc_om_try_insert,                                  \
        ccc_ordered_multimap *: ccc_omm_try_insert,                            \
        ccc_flat_ordered_map *: ccc_fom_try_insert,                            \
        ccc_flat_realtime_ordered_map *: ccc_frm_try_insert,                   \
        ccc_realtime_ordered_map *: ccc_rom_try_insert)((container_ptr),       \
                                                        try_insert_args)

#define ccc_impl_try_insert_r(container_ptr, try_insert_args...)               \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_try_insert(container_ptr, try_insert_args).impl_              \
    }

#define ccc_impl_insert_or_assign(container_ptr, insert_or_assign_args...)     \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_insert_or_assign,                         \
        ccc_ordered_map *: ccc_om_insert_or_assign,                            \
        ccc_ordered_multimap *: ccc_omm_insert_or_assign,                      \
        ccc_flat_ordered_map *: ccc_fom_insert_or_assign,                      \
        ccc_flat_realtime_ordered_map *: ccc_frm_insert_or_assign,             \
        ccc_realtime_ordered_map *: ccc_rom_insert_or_assign)(                 \
        (container_ptr), insert_or_assign_args)

#define ccc_impl_insert_or_assign_r(container_ptr, insert_or_assign_args...)   \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_insert_or_assign(container_ptr, insert_or_assign_args).impl_  \
    }

#define ccc_impl_remove(container_ptr, key_val_container_handle_ptr...)        \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_remove,                                   \
        ccc_ordered_map *: ccc_om_remove,                                      \
        ccc_ordered_multimap *: ccc_omm_remove,                                \
        ccc_flat_ordered_map *: ccc_fom_remove,                                \
        ccc_flat_realtime_ordered_map *: ccc_frm_remove,                       \
        ccc_realtime_ordered_map                                               \
            *: ccc_rom_remove)((container_ptr), key_val_container_handle_ptr)

#define ccc_impl_remove_r(container_ptr, key_val_container_handle_ptr...)      \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_remove(container_ptr, key_val_container_handle_ptr).impl_     \
    }

#define ccc_impl_remove_entry(container_entry_ptr)                             \
    _Generic((container_entry_ptr),                                            \
        ccc_fhmap_entry *: ccc_fhm_remove_entry,                               \
        ccc_omap_entry *: ccc_om_remove_entry,                                 \
        ccc_ommap_entry *: ccc_omm_remove_entry,                               \
        ccc_fomap_entry *: ccc_fom_remove_entry,                               \
        ccc_fromap_entry *: ccc_frm_remove_entry,                              \
        ccc_romap_entry *: ccc_rom_remove_entry,                               \
        ccc_fhmap_entry const *: ccc_fhm_remove_entry,                         \
        ccc_fromap_entry const *: ccc_frm_remove_entry,                        \
        ccc_omap_entry const *: ccc_om_remove_entry,                           \
        ccc_fomap_entry const *: ccc_fom_remove_entry,                         \
        ccc_romap_entry const *: ccc_rom_remove_entry)((container_entry_ptr))

#define ccc_impl_remove_entry_r(container_entry_ptr)                           \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_remove_entry(container_entry_ptr).impl_                       \
    }

#define ccc_impl_entry(container_ptr, key_ptr...)                              \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_entry,                                    \
        ccc_flat_hash_map const *: ccc_fhm_entry,                              \
        ccc_ordered_map *: ccc_om_entry,                                       \
        ccc_ordered_multimap *: ccc_omm_entry,                                 \
        ccc_flat_ordered_map *: ccc_fom_entry,                                 \
        ccc_flat_realtime_ordered_map *: ccc_frm_entry,                        \
        ccc_flat_realtime_ordered_map const *: ccc_frm_entry,                  \
        ccc_realtime_ordered_map *: ccc_rom_entry,                             \
        ccc_realtime_ordered_map const *: ccc_rom_entry)((container_ptr),      \
                                                         key_ptr)

#define ccc_impl_entry_r(container_ptr, key_ptr...)                            \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map                                                      \
            *: &(ccc_fhmap_entry){ccc_fhm_entry(                               \
                                      (ccc_flat_hash_map *)(container_ptr),    \
                                      key_ptr)                                 \
                                      .impl_},                                 \
        ccc_flat_hash_map const *: &(                                          \
                 ccc_fhmap_entry){ccc_fhm_entry(                               \
                                      (ccc_flat_hash_map *)(container_ptr),    \
                                      key_ptr)                                 \
                                      .impl_},                                 \
        ccc_ordered_map *: &(                                                  \
                 ccc_omap_entry){ccc_om_entry(                                 \
                                     (ccc_ordered_map *)(container_ptr),       \
                                     key_ptr)                                  \
                                     .impl_},                                  \
        ccc_ordered_multimap *: &(                                             \
                 ccc_ommap_entry){ccc_omm_entry(                               \
                                      (ccc_ordered_multimap *)(container_ptr), \
                                      key_ptr)                                 \
                                      .impl_},                                 \
        ccc_flat_ordered_map *: &(                                             \
                 ccc_fomap_entry){ccc_fom_entry(                               \
                                      (ccc_flat_ordered_map *)(container_ptr), \
                                      key_ptr)                                 \
                                      .impl_},                                 \
        ccc_flat_realtime_ordered_map *: &(                                    \
                 ccc_fromap_entry){ccc_frm_entry(                              \
                                       (ccc_flat_realtime_ordered_map          \
                                            *)(container_ptr),                 \
                                       key_ptr)                                \
                                       .impl_},                                \
        ccc_flat_realtime_ordered_map const *: &(                              \
                 ccc_fromap_entry){ccc_frm_entry(                              \
                                       (ccc_flat_realtime_ordered_map          \
                                            *)(container_ptr),                 \
                                       key_ptr)                                \
                                       .impl_},                                \
        ccc_realtime_ordered_map *: &(                                         \
                 ccc_romap_entry){ccc_rom_entry((ccc_realtime_ordered_map      \
                                                     *)(container_ptr),        \
                                                key_ptr)                       \
                                      .impl_},                                 \
        ccc_realtime_ordered_map const *: &(ccc_romap_entry){                  \
            ccc_rom_entry((ccc_realtime_ordered_map *)(container_ptr),         \
                          key_ptr)                                             \
                .impl_})

#define ccc_impl_and_modify(container_entry_ptr, mod_fn)                       \
    _Generic((container_entry_ptr),                                            \
        ccc_fhmap_entry *: ccc_fhm_and_modify,                                 \
        ccc_omap_entry *: ccc_om_and_modify,                                   \
        ccc_ommap_entry *: ccc_omm_and_modify,                                 \
        ccc_fomap_entry *: ccc_fom_and_modify,                                 \
        ccc_romap_entry *: ccc_rom_and_modify,                                 \
        ccc_fromap_entry *: ccc_frm_and_modify,                                \
        ccc_fhmap_entry const *: ccc_fhm_and_modify,                           \
        ccc_fromap_entry const *: ccc_frm_and_modify,                          \
        ccc_omap_entry const *: ccc_om_and_modify,                             \
        ccc_ommap_entry const *: ccc_omm_and_modify,                           \
        ccc_fomap_entry const *: ccc_fom_and_modify,                           \
        ccc_romap_entry const *: ccc_rom_and_modify)((container_entry_ptr),    \
                                                     (mod_fn))

#define ccc_impl_and_modify_aux(container_entry_ptr, mod_fn, aux_data_ptr...)  \
    _Generic((container_entry_ptr),                                            \
        ccc_fhmap_entry *: ccc_fhm_and_modify_aux,                             \
        ccc_omap_entry *: ccc_om_and_modify_aux,                               \
        ccc_ommap_entry *: ccc_omm_and_modify_aux,                             \
        ccc_fomap_entry *: ccc_fom_and_modify_aux,                             \
        ccc_fromap_entry *: ccc_frm_and_modify_aux,                            \
        ccc_romap_entry *: ccc_rom_and_modify_aux,                             \
        ccc_fhmap_entry const *: ccc_fhm_and_modify_aux,                       \
        ccc_omap_entry const *: ccc_om_and_modify_aux,                         \
        ccc_ommap_entry const *: ccc_omm_and_modify_aux,                       \
        ccc_fromap_entry const *: ccc_frm_and_modify_aux,                      \
        ccc_fomap_entry const *: ccc_fom_and_modify_aux,                       \
        ccc_romap_entry const *: ccc_rom_and_modify_aux)(                      \
        (container_entry_ptr), (mod_fn), aux_data_ptr)

#define ccc_impl_insert_entry(container_entry_ptr,                             \
                              key_val_container_handle_ptr...)                 \
    _Generic((container_entry_ptr),                                            \
        ccc_fhmap_entry *: ccc_fhm_insert_entry,                               \
        ccc_omap_entry *: ccc_om_insert_entry,                                 \
        ccc_ommap_entry *: ccc_omm_insert_entry,                               \
        ccc_fomap_entry *: ccc_fom_insert_entry,                               \
        ccc_romap_entry *: ccc_rom_insert_entry,                               \
        ccc_fromap_entry *: ccc_frm_insert_entry,                              \
        ccc_fhmap_entry const *: ccc_fhm_insert_entry,                         \
        ccc_omap_entry const *: ccc_om_insert_entry,                           \
        ccc_ommap_entry const *: ccc_omm_insert_entry,                         \
        ccc_fomap_entry const *: ccc_fom_insert_entry,                         \
        ccc_fromap_entry const *: ccc_frm_insert_entry,                        \
        ccc_romap_entry const *: ccc_rom_insert_entry)(                        \
        (container_entry_ptr), key_val_container_handle_ptr)

#define ccc_impl_or_insert(container_entry_ptr,                                \
                           key_val_container_handle_ptr...)                    \
    _Generic((container_entry_ptr),                                            \
        ccc_fhmap_entry *: ccc_fhm_or_insert,                                  \
        ccc_omap_entry *: ccc_om_or_insert,                                    \
        ccc_ommap_entry *: ccc_omm_or_insert,                                  \
        ccc_fomap_entry *: ccc_fom_or_insert,                                  \
        ccc_romap_entry *: ccc_rom_or_insert,                                  \
        ccc_fromap_entry *: ccc_frm_or_insert,                                 \
        ccc_fhmap_entry const *: ccc_fhm_or_insert,                            \
        ccc_omap_entry const *: ccc_om_or_insert,                              \
        ccc_ommap_entry const *: ccc_omm_or_insert,                            \
        ccc_fromap_entry const *: ccc_frm_or_insert,                           \
        ccc_fomap_entry const *: ccc_fom_or_insert,                            \
        ccc_romap_entry const *: ccc_rom_or_insert)(                           \
        (container_entry_ptr), key_val_container_handle_ptr)

#define ccc_impl_unwrap(container_entry_ptr)                                   \
    _Generic((container_entry_ptr),                                            \
        ccc_entry *: ccc_entry_unwrap,                                         \
        ccc_entry const *: ccc_entry_unwrap,                                   \
        ccc_fhmap_entry *: ccc_fhm_unwrap,                                     \
        ccc_fhmap_entry const *: ccc_fhm_unwrap,                               \
        ccc_omap_entry *: ccc_om_unwrap,                                       \
        ccc_omap_entry const *: ccc_om_unwrap,                                 \
        ccc_ommap_entry *: ccc_omm_unwrap,                                     \
        ccc_ommap_entry const *: ccc_omm_unwrap,                               \
        ccc_fomap_entry *: ccc_fom_unwrap,                                     \
        ccc_fomap_entry const *: ccc_fom_unwrap,                               \
        ccc_fromap_entry *: ccc_frm_unwrap,                                    \
        ccc_fromap_entry const *: ccc_frm_unwrap,                              \
        ccc_romap_entry *: ccc_rom_unwrap,                                     \
        ccc_romap_entry const *: ccc_rom_unwrap)((container_entry_ptr))

#define ccc_impl_occupied(container_entry_ptr)                                 \
    _Generic((container_entry_ptr),                                            \
        ccc_entry *: ccc_entry_occupied,                                       \
        ccc_entry const *: ccc_entry_occupied,                                 \
        ccc_fhmap_entry *: ccc_fhm_occupied,                                   \
        ccc_fhmap_entry const *: ccc_fhm_occupied,                             \
        ccc_omap_entry *: ccc_om_occupied,                                     \
        ccc_omap_entry const *: ccc_om_occupied,                               \
        ccc_ommap_entry *: ccc_omm_occupied,                                   \
        ccc_ommap_entry const *: ccc_omm_occupied,                             \
        ccc_fomap_entry *: ccc_fom_occupied,                                   \
        ccc_fomap_entry const *: ccc_fom_occupied,                             \
        ccc_fromap_entry *: ccc_frm_occupied,                                  \
        ccc_fromap_entry const *: ccc_frm_occupied,                            \
        ccc_romap_entry *: ccc_rom_occupied,                                   \
        ccc_romap_entry const *: ccc_rom_occupied)((container_entry_ptr))

#define ccc_impl_insert_error(container_entry_ptr)                             \
    _Generic((container_entry_ptr),                                            \
        ccc_entry *: ccc_entry_insert_error,                                   \
        ccc_entry const *: ccc_entry_insert_error,                             \
        ccc_fhmap_entry *: ccc_fhm_insert_error,                               \
        ccc_fhmap_entry const *: ccc_fhm_insert_error,                         \
        ccc_omap_entry *: ccc_om_insert_error,                                 \
        ccc_omap_entry const *: ccc_om_insert_error,                           \
        ccc_ommap_entry *: ccc_omm_insert_error,                               \
        ccc_ommap_entry const *: ccc_omm_insert_error,                         \
        ccc_fomap_entry *: ccc_fom_insert_error,                               \
        ccc_fomap_entry const *: ccc_fom_insert_error,                         \
        ccc_fromap_entry *: ccc_frm_insert_error,                              \
        ccc_fromap_entry const *: ccc_frm_insert_error,                        \
        ccc_romap_entry *: ccc_rom_insert_error,                               \
        ccc_romap_entry const *: ccc_rom_insert_error)((container_entry_ptr))

/*======================    Misc Search Interface
 * ================================*/

#define ccc_impl_get_key_val(container_ptr, key_ptr...)                        \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_get_key_val,                              \
        ccc_flat_hash_map const *: ccc_fhm_get_key_val,                        \
        ccc_ordered_map *: ccc_om_get_key_val,                                 \
        ccc_ordered_multimap *: ccc_omm_get_key_val,                           \
        ccc_flat_ordered_map *: ccc_fom_get_key_val,                           \
        ccc_flat_realtime_ordered_map *: ccc_frm_get_key_val,                  \
        ccc_flat_realtime_ordered_map const *: ccc_frm_get_key_val,            \
        ccc_realtime_ordered_map *: ccc_rom_get_key_val,                       \
        ccc_realtime_ordered_map const *: ccc_rom_get_key_val)(                \
        (container_ptr), key_ptr)

#define ccc_impl_contains(container_ptr, key_ptr...)                           \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_contains,                                 \
        ccc_flat_hash_map const *: ccc_fhm_contains,                           \
        ccc_ordered_map *: ccc_om_contains,                                    \
        ccc_flat_ordered_map *: ccc_fom_contains,                              \
        ccc_flat_realtime_ordered_map *: ccc_frm_contains,                     \
        ccc_flat_realtime_ordered_map const *: ccc_frm_contains,               \
        ccc_realtime_ordered_map *: ccc_rom_contains,                          \
        ccc_realtime_ordered_map const *: ccc_rom_contains,                    \
        ccc_ordered_multimap *: ccc_omm_contains)((container_ptr), key_ptr)

/*================       Sequential Containers Interface =====================*/

#define ccc_impl_push(container_ptr, container_handle_ptr...)                  \
    _Generic((container_ptr),                                                  \
        ccc_flat_priority_queue *: ccc_fpq_push,                               \
        ccc_priority_queue *: ccc_pq_push)((container_ptr),                    \
                                           container_handle_ptr)

#define ccc_impl_push_back(container_ptr, container_handle_ptr...)             \
    _Generic((container_ptr),                                                  \
        ccc_flat_double_ended_queue *: ccc_fdeq_push_back,                     \
        ccc_doubly_linked_list *: ccc_dll_push_back,                           \
        ccc_buffer *: ccc_buf_push_back)((container_ptr),                      \
                                         container_handle_ptr)

#define ccc_impl_push_front(container_ptr, container_handle_ptr...)            \
    _Generic((container_ptr),                                                  \
        ccc_flat_double_ended_queue *: ccc_fdeq_push_front,                    \
        ccc_doubly_linked_list *: ccc_dll_push_front,                          \
        ccc_singly_linked_list *: ccc_sll_push_front)((container_ptr),         \
                                                      container_handle_ptr)

#define ccc_impl_pop(container_ptr)                                            \
    _Generic((container_ptr),                                                  \
        ccc_flat_priority_queue *: ccc_fpq_pop,                                \
        ccc_priority_queue *: ccc_pq_pop)((container_ptr))

#define ccc_impl_pop_front(container_ptr)                                      \
    _Generic((container_ptr),                                                  \
        ccc_flat_double_ended_queue *: ccc_fdeq_pop_front,                     \
        ccc_doubly_linked_list *: ccc_dll_pop_front,                           \
        ccc_singly_linked_list *: ccc_sll_pop_front)((container_ptr))

#define ccc_impl_pop_back(container_ptr)                                       \
    _Generic((container_ptr),                                                  \
        ccc_flat_double_ended_queue *: ccc_fdeq_pop_back,                      \
        ccc_doubly_linked_list *: ccc_dll_pop_back,                            \
        ccc_buffer *: ccc_buf_pop_back)((container_ptr))

#define ccc_impl_front(container_ptr)                                          \
    _Generic((container_ptr),                                                  \
        ccc_flat_double_ended_queue *: ccc_fdeq_front,                         \
        ccc_doubly_linked_list *: ccc_dll_front,                               \
        ccc_flat_priority_queue *: ccc_fpq_front,                              \
        ccc_priority_queue *: ccc_pq_front,                                    \
        ccc_singly_linked_list *: ccc_sll_front,                               \
        ccc_flat_double_ended_queue const *: ccc_fdeq_front,                   \
        ccc_doubly_linked_list const *: ccc_dll_front,                         \
        ccc_flat_priority_queue const *: ccc_fpq_front,                        \
        ccc_priority_queue const *: ccc_pq_front,                              \
        ccc_singly_linked_list const *: ccc_sll_front)((container_ptr))

#define ccc_impl_back(container_ptr)                                           \
    _Generic((container_ptr),                                                  \
        ccc_flat_double_ended_queue *: ccc_fdeq_back,                          \
        ccc_doubly_linked_list *: ccc_dll_back,                                \
        ccc_buffer *: ccc_buf_back,                                            \
        ccc_flat_double_ended_queue const *: ccc_fdeq_back,                    \
        ccc_doubly_linked_list const *: ccc_dll_back,                          \
        ccc_buffer const *: ccc_buf_back)((container_ptr))

/*================       Priority Queue Update Interface =====================*/

#define ccc_impl_update(container_ptr, update_args...)                         \
    _Generic((container_ptr),                                                  \
        ccc_ordered_multimap *: ccc_omm_update,                                \
        ccc_flat_priority_queue *: ccc_fpq_update,                             \
        ccc_priority_queue *: ccc_pq_update)((container_ptr), update_args)

#define ccc_impl_increase(container_ptr, increase_args...)                     \
    _Generic((container_ptr),                                                  \
        ccc_ordered_multimap *: ccc_omm_increase,                              \
        ccc_flat_priority_queue *: ccc_fpq_increase,                           \
        ccc_priority_queue *: ccc_pq_increase)((container_ptr), increase_args)

#define ccc_impl_decrease(container_ptr, decrease_args...)                     \
    _Generic((container_ptr),                                                  \
        ccc_ordered_multimap *: ccc_omm_decrease,                              \
        ccc_flat_priority_queue *: ccc_fpq_decrease,                           \
        ccc_priority_queue *: ccc_pq_decrease)((container_ptr), decrease_args)

#define ccc_impl_extract(container_ptr, container_handle_ptr...)               \
    _Generic((container_ptr),                                                  \
        ccc_ordered_multimap *: ccc_omm_extract,                               \
        ccc_doubly_linked_list *: ccc_dll_extract,                             \
        ccc_singly_linked_list *: ccc_sll_extract,                             \
        ccc_priority_queue *: ccc_pq_extract)((container_ptr),                 \
                                              container_handle_ptr)

#define ccc_impl_erase(container_ptr, container_handle_ptr...)                 \
    _Generic((container_ptr), ccc_flat_priority_queue *: ccc_fpq_erase)(       \
        (container_ptr), container_handle_ptr)

#define ccc_impl_extract_range(container_ptr,                                  \
                               container_handle_begin_end_ptr...)              \
    _Generic((container_ptr),                                                  \
        ccc_doubly_linked_list *: ccc_dll_extract_range,                       \
        ccc_singly_linked_list *: ccc_sll_extract_range)(                      \
        (container_ptr), container_handle_begin_end_ptr)

/*===================       Iterators Interface ==============================*/

#define ccc_impl_begin(container_ptr)                                          \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_begin,                                           \
        ccc_flat_hash_map *: ccc_fhm_begin,                                    \
        ccc_ordered_map *: ccc_om_begin,                                       \
        ccc_flat_ordered_map *: ccc_fom_begin,                                 \
        ccc_flat_double_ended_queue *: ccc_fdeq_begin,                         \
        ccc_ordered_multimap *: ccc_omm_begin,                                 \
        ccc_singly_linked_list *: ccc_sll_begin,                               \
        ccc_doubly_linked_list *: ccc_dll_begin,                               \
        ccc_realtime_ordered_map *: ccc_rom_begin,                             \
        ccc_flat_realtime_ordered_map *: ccc_frm_begin,                        \
        ccc_buffer const *: ccc_buf_begin,                                     \
        ccc_flat_hash_map const *: ccc_fhm_begin,                              \
        ccc_ordered_map const *: ccc_om_begin,                                 \
        ccc_flat_ordered_map const *: ccc_fom_begin,                           \
        ccc_flat_double_ended_queue const *: ccc_fdeq_begin,                   \
        ccc_ordered_multimap const *: ccc_omm_begin,                           \
        ccc_singly_linked_list const *: ccc_sll_begin,                         \
        ccc_doubly_linked_list const *: ccc_dll_begin,                         \
        ccc_flat_realtime_ordered_map const *: ccc_frm_begin,                  \
        ccc_realtime_ordered_map const *: ccc_rom_begin)((container_ptr))

#define ccc_impl_rbegin(container_ptr)                                         \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_rbegin,                                          \
        ccc_ordered_map *: ccc_om_rbegin,                                      \
        ccc_flat_ordered_map *: ccc_fom_rbegin,                                \
        ccc_flat_double_ended_queue *: ccc_fdeq_rbegin,                        \
        ccc_ordered_multimap *: ccc_omm_rbegin,                                \
        ccc_doubly_linked_list *: ccc_dll_rbegin,                              \
        ccc_realtime_ordered_map *: ccc_rom_rbegin,                            \
        ccc_flat_realtime_ordered_map *: ccc_frm_rbegin,                       \
        ccc_buffer const *: ccc_buf_rbegin,                                    \
        ccc_ordered_map const *: ccc_om_rbegin,                                \
        ccc_flat_ordered_map const *: ccc_fom_rbegin,                          \
        ccc_flat_double_ended_queue const *: ccc_fdeq_rbegin,                  \
        ccc_ordered_multimap const *: ccc_omm_rbegin,                          \
        ccc_doubly_linked_list const *: ccc_dll_rbegin,                        \
        ccc_flat_realtime_ordered_map const *: ccc_frm_rbegin,                 \
        ccc_realtime_ordered_map const *: ccc_rom_rbegin)((container_ptr))

#define ccc_impl_next(container_ptr, void_iter_ptr)                            \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_next,                                            \
        ccc_flat_hash_map *: ccc_fhm_next,                                     \
        ccc_ordered_map *: ccc_om_next,                                        \
        ccc_flat_ordered_map *: ccc_fom_next,                                  \
        ccc_flat_double_ended_queue *: ccc_fdeq_next,                          \
        ccc_ordered_multimap *: ccc_omm_next,                                  \
        ccc_singly_linked_list *: ccc_sll_next,                                \
        ccc_doubly_linked_list *: ccc_dll_next,                                \
        ccc_realtime_ordered_map *: ccc_rom_next,                              \
        ccc_flat_realtime_ordered_map *: ccc_frm_next,                         \
        ccc_buffer const *: ccc_buf_next,                                      \
        ccc_flat_hash_map const *: ccc_fhm_next,                               \
        ccc_ordered_map const *: ccc_om_next,                                  \
        ccc_flat_ordered_map const *: ccc_fom_next,                            \
        ccc_flat_double_ended_queue const *: ccc_fdeq_next,                    \
        ccc_ordered_multimap const *: ccc_omm_next,                            \
        ccc_singly_linked_list const *: ccc_sll_next,                          \
        ccc_doubly_linked_list const *: ccc_dll_next,                          \
        ccc_flat_realtime_ordered_map const *: ccc_frm_next,                   \
        ccc_realtime_ordered_map const *: ccc_rom_next)((container_ptr),       \
                                                        (void_iter_ptr))

#define ccc_impl_rnext(container_ptr, void_iter_ptr)                           \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_rnext,                                           \
        ccc_ordered_map *: ccc_om_rnext,                                       \
        ccc_flat_ordered_map *: ccc_fom_rnext,                                 \
        ccc_flat_double_ended_queue *: ccc_fdeq_rnext,                         \
        ccc_ordered_multimap *: ccc_omm_rnext,                                 \
        ccc_doubly_linked_list *: ccc_dll_rnext,                               \
        ccc_realtime_ordered_map *: ccc_rom_rnext,                             \
        ccc_flat_realtime_ordered_map *: ccc_frm_rnext,                        \
        ccc_buffer const *: ccc_buf_rnext,                                     \
        ccc_ordered_map const *: ccc_om_rnext,                                 \
        ccc_flat_ordered_map const *: ccc_fom_rnext,                           \
        ccc_flat_double_ended_queue const *: ccc_fdeq_rnext,                   \
        ccc_ordered_multimap const *: ccc_omm_rnext,                           \
        ccc_doubly_linked_list const *: ccc_dll_rnext,                         \
        ccc_flat_realtime_ordered_map const *: ccc_frm_rnext,                  \
        ccc_realtime_ordered_map const *: ccc_rom_rnext)((container_ptr),      \
                                                         (void_iter_ptr))

#define ccc_impl_end(container_ptr)                                            \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_end,                                             \
        ccc_flat_hash_map *: ccc_fhm_end,                                      \
        ccc_ordered_map *: ccc_om_end,                                         \
        ccc_flat_ordered_map *: ccc_fom_end,                                   \
        ccc_flat_double_ended_queue *: ccc_fdeq_end,                           \
        ccc_ordered_multimap *: ccc_omm_end,                                   \
        ccc_singly_linked_list *: ccc_sll_end,                                 \
        ccc_doubly_linked_list *: ccc_dll_end,                                 \
        ccc_realtime_ordered_map *: ccc_rom_end,                               \
        ccc_flat_realtime_ordered_map *: ccc_frm_end,                          \
        ccc_buffer const *: ccc_buf_end,                                       \
        ccc_flat_hash_map const *: ccc_fhm_end,                                \
        ccc_ordered_map const *: ccc_om_end,                                   \
        ccc_flat_ordered_map const *: ccc_fom_end,                             \
        ccc_flat_double_ended_queue const *: ccc_fdeq_end,                     \
        ccc_ordered_multimap const *: ccc_omm_end,                             \
        ccc_singly_linked_list const *: ccc_sll_end,                           \
        ccc_doubly_linked_list const *: ccc_dll_end,                           \
        ccc_flat_realtime_ordered_map const *: ccc_frm_end,                    \
        ccc_realtime_ordered_map const *: ccc_rom_end)((container_ptr))

#define ccc_impl_rend(container_ptr)                                           \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_rend,                                            \
        ccc_ordered_map *: ccc_om_rend,                                        \
        ccc_flat_ordered_map *: ccc_fom_rend,                                  \
        ccc_flat_double_ended_queue *: ccc_fdeq_rend,                          \
        ccc_ordered_multimap *: ccc_omm_rend,                                  \
        ccc_doubly_linked_list *: ccc_dll_rend,                                \
        ccc_realtime_ordered_map *: ccc_rom_rend,                              \
        ccc_flat_realtime_ordered_map *: ccc_frm_rend,                         \
        ccc_buffer const *: ccc_buf_rend,                                      \
        ccc_ordered_map const *: ccc_om_rend,                                  \
        ccc_flat_ordered_map const *: ccc_fom_rend,                            \
        ccc_flat_double_ended_queue const *: ccc_fdeq_rend,                    \
        ccc_ordered_multimap const *: ccc_omm_rend,                            \
        ccc_doubly_linked_list const *: ccc_dll_rend,                          \
        ccc_flat_realtime_ordered_map const *: ccc_frm_rend,                   \
        ccc_realtime_ordered_map const *: ccc_rom_rend)((container_ptr))

#define ccc_impl_equal_range(container_ptr, begin_and_end_key_ptr...)          \
    _Generic((container_ptr),                                                  \
        ccc_ordered_map *: ccc_om_equal_range,                                 \
        ccc_flat_ordered_map *: ccc_fom_equal_range,                           \
        ccc_ordered_multimap *: ccc_omm_equal_range,                           \
        ccc_flat_realtime_ordered_map *: ccc_frm_equal_range,                  \
        ccc_flat_realtime_ordered_map const *: ccc_frm_equal_range,            \
        ccc_realtime_ordered_map *: ccc_rom_equal_range,                       \
        ccc_realtime_ordered_map const *: ccc_rom_equal_range)(                \
        (container_ptr), begin_and_end_key_ptr)

#define ccc_impl_equal_range_r(container_ptr, begin_and_end_key_ptr...)        \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_impl_equal_range(container_ptr, begin_and_end_key_ptr).impl_       \
    }

#define ccc_impl_equal_rrange(container_ptr, rbegin_and_rend_key_ptr...)       \
    _Generic((container_ptr),                                                  \
        ccc_ordered_map *: ccc_om_equal_rrange,                                \
        ccc_flat_ordered_map *: ccc_fom_equal_rrange,                          \
        ccc_ordered_multimap *: ccc_omm_equal_rrange,                          \
        ccc_flat_realtime_ordered_map *: ccc_frm_equal_rrange,                 \
        ccc_flat_realtime_ordered_map const *: ccc_frm_equal_rrange,           \
        ccc_realtime_ordered_map *: ccc_rom_equal_rrange,                      \
        ccc_realtime_ordered_map const *: ccc_rom_equal_rrange)(               \
        (container_ptr), rbegin_and_rend_key_ptr)

#define ccc_impl_equal_rrange_r(container_ptr, rbegin_and_rend_key_ptr...)     \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_impl_equal_rrange(container_ptr, rbegin_and_rend_key_ptr).impl_    \
    }

#define ccc_impl_splice(container_ptr, splice_args...)                         \
    _Generic((container_ptr),                                                  \
        ccc_singly_linked_list *: ccc_sll_splice,                              \
        ccc_singly_linked_list const *: ccc_sll_splice,                        \
        ccc_doubly_linked_list *: ccc_dll_splice,                              \
        ccc_doubly_linked_list const *: ccc_dll_splice)((container_ptr),       \
                                                        splice_args)

#define ccc_impl_splice_range(container_ptr, splice_range_args...)             \
    _Generic((container_ptr),                                                  \
        ccc_singly_linked_list *: ccc_sll_splice_range,                        \
        ccc_singly_linked_list const *: ccc_sll_splice_range,                  \
        ccc_doubly_linked_list *: ccc_dll_splice_range,                        \
        ccc_doubly_linked_list const *: ccc_dll_splice_range)(                 \
        (container_ptr), splice_range_args)

/*===================    Standard Getters Interface
 * ==============================*/

#define ccc_impl_size(container_ptr)                                           \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_size,                                            \
        ccc_flat_hash_map *: ccc_fhm_size,                                     \
        ccc_ordered_map *: ccc_om_size,                                        \
        ccc_flat_ordered_map *: ccc_fom_size,                                  \
        ccc_flat_priority_queue *: ccc_fpq_size,                               \
        ccc_flat_double_ended_queue *: ccc_fdeq_size,                          \
        ccc_ordered_multimap *: ccc_omm_size,                                  \
        ccc_priority_queue *: ccc_pq_size,                                     \
        ccc_singly_linked_list *: ccc_sll_size,                                \
        ccc_doubly_linked_list *: ccc_dll_size,                                \
        ccc_realtime_ordered_map *: ccc_rom_size,                              \
        ccc_flat_realtime_ordered_map *: ccc_frm_size,                         \
        ccc_buffer const *: ccc_buf_size,                                      \
        ccc_flat_hash_map const *: ccc_fhm_size,                               \
        ccc_ordered_map const *: ccc_om_size,                                  \
        ccc_flat_ordered_map const *: ccc_fom_size,                            \
        ccc_flat_priority_queue const *: ccc_fpq_size,                         \
        ccc_flat_double_ended_queue const *: ccc_fdeq_size,                    \
        ccc_ordered_multimap const *: ccc_omm_size,                            \
        ccc_priority_queue const *: ccc_pq_size,                               \
        ccc_singly_linked_list const *: ccc_sll_size,                          \
        ccc_doubly_linked_list const *: ccc_dll_size,                          \
        ccc_flat_realtime_ordered_map const *: ccc_frm_size,                   \
        ccc_realtime_ordered_map const *: ccc_rom_size)((container_ptr))

#define ccc_impl_is_empty(container_ptr)                                       \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_is_empty,                                        \
        ccc_flat_hash_map *: ccc_fhm_is_empty,                                 \
        ccc_ordered_map *: ccc_om_is_empty,                                    \
        ccc_flat_ordered_map *: ccc_fom_is_empty,                              \
        ccc_flat_priority_queue *: ccc_fpq_is_empty,                           \
        ccc_flat_double_ended_queue *: ccc_fdeq_is_empty,                      \
        ccc_ordered_multimap *: ccc_omm_is_empty,                              \
        ccc_priority_queue *: ccc_pq_is_empty,                                 \
        ccc_singly_linked_list *: ccc_sll_is_empty,                            \
        ccc_doubly_linked_list *: ccc_dll_is_empty,                            \
        ccc_realtime_ordered_map *: ccc_rom_is_empty,                          \
        ccc_flat_realtime_ordered_map *: ccc_frm_is_empty,                     \
        ccc_buffer const *: ccc_buf_is_empty,                                  \
        ccc_flat_hash_map const *: ccc_fhm_is_empty,                           \
        ccc_ordered_map const *: ccc_om_is_empty,                              \
        ccc_flat_ordered_map const *: ccc_fom_is_empty,                        \
        ccc_flat_priority_queue const *: ccc_fpq_is_empty,                     \
        ccc_flat_double_ended_queue const *: ccc_fdeq_is_empty,                \
        ccc_ordered_multimap const *: ccc_omm_is_empty,                        \
        ccc_priority_queue const *: ccc_pq_is_empty,                           \
        ccc_singly_linked_list const *: ccc_sll_is_empty,                      \
        ccc_doubly_linked_list const *: ccc_dll_is_empty,                      \
        ccc_flat_realtime_ordered_map const *: ccc_frm_is_empty,               \
        ccc_realtime_ordered_map const *: ccc_rom_is_empty)((container_ptr))

#define ccc_impl_validate(container_ptr)                                       \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_validate,                                 \
        ccc_ordered_map *: ccc_om_validate,                                    \
        ccc_flat_ordered_map *: ccc_fom_validate,                              \
        ccc_flat_priority_queue *: ccc_fpq_validate,                           \
        ccc_flat_double_ended_queue *: ccc_fdeq_validate,                      \
        ccc_ordered_multimap *: ccc_omm_validate,                              \
        ccc_priority_queue *: ccc_pq_validate,                                 \
        ccc_singly_linked_list *: ccc_sll_validate,                            \
        ccc_doubly_linked_list *: ccc_dll_validate,                            \
        ccc_realtime_ordered_map *: ccc_rom_validate,                          \
        ccc_flat_realtime_ordered_map *: ccc_frm_validate,                     \
        ccc_flat_hash_map const *: ccc_fhm_validate,                           \
        ccc_ordered_map const *: ccc_om_validate,                              \
        ccc_flat_ordered_map const *: ccc_fom_validate,                        \
        ccc_flat_priority_queue const *: ccc_fpq_validate,                     \
        ccc_flat_double_ended_queue const *: ccc_fdeq_validate,                \
        ccc_ordered_multimap const *: ccc_omm_validate,                        \
        ccc_priority_queue const *: ccc_pq_validate,                           \
        ccc_singly_linked_list const *: ccc_sll_validate,                      \
        ccc_doubly_linked_list const *: ccc_dll_validate,                      \
        ccc_flat_realtime_ordered_map const *: ccc_frm_validate,               \
        ccc_realtime_ordered_map const *: ccc_rom_validate)((container_ptr))

#define ccc_impl_print(container_ptr, printer_fn)                              \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_print,                                    \
        ccc_ordered_map *: ccc_om_print,                                       \
        ccc_flat_ordered_map *: ccc_fom_print,                                 \
        ccc_flat_priority_queue *: ccc_fpq_print,                              \
        ccc_flat_double_ended_queue *: ccc_fdeq_print,                         \
        ccc_ordered_multimap *: ccc_omm_print,                                 \
        ccc_priority_queue *: ccc_pq_print,                                    \
        ccc_singly_linked_list *: ccc_sll_print,                               \
        ccc_doubly_linked_list *: ccc_dll_print,                               \
        ccc_realtime_ordered_map *: ccc_rom_print,                             \
        ccc_flat_realtime_ordered_map *: ccc_frm_print,                        \
        ccc_flat_hash_map const *: ccc_fhm_print,                              \
        ccc_ordered_map const *: ccc_om_print,                                 \
        ccc_flat_ordered_map const *: ccc_fom_print,                           \
        ccc_flat_priority_queue const *: ccc_fpq_print,                        \
        ccc_flat_double_ended_queue const *: ccc_fdeq_print,                   \
        ccc_ordered_multimap const *: ccc_omm_print,                           \
        ccc_priority_queue const *: ccc_pq_print,                              \
        ccc_singly_linked_list const *: ccc_sll_print,                         \
        ccc_doubly_linked_list const *: ccc_dll_print,                         \
        ccc_flat_realtime_ordered_map const *: ccc_frm_print,                  \
        ccc_realtime_ordered_map const *: ccc_rom_print)((container_ptr),      \
                                                         (printer_fn))

#endif /* CCC_IMPL_TRAITS_H */
