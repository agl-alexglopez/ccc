#ifndef CCC_IMPL_TRAITS_H
#define CCC_IMPL_TRAITS_H

/* NOLINTBEGIN */
#include "buffer.h"
#include "double_ended_priority_queue.h"
#include "doubly_linked_list.h"
#include "flat_double_ended_queue.h"
#include "flat_hash_map.h"
#include "flat_priority_queue.h"
#include "flat_realtime_ordered_map.h"
#include "ordered_map.h"
#include "priority_queue.h"
#include "realtime_ordered_map.h"
#include "singly_linked_list.h"
#include "types.h"
/* NOLINTEND */

/*======================     Entry API  =====================================*/

#define ccc_impl_insert(container_ptr, insert_args...)                         \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_insert,                                   \
        ccc_ordered_map *: ccc_om_insert,                                      \
        ccc_flat_realtime_ordered_map *: ccc_frm_insert,                       \
        ccc_realtime_ordered_map *: ccc_rom_insert)((container_ptr),           \
                                                    insert_args)

#define ccc_impl_insert_vr(container_ptr, key_val_container_handle_ptr...)     \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_insert(container_ptr, key_val_container_handle_ptr).impl_     \
    }

#define ccc_impl_try_insert(container_ptr, try_insert_args...)                 \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_try_insert,                               \
        ccc_ordered_map *: ccc_om_try_insert,                                  \
        ccc_realtime_ordered_map *: ccc_rom_try_insert)((container_ptr),       \
                                                        try_insert_args)

#define ccc_impl_try_insert_vr(container_ptr, try_insert_args...)              \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_try_insert(container_ptr, try_insert_args).impl_              \
    }

#define ccc_impl_insert_or_assign(container_ptr, insert_or_assign_args...)     \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_insert_or_assign,                         \
        ccc_ordered_map *: ccc_om_insert_or_assign,                            \
        ccc_realtime_ordered_map *: ccc_rom_insert_or_assign)(                 \
        (container_ptr), insert_or_assign_args)

#define ccc_impl_insert_or_assign_vr(container_ptr, insert_or_assign_args...)  \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_insert_or_assign(container_ptr, insert_or_assign_args).impl_  \
    }

#define ccc_impl_remove(container_ptr, key_val_container_handle_ptr...)        \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_remove,                                   \
        ccc_ordered_map *: ccc_om_remove,                                      \
        ccc_realtime_ordered_map                                               \
            *: ccc_rom_remove)((container_ptr), key_val_container_handle_ptr)

#define ccc_impl_remove_vr(container_ptr, key_val_container_handle_ptr...)     \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_remove(container_ptr, key_val_container_handle_ptr).impl_     \
    }

#define ccc_impl_remove_entry(container_entry_ptr)                             \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry *: ccc_fhm_remove_entry,                              \
        ccc_o_map_entry *: ccc_om_remove_entry,                                \
        ccc_rtom_entry *: ccc_rom_remove_entry,                                \
        ccc_fh_map_entry const *: ccc_fhm_remove_entry,                        \
        ccc_o_map_entry const *: ccc_om_remove_entry,                          \
        ccc_rtom_entry const *: ccc_rom_remove_entry)((container_entry_ptr))

#define ccc_impl_remove_entry_vr(container_entry_ptr)                          \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_remove_entry(container_entry_ptr).impl_                       \
    }

#define ccc_impl_entry(container_ptr, key_ptr...)                              \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_entry,                                    \
        ccc_flat_hash_map const *: ccc_fhm_entry,                              \
        ccc_ordered_map *: ccc_om_entry,                                       \
        ccc_realtime_ordered_map *: ccc_rom_entry,                             \
        ccc_realtime_ordered_map const *: ccc_rom_entry)((container_ptr),      \
                                                         key_ptr)

