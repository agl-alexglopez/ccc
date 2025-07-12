/** @cond
Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
@endcond */
#ifndef CCC_IMPL_TRAITS_H
#define CCC_IMPL_TRAITS_H

/* NOLINTBEGIN */
#include "../bitset.h"
#include "../buffer.h"
#include "../doubly_linked_list.h"
#include "../flat_double_ended_queue.h"
#include "../flat_hash_map.h"
#include "../flat_priority_queue.h"
#include "../handle_ordered_map.h"
#include "../handle_realtime_ordered_map.h"
#include "../ordered_map.h"
#include "../priority_queue.h"
#include "../realtime_ordered_map.h"
#include "../singly_linked_list.h"
#include "../types.h"
/* NOLINTEND */

/*====================     Entry/Handle Interface   =========================*/

#define ccc_impl_swap_entry(container_ptr, swap_args...)                       \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_swap_entry,                               \
        ccc_ordered_map *: ccc_om_swap_entry,                                  \
        ccc_realtime_ordered_map *: ccc_rom_swap_entry)((container_ptr),       \
                                                        swap_args)

#define ccc_impl_swap_entry_r(container_ptr, key_val_container_handle_ptr...)  \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_swap_entry(container_ptr, key_val_container_handle_ptr).impl  \
    }

#define ccc_impl_swap_handle(container_ptr, swap_args...)                      \
    _Generic((container_ptr),                                                  \
        ccc_handle_ordered_map *: ccc_hom_swap_handle,                         \
        ccc_handle_realtime_ordered_map                                        \
            *: ccc_hrm_swap_handle)((container_ptr), swap_args)

#define ccc_impl_swap_handle_r(container_ptr, key_val_container_handle_ptr...) \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_impl_swap_handle(container_ptr, key_val_container_handle_ptr).impl \
    }

#define ccc_impl_try_insert(container_ptr, try_insert_args...)                 \
    _Generic((container_ptr),                                                  \
        ccc_handle_ordered_map *: ccc_hom_try_insert,                          \
        ccc_handle_realtime_ordered_map *: ccc_hrm_try_insert,                 \
        ccc_flat_hash_map *: ccc_fhm_try_insert,                               \
        ccc_ordered_map *: ccc_om_try_insert,                                  \
        ccc_realtime_ordered_map *: ccc_rom_try_insert)((container_ptr),       \
                                                        try_insert_args)

#define ccc_impl_try_insert_r(container_ptr, try_insert_args...)               \
    _Generic((container_ptr),                                                  \
        ccc_handle_ordered_map                                                 \
            *: &(ccc_handle){ccc_hom_try_insert(                               \
                                 (ccc_handle_ordered_map *)container_ptr,      \
                                 try_insert_args)                              \
                                 .impl},                                       \
        ccc_handle_realtime_ordered_map                                        \
            *: &(ccc_handle){ccc_hrm_try_insert(                               \
                                 (ccc_handle_realtime_ordered_map *)           \
                                     container_ptr,                            \
                                 try_insert_args)                              \
                                 .impl},                                       \
        ccc_flat_hash_map                                                      \
            *: &(ccc_entry){ccc_fhm_try_insert(                                \
                                (ccc_flat_hash_map *)container_ptr,            \
                                try_insert_args)                               \
                                .impl},                                        \
        ccc_ordered_map                                                        \
            *: &(                                                              \
                ccc_entry){ccc_om_try_insert((ccc_ordered_map *)container_ptr, \
                                             (ccc_omap_elem *)try_insert_args) \
                               .impl},                                         \
        ccc_realtime_ordered_map                                               \
            *: &(ccc_entry){                                                   \
                ccc_rom_try_insert((ccc_realtime_ordered_map *)container_ptr,  \
                                   (ccc_romap_elem *)try_insert_args)          \
                    .impl})

#define ccc_impl_insert_or_assign(container_ptr, insert_or_assign_args...)     \
    _Generic((container_ptr),                                                  \
        ccc_handle_ordered_map *: ccc_hom_insert_or_assign,                    \
        ccc_handle_realtime_ordered_map *: ccc_hrm_insert_or_assign,           \
        ccc_flat_hash_map *: ccc_fhm_insert_or_assign,                         \
        ccc_ordered_map *: ccc_om_insert_or_assign,                            \
        ccc_realtime_ordered_map *: ccc_rom_insert_or_assign)(                 \
        (container_ptr), insert_or_assign_args)

#define ccc_impl_insert_or_assign_r(container_ptr, insert_or_assign_args...)   \
    _Generic((container_ptr),                                                  \
        ccc_handle_ordered_map                                                 \
            *: &(ccc_handle){ccc_hom_insert_or_assign(                         \
                                 (ccc_handle_ordered_map *)container_ptr,      \
                                 insert_or_assign_args)                        \
                                 .impl},                                       \
        ccc_handle_realtime_ordered_map                                        \
            *: &(ccc_handle){ccc_hrm_insert_or_assign(                         \
                                 (ccc_handle_realtime_ordered_map *)           \
                                     container_ptr,                            \
                                 insert_or_assign_args)                        \
                                 .impl},                                       \
        ccc_flat_hash_map                                                      \
            *: &(ccc_entry){ccc_fhm_insert_or_assign(                          \
                                (ccc_flat_hash_map *)container_ptr,            \
                                insert_or_assign_args)                         \
                                .impl},                                        \
        ccc_ordered_map                                                        \
            *: &(ccc_entry){ccc_om_insert_or_assign(                           \
                                (ccc_ordered_map *)container_ptr,              \
                                (ccc_omap_elem *)insert_or_assign_args)        \
                                .impl},                                        \
        ccc_realtime_ordered_map                                               \
            *: &(ccc_entry){ccc_rom_insert_or_assign(                          \
                                (ccc_realtime_ordered_map *)container_ptr,     \
                                (ccc_romap_elem *)insert_or_assign_args)       \
                                .impl})

