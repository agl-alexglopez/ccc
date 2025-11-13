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
#ifndef CCC_PRIVATE_REALTIME_ORDERED_MAP_H
#define CCC_PRIVATE_REALTIME_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../types.h"
#include "private_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private A WAVL node follows traditional balanced binary tree constructs
except for the rank field which can be simplified to an even/odd parity. */
struct CCC_Realtime_ordered_map_node
{
    /** @private Children in an array to unite left and right cases. */
    struct CCC_Realtime_ordered_map_node *branch[2];
    /** @private The parent node needed for iteration and rotation. */
    struct CCC_Realtime_ordered_map_node *parent;
    /** @private The rank for rank difference calculations 1(odd) or 0(even). */
    uint8_t parity;
};

/** @private The realtime ordered map offers strict `O(log(N))` searching,
inserting, and deleting operations with the Weak AVL Tree Rank Balance
framework. The number of rotations after an operation are kept to a maximum of
two, which neither the Red-Black Tree or AVL tree are able to achieve. However,
there may be `O(log(N))` rank changes, but these are efficient bit flip ops.

This makes the Weak AVL tree the leader in terms of minimal rotations and a
hybrid of the search strengths of an AVL tree with the favorable fix-up
maintenance of a Red-Black Tree. In fact, under a workload that is strictly
insertions, the WAVL tree is identical to an AVL tree in terms of balance and
shape, making it fast for searching while performing fewer rotations than the
AVL tree. The implementation is also simpler than either of the other trees. */
struct CCC_Realtime_ordered_map
{
    /** @private The root of the tree or the sentinel end if empty. */
    struct CCC_Realtime_ordered_map_node *root;
    /** @private The end sentinel in the struct for fewer code branches. */
    struct CCC_Realtime_ordered_map_node end;
    /** @private The count of stored nodes in the tree. */
    size_t count;
    /** @private The byte offset of the key in the user struct. */
    size_t key_offset;
    /** @private The byte offset of the intrusive element in the user struct. */
    size_t node_node_offset;
    /** @private The size of the user struct holding the intruder. */
    size_t sizeof_type;
    /** @private An allocation function, if any. */
    CCC_Allocator *allocate;
    /** @private The comparison function for three way comparison. */
    CCC_Key_comparator *order;
    /** @private Auxiliary data, if any. */
    void *context;
};

/** @private An entry is a way to store a node or the information needed to
insert a node without a second query. The user can then take different actions
depending on the Occupied or Vacant status of the entry. */
struct CCC_rtree_entry
{
    /** @private The tree associated with this query. */
    struct CCC_Realtime_ordered_map *rom;
    /** @private The result of the last comparison to find the user specified
    node. Equal if found or indicates the direction the node should be
    inserted from the parent we currently store in the entry. */
    CCC_Order last_order;
    /** @private The stored node or it's parent if it does not exist. */
    struct CCC_Entry entry;
};

/** @private Enable return by compound literal reference on the stack. Think
of this method as return by value but with the additional ability to pass by
pointer in a functional style. `fnB(&(union
CCC_Realtime_ordered_map_entry){fnA().private});` */
union CCC_Realtime_ordered_map_entry
{
    /** @private The field containing the entry struct. */
    struct CCC_rtree_entry private;
};

/*=========================   Private Interface  ============================*/

/** @private */
void *CCC_private_realtime_ordered_map_key_in_slot(
    struct CCC_Realtime_ordered_map const *rom, void const *slot);
/** @private */
struct CCC_Realtime_ordered_map_node *
CCC_private_Realtime_ordered_map_node_in_slot(
    struct CCC_Realtime_ordered_map const *rom, void const *slot);
/** @private */
struct CCC_rtree_entry CCC_private_realtime_ordered_map_entry(
    struct CCC_Realtime_ordered_map const *rom, void const *key);
/** @private */
void *CCC_private_realtime_ordered_map_insert(
    struct CCC_Realtime_ordered_map *rom,
    struct CCC_Realtime_ordered_map_node *parent, CCC_Order last_order,
    struct CCC_Realtime_ordered_map_node *out_handle);

/*==========================   Initialization     ===========================*/

