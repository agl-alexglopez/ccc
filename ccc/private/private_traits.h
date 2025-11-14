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
#ifndef CCC_PRIVATE_TRAITS_H
#define CCC_PRIVATE_TRAITS_H

/* NOLINTBEGIN */
#include "../adaptive_map.h"
#include "../bitset.h"
#include "../bounded_map.h"
#include "../buffer.h"
#include "../doubly_linked_list.h"
#include "../flat_double_ended_queue.h"
#include "../flat_hash_map.h"
#include "../flat_priority_queue.h"
#include "../handle_adaptive_map.h"
#include "../handle_bounded_map.h"
#include "../priority_queue.h"
#include "../singly_linked_list.h"
#include "../types.h"
/* NOLINTEND */

/*====================     Entry/Handle Interface   =========================*/

#define CCC_private_swap_entry(container_pointer, swap_args...)                \
    _Generic((container_pointer),                                              \
        CCC_Flat_hash_map *: CCC_flat_hash_map_swap_entry,                     \
        CCC_Adaptive_map *: CCC_adaptive_map_swap_entry,                       \
        CCC_Bounded_map *: CCC_bounded_map_swap_entry)((container_pointer),    \
                                                       swap_args)

#define CCC_private_swap_entry_r(container_pointer,                            \
                                 key_val_container_handle_pointer...)          \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_private_swap_entry(container_pointer,                              \
                               key_val_container_handle_pointer)               \
            .private                                                           \
    }

#define CCC_private_swap_handle(container_pointer, swap_args...)               \
    _Generic((container_pointer),                                              \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_swap_handle,        \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_swap_handle)(         \
        (container_pointer), swap_args)

#define CCC_private_swap_handle_r(container_pointer,                           \
                                  key_val_container_handle_pointer...)         \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_private_swap_handle(container_pointer,                             \
                                key_val_container_handle_pointer)              \
            .private                                                           \
    }

#define CCC_private_try_insert(container_pointer, try_insert_args...)          \
    _Generic((container_pointer),                                              \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_try_insert,         \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_try_insert,           \
        CCC_Flat_hash_map *: CCC_flat_hash_map_try_insert,                     \
        CCC_Adaptive_map *: CCC_adaptive_map_try_insert,                       \
        CCC_Bounded_map *: CCC_bounded_map_try_insert)((container_pointer),    \
                                                       try_insert_args)

#define CCC_private_try_insert_r(container_pointer, try_insert_args...)        \
    _Generic((container_pointer),                                              \
        CCC_Handle_adaptive_map *: &(                                          \
                 CCC_Handle){CCC_handle_adaptive_map_try_insert(               \
                                 (CCC_Handle_adaptive_map *)container_pointer, \
                                 try_insert_args)                              \
                                 .private},                                    \
        CCC_Handle_bounded_map *: &(                                           \
                 CCC_Handle){CCC_handle_bounded_map_try_insert(                \
                                 (CCC_Handle_bounded_map *)container_pointer,  \
                                 try_insert_args)                              \
                                 .private},                                    \
        CCC_Flat_hash_map *: &(                                                \
                 CCC_Entry){CCC_flat_hash_map_try_insert(                      \
                                (CCC_Flat_hash_map *)container_pointer,        \
                                try_insert_args)                               \
                                .private},                                     \
        CCC_Adaptive_map *: &(                                                 \
                 CCC_Entry){CCC_adaptive_map_try_insert(                       \
                                (CCC_Adaptive_map *)container_pointer,         \
                                (CCC_Adaptive_map_node *)try_insert_args)      \
                                .private},                                     \
        CCC_Bounded_map *: &(CCC_Entry){                                       \
            CCC_bounded_map_try_insert(                                        \
                (CCC_Bounded_map *)container_pointer,                          \
                (CCC_Bounded_map_node *)try_insert_args)                       \
                .private})

#define CCC_private_insert_or_assign(container_pointer,                        \
                                     insert_or_assign_args...)                 \
    _Generic((container_pointer),                                              \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_insert_or_assign,   \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_insert_or_assign,     \
        CCC_Flat_hash_map *: CCC_flat_hash_map_insert_or_assign,               \
        CCC_Adaptive_map *: CCC_adaptive_map_insert_or_assign,                 \
        CCC_Bounded_map *: CCC_bounded_map_insert_or_assign)(                  \
        (container_pointer), insert_or_assign_args)

#define CCC_private_insert_or_assign_r(container_pointer,                      \
                                       insert_or_assign_args...)               \
    _Generic((container_pointer),                                              \
        CCC_Handle_adaptive_map *: &(                                          \
                 CCC_Handle){CCC_handle_adaptive_map_insert_or_assign(         \
                                 (CCC_Handle_adaptive_map *)container_pointer, \
                                 insert_or_assign_args)                        \
                                 .private},                                    \
        CCC_Handle_bounded_map *: &(                                           \
                 CCC_Handle){CCC_handle_bounded_map_insert_or_assign(          \
                                 (CCC_Handle_bounded_map *)container_pointer,  \
                                 insert_or_assign_args)                        \
                                 .private},                                    \
        CCC_Flat_hash_map *: &(                                                \
                 CCC_Entry){CCC_flat_hash_map_insert_or_assign(                \
                                (CCC_Flat_hash_map *)container_pointer,        \
                                insert_or_assign_args)                         \
                                .private},                                     \
        CCC_Adaptive_map *: &(                                                 \
                 CCC_Entry){CCC_adaptive_map_insert_or_assign(                 \
                                (CCC_Adaptive_map *)container_pointer,         \
                                (CCC_Adaptive_map_node *)                      \
                                    insert_or_assign_args)                     \
                                .private},                                     \
        CCC_Bounded_map *: &(CCC_Entry){                                       \
            CCC_bounded_map_insert_or_assign(                                  \
                (CCC_Bounded_map *)container_pointer,                          \
                (CCC_Bounded_map_node *)insert_or_assign_args)                 \
                .private})

