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
#include "../array_adaptive_map.h"
#include "../array_tree_map.h"
#include "../bitset.h"
#include "../buffer.h"
#include "../doubly_linked_list.h"
#include "../flat_double_ended_queue.h"
#include "../flat_hash_map.h"
#include "../flat_priority_queue.h"
#include "../priority_queue.h"
#include "../singly_linked_list.h"
#include "../tree_map.h"
#include "../types.h"
/* NOLINTEND */

/*====================     Entry/Handle Interface   =========================*/

#define CCC_private_swap_entry(container_pointer, swap_args...)                \
    _Generic((container_pointer),                                              \
        CCC_Flat_hash_map *: CCC_flat_hash_map_swap_entry,                     \
        CCC_Adaptive_map *: CCC_adaptive_map_swap_entry,                       \
        CCC_Tree_map *: CCC_tree_map_swap_entry)((container_pointer),          \
                                                 swap_args)

#define CCC_private_swap_entry_wrap(container_pointer,                         \
                                    key_val_container_array_pointer...)        \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_private_swap_entry(container_pointer,                              \
                               key_val_container_array_pointer)                \
            .private                                                           \
    }

#define CCC_private_swap_handle(container_pointer, swap_args...)               \
    _Generic((container_pointer),                                              \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_swap_handle,          \
        CCC_Array_tree_map *: CCC_array_tree_map_swap_handle)(                 \
        (container_pointer), swap_args)

#define CCC_private_swap_handle_wrap(container_pointer,                        \
                                     key_val_container_array_pointer...)       \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_private_swap_handle(container_pointer,                             \
                                key_val_container_array_pointer)               \
            .private                                                           \
    }

#define CCC_private_try_insert(container_pointer, try_insert_args...)          \
    _Generic((container_pointer),                                              \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_try_insert,           \
        CCC_Array_tree_map *: CCC_array_tree_map_try_insert,                   \
        CCC_Flat_hash_map *: CCC_flat_hash_map_try_insert,                     \
        CCC_Adaptive_map *: CCC_adaptive_map_try_insert,                       \
        CCC_Tree_map *: CCC_tree_map_try_insert)((container_pointer),          \
                                                 try_insert_args)

#define CCC_private_try_insert_wrap(container_pointer, try_insert_args...)     \
    _Generic((container_pointer),                                              \
        CCC_Array_adaptive_map *: &(                                           \
                 CCC_Handle){CCC_array_adaptive_map_try_insert(                \
                                 (CCC_Array_adaptive_map *)container_pointer,  \
                                 try_insert_args)                              \
                                 .private},                                    \
        CCC_Array_tree_map *: &(                                               \
                 CCC_Handle){CCC_array_tree_map_try_insert(                    \
                                 (CCC_Array_tree_map *)container_pointer,      \
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
        CCC_Tree_map *: &(CCC_Entry){                                          \
            CCC_tree_map_try_insert((CCC_Tree_map *)container_pointer,         \
                                    (CCC_Tree_map_node *)try_insert_args)      \
                .private})

#define CCC_private_insert_or_assign(container_pointer,                        \
                                     insert_or_assign_args...)                 \
    _Generic((container_pointer),                                              \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_insert_or_assign,     \
        CCC_Array_tree_map *: CCC_array_tree_map_insert_or_assign,             \
        CCC_Flat_hash_map *: CCC_flat_hash_map_insert_or_assign,               \
        CCC_Adaptive_map *: CCC_adaptive_map_insert_or_assign,                 \
        CCC_Tree_map *: CCC_tree_map_insert_or_assign)((container_pointer),    \
                                                       insert_or_assign_args)

#define CCC_private_insert_or_assign_wrap(container_pointer,                   \
                                          insert_or_assign_args...)            \
    _Generic((container_pointer),                                              \
        CCC_Array_adaptive_map *: &(                                           \
                 CCC_Handle){CCC_array_adaptive_map_insert_or_assign(          \
                                 (CCC_Array_adaptive_map *)container_pointer,  \
                                 insert_or_assign_args)                        \
                                 .private},                                    \
        CCC_Array_tree_map *: &(                                               \
                 CCC_Handle){CCC_array_tree_map_insert_or_assign(              \
                                 (CCC_Array_tree_map *)container_pointer,      \
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
        CCC_Tree_map *: &(CCC_Entry){                                          \
            CCC_tree_map_insert_or_assign(                                     \
                (CCC_Tree_map *)container_pointer,                             \
                (CCC_Tree_map_node *)insert_or_assign_args)                    \
                .private})

#define CCC_private_remove(container_pointer,                                  \
                           key_val_container_array_pointer...)                 \
    _Generic((container_pointer),                                              \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_remove,               \
        CCC_Array_tree_map *: CCC_array_tree_map_remove,                       \
        CCC_Flat_hash_map *: CCC_flat_hash_map_remove,                         \
        CCC_Adaptive_map *: CCC_adaptive_map_remove,                           \
        CCC_Tree_map *: CCC_tree_map_remove)((container_pointer),              \
                                             key_val_container_array_pointer)

#define CCC_private_remove_wrap(container_pointer,                             \
                                key_val_container_array_pointer...)            \
    _Generic((container_pointer),                                              \
        CCC_Array_adaptive_map *: &(                                           \
                 CCC_Handle){CCC_array_adaptive_map_remove(                    \
                                 (CCC_Array_adaptive_map *)container_pointer,  \
                                 key_val_container_array_pointer)              \
                                 .private},                                    \
        CCC_Array_tree_map *: &(                                               \
                 CCC_Handle){CCC_array_tree_map_remove(                        \
                                 (CCC_Array_tree_map *)container_pointer,      \
                                 key_val_container_array_pointer)              \
                                 .private},                                    \
        CCC_Flat_hash_map *: &(                                                \
                 CCC_Entry){CCC_flat_hash_map_remove(                          \
                                (CCC_Flat_hash_map *)container_pointer,        \
                                key_val_container_array_pointer)               \
                                .private},                                     \
        CCC_Adaptive_map *: &(                                                 \
                 CCC_Entry){CCC_adaptive_map_remove(                           \
                                (CCC_Adaptive_map *)container_pointer,         \
                                (CCC_Adaptive_map_node *)                      \
                                    key_val_container_array_pointer)           \
                                .private},                                     \
        CCC_Tree_map *: &(CCC_Entry){                                          \
            CCC_tree_map_remove(                                               \
                (CCC_Tree_map *)container_pointer,                             \
                (CCC_Tree_map_node *)key_val_container_array_pointer)          \
                .private})

