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
#ifndef CCC_IMPL_ORDERED_MULTIMAP_H
#define CCC_IMPL_ORDERED_MULTIMAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../types.h"
#include "impl_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private This node type will support more expressive implementations of a
multimap. The array of pointers is to support unifying left and right symmetric
cases. The union is only relevant the multimap. Duplicate values are placed in a
circular doubly linked list. The head of this doubly linked list is the oldest
duplicate (aka round robin). The node still in the tree then sacrifices its
parent field to track this head of all duplicates. The duplicate then uses its
parent field to track the parent of the node in the tree. Duplicates can be
detected by detecting a cycle in the tree (graph) that only uses child/branch
pointers, which is normally impossible in a binary tree.

No additional flags or bits are needed. Splay trees, the current underlying
implementation, do not support duplicate values by default and I have found
trimming the tree with these "fat" tree nodes holding duplicates can net a nice
performance boost in the pop operation for the multimap. This works thanks to
type punning in C combined with the types in the union having the same size so
it's safe. This would be undefined in C++. */
struct ccc_ommap_elem
{
    /** @private */
    union
    {
        /** @private The child nodes in a array unite left and right cases. */
        struct ccc_ommap_elem *branch[2];
        /** @private The next, previous link in the duplicate list. */
        struct ccc_ommap_elem *link[2];
    };
    /** @private */
    union
    {
        /** @private The parent is useful for iteration. */
        struct ccc_ommap_elem *parent;
        /** @private The sacrificed pointer to track duplicates. */
        struct ccc_ommap_elem *dup_head;
    };
};

/** @private Runs the top down splay tree algorithm over a node based tree. A
Splay Tree offers amortized `O(log(N))` because it is a self-optimizing
structure that operates on assumptions about usage patterns. Often, these
assumptions result in frequently accessed elements remaining a constant distance
from the root for O(1) access. However, anti-patterns can arise that harm
performance. The user should carefully consider if their data access pattern
can benefit from a skewed distribution before choosing this container. */
struct ccc_ommap
{
    /** @private The root of the splay tree. The "hot" node after a query. */
    struct ccc_ommap_elem *root;
    /** @private The sentinel used to eliminate branches. */
    struct ccc_ommap_elem end;
    /** @private The number of stored tree nodes. */
    size_t size;
    /** @private The size of the user type stored in the tree. */
    size_t sizeof_type;
    /** @private The byte offset of the intrusive element. */
    size_t node_elem_offset;
    /** @private The byte offset of the user key in the user type. */
    size_t key_offset;
    /** @private The user defined comparison callback function. */
    ccc_any_key_cmp_fn *cmp;
    /** @private The user defined allocation function, if any. */
    ccc_any_alloc_fn *alloc;
    /** @private Auxiliary data, if any. */
    void *aux;
};

/** @private An entry is a way to store a node or the information needed to
insert a node without a second query. The user can then take different actions
depending on the Occupied or Vacant status of the entry.

Unlike all the other data structure that offer the entry abstraction the ordered
multimap does not need to store any special information for a more efficient
second query. The element, or its closest match, is splayed to the root upon
each query. If the user proceeds to insert a new element a second query will
result in a constant time operation to make the new element the new root. If
intervening operations take place between obtaining an entry and inserting the
new element, the best fit will still be close to the root and splaying it again
and then inserting this new element will not be too expensive. Intervening
operations unrelated this entry would also be considered an anti pattern of the
Entry API. */
struct ccc_omultimap_entry
{
    /** @private */
    struct ccc_ommap *t;
    /** @private */
    struct ccc_ent entry;
};

/** @private Enable return by compound literal reference on the stack. Think
of this method as return by value but with the additional ability to pass by
pointer in a functional style. `fnB(&(union ccc_ommap_entry){fnA().impl});` */
union ccc_ommap_entry
{
    /** @private The field containing the entry struct. */
    struct ccc_omultimap_entry impl;
};

/*==========================  Private Interface  ============================*/

/** @private */
void *ccc_impl_omm_key_in_slot(struct ccc_ommap const *t, void const *slot);
/** @private */
struct ccc_ommap_elem *ccc_impl_ommap_elem_in_slot(struct ccc_ommap const *t,
                                                   void const *slot);
/** @private */
struct ccc_omultimap_entry ccc_impl_omm_entry(struct ccc_ommap *t,
                                              void const *key);
/** @private */
void *ccc_impl_omm_multimap_insert(struct ccc_ommap *t,
                                   struct ccc_ommap_elem *n);

/*==========================   Initialization  ==============================*/