#define CCC_private_remove(container_pointer,                                  \
                           key_val_container_handle_pointer...)                \
    _Generic((container_pointer),                                              \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_remove,             \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_remove,               \
        CCC_Flat_hash_map *: CCC_flat_hash_map_remove,                         \
        CCC_Adaptive_map *: CCC_adaptive_map_remove,                           \
        CCC_Bounded_map *: CCC_bounded_map_remove)(                            \
        (container_pointer), key_val_container_handle_pointer)

#define CCC_private_remove_r(container_pointer,                                \
                             key_val_container_handle_pointer...)              \
    _Generic((container_pointer),                                              \
        CCC_Handle_adaptive_map *: &(                                          \
                 CCC_Handle){CCC_handle_adaptive_map_remove(                   \
                                 (CCC_Handle_adaptive_map *)container_pointer, \
                                 key_val_container_handle_pointer)             \
                                 .private},                                    \
        CCC_Handle_bounded_map *: &(                                           \
                 CCC_Handle){CCC_handle_bounded_map_remove(                    \
                                 (CCC_Handle_bounded_map *)container_pointer,  \
                                 key_val_container_handle_pointer)             \
                                 .private},                                    \
        CCC_Flat_hash_map *: &(                                                \
                 CCC_Entry){CCC_flat_hash_map_remove(                          \
                                (CCC_Flat_hash_map *)container_pointer,        \
                                key_val_container_handle_pointer)              \
                                .private},                                     \
        CCC_Adaptive_map *: &(                                                 \
                 CCC_Entry){CCC_adaptive_map_remove(                           \
                                (CCC_Adaptive_map *)container_pointer,         \
                                (CCC_Adaptive_map_node *)                      \
                                    key_val_container_handle_pointer)          \
                                .private},                                     \
        CCC_Bounded_map *: &(CCC_Entry){                                       \
            CCC_bounded_map_remove(                                            \
                (CCC_Bounded_map *)container_pointer,                          \
                (CCC_Bounded_map_node *)key_val_container_handle_pointer)      \
                .private})

#define CCC_private_remove_entry(container_entry_pointer)                      \
    _Generic((container_entry_pointer),                                        \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_remove_entry,             \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_remove_entry,               \
        CCC_Bounded_map_entry *: CCC_bounded_map_remove_entry,                 \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_remove_entry,       \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_remove_entry,         \
        CCC_Bounded_map_entry const *: CCC_bounded_map_remove_entry)(          \
        (container_entry_pointer))

#define CCC_private_remove_entry_r(container_entry_pointer)                    \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_private_remove_entry(container_entry_pointer).private              \
    }

#define CCC_private_remove_handle(container_handle_pointer)                    \
    _Generic((container_handle_pointer),                                       \
        CCC_Handle_adaptive_map_handle                                         \
            *: CCC_handle_adaptive_map_remove_handle,                          \
        CCC_Handle_adaptive_map_handle const                                   \
            *: CCC_handle_adaptive_map_remove_handle,                          \
        CCC_Handle_bounded_map_handle *: CCC_handle_bounded_map_remove_handle, \
        CCC_Handle_bounded_map_handle const                                    \
            *: CCC_handle_bounded_map_remove_handle)(                          \
        (container_handle_pointer))

#define CCC_private_remove_handle_r(container_handle_pointer)                  \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_private_remove_handle(container_handle_pointer).private            \
    }

#define CCC_private_entry(container_pointer, key_pointer...)                   \
    _Generic((container_pointer),                                              \
        CCC_Flat_hash_map *: CCC_flat_hash_map_entry,                          \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_entry,                    \
        CCC_Adaptive_map *: CCC_adaptive_map_entry,                            \
        CCC_Bounded_map *: CCC_bounded_map_entry,                              \
        CCC_Bounded_map const *: CCC_bounded_map_entry)((container_pointer),   \
                                                        key_pointer)

