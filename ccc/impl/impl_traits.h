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

#define CCC_impl_swap_entry(container_ptr, swap_args...)                       \
    _Generic((container_ptr),                                                  \
        CCC_flat_hash_map *: CCC_fhm_swap_entry,                               \
        CCC_ordered_map *: CCC_om_swap_entry,                                  \
        CCC_realtime_ordered_map *: CCC_rom_swap_entry)((container_ptr),       \
                                                        swap_args)

#define CCC_impl_swap_entry_r(container_ptr, key_val_container_handle_ptr...)  \
    &(CCC_entry)                                                               \
    {                                                                          \
        CCC_impl_swap_entry(container_ptr, key_val_container_handle_ptr).impl  \
    }

#define CCC_impl_swap_handle(container_ptr, swap_args...)                      \
    _Generic((container_ptr),                                                  \
        CCC_handle_ordered_map *: CCC_hom_swap_handle,                         \
        CCC_handle_realtime_ordered_map *: CCC_hrm_swap_handle)(               \
        (container_ptr), swap_args)

#define CCC_impl_swap_handle_r(container_ptr, key_val_container_handle_ptr...) \
    &(CCC_handle)                                                              \
    {                                                                          \
        CCC_impl_swap_handle(container_ptr, key_val_container_handle_ptr).impl \
    }

#define CCC_impl_try_insert(container_ptr, try_insert_args...)                 \
    _Generic((container_ptr),                                                  \
        CCC_handle_ordered_map *: CCC_hom_try_insert,                          \
        CCC_handle_realtime_ordered_map *: CCC_hrm_try_insert,                 \
        CCC_flat_hash_map *: CCC_fhm_try_insert,                               \
        CCC_ordered_map *: CCC_om_try_insert,                                  \
        CCC_realtime_ordered_map *: CCC_rom_try_insert)((container_ptr),       \
                                                        try_insert_args)

#define CCC_impl_try_insert_r(container_ptr, try_insert_args...)               \
    _Generic((container_ptr),                                                  \
        CCC_handle_ordered_map *: &(                                           \
                 CCC_handle){CCC_hom_try_insert(                               \
                                 (CCC_handle_ordered_map *)container_ptr,      \
                                 try_insert_args)                              \
                                 .impl},                                       \
        CCC_handle_realtime_ordered_map *: &(                                  \
                 CCC_handle){CCC_hrm_try_insert(                               \
                                 (CCC_handle_realtime_ordered_map *)           \
                                     container_ptr,                            \
                                 try_insert_args)                              \
                                 .impl},                                       \
        CCC_flat_hash_map *: &(                                                \
                 CCC_entry){CCC_fhm_try_insert(                                \
                                (CCC_flat_hash_map *)container_ptr,            \
                                try_insert_args)                               \
                                .impl},                                        \
        CCC_ordered_map *: &(CCC_entry){CCC_om_try_insert(                     \
                                            (CCC_ordered_map *)container_ptr,  \
                                            (CCC_omap_elem *)try_insert_args)  \
                                            .impl},                            \
        CCC_realtime_ordered_map *: &(CCC_entry){                              \
            CCC_rom_try_insert((CCC_realtime_ordered_map *)container_ptr,      \
                               (CCC_romap_elem *)try_insert_args)              \
                .impl})

#define CCC_impl_insert_or_assign(container_ptr, insert_or_assign_args...)     \
    _Generic((container_ptr),                                                  \
        CCC_handle_ordered_map *: CCC_hom_insert_or_assign,                    \
        CCC_handle_realtime_ordered_map *: CCC_hrm_insert_or_assign,           \
        CCC_flat_hash_map *: CCC_fhm_insert_or_assign,                         \
        CCC_ordered_map *: CCC_om_insert_or_assign,                            \
        CCC_realtime_ordered_map *: CCC_rom_insert_or_assign)(                 \
        (container_ptr), insert_or_assign_args)

#define CCC_impl_insert_or_assign_r(container_ptr, insert_or_assign_args...)   \
    _Generic((container_ptr),                                                  \
        CCC_handle_ordered_map *: &(                                           \
                 CCC_handle){CCC_hom_insert_or_assign(                         \
                                 (CCC_handle_ordered_map *)container_ptr,      \
                                 insert_or_assign_args)                        \
                                 .impl},                                       \
        CCC_handle_realtime_ordered_map *: &(                                  \
                 CCC_handle){CCC_hrm_insert_or_assign(                         \
                                 (CCC_handle_realtime_ordered_map *)           \
                                     container_ptr,                            \
                                 insert_or_assign_args)                        \
                                 .impl},                                       \
        CCC_flat_hash_map *: &(                                                \
                 CCC_entry){CCC_fhm_insert_or_assign(                          \
                                (CCC_flat_hash_map *)container_ptr,            \
                                insert_or_assign_args)                         \
                                .impl},                                        \
        CCC_ordered_map *: &(                                                  \
                 CCC_entry){CCC_om_insert_or_assign(                           \
                                (CCC_ordered_map *)container_ptr,              \
                                (CCC_omap_elem *)insert_or_assign_args)        \
                                .impl},                                        \
        CCC_realtime_ordered_map *: &(CCC_entry){                              \
            CCC_rom_insert_or_assign(                                          \
                (CCC_realtime_ordered_map *)container_ptr,                     \
                (CCC_romap_elem *)insert_or_assign_args)                       \
                .impl})

#define CCC_impl_remove(container_ptr, key_val_container_handle_ptr...)        \
    _Generic((container_ptr),                                                  \
        CCC_handle_ordered_map *: CCC_hom_remove,                              \
        CCC_handle_realtime_ordered_map *: CCC_hrm_remove,                     \
        CCC_flat_hash_map *: CCC_fhm_remove,                                   \
        CCC_ordered_map *: CCC_om_remove,                                      \
        CCC_realtime_ordered_map *: CCC_rom_remove)(                           \
        (container_ptr), key_val_container_handle_ptr)