#define ccc_impl_entry_vr(container_ptr, key_ptr...)                           \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map                                                      \
            *: &(ccc_fh_map_entry){ccc_fhm_entry(                              \
                                       (ccc_flat_hash_map *)(container_ptr),   \
                                       key_ptr)                                \
                                       .impl_},                                \
        ccc_flat_hash_map const *: &(                                          \
                 ccc_fh_map_entry){ccc_fhm_entry(                              \
                                       (ccc_flat_hash_map *)(container_ptr),   \
                                       key_ptr)                                \
                                       .impl_},                                \
        ccc_ordered_map *: &(                                                  \
                 ccc_o_map_entry){ccc_om_entry(                                \
                                      (ccc_ordered_map *)(container_ptr),      \
                                      key_ptr)                                 \
                                      .impl_},                                 \
        ccc_realtime_ordered_map *: &(                                         \
                 ccc_rtom_entry){ccc_rom_entry((ccc_realtime_ordered_map       \
                                                    *)(container_ptr),         \
                                               key_ptr)                        \
                                     .impl_},                                  \
        ccc_realtime_ordered_map const *: &(ccc_rtom_entry){                   \
            ccc_rom_entry((ccc_realtime_ordered_map *)(container_ptr),         \
                          key_ptr)                                             \
                .impl_})

#define ccc_impl_and_modify(container_entry_ptr, mod_fn)                       \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry *: ccc_fhm_and_modify,                                \
        ccc_o_map_entry *: ccc_om_and_modify,                                  \
        ccc_rtom_entry *: ccc_rom_and_modify,                                  \
        ccc_fh_map_entry const *: ccc_fhm_and_modify,                          \
        ccc_o_map_entry const *: ccc_om_and_modify,                            \
        ccc_rtom_entry const *: ccc_rom_and_modify)((container_entry_ptr),     \
                                                    (mod_fn))

#define ccc_impl_and_modify_aux(container_entry_ptr, mod_fn, aux_data_ptr...)  \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry *: ccc_fhm_and_modify_aux,                            \
        ccc_o_map_entry *: ccc_om_and_modify_aux,                              \
        ccc_rtom_entry *: ccc_rom_and_modify_aux,                              \
        ccc_fh_map_entry const *: ccc_fhm_and_modify_aux,                      \
        ccc_o_map_entry const *: ccc_om_and_modify_aux,                        \
        ccc_rtom_entry const *: ccc_rom_and_modify_aux)(                       \
        (container_entry_ptr), (mod_fn), aux_data_ptr)

#define ccc_impl_insert_entry(container_entry_ptr,                             \
                              key_val_container_handle_ptr...)                 \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry *: ccc_fhm_insert_entry,                              \
        ccc_o_map_entry *: ccc_om_insert_entry,                                \
        ccc_rtom_entry *: ccc_rom_insert_entry,                                \
        ccc_fh_map_entry const *: ccc_fhm_insert_entry,                        \
        ccc_o_map_entry const *: ccc_om_insert_entry,                          \
        ccc_rtom_entry const *: ccc_rom_insert_entry)(                         \
        (container_entry_ptr), key_val_container_handle_ptr)

#define ccc_impl_or_insert(container_entry_ptr,                                \
                           key_val_container_handle_ptr...)                    \
    _Generic((container_entry_ptr),                                            \
        ccc_fh_map_entry *: ccc_fhm_or_insert,                                 \
        ccc_o_map_entry *: ccc_om_or_insert,                                   \
        ccc_rtom_entry *: ccc_rom_or_insert,                                   \
        ccc_fh_map_entry const *: ccc_fhm_or_insert,                           \
        ccc_o_map_entry const *: ccc_om_or_insert,                             \
        ccc_rtom_entry const *: ccc_rom_or_insert)(                            \
        (container_entry_ptr), key_val_container_handle_ptr)

#define ccc_impl_unwrap(container_entry_ptr)                                   \
    _Generic((container_entry_ptr),                                            \
        ccc_entry *: ccc_entry_unwrap,                                         \
        ccc_entry const *: ccc_entry_unwrap,                                   \
        ccc_fh_map_entry *: ccc_fhm_unwrap,                                    \
        ccc_fh_map_entry const *: ccc_fhm_unwrap,                              \
        ccc_o_map_entry *: ccc_om_unwrap,                                      \
        ccc_o_map_entry const *: ccc_om_unwrap,                                \
        ccc_rtom_entry *: ccc_rom_unwrap,                                      \
        ccc_rtom_entry const *: ccc_rom_unwrap)((container_entry_ptr))

#define ccc_impl_occupied(container_entry_ptr)                                 \
    _Generic((container_entry_ptr),                                            \
        ccc_entry *: ccc_entry_occupied,                                       \
        ccc_entry const *: ccc_entry_occupied,                                 \
        ccc_fh_map_entry *: ccc_fhm_occupied,                                  \
        ccc_fh_map_entry const *: ccc_fhm_occupied,                            \
        ccc_o_map_entry *: ccc_om_occupied,                                    \
        ccc_o_map_entry const *: ccc_om_occupied,                              \
        ccc_rtom_entry *: ccc_rom_occupied,                                    \
        ccc_rtom_entry const *: ccc_rom_occupied)((container_entry_ptr))