#define CCC_private_entry_r(container_pointer, key_pointer...)                 \
    _Generic((container_pointer),                                              \
        CCC_Flat_hash_map *: &(                                                \
                 CCC_Flat_hash_map_entry){CCC_flat_hash_map_entry(             \
                                              (CCC_Flat_hash_map               \
                                                   *)(container_pointer),      \
                                              key_pointer)                     \
                                              .private},                       \
        CCC_Flat_hash_map const *: &(                                          \
                 CCC_Flat_hash_map_entry){CCC_flat_hash_map_entry(             \
                                              (CCC_Flat_hash_map               \
                                                   *)(container_pointer),      \
                                              key_pointer)                     \
                                              .private},                       \
        CCC_Adaptive_map *: &(                                                 \
                 CCC_Adaptive_map_entry){CCC_adaptive_map_entry(               \
                                             (CCC_Adaptive_map                 \
                                                  *)(container_pointer),       \
                                             key_pointer)                      \
                                             .private},                        \
        CCC_Bounded_map *: &(                                                  \
                 CCC_Bounded_map_entry){CCC_bounded_map_entry(                 \
                                            (CCC_Bounded_map                   \
                                                 *)(container_pointer),        \
                                            key_pointer)                       \
                                            .private},                         \
        CCC_Bounded_map const *: &(CCC_Bounded_map_entry){                     \
            CCC_bounded_map_entry((CCC_Bounded_map *)(container_pointer),      \
                                  key_pointer)                                 \
                .private})

#define CCC_private_handle(container_pointer, key_pointer...)                  \
    _Generic((container_pointer),                                              \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_handle,             \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_handle,               \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_handle)(        \
        (container_pointer), key_pointer)

#define CCC_private_handle_r(container_pointer, key_pointer...)                \
    _Generic(                                                                  \
        (container_pointer),                                                   \
        CCC_Handle_adaptive_map *: &(                                          \
            CCC_Handle_adaptive_map_handle){CCC_handle_adaptive_map_handle(    \
                                                (CCC_Handle_adaptive_map       \
                                                     *)(container_pointer),    \
                                                key_pointer)                   \
                                                .private},                     \
        CCC_Handle_bounded_map *: &(                                           \
            CCC_Handle_bounded_map_handle){CCC_handle_bounded_map_handle(      \
                                               (CCC_Handle_bounded_map         \
                                                    *)(container_pointer),     \
                                               key_pointer)                    \
                                               .private},                      \
        CCC_Handle_bounded_map const *: &(CCC_Handle_bounded_map_handle){      \
            CCC_handle_bounded_map_handle(                                     \
                (CCC_Handle_bounded_map *)(container_pointer), key_pointer)    \
                .private})

#define CCC_private_and_modify(container_entry_pointer, mod_fn)                \
    _Generic((container_entry_pointer),                                        \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_and_modify,               \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_and_modify,                 \
        CCC_Handle_adaptive_map_handle *: CCC_handle_adaptive_map_and_modify,  \
        CCC_Bounded_map_entry *: CCC_bounded_map_and_modify,                   \
        CCC_Handle_bounded_map_handle *: CCC_handle_bounded_map_and_modify,    \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_and_modify,         \
        CCC_Handle_bounded_map_handle const                                    \
            *: CCC_handle_bounded_map_and_modify,                              \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_and_modify,           \
        CCC_Handle_adaptive_map_handle const                                   \
            *: CCC_handle_adaptive_map_and_modify,                             \
        CCC_Bounded_map_entry const *: CCC_bounded_map_and_modify)(            \
        (container_entry_pointer), (mod_fn))

#define CCC_private_and_modify_context(container_entry_pointer, mod_fn,        \
                                       context_data_pointer...)                \
    _Generic((container_entry_pointer),                                        \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_and_modify_context,       \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_and_modify_context,         \
        CCC_Handle_adaptive_map_handle                                         \
            *: CCC_handle_adaptive_map_and_modify_context,                     \
        CCC_Handle_bounded_map_handle                                          \
            *: CCC_handle_bounded_map_and_modify_context,                      \
        CCC_Bounded_map_entry *: CCC_bounded_map_and_modify_context,           \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_and_modify_context, \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_and_modify_context,   \
        CCC_Handle_bounded_map_handle const                                    \
            *: CCC_handle_bounded_map_and_modify_context,                      \
        CCC_Handle_adaptive_map_handle const                                   \
            *: CCC_handle_adaptive_map_and_modify_context,                     \
        CCC_Bounded_map_entry const *: CCC_bounded_map_and_modify_context)(    \
        (container_entry_pointer), (mod_fn), context_data_pointer)

#define CCC_private_insert_entry(container_entry_pointer,                      \
                                 key_val_container_handle_pointer...)          \
    _Generic((container_entry_pointer),                                        \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_insert_entry,             \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_insert_entry,               \
        CCC_Bounded_map_entry *: CCC_bounded_map_insert_entry,                 \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_insert_entry,       \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_insert_entry,         \
        CCC_Bounded_map_entry const *: CCC_bounded_map_insert_entry)(          \
        (container_entry_pointer), key_val_container_handle_pointer)

#define CCC_private_insert_handle(container_handle_pointer,                    \
                                  key_val_container_handle_pointer...)         \
    _Generic((container_handle_pointer),                                       \
        CCC_Handle_adaptive_map_handle                                         \
            *: CCC_handle_adaptive_map_insert_handle,                          \
        CCC_Handle_adaptive_map_handle const                                   \
            *: CCC_handle_adaptive_map_insert_handle,                          \
        CCC_Handle_bounded_map_handle *: CCC_handle_bounded_map_insert_handle, \
        CCC_Handle_bounded_map_handle const                                    \
            *: CCC_handle_bounded_map_insert_handle)(                          \
        (container_handle_pointer), key_val_container_handle_pointer)

