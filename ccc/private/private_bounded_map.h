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
#ifndef CCC_PRIVATE_BOUNDED_MAP_H
#define CCC_PRIVATE_BOUNDED_MAP_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../types.h"
#include "private_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private A WAVL node follows traditional balanced binary tree constructs
except for the rank field which can be simplified to an even/odd parity. */
struct CCC_Bounded_map_node
{
    /** @private Children in an array to unite left and right cases. */
    struct CCC_Bounded_map_node *branch[2];
    /** @private The parent node needed for iteration and rotation. */
    struct CCC_Bounded_map_node *parent;
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
struct CCC_Bounded_map
{
    /** @private The root of the tree or the sentinel end if empty. */
    struct CCC_Bounded_map_node *root;
    /** @private The end sentinel in the struct for fewer code branches. */
    struct CCC_Bounded_map_node end;
    /** @private The count of stored nodes in the tree. */
    size_t count;
    /** @private The byte offset of the key in the user struct. */
    size_t key_offset;
    /** @private The byte offset of the intrusive element in the user struct. */
    size_t node_node_offset;
    /** @private The size of the user struct holding the intruder. */
    size_t sizeof_type;
    /** @private The comparison function for three way comparison. */
    CCC_Key_comparator *compare;
    /** @private An allocation function, if any. */
    CCC_Allocator *allocate;
    /** @private Auxiliary data, if any. */
    void *context;
};

/** @private An entry is a way to store a node or the information needed to
insert a node without a second query. The user can then take different actions
depending on the Occupied or Vacant status of the entry. */
struct CCC_Bounded_map_entry
{
    /** @private The tree associated with this query. */
    struct CCC_Bounded_map *map;
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
CCC_Bounded_map_entry){fnA().private});` */
union CCC_Bounded_map_entry_wrap
{
    /** @private The field containing the entry struct. */
    struct CCC_Bounded_map_entry private;
};

/*=========================   Private Interface  ============================*/

/** @private */
void *CCC_private_bounded_map_key_in_slot(struct CCC_Bounded_map const *,
                                          void const *slot);
/** @private */
struct CCC_Bounded_map_node *
CCC_private_Bounded_map_node_in_slot(struct CCC_Bounded_map const *,
                                     void const *slot);
/** @private */
struct CCC_Bounded_map_entry
CCC_private_bounded_map_entry(struct CCC_Bounded_map const *, void const *key);
/** @private */
void *CCC_private_bounded_map_insert(
    struct CCC_Bounded_map *, struct CCC_Bounded_map_node *parent,
    CCC_Order last_order, struct CCC_Bounded_map_node *type_output_intruder);

/*==========================   Initialization     ===========================*/

/** @private */
#define CCC_private_bounded_map_initialize(                                    \
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
        .compare = (private_key_order_fn),                                     \
        .allocate = (private_allocate),                                        \
        .context = (private_context_data),                                     \
    }

/*==================   Helper Macros for Repeated Logic     =================*/

/** @private */
#define CCC_private_bounded_map_new(Bounded_map_entry)                         \
    (__extension__({                                                           \
        void *private_bounded_map_ins_allocate_ret = NULL;                     \
        if ((Bounded_map_entry)->map->allocate)                                \
        {                                                                      \
            private_bounded_map_ins_allocate_ret                               \
                = (Bounded_map_entry)                                          \
                      ->map->allocate((CCC_Allocator_context){                 \
                          .input = NULL,                                       \
                          .bytes = (Bounded_map_entry)->map->sizeof_type,      \
                          .context = (Bounded_map_entry)->map->context,        \
                      });                                                      \
        }                                                                      \
        private_bounded_map_ins_allocate_ret;                                  \
    }))

/** @private */
#define CCC_private_bounded_map_insert_key_val(Bounded_map_entry, new_mem,     \
                                               lazy_key_value...)              \
    (__extension__({                                                           \
        if (new_mem)                                                           \
        {                                                                      \
            *new_mem = lazy_key_value;                                         \
            new_mem = CCC_private_bounded_map_insert(                          \
                (Bounded_map_entry)->map,                                      \
                CCC_private_Bounded_map_node_in_slot(                          \
                    (Bounded_map_entry)->map,                                  \
                    (Bounded_map_entry)->entry.type),                          \
                (Bounded_map_entry)->last_order,                               \
                CCC_private_Bounded_map_node_in_slot((Bounded_map_entry)->map, \
                                                     new_mem));                \
        }                                                                      \
    }))

/** @private */
#define CCC_private_bounded_map_insert_and_copy_key(                           \
    bounded_map_insert_entry, bounded_map_insert_entry_ret, key,               \
    type_compound_literal...)                                                  \
    (__extension__({                                                           \
        typeof(type_compound_literal) *private_bounded_map_new_ins_base        \
            = CCC_private_bounded_map_new((&bounded_map_insert_entry));        \
        bounded_map_insert_entry_ret = (struct CCC_Entry){                     \
            .type = private_bounded_map_new_ins_base,                          \
            .status = CCC_ENTRY_INSERT_ERROR,                                  \
        };                                                                     \
        if (private_bounded_map_new_ins_base)                                  \
        {                                                                      \
            *private_bounded_map_new_ins_base = type_compound_literal;         \
            *((typeof(key) *)CCC_private_bounded_map_key_in_slot(              \
                bounded_map_insert_entry.map,                                  \
                private_bounded_map_new_ins_base))                             \
                = key;                                                         \
            (void)CCC_private_bounded_map_insert(                              \
                bounded_map_insert_entry.map,                                  \
                CCC_private_Bounded_map_node_in_slot(                          \
                    bounded_map_insert_entry.map,                              \
                    bounded_map_insert_entry.entry.type),                      \
                bounded_map_insert_entry.last_order,                           \
                CCC_private_Bounded_map_node_in_slot(                          \
                    bounded_map_insert_entry.map,                              \
                    private_bounded_map_new_ins_base));                        \
        }                                                                      \
    }))

/*==================     Core Macro Implementations     =====================*/

/** @private */
#define CCC_private_bounded_map_and_modify_w(Bounded_map_entry_pointer,        \
                                             type_name, closure_over_T...)     \
    (__extension__({                                                           \
        __auto_type private_bounded_map_ent_pointer                            \
            = (Bounded_map_entry_pointer);                                     \
        struct CCC_Bounded_map_entry private_bounded_map_mod_ent               \
            = {.entry = {.status = CCC_ENTRY_ARGUMENT_ERROR}};                 \
        if (private_bounded_map_ent_pointer)                                   \
        {                                                                      \
            private_bounded_map_mod_ent                                        \
                = private_bounded_map_ent_pointer->private;                    \
            if (private_bounded_map_mod_ent.entry.status & CCC_ENTRY_OCCUPIED) \
            {                                                                  \
                type_name *const T = private_bounded_map_mod_ent.entry.type;   \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_bounded_map_mod_ent;                                           \
    }))

/** @private */
#define CCC_private_bounded_map_or_insert_w(Bounded_map_entry_pointer,         \
                                            lazy_key_value...)                 \
    (__extension__({                                                           \
        __auto_type private_or_ins_entry_pointer                               \
            = (Bounded_map_entry_pointer);                                     \
        typeof(lazy_key_value) *private_bounded_map_or_ins_ret = NULL;         \
        if (private_or_ins_entry_pointer)                                      \
        {                                                                      \
            if (private_or_ins_entry_pointer->private.entry.status             \
                == CCC_ENTRY_OCCUPIED)                                         \
            {                                                                  \
                private_bounded_map_or_ins_ret                                 \
                    = private_or_ins_entry_pointer->private.entry.type;        \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_bounded_map_or_ins_ret = CCC_private_bounded_map_new(  \
                    &private_or_ins_entry_pointer->private);                   \
                CCC_private_bounded_map_insert_key_val(                        \
                    &private_or_ins_entry_pointer->private,                    \
                    private_bounded_map_or_ins_ret, lazy_key_value);           \
            }                                                                  \
        }                                                                      \
        private_bounded_map_or_ins_ret;                                        \
    }))

/** @private */
#define CCC_private_bounded_map_insert_entry_w(Bounded_map_entry_pointer,      \
                                               lazy_key_value...)              \
    (__extension__({                                                           \
        __auto_type private_ins_entry_pointer = (Bounded_map_entry_pointer);   \
        typeof(lazy_key_value) *private_bounded_map_ins_ent_ret = NULL;        \
        if (private_ins_entry_pointer)                                         \
        {                                                                      \
            if (!(private_ins_entry_pointer->private.entry.status              \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                private_bounded_map_ins_ent_ret = CCC_private_bounded_map_new( \
                    &private_ins_entry_pointer->private);                      \
                CCC_private_bounded_map_insert_key_val(                        \
                    &private_ins_entry_pointer->private,                       \
                    private_bounded_map_ins_ent_ret, lazy_key_value);          \
            }                                                                  \
            else if (private_ins_entry_pointer->private.entry.status           \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct CCC_Bounded_map_node private_ins_ent_saved              \
                    = *CCC_private_Bounded_map_node_in_slot(                   \
                        private_ins_entry_pointer->private.map,                \
                        private_ins_entry_pointer->private.entry.type);        \
                *((typeof(private_bounded_map_ins_ent_ret))                    \
                      private_ins_entry_pointer->private.entry.type)           \
                    = lazy_key_value;                                          \
                *CCC_private_Bounded_map_node_in_slot(                         \
                    private_ins_entry_pointer->private.map,                    \
                    private_ins_entry_pointer->private.entry.type)             \
                    = private_ins_ent_saved;                                   \
                private_bounded_map_ins_ent_ret                                \
                    = private_ins_entry_pointer->private.entry.type;           \
            }                                                                  \
        }                                                                      \
        private_bounded_map_ins_ent_ret;                                       \
    }))

/** @private */
#define CCC_private_bounded_map_try_insert_w(Bounded_map_pointer, key,         \
                                             type_compound_literal...)         \
    (__extension__({                                                           \
        struct CCC_Bounded_map *const private_try_ins_map_pointer              \
            = (Bounded_map_pointer);                                           \
        struct CCC_Entry private_bounded_map_try_ins_ent_ret                   \
            = {.status = CCC_ENTRY_ARGUMENT_ERROR};                            \
        if (private_try_ins_map_pointer)                                       \
        {                                                                      \
            __auto_type private_bounded_map_key = (key);                       \
            struct CCC_Bounded_map_entry private_bounded_map_try_ins_ent       \
                = CCC_private_bounded_map_entry(                               \
                    private_try_ins_map_pointer,                               \
                    (void *)&private_bounded_map_key);                         \
            if (!(private_bounded_map_try_ins_ent.entry.status                 \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                CCC_private_bounded_map_insert_and_copy_key(                   \
                    private_bounded_map_try_ins_ent,                           \
                    private_bounded_map_try_ins_ent_ret,                       \
                    private_bounded_map_key, type_compound_literal);           \
            }                                                                  \
            else if (private_bounded_map_try_ins_ent.entry.status              \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                private_bounded_map_try_ins_ent_ret                            \
                    = private_bounded_map_try_ins_ent.entry;                   \
            }                                                                  \
        }                                                                      \
        private_bounded_map_try_ins_ent_ret;                                   \
    }))

/** @private */
#define CCC_private_bounded_map_insert_or_assign_w(Bounded_map_pointer, key,   \
                                                   type_compound_literal...)   \
    (__extension__({                                                           \
        struct CCC_Bounded_map *const private_ins_or_assign_map_pointer        \
            = (Bounded_map_pointer);                                           \
        struct CCC_Entry private_bounded_map_ins_or_assign_ent_ret             \
            = {.status = CCC_ENTRY_ARGUMENT_ERROR};                            \
        if (private_ins_or_assign_map_pointer)                                 \
        {                                                                      \
            __auto_type private_bounded_map_key = (key);                       \
            struct CCC_Bounded_map_entry private_bounded_map_ins_or_assign_ent \
                = CCC_private_bounded_map_entry(                               \
                    private_ins_or_assign_map_pointer,                         \
                    (void *)&private_bounded_map_key);                         \
            if (!(private_bounded_map_ins_or_assign_ent.entry.status           \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                CCC_private_bounded_map_insert_and_copy_key(                   \
                    private_bounded_map_ins_or_assign_ent,                     \
                    private_bounded_map_ins_or_assign_ent_ret,                 \
                    private_bounded_map_key, type_compound_literal);           \
            }                                                                  \
            else if (private_bounded_map_ins_or_assign_ent.entry.status        \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct CCC_Bounded_map_node private_ins_ent_saved              \
                    = *CCC_private_Bounded_map_node_in_slot(                   \
                        private_bounded_map_ins_or_assign_ent.map,             \
                        private_bounded_map_ins_or_assign_ent.entry.type);     \
                *((typeof(type_compound_literal) *)                            \
                      private_bounded_map_ins_or_assign_ent.entry.type)        \
                    = type_compound_literal;                                   \
                *CCC_private_Bounded_map_node_in_slot(                         \
                    private_bounded_map_ins_or_assign_ent.map,                 \
                    private_bounded_map_ins_or_assign_ent.entry.type)          \
                    = private_ins_ent_saved;                                   \
                private_bounded_map_ins_or_assign_ent_ret                      \
                    = private_bounded_map_ins_or_assign_ent.entry;             \
                *((typeof(private_bounded_map_key) *)                          \
                      CCC_private_bounded_map_key_in_slot(                     \
                          private_bounded_map_ins_or_assign_ent.map,           \
                          private_bounded_map_ins_or_assign_ent_ret.type))     \
                    = private_bounded_map_key;                                 \
            }                                                                  \
        }                                                                      \
        private_bounded_map_ins_or_assign_ent_ret;                             \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_PRIVATE_REALTIME__ADAPTIVE_MAP_H */