#define ccc_impl_insert_error(container_entry_ptr)                             \
    _Generic((container_entry_ptr),                                            \
        ccc_entry *: ccc_entry_insert_error,                                   \
        ccc_entry const *: ccc_entry_insert_error,                             \
        ccc_fh_map_entry *: ccc_fhm_insert_error,                              \
        ccc_fh_map_entry const *: ccc_fhm_insert_error,                        \
        ccc_o_map_entry *: ccc_om_insert_error,                                \
        ccc_o_map_entry const *: ccc_om_insert_error,                          \
        ccc_rtom_entry *: ccc_rom_insert_error,                                \
        ccc_rtom_entry const *: ccc_rom_insert_error)((container_entry_ptr))

/*======================    Misc Search API  ================================*/

#define ccc_impl_get_key_val(container_ptr, key_ptr...)                        \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_get_key_val,                              \
        ccc_flat_hash_map const *: ccc_fhm_get_key_val,                        \
        ccc_ordered_map *: ccc_om_get_key_val,                                 \
        ccc_realtime_ordered_map *: ccc_rom_get_key_val,                       \
        ccc_realtime_ordered_map const *: ccc_rom_get_key_val)(                \
        (container_ptr), key_ptr)

#define ccc_impl_contains(container_ptr, key_ptr...)                           \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_contains,                                 \
        ccc_flat_hash_map const *: ccc_fhm_contains,                           \
        ccc_ordered_map *: ccc_om_contains,                                    \
        ccc_realtime_ordered_map *: ccc_rom_contains,                          \
        ccc_realtime_ordered_map const *: ccc_rom_contains,                    \
        ccc_double_ended_priority_queue *: ccc_depq_contains)((container_ptr), \
                                                              key_ptr)

/*================       Sequential Containers API =====================*/

#define ccc_impl_push(container_ptr, container_handle_ptr...)                  \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_push,                      \
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

/*================       Priority Queue Update API =====================*/

#define ccc_impl_update(container_ptr, update_args...)                         \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_update,                    \
        ccc_flat_priority_queue *: ccc_fpq_update,                             \
        ccc_priority_queue *: ccc_pq_update)((container_ptr), update_args)

#define ccc_impl_increase(container_ptr, increase_args...)                     \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_increase,                  \
        ccc_flat_priority_queue *: ccc_fpq_increase,                           \
        ccc_priority_queue *: ccc_pq_increase)((container_ptr), increase_args)

#define ccc_impl_decrease(container_ptr, decrease_args...)                     \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_decrease,                  \
        ccc_flat_priority_queue *: ccc_fpq_decrease,                           \
        ccc_priority_queue *: ccc_pq_decrease)((container_ptr), decrease_args)

#define ccc_impl_erase(container_ptr, container_handle_ptr...)                 \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_erase,                     \
        ccc_doubly_linked_list *: ccc_dll_erase,                               \
        ccc_singly_linked_list *: ccc_sll_erase,                               \
        ccc_flat_priority_queue *: ccc_fpq_erase,                              \
        ccc_priority_queue *: ccc_pq_erase)((container_ptr),                   \
                                            container_handle_ptr)

#define ccc_impl_erase_range(container_ptr, container_handle_begin_end_ptr...) \
    _Generic((container_ptr),                                                  \
        ccc_doubly_linked_list *: ccc_dll_erase_range,                         \
        ccc_singly_linked_list *: ccc_sll_erase_range)(                        \
        (container_ptr), container_handle_begin_end_ptr)

/*===================       Iterators API ==============================*/