#define CCC_private_or_insert(container_entry_pointer,                         \
                              key_val_container_handle_pointer...)             \
    _Generic((container_entry_pointer),                                        \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_or_insert,                \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_or_insert,                  \
        CCC_Handle_adaptive_map_handle *: CCC_handle_adaptive_map_or_insert,   \
        CCC_Bounded_map_entry *: CCC_bounded_map_or_insert,                    \
        CCC_Handle_bounded_map_handle *: CCC_handle_bounded_map_or_insert,     \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_or_insert,          \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_or_insert,            \
        CCC_Handle_bounded_map_handle const                                    \
            *: CCC_handle_bounded_map_or_insert,                               \
        CCC_Handle_adaptive_map_handle const                                   \
            *: CCC_handle_adaptive_map_or_insert,                              \
        CCC_Bounded_map_entry const *: CCC_bounded_map_or_insert)(             \
        (container_entry_pointer), key_val_container_handle_pointer)

#define CCC_private_unwrap(container_entry_pointer)                            \
    _Generic((container_entry_pointer),                                        \
        CCC_Entry *: CCC_entry_unwrap,                                         \
        CCC_Entry const *: CCC_entry_unwrap,                                   \
        CCC_Handle *: CCC_handle_unwrap,                                       \
        CCC_Handle const *: CCC_handle_unwrap,                                 \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_unwrap,                   \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_unwrap,             \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_unwrap,                     \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_unwrap,               \
        CCC_Handle_adaptive_map_handle *: CCC_handle_adaptive_map_unwrap,      \
        CCC_Handle_adaptive_map_handle const                                   \
            *: CCC_handle_adaptive_map_unwrap,                                 \
        CCC_Handle_bounded_map_handle *: CCC_handle_bounded_map_unwrap,        \
        CCC_Handle_bounded_map_handle const *: CCC_handle_bounded_map_unwrap,  \
        CCC_Bounded_map_entry *: CCC_bounded_map_unwrap,                       \
        CCC_Bounded_map_entry const *: CCC_bounded_map_unwrap)(                \
        (container_entry_pointer))

#define CCC_private_occupied(container_entry_pointer)                          \
    _Generic((container_entry_pointer),                                        \
        CCC_Entry *: CCC_entry_occupied,                                       \
        CCC_Entry const *: CCC_entry_occupied,                                 \
        CCC_Handle *: CCC_handle_occupied,                                     \
        CCC_Handle const *: CCC_handle_occupied,                               \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_occupied,                 \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_occupied,           \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_occupied,                   \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_occupied,             \
        CCC_Handle_adaptive_map_handle *: CCC_handle_adaptive_map_occupied,    \
        CCC_Handle_adaptive_map_handle const                                   \
            *: CCC_handle_adaptive_map_occupied,                               \
        CCC_Handle_bounded_map_handle *: CCC_handle_bounded_map_occupied,      \
        CCC_Handle_bounded_map_handle const                                    \
            *: CCC_handle_bounded_map_occupied,                                \
        CCC_Bounded_map_entry *: CCC_bounded_map_occupied,                     \
        CCC_Bounded_map_entry const *: CCC_bounded_map_occupied)(              \
        (container_entry_pointer))

#define CCC_private_insert_error(container_entry_pointer)                      \
    _Generic((container_entry_pointer),                                        \
        CCC_Entry *: CCC_entry_insert_error,                                   \
        CCC_Entry const *: CCC_entry_insert_error,                             \
        CCC_Handle *: CCC_handle_insert_error,                                 \
        CCC_Handle const *: CCC_handle_insert_error,                           \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_insert_error,             \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_insert_error,       \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_insert_error,               \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_insert_error,         \
        CCC_Handle_adaptive_map_handle                                         \
            *: CCC_handle_adaptive_map_insert_error,                           \
        CCC_Handle_adaptive_map_handle const                                   \
            *: CCC_handle_adaptive_map_insert_error,                           \
        CCC_Handle_bounded_map_handle *: CCC_handle_bounded_map_insert_error,  \
        CCC_Handle_bounded_map_handle const                                    \
            *: CCC_handle_bounded_map_insert_error,                            \
        CCC_Bounded_map_entry *: CCC_bounded_map_insert_error,                 \
        CCC_Bounded_map_entry const *: CCC_bounded_map_insert_error)(          \
        (container_entry_pointer))

/*======================    Misc Search Interface ===========================*/

#define CCC_private_get_key_val(container_pointer, key_pointer...)             \
    _Generic((container_pointer),                                              \
        CCC_Flat_hash_map *: CCC_flat_hash_map_get_key_val,                    \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_get_key_val,              \
        CCC_Adaptive_map *: CCC_adaptive_map_get_key_val,                      \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_get_key_val,        \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_get_key_val,          \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_get_key_val,    \
        CCC_Bounded_map *: CCC_bounded_map_get_key_val,                        \
        CCC_Bounded_map const *: CCC_bounded_map_get_key_val)(                 \
        (container_pointer), key_pointer)

