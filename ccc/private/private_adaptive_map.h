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
#ifndef CCC_PRIVATE_ADAPTIVE_MAP_H
#define CCC_PRIVATE_ADAPTIVE_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../types.h"
#include "private_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private An ordered map element in a splay tree requires no special fields.
In fact the parent could be eliminated, but it is important in providing clean
iterative traversals with the begin, end, next abstraction for the user. */
struct CCC_Adaptive_map_node
{
    /** @private The child nodes in a array unite left and right cases. */
    struct CCC_Adaptive_map_node *branch[2];
    /** @private The parent is useful for iteration. Not required for splay. */
    struct CCC_Adaptive_map_node *parent;
};

/** @private Runs the top down splay tree algorithm over a node based tree. A
Splay Tree offers amortized `O(log(N))` because it is a self-optimizing
structure that operates on assumptions about usage patterns. Often, these
assumptions result in frequently accessed elements remaining a constant distance
from the root for O(1) access. However, anti-patterns can arise that harm
performance. The user should carefully consider if their data access pattern
can benefit from a skewed distribution before choosing this container. */
struct CCC_Adaptive_map
{
    /** @private The root of the splay tree. The "hot" node after a query. */
    struct CCC_Adaptive_map_node *root;
    /** @private The sentinel used to eliminate branches. */
    struct CCC_Adaptive_map_node end;
    /** @private The number of stored tree nodes. */
    size_t size;
    /** @private The size of the user type stored in the tree. */
    size_t sizeof_type;
    /** @private The byte offset of the intrusive element. */
    size_t node_node_offset;
    /** @private The byte offset of the user key in the user type. */
    size_t key_offset;
    /** @private The user defined comparison callback function. */
    CCC_Key_comparator *compare;
    /** @private The user defined allocation function, if any. */
    CCC_Allocator *allocate;
    /** @private Auxiliary data, if any. */
    void *context;
};

/** @private An entry is a way to store a node or the information needed to
insert a node without a second query. The user can then take different actions
depending on the Occupied or Vacant status of the entry.

Unlike all the other data structure that offer the entry abstraction the ordered
map does not need to store any special information for a more efficient second
query. The element, or its closest match, is splayed to the root upon each
query. If the user proceeds to insert a new element a second query will result
in a constant time operation to make the new element the new root. If
intervening operations take place between obtaining an entry and inserting the
new element, the best fit will still be close to the root and splaying it again
and then inserting this new element will not be too expensive. Intervening
operations unrelated this entry would also be considered an anti pattern of the
Entry API. */
struct CCC_Adaptive_map_entry
{
    /** @private The tree associated with this query. */
    struct CCC_Adaptive_map *map;
    /** @private The stored node or empty if not found. */
    struct CCC_Entry entry;
};

/** @private Enable return by compound literal reference on the stack. Think
of this method as return by value but with the additional ability to pass by
pointer in a functional style. `fnB(&(union
CCC_Adaptive_map_entry){fnA().private});` */
union CCC_Adaptive_map_entry_wrap
{
    /** @private The field containing the entry struct. */
    struct CCC_Adaptive_map_entry private;
};

/*==========================  Private Interface  ============================*/

/** @private */
void *CCC_private_adaptive_map_key_in_slot(struct CCC_Adaptive_map const *,
                                           void const *slot);
/** @private */
struct CCC_Adaptive_map_node *
CCC_private_Adaptive_map_node_in_slot(struct CCC_Adaptive_map const *,
                                      void const *slot);
/** @private */
struct CCC_Adaptive_map_entry
CCC_private_adaptive_map_entry(struct CCC_Adaptive_map *, void const *key);
/** @private */
void *CCC_private_adaptive_map_insert(struct CCC_Adaptive_map *,
                                      struct CCC_Adaptive_map_node *);

/*======================   Macro Implementations     ========================*/

/** @private */
#define CCC_private_adaptive_map_initialize(                                   \
    private_tree_name, private_struct_name, private_node_node_field,           \
    private_key_node_field, private_key_comparator, private_allocate,          \
    private_context_data)                                                      \
    {                                                                          \
        .root = &(private_tree_name).end,                                      \
        .end                                                                   \
        = {.branch = {&(private_tree_name).end, &(private_tree_name).end},     \
           .parent = &(private_tree_name).end},                                \
        .allocate = (private_allocate),                                        \
        .compare = (private_key_comparator),                                   \
        .context = (private_context_data),                                     \
        .size = 0,                                                             \
        .sizeof_type = sizeof(private_struct_name),                            \
        .node_node_offset                                                      \
        = offsetof(private_struct_name, private_node_node_field),              \
        .key_offset = offsetof(private_struct_name, private_key_node_field),   \
    }

/** @private */
#define CCC_private_adaptive_map_new(adaptive_map_entry)                       \
    (__extension__({                                                           \
        void *private_adaptive_map_ins_allocate_ret = NULL;                    \
        if ((adaptive_map_entry)->map->allocate)                               \
        {                                                                      \
            private_adaptive_map_ins_allocate_ret                              \
                = (adaptive_map_entry)                                         \
                      ->map->allocate((CCC_Allocator_context){                 \
                          .input = NULL,                                       \
                          .bytes = (adaptive_map_entry)->map->sizeof_type,     \
                          .context = (adaptive_map_entry)->map->context,       \
                      });                                                      \
        }                                                                      \
        private_adaptive_map_ins_allocate_ret;                                 \
    }))