#define CCC_private_remove_entry(container_entry_pointer)                      \
    _Generic((container_entry_pointer),                                        \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_remove_entry,             \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_remove_entry,               \
        CCC_Tree_map_entry *: CCC_tree_map_remove_entry,                       \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_remove_entry,       \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_remove_entry,         \
        CCC_Tree_map_entry const *: CCC_tree_map_remove_entry)(                \
        (container_entry_pointer))

#define CCC_private_remove_entry_wrap(container_entry_pointer)                 \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_private_remove_entry(container_entry_pointer).private              \
    }

#define CCC_private_remove_handle(container_array_pointer)                     \
    _Generic((container_array_pointer),                                        \
        CCC_Array_adaptive_map_handle *: CCC_array_adaptive_map_remove_handle, \
        CCC_Array_adaptive_map_handle const                                    \
            *: CCC_array_adaptive_map_remove_handle,                           \
        CCC_Array_tree_map_handle *: CCC_array_tree_map_remove_handle,         \
        CCC_Array_tree_map_handle const *: CCC_array_tree_map_remove_handle)(  \
        (container_array_pointer))

#define CCC_private_remove_handle_wrap(container_array_pointer)                \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_private_remove_handle(container_array_pointer).private             \
    }

#define CCC_private_entry(container_pointer, key_pointer...)                   \
    _Generic((container_pointer),                                              \
        CCC_Flat_hash_map *: CCC_flat_hash_map_entry,                          \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_entry,                    \
        CCC_Adaptive_map *: CCC_adaptive_map_entry,                            \
        CCC_Tree_map *: CCC_tree_map_entry,                                    \
        CCC_Tree_map const *: CCC_tree_map_entry)((container_pointer),         \
                                                  key_pointer)

#define CCC_private_entry_wrap(container_pointer, key_pointer...)              \
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
        CCC_Tree_map *: &(                                                     \
                 CCC_Tree_map_entry){CCC_tree_map_entry(                       \
                                         (CCC_Tree_map *)(container_pointer),  \
                                         key_pointer)                          \
                                         .private},                            \
        CCC_Tree_map const *: &(CCC_Tree_map_entry){                           \
            CCC_tree_map_entry((CCC_Tree_map *)(container_pointer),            \
                               key_pointer)                                    \
                .private})

#define CCC_private_handle(container_pointer, key_pointer...)                  \
    _Generic((container_pointer),                                              \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_handle,               \
        CCC_Array_tree_map *: CCC_array_tree_map_handle,                       \
        CCC_Array_tree_map const *: CCC_array_tree_map_handle)(                \
        (container_pointer), key_pointer)

#define CCC_private_handle_wrap(container_pointer, key_pointer...)             \
    _Generic(                                                                  \
        (container_pointer),                                                   \
        CCC_Array_adaptive_map *: &(                                           \
            CCC_Array_adaptive_map_handle){CCC_array_adaptive_map_handle(      \
                                               (CCC_Array_adaptive_map         \
                                                    *)(container_pointer),     \
                                               key_pointer)                    \
                                               .private},                      \
        CCC_Array_tree_map *: &(                                               \
            CCC_Array_tree_map_handle){CCC_array_tree_map_handle(              \
                                           (CCC_Array_tree_map                 \
                                                *)(container_pointer),         \
                                           key_pointer)                        \
                                           .private},                          \
        CCC_Array_tree_map const *: &(CCC_Array_tree_map_handle){              \
            CCC_array_tree_map_handle(                                         \
                (CCC_Array_tree_map *)(container_pointer), key_pointer)        \
                .private})

#define CCC_private_and_modify(container_entry_pointer, mod_fn)                \
    _Generic((container_entry_pointer),                                        \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_and_modify,               \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_and_modify,                 \
        CCC_Array_adaptive_map_handle *: CCC_array_adaptive_map_and_modify,    \
        CCC_Tree_map_entry *: CCC_tree_map_and_modify,                         \
        CCC_Array_tree_map_handle *: CCC_array_tree_map_and_modify,            \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_and_modify,         \
        CCC_Array_tree_map_handle const *: CCC_array_tree_map_and_modify,      \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_and_modify,           \
        CCC_Array_adaptive_map_handle const                                    \
            *: CCC_array_adaptive_map_and_modify,                              \
        CCC_Tree_map_entry const *: CCC_tree_map_and_modify)(                  \
        (container_entry_pointer), (mod_fn))