/** @private */
#define ccc_impl_omm_init(impl_tree_name, impl_struct_name,                    \
                          impl_node_elem_field, impl_key_elem_field,           \
                          impl_key_cmp_fn, impl_alloc_fn, impl_aux_data)       \
    {                                                                          \
        .root = &(impl_tree_name).end,                                         \
        .end = {{.branch = {&(impl_tree_name).end, &(impl_tree_name).end}},    \
                {.parent = &(impl_tree_name).end}},                            \
        .alloc = (impl_alloc_fn),                                              \
        .cmp = (impl_key_cmp_fn),                                              \
        .aux = (impl_aux_data),                                                \
        .size = 0,                                                             \
        .sizeof_type = sizeof(impl_struct_name),                               \
        .node_elem_offset = offsetof(impl_struct_name, impl_node_elem_field),  \
        .key_offset = offsetof(impl_struct_name, impl_key_elem_field),         \
    }

/*==================   Helper Macros for Repeated Logic     =================*/

/** @private */
#define ccc_impl_omm_new(ordered_map_entry)                                    \
    (__extension__({                                                           \
        void *impl_omm_ins_alloc_ret = NULL;                                   \
        if ((ordered_map_entry)->t->alloc)                                     \
        {                                                                      \
            impl_omm_ins_alloc_ret                                             \
                = (ordered_map_entry)                                          \
                      ->t->alloc(NULL, (ordered_map_entry)->t->sizeof_type,    \
                                 (ordered_map_entry)->t->aux);                 \
        }                                                                      \
        impl_omm_ins_alloc_ret;                                                \
    }))

/** @private */
#define ccc_impl_omm_insert_key_val(ordered_map_entry, new_mem,                \
                                    lazy_key_value...)                         \
    (__extension__({                                                           \
        if (new_mem)                                                           \
        {                                                                      \
            *new_mem = lazy_key_value;                                         \
            new_mem = ccc_impl_omm_multimap_insert(                            \
                (ordered_map_entry)->t,                                        \
                ccc_impl_ommap_elem_in_slot((ordered_map_entry)->t, new_mem)); \
        }                                                                      \
    }))

/** @private */
#define ccc_impl_omm_insert_and_copy_key(                                      \
    omm_insert_entry, omm_insert_entry_ret, key, lazy_value...)                \
    (__extension__({                                                           \
        typeof(lazy_value) *impl_omm_new_ins_base                              \
            = ccc_impl_omm_new((&omm_insert_entry));                           \
        omm_insert_entry_ret = (struct ccc_ent){                               \
            .e = impl_omm_new_ins_base,                                        \
            .stats = CCC_ENTRY_INSERT_ERROR,                                   \
        };                                                                     \
        if (impl_omm_new_ins_base)                                             \
        {                                                                      \
            *((typeof(lazy_value) *)impl_omm_new_ins_base) = lazy_value;       \
            *((typeof(key) *)ccc_impl_omm_key_in_slot(omm_insert_entry.t,      \
                                                      impl_omm_new_ins_base))  \
                = key;                                                         \
            (void)ccc_impl_omm_multimap_insert(                                \
                omm_insert_entry.t,                                            \
                ccc_impl_ommap_elem_in_slot(omm_insert_entry.t,                \
                                            impl_omm_new_ins_base));           \
        }                                                                      \
    }))

/*=====================     Core Macro Implementations     ==================*/