#define ccc_impl_remove(container_ptr, key_val_container_handle_ptr...)        \
    _Generic((container_ptr),                                                  \
        ccc_handle_ordered_map *: ccc_hom_remove,                              \
        ccc_handle_realtime_ordered_map *: ccc_hrm_remove,                     \
        ccc_flat_hash_map *: ccc_fhm_remove,                                   \
        ccc_ordered_map *: ccc_om_remove,                                      \
        ccc_realtime_ordered_map                                               \
            *: ccc_rom_remove)((container_ptr), key_val_container_handle_ptr)

#define ccc_impl_remove_r(container_ptr, key_val_container_handle_ptr...)      \
    _Generic((container_ptr),                                                  \
        ccc_handle_ordered_map                                                 \
            *: &(ccc_handle){ccc_hom_remove(                                   \
                                 (ccc_handle_ordered_map *)container_ptr,      \
                                 key_val_container_handle_ptr)                 \
                                 .impl},                                       \
        ccc_handle_realtime_ordered_map                                        \
            *: &(                                                              \
                ccc_handle){ccc_hrm_remove((ccc_handle_realtime_ordered_map *) \
                                               container_ptr,                  \
                                           key_val_container_handle_ptr)       \
                                .impl},                                        \
        ccc_flat_hash_map                                                      \
            *: &(ccc_entry){ccc_fhm_remove((ccc_flat_hash_map *)container_ptr, \
                                           key_val_container_handle_ptr)       \
                                .impl},                                        \
        ccc_ordered_map                                                        \
            *: &(ccc_entry){ccc_om_remove(                                     \
                                (ccc_ordered_map *)container_ptr,              \
                                (ccc_omap_elem *)key_val_container_handle_ptr) \
                                .impl},                                        \
        ccc_realtime_ordered_map                                               \
            *: &(ccc_entry){                                                   \
                ccc_rom_remove((ccc_realtime_ordered_map *)container_ptr,      \
                               (ccc_romap_elem *)key_val_container_handle_ptr) \
                    .impl})

#define ccc_impl_remove_entry(container_entry_ptr)                             \
    _Generic((container_entry_ptr),                                            \
        ccc_fhmap_entry *: ccc_fhm_remove_entry,                               \
        ccc_omap_entry *: ccc_om_remove_entry,                                 \
        ccc_romap_entry *: ccc_rom_remove_entry,                               \
        ccc_fhmap_entry const *: ccc_fhm_remove_entry,                         \
        ccc_omap_entry const *: ccc_om_remove_entry,                           \
        ccc_romap_entry const *: ccc_rom_remove_entry)((container_entry_ptr))

#define ccc_impl_remove_entry_r(container_entry_ptr)                           \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_remove_entry(container_entry_ptr).impl                        \
    }

#define ccc_impl_remove_handle(container_handle_ptr)                           \
    _Generic((container_handle_ptr),                                           \
        ccc_homap_handle *: ccc_hom_remove_handle,                             \
        ccc_homap_handle const *: ccc_hom_remove_handle,                       \
        ccc_hromap_handle *: ccc_hrm_remove_handle,                            \
        ccc_hromap_handle const *: ccc_hrm_remove_handle)(                     \
        (container_handle_ptr))

#define ccc_impl_remove_handle_r(container_handle_ptr)                         \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_impl_remove_handle(container_handle_ptr).impl                      \
    }

#define ccc_impl_entry(container_ptr, key_ptr...)                              \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_entry,                                    \
        ccc_flat_hash_map const *: ccc_fhm_entry,                              \
        ccc_ordered_map *: ccc_om_entry,                                       \
        ccc_realtime_ordered_map *: ccc_rom_entry,                             \
        ccc_realtime_ordered_map const *: ccc_rom_entry)((container_ptr),      \
                                                         key_ptr)

#define ccc_impl_entry_r(container_ptr, key_ptr...)                            \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map                                                      \
            *: &(ccc_fhmap_entry){ccc_fhm_entry(                               \
                                      (ccc_flat_hash_map *)(container_ptr),    \
                                      key_ptr)                                 \
                                      .impl},                                  \
        ccc_flat_hash_map const *: &(                                          \
                 ccc_fhmap_entry){ccc_fhm_entry(                               \
                                      (ccc_flat_hash_map *)(container_ptr),    \
                                      key_ptr)                                 \
                                      .impl},                                  \
        ccc_ordered_map *: &(                                                  \
                 ccc_omap_entry){ccc_om_entry(                                 \
                                     (ccc_ordered_map *)(container_ptr),       \
                                     key_ptr)                                  \
                                     .impl},                                   \
        ccc_realtime_ordered_map *: &(                                         \
                 ccc_romap_entry){ccc_rom_entry((ccc_realtime_ordered_map      \
                                                     *)(container_ptr),        \
                                                key_ptr)                       \
                                      .impl},                                  \
        ccc_realtime_ordered_map const *: &(ccc_romap_entry){                  \
            ccc_rom_entry((ccc_realtime_ordered_map *)(container_ptr),         \
                          key_ptr)                                             \
                .impl})