#define CCC_private_and_modify_context(container_entry_pointer, mod_fn,        \
                                       context_data_pointer...)                \
    _Generic((container_entry_pointer),                                        \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_and_modify_context,       \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_and_modify_context,         \
        CCC_Array_adaptive_map_handle                                          \
            *: CCC_array_adaptive_map_and_modify_context,                      \
        CCC_Array_tree_map_handle *: CCC_array_tree_map_and_modify_context,    \
        CCC_Tree_map_entry *: CCC_tree_map_and_modify_context,                 \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_and_modify_context, \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_and_modify_context,   \
        CCC_Array_tree_map_handle const                                        \
            *: CCC_array_tree_map_and_modify_context,                          \
        CCC_Array_adaptive_map_handle const                                    \
            *: CCC_array_adaptive_map_and_modify_context,                      \
        CCC_Tree_map_entry const *: CCC_tree_map_and_modify_context)(          \
        (container_entry_pointer), (mod_fn), context_data_pointer)

#define CCC_private_insert_entry(container_entry_pointer,                      \
                                 key_val_container_array_pointer...)           \
    _Generic((container_entry_pointer),                                        \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_insert_entry,             \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_insert_entry,               \
        CCC_Tree_map_entry *: CCC_tree_map_insert_entry,                       \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_insert_entry,       \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_insert_entry,         \
        CCC_Tree_map_entry const *: CCC_tree_map_insert_entry)(                \
        (container_entry_pointer), key_val_container_array_pointer)

#define CCC_private_insert_handle(container_array_pointer,                     \
                                  key_val_container_array_pointer...)          \
    _Generic((container_array_pointer),                                        \
        CCC_Array_adaptive_map_handle *: CCC_array_adaptive_map_insert_handle, \
        CCC_Array_adaptive_map_handle const                                    \
            *: CCC_array_adaptive_map_insert_handle,                           \
        CCC_Array_tree_map_handle *: CCC_array_tree_map_insert_handle,         \
        CCC_Array_tree_map_handle const *: CCC_array_tree_map_insert_handle)(  \
        (container_array_pointer), key_val_container_array_pointer)

#define CCC_private_or_insert(container_entry_pointer,                         \
                              key_val_container_array_pointer...)              \
    _Generic((container_entry_pointer),                                        \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_or_insert,                \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_or_insert,                  \
        CCC_Array_adaptive_map_handle *: CCC_array_adaptive_map_or_insert,     \
        CCC_Tree_map_entry *: CCC_tree_map_or_insert,                          \
        CCC_Array_tree_map_handle *: CCC_array_tree_map_or_insert,             \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_or_insert,          \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_or_insert,            \
        CCC_Array_tree_map_handle const *: CCC_array_tree_map_or_insert,       \
        CCC_Array_adaptive_map_handle const                                    \
            *: CCC_array_adaptive_map_or_insert,                               \
        CCC_Tree_map_entry const *: CCC_tree_map_or_insert)(                   \
        (container_entry_pointer), key_val_container_array_pointer)

#define CCC_private_unwrap(container_entry_pointer)                            \
    _Generic((container_entry_pointer),                                        \
        CCC_Entry *: CCC_entry_unwrap,                                         \
        CCC_Entry const *: CCC_entry_unwrap,                                   \
        CCC_Handle *: CCC_array_unwrap,                                        \
        CCC_Handle const *: CCC_array_unwrap,                                  \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_unwrap,                   \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_unwrap,             \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_unwrap,                     \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_unwrap,               \
        CCC_Array_adaptive_map_handle *: CCC_array_adaptive_map_unwrap,        \
        CCC_Array_adaptive_map_handle const *: CCC_array_adaptive_map_unwrap,  \
        CCC_Array_tree_map_handle *: CCC_array_tree_map_unwrap,                \
        CCC_Array_tree_map_handle const *: CCC_array_tree_map_unwrap,          \
        CCC_Tree_map_entry *: CCC_tree_map_unwrap,                             \
        CCC_Tree_map_entry const *: CCC_tree_map_unwrap)(                      \
        (container_entry_pointer))

#define CCC_private_occupied(container_entry_pointer)                          \
    _Generic((container_entry_pointer),                                        \
        CCC_Entry *: CCC_entry_occupied,                                       \
        CCC_Entry const *: CCC_entry_occupied,                                 \
        CCC_Handle *: CCC_array_occupied,                                      \
        CCC_Handle const *: CCC_array_occupied,                                \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_occupied,                 \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_occupied,           \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_occupied,                   \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_occupied,             \
        CCC_Array_adaptive_map_handle *: CCC_array_adaptive_map_occupied,      \
        CCC_Array_adaptive_map_handle const                                    \
            *: CCC_array_adaptive_map_occupied,                                \
        CCC_Array_tree_map_handle *: CCC_array_tree_map_occupied,              \
        CCC_Array_tree_map_handle const *: CCC_array_tree_map_occupied,        \
        CCC_Tree_map_entry *: CCC_tree_map_occupied,                           \
        CCC_Tree_map_entry const *: CCC_tree_map_occupied)(                    \
        (container_entry_pointer))