/** @private */
#define CCC_private_realtime_ordered_map_initialize(                           \
    private_map_name, private_struct_name, private_node_node_field,            \
    private_key_node_field, private_key_order_fn, private_allocate,            \
    private_context_data)                                                      \
    {                                                                          \
        .root = &(private_map_name).end,                                       \
        .end = {.parity = 1,                                                   \
                .parent = &(private_map_name).end,                             \
                .branch = {&(private_map_name).end, &(private_map_name).end}}, \
        .count = 0,                                                            \
        .key_offset = offsetof(private_struct_name, private_key_node_field),   \
        .node_node_offset                                                      \
        = offsetof(private_struct_name, private_node_node_field),              \
        .sizeof_type = sizeof(private_struct_name),                            \
        .allocate = (private_allocate),                                        \
        .order = (private_key_order_fn),                                       \
        .context = (private_context_data),                                     \
    }

/*==================   Helper Macros for Repeated Logic     =================*/

/** @private */
#define CCC_private_realtime_ordered_map_new(Realtime_ordered_map_entry)       \
    (__extension__({                                                           \
        void *private_realtime_ordered_map_ins_allocate_ret = NULL;            \
        if ((Realtime_ordered_map_entry)->rom->allocate)                       \
        {                                                                      \
            private_realtime_ordered_map_ins_allocate_ret                      \
                = (Realtime_ordered_map_entry)                                 \
                      ->rom->allocate((CCC_Allocator_context){                 \
                          .input = NULL,                                       \
                          .bytes                                               \
                          = (Realtime_ordered_map_entry)->rom->sizeof_type,    \
                          .context                                             \
                          = (Realtime_ordered_map_entry)->rom->context,        \
                      });                                                      \
        }                                                                      \
        private_realtime_ordered_map_ins_allocate_ret;                         \
    }))

/** @private */
#define CCC_private_realtime_ordered_map_insert_key_val(                       \
    Realtime_ordered_map_entry, new_mem, lazy_key_value...)                    \
    (__extension__({                                                           \
        if (new_mem)                                                           \
        {                                                                      \
            *new_mem = lazy_key_value;                                         \
            new_mem = CCC_private_realtime_ordered_map_insert(                 \
                (Realtime_ordered_map_entry)->rom,                             \
                CCC_private_Realtime_ordered_map_node_in_slot(                 \
                    (Realtime_ordered_map_entry)->rom,                         \
                    (Realtime_ordered_map_entry)->entry.e),                    \
                (Realtime_ordered_map_entry)->last_order,                      \
                CCC_private_Realtime_ordered_map_node_in_slot(                 \
                    (Realtime_ordered_map_entry)->rom, new_mem));              \
        }                                                                      \
    }))

/** @private */
#define CCC_private_realtime_ordered_map_insert_and_copy_key(                  \
    realtime_ordered_map_insert_entry, realtime_ordered_map_insert_entry_ret,  \
    key, lazy_value...)                                                        \
    (__extension__({                                                           \
        typeof(lazy_value) *private_realtime_ordered_map_new_ins_base          \
            = CCC_private_realtime_ordered_map_new(                            \
                (&realtime_ordered_map_insert_entry));                         \
        realtime_ordered_map_insert_entry_ret = (struct CCC_Entry){            \
            .e = private_realtime_ordered_map_new_ins_base,                    \
            .stats = CCC_ENTRY_INSERT_ERROR,                                   \
        };                                                                     \
        if (private_realtime_ordered_map_new_ins_base)                         \
        {                                                                      \
            *private_realtime_ordered_map_new_ins_base = lazy_value;           \
            *((typeof(key) *)CCC_private_realtime_ordered_map_key_in_slot(     \
                realtime_ordered_map_insert_entry.rom,                         \
                private_realtime_ordered_map_new_ins_base))                    \
                = key;                                                         \
            (void)CCC_private_realtime_ordered_map_insert(                     \
                realtime_ordered_map_insert_entry.rom,                         \
                CCC_private_Realtime_ordered_map_node_in_slot(                 \
                    realtime_ordered_map_insert_entry.rom,                     \
                    realtime_ordered_map_insert_entry.entry.e),                \
                realtime_ordered_map_insert_entry.last_order,                  \
                CCC_private_Realtime_ordered_map_node_in_slot(                 \
                    realtime_ordered_map_insert_entry.rom,                     \
                    private_realtime_ordered_map_new_ins_base));               \
        }                                                                      \
    }))

/*==================     Core Macro Implementations     =====================*/