#define CCC_impl_remove_r(container_ptr, key_val_container_handle_ptr...)      \
    _Generic((container_ptr),                                                  \
        CCC_handle_ordered_map *: &(                                           \
                 CCC_handle){CCC_hom_remove(                                   \
                                 (CCC_handle_ordered_map *)container_ptr,      \
                                 key_val_container_handle_ptr)                 \
                                 .impl},                                       \
        CCC_handle_realtime_ordered_map *: &(                                  \
                 CCC_handle){CCC_hrm_remove(                                   \
                                 (CCC_handle_realtime_ordered_map *)           \
                                     container_ptr,                            \
                                 key_val_container_handle_ptr)                 \
                                 .impl},                                       \
        CCC_flat_hash_map *: &(                                                \
                 CCC_entry){CCC_fhm_remove((CCC_flat_hash_map *)container_ptr, \
                                           key_val_container_handle_ptr)       \
                                .impl},                                        \
        CCC_ordered_map *: &(                                                  \
                 CCC_entry){CCC_om_remove(                                     \
                                (CCC_ordered_map *)container_ptr,              \
                                (CCC_omap_elem *)key_val_container_handle_ptr) \
                                .impl},                                        \
        CCC_realtime_ordered_map *: &(CCC_entry){                              \
            CCC_rom_remove((CCC_realtime_ordered_map *)container_ptr,          \
                           (CCC_romap_elem *)key_val_container_handle_ptr)     \
                .impl})

#define CCC_impl_remove_entry(container_entry_ptr)                             \
    _Generic((container_entry_ptr),                                            \
        CCC_fhmap_entry *: CCC_fhm_remove_entry,                               \
        CCC_omap_entry *: CCC_om_remove_entry,                                 \
        CCC_romap_entry *: CCC_rom_remove_entry,                               \
        CCC_fhmap_entry const *: CCC_fhm_remove_entry,                         \
        CCC_omap_entry const *: CCC_om_remove_entry,                           \
        CCC_romap_entry const *: CCC_rom_remove_entry)((container_entry_ptr))

#define CCC_impl_remove_entry_r(container_entry_ptr)                           \
    &(CCC_entry)                                                               \
    {                                                                          \
        CCC_impl_remove_entry(container_entry_ptr).impl                        \
    }

#define CCC_impl_remove_handle(container_handle_ptr)                           \
    _Generic((container_handle_ptr),                                           \
        CCC_homap_handle *: CCC_hom_remove_handle,                             \
        CCC_homap_handle const *: CCC_hom_remove_handle,                       \
        CCC_hromap_handle *: CCC_hrm_remove_handle,                            \
        CCC_hromap_handle const *: CCC_hrm_remove_handle)(                     \
        (container_handle_ptr))

#define CCC_impl_remove_handle_r(container_handle_ptr)                         \
    &(CCC_handle)                                                              \
    {                                                                          \
        CCC_impl_remove_handle(container_handle_ptr).impl                      \
    }

#define CCC_impl_entry(container_ptr, key_ptr...)                              \
    _Generic((container_ptr),                                                  \
        CCC_flat_hash_map *: CCC_fhm_entry,                                    \
        CCC_flat_hash_map const *: CCC_fhm_entry,                              \
        CCC_ordered_map *: CCC_om_entry,                                       \
        CCC_realtime_ordered_map *: CCC_rom_entry,                             \
        CCC_realtime_ordered_map const *: CCC_rom_entry)((container_ptr),      \
                                                         key_ptr)

#define CCC_impl_entry_r(container_ptr, key_ptr...)                            \
    _Generic((container_ptr),                                                  \
        CCC_flat_hash_map *: &(                                                \
                 CCC_fhmap_entry){CCC_fhm_entry(                               \
                                      (CCC_flat_hash_map *)(container_ptr),    \
                                      key_ptr)                                 \
                                      .impl},                                  \
        CCC_flat_hash_map const *: &(                                          \
                 CCC_fhmap_entry){CCC_fhm_entry(                               \
                                      (CCC_flat_hash_map *)(container_ptr),    \
                                      key_ptr)                                 \
                                      .impl},                                  \
        CCC_ordered_map *: &(                                                  \
                 CCC_omap_entry){CCC_om_entry(                                 \
                                     (CCC_ordered_map *)(container_ptr),       \
                                     key_ptr)                                  \
                                     .impl},                                   \
        CCC_realtime_ordered_map *: &(                                         \
                 CCC_romap_entry){CCC_rom_entry((CCC_realtime_ordered_map      \
                                                     *)(container_ptr),        \
                                                key_ptr)                       \
                                      .impl},                                  \
        CCC_realtime_ordered_map const *: &(CCC_romap_entry){                  \
            CCC_rom_entry((CCC_realtime_ordered_map *)(container_ptr),         \
                          key_ptr)                                             \
                .impl})

#define CCC_impl_handle(container_ptr, key_ptr...)                             \
    _Generic((container_ptr),                                                  \
        CCC_handle_ordered_map *: CCC_hom_handle,                              \
        CCC_handle_realtime_ordered_map *: CCC_hrm_handle,                     \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_handle)(              \
        (container_ptr), key_ptr)

#define CCC_impl_handle_r(container_ptr, key_ptr...)                           \
    _Generic((container_ptr),                                                  \
        CCC_handle_ordered_map *: &(                                           \
                 CCC_homap_handle){CCC_hom_handle((CCC_handle_ordered_map      \
                                                       *)(container_ptr),      \
                                                  key_ptr)                     \
                                       .impl},                                 \
        CCC_handle_realtime_ordered_map *: &(                                  \
                 CCC_hromap_handle){CCC_hrm_handle(                            \
                                        (CCC_handle_realtime_ordered_map       \
                                             *)(container_ptr),                \
                                        key_ptr)                               \
                                        .impl},                                \
        CCC_handle_realtime_ordered_map const *: &(CCC_hromap_handle){         \
            CCC_hrm_handle((CCC_handle_realtime_ordered_map *)(container_ptr), \
                           key_ptr)                                            \
                .impl})