#define CCC_private_insert_error(container_entry_pointer)                      \
    _Generic((container_entry_pointer),                                        \
        CCC_Entry *: CCC_entry_insert_error,                                   \
        CCC_Entry const *: CCC_entry_insert_error,                             \
        CCC_Handle *: CCC_array_insert_error,                                  \
        CCC_Handle const *: CCC_array_insert_error,                            \
        CCC_Flat_hash_map_entry *: CCC_flat_hash_map_insert_error,             \
        CCC_Flat_hash_map_entry const *: CCC_flat_hash_map_insert_error,       \
        CCC_Adaptive_map_entry *: CCC_adaptive_map_insert_error,               \
        CCC_Adaptive_map_entry const *: CCC_adaptive_map_insert_error,         \
        CCC_Array_adaptive_map_handle *: CCC_array_adaptive_map_insert_error,  \
        CCC_Array_adaptive_map_handle const                                    \
            *: CCC_array_adaptive_map_insert_error,                            \
        CCC_Array_tree_map_handle *: CCC_array_tree_map_insert_error,          \
        CCC_Array_tree_map_handle const *: CCC_array_tree_map_insert_error,    \
        CCC_Tree_map_entry *: CCC_tree_map_insert_error,                       \
        CCC_Tree_map_entry const *: CCC_tree_map_insert_error)(                \
        (container_entry_pointer))

/*======================    Misc Search Interface ===========================*/

#define CCC_private_get_key_value(container_pointer, key_pointer...)           \
    _Generic((container_pointer),                                              \
        CCC_Flat_hash_map *: CCC_flat_hash_map_get_key_value,                  \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_get_key_value,            \
        CCC_Adaptive_map *: CCC_adaptive_map_get_key_value,                    \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_get_key_value,        \
        CCC_Array_tree_map *: CCC_array_tree_map_get_key_value,                \
        CCC_Array_tree_map const *: CCC_array_tree_map_get_key_value,          \
        CCC_Tree_map *: CCC_tree_map_get_key_value,                            \
        CCC_Tree_map const *: CCC_tree_map_get_key_value)((container_pointer), \
                                                          key_pointer)

#define CCC_private_contains(container_pointer, key_pointer...)                \
    _Generic((container_pointer),                                              \
        CCC_Flat_hash_map *: CCC_flat_hash_map_contains,                       \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_contains,                 \
        CCC_Adaptive_map *: CCC_adaptive_map_contains,                         \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_contains,             \
        CCC_Array_tree_map *: CCC_array_tree_map_contains,                     \
        CCC_Array_tree_map const *: CCC_array_tree_map_contains,               \
        CCC_Tree_map *: CCC_tree_map_contains,                                 \
        CCC_Tree_map const *: CCC_tree_map_contains)((container_pointer),      \
                                                     key_pointer)

/*================    Sequential Containers Interface   =====================*/

#define CCC_private_push(container_pointer, container_array_pointer...)        \
    _Generic((container_pointer),                                              \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_push,               \
        CCC_Priority_queue *: CCC_priority_queue_push)(                        \
        (container_pointer), container_array_pointer)

#define CCC_private_push_back(container_pointer, container_array_pointer...)   \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_push_back,                                    \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_push_back,  \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_push_back,            \
        CCC_Buffer *: CCC_buffer_push_back)((container_pointer),               \
                                            container_array_pointer)

#define CCC_private_push_front(container_pointer, container_array_pointer...)  \
    _Generic((container_pointer),                                              \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_push_front, \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_push_front,           \
        CCC_Singly_linked_list *: CCC_singly_linked_list_push_front)(          \
        (container_pointer), container_array_pointer)

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
        CCC_Bitset *: CCC_bitset_pop_back,                                     \
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

#define CCC_private_extract(container_pointer, container_array_pointer...)     \
    _Generic((container_pointer),                                              \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_extract,              \
        CCC_Singly_linked_list *: CCC_singly_linked_list_extract,              \
        CCC_Priority_queue *: CCC_priority_queue_extract)(                     \
        (container_pointer), container_array_pointer)

#define CCC_private_erase(container_pointer, container_array_pointer...)       \
    _Generic((container_pointer),                                              \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_erase)(             \
        (container_pointer), container_array_pointer)

#define CCC_private_extract_range(container_pointer,                           \
                                  container_array_begin_end_pointer...)        \
    _Generic((container_pointer),                                              \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_extract_range,        \
        CCC_Singly_linked_list *: CCC_singly_linked_list_extract_range)(       \
        (container_pointer), container_array_begin_end_pointer)

/*===================       Iterators Interface ==============================*/

#define CCC_private_begin(container_pointer)                                   \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_begin,                                        \
        CCC_Flat_hash_map *: CCC_flat_hash_map_begin,                          \
        CCC_Adaptive_map *: CCC_adaptive_map_begin,                            \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_begin,                \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_begin,      \
        CCC_Singly_linked_list *: CCC_singly_linked_list_begin,                \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_begin,                \
        CCC_Tree_map *: CCC_tree_map_begin,                                    \
        CCC_Array_tree_map *: CCC_array_tree_map_begin,                        \
        CCC_Buffer const *: CCC_buffer_begin,                                  \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_begin,                    \
        CCC_Adaptive_map const *: CCC_adaptive_map_begin,                      \
        CCC_Array_adaptive_map const *: CCC_array_adaptive_map_begin,          \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_begin,                              \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_begin,          \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_begin,          \
        CCC_Array_tree_map const *: CCC_array_tree_map_begin,                  \
        CCC_Tree_map const *: CCC_tree_map_begin)((container_pointer))