/** @private */
#define CCC_private_adaptive_map_insert_key_val(adaptive_map_entry, new_mem,   \
                                                lazy_key_value...)             \
    (__extension__({                                                           \
        if (new_mem)                                                           \
        {                                                                      \
            *new_mem = lazy_key_value;                                         \
            new_mem = CCC_private_adaptive_map_insert(                         \
                (adaptive_map_entry)->map,                                     \
                CCC_private_Adaptive_map_node_in_slot(                         \
                    (adaptive_map_entry)->map, new_mem));                      \
        }                                                                      \
    }))

/** @private */
#define CCC_private_adaptive_map_insert_and_copy_key(                          \
    om_insert_entry, om_insert_entry_ret, key, lazy_value...)                  \
    (__extension__({                                                           \
        typeof(lazy_value) *private_adaptive_map_new_ins_base                  \
            = CCC_private_adaptive_map_new((&om_insert_entry));                \
        om_insert_entry_ret = (struct CCC_Entry){                              \
            .type = private_adaptive_map_new_ins_base,                         \
            .status = CCC_ENTRY_INSERT_ERROR,                                  \
        };                                                                     \
        if (private_adaptive_map_new_ins_base)                                 \
        {                                                                      \
            *((typeof(lazy_value) *)private_adaptive_map_new_ins_base)         \
                = lazy_value;                                                  \
            *((typeof(key) *)CCC_private_adaptive_map_key_in_slot(             \
                om_insert_entry.map, private_adaptive_map_new_ins_base))       \
                = key;                                                         \
            (void)CCC_private_adaptive_map_insert(                             \
                om_insert_entry.map,                                           \
                CCC_private_Adaptive_map_node_in_slot(                         \
                    om_insert_entry.map, private_adaptive_map_new_ins_base));  \
        }                                                                      \
    }))

/*=====================     Core Macro Implementations     ==================*/