#define ccc_impl_begin(container_ptr)                                          \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_begin,                                           \
        ccc_flat_hash_map *: ccc_fhm_begin,                                    \
        ccc_ordered_map *: ccc_om_begin,                                       \
        ccc_flat_double_ended_queue *: ccc_fdeq_begin,                         \
        ccc_double_ended_priority_queue *: ccc_depq_begin,                     \
        ccc_singly_linked_list *: ccc_sll_begin,                               \
        ccc_doubly_linked_list *: ccc_dll_begin,                               \
        ccc_realtime_ordered_map *: ccc_rom_begin,                             \
        ccc_flat_realtime_ordered_map *: ccc_frm_begin,                        \
        ccc_buffer const *: ccc_buf_begin,                                     \
        ccc_flat_hash_map const *: ccc_fhm_begin,                              \
        ccc_ordered_map const *: ccc_om_begin,                                 \
        ccc_flat_double_ended_queue const *: ccc_fdeq_begin,                   \
        ccc_double_ended_priority_queue const *: ccc_depq_begin,               \
        ccc_singly_linked_list const *: ccc_sll_begin,                         \
        ccc_doubly_linked_list const *: ccc_dll_begin,                         \
        ccc_flat_realtime_ordered_map const *: ccc_frm_begin,                  \
        ccc_realtime_ordered_map const *: ccc_rom_begin)((container_ptr))

#define ccc_impl_rbegin(container_ptr)                                         \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_rbegin,                                          \
        ccc_ordered_map *: ccc_om_rbegin,                                      \
        ccc_flat_double_ended_queue *: ccc_fdeq_rbegin,                        \
        ccc_double_ended_priority_queue *: ccc_depq_rbegin,                    \
        ccc_doubly_linked_list *: ccc_dll_rbegin,                              \
        ccc_realtime_ordered_map *: ccc_rom_rbegin,                            \
        ccc_flat_realtime_ordered_map *: ccc_frm_rbegin,                       \
        ccc_buffer const *: ccc_buf_rbegin,                                    \
        ccc_ordered_map const *: ccc_om_rbegin,                                \
        ccc_flat_double_ended_queue const *: ccc_fdeq_rbegin,                  \
        ccc_double_ended_priority_queue const *: ccc_depq_rbegin,              \
        ccc_doubly_linked_list const *: ccc_dll_rbegin,                        \
        ccc_flat_realtime_ordered_map const *: ccc_frm_rbegin,                 \
        ccc_realtime_ordered_map const *: ccc_rom_rbegin)((container_ptr))

#define ccc_impl_next(container_ptr, void_iter_ptr)                            \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_next,                                            \
        ccc_flat_hash_map *: ccc_fhm_next,                                     \
        ccc_ordered_map *: ccc_om_next,                                        \
        ccc_flat_double_ended_queue *: ccc_fdeq_next,                          \
        ccc_double_ended_priority_queue *: ccc_depq_next,                      \
        ccc_singly_linked_list *: ccc_sll_next,                                \
        ccc_doubly_linked_list *: ccc_dll_next,                                \
        ccc_realtime_ordered_map *: ccc_rom_next,                              \
        ccc_flat_realtime_ordered_map *: ccc_frm_next,                         \
        ccc_buffer const *: ccc_buf_next,                                      \
        ccc_flat_hash_map const *: ccc_fhm_next,                               \
        ccc_ordered_map const *: ccc_om_next,                                  \
        ccc_flat_double_ended_queue const *: ccc_fdeq_next,                    \
        ccc_double_ended_priority_queue const *: ccc_depq_next,                \
        ccc_singly_linked_list const *: ccc_sll_next,                          \
        ccc_doubly_linked_list const *: ccc_dll_next,                          \
        ccc_flat_realtime_ordered_map const *: ccc_frm_next,                   \
        ccc_realtime_ordered_map const *: ccc_rom_next)((container_ptr),       \
                                                        (void_iter_ptr))

#define ccc_impl_rnext(container_ptr, void_iter_ptr)                           \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_rnext,                                           \
        ccc_ordered_map *: ccc_om_rnext,                                       \
        ccc_flat_double_ended_queue *: ccc_fdeq_rnext,                         \
        ccc_double_ended_priority_queue *: ccc_depq_rnext,                     \
        ccc_doubly_linked_list *: ccc_dll_rnext,                               \
        ccc_realtime_ordered_map *: ccc_rom_rnext,                             \
        ccc_flat_realtime_ordered_map *: ccc_frm_rnext,                        \
        ccc_buffer const *: ccc_buf_rnext,                                     \
        ccc_ordered_map const *: ccc_om_rnext,                                 \
        ccc_flat_double_ended_queue const *: ccc_fdeq_rnext,                   \
        ccc_double_ended_priority_queue const *: ccc_depq_rnext,               \
        ccc_doubly_linked_list const *: ccc_dll_rnext,                         \
        ccc_flat_realtime_ordered_map const *: ccc_frm_rnext,                  \
        ccc_realtime_ordered_map const *: ccc_rom_rnext)((container_ptr),      \
                                                         (void_iter_ptr))