#define CCC_private_reverse_begin(container_pointer)                           \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_reverse_begin,                                \
        CCC_Adaptive_map *: CCC_adaptive_map_reverse_begin,                    \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_reverse_begin,        \
        CCC_Flat_double_ended_queue                                            \
            *: CCC_flat_double_ended_queue_reverse_begin,                      \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_reverse_begin,        \
        CCC_Tree_map *: CCC_tree_map_reverse_begin,                            \
        CCC_Array_tree_map *: CCC_array_tree_map_reverse_begin,                \
        CCC_Buffer const *: CCC_buffer_reverse_begin,                          \
        CCC_Adaptive_map const *: CCC_adaptive_map_reverse_begin,              \
        CCC_Array_adaptive_map const *: CCC_array_adaptive_map_reverse_begin,  \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_reverse_begin,                      \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_reverse_begin,  \
        CCC_Array_tree_map const *: CCC_array_tree_map_reverse_begin,          \
        CCC_Tree_map const *: CCC_tree_map_reverse_begin)((container_pointer))

#define CCC_private_next(container_pointer, void_iterator_pointer)             \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_next,                                         \
        CCC_Flat_hash_map *: CCC_flat_hash_map_next,                           \
        CCC_Adaptive_map *: CCC_adaptive_map_next,                             \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_next,                 \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_next,       \
        CCC_Singly_linked_list *: CCC_singly_linked_list_next,                 \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_next,                 \
        CCC_Tree_map *: CCC_tree_map_next,                                     \
        CCC_Array_tree_map *: CCC_array_tree_map_next,                         \
        CCC_Buffer const *: CCC_buffer_next,                                   \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_next,                     \
        CCC_Adaptive_map const *: CCC_adaptive_map_next,                       \
        CCC_Array_adaptive_map const *: CCC_array_adaptive_map_next,           \
        CCC_Flat_double_ended_queue const *: CCC_flat_double_ended_queue_next, \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_next,           \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_next,           \
        CCC_Array_tree_map const *: CCC_array_tree_map_next,                   \
        CCC_Tree_map const *: CCC_tree_map_next)((container_pointer),          \
                                                 (void_iterator_pointer))

#define CCC_private_reverse_next(container_pointer, void_iterator_pointer)     \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_reverse_next,                                 \
        CCC_Adaptive_map *: CCC_adaptive_map_reverse_next,                     \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_reverse_next,         \
        CCC_Flat_double_ended_queue                                            \
            *: CCC_flat_double_ended_queue_reverse_next,                       \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_reverse_next,         \
        CCC_Tree_map *: CCC_tree_map_reverse_next,                             \
        CCC_Array_tree_map *: CCC_array_tree_map_reverse_next,                 \
        CCC_Buffer const *: CCC_buffer_reverse_next,                           \
        CCC_Adaptive_map const *: CCC_adaptive_map_reverse_next,               \
        CCC_Array_adaptive_map const *: CCC_array_adaptive_map_reverse_next,   \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_reverse_next,                       \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_reverse_next,   \
        CCC_Array_tree_map const *: CCC_array_tree_map_reverse_next,           \
        CCC_Tree_map const *: CCC_tree_map_reverse_next)(                      \
        (container_pointer), (void_iterator_pointer))

#define CCC_private_end(container_pointer)                                     \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_end,                                          \
        CCC_Flat_hash_map *: CCC_flat_hash_map_end,                            \
        CCC_Adaptive_map *: CCC_adaptive_map_end,                              \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_end,                  \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_end,        \
        CCC_Singly_linked_list *: CCC_singly_linked_list_end,                  \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_end,                  \
        CCC_Tree_map *: CCC_tree_map_end,                                      \
        CCC_Array_tree_map *: CCC_array_tree_map_end,                          \
        CCC_Buffer const *: CCC_buffer_end,                                    \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_end,                      \
        CCC_Adaptive_map const *: CCC_adaptive_map_end,                        \
        CCC_Array_adaptive_map const *: CCC_array_adaptive_map_end,            \
        CCC_Flat_double_ended_queue const *: CCC_flat_double_ended_queue_end,  \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_end,            \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_end,            \
        CCC_Array_tree_map const *: CCC_array_tree_map_end,                    \
        CCC_Tree_map const *: CCC_tree_map_end)((container_pointer))

#define CCC_private_reverse_end(container_pointer)                             \
    _Generic((container_pointer),                                              \
        CCC_Buffer *: CCC_buffer_reverse_end,                                  \
        CCC_Adaptive_map *: CCC_adaptive_map_reverse_end,                      \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_reverse_end,          \
        CCC_Flat_double_ended_queue                                            \
            *: CCC_flat_double_ended_queue_reverse_end,                        \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_reverse_end,          \
        CCC_Tree_map *: CCC_tree_map_reverse_end,                              \
        CCC_Array_tree_map *: CCC_array_tree_map_reverse_end,                  \
        CCC_Buffer const *: CCC_buffer_reverse_end,                            \
        CCC_Adaptive_map const *: CCC_adaptive_map_reverse_end,                \
        CCC_Array_adaptive_map const *: CCC_array_adaptive_map_reverse_end,    \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_reverse_end,                        \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_reverse_end,    \
        CCC_Array_tree_map const *: CCC_array_tree_map_reverse_end,            \
        CCC_Tree_map const *: CCC_tree_map_reverse_end)((container_pointer))