#define CCC_impl_and_modify(container_entry_ptr, mod_fn)                       \
    _Generic((container_entry_ptr),                                            \
        CCC_fhmap_entry *: CCC_fhm_and_modify,                                 \
        CCC_omap_entry *: CCC_om_and_modify,                                   \
        CCC_homap_handle *: CCC_hom_and_modify,                                \
        CCC_romap_entry *: CCC_rom_and_modify,                                 \
        CCC_hromap_handle *: CCC_hrm_and_modify,                               \
        CCC_fhmap_entry const *: CCC_fhm_and_modify,                           \
        CCC_hromap_handle const *: CCC_hrm_and_modify,                         \
        CCC_omap_entry const *: CCC_om_and_modify,                             \
        CCC_homap_handle const *: CCC_hom_and_modify,                          \
        CCC_romap_entry const *: CCC_rom_and_modify)((container_entry_ptr),    \
                                                     (mod_fn))

#define CCC_impl_and_modify_aux(container_entry_ptr, mod_fn, aux_data_ptr...)  \
    _Generic((container_entry_ptr),                                            \
        CCC_fhmap_entry *: CCC_fhm_and_modify_aux,                             \
        CCC_omap_entry *: CCC_om_and_modify_aux,                               \
        CCC_homap_handle *: CCC_hom_and_modify_aux,                            \
        CCC_hromap_handle *: CCC_hrm_and_modify_aux,                           \
        CCC_romap_entry *: CCC_rom_and_modify_aux,                             \
        CCC_fhmap_entry const *: CCC_fhm_and_modify_aux,                       \
        CCC_omap_entry const *: CCC_om_and_modify_aux,                         \
        CCC_hromap_handle const *: CCC_hrm_and_modify_aux,                     \
        CCC_homap_handle const *: CCC_hom_and_modify_aux,                      \
        CCC_romap_entry const *: CCC_rom_and_modify_aux)(                      \
        (container_entry_ptr), (mod_fn), aux_data_ptr)

#define CCC_impl_insert_entry(container_entry_ptr,                             \
                              key_val_container_handle_ptr...)                 \
    _Generic((container_entry_ptr),                                            \
        CCC_fhmap_entry *: CCC_fhm_insert_entry,                               \
        CCC_omap_entry *: CCC_om_insert_entry,                                 \
        CCC_romap_entry *: CCC_rom_insert_entry,                               \
        CCC_fhmap_entry const *: CCC_fhm_insert_entry,                         \
        CCC_omap_entry const *: CCC_om_insert_entry,                           \
        CCC_romap_entry const *: CCC_rom_insert_entry)(                        \
        (container_entry_ptr), key_val_container_handle_ptr)

#define CCC_impl_insert_handle(container_handle_ptr,                           \
                               key_val_container_handle_ptr...)                \
    _Generic((container_handle_ptr),                                           \
        CCC_homap_handle *: CCC_hom_insert_handle,                             \
        CCC_homap_handle const *: CCC_hom_insert_handle,                       \
        CCC_hromap_handle *: CCC_hrm_insert_handle,                            \
        CCC_hromap_handle const *: CCC_hrm_insert_handle)(                     \
        (container_handle_ptr), key_val_container_handle_ptr)

#define CCC_impl_or_insert(container_entry_ptr,                                \
                           key_val_container_handle_ptr...)                    \
    _Generic((container_entry_ptr),                                            \
        CCC_fhmap_entry *: CCC_fhm_or_insert,                                  \
        CCC_omap_entry *: CCC_om_or_insert,                                    \
        CCC_homap_handle *: CCC_hom_or_insert,                                 \
        CCC_romap_entry *: CCC_rom_or_insert,                                  \
        CCC_hromap_handle *: CCC_hrm_or_insert,                                \
        CCC_fhmap_entry const *: CCC_fhm_or_insert,                            \
        CCC_omap_entry const *: CCC_om_or_insert,                              \
        CCC_hromap_handle const *: CCC_hrm_or_insert,                          \
        CCC_homap_handle const *: CCC_hom_or_insert,                           \
        CCC_romap_entry const *: CCC_rom_or_insert)(                           \
        (container_entry_ptr), key_val_container_handle_ptr)

#define CCC_impl_unwrap(container_entry_ptr)                                   \
    _Generic((container_entry_ptr),                                            \
        CCC_entry *: CCC_entry_unwrap,                                         \
        CCC_entry const *: CCC_entry_unwrap,                                   \
        CCC_handle *: CCC_handle_unwrap,                                       \
        CCC_handle const *: CCC_handle_unwrap,                                 \
        CCC_fhmap_entry *: CCC_fhm_unwrap,                                     \
        CCC_fhmap_entry const *: CCC_fhm_unwrap,                               \
        CCC_omap_entry *: CCC_om_unwrap,                                       \
        CCC_omap_entry const *: CCC_om_unwrap,                                 \
        CCC_homap_handle *: CCC_hom_unwrap,                                    \
        CCC_homap_handle const *: CCC_hom_unwrap,                              \
        CCC_hromap_handle *: CCC_hrm_unwrap,                                   \
        CCC_hromap_handle const *: CCC_hrm_unwrap,                             \
        CCC_romap_entry *: CCC_rom_unwrap,                                     \
        CCC_romap_entry const *: CCC_rom_unwrap)((container_entry_ptr))