#define ccc_impl_handle(container_ptr, key_ptr...)                             \
    _Generic((container_ptr),                                                  \
        ccc_handle_ordered_map *: ccc_hom_handle,                              \
        ccc_handle_realtime_ordered_map *: ccc_hrm_handle,                     \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_handle)(              \
        (container_ptr), key_ptr)

#define ccc_impl_handle_r(container_ptr, key_ptr...)                           \
    _Generic((container_ptr),                                                  \
        ccc_handle_ordered_map                                                 \
            *: &(ccc_homap_handle){ccc_hom_handle((ccc_handle_ordered_map      \
                                                       *)(container_ptr),      \
                                                  key_ptr)                     \
                                       .impl},                                 \
        ccc_handle_realtime_ordered_map                                        \
            *: &(ccc_hromap_handle){ccc_hrm_handle(                            \
                                        (ccc_handle_realtime_ordered_map       \
                                             *)(container_ptr),                \
                                        key_ptr)                               \
                                        .impl},                                \
        ccc_handle_realtime_ordered_map const *: &(ccc_hromap_handle){         \
            ccc_hrm_handle((ccc_handle_realtime_ordered_map *)(container_ptr), \
                           key_ptr)                                            \
                .impl})

#define ccc_impl_and_modify(container_entry_ptr, mod_fn)                       \
    _Generic((container_entry_ptr),                                            \
        ccc_fhmap_entry *: ccc_fhm_and_modify,                                 \
        ccc_omap_entry *: ccc_om_and_modify,                                   \
        ccc_homap_handle *: ccc_hom_and_modify,                                \
        ccc_romap_entry *: ccc_rom_and_modify,                                 \
        ccc_hromap_handle *: ccc_hrm_and_modify,                               \
        ccc_fhmap_entry const *: ccc_fhm_and_modify,                           \
        ccc_hromap_handle const *: ccc_hrm_and_modify,                         \
        ccc_omap_entry const *: ccc_om_and_modify,                             \
        ccc_homap_handle const *: ccc_hom_and_modify,                          \
        ccc_romap_entry const *: ccc_rom_and_modify)((container_entry_ptr),    \
                                                     (mod_fn))

#define ccc_impl_and_modify_aux(container_entry_ptr, mod_fn, aux_data_ptr...)  \
    _Generic((container_entry_ptr),                                            \
        ccc_fhmap_entry *: ccc_fhm_and_modify_aux,                             \
        ccc_omap_entry *: ccc_om_and_modify_aux,                               \
        ccc_homap_handle *: ccc_hom_and_modify_aux,                            \
        ccc_hromap_handle *: ccc_hrm_and_modify_aux,                           \
        ccc_romap_entry *: ccc_rom_and_modify_aux,                             \
        ccc_fhmap_entry const *: ccc_fhm_and_modify_aux,                       \
        ccc_omap_entry const *: ccc_om_and_modify_aux,                         \
        ccc_hromap_handle const *: ccc_hrm_and_modify_aux,                     \
        ccc_homap_handle const *: ccc_hom_and_modify_aux,                      \
        ccc_romap_entry const *: ccc_rom_and_modify_aux)(                      \
        (container_entry_ptr), (mod_fn), aux_data_ptr)

#define ccc_impl_insert_entry(container_entry_ptr,                             \
                              key_val_container_handle_ptr...)                 \
    _Generic((container_entry_ptr),                                            \
        ccc_fhmap_entry *: ccc_fhm_insert_entry,                               \
        ccc_omap_entry *: ccc_om_insert_entry,                                 \
        ccc_romap_entry *: ccc_rom_insert_entry,                               \
        ccc_fhmap_entry const *: ccc_fhm_insert_entry,                         \
        ccc_omap_entry const *: ccc_om_insert_entry,                           \
        ccc_romap_entry const *: ccc_rom_insert_entry)(                        \
        (container_entry_ptr), key_val_container_handle_ptr)

#define ccc_impl_insert_handle(container_handle_ptr,                           \
                               key_val_container_handle_ptr...)                \
    _Generic((container_handle_ptr),                                           \
        ccc_homap_handle *: ccc_hom_insert_handle,                             \
        ccc_homap_handle const *: ccc_hom_insert_handle,                       \
        ccc_hromap_handle *: ccc_hrm_insert_handle,                            \
        ccc_hromap_handle const *: ccc_hrm_insert_handle)(                     \
        (container_handle_ptr), key_val_container_handle_ptr)

