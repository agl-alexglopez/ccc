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
#ifndef CCC_PRIVATE_TREE_MAP_H
#define CCC_PRIVATE_TREE_MAP_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../types.h"
#include "private_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @internal A WAVL node follows traditional balanced binary tree constructs
except for the rank field which can be simplified to an even/odd parity. */
struct CCC_Tree_map_node
{
    /** @internal Children in an array to unite left and right cases. */
    struct CCC_Tree_map_node *branch[2];
    /** @internal The parent node needed for iteration and rotation. */
    struct CCC_Tree_map_node *parent;
    /** @internal The rank parity of a node 1(odd) or 0(even). */
    uint8_t parity;
};

/** @internal The realtime ordered map offers strict `O(log(N))` searching,
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
struct CCC_Tree_map
{
    /** @internal The root of the tree or the sentinel end if empty. */
    struct CCC_Tree_map_node *root;
    /** @internal The count of stored nodes in the tree. */
    size_t count;
    /** @internal The byte offset of the key in the user struct. */
    size_t key_offset;
    /** @internal The byte offset of the intrusive element in the user struct.
     */
    size_t type_intruder_offset;
    /** @internal The size of the user struct holding the intruder. */
    size_t sizeof_type;
    /** @internal The comparison function for three way comparison. */
    CCC_Key_comparator *compare;
    /** @internal An allocation function, if any. */
    CCC_Allocator *allocate;
    /** @internal Auxiliary data, if any. */
    void *context;
};

/** @internal An entry is a way to store a node or the information needed to
insert a node without a second query. The user can then take different actions
depending on the Occupied or Vacant status of the entry. */
struct CCC_Tree_map_entry
{
    /** @internal The tree associated with this query. */
    struct CCC_Tree_map *map;
    /** @internal The result of the last comparison to find the user specified
    node. Equal if found or indicates the direction the node should be
    inserted from the parent we currently store in the entry. */
    CCC_Order last_order;
    /** @internal The stored node or it's parent if it does not exist. */
    struct CCC_Entry entry;
};

/** @internal Enable return by compound literal reference on the stack. Think
of this method as return by value but with the additional ability to pass by
pointer in a functional style. `fnB(&(union
CCC_Tree_map_entry){fnA().private});` */
union CCC_Tree_map_entry_wrap
{
    /** @internal The field containing the entry struct. */
    struct CCC_Tree_map_entry private;
};

/*=========================   Private Interface  ============================*/

/** @internal */
void *CCC_private_tree_map_key_in_slot(struct CCC_Tree_map const *,
                                       void const *slot);
/** @internal */
struct CCC_Tree_map_node *
CCC_private_tree_map_node_in_slot(struct CCC_Tree_map const *,
                                  void const *slot);
/** @internal */
struct CCC_Tree_map_entry
CCC_private_tree_map_entry(struct CCC_Tree_map const *, void const *key);
/** @internal */
void *CCC_private_tree_map_insert(
    struct CCC_Tree_map *, struct CCC_Tree_map_node *parent,
    CCC_Order last_order, struct CCC_Tree_map_node *type_output_intruder);

/*==========================   Initialization     ===========================*/

/** @internal */
#define CCC_private_tree_map_initialize(                                       \
    private_struct_name, private_node_node_field, private_key_node_field,      \
    private_key_order_fn, private_allocate, private_context_data)              \
    {                                                                          \
        .root = NULL,                                                          \
        .count = 0,                                                            \
        .key_offset = offsetof(private_struct_name, private_key_node_field),   \
        .type_intruder_offset                                                  \
        = offsetof(private_struct_name, private_node_node_field),              \
        .sizeof_type = sizeof(private_struct_name),                            \
        .compare = (private_key_order_fn),                                     \
        .allocate = (private_allocate),                                        \
        .context = (private_context_data),                                     \
    }