#define CCC_impl_occupied(container_entry_ptr)                                 \
    _Generic((container_entry_ptr),                                            \
        CCC_entry *: CCC_entry_occupied,                                       \
        CCC_entry const *: CCC_entry_occupied,                                 \
        CCC_handle *: CCC_handle_occupied,                                     \
        CCC_handle const *: CCC_handle_occupied,                               \
        CCC_fhmap_entry *: CCC_fhm_occupied,                                   \
        CCC_fhmap_entry const *: CCC_fhm_occupied,                             \
        CCC_omap_entry *: CCC_om_occupied,                                     \
        CCC_omap_entry const *: CCC_om_occupied,                               \
        CCC_homap_handle *: CCC_hom_occupied,                                  \
        CCC_homap_handle const *: CCC_hom_occupied,                            \
        CCC_hromap_handle *: CCC_hrm_occupied,                                 \
        CCC_hromap_handle const *: CCC_hrm_occupied,                           \
        CCC_romap_entry *: CCC_rom_occupied,                                   \
        CCC_romap_entry const *: CCC_rom_occupied)((container_entry_ptr))

#define CCC_impl_insert_error(container_entry_ptr)                             \
    _Generic((container_entry_ptr),                                            \
        CCC_entry *: CCC_entry_insert_error,                                   \
        CCC_entry const *: CCC_entry_insert_error,                             \
        CCC_handle *: CCC_handle_insert_error,                                 \
        CCC_handle const *: CCC_handle_insert_error,                           \
        CCC_fhmap_entry *: CCC_fhm_insert_error,                               \
        CCC_fhmap_entry const *: CCC_fhm_insert_error,                         \
        CCC_omap_entry *: CCC_om_insert_error,                                 \
        CCC_omap_entry const *: CCC_om_insert_error,                           \
        CCC_homap_handle *: CCC_hom_insert_error,                              \
        CCC_homap_handle const *: CCC_hom_insert_error,                        \
        CCC_hromap_handle *: CCC_hrm_insert_error,                             \
        CCC_hromap_handle const *: CCC_hrm_insert_error,                       \
        CCC_romap_entry *: CCC_rom_insert_error,                               \
        CCC_romap_entry const *: CCC_rom_insert_error)((container_entry_ptr))

/*======================    Misc Search Interface ===========================*/

#define CCC_impl_get_key_val(container_ptr, key_ptr...)                        \
    _Generic((container_ptr),                                                  \
        CCC_flat_hash_map *: CCC_fhm_get_key_val,                              \
        CCC_flat_hash_map const *: CCC_fhm_get_key_val,                        \
        CCC_ordered_map *: CCC_om_get_key_val,                                 \
        CCC_handle_ordered_map *: CCC_hom_get_key_val,                         \
        CCC_handle_realtime_ordered_map *: CCC_hrm_get_key_val,                \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_get_key_val,          \
        CCC_realtime_ordered_map *: CCC_rom_get_key_val,                       \
        CCC_realtime_ordered_map const *: CCC_rom_get_key_val)(                \
        (container_ptr), key_ptr)

#define CCC_impl_contains(container_ptr, key_ptr...)                           \
    _Generic((container_ptr),                                                  \
        CCC_flat_hash_map *: CCC_fhm_contains,                                 \
        CCC_flat_hash_map const *: CCC_fhm_contains,                           \
        CCC_ordered_map *: CCC_om_contains,                                    \
        CCC_handle_ordered_map *: CCC_hom_contains,                            \
        CCC_handle_realtime_ordered_map *: CCC_hrm_contains,                   \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_contains,             \
        CCC_realtime_ordered_map *: CCC_rom_contains,                          \
        CCC_realtime_ordered_map const *: CCC_rom_contains)((container_ptr),   \
                                                            key_ptr)

/*================    Sequential Containers Interface   =====================*/

#define CCC_impl_push(container_ptr, container_handle_ptr...)                  \
    _Generic((container_ptr),                                                  \
        CCC_flat_priority_queue *: CCC_fpq_push,                               \
        CCC_priority_queue *: CCC_pq_push)((container_ptr),                    \
                                           container_handle_ptr)

#define CCC_impl_push_back(container_ptr, container_handle_ptr...)             \
    _Generic((container_ptr),                                                  \
        CCC_bitset *: CCC_bs_push_back,                                        \
        CCC_flat_double_ended_queue *: CCC_fdeq_push_back,                     \
        CCC_doubly_linked_list *: CCC_dll_push_back,                           \
        CCC_buffer *: CCC_buf_push_back)((container_ptr),                      \
                                         container_handle_ptr)

#define CCC_impl_push_front(container_ptr, container_handle_ptr...)            \
    _Generic((container_ptr),                                                  \
        CCC_flat_double_ended_queue *: CCC_fdeq_push_front,                    \
        CCC_doubly_linked_list *: CCC_dll_push_front,                          \
        CCC_singly_linked_list *: CCC_sll_push_front)((container_ptr),         \
                                                      container_handle_ptr)

#define CCC_impl_pop(container_ptr, ...)                                       \
    _Generic((container_ptr),                                                  \
        CCC_flat_priority_queue *: CCC_fpq_pop,                                \
        CCC_priority_queue *: CCC_pq_pop)(                                     \
        (container_ptr)__VA_OPT__(, __VA_ARGS__))

#define CCC_impl_pop_front(container_ptr)                                      \
    _Generic((container_ptr),                                                  \
        CCC_flat_double_ended_queue *: CCC_fdeq_pop_front,                     \
        CCC_doubly_linked_list *: CCC_dll_pop_front,                           \
        CCC_singly_linked_list *: CCC_sll_pop_front)((container_ptr))

#define CCC_impl_pop_back(container_ptr)                                       \
    _Generic((container_ptr),                                                  \
        CCC_flat_double_ended_queue *: CCC_fdeq_pop_back,                      \
        CCC_doubly_linked_list *: CCC_dll_pop_back,                            \
        CCC_buffer *: CCC_buf_pop_back)((container_ptr))