#define ccc_impl_or_insert(container_entry_ptr,                                \
                           key_val_container_handle_ptr...)                    \
    _Generic((container_entry_ptr),                                            \
        ccc_fhmap_entry *: ccc_fhm_or_insert,                                  \
        ccc_omap_entry *: ccc_om_or_insert,                                    \
        ccc_homap_handle *: ccc_hom_or_insert,                                 \
        ccc_romap_entry *: ccc_rom_or_insert,                                  \
        ccc_hromap_handle *: ccc_hrm_or_insert,                                \
        ccc_fhmap_entry const *: ccc_fhm_or_insert,                            \
        ccc_omap_entry const *: ccc_om_or_insert,                              \
        ccc_hromap_handle const *: ccc_hrm_or_insert,                          \
        ccc_homap_handle const *: ccc_hom_or_insert,                           \
        ccc_romap_entry const *: ccc_rom_or_insert)(                           \
        (container_entry_ptr), key_val_container_handle_ptr)

#define ccc_impl_unwrap(container_entry_ptr)                                   \
    _Generic((container_entry_ptr),                                            \
        ccc_entry *: ccc_entry_unwrap,                                         \
        ccc_entry const *: ccc_entry_unwrap,                                   \
        ccc_handle *: ccc_handle_unwrap,                                       \
        ccc_handle const *: ccc_handle_unwrap,                                 \
        ccc_fhmap_entry *: ccc_fhm_unwrap,                                     \
        ccc_fhmap_entry const *: ccc_fhm_unwrap,                               \
        ccc_omap_entry *: ccc_om_unwrap,                                       \
        ccc_omap_entry const *: ccc_om_unwrap,                                 \
        ccc_homap_handle *: ccc_hom_unwrap,                                    \
        ccc_homap_handle const *: ccc_hom_unwrap,                              \
        ccc_hromap_handle *: ccc_hrm_unwrap,                                   \
        ccc_hromap_handle const *: ccc_hrm_unwrap,                             \
        ccc_romap_entry *: ccc_rom_unwrap,                                     \
        ccc_romap_entry const *: ccc_rom_unwrap)((container_entry_ptr))

#define ccc_impl_occupied(container_entry_ptr)                                 \
    _Generic((container_entry_ptr),                                            \
        ccc_entry *: ccc_entry_occupied,                                       \
        ccc_entry const *: ccc_entry_occupied,                                 \
        ccc_handle *: ccc_handle_occupied,                                     \
        ccc_handle const *: ccc_handle_occupied,                               \
        ccc_fhmap_entry *: ccc_fhm_occupied,                                   \
        ccc_fhmap_entry const *: ccc_fhm_occupied,                             \
        ccc_omap_entry *: ccc_om_occupied,                                     \
        ccc_omap_entry const *: ccc_om_occupied,                               \
        ccc_homap_handle *: ccc_hom_occupied,                                  \
        ccc_homap_handle const *: ccc_hom_occupied,                            \
        ccc_hromap_handle *: ccc_hrm_occupied,                                 \
        ccc_hromap_handle const *: ccc_hrm_occupied,                           \
        ccc_romap_entry *: ccc_rom_occupied,                                   \
        ccc_romap_entry const *: ccc_rom_occupied)((container_entry_ptr))

#define ccc_impl_insert_error(container_entry_ptr)                             \
    _Generic((container_entry_ptr),                                            \
        ccc_entry *: ccc_entry_insert_error,                                   \
        ccc_entry const *: ccc_entry_insert_error,                             \
        ccc_handle *: ccc_handle_insert_error,                                 \
        ccc_handle const *: ccc_handle_insert_error,                           \
        ccc_fhmap_entry *: ccc_fhm_insert_error,                               \
        ccc_fhmap_entry const *: ccc_fhm_insert_error,                         \
        ccc_omap_entry *: ccc_om_insert_error,                                 \
        ccc_omap_entry const *: ccc_om_insert_error,                           \
        ccc_homap_handle *: ccc_hom_insert_error,                              \
        ccc_homap_handle const *: ccc_hom_insert_error,                        \
        ccc_hromap_handle *: ccc_hrm_insert_error,                             \
        ccc_hromap_handle const *: ccc_hrm_insert_error,                       \
        ccc_romap_entry *: ccc_rom_insert_error,                               \
        ccc_romap_entry const *: ccc_rom_insert_error)((container_entry_ptr))

/*======================    Misc Search Interface ===========================*/

#define ccc_impl_get_key_val(container_ptr, key_ptr...)                        \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_get_key_val,                              \
        ccc_flat_hash_map const *: ccc_fhm_get_key_val,                        \
        ccc_ordered_map *: ccc_om_get_key_val,                                 \
        ccc_handle_ordered_map *: ccc_hom_get_key_val,                         \
        ccc_handle_realtime_ordered_map *: ccc_hrm_get_key_val,                \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_get_key_val,          \
        ccc_realtime_ordered_map *: ccc_rom_get_key_val,                       \
        ccc_realtime_ordered_map const *: ccc_rom_get_key_val)(                \
        (container_ptr), key_ptr)