/** @private */
#define CCC_private_realtime_ordered_map_and_modify_w(                         \
    Realtime_ordered_map_entry_ptr, type_name, closure_over_T...)              \
    (__extension__({                                                           \
        __auto_type private_realtime_ordered_map_ent_ptr                       \
            = (Realtime_ordered_map_entry_ptr);                                \
        struct CCC_rtree_entry private_realtime_ordered_map_mod_ent            \
            = {.entry = {.stats = CCC_ENTRY_ARGUMENT_ERROR}};                  \
        if (private_realtime_ordered_map_ent_ptr)                              \
        {                                                                      \
            private_realtime_ordered_map_mod_ent                               \
                = private_realtime_ordered_map_ent_ptr->private;               \
            if (private_realtime_ordered_map_mod_ent.entry.stats               \
                & CCC_ENTRY_OCCUPIED)                                          \
            {                                                                  \
                type_name *const T                                             \
                    = private_realtime_ordered_map_mod_ent.entry.e;            \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_realtime_ordered_map_mod_ent;                                  \
    }))

/** @private */
#define CCC_private_realtime_ordered_map_or_insert_w(                          \
    Realtime_ordered_map_entry_ptr, lazy_key_value...)                         \
    (__extension__({                                                           \
        __auto_type private_or_ins_entry_ptr                                   \
            = (Realtime_ordered_map_entry_ptr);                                \
        typeof(lazy_key_value) *private_realtime_ordered_map_or_ins_ret        \
            = NULL;                                                            \
        if (private_or_ins_entry_ptr)                                          \
        {                                                                      \
            if (private_or_ins_entry_ptr->private.entry.stats                  \
                == CCC_ENTRY_OCCUPIED)                                         \
            {                                                                  \
                private_realtime_ordered_map_or_ins_ret                        \
                    = private_or_ins_entry_ptr->private.entry.e;               \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_realtime_ordered_map_or_ins_ret                        \
                    = CCC_private_realtime_ordered_map_new(                    \
                        &private_or_ins_entry_ptr->private);                   \
                CCC_private_realtime_ordered_map_insert_key_val(               \
                    &private_or_ins_entry_ptr->private,                        \
                    private_realtime_ordered_map_or_ins_ret, lazy_key_value);  \
            }                                                                  \
        }                                                                      \
        private_realtime_ordered_map_or_ins_ret;                               \
    }))

/** @private */
#define CCC_private_realtime_ordered_map_insert_entry_w(                       \
    Realtime_ordered_map_entry_ptr, lazy_key_value...)                         \
    (__extension__({                                                           \
        __auto_type private_ins_entry_ptr = (Realtime_ordered_map_entry_ptr);  \
        typeof(lazy_key_value) *private_realtime_ordered_map_ins_ent_ret       \
            = NULL;                                                            \
        if (private_ins_entry_ptr)                                             \
        {                                                                      \
            if (!(private_ins_entry_ptr->private.entry.stats                   \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                private_realtime_ordered_map_ins_ent_ret                       \
                    = CCC_private_realtime_ordered_map_new(                    \
                        &private_ins_entry_ptr->private);                      \
                CCC_private_realtime_ordered_map_insert_key_val(               \
                    &private_ins_entry_ptr->private,                           \
                    private_realtime_ordered_map_ins_ent_ret, lazy_key_value); \
            }                                                                  \
            else if (private_ins_entry_ptr->private.entry.stats                \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct CCC_Realtime_ordered_map_node private_ins_ent_saved     \
                    = *CCC_private_Realtime_ordered_map_node_in_slot(          \
                        private_ins_entry_ptr->private.rom,                    \
                        private_ins_entry_ptr->private.entry.e);               \
                *((typeof(private_realtime_ordered_map_ins_ent_ret))           \
                      private_ins_entry_ptr->private.entry.e)                  \
                    = lazy_key_value;                                          \
                *CCC_private_Realtime_ordered_map_node_in_slot(                \
                    private_ins_entry_ptr->private.rom,                        \
                    private_ins_entry_ptr->private.entry.e)                    \
                    = private_ins_ent_saved;                                   \
                private_realtime_ordered_map_ins_ent_ret                       \
                    = private_ins_entry_ptr->private.entry.e;                  \
            }                                                                  \
        }                                                                      \
        private_realtime_ordered_map_ins_ent_ret;                              \
    }))

/** @private */
#define CCC_private_realtime_ordered_map_try_insert_w(                         \
    Realtime_ordered_map_ptr, key, lazy_value...)                              \
    (__extension__({                                                           \
        struct CCC_Realtime_ordered_map *const private_try_ins_map_ptr         \
            = (Realtime_ordered_map_ptr);                                      \
        struct CCC_Entry private_realtime_ordered_map_try_ins_ent_ret          \
            = {.stats = CCC_ENTRY_ARGUMENT_ERROR};                             \
        if (private_try_ins_map_ptr)                                           \
        {                                                                      \
            __auto_type private_realtime_ordered_map_key = (key);              \
            struct CCC_rtree_entry private_realtime_ordered_map_try_ins_ent    \
                = CCC_private_realtime_ordered_map_entry(                      \
                    private_try_ins_map_ptr,                                   \
                    (void *)&private_realtime_ordered_map_key);                \
            if (!(private_realtime_ordered_map_try_ins_ent.entry.stats         \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                CCC_private_realtime_ordered_map_insert_and_copy_key(          \
                    private_realtime_ordered_map_try_ins_ent,                  \
                    private_realtime_ordered_map_try_ins_ent_ret,              \
                    private_realtime_ordered_map_key, lazy_value);             \
            }                                                                  \
            else if (private_realtime_ordered_map_try_ins_ent.entry.stats      \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                private_realtime_ordered_map_try_ins_ent_ret                   \
                    = private_realtime_ordered_map_try_ins_ent.entry;          \
            }                                                                  \
        }                                                                      \
        private_realtime_ordered_map_try_ins_ent_ret;                          \
    }))

/** @private */
#define CCC_private_realtime_ordered_map_insert_or_assign_w(                   \
    Realtime_ordered_map_ptr, key, lazy_value...)                              \
    (__extension__({                                                           \
        struct CCC_Realtime_ordered_map *const private_ins_or_assign_map_ptr   \
            = (Realtime_ordered_map_ptr);                                      \
        struct CCC_Entry private_realtime_ordered_map_ins_or_assign_ent_ret    \
            = {.stats = CCC_ENTRY_ARGUMENT_ERROR};                             \
        if (private_ins_or_assign_map_ptr)                                     \
        {                                                                      \
            __auto_type private_realtime_ordered_map_key = (key);              \
            struct CCC_rtree_entry                                             \
                private_realtime_ordered_map_ins_or_assign_ent                 \
                = CCC_private_realtime_ordered_map_entry(                      \
                    private_ins_or_assign_map_ptr,                             \
                    (void *)&private_realtime_ordered_map_key);                \
            if (!(private_realtime_ordered_map_ins_or_assign_ent.entry.stats   \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                CCC_private_realtime_ordered_map_insert_and_copy_key(          \
                    private_realtime_ordered_map_ins_or_assign_ent,            \
                    private_realtime_ordered_map_ins_or_assign_ent_ret,        \
                    private_realtime_ordered_map_key, lazy_value);             \
            }                                                                  \
            else if (private_realtime_ordered_map_ins_or_assign_ent.entry      \
                         .stats                                                \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct CCC_Realtime_ordered_map_node private_ins_ent_saved     \
                    = *CCC_private_Realtime_ordered_map_node_in_slot(          \
                        private_realtime_ordered_map_ins_or_assign_ent.rom,    \
                        private_realtime_ordered_map_ins_or_assign_ent.entry   \
                            .e);                                               \
                *((typeof(lazy_value) *)                                       \
                      private_realtime_ordered_map_ins_or_assign_ent.entry.e)  \
                    = lazy_value;                                              \
                *CCC_private_Realtime_ordered_map_node_in_slot(                \
                    private_realtime_ordered_map_ins_or_assign_ent.rom,        \
                    private_realtime_ordered_map_ins_or_assign_ent.entry.e)    \
                    = private_ins_ent_saved;                                   \
                private_realtime_ordered_map_ins_or_assign_ent_ret             \
                    = private_realtime_ordered_map_ins_or_assign_ent.entry;    \
                *((typeof(private_realtime_ordered_map_key) *)                 \
                      CCC_private_realtime_ordered_map_key_in_slot(            \
                          private_realtime_ordered_map_ins_or_assign_ent.rom,  \
                          private_realtime_ordered_map_ins_or_assign_ent_ret   \
                              .e))                                             \
                    = private_realtime_ordered_map_key;                        \
            }                                                                  \
        }                                                                      \
        private_realtime_ordered_map_ins_or_assign_ent_ret;                    \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_PRIVATE_REALTIME__ORDERED_MAP_H */