#define CCC_impl_front(container_ptr)                                          \
    _Generic((container_ptr),                                                  \
        CCC_flat_double_ended_queue *: CCC_fdeq_front,                         \
        CCC_doubly_linked_list *: CCC_dll_front,                               \
        CCC_flat_priority_queue *: CCC_fpq_front,                              \
        CCC_priority_queue *: CCC_pq_front,                                    \
        CCC_singly_linked_list *: CCC_sll_front,                               \
        CCC_flat_double_ended_queue const *: CCC_fdeq_front,                   \
        CCC_doubly_linked_list const *: CCC_dll_front,                         \
        CCC_flat_priority_queue const *: CCC_fpq_front,                        \
        CCC_priority_queue const *: CCC_pq_front,                              \
        CCC_singly_linked_list const *: CCC_sll_front)((container_ptr))

#define CCC_impl_back(container_ptr)                                           \
    _Generic((container_ptr),                                                  \
        CCC_flat_double_ended_queue *: CCC_fdeq_back,                          \
        CCC_doubly_linked_list *: CCC_dll_back,                                \
        CCC_buffer *: CCC_buf_back,                                            \
        CCC_flat_double_ended_queue const *: CCC_fdeq_back,                    \
        CCC_doubly_linked_list const *: CCC_dll_back,                          \
        CCC_buffer const *: CCC_buf_back)((container_ptr))

/*================       Priority Queue Update Interface =====================*/

#define CCC_impl_update(container_ptr, update_args...)                         \
    _Generic((container_ptr),                                                  \
        CCC_flat_priority_queue *: CCC_fpq_update,                             \
        CCC_priority_queue *: CCC_pq_update)((container_ptr), update_args)

#define CCC_impl_increase(container_ptr, increase_args...)                     \
    _Generic((container_ptr),                                                  \
        CCC_flat_priority_queue *: CCC_fpq_increase,                           \
        CCC_priority_queue *: CCC_pq_increase)((container_ptr), increase_args)

#define CCC_impl_decrease(container_ptr, decrease_args...)                     \
    _Generic((container_ptr),                                                  \
        CCC_flat_priority_queue *: CCC_fpq_decrease,                           \
        CCC_priority_queue *: CCC_pq_decrease)((container_ptr), decrease_args)

#define CCC_impl_extract(container_ptr, container_handle_ptr...)               \
    _Generic((container_ptr),                                                  \
        CCC_doubly_linked_list *: CCC_dll_extract,                             \
        CCC_singly_linked_list *: CCC_sll_extract,                             \
        CCC_priority_queue *: CCC_pq_extract)((container_ptr),                 \
                                              container_handle_ptr)

#define CCC_impl_erase(container_ptr, container_handle_ptr...)                 \
    _Generic((container_ptr), CCC_flat_priority_queue *: CCC_fpq_erase)(       \
        (container_ptr), container_handle_ptr)

#define CCC_impl_extract_range(container_ptr,                                  \
                               container_handle_begin_end_ptr...)              \
    _Generic((container_ptr),                                                  \
        CCC_doubly_linked_list *: CCC_dll_extract_range,                       \
        CCC_singly_linked_list *: CCC_sll_extract_range)(                      \
        (container_ptr), container_handle_begin_end_ptr)

/*===================       Iterators Interface ==============================*/

#define CCC_impl_begin(container_ptr)                                          \
    _Generic((container_ptr),                                                  \
        CCC_buffer *: CCC_buf_begin,                                           \
        CCC_flat_hash_map *: CCC_fhm_begin,                                    \
        CCC_ordered_map *: CCC_om_begin,                                       \
        CCC_handle_ordered_map *: CCC_hom_begin,                               \
        CCC_flat_double_ended_queue *: CCC_fdeq_begin,                         \
        CCC_singly_linked_list *: CCC_sll_begin,                               \
        CCC_doubly_linked_list *: CCC_dll_begin,                               \
        CCC_realtime_ordered_map *: CCC_rom_begin,                             \
        CCC_handle_realtime_ordered_map *: CCC_hrm_begin,                      \
        CCC_buffer const *: CCC_buf_begin,                                     \
        CCC_flat_hash_map const *: CCC_fhm_begin,                              \
        CCC_ordered_map const *: CCC_om_begin,                                 \
        CCC_handle_ordered_map const *: CCC_hom_begin,                         \
        CCC_flat_double_ended_queue const *: CCC_fdeq_begin,                   \
        CCC_singly_linked_list const *: CCC_sll_begin,                         \
        CCC_doubly_linked_list const *: CCC_dll_begin,                         \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_begin,                \
        CCC_realtime_ordered_map const *: CCC_rom_begin)((container_ptr))

#define CCC_impl_rbegin(container_ptr)                                         \
    _Generic((container_ptr),                                                  \
        CCC_buffer *: CCC_buf_rbegin,                                          \
        CCC_ordered_map *: CCC_om_rbegin,                                      \
        CCC_handle_ordered_map *: CCC_hom_rbegin,                              \
        CCC_flat_double_ended_queue *: CCC_fdeq_rbegin,                        \
        CCC_doubly_linked_list *: CCC_dll_rbegin,                              \
        CCC_realtime_ordered_map *: CCC_rom_rbegin,                            \
        CCC_handle_realtime_ordered_map *: CCC_hrm_rbegin,                     \
        CCC_buffer const *: CCC_buf_rbegin,                                    \
        CCC_ordered_map const *: CCC_om_rbegin,                                \
        CCC_handle_ordered_map const *: CCC_hom_rbegin,                        \
        CCC_flat_double_ended_queue const *: CCC_fdeq_rbegin,                  \
        CCC_doubly_linked_list const *: CCC_dll_rbegin,                        \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_rbegin,               \
        CCC_realtime_ordered_map const *: CCC_rom_rbegin)((container_ptr))