#define ccc_impl_contains(container_ptr, key_ptr...)                           \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_contains,                                 \
        ccc_flat_hash_map const *: ccc_fhm_contains,                           \
        ccc_ordered_map *: ccc_om_contains,                                    \
        ccc_handle_ordered_map *: ccc_hom_contains,                            \
        ccc_handle_realtime_ordered_map *: ccc_hrm_contains,                   \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_contains,             \
        ccc_realtime_ordered_map *: ccc_rom_contains,                          \
        ccc_realtime_ordered_map const *: ccc_rom_contains)((container_ptr),   \
                                                            key_ptr)

/*================    Sequential Containers Interface   =====================*/

#define ccc_impl_push(container_ptr, container_handle_ptr...)                  \
    _Generic((container_ptr),                                                  \
        ccc_flat_priority_queue *: ccc_fpq_push,                               \
        ccc_priority_queue *: ccc_pq_push)((container_ptr),                    \
                                           container_handle_ptr)

#define ccc_impl_push_back(container_ptr, container_handle_ptr...)             \
    _Generic((container_ptr),                                                  \
        ccc_bitset *: ccc_bs_push_back,                                        \
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
        ccc_flat_priority_queue *: ccc_fpq_update,                             \
        ccc_priority_queue *: ccc_pq_update)((container_ptr), update_args)

#define ccc_impl_increase(container_ptr, increase_args...)                     \
    _Generic((container_ptr),                                                  \
        ccc_flat_priority_queue *: ccc_fpq_increase,                           \
        ccc_priority_queue *: ccc_pq_increase)((container_ptr), increase_args)

#define ccc_impl_decrease(container_ptr, decrease_args...)                     \
    _Generic((container_ptr),                                                  \
        ccc_flat_priority_queue *: ccc_fpq_decrease,                           \
        ccc_priority_queue *: ccc_pq_decrease)((container_ptr), decrease_args)

#define ccc_impl_extract(container_ptr, container_handle_ptr...)               \
    _Generic((container_ptr),                                                  \
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
        ccc_handle_ordered_map *: ccc_hom_begin,                               \
        ccc_flat_double_ended_queue *: ccc_fdeq_begin,                         \
        ccc_singly_linked_list *: ccc_sll_begin,                               \
        ccc_doubly_linked_list *: ccc_dll_begin,                               \
        ccc_realtime_ordered_map *: ccc_rom_begin,                             \
        ccc_handle_realtime_ordered_map *: ccc_hrm_begin,                      \
        ccc_buffer const *: ccc_buf_begin,                                     \
        ccc_flat_hash_map const *: ccc_fhm_begin,                              \
        ccc_ordered_map const *: ccc_om_begin,                                 \
        ccc_handle_ordered_map const *: ccc_hom_begin,                         \
        ccc_flat_double_ended_queue const *: ccc_fdeq_begin,                   \
        ccc_singly_linked_list const *: ccc_sll_begin,                         \
        ccc_doubly_linked_list const *: ccc_dll_begin,                         \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_begin,                \
        ccc_realtime_ordered_map const *: ccc_rom_begin)((container_ptr))

#define ccc_impl_rbegin(container_ptr)                                         \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_rbegin,                                          \
        ccc_ordered_map *: ccc_om_rbegin,                                      \
        ccc_handle_ordered_map *: ccc_hom_rbegin,                              \
        ccc_flat_double_ended_queue *: ccc_fdeq_rbegin,                        \
        ccc_doubly_linked_list *: ccc_dll_rbegin,                              \
        ccc_realtime_ordered_map *: ccc_rom_rbegin,                            \
        ccc_handle_realtime_ordered_map *: ccc_hrm_rbegin,                     \
        ccc_buffer const *: ccc_buf_rbegin,                                    \
        ccc_ordered_map const *: ccc_om_rbegin,                                \
        ccc_handle_ordered_map const *: ccc_hom_rbegin,                        \
        ccc_flat_double_ended_queue const *: ccc_fdeq_rbegin,                  \
        ccc_doubly_linked_list const *: ccc_dll_rbegin,                        \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_rbegin,               \
        ccc_realtime_ordered_map const *: ccc_rom_rbegin)((container_ptr))

#define ccc_impl_next(container_ptr, void_iter_ptr)                            \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_next,                                            \
        ccc_flat_hash_map *: ccc_fhm_next,                                     \
        ccc_ordered_map *: ccc_om_next,                                        \
        ccc_handle_ordered_map *: ccc_hom_next,                                \
        ccc_flat_double_ended_queue *: ccc_fdeq_next,                          \
        ccc_singly_linked_list *: ccc_sll_next,                                \
        ccc_doubly_linked_list *: ccc_dll_next,                                \
        ccc_realtime_ordered_map *: ccc_rom_next,                              \
        ccc_handle_realtime_ordered_map *: ccc_hrm_next,                       \
        ccc_buffer const *: ccc_buf_next,                                      \
        ccc_flat_hash_map const *: ccc_fhm_next,                               \
        ccc_ordered_map const *: ccc_om_next,                                  \
        ccc_handle_ordered_map const *: ccc_hom_next,                          \
        ccc_flat_double_ended_queue const *: ccc_fdeq_next,                    \
        ccc_singly_linked_list const *: ccc_sll_next,                          \
        ccc_doubly_linked_list const *: ccc_dll_next,                          \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_next,                 \
        ccc_realtime_ordered_map const *: ccc_rom_next)((container_ptr),       \
                                                        (void_iter_ptr))