#define CCC_private_contains(container_pointer, key_pointer...)                \
    _Generic((container_pointer),                                              \
        CCC_Flat_hash_map *: CCC_flat_hash_map_contains,                       \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_contains,                 \
        CCC_Adaptive_map *: CCC_adaptive_map_contains,                         \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_contains,           \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_contains,             \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_contains,       \
        CCC_Bounded_map *: CCC_bounded_map_contains,                           \
        CCC_Bounded_map const *: CCC_bounded_map_contains)(                    \
        (container_pointer), key_pointer)

/*================    Sequential Containers Interface   =====================*/

#define CCC_private_push(container_pointer, container_handle_pointer...)       \
    _Generic((container_pointer),                                              \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_push,               \
        CCC_Priority_queue *: CCC_priority_queue_push)(                        \
        (container_pointer), container_handle_pointer)

#define CCC_private_push_back(container_pointer, container_handle_pointer...)  \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_push_back,                                    \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_push_back,  \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_push_back,            \
        CCC_Buffer *: CCC_buffer_push_back)((container_pointer),               \
                                            container_handle_pointer)

#define CCC_private_push_front(container_pointer, container_handle_pointer...) \
    _Generic((container_pointer),                                              \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_push_front, \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_push_front,           \
        CCC_Singly_linked_list *: CCC_singly_linked_list_push_front)(          \
        (container_pointer), container_handle_pointer)

#define CCC_private_pop(container_pointer, ...)                                \
    _Generic((container_pointer),                                              \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_pop,                \
        CCC_Priority_queue *: CCC_priority_queue_pop)(                         \
        (container_pointer)__VA_OPT__(, __VA_ARGS__))

#define CCC_private_pop_front(container_pointer)                               \
    _Generic((container_pointer),                                              \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_pop_front,  \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_pop_front,            \
        CCC_Singly_linked_list *: CCC_singly_linked_list_pop_front)(           \
        (container_pointer))

#define CCC_private_pop_back(container_pointer)                                \
    _Generic((container_pointer),                                              \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_pop_back,   \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_pop_back,             \
        CCC_Buffer *: CCC_buffer_pop_back)((container_pointer))

#define CCC_private_front(container_pointer)                                   \
    _Generic((container_pointer),                                              \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_front,      \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_front,                \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_front,              \
        CCC_Priority_queue *: CCC_priority_queue_front,                        \
        CCC_Singly_linked_list *: CCC_singly_linked_list_front,                \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_front,                              \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_front,          \
        CCC_Flat_priority_queue const *: CCC_flat_priority_queue_front,        \
        CCC_Priority_queue const *: CCC_priority_queue_front,                  \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_front)(         \
        (container_pointer))

#define CCC_private_back(container_pointer)                                    \
    _Generic((container_pointer),                                              \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_back,       \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_back,                 \
        CCC_Buffer *: CCC_buffer_back,                                         \
        CCC_Flat_double_ended_queue const *: CCC_flat_double_ended_queue_back, \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_back,           \
        CCC_Buffer const *: CCC_buffer_back)((container_pointer))

/*================       Priority Queue Update Interface =====================*/

#define CCC_private_update(container_pointer, update_args...)                  \
    _Generic((container_pointer),                                              \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_update,             \
        CCC_Priority_queue *: CCC_priority_queue_update)((container_pointer),  \
                                                         update_args)

#define CCC_private_increase(container_pointer, increase_args...)              \
    _Generic((container_pointer),                                              \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_increase,           \
        CCC_Priority_queue *: CCC_priority_queue_increase)(                    \
        (container_pointer), increase_args)

#define CCC_private_decrease(container_pointer, decrease_args...)              \
    _Generic((container_pointer),                                              \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_decrease,           \
        CCC_Priority_queue *: CCC_priority_queue_decrease)(                    \
        (container_pointer), decrease_args)

#define CCC_private_extract(container_pointer, container_handle_pointer...)    \
    _Generic((container_pointer),                                              \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_extract,              \
        CCC_Singly_linked_list *: CCC_singly_linked_list_extract,              \
        CCC_Priority_queue *: CCC_priority_queue_extract)(                     \
        (container_pointer), container_handle_pointer)

#define CCC_private_erase(container_pointer, container_handle_pointer...)      \
    _Generic((container_pointer),                                              \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_erase)(             \
        (container_pointer), container_handle_pointer)

#define CCC_private_extract_range(container_pointer,                           \
                                  container_handle_begin_end_pointer...)       \
    _Generic((container_pointer),                                              \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_extract_range,        \
        CCC_Singly_linked_list *: CCC_singly_linked_list_extract_range)(       \
        (container_pointer), container_handle_begin_end_pointer)

/*===================       Iterators Interface ==============================*/

#define CCC_private_begin(container_pointer)                                   \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_begin,                                        \
        CCC_Flat_hash_map *: CCC_flat_hash_map_begin,                          \
        CCC_Adaptive_map *: CCC_adaptive_map_begin,                            \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_begin,              \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_begin,      \
        CCC_Singly_linked_list *: CCC_singly_linked_list_begin,                \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_begin,                \
        CCC_Bounded_map *: CCC_bounded_map_begin,                              \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_begin,                \
        CCC_Buffer const *: CCC_buffer_begin,                                  \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_begin,                    \
        CCC_Adaptive_map const *: CCC_adaptive_map_begin,                      \
        CCC_Handle_adaptive_map const *: CCC_handle_adaptive_map_begin,        \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_begin,                              \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_begin,          \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_begin,          \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_begin,          \
        CCC_Bounded_map const *: CCC_bounded_map_begin)((container_pointer))