#define CCC_impl_next(container_ptr, void_iter_ptr)                            \
    _Generic((container_ptr),                                                  \
        CCC_buffer *: CCC_buf_next,                                            \
        CCC_flat_hash_map *: CCC_fhm_next,                                     \
        CCC_ordered_map *: CCC_om_next,                                        \
        CCC_handle_ordered_map *: CCC_hom_next,                                \
        CCC_flat_double_ended_queue *: CCC_fdeq_next,                          \
        CCC_singly_linked_list *: CCC_sll_next,                                \
        CCC_doubly_linked_list *: CCC_dll_next,                                \
        CCC_realtime_ordered_map *: CCC_rom_next,                              \
        CCC_handle_realtime_ordered_map *: CCC_hrm_next,                       \
        CCC_buffer const *: CCC_buf_next,                                      \
        CCC_flat_hash_map const *: CCC_fhm_next,                               \
        CCC_ordered_map const *: CCC_om_next,                                  \
        CCC_handle_ordered_map const *: CCC_hom_next,                          \
        CCC_flat_double_ended_queue const *: CCC_fdeq_next,                    \
        CCC_singly_linked_list const *: CCC_sll_next,                          \
        CCC_doubly_linked_list const *: CCC_dll_next,                          \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_next,                 \
        CCC_realtime_ordered_map const *: CCC_rom_next)((container_ptr),       \
                                                        (void_iter_ptr))

#define CCC_impl_rnext(container_ptr, void_iter_ptr)                           \
    _Generic((container_ptr),                                                  \
        CCC_buffer *: CCC_buf_rnext,                                           \
        CCC_ordered_map *: CCC_om_rnext,                                       \
        CCC_handle_ordered_map *: CCC_hom_rnext,                               \
        CCC_flat_double_ended_queue *: CCC_fdeq_rnext,                         \
        CCC_doubly_linked_list *: CCC_dll_rnext,                               \
        CCC_realtime_ordered_map *: CCC_rom_rnext,                             \
        CCC_handle_realtime_ordered_map *: CCC_hrm_rnext,                      \
        CCC_buffer const *: CCC_buf_rnext,                                     \
        CCC_ordered_map const *: CCC_om_rnext,                                 \
        CCC_handle_ordered_map const *: CCC_hom_rnext,                         \
        CCC_flat_double_ended_queue const *: CCC_fdeq_rnext,                   \
        CCC_doubly_linked_list const *: CCC_dll_rnext,                         \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_rnext,                \
        CCC_realtime_ordered_map const *: CCC_rom_rnext)((container_ptr),      \
                                                         (void_iter_ptr))

#define CCC_impl_end(container_ptr)                                            \
    _Generic((container_ptr),                                                  \
        CCC_buffer *: CCC_buf_end,                                             \
        CCC_flat_hash_map *: CCC_fhm_end,                                      \
        CCC_ordered_map *: CCC_om_end,                                         \
        CCC_handle_ordered_map *: CCC_hom_end,                                 \
        CCC_flat_double_ended_queue *: CCC_fdeq_end,                           \
        CCC_singly_linked_list *: CCC_sll_end,                                 \
        CCC_doubly_linked_list *: CCC_dll_end,                                 \
        CCC_realtime_ordered_map *: CCC_rom_end,                               \
        CCC_handle_realtime_ordered_map *: CCC_hrm_end,                        \
        CCC_buffer const *: CCC_buf_end,                                       \
        CCC_flat_hash_map const *: CCC_fhm_end,                                \
        CCC_ordered_map const *: CCC_om_end,                                   \
        CCC_handle_ordered_map const *: CCC_hom_end,                           \
        CCC_flat_double_ended_queue const *: CCC_fdeq_end,                     \
        CCC_singly_linked_list const *: CCC_sll_end,                           \
        CCC_doubly_linked_list const *: CCC_dll_end,                           \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_end,                  \
        CCC_realtime_ordered_map const *: CCC_rom_end)((container_ptr))

#define CCC_impl_rend(container_ptr)                                           \
    _Generic((container_ptr),                                                  \
        CCC_buffer *: CCC_buf_rend,                                            \
        CCC_ordered_map *: CCC_om_rend,                                        \
        CCC_handle_ordered_map *: CCC_hom_rend,                                \
        CCC_flat_double_ended_queue *: CCC_fdeq_rend,                          \
        CCC_doubly_linked_list *: CCC_dll_rend,                                \
        CCC_realtime_ordered_map *: CCC_rom_rend,                              \
        CCC_handle_realtime_ordered_map *: CCC_hrm_rend,                       \
        CCC_buffer const *: CCC_buf_rend,                                      \
        CCC_ordered_map const *: CCC_om_rend,                                  \
        CCC_handle_ordered_map const *: CCC_hom_rend,                          \
        CCC_flat_double_ended_queue const *: CCC_fdeq_rend,                    \
        CCC_doubly_linked_list const *: CCC_dll_rend,                          \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_rend,                 \
        CCC_realtime_ordered_map const *: CCC_rom_rend)((container_ptr))

#define CCC_impl_equal_range(container_ptr, begin_and_end_key_ptr...)          \
    _Generic((container_ptr),                                                  \
        CCC_ordered_map *: CCC_om_equal_range,                                 \
        CCC_handle_ordered_map *: CCC_hom_equal_range,                         \
        CCC_handle_realtime_ordered_map *: CCC_hrm_equal_range,                \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_equal_range,          \
        CCC_realtime_ordered_map *: CCC_rom_equal_range,                       \
        CCC_realtime_ordered_map const *: CCC_rom_equal_range)(                \
        (container_ptr), begin_and_end_key_ptr)

#define CCC_impl_equal_range_r(container_ptr, begin_and_end_key_ptr...)        \
    &(CCC_range)                                                               \
    {                                                                          \
        CCC_impl_equal_range(container_ptr, begin_and_end_key_ptr).impl        \
    }

#define CCC_impl_equal_rrange(container_ptr, rbegin_and_rend_key_ptr...)       \
    _Generic((container_ptr),                                                  \
        CCC_ordered_map *: CCC_om_equal_rrange,                                \
        CCC_handle_ordered_map *: CCC_hom_equal_rrange,                        \
        CCC_handle_realtime_ordered_map *: CCC_hrm_equal_rrange,               \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_equal_rrange,         \
        CCC_realtime_ordered_map *: CCC_rom_equal_rrange,                      \
        CCC_realtime_ordered_map const *: CCC_rom_equal_rrange)(               \
        (container_ptr), rbegin_and_rend_key_ptr)