#define ccc_impl_rnext(container_ptr, void_iter_ptr)                           \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_rnext,                                           \
        ccc_ordered_map *: ccc_om_rnext,                                       \
        ccc_handle_ordered_map *: ccc_hom_rnext,                               \
        ccc_flat_double_ended_queue *: ccc_fdeq_rnext,                         \
        ccc_doubly_linked_list *: ccc_dll_rnext,                               \
        ccc_realtime_ordered_map *: ccc_rom_rnext,                             \
        ccc_handle_realtime_ordered_map *: ccc_hrm_rnext,                      \
        ccc_buffer const *: ccc_buf_rnext,                                     \
        ccc_ordered_map const *: ccc_om_rnext,                                 \
        ccc_handle_ordered_map const *: ccc_hom_rnext,                         \
        ccc_flat_double_ended_queue const *: ccc_fdeq_rnext,                   \
        ccc_doubly_linked_list const *: ccc_dll_rnext,                         \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_rnext,                \
        ccc_realtime_ordered_map const *: ccc_rom_rnext)((container_ptr),      \
                                                         (void_iter_ptr))

#define ccc_impl_end(container_ptr)                                            \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_end,                                             \
        ccc_flat_hash_map *: ccc_fhm_end,                                      \
        ccc_ordered_map *: ccc_om_end,                                         \
        ccc_handle_ordered_map *: ccc_hom_end,                                 \
        ccc_flat_double_ended_queue *: ccc_fdeq_end,                           \
        ccc_singly_linked_list *: ccc_sll_end,                                 \
        ccc_doubly_linked_list *: ccc_dll_end,                                 \
        ccc_realtime_ordered_map *: ccc_rom_end,                               \
        ccc_handle_realtime_ordered_map *: ccc_hrm_end,                        \
        ccc_buffer const *: ccc_buf_end,                                       \
        ccc_flat_hash_map const *: ccc_fhm_end,                                \
        ccc_ordered_map const *: ccc_om_end,                                   \
        ccc_handle_ordered_map const *: ccc_hom_end,                           \
        ccc_flat_double_ended_queue const *: ccc_fdeq_end,                     \
        ccc_singly_linked_list const *: ccc_sll_end,                           \
        ccc_doubly_linked_list const *: ccc_dll_end,                           \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_end,                  \
        ccc_realtime_ordered_map const *: ccc_rom_end)((container_ptr))

#define ccc_impl_rend(container_ptr)                                           \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_rend,                                            \
        ccc_ordered_map *: ccc_om_rend,                                        \
        ccc_handle_ordered_map *: ccc_hom_rend,                                \
        ccc_flat_double_ended_queue *: ccc_fdeq_rend,                          \
        ccc_doubly_linked_list *: ccc_dll_rend,                                \
        ccc_realtime_ordered_map *: ccc_rom_rend,                              \
        ccc_handle_realtime_ordered_map *: ccc_hrm_rend,                       \
        ccc_buffer const *: ccc_buf_rend,                                      \
        ccc_ordered_map const *: ccc_om_rend,                                  \
        ccc_handle_ordered_map const *: ccc_hom_rend,                          \
        ccc_flat_double_ended_queue const *: ccc_fdeq_rend,                    \
        ccc_doubly_linked_list const *: ccc_dll_rend,                          \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_rend,                 \
        ccc_realtime_ordered_map const *: ccc_rom_rend)((container_ptr))

#define ccc_impl_equal_range(container_ptr, begin_and_end_key_ptr...)          \
    _Generic((container_ptr),                                                  \
        ccc_ordered_map *: ccc_om_equal_range,                                 \
        ccc_handle_ordered_map *: ccc_hom_equal_range,                         \
        ccc_handle_realtime_ordered_map *: ccc_hrm_equal_range,                \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_equal_range,          \
        ccc_realtime_ordered_map *: ccc_rom_equal_range,                       \
        ccc_realtime_ordered_map const *: ccc_rom_equal_range)(                \
        (container_ptr), begin_and_end_key_ptr)

#define ccc_impl_equal_range_r(container_ptr, begin_and_end_key_ptr...)        \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_impl_equal_range(container_ptr, begin_and_end_key_ptr).impl        \
    }

#define ccc_impl_equal_rrange(container_ptr, rbegin_and_rend_key_ptr...)       \
    _Generic((container_ptr),                                                  \
        ccc_ordered_map *: ccc_om_equal_rrange,                                \
        ccc_handle_ordered_map *: ccc_hom_equal_rrange,                        \
        ccc_handle_realtime_ordered_map *: ccc_hrm_equal_rrange,               \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_equal_rrange,         \
        ccc_realtime_ordered_map *: ccc_rom_equal_rrange,                      \
        ccc_realtime_ordered_map const *: ccc_rom_equal_rrange)(               \
        (container_ptr), rbegin_and_rend_key_ptr)

#define ccc_impl_equal_rrange_r(container_ptr, rbegin_and_rend_key_ptr...)     \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_impl_equal_rrange(container_ptr, rbegin_and_rend_key_ptr).impl     \
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

/*===================         Memory Management       =======================*/