/** @private */
#define CCC_private_adaptive_map_and_modify_w(adaptive_map_entry_pointer,      \
                                              type_name, closure_over_T...)    \
    (__extension__({                                                           \
        __auto_type private_adaptive_map_ent_pointer                           \
            = (adaptive_map_entry_pointer);                                    \
        struct CCC_Adaptive_map_entry private_adaptive_map_mod_ent             \
            = {.entry = {.status = CCC_ENTRY_ARGUMENT_ERROR}};                 \
        if (private_adaptive_map_ent_pointer)                                  \
        {                                                                      \
            private_adaptive_map_mod_ent                                       \
                = private_adaptive_map_ent_pointer->private;                   \
            if (private_adaptive_map_mod_ent.entry.status                      \
                & CCC_ENTRY_OCCUPIED)                                          \
            {                                                                  \
                type_name *const T = private_adaptive_map_mod_ent.entry.type;  \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_adaptive_map_mod_ent;                                          \
    }))

/** @private */
#define CCC_private_adaptive_map_or_insert_w(adaptive_map_entry_pointer,       \
                                             lazy_key_value...)                \
    (__extension__({                                                           \
        __auto_type private_or_ins_entry_pointer                               \
            = (adaptive_map_entry_pointer);                                    \
        typeof(lazy_key_value) *private_or_ins_ret = NULL;                     \
        if (private_or_ins_entry_pointer)                                      \
        {                                                                      \
            if (private_or_ins_entry_pointer->private.entry.status             \
                == CCC_ENTRY_OCCUPIED)                                         \
            {                                                                  \
                private_or_ins_ret                                             \
                    = private_or_ins_entry_pointer->private.entry.type;        \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_or_ins_ret = CCC_private_adaptive_map_new(             \
                    &private_or_ins_entry_pointer->private);                   \
                CCC_private_adaptive_map_insert_key_val(                       \
                    &private_or_ins_entry_pointer->private,                    \
                    private_or_ins_ret, lazy_key_value);                       \
            }                                                                  \
        }                                                                      \
        private_or_ins_ret;                                                    \
    }))

/** @private */
#define CCC_private_adaptive_map_insert_entry_w(adaptive_map_entry_pointer,    \
                                                lazy_key_value...)             \
    (__extension__({                                                           \
        __auto_type private_ins_entry_pointer = (adaptive_map_entry_pointer);  \
        typeof(lazy_key_value) *private_adaptive_map_ins_ent_ret = NULL;       \
        if (private_ins_entry_pointer)                                         \
        {                                                                      \
            if (!(private_ins_entry_pointer->private.entry.status              \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                private_adaptive_map_ins_ent_ret                               \
                    = CCC_private_adaptive_map_new(                            \
                        &private_ins_entry_pointer->private);                  \
                CCC_private_adaptive_map_insert_key_val(                       \
                    &private_ins_entry_pointer->private,                       \
                    private_adaptive_map_ins_ent_ret, lazy_key_value);         \
            }                                                                  \
            else if (private_ins_entry_pointer->private.entry.status           \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct CCC_Adaptive_map_node private_ins_ent_saved             \
                    = *CCC_private_Adaptive_map_node_in_slot(                  \
                        private_ins_entry_pointer->private.map,                \
                        private_ins_entry_pointer->private.entry.type);        \
                *((typeof(lazy_key_value) *)                                   \
                      private_ins_entry_pointer->private.entry.type)           \
                    = lazy_key_value;                                          \
                *CCC_private_Adaptive_map_node_in_slot(                        \
                    private_ins_entry_pointer->private.map,                    \
                    private_ins_entry_pointer->private.entry.type)             \
                    = private_ins_ent_saved;                                   \
                private_adaptive_map_ins_ent_ret                               \
                    = private_ins_entry_pointer->private.entry.type;           \
            }                                                                  \
        }                                                                      \
        private_adaptive_map_ins_ent_ret;                                      \
    }))

/** @private */
#define CCC_private_adaptive_map_try_insert_w(adaptive_map_pointer, key,       \
                                              lazy_value...)                   \
    (__extension__({                                                           \
        __auto_type private_try_ins_map_pointer = (adaptive_map_pointer);      \
        struct CCC_Entry private_adaptive_map_try_ins_ent_ret                  \
            = {.status = CCC_ENTRY_ARGUMENT_ERROR};                            \
        if (private_try_ins_map_pointer)                                       \
        {                                                                      \
            __auto_type private_adaptive_map_key = (key);                      \
            struct CCC_Adaptive_map_entry private_adaptive_map_try_ins_ent     \
                = CCC_private_adaptive_map_entry(                              \
                    private_try_ins_map_pointer,                               \
                    (void *)&private_adaptive_map_key);                        \
            if (!(private_adaptive_map_try_ins_ent.entry.status                \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                CCC_private_adaptive_map_insert_and_copy_key(                  \
                    private_adaptive_map_try_ins_ent,                          \
                    private_adaptive_map_try_ins_ent_ret,                      \
                    private_adaptive_map_key, lazy_value);                     \
            }                                                                  \
            else if (private_adaptive_map_try_ins_ent.entry.status             \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                private_adaptive_map_try_ins_ent_ret                           \
                    = private_adaptive_map_try_ins_ent.entry;                  \
            }                                                                  \
        }                                                                      \
        private_adaptive_map_try_ins_ent_ret;                                  \
    }))

/** @private */
#define CCC_private_adaptive_map_insert_or_assign_w(adaptive_map_pointer, key, \
                                                    lazy_value...)             \
    (__extension__({                                                           \
        __auto_type private_ins_or_assign_map_pointer                          \
            = (adaptive_map_pointer);                                          \
        struct CCC_Entry private_adaptive_map_ins_or_assign_ent_ret            \
            = {.status = CCC_ENTRY_ARGUMENT_ERROR};                            \
        if (private_ins_or_assign_map_pointer)                                 \
        {                                                                      \
            __auto_type private_adaptive_map_key = (key);                      \
            struct CCC_Adaptive_map_entry                                      \
                private_adaptive_map_ins_or_assign_ent                         \
                = CCC_private_adaptive_map_entry(                              \
                    private_ins_or_assign_map_pointer,                         \
                    (void *)&private_adaptive_map_key);                        \
            if (!(private_adaptive_map_ins_or_assign_ent.entry.status          \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                CCC_private_adaptive_map_insert_and_copy_key(                  \
                    private_adaptive_map_ins_or_assign_ent,                    \
                    private_adaptive_map_ins_or_assign_ent_ret,                \
                    private_adaptive_map_key, lazy_value);                     \
            }                                                                  \
            else if (private_adaptive_map_ins_or_assign_ent.entry.status       \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct CCC_Adaptive_map_node private_ins_ent_saved             \
                    = *CCC_private_Adaptive_map_node_in_slot(                  \
                        private_adaptive_map_ins_or_assign_ent.map,            \
                        private_adaptive_map_ins_or_assign_ent.entry.type);    \
                *((typeof(lazy_value) *)                                       \
                      private_adaptive_map_ins_or_assign_ent.entry.type)       \
                    = lazy_value;                                              \
                *CCC_private_Adaptive_map_node_in_slot(                        \
                    private_adaptive_map_ins_or_assign_ent.map,                \
                    private_adaptive_map_ins_or_assign_ent.entry.type)         \
                    = private_ins_ent_saved;                                   \
                private_adaptive_map_ins_or_assign_ent_ret                     \
                    = private_adaptive_map_ins_or_assign_ent.entry;            \
                *((typeof(private_adaptive_map_key) *)                         \
                      CCC_private_adaptive_map_key_in_slot(                    \
                          private_ins_or_assign_map_pointer,                   \
                          private_adaptive_map_ins_or_assign_ent_ret.type))    \
                    = private_adaptive_map_key;                                \
            }                                                                  \
        }                                                                      \
        private_adaptive_map_ins_or_assign_ent_ret;                            \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_PRIVATE_ADAPTIVE_MAP_H */