/** @private */
#define ccc_impl_omm_and_modify_w(ordered_map_entry_ptr, type_name,            \
                                  closure_over_T...)                           \
    (__extension__({                                                           \
        __auto_type impl_omm_ent_ptr = (ordered_map_entry_ptr);                \
        struct ccc_omultimap_entry impl_omm_mod_ent                            \
            = {.entry = {.stats = CCC_ENTRY_ARG_ERROR}};                       \
        if (impl_omm_ent_ptr)                                                  \
        {                                                                      \
            impl_omm_mod_ent = impl_omm_ent_ptr->impl;                         \
            if (impl_omm_mod_ent.entry.stats & CCC_ENTRY_OCCUPIED)             \
            {                                                                  \
                type_name *const T = impl_omm_mod_ent.entry.e;                 \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_omm_mod_ent;                                                      \
    }))

/** @private */
#define ccc_impl_omm_or_insert_w(ordered_map_entry_ptr, lazy_key_value...)     \
    (__extension__({                                                           \
        __auto_type impl_omm_or_ins_ent_ptr = (ordered_map_entry_ptr);         \
        typeof(lazy_key_value) *impl_or_ins_ret = NULL;                        \
        if (impl_omm_or_ins_ent_ptr)                                           \
        {                                                                      \
            struct ccc_omultimap_entry *impl_omm_or_ins_ent                    \
                = &impl_omm_or_ins_ent_ptr->impl;                              \
            if (impl_omm_or_ins_ent->entry.stats == CCC_ENTRY_OCCUPIED)        \
            {                                                                  \
                impl_or_ins_ret = impl_omm_or_ins_ent->entry.e;                \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_or_ins_ret = ccc_impl_omm_new(impl_omm_or_ins_ent);       \
                ccc_impl_omm_insert_key_val(impl_omm_or_ins_ent,               \
                                            impl_or_ins_ret, lazy_key_value);  \
            }                                                                  \
        }                                                                      \
        impl_or_ins_ret;                                                       \
    }))

/** @private */
#define ccc_impl_omm_insert_entry_w(ordered_map_entry_ptr, lazy_key_value...)  \
    (__extension__({                                                           \
        __auto_type impl_omm_ins_ent_ptr = (ordered_map_entry_ptr);            \
        typeof(lazy_key_value) *impl_omm_ins_ent_ret = NULL;                   \
        if (impl_omm_ins_ent_ptr)                                              \
        {                                                                      \
            struct ccc_omultimap_entry *impl_omm_ins_ent                       \
                = &impl_omm_ins_ent_ptr->impl;                                 \
            impl_omm_ins_ent_ret = ccc_impl_omm_new(impl_omm_ins_ent);         \
            ccc_impl_omm_insert_key_val(impl_omm_ins_ent,                      \
                                        impl_omm_ins_ent_ret, lazy_key_value); \
        }                                                                      \
        impl_omm_ins_ent_ret;                                                  \
    }))

/** @private */
#define ccc_impl_omm_try_insert_w(ordered_map_ptr, key, lazy_value...)         \
    (__extension__({                                                           \
        __auto_type impl_omm_try_ins_ptr = (ordered_map_ptr);                  \
        struct ccc_ent impl_omm_try_ins_ent_ret                                \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_omm_try_ins_ptr)                                              \
        {                                                                      \
            __auto_type impl_omm_key = (key);                                  \
            struct ccc_omultimap_entry impl_omm_try_ins_ent                    \
                = ccc_impl_omm_entry(impl_omm_try_ins_ptr,                     \
                                     (void *)&impl_omm_key);                   \
            if (!(impl_omm_try_ins_ent.entry.stats & CCC_ENTRY_OCCUPIED))      \
            {                                                                  \
                ccc_impl_omm_insert_and_copy_key(impl_omm_try_ins_ent,         \
                                                 impl_omm_try_ins_ent_ret,     \
                                                 impl_omm_key, lazy_value);    \
            }                                                                  \
            else if (impl_omm_try_ins_ent.entry.stats == CCC_ENTRY_OCCUPIED)   \
            {                                                                  \
                impl_omm_try_ins_ent_ret = impl_omm_try_ins_ent.entry;         \
            }                                                                  \
        }                                                                      \
        impl_omm_try_ins_ent_ret;                                              \
    }))

/** @private */
#define ccc_impl_omm_insert_or_assign_w(ordered_map_ptr, key, lazy_value...)   \
    (__extension__({                                                           \
        __auto_type impl_omm_ins_or_assign_ptr = (ordered_map_ptr);            \
        struct ccc_ent impl_omm_ins_or_assign_ent_ret                          \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_omm_ins_or_assign_ptr)                                        \
        {                                                                      \
            __auto_type impl_omm_key = (key);                                  \
            struct ccc_omultimap_entry impl_omm_ins_or_assign_ent              \
                = ccc_impl_omm_entry(impl_omm_ins_or_assign_ptr,               \
                                     (void *)&impl_omm_key);                   \
            if (!(impl_omm_ins_or_assign_ent.entry.stats                       \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                ccc_impl_omm_insert_and_copy_key(                              \
                    impl_omm_ins_or_assign_ent,                                \
                    impl_omm_ins_or_assign_ent_ret, impl_omm_key, lazy_value); \
            }                                                                  \
            else if (impl_omm_ins_or_assign_ent.entry.stats                    \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct ccc_ommap_elem impl_ins_ent_saved                       \
                    = *ccc_impl_ommap_elem_in_slot(                            \
                        impl_omm_ins_or_assign_ent.t,                          \
                        impl_omm_ins_or_assign_ent.entry.e);                   \
                *((typeof(lazy_value) *)impl_omm_ins_or_assign_ent.entry.e)    \
                    = lazy_value;                                              \
                *ccc_impl_ommap_elem_in_slot(                                  \
                    impl_omm_ins_or_assign_ent.t,                              \
                    impl_omm_ins_or_assign_ent.entry.e)                        \
                    = impl_ins_ent_saved;                                      \
                impl_omm_ins_or_assign_ent_ret                                 \
                    = impl_omm_ins_or_assign_ent.entry;                        \
                *((typeof(impl_omm_key) *)ccc_impl_omm_key_in_slot(            \
                    impl_omm_ins_or_assign_ptr,                                \
                    impl_omm_ins_or_assign_ent_ret.e))                         \
                    = impl_omm_key;                                            \
            }                                                                  \
        }                                                                      \
        impl_omm_ins_or_assign_ent_ret;                                        \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_ORDERED_MULTIMAP_H */