#define ccc_impl_copy(dst_container_ptr, src_container_ptr, alloc_fn_ptr)      \
    _Generic((dst_container_ptr),                                              \
        ccc_bitset *: ccc_bs_copy,                                             \
        ccc_flat_hash_map *: ccc_fhm_copy,                                     \
        ccc_handle_ordered_map *: ccc_hom_copy,                                \
        ccc_flat_priority_queue *: ccc_fpq_copy,                               \
        ccc_flat_double_ended_queue *: ccc_fdeq_copy,                          \
        ccc_handle_realtime_ordered_map *: ccc_hrm_copy)(                      \
        (dst_container_ptr), (src_container_ptr), (alloc_fn_ptr))

#define ccc_impl_reserve(container_ptr, n_to_add, alloc_fn_ptr)                \
    _Generic((container_ptr),                                                  \
        ccc_bitset *: ccc_bs_reserve,                                          \
        ccc_buffer *: ccc_buf_reserve,                                         \
        ccc_flat_hash_map *: ccc_fhm_reserve,                                  \
        ccc_handle_ordered_map *: ccc_hom_reserve,                             \
        ccc_flat_priority_queue *: ccc_fpq_reserve,                            \
        ccc_flat_double_ended_queue *: ccc_fdeq_reserve,                       \
        ccc_handle_realtime_ordered_map                                        \
            *: ccc_hrm_reserve)((container_ptr), (n_to_add), (alloc_fn_ptr))

#define ccc_impl_clear(container_ptr, ...)                                     \
    _Generic((container_ptr),                                                  \
        ccc_bitset *: ccc_bs_clear,                                            \
        ccc_buffer *: ccc_buf_clear,                                           \
        ccc_flat_hash_map *: ccc_fhm_clear,                                    \
        ccc_handle_ordered_map *: ccc_hom_clear,                               \
        ccc_flat_priority_queue *: ccc_fpq_clear,                              \
        ccc_flat_double_ended_queue *: ccc_fdeq_clear,                         \
        ccc_singly_linked_list *: ccc_sll_clear,                               \
        ccc_doubly_linked_list *: ccc_dll_clear,                               \
        ccc_ordered_map *: ccc_om_clear,                                       \
        ccc_priority_queue *: ccc_pq_clear,                                    \
        ccc_realtime_ordered_map *: ccc_rom_clear,                             \
        ccc_handle_realtime_ordered_map                                        \
            *: ccc_hrm_clear)((container_ptr)__VA_OPT__(, __VA_ARGS__))

#define ccc_impl_clear_and_free(container_ptr, ...)                            \
    _Generic((container_ptr),                                                  \
        ccc_bitset *: ccc_bs_clear_and_free,                                   \
        ccc_buffer *: ccc_buf_clear_and_free,                                  \
        ccc_flat_hash_map *: ccc_fhm_clear_and_free,                           \
        ccc_handle_ordered_map *: ccc_hom_clear_and_free,                      \
        ccc_flat_priority_queue *: ccc_fpq_clear_and_free,                     \
        ccc_flat_double_ended_queue *: ccc_fdeq_clear_and_free,                \
        ccc_handle_realtime_ordered_map *: ccc_hrm_clear_and_free)(            \
        (container_ptr)__VA_OPT__(, __VA_ARGS__))

#define ccc_impl_clear_and_free_reserve(container_ptr,                         \
                                        destructor_and_free_args...)           \
    _Generic((container_ptr),                                                  \
        ccc_bitset *: ccc_bs_clear_and_free_reserve,                           \
        ccc_buffer *: ccc_buf_clear_and_free_reserve,                          \
        ccc_flat_hash_map *: ccc_fhm_clear_and_free_reserve,                   \
        ccc_handle_ordered_map *: ccc_hom_clear_and_free_reserve,              \
        ccc_flat_priority_queue *: ccc_fpq_clear_and_free_reserve,             \
        ccc_flat_double_ended_queue *: ccc_fdeq_clear_and_free_reserve,        \
        ccc_handle_realtime_ordered_map *: ccc_hrm_clear_and_free_reserve)(    \
        (container_ptr), destructor_and_free_args)

/*===================    Standard Getters Interface   =======================*/

#define ccc_impl_count(container_ptr)                                          \
    _Generic((container_ptr),                                                  \
        ccc_bitset *: ccc_bs_count,                                            \
        ccc_buffer *: ccc_buf_count,                                           \
        ccc_flat_hash_map *: ccc_fhm_count,                                    \
        ccc_ordered_map *: ccc_om_count,                                       \
        ccc_handle_ordered_map *: ccc_hom_count,                               \
        ccc_flat_priority_queue *: ccc_fpq_count,                              \
        ccc_flat_double_ended_queue *: ccc_fdeq_count,                         \
        ccc_priority_queue *: ccc_pq_count,                                    \
        ccc_singly_linked_list *: ccc_sll_count,                               \
        ccc_doubly_linked_list *: ccc_dll_count,                               \
        ccc_realtime_ordered_map *: ccc_rom_count,                             \
        ccc_handle_realtime_ordered_map *: ccc_hrm_count,                      \
        ccc_buffer const *: ccc_buf_count,                                     \
        ccc_flat_hash_map const *: ccc_fhm_count,                              \
        ccc_ordered_map const *: ccc_om_count,                                 \
        ccc_handle_ordered_map const *: ccc_hom_count,                         \
        ccc_flat_priority_queue const *: ccc_fpq_count,                        \
        ccc_flat_double_ended_queue const *: ccc_fdeq_count,                   \
        ccc_priority_queue const *: ccc_pq_count,                              \
        ccc_singly_linked_list const *: ccc_sll_count,                         \
        ccc_doubly_linked_list const *: ccc_dll_count,                         \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_count,                \
        ccc_realtime_ordered_map const *: ccc_rom_count)((container_ptr))