#define CCC_private_reverse_begin(container_pointer)                           \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_reverse_begin,                                \
        CCC_Adaptive_map *: CCC_adaptive_map_reverse_begin,                    \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_reverse_begin,      \
        CCC_Flat_double_ended_queue                                            \
            *: CCC_flat_double_ended_queue_reverse_begin,                      \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_reverse_begin,        \
        CCC_Bounded_map *: CCC_bounded_map_reverse_begin,                      \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_reverse_begin,        \
        CCC_Buffer const *: CCC_buffer_reverse_begin,                          \
        CCC_Adaptive_map const *: CCC_adaptive_map_reverse_begin,              \
        CCC_Handle_adaptive_map const                                          \
            *: CCC_handle_adaptive_map_reverse_begin,                          \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_reverse_begin,                      \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_reverse_begin,  \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_reverse_begin,  \
        CCC_Bounded_map const *: CCC_bounded_map_reverse_begin)(               \
        (container_pointer))

#define CCC_private_next(container_pointer, void_iter_pointer)                 \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_next,                                         \
        CCC_Flat_hash_map *: CCC_flat_hash_map_next,                           \
        CCC_Adaptive_map *: CCC_adaptive_map_next,                             \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_next,               \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_next,       \
        CCC_Singly_linked_list *: CCC_singly_linked_list_next,                 \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_next,                 \
        CCC_Bounded_map *: CCC_bounded_map_next,                               \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_next,                 \
        CCC_Buffer const *: CCC_buffer_next,                                   \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_next,                     \
        CCC_Adaptive_map const *: CCC_adaptive_map_next,                       \
        CCC_Handle_adaptive_map const *: CCC_handle_adaptive_map_next,         \
        CCC_Flat_double_ended_queue const *: CCC_flat_double_ended_queue_next, \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_next,           \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_next,           \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_next,           \
        CCC_Bounded_map const *: CCC_bounded_map_next)((container_pointer),    \
                                                       (void_iter_pointer))

#define CCC_private_reverse_next(container_pointer, void_iter_pointer)         \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_reverse_next,                                 \
        CCC_Adaptive_map *: CCC_adaptive_map_reverse_next,                     \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_reverse_next,       \
        CCC_Flat_double_ended_queue                                            \
            *: CCC_flat_double_ended_queue_reverse_next,                       \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_reverse_next,         \
        CCC_Bounded_map *: CCC_bounded_map_reverse_next,                       \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_reverse_next,         \
        CCC_Buffer const *: CCC_buffer_reverse_next,                           \
        CCC_Adaptive_map const *: CCC_adaptive_map_reverse_next,               \
        CCC_Handle_adaptive_map const *: CCC_handle_adaptive_map_reverse_next, \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_reverse_next,                       \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_reverse_next,   \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_reverse_next,   \
        CCC_Bounded_map const *: CCC_bounded_map_reverse_next)(                \
        (container_pointer), (void_iter_pointer))

#define CCC_private_end(container_pointer)                                     \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_end,                                          \
        CCC_Flat_hash_map *: CCC_flat_hash_map_end,                            \
        CCC_Adaptive_map *: CCC_adaptive_map_end,                              \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_end,                \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_end,        \
        CCC_Singly_linked_list *: CCC_singly_linked_list_end,                  \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_end,                  \
        CCC_Bounded_map *: CCC_bounded_map_end,                                \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_end,                  \
        CCC_Buffer const *: CCC_buffer_end,                                    \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_end,                      \
        CCC_Adaptive_map const *: CCC_adaptive_map_end,                        \
        CCC_Handle_adaptive_map const *: CCC_handle_adaptive_map_end,          \
        CCC_Flat_double_ended_queue const *: CCC_flat_double_ended_queue_end,  \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_end,            \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_end,            \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_end,            \
        CCC_Bounded_map const *: CCC_bounded_map_end)((container_pointer))

#define CCC_private_reverse_end(container_pointer)                             \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_reverse_end,                                  \
        CCC_Adaptive_map *: CCC_adaptive_map_reverse_end,                      \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_reverse_end,        \
        CCC_Flat_double_ended_queue                                            \
            *: CCC_flat_double_ended_queue_reverse_end,                        \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_reverse_end,          \
        CCC_Bounded_map *: CCC_bounded_map_reverse_end,                        \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_reverse_end,          \
        CCC_Buffer const *: CCC_buffer_reverse_end,                            \
        CCC_Adaptive_map const *: CCC_adaptive_map_reverse_end,                \
        CCC_Handle_adaptive_map const *: CCC_handle_adaptive_map_reverse_end,  \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_reverse_end,                        \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_reverse_end,    \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_reverse_end,    \
        CCC_Bounded_map const *: CCC_bounded_map_reverse_end)(                 \
        (container_pointer))