#define ccc_impl_end(container_ptr)                                            \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_end,                                             \
        ccc_flat_hash_map *: ccc_fhm_end,                                      \
        ccc_ordered_map *: ccc_om_end,                                         \
        ccc_flat_double_ended_queue *: ccc_fdeq_end,                           \
        ccc_double_ended_priority_queue *: ccc_depq_end,                       \
        ccc_singly_linked_list *: ccc_sll_end,                                 \
        ccc_doubly_linked_list *: ccc_dll_end,                                 \
        ccc_realtime_ordered_map *: ccc_rom_end,                               \
        ccc_flat_realtime_ordered_map *: ccc_frm_end,                          \
        ccc_buffer const *: ccc_buf_end,                                       \
        ccc_flat_hash_map const *: ccc_fhm_end,                                \
        ccc_ordered_map const *: ccc_om_end,                                   \
        ccc_flat_double_ended_queue const *: ccc_fdeq_end,                     \
        ccc_double_ended_priority_queue const *: ccc_depq_end,                 \
        ccc_singly_linked_list const *: ccc_sll_end,                           \
        ccc_doubly_linked_list const *: ccc_dll_end,                           \
        ccc_flat_realtime_ordered_map const *: ccc_frm_end,                    \
        ccc_realtime_ordered_map const *: ccc_rom_end)((container_ptr))

#define ccc_impl_rend(container_ptr)                                           \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_rend,                                            \
        ccc_ordered_map *: ccc_om_rend,                                        \
        ccc_flat_double_ended_queue *: ccc_fdeq_rend,                          \
        ccc_double_ended_priority_queue *: ccc_depq_rend,                      \
        ccc_doubly_linked_list *: ccc_dll_rend,                                \
        ccc_realtime_ordered_map *: ccc_rom_rend,                              \
        ccc_flat_realtime_ordered_map *: ccc_frm_rend,                         \
        ccc_buffer const *: ccc_buf_rend,                                      \
        ccc_ordered_map const *: ccc_om_rend,                                  \
        ccc_flat_double_ended_queue const *: ccc_fdeq_rend,                    \
        ccc_double_ended_priority_queue const *: ccc_depq_rend,                \
        ccc_doubly_linked_list const *: ccc_dll_rend,                          \
        ccc_flat_realtime_ordered_map const *: ccc_frm_rend,                   \
        ccc_realtime_ordered_map const *: ccc_rom_rend)((container_ptr))

#define ccc_impl_equal_range(container_ptr, begin_and_end_key_ptr...)          \
    _Generic((container_ptr),                                                  \
        ccc_ordered_map *: ccc_om_equal_range,                                 \
        ccc_double_ended_priority_queue *: ccc_depq_equal_range,               \
        ccc_realtime_ordered_map *: ccc_rom_equal_range,                       \
        ccc_realtime_ordered_map const *: ccc_rom_equal_range)(                \
        (container_ptr), begin_and_end_key_ptr)

#define ccc_impl_equal_range_vr(container_ptr, begin_and_end_key_ptr...)       \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_impl_equal_range(container_ptr, begin_and_end_key_ptr).impl_       \
    }

#define ccc_impl_equal_rrange(container_ptr, rbegin_and_rend_key_ptr...)       \
    _Generic((container_ptr),                                                  \
        ccc_ordered_map *: ccc_om_equal_rrange,                                \
        ccc_double_ended_priority_queue *: ccc_depq_equal_rrange,              \
        ccc_realtime_ordered_map *: ccc_rom_equal_rrange,                      \
        ccc_realtime_ordered_map const *: ccc_rom_equal_rrange)(               \
        (container_ptr), rbegin_and_rend_key_ptr)

#define ccc_impl_equal_rrange_vr(container_ptr, rbegin_and_rend_key_ptr...)    \
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

/*===================    Standard Getters API ==============================*/