#define CCC_impl_equal_rrange_r(container_ptr, rbegin_and_rend_key_ptr...)     \
    &(CCC_rrange)                                                              \
    {                                                                          \
        CCC_impl_equal_rrange(container_ptr, rbegin_and_rend_key_ptr).impl     \
    }

#define CCC_impl_splice(container_ptr, splice_args...)                         \
    _Generic((container_ptr),                                                  \
        CCC_singly_linked_list *: CCC_sll_splice,                              \
        CCC_singly_linked_list const *: CCC_sll_splice,                        \
        CCC_doubly_linked_list *: CCC_dll_splice,                              \
        CCC_doubly_linked_list const *: CCC_dll_splice)((container_ptr),       \
                                                        splice_args)

#define CCC_impl_splice_range(container_ptr, splice_range_args...)             \
    _Generic((container_ptr),                                                  \
        CCC_singly_linked_list *: CCC_sll_splice_range,                        \
        CCC_singly_linked_list const *: CCC_sll_splice_range,                  \
        CCC_doubly_linked_list *: CCC_dll_splice_range,                        \
        CCC_doubly_linked_list const *: CCC_dll_splice_range)(                 \
        (container_ptr), splice_range_args)

/*===================         Memory Management       =======================*/

#define CCC_impl_copy(dst_container_ptr, src_container_ptr, alloc_fn_ptr)      \
    _Generic((dst_container_ptr),                                              \
        CCC_bitset *: CCC_bs_copy,                                             \
        CCC_flat_hash_map *: CCC_fhm_copy,                                     \
        CCC_handle_ordered_map *: CCC_hom_copy,                                \
        CCC_flat_priority_queue *: CCC_fpq_copy,                               \
        CCC_flat_double_ended_queue *: CCC_fdeq_copy,                          \
        CCC_handle_realtime_ordered_map *: CCC_hrm_copy)(                      \
        (dst_container_ptr), (src_container_ptr), (alloc_fn_ptr))

#define CCC_impl_reserve(container_ptr, n_to_add, alloc_fn_ptr)                \
    _Generic((container_ptr),                                                  \
        CCC_bitset *: CCC_bs_reserve,                                          \
        CCC_buffer *: CCC_buf_reserve,                                         \
        CCC_flat_hash_map *: CCC_fhm_reserve,                                  \
        CCC_handle_ordered_map *: CCC_hom_reserve,                             \
        CCC_flat_priority_queue *: CCC_fpq_reserve,                            \
        CCC_flat_double_ended_queue *: CCC_fdeq_reserve,                       \
        CCC_handle_realtime_ordered_map *: CCC_hrm_reserve)(                   \
        (container_ptr), (n_to_add), (alloc_fn_ptr))

#define CCC_impl_clear(container_ptr, ...)                                     \
    _Generic((container_ptr),                                                  \
        CCC_bitset *: CCC_bs_clear,                                            \
        CCC_buffer *: CCC_buf_clear,                                           \
        CCC_flat_hash_map *: CCC_fhm_clear,                                    \
        CCC_handle_ordered_map *: CCC_hom_clear,                               \
        CCC_flat_priority_queue *: CCC_fpq_clear,                              \
        CCC_flat_double_ended_queue *: CCC_fdeq_clear,                         \
        CCC_singly_linked_list *: CCC_sll_clear,                               \
        CCC_doubly_linked_list *: CCC_dll_clear,                               \
        CCC_ordered_map *: CCC_om_clear,                                       \
        CCC_priority_queue *: CCC_pq_clear,                                    \
        CCC_realtime_ordered_map *: CCC_rom_clear,                             \
        CCC_handle_realtime_ordered_map *: CCC_hrm_clear)(                     \
        (container_ptr)__VA_OPT__(, __VA_ARGS__))

#define CCC_impl_clear_and_free(container_ptr, ...)                            \
    _Generic((container_ptr),                                                  \
        CCC_bitset *: CCC_bs_clear_and_free,                                   \
        CCC_buffer *: CCC_buf_clear_and_free,                                  \
        CCC_flat_hash_map *: CCC_fhm_clear_and_free,                           \
        CCC_handle_ordered_map *: CCC_hom_clear_and_free,                      \
        CCC_flat_priority_queue *: CCC_fpq_clear_and_free,                     \
        CCC_flat_double_ended_queue *: CCC_fdeq_clear_and_free,                \
        CCC_handle_realtime_ordered_map *: CCC_hrm_clear_and_free)(            \
        (container_ptr)__VA_OPT__(, __VA_ARGS__))

#define CCC_impl_clear_and_free_reserve(container_ptr,                         \
                                        destructor_and_free_args...)           \
    _Generic((container_ptr),                                                  \
        CCC_bitset *: CCC_bs_clear_and_free_reserve,                           \
        CCC_buffer *: CCC_buf_clear_and_free_reserve,                          \
        CCC_flat_hash_map *: CCC_fhm_clear_and_free_reserve,                   \
        CCC_handle_ordered_map *: CCC_hom_clear_and_free_reserve,              \
        CCC_flat_priority_queue *: CCC_fpq_clear_and_free_reserve,             \
        CCC_flat_double_ended_queue *: CCC_fdeq_clear_and_free_reserve,        \
        CCC_handle_realtime_ordered_map *: CCC_hrm_clear_and_free_reserve)(    \
        (container_ptr), destructor_and_free_args)

/*===================    Standard Getters Interface   =======================*/