/** @internal */
#define CCC_private_tree_map_from(                                             \
    private_type_intruder_field_name, private_key_field_name, private_compare, \
    private_allocate, private_destroy, private_context_data,                   \
    private_compound_literal_array...)                                         \
    (__extension__({                                                           \
        typeof(*private_compound_literal_array) *private_tree_map_type_array   \
            = private_compound_literal_array;                                  \
        struct CCC_Tree_map private_map = CCC_private_tree_map_initialize(     \
            typeof(*private_tree_map_type_array),                              \
            private_type_intruder_field_name, private_key_field_name,          \
            private_compare, private_allocate, private_context_data);          \
        if (private_map.allocate)                                              \
        {                                                                      \
            size_t const private_count                                         \
                = sizeof(private_compound_literal_array)                       \
                / sizeof(*private_tree_map_type_array);                        \
            for (size_t private_i = 0; private_i < private_count; ++private_i) \
            {                                                                  \
                struct CCC_Tree_map_entry private_tree_map_entry               \
                    = CCC_private_tree_map_entry(                              \
                        &private_map,                                          \
                        (void *)&private_tree_map_type_array[private_i]        \
                            .private_key_field_name);                          \
                if (!(private_tree_map_entry.entry.status                      \
                      & CCC_ENTRY_OCCUPIED))                                   \
                {                                                              \
                    typeof(*private_tree_map_type_array) *const                \
                        private_new_slot                                       \
                        = private_map.allocate((CCC_Allocator_context){        \
                            .input = NULL,                                     \
                            .bytes = private_map.sizeof_type,                  \
                            .context = private_map.context,                    \
                        });                                                    \
                    if (!private_new_slot)                                     \
                    {                                                          \
                        (void)CCC_tree_map_clear(&private_map,                 \
                                                 private_destroy);             \
                        break;                                                 \
                    }                                                          \
                    *private_new_slot                                          \
                        = private_tree_map_type_array[private_i];              \
                    CCC_private_tree_map_insert(                               \
                        &private_map,                                          \
                        CCC_private_tree_map_node_in_slot(                     \
                            &private_map, private_tree_map_entry.entry.type),  \
                        private_tree_map_entry.last_order,                     \
                        CCC_private_tree_map_node_in_slot(&private_map,        \
                                                          private_new_slot));  \
                }                                                              \
                else                                                           \
                {                                                              \
                    struct CCC_Tree_map_node private_node_saved                \
                        = *CCC_private_tree_map_node_in_slot(                  \
                            &private_map, private_tree_map_entry.entry.type);  \
                    *((typeof(*private_tree_map_type_array) *)                 \
                          private_tree_map_entry.entry.type)                   \
                        = private_tree_map_type_array[private_i];              \
                    *CCC_private_tree_map_node_in_slot(                        \
                        &private_map, private_tree_map_entry.entry.type)       \
                        = private_node_saved;                                  \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_map;                                                           \
    }))

/*==================   Helper Macros for Repeated Logic     =================*/

/** @internal */
#define CCC_private_tree_map_new(Tree_map_entry)                               \
    (__extension__({                                                           \
        void *private_tree_map_ins_allocate_ret = NULL;                        \
        if ((Tree_map_entry)->map->allocate)                                   \
        {                                                                      \
            private_tree_map_ins_allocate_ret                                  \
                = (Tree_map_entry)                                             \
                      ->map->allocate((CCC_Allocator_context){                 \
                          .input = NULL,                                       \
                          .bytes = (Tree_map_entry)->map->sizeof_type,         \
                          .context = (Tree_map_entry)->map->context,           \
                      });                                                      \
        }                                                                      \
        private_tree_map_ins_allocate_ret;                                     \
    }))

/** @internal */
#define CCC_private_tree_map_insert_key_val(Tree_map_entry, new_data,          \
                                            type_compound_literal...)          \
    (__extension__({                                                           \
        if (new_data)                                                          \
        {                                                                      \
            *new_data = type_compound_literal;                                 \
            new_data = CCC_private_tree_map_insert(                            \
                (Tree_map_entry)->map,                                         \
                CCC_private_tree_map_node_in_slot(                             \
                    (Tree_map_entry)->map, (Tree_map_entry)->entry.type),      \
                (Tree_map_entry)->last_order,                                  \
                CCC_private_tree_map_node_in_slot((Tree_map_entry)->map,       \
                                                  new_data));                  \
        }                                                                      \
    }))

/** @internal */
#define CCC_private_tree_map_insert_and_copy_key(                              \
    tree_map_insert_entry, tree_map_insert_entry_ret, key,                     \
    type_compound_literal...)                                                  \
    (__extension__({                                                           \
        typeof(type_compound_literal) *private_tree_map_new_ins_base           \
            = CCC_private_tree_map_new((&tree_map_insert_entry));              \
        tree_map_insert_entry_ret = (struct CCC_Entry){                        \
            .type = private_tree_map_new_ins_base,                             \
            .status = CCC_ENTRY_INSERT_ERROR,                                  \
        };                                                                     \
        if (private_tree_map_new_ins_base)                                     \
        {                                                                      \
            *private_tree_map_new_ins_base = type_compound_literal;            \
            *((typeof(key) *)CCC_private_tree_map_key_in_slot(                 \
                tree_map_insert_entry.map, private_tree_map_new_ins_base))     \
                = key;                                                         \
            (void)CCC_private_tree_map_insert(                                 \
                tree_map_insert_entry.map,                                     \
                CCC_private_tree_map_node_in_slot(                             \
                    tree_map_insert_entry.map,                                 \
                    tree_map_insert_entry.entry.type),                         \
                tree_map_insert_entry.last_order,                              \
                CCC_private_tree_map_node_in_slot(                             \
                    tree_map_insert_entry.map,                                 \
                    private_tree_map_new_ins_base));                           \
        }                                                                      \
    }))

/*==================     Core Macro Implementations     =====================*/

/** @internal */
#define CCC_private_tree_map_and_modify_with(Tree_map_entry_pointer,           \
                                             type_name, closure_over_T...)     \
    (__extension__({                                                           \
        __auto_type private_tree_map_ent_pointer = (Tree_map_entry_pointer);   \
        struct CCC_Tree_map_entry private_tree_map_mod_ent                     \
            = {.entry = {.status = CCC_ENTRY_ARGUMENT_ERROR}};                 \
        if (private_tree_map_ent_pointer)                                      \
        {                                                                      \
            private_tree_map_mod_ent = private_tree_map_ent_pointer->private;  \
            if (private_tree_map_mod_ent.entry.status & CCC_ENTRY_OCCUPIED)    \
            {                                                                  \
                type_name *const T = private_tree_map_mod_ent.entry.type;      \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_tree_map_mod_ent;                                              \
    }))

/** @internal */
#define CCC_private_tree_map_or_insert_with(Tree_map_entry_pointer,            \
                                            type_compound_literal...)          \
    (__extension__({                                                           \
        __auto_type private_or_ins_entry_pointer = (Tree_map_entry_pointer);   \
        typeof(type_compound_literal) *private_tree_map_or_ins_ret = NULL;     \
        if (private_or_ins_entry_pointer)                                      \
        {                                                                      \
            if (private_or_ins_entry_pointer->private.entry.status             \
                == CCC_ENTRY_OCCUPIED)                                         \
            {                                                                  \
                private_tree_map_or_ins_ret                                    \
                    = private_or_ins_entry_pointer->private.entry.type;        \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_tree_map_or_ins_ret = CCC_private_tree_map_new(        \
                    &private_or_ins_entry_pointer->private);                   \
                CCC_private_tree_map_insert_key_val(                           \
                    &private_or_ins_entry_pointer->private,                    \
                    private_tree_map_or_ins_ret, type_compound_literal);       \
            }                                                                  \
        }                                                                      \
        private_tree_map_or_ins_ret;                                           \
    }))

/** @internal */
#define CCC_private_tree_map_insert_entry_with(Tree_map_entry_pointer,         \
                                               type_compound_literal...)       \
    (__extension__({                                                           \
        __auto_type private_ins_entry_pointer = (Tree_map_entry_pointer);      \
        typeof(type_compound_literal) *private_tree_map_ins_ent_ret = NULL;    \
        if (private_ins_entry_pointer)                                         \
        {                                                                      \
            if (!(private_ins_entry_pointer->private.entry.status              \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                private_tree_map_ins_ent_ret = CCC_private_tree_map_new(       \
                    &private_ins_entry_pointer->private);                      \
                CCC_private_tree_map_insert_key_val(                           \
                    &private_ins_entry_pointer->private,                       \
                    private_tree_map_ins_ent_ret, type_compound_literal);      \
            }                                                                  \
            else if (private_ins_entry_pointer->private.entry.status           \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct CCC_Tree_map_node private_ins_ent_saved                 \
                    = *CCC_private_tree_map_node_in_slot(                      \
                        private_ins_entry_pointer->private.map,                \
                        private_ins_entry_pointer->private.entry.type);        \
                *((typeof(private_tree_map_ins_ent_ret))                       \
                      private_ins_entry_pointer->private.entry.type)           \
                    = type_compound_literal;                                   \
                *CCC_private_tree_map_node_in_slot(                            \
                    private_ins_entry_pointer->private.map,                    \
                    private_ins_entry_pointer->private.entry.type)             \
                    = private_ins_ent_saved;                                   \
                private_tree_map_ins_ent_ret                                   \
                    = private_ins_entry_pointer->private.entry.type;           \
            }                                                                  \
        }                                                                      \
        private_tree_map_ins_ent_ret;                                          \
    }))

/** @internal */
#define CCC_private_tree_map_try_insert_with(Tree_map_pointer, key,            \
                                             type_compound_literal...)         \
    (__extension__({                                                           \
        struct CCC_Tree_map *const private_try_ins_map_pointer                 \
            = (Tree_map_pointer);                                              \
        struct CCC_Entry private_tree_map_try_ins_ent_ret                      \
            = {.status = CCC_ENTRY_ARGUMENT_ERROR};                            \
        if (private_try_ins_map_pointer)                                       \
        {                                                                      \
            __auto_type private_tree_map_key = (key);                          \
            struct CCC_Tree_map_entry private_tree_map_try_ins_ent             \
                = CCC_private_tree_map_entry(private_try_ins_map_pointer,      \
                                             (void *)&private_tree_map_key);   \
            if (!(private_tree_map_try_ins_ent.entry.status                    \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                CCC_private_tree_map_insert_and_copy_key(                      \
                    private_tree_map_try_ins_ent,                              \
                    private_tree_map_try_ins_ent_ret, private_tree_map_key,    \
                    type_compound_literal);                                    \
            }                                                                  \
            else if (private_tree_map_try_ins_ent.entry.status                 \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                private_tree_map_try_ins_ent_ret                               \
                    = private_tree_map_try_ins_ent.entry;                      \
            }                                                                  \
        }                                                                      \
        private_tree_map_try_ins_ent_ret;                                      \
    }))

/** @internal */
#define CCC_private_tree_map_insert_or_assign_with(Tree_map_pointer, key,      \
                                                   type_compound_literal...)   \
    (__extension__({                                                           \
        struct CCC_Tree_map *const private_ins_or_assign_map_pointer           \
            = (Tree_map_pointer);                                              \
        struct CCC_Entry private_tree_map_ins_or_assign_ent_ret                \
            = {.status = CCC_ENTRY_ARGUMENT_ERROR};                            \
        if (private_ins_or_assign_map_pointer)                                 \
        {                                                                      \
            __auto_type private_tree_map_key = (key);                          \
            struct CCC_Tree_map_entry private_tree_map_ins_or_assign_ent       \
                = CCC_private_tree_map_entry(                                  \
                    private_ins_or_assign_map_pointer,                         \
                    (void *)&private_tree_map_key);                            \
            if (!(private_tree_map_ins_or_assign_ent.entry.status              \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                CCC_private_tree_map_insert_and_copy_key(                      \
                    private_tree_map_ins_or_assign_ent,                        \
                    private_tree_map_ins_or_assign_ent_ret,                    \
                    private_tree_map_key, type_compound_literal);              \
            }                                                                  \
            else if (private_tree_map_ins_or_assign_ent.entry.status           \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct CCC_Tree_map_node private_ins_ent_saved                 \
                    = *CCC_private_tree_map_node_in_slot(                      \
                        private_tree_map_ins_or_assign_ent.map,                \
                        private_tree_map_ins_or_assign_ent.entry.type);        \
                *((typeof(type_compound_literal) *)                            \
                      private_tree_map_ins_or_assign_ent.entry.type)           \
                    = type_compound_literal;                                   \
                *CCC_private_tree_map_node_in_slot(                            \
                    private_tree_map_ins_or_assign_ent.map,                    \
                    private_tree_map_ins_or_assign_ent.entry.type)             \
                    = private_ins_ent_saved;                                   \
                private_tree_map_ins_or_assign_ent_ret                         \
                    = private_tree_map_ins_or_assign_ent.entry;                \
                *((typeof(private_tree_map_key) *)                             \
                      CCC_private_tree_map_key_in_slot(                        \
                          private_tree_map_ins_or_assign_ent.map,              \
                          private_tree_map_ins_or_assign_ent_ret.type))        \
                    = private_tree_map_key;                                    \
            }                                                                  \
        }                                                                      \
        private_tree_map_ins_or_assign_ent_ret;                                \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_PRIVATE_REALTIME__ADAPTIVE_MAP_H */