#define ccc_impl_size(container_ptr)                                           \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_size,                                            \
        ccc_flat_hash_map *: ccc_fhm_size,                                     \
        ccc_ordered_map *: ccc_om_size,                                        \
        ccc_flat_priority_queue *: ccc_fpq_size,                               \
        ccc_flat_double_ended_queue *: ccc_fdeq_size,                          \
        ccc_double_ended_priority_queue *: ccc_depq_size,                      \
        ccc_priority_queue *: ccc_pq_size,                                     \
        ccc_singly_linked_list *: ccc_sll_size,                                \
        ccc_doubly_linked_list *: ccc_dll_size,                                \
        ccc_realtime_ordered_map *: ccc_rom_size,                              \
        ccc_flat_realtime_ordered_map *: ccc_frm_size,                         \
        ccc_buffer const *: ccc_buf_size,                                      \
        ccc_flat_hash_map const *: ccc_fhm_size,                               \
        ccc_ordered_map const *: ccc_om_size,                                  \
        ccc_flat_priority_queue const *: ccc_fpq_size,                         \
        ccc_flat_double_ended_queue const *: ccc_fdeq_size,                    \
        ccc_double_ended_priority_queue const *: ccc_depq_size,                \
        ccc_priority_queue const *: ccc_pq_size,                               \
        ccc_singly_linked_list const *: ccc_sll_size,                          \
        ccc_doubly_linked_list const *: ccc_dll_size,                          \
        ccc_flat_realtime_ordered_map const *: ccc_frm_size,                   \
        ccc_realtime_ordered_map const *: ccc_rom_size)((container_ptr))

#define ccc_impl_empty(container_ptr)                                          \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_empty,                                           \
        ccc_flat_hash_map *: ccc_fhm_empty,                                    \
        ccc_ordered_map *: ccc_om_empty,                                       \
        ccc_flat_priority_queue *: ccc_fpq_empty,                              \
        ccc_flat_double_ended_queue *: ccc_fdeq_empty,                         \
        ccc_double_ended_priority_queue *: ccc_depq_empty,                     \
        ccc_priority_queue *: ccc_pq_empty,                                    \
        ccc_singly_linked_list *: ccc_sll_empty,                               \
        ccc_doubly_linked_list *: ccc_dll_empty,                               \
        ccc_realtime_ordered_map *: ccc_rom_empty,                             \
        ccc_flat_realtime_ordered_map *: ccc_frm_empty,                        \
        ccc_buffer const *: ccc_buf_empty,                                     \
        ccc_flat_hash_map const *: ccc_fhm_empty,                              \
        ccc_ordered_map const *: ccc_om_empty,                                 \
        ccc_flat_priority_queue const *: ccc_fpq_empty,                        \
        ccc_flat_double_ended_queue const *: ccc_fdeq_empty,                   \
        ccc_double_ended_priority_queue const *: ccc_depq_empty,               \
        ccc_priority_queue const *: ccc_pq_empty,                              \
        ccc_singly_linked_list const *: ccc_sll_empty,                         \
        ccc_doubly_linked_list const *: ccc_dll_empty,                         \
        ccc_flat_realtime_ordered_map const *: ccc_frm_empty,                  \
        ccc_realtime_ordered_map const *: ccc_rom_empty)((container_ptr))

#define ccc_impl_validate(container_ptr)                                       \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_validate,                                 \
        ccc_ordered_map *: ccc_om_validate,                                    \
        ccc_flat_priority_queue *: ccc_fpq_validate,                           \
        ccc_flat_double_ended_queue *: ccc_fdeq_validate,                      \
        ccc_double_ended_priority_queue *: ccc_depq_validate,                  \
        ccc_priority_queue *: ccc_pq_validate,                                 \
        ccc_singly_linked_list *: ccc_sll_validate,                            \
        ccc_doubly_linked_list *: ccc_dll_validate,                            \
        ccc_realtime_ordered_map *: ccc_rom_validate,                          \
        ccc_flat_realtime_ordered_map *: ccc_frm_validate,                     \
        ccc_flat_hash_map const *: ccc_fhm_validate,                           \
        ccc_ordered_map const *: ccc_om_validate,                              \
        ccc_flat_priority_queue const *: ccc_fpq_validate,                     \
        ccc_flat_double_ended_queue const *: ccc_fdeq_validate,                \
        ccc_double_ended_priority_queue const *: ccc_depq_validate,            \
        ccc_priority_queue const *: ccc_pq_validate,                           \
        ccc_singly_linked_list const *: ccc_sll_validate,                      \
        ccc_doubly_linked_list const *: ccc_dll_validate,                      \
        ccc_flat_realtime_ordered_map const *: ccc_frm_validate,               \
        ccc_realtime_ordered_map const *: ccc_rom_validate)((container_ptr))

#endif /* CCC_IMPL_TRAITS_H */