#define CCC_private_equal_range(container_pointer,                             \
                                begin_and_end_key_pointer...)                  \
    _Generic((container_pointer),                                              \
        CCC_Adaptive_map *: CCC_adaptive_map_equal_range,                      \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_equal_range,        \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_equal_range,          \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_equal_range,    \
        CCC_Bounded_map *: CCC_bounded_map_equal_range,                        \
        CCC_Bounded_map const *: CCC_bounded_map_equal_range)(                 \
        (container_pointer), begin_and_end_key_pointer)

#define CCC_private_equal_range_r(container_pointer,                           \
                                  begin_and_end_key_pointer...)                \
    &(CCC_Range)                                                               \
    {                                                                          \
        CCC_private_equal_range(container_pointer, begin_and_end_key_pointer)  \
            .private                                                           \
    }

#define CCC_private_equal_range_reverse(                                       \
    container_pointer, reverse_begin_and_reverse_end_key_pointer...)           \
    _Generic((container_pointer),                                              \
        CCC_Adaptive_map *: CCC_adaptive_map_equal_range_reverse,              \
        CCC_Handle_adaptive_map                                                \
            *: CCC_handle_adaptive_map_equal_range_reverse,                    \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_equal_range_reverse,  \
        CCC_Handle_bounded_map const                                           \
            *: CCC_handle_bounded_map_equal_range_reverse,                     \
        CCC_Bounded_map *: CCC_bounded_map_equal_range_reverse,                \
        CCC_Bounded_map const *: CCC_bounded_map_equal_range_reverse)(         \
        (container_pointer), reverse_begin_and_reverse_end_key_pointer)

#define CCC_private_equal_range_reverse_r(                                     \
    container_pointer, reverse_begin_and_reverse_end_key_pointer...)           \
    &(CCC_Range_reverse)                                                       \
    {                                                                          \
        CCC_private_equal_range_reverse(                                       \
            container_pointer, reverse_begin_and_reverse_end_key_pointer)      \
            .private                                                           \
    }

#define CCC_private_splice(container_pointer, splice_args...)                  \
    _Generic((container_pointer),                                              \
        CCC_Singly_linked_list *: CCC_singly_linked_list_splice,               \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_splice,         \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_splice,               \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_splice)(        \
        (container_pointer), splice_args)

#define CCC_private_splice_range(container_pointer, splice_range_args...)      \
    _Generic((container_pointer),                                              \
        CCC_Singly_linked_list *: CCC_singly_linked_list_splice_range,         \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_splice_range,   \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_splice_range,         \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_splice_range)(  \
        (container_pointer), splice_range_args)

/*===================         Memory Management       =======================*/

#define CCC_private_copy(dst_container_pointer, src_container_pointer,         \
                         allocate_pointer)                                     \
    _Generic((dst_container_pointer),                                          \
        CCC_Bitset *: CCC_bitset_copy,                                         \
        CCC_Flat_hash_map *: CCC_flat_hash_map_copy,                           \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_copy,               \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_copy,               \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_copy,       \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_copy)(                \
        (dst_container_pointer), (src_container_pointer), (allocate_pointer))

#define CCC_private_reserve(container_pointer, n_to_add, allocate_pointer)     \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_reserve,                                      \
        CCC_Buffer *: CCC_buffer_reserve,                                      \
        CCC_Flat_hash_map *: CCC_flat_hash_map_reserve,                        \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_reserve,            \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_reserve,            \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_reserve,    \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_reserve)(             \
        (container_pointer), (n_to_add), (allocate_pointer))

#define CCC_private_clear(container_pointer, ...)                              \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_clear,                                        \
        CCC_Buffer *: CCC_buffer_clear,                                        \
        CCC_Flat_hash_map *: CCC_flat_hash_map_clear,                          \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_clear,              \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_clear,              \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_clear,      \
        CCC_Singly_linked_list *: CCC_singly_linked_list_clear,                \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_clear,                \
        CCC_Adaptive_map *: CCC_adaptive_map_clear,                            \
        CCC_Priority_queue *: CCC_priority_queue_clear,                        \
        CCC_Bounded_map *: CCC_bounded_map_clear,                              \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_clear)(               \
        (container_pointer)__VA_OPT__(, __VA_ARGS__))

#define CCC_private_clear_and_free(container_pointer, ...)                     \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_clear_and_free,                               \
        CCC_Buffer *: CCC_buffer_clear_and_free,                               \
        CCC_Flat_hash_map *: CCC_flat_hash_map_clear_and_free,                 \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_clear_and_free,     \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_clear_and_free,     \
        CCC_Flat_double_ended_queue                                            \
            *: CCC_flat_double_ended_queue_clear_and_free,                     \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_clear_and_free)(      \
        (container_pointer)__VA_OPT__(, __VA_ARGS__))