#define CCC_impl_count(container_ptr)                                          \
    _Generic((container_ptr),                                                  \
        CCC_bitset *: CCC_bs_count,                                            \
        CCC_buffer *: CCC_buf_count,                                           \
        CCC_flat_hash_map *: CCC_fhm_count,                                    \
        CCC_ordered_map *: CCC_om_count,                                       \
        CCC_handle_ordered_map *: CCC_hom_count,                               \
        CCC_flat_priority_queue *: CCC_fpq_count,                              \
        CCC_flat_double_ended_queue *: CCC_fdeq_count,                         \
        CCC_priority_queue *: CCC_pq_count,                                    \
        CCC_singly_linked_list *: CCC_sll_count,                               \
        CCC_doubly_linked_list *: CCC_dll_count,                               \
        CCC_realtime_ordered_map *: CCC_rom_count,                             \
        CCC_handle_realtime_ordered_map *: CCC_hrm_count,                      \
        CCC_buffer const *: CCC_buf_count,                                     \
        CCC_flat_hash_map const *: CCC_fhm_count,                              \
        CCC_ordered_map const *: CCC_om_count,                                 \
        CCC_handle_ordered_map const *: CCC_hom_count,                         \
        CCC_flat_priority_queue const *: CCC_fpq_count,                        \
        CCC_flat_double_ended_queue const *: CCC_fdeq_count,                   \
        CCC_priority_queue const *: CCC_pq_count,                              \
        CCC_singly_linked_list const *: CCC_sll_count,                         \
        CCC_doubly_linked_list const *: CCC_dll_count,                         \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_count,                \
        CCC_realtime_ordered_map const *: CCC_rom_count)((container_ptr))

#define CCC_impl_capacity(container_ptr)                                       \
    _Generic((container_ptr),                                                  \
        CCC_bitset *: CCC_bs_capacity,                                         \
        CCC_buffer *: CCC_buf_capacity,                                        \
        CCC_flat_hash_map *: CCC_fhm_capacity,                                 \
        CCC_handle_ordered_map *: CCC_hom_capacity,                            \
        CCC_flat_priority_queue *: CCC_fpq_capacity,                           \
        CCC_flat_double_ended_queue *: CCC_fdeq_capacity,                      \
        CCC_handle_realtime_ordered_map *: CCC_hrm_capacity,                   \
        CCC_bitset const *: CCC_bs_capacity,                                   \
        CCC_buffer const *: CCC_buf_capacity,                                  \
        CCC_flat_hash_map const *: CCC_fhm_capacity,                           \
        CCC_handle_ordered_map const *: CCC_hom_capacity,                      \
        CCC_flat_priority_queue const *: CCC_fpq_capacity,                     \
        CCC_flat_double_ended_queue const *: CCC_fdeq_capacity,                \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_capacity)(            \
        (container_ptr))

#define CCC_impl_is_empty(container_ptr)                                       \
    _Generic((container_ptr),                                                  \
        CCC_buffer *: CCC_buf_is_empty,                                        \
        CCC_flat_hash_map *: CCC_fhm_is_empty,                                 \
        CCC_ordered_map *: CCC_om_is_empty,                                    \
        CCC_handle_ordered_map *: CCC_hom_is_empty,                            \
        CCC_flat_priority_queue *: CCC_fpq_is_empty,                           \
        CCC_flat_double_ended_queue *: CCC_fdeq_is_empty,                      \
        CCC_priority_queue *: CCC_pq_is_empty,                                 \
        CCC_singly_linked_list *: CCC_sll_is_empty,                            \
        CCC_doubly_linked_list *: CCC_dll_is_empty,                            \
        CCC_realtime_ordered_map *: CCC_rom_is_empty,                          \
        CCC_handle_realtime_ordered_map *: CCC_hrm_is_empty,                   \
        CCC_buffer const *: CCC_buf_is_empty,                                  \
        CCC_flat_hash_map const *: CCC_fhm_is_empty,                           \
        CCC_ordered_map const *: CCC_om_is_empty,                              \
        CCC_handle_ordered_map const *: CCC_hom_is_empty,                      \
        CCC_flat_priority_queue const *: CCC_fpq_is_empty,                     \
        CCC_flat_double_ended_queue const *: CCC_fdeq_is_empty,                \
        CCC_priority_queue const *: CCC_pq_is_empty,                           \
        CCC_singly_linked_list const *: CCC_sll_is_empty,                      \
        CCC_doubly_linked_list const *: CCC_dll_is_empty,                      \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_is_empty,             \
        CCC_realtime_ordered_map const *: CCC_rom_is_empty)((container_ptr))

#define CCC_impl_validate(container_ptr)                                       \
    _Generic((container_ptr),                                                  \
        CCC_flat_hash_map *: CCC_fhm_validate,                                 \
        CCC_ordered_map *: CCC_om_validate,                                    \
        CCC_handle_ordered_map *: CCC_hom_validate,                            \
        CCC_flat_priority_queue *: CCC_fpq_validate,                           \
        CCC_flat_double_ended_queue *: CCC_fdeq_validate,                      \
        CCC_priority_queue *: CCC_pq_validate,                                 \
        CCC_singly_linked_list *: CCC_sll_validate,                            \
        CCC_doubly_linked_list *: CCC_dll_validate,                            \
        CCC_realtime_ordered_map *: CCC_rom_validate,                          \
        CCC_handle_realtime_ordered_map *: CCC_hrm_validate,                   \
        CCC_flat_hash_map const *: CCC_fhm_validate,                           \
        CCC_ordered_map const *: CCC_om_validate,                              \
        CCC_handle_ordered_map const *: CCC_hom_validate,                      \
        CCC_flat_priority_queue const *: CCC_fpq_validate,                     \
        CCC_flat_double_ended_queue const *: CCC_fdeq_validate,                \
        CCC_priority_queue const *: CCC_pq_validate,                           \
        CCC_singly_linked_list const *: CCC_sll_validate,                      \
        CCC_doubly_linked_list const *: CCC_dll_validate,                      \
        CCC_handle_realtime_ordered_map const *: CCC_hrm_validate,             \
        CCC_realtime_ordered_map const *: CCC_rom_validate)((container_ptr))

#endif /* CCC_IMPL_TRAITS_H */