#define CCC_private_equal_range(container_pointer,                             \
                                begin_and_end_key_pointer...)                  \
    _Generic((container_pointer),                                              \
        CCC_Adaptive_map *: CCC_adaptive_map_equal_range,                      \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_equal_range,          \
        CCC_Array_tree_map *: CCC_array_tree_map_equal_range,                  \
        CCC_Array_tree_map const *: CCC_array_tree_map_equal_range,            \
        CCC_Tree_map *: CCC_tree_map_equal_range,                              \
        CCC_Tree_map const *: CCC_tree_map_equal_range)(                       \
        (container_pointer), begin_and_end_key_pointer)

#define CCC_private_equal_range_wrap(container_pointer,                        \
                                     begin_and_end_key_pointer...)             \
    _Generic((container_pointer),                                              \
        CCC_Adaptive_map *: &(                                                 \
                 CCC_Range){CCC_adaptive_map_equal_range(                      \
                                (CCC_Adaptive_map *)(container_pointer),       \
                                begin_and_end_key_pointer)                     \
                                .private},                                     \
        CCC_Tree_map *: &(CCC_Range){CCC_tree_map_equal_range(                 \
                                         (CCC_Tree_map *)(container_pointer),  \
                                         begin_and_end_key_pointer)            \
                                         .private},                            \
        CCC_Tree_map const *: &(                                               \
                 CCC_Range){CCC_tree_map_equal_range(                          \
                                (CCC_Tree_map const *)(container_pointer),     \
                                begin_and_end_key_pointer)                     \
                                .private},                                     \
        CCC_Array_tree_map *: &(                                               \
                 CCC_Handle_range){CCC_array_tree_map_equal_range(             \
                                       (CCC_Array_tree_map                     \
                                            *)(container_pointer),             \
                                       begin_and_end_key_pointer)              \
                                       .private},                              \
        CCC_Array_tree_map const *: &(                                         \
                 CCC_Handle_range){CCC_array_tree_map_equal_range(             \
                                       (CCC_Array_tree_map const               \
                                            *)(container_pointer),             \
                                       begin_and_end_key_pointer)              \
                                       .private},                              \
        CCC_Array_adaptive_map *: &(CCC_Handle_range){                         \
            CCC_array_adaptive_map_equal_range(                                \
                (CCC_Array_adaptive_map *)(container_pointer),                 \
                begin_and_end_key_pointer)                                     \
                .private})

#define CCC_private_equal_range_reverse(                                       \
    container_pointer, reverse_begin_and_reverse_end_key_pointer...)           \
    _Generic((container_pointer),                                              \
        CCC_Adaptive_map *: CCC_adaptive_map_equal_range_reverse,              \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_equal_range_reverse,  \
        CCC_Array_tree_map *: CCC_array_tree_map_equal_range_reverse,          \
        CCC_Array_tree_map const *: CCC_array_tree_map_equal_range_reverse,    \
        CCC_Tree_map *: CCC_tree_map_equal_range_reverse,                      \
        CCC_Tree_map const *: CCC_tree_map_equal_range_reverse)(               \
        (container_pointer), reverse_begin_and_reverse_end_key_pointer)

#define CCC_private_equal_range_reverse_wrap(container_pointer,                \
                                             begin_and_end_key_pointer...)     \
    _Generic(                                                                  \
        (container_pointer),                                                   \
        CCC_Adaptive_map *: &(                                                 \
            CCC_Range_reverse){CCC_adaptive_map_equal_range_reverse(           \
                                   (CCC_Adaptive_map *)(container_pointer),    \
                                   begin_and_end_key_pointer)                  \
                                   .private},                                  \
        CCC_Tree_map *: &(                                                     \
            CCC_Range_reverse){CCC_tree_map_equal_range_reverse(               \
                                   (CCC_Tree_map *)(container_pointer),        \
                                   begin_and_end_key_pointer)                  \
                                   .private},                                  \
        CCC_Tree_map const *: &(                                               \
            CCC_Range_reverse){CCC_tree_map_equal_range_reverse(               \
                                   (CCC_Tree_map const *)(container_pointer),  \
                                   begin_and_end_key_pointer)                  \
                                   .private},                                  \
        CCC_Array_tree_map *: &(                                               \
            CCC_Handle_range_reverse){CCC_array_tree_map_equal_range_reverse(  \
                                          (CCC_Array_tree_map                  \
                                               *)(container_pointer),          \
                                          begin_and_end_key_pointer)           \
                                          .private},                           \
        CCC_Array_tree_map const *: &(                                         \
            CCC_Handle_range_reverse){CCC_array_tree_map_equal_range_reverse(  \
                                          (CCC_Array_tree_map const            \
                                               *)(container_pointer),          \
                                          begin_and_end_key_pointer)           \
                                          .private},                           \
        CCC_Array_adaptive_map *: &(CCC_Handle_range_reverse){                 \
            CCC_array_adaptive_map_equal_range_reverse(                        \
                (CCC_Array_adaptive_map *)(container_pointer),                 \
                begin_and_end_key_pointer)                                     \
                .private})

/** These generic macros will take precedence over any name shortening the user
may have requested from the types.h file. The standard range name conflicts
with these generic selectors. But if the user wants traits we should select
the appropriate range from the selections. */
#ifdef range_begin
#    undef range_begin
#endif
#ifdef range_end
#    undef range_end
#endif
#ifdef range_reverse_begin
#    undef range_reverse_begin
#endif
#ifdef range_reverse_end
#    undef range_reverse_end
#endif