#define ccc_impl_capacity(container_ptr)                                       \
    _Generic((container_ptr),                                                  \
        ccc_bitset *: ccc_bs_capacity,                                         \
        ccc_buffer *: ccc_buf_capacity,                                        \
        ccc_flat_hash_map *: ccc_fhm_capacity,                                 \
        ccc_handle_ordered_map *: ccc_hom_capacity,                            \
        ccc_flat_priority_queue *: ccc_fpq_capacity,                           \
        ccc_flat_double_ended_queue *: ccc_fdeq_capacity,                      \
        ccc_handle_realtime_ordered_map *: ccc_hrm_capacity,                   \
        ccc_bitset const *: ccc_bs_capacity,                                   \
        ccc_buffer const *: ccc_buf_capacity,                                  \
        ccc_flat_hash_map const *: ccc_fhm_capacity,                           \
        ccc_handle_ordered_map const *: ccc_hom_capacity,                      \
        ccc_flat_priority_queue const *: ccc_fpq_capacity,                     \
        ccc_flat_double_ended_queue const *: ccc_fdeq_capacity,                \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_capacity)(            \
        (container_ptr))

#define ccc_impl_is_empty(container_ptr)                                       \
    _Generic((container_ptr),                                                  \
        ccc_buffer *: ccc_buf_is_empty,                                        \
        ccc_flat_hash_map *: ccc_fhm_is_empty,                                 \
        ccc_ordered_map *: ccc_om_is_empty,                                    \
        ccc_handle_ordered_map *: ccc_hom_is_empty,                            \
        ccc_flat_priority_queue *: ccc_fpq_is_empty,                           \
        ccc_flat_double_ended_queue *: ccc_fdeq_is_empty,                      \
        ccc_priority_queue *: ccc_pq_is_empty,                                 \
        ccc_singly_linked_list *: ccc_sll_is_empty,                            \
        ccc_doubly_linked_list *: ccc_dll_is_empty,                            \
        ccc_realtime_ordered_map *: ccc_rom_is_empty,                          \
        ccc_handle_realtime_ordered_map *: ccc_hrm_is_empty,                   \
        ccc_buffer const *: ccc_buf_is_empty,                                  \
        ccc_flat_hash_map const *: ccc_fhm_is_empty,                           \
        ccc_ordered_map const *: ccc_om_is_empty,                              \
        ccc_handle_ordered_map const *: ccc_hom_is_empty,                      \
        ccc_flat_priority_queue const *: ccc_fpq_is_empty,                     \
        ccc_flat_double_ended_queue const *: ccc_fdeq_is_empty,                \
        ccc_priority_queue const *: ccc_pq_is_empty,                           \
        ccc_singly_linked_list const *: ccc_sll_is_empty,                      \
        ccc_doubly_linked_list const *: ccc_dll_is_empty,                      \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_is_empty,             \
        ccc_realtime_ordered_map const *: ccc_rom_is_empty)((container_ptr))

#define ccc_impl_validate(container_ptr)                                       \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map *: ccc_fhm_validate,                                 \
        ccc_ordered_map *: ccc_om_validate,                                    \
        ccc_handle_ordered_map *: ccc_hom_validate,                            \
        ccc_flat_priority_queue *: ccc_fpq_validate,                           \
        ccc_flat_double_ended_queue *: ccc_fdeq_validate,                      \
        ccc_priority_queue *: ccc_pq_validate,                                 \
        ccc_singly_linked_list *: ccc_sll_validate,                            \
        ccc_doubly_linked_list *: ccc_dll_validate,                            \
        ccc_realtime_ordered_map *: ccc_rom_validate,                          \
        ccc_handle_realtime_ordered_map *: ccc_hrm_validate,                   \
        ccc_flat_hash_map const *: ccc_fhm_validate,                           \
        ccc_ordered_map const *: ccc_om_validate,                              \
        ccc_handle_ordered_map const *: ccc_hom_validate,                      \
        ccc_flat_priority_queue const *: ccc_fpq_validate,                     \
        ccc_flat_double_ended_queue const *: ccc_fdeq_validate,                \
        ccc_priority_queue const *: ccc_pq_validate,                           \
        ccc_singly_linked_list const *: ccc_sll_validate,                      \
        ccc_doubly_linked_list const *: ccc_dll_validate,                      \
        ccc_handle_realtime_ordered_map const *: ccc_hrm_validate,             \
        ccc_realtime_ordered_map const *: ccc_rom_validate)((container_ptr))

#endif /* CCC_IMPL_TRAITS_H */