#define CCC_private_clear_and_free_reserve(container_pointer,                  \
                                           destructor_and_free_args...)        \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_clear_and_free_reserve,                       \
        CCC_Buffer *: CCC_buffer_clear_and_free_reserve,                       \
        CCC_Flat_hash_map *: CCC_flat_hash_map_clear_and_free_reserve,         \
        CCC_Handle_adaptive_map                                                \
            *: CCC_handle_adaptive_map_clear_and_free_reserve,                 \
        CCC_Flat_priority_queue                                                \
            *: CCC_flat_priority_queue_clear_and_free_reserve,                 \
        CCC_Flat_double_ended_queue                                            \
            *: CCC_flat_double_ended_queue_clear_and_free_reserve,             \
        CCC_Handle_bounded_map                                                 \
            *: CCC_handle_bounded_map_clear_and_free_reserve)(                 \
        (container_pointer), destructor_and_free_args)

/*===================    Standard Getters Interface   =======================*/

#define CCC_private_count(container_pointer)                                   \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_count,                                        \
        CCC_Buffer *: CCC_buffer_count,                                        \
        CCC_Flat_hash_map *: CCC_flat_hash_map_count,                          \
        CCC_Adaptive_map *: CCC_adaptive_map_count,                            \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_count,              \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_count,              \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_count,      \
        CCC_Priority_queue *: CCC_priority_queue_count,                        \
        CCC_Singly_linked_list *: CCC_singly_linked_list_count,                \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_count,                \
        CCC_Bounded_map *: CCC_bounded_map_count,                              \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_count,                \
        CCC_Buffer const *: CCC_buffer_count,                                  \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_count,                    \
        CCC_Adaptive_map const *: CCC_adaptive_map_count,                      \
        CCC_Handle_adaptive_map const *: CCC_handle_adaptive_map_count,        \
        CCC_Flat_priority_queue const *: CCC_flat_priority_queue_count,        \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_count,                              \
        CCC_Priority_queue const *: CCC_priority_queue_count,                  \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_count,          \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_count,          \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_count,          \
        CCC_Bounded_map const *: CCC_bounded_map_count)((container_pointer))

#define CCC_private_capacity(container_pointer)                                \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_capacity,                                     \
        CCC_Buffer *: CCC_buffer_capacity,                                     \
        CCC_Flat_hash_map *: CCC_flat_hash_map_capacity,                       \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_capacity,           \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_capacity,           \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_capacity,   \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_capacity,             \
        CCC_Bitset const *: CCC_bitset_capacity,                               \
        CCC_Buffer const *: CCC_buffer_capacity,                               \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_capacity,                 \
        CCC_Handle_adaptive_map const *: CCC_handle_adaptive_map_capacity,     \
        CCC_Flat_priority_queue const *: CCC_flat_priority_queue_capacity,     \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_capacity,                           \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_capacity)(      \
        (container_pointer))

#define CCC_private_is_empty(container_pointer)                                \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_is_empty,                                     \
        CCC_Flat_hash_map *: CCC_flat_hash_map_is_empty,                       \
        CCC_Adaptive_map *: CCC_adaptive_map_is_empty,                         \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_is_empty,           \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_is_empty,           \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_is_empty,   \
        CCC_Priority_queue *: CCC_priority_queue_is_empty,                     \
        CCC_Singly_linked_list *: CCC_singly_linked_list_is_empty,             \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_is_empty,             \
        CCC_Bounded_map *: CCC_bounded_map_is_empty,                           \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_is_empty,             \
        CCC_Buffer const *: CCC_buffer_is_empty,                               \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_is_empty,                 \
        CCC_Adaptive_map const *: CCC_adaptive_map_is_empty,                   \
        CCC_Handle_adaptive_map const *: CCC_handle_adaptive_map_is_empty,     \
        CCC_Flat_priority_queue const *: CCC_flat_priority_queue_is_empty,     \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_is_empty,                           \
        CCC_Priority_queue const *: CCC_priority_queue_is_empty,               \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_is_empty,       \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_is_empty,       \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_is_empty,       \
        CCC_Bounded_map const *: CCC_bounded_map_is_empty)(                    \
        (container_pointer))

#define CCC_private_validate(container_pointer)                                \
    _Generic((container_pointer),                                              \
        CCC_Flat_hash_map *: CCC_flat_hash_map_validate,                       \
        CCC_Adaptive_map *: CCC_adaptive_map_validate,                         \
        CCC_Handle_adaptive_map *: CCC_handle_adaptive_map_validate,           \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_validate,           \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_validate,   \
        CCC_Priority_queue *: CCC_priority_queue_validate,                     \
        CCC_Singly_linked_list *: CCC_singly_linked_list_validate,             \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_validate,             \
        CCC_Bounded_map *: CCC_bounded_map_validate,                           \
        CCC_Handle_bounded_map *: CCC_handle_bounded_map_validate,             \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_validate,                 \
        CCC_Adaptive_map const *: CCC_adaptive_map_validate,                   \
        CCC_Handle_adaptive_map const *: CCC_handle_adaptive_map_validate,     \
        CCC_Flat_priority_queue const *: CCC_flat_priority_queue_validate,     \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_validate,                           \
        CCC_Priority_queue const *: CCC_priority_queue_validate,               \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_validate,       \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_validate,       \
        CCC_Handle_bounded_map const *: CCC_handle_bounded_map_validate,       \
        CCC_Bounded_map const *: CCC_bounded_map_validate)(                    \
        (container_pointer))

#endif /* CCC_PRIVATE_TRAITS_H */