#define CCC_private_range_begin(range_pointer)                                 \
    _Generic((range_pointer),                                                  \
        CCC_Range *: CCC_range_begin,                                          \
        CCC_Range const *: CCC_range_begin,                                    \
        CCC_Handle_range *: CCC_array_range_begin,                             \
        CCC_Handle_range const *: CCC_array_range_begin)((range_pointer))

#define CCC_private_range_end(range_pointer)                                   \
    _Generic((range_pointer),                                                  \
        CCC_Range *: CCC_range_end,                                            \
        CCC_Range const *: CCC_range_end,                                      \
        CCC_Handle_range *: CCC_array_range_end,                               \
        CCC_Handle_range const *: CCC_array_range_end)((range_pointer))

#define CCC_private_range_reverse_begin(range_reverse_pointer)                 \
    _Generic((range_reverse_pointer),                                          \
        CCC_Range_reverse *: CCC_range_reverse_begin,                          \
        CCC_Range_reverse const *: CCC_range_reverse_begin,                    \
        CCC_Handle_range_reverse *: CCC_array_range_reverse_begin,             \
        CCC_Handle_range_reverse const *: CCC_array_range_reverse_begin)(      \
        (range_reverse_pointer))

#define CCC_private_range_reverse_end(range_reverse_pointer)                   \
    _Generic((range_reverse_pointer),                                          \
        CCC_Range_reverse *: CCC_range_reverse_end,                            \
        CCC_Range_reverse const *: CCC_range_reverse_end,                      \
        CCC_Handle_range_reverse *: CCC_array_range_reverse_end,               \
        CCC_Handle_range_reverse const *: CCC_array_range_reverse_end)(        \
        (range_reverse_pointer))

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

#define CCC_private_copy(destination_container_pointer,                        \
                         source_container_pointer, allocate_pointer)           \
    _Generic((destination_container_pointer),                                  \
        CCC_Bitset *: CCC_bitset_copy,                                         \
        CCC_Flat_hash_map *: CCC_flat_hash_map_copy,                           \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_copy,                 \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_copy,               \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_copy,       \
        CCC_Array_tree_map *: CCC_array_tree_map_copy)(                        \
        (destination_container_pointer), (source_container_pointer),           \
        (allocate_pointer))

#define CCC_private_reserve(container_pointer, n_to_add, allocate_pointer)     \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_reserve,                                      \
        CCC_Buffer *: CCC_buffer_reserve,                                      \
        CCC_Flat_hash_map *: CCC_flat_hash_map_reserve,                        \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_reserve,              \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_reserve,            \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_reserve,    \
        CCC_Array_tree_map *: CCC_array_tree_map_reserve)(                     \
        (container_pointer), (n_to_add), (allocate_pointer))

#define CCC_private_clear(container_pointer, ...)                              \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_clear,                                        \
        CCC_Buffer *: CCC_buffer_clear,                                        \
        CCC_Flat_hash_map *: CCC_flat_hash_map_clear,                          \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_clear,                \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_clear,              \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_clear,      \
        CCC_Singly_linked_list *: CCC_singly_linked_list_clear,                \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_clear,                \
        CCC_Adaptive_map *: CCC_adaptive_map_clear,                            \
        CCC_Priority_queue *: CCC_priority_queue_clear,                        \
        CCC_Tree_map *: CCC_tree_map_clear,                                    \
        CCC_Array_tree_map *: CCC_array_tree_map_clear)(                       \
        (container_pointer)__VA_OPT__(, __VA_ARGS__))

#define CCC_private_clear_and_free(container_pointer, ...)                     \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_clear_and_free,                               \
        CCC_Buffer *: CCC_buffer_clear_and_free,                               \
        CCC_Flat_hash_map *: CCC_flat_hash_map_clear_and_free,                 \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_clear_and_free,       \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_clear_and_free,     \
        CCC_Flat_double_ended_queue                                            \
            *: CCC_flat_double_ended_queue_clear_and_free,                     \
        CCC_Array_tree_map *: CCC_array_tree_map_clear_and_free)(              \
        (container_pointer)__VA_OPT__(, __VA_ARGS__))

#define CCC_private_clear_and_free_reserve(container_pointer,                  \
                                           destructor_and_free_args...)        \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_clear_and_free_reserve,                       \
        CCC_Buffer *: CCC_buffer_clear_and_free_reserve,                       \
        CCC_Flat_hash_map *: CCC_flat_hash_map_clear_and_free_reserve,         \
        CCC_Array_adaptive_map                                                 \
            *: CCC_array_adaptive_map_clear_and_free_reserve,                  \
        CCC_Flat_priority_queue                                                \
            *: CCC_flat_priority_queue_clear_and_free_reserve,                 \
        CCC_Flat_double_ended_queue                                            \
            *: CCC_flat_double_ended_queue_clear_and_free_reserve,             \
        CCC_Array_tree_map *: CCC_array_tree_map_clear_and_free_reserve)(      \
        (container_pointer), destructor_and_free_args)

/*===================    Standard Getters Interface   =======================*/

#define CCC_private_count(container_pointer)                                   \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_count,                                        \
        CCC_Buffer *: CCC_buffer_count,                                        \
        CCC_Flat_hash_map *: CCC_flat_hash_map_count,                          \
        CCC_Adaptive_map *: CCC_adaptive_map_count,                            \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_count,                \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_count,              \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_count,      \
        CCC_Priority_queue *: CCC_priority_queue_count,                        \
        CCC_Singly_linked_list *: CCC_singly_linked_list_count,                \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_count,                \
        CCC_Tree_map *: CCC_tree_map_count,                                    \
        CCC_Array_tree_map *: CCC_array_tree_map_count,                        \
        CCC_Bitset const *: CCC_bitset_count,                                  \
        CCC_Buffer const *: CCC_buffer_count,                                  \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_count,                    \
        CCC_Adaptive_map const *: CCC_adaptive_map_count,                      \
        CCC_Array_adaptive_map const *: CCC_array_adaptive_map_count,          \
        CCC_Flat_priority_queue const *: CCC_flat_priority_queue_count,        \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_count,                              \
        CCC_Priority_queue const *: CCC_priority_queue_count,                  \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_count,          \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_count,          \
        CCC_Array_tree_map const *: CCC_array_tree_map_count,                  \
        CCC_Tree_map const *: CCC_tree_map_count)((container_pointer))

#define CCC_private_capacity(container_pointer)                                \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_capacity,                                     \
        CCC_Buffer *: CCC_buffer_capacity,                                     \
        CCC_Flat_hash_map *: CCC_flat_hash_map_capacity,                       \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_capacity,             \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_capacity,           \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_capacity,   \
        CCC_Array_tree_map *: CCC_array_tree_map_capacity,                     \
        CCC_Bitset const *: CCC_bitset_capacity,                               \
        CCC_Buffer const *: CCC_buffer_capacity,                               \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_capacity,                 \
        CCC_Array_adaptive_map const *: CCC_array_adaptive_map_capacity,       \
        CCC_Flat_priority_queue const *: CCC_flat_priority_queue_capacity,     \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_capacity,                           \
        CCC_Array_tree_map const *: CCC_array_tree_map_capacity)(              \
        (container_pointer))

#define CCC_private_is_empty(container_pointer)                                \
    _Generic((container_pointer),                                              \
        CCC_Bitset *: CCC_bitset_is_empty,                                     \
        CCC_Buffer *: CCC_buffer_is_empty,                                     \
        CCC_Flat_hash_map *: CCC_flat_hash_map_is_empty,                       \
        CCC_Adaptive_map *: CCC_adaptive_map_is_empty,                         \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_is_empty,             \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_is_empty,           \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_is_empty,   \
        CCC_Priority_queue *: CCC_priority_queue_is_empty,                     \
        CCC_Singly_linked_list *: CCC_singly_linked_list_is_empty,             \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_is_empty,             \
        CCC_Tree_map *: CCC_tree_map_is_empty,                                 \
        CCC_Array_tree_map *: CCC_array_tree_map_is_empty,                     \
        CCC_Bitset const *: CCC_bitset_is_empty,                               \
        CCC_Buffer const *: CCC_buffer_is_empty,                               \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_is_empty,                 \
        CCC_Adaptive_map const *: CCC_adaptive_map_is_empty,                   \
        CCC_Array_adaptive_map const *: CCC_array_adaptive_map_is_empty,       \
        CCC_Flat_priority_queue const *: CCC_flat_priority_queue_is_empty,     \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_is_empty,                           \
        CCC_Priority_queue const *: CCC_priority_queue_is_empty,               \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_is_empty,       \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_is_empty,       \
        CCC_Array_tree_map const *: CCC_array_tree_map_is_empty,               \
        CCC_Tree_map const *: CCC_tree_map_is_empty)((container_pointer))

#define CCC_private_validate(container_pointer)                                \
    _Generic((container_pointer),                                              \
        CCC_Flat_hash_map *: CCC_flat_hash_map_validate,                       \
        CCC_Adaptive_map *: CCC_adaptive_map_validate,                         \
        CCC_Array_adaptive_map *: CCC_array_adaptive_map_validate,             \
        CCC_Flat_priority_queue *: CCC_flat_priority_queue_validate,           \
        CCC_Flat_double_ended_queue *: CCC_flat_double_ended_queue_validate,   \
        CCC_Priority_queue *: CCC_priority_queue_validate,                     \
        CCC_Singly_linked_list *: CCC_singly_linked_list_validate,             \
        CCC_Doubly_linked_list *: CCC_doubly_linked_list_validate,             \
        CCC_Tree_map *: CCC_tree_map_validate,                                 \
        CCC_Array_tree_map *: CCC_array_tree_map_validate,                     \
        CCC_Flat_hash_map const *: CCC_flat_hash_map_validate,                 \
        CCC_Adaptive_map const *: CCC_adaptive_map_validate,                   \
        CCC_Array_adaptive_map const *: CCC_array_adaptive_map_validate,       \
        CCC_Flat_priority_queue const *: CCC_flat_priority_queue_validate,     \
        CCC_Flat_double_ended_queue const                                      \
            *: CCC_flat_double_ended_queue_validate,                           \
        CCC_Priority_queue const *: CCC_priority_queue_validate,               \
        CCC_Singly_linked_list const *: CCC_singly_linked_list_validate,       \
        CCC_Doubly_linked_list const *: CCC_doubly_linked_list_validate,       \
        CCC_Array_tree_map const *: CCC_array_tree_map_validate,               \
        CCC_Tree_map const *: CCC_tree_map_validate)((container_pointer))

#endif /* CCC_PRIVATE_TRAITS_H */
