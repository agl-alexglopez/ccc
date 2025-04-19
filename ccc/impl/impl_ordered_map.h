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
#ifndef CCC_IMPL_ORDERED_MAP_H
#define CCC_IMPL_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../types.h"
#include "impl_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private */
struct ccc_omap_elem
{
    struct ccc_omap_elem *branch[2];
    struct ccc_omap_elem *parent;
};

/** @private */
struct ccc_omap
{
    struct ccc_omap_elem *root;
    struct ccc_omap_elem end;
    ccc_any_alloc_fn *alloc;
    ccc_any_key_cmp_fn *cmp;
    void *aux;
    size_t size;
    size_t sizeof_type;
    size_t node_elem_offset;
    size_t key_offset;
};

/** @private */
struct ccc_otree_entry
{
    struct ccc_omap *t;
    struct ccc_ent entry;
};

/** @private */
union ccc_omap_entry
{
    struct ccc_otree_entry impl;
};

/*==========================  Private Interface  ============================*/

/** @private */
void *ccc_impl_om_key_in_slot(struct ccc_omap const *t, void const *slot);
/** @private */
struct ccc_omap_elem *ccc_impl_omap_elem_in_slot(struct ccc_omap const *t,
                                                 void const *slot);
/** @private */
struct ccc_otree_entry ccc_impl_om_entry(struct ccc_omap *t, void const *key);
/** @private */
void *ccc_impl_om_insert(struct ccc_omap *t, struct ccc_omap_elem *n);

/*======================   Macro Implementations     ========================*/

/** @private */
#define ccc_impl_om_init(impl_tree_name, impl_struct_name,                     \
                         impl_node_elem_field, impl_key_elem_field,            \
                         impl_key_cmp_fn, impl_alloc_fn, impl_aux_data)        \
    {                                                                          \
        .root = &(impl_tree_name).end,                                         \
        .end = {.branch = {&(impl_tree_name).end, &(impl_tree_name).end},      \
                .parent = &(impl_tree_name).end},                              \
        .alloc = (impl_alloc_fn),                                              \
        .cmp = (impl_key_cmp_fn),                                              \
        .aux = (impl_aux_data),                                                \
        .size = 0,                                                             \
        .sizeof_type = sizeof(impl_struct_name),                               \
        .node_elem_offset = offsetof(impl_struct_name, impl_node_elem_field),  \
        .key_offset = offsetof(impl_struct_name, impl_key_elem_field),         \
    }

/** @private */
#define ccc_impl_om_new(ordered_map_entry)                                     \
    (__extension__({                                                           \
        void *impl_om_ins_alloc_ret = NULL;                                    \
        if ((ordered_map_entry)->t->alloc)                                     \
        {                                                                      \
            impl_om_ins_alloc_ret                                              \
                = (ordered_map_entry)                                          \
                      ->t->alloc(NULL, (ordered_map_entry)->t->sizeof_type,    \
                                 (ordered_map_entry)->t->aux);                 \
        }                                                                      \
        impl_om_ins_alloc_ret;                                                 \
    }))

/** @private */
#define ccc_impl_om_insert_key_val(ordered_map_entry, new_mem,                 \
                                   lazy_key_value...)                          \
    (__extension__({                                                           \
        if (new_mem)                                                           \
        {                                                                      \
            *new_mem = lazy_key_value;                                         \
            new_mem = ccc_impl_om_insert(                                      \
                (ordered_map_entry)->t,                                        \
                ccc_impl_omap_elem_in_slot((ordered_map_entry)->t, new_mem));  \
        }                                                                      \
    }))

/** @private */
#define ccc_impl_om_insert_and_copy_key(om_insert_entry, om_insert_entry_ret,  \
                                        key, lazy_value...)                    \
    (__extension__({                                                           \
        typeof(lazy_value) *impl_om_new_ins_base                               \
            = ccc_impl_om_new((&om_insert_entry));                             \
        om_insert_entry_ret = (struct ccc_ent){                                \
            .e = impl_om_new_ins_base,                                         \
            .stats = CCC_ENTRY_INSERT_ERROR,                                   \
        };                                                                     \
        if (impl_om_new_ins_base)                                              \
        {                                                                      \
            *((typeof(lazy_value) *)impl_om_new_ins_base) = lazy_value;        \
            *((typeof(key) *)ccc_impl_om_key_in_slot(om_insert_entry.t,        \
                                                     impl_om_new_ins_base))    \
                = key;                                                         \
            (void)ccc_impl_om_insert(                                          \
                om_insert_entry.t,                                             \
                ccc_impl_omap_elem_in_slot(om_insert_entry.t,                  \
                                           impl_om_new_ins_base));             \
        }                                                                      \
    }))

/*=====================     Core Macro Implementations     ==================*/

/** @private */
#define ccc_impl_om_and_modify_w(ordered_map_entry_ptr, type_name,             \
                                 closure_over_T...)                            \
    (__extension__({                                                           \
        __auto_type impl_om_ent_ptr = (ordered_map_entry_ptr);                 \
        struct ccc_otree_entry impl_om_mod_ent                                 \
            = {.entry = {.stats = CCC_ENTRY_ARG_ERROR}};                       \
        if (impl_om_ent_ptr)                                                   \
        {                                                                      \
            impl_om_mod_ent = impl_om_ent_ptr->impl;                           \
            if (impl_om_mod_ent.entry.stats & CCC_ENTRY_OCCUPIED)              \
            {                                                                  \
                type_name *const T = impl_om_mod_ent.entry.e;                  \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_om_mod_ent;                                                       \
    }))

/** @private */
#define ccc_impl_om_or_insert_w(ordered_map_entry_ptr, lazy_key_value...)      \
    (__extension__({                                                           \
        __auto_type impl_or_ins_entry_ptr = (ordered_map_entry_ptr);           \
        typeof(lazy_key_value) *impl_or_ins_ret = NULL;                        \
        if (impl_or_ins_entry_ptr)                                             \
        {                                                                      \
            struct ccc_otree_entry *impl_om_or_ins_ent                         \
                = &impl_or_ins_entry_ptr->impl;                                \
            if (impl_om_or_ins_ent->entry.stats == CCC_ENTRY_OCCUPIED)         \
            {                                                                  \
                impl_or_ins_ret = impl_om_or_ins_ent->entry.e;                 \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_or_ins_ret = ccc_impl_om_new(impl_om_or_ins_ent);         \
                ccc_impl_om_insert_key_val(impl_om_or_ins_ent,                 \
                                           impl_or_ins_ret, lazy_key_value);   \
            }                                                                  \
        }                                                                      \
        impl_or_ins_ret;                                                       \
    }))

/** @private */
#define ccc_impl_om_insert_entry_w(ordered_map_entry_ptr, lazy_key_value...)   \
    (__extension__({                                                           \
        __auto_type impl_ins_entry_ptr = (ordered_map_entry_ptr);              \
        typeof(lazy_key_value) *impl_om_ins_ent_ret = NULL;                    \
        if (impl_ins_entry_ptr)                                                \
        {                                                                      \
            struct ccc_otree_entry *impl_om_ins_ent                            \
                = &impl_ins_entry_ptr->impl;                                   \
            if (!(impl_om_ins_ent->entry.stats & CCC_ENTRY_OCCUPIED))          \
            {                                                                  \
                impl_om_ins_ent_ret = ccc_impl_om_new(impl_om_ins_ent);        \
                ccc_impl_om_insert_key_val(                                    \
                    impl_om_ins_ent, impl_om_ins_ent_ret, lazy_key_value);     \
            }                                                                  \
            else if (impl_om_ins_ent->entry.stats == CCC_ENTRY_OCCUPIED)       \
            {                                                                  \
                struct ccc_omap_elem impl_ins_ent_saved                        \
                    = *ccc_impl_omap_elem_in_slot(impl_om_ins_ent->t,          \
                                                  impl_om_ins_ent->entry.e);   \
                *((typeof(lazy_key_value) *)impl_om_ins_ent->entry.e)          \
                    = lazy_key_value;                                          \
                *ccc_impl_omap_elem_in_slot(impl_om_ins_ent->t,                \
                                            impl_om_ins_ent->entry.e)          \
                    = impl_ins_ent_saved;                                      \
                impl_om_ins_ent_ret = impl_om_ins_ent->entry.e;                \
            }                                                                  \
        }                                                                      \
        impl_om_ins_ent_ret;                                                   \
    }))

/** @private */
#define ccc_impl_om_try_insert_w(ordered_map_ptr, key, lazy_value...)          \
    (__extension__({                                                           \
        __auto_type impl_try_ins_map_ptr = (ordered_map_ptr);                  \
        struct ccc_ent impl_om_try_ins_ent_ret                                 \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_try_ins_map_ptr)                                              \
        {                                                                      \
            __auto_type impl_om_key = (key);                                   \
            struct ccc_otree_entry impl_om_try_ins_ent = ccc_impl_om_entry(    \
                impl_try_ins_map_ptr, (void *)&impl_om_key);                   \
            if (!(impl_om_try_ins_ent.entry.stats & CCC_ENTRY_OCCUPIED))       \
            {                                                                  \
                ccc_impl_om_insert_and_copy_key(impl_om_try_ins_ent,           \
                                                impl_om_try_ins_ent_ret,       \
                                                impl_om_key, lazy_value);      \
            }                                                                  \
            else if (impl_om_try_ins_ent.entry.stats == CCC_ENTRY_OCCUPIED)    \
            {                                                                  \
                impl_om_try_ins_ent_ret = impl_om_try_ins_ent.entry;           \
            }                                                                  \
        }                                                                      \
        impl_om_try_ins_ent_ret;                                               \
    }))

/** @private */
#define ccc_impl_om_insert_or_assign_w(ordered_map_ptr, key, lazy_value...)    \
    (__extension__({                                                           \
        __auto_type impl_ins_or_assign_map_ptr = (ordered_map_ptr);            \
        struct ccc_ent impl_om_ins_or_assign_ent_ret                           \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_ins_or_assign_map_ptr)                                        \
        {                                                                      \
            __auto_type impl_om_key = (key);                                   \
            struct ccc_otree_entry impl_om_ins_or_assign_ent                   \
                = ccc_impl_om_entry(impl_ins_or_assign_map_ptr,                \
                                    (void *)&impl_om_key);                     \
            if (!(impl_om_ins_or_assign_ent.entry.stats & CCC_ENTRY_OCCUPIED)) \
            {                                                                  \
                ccc_impl_om_insert_and_copy_key(impl_om_ins_or_assign_ent,     \
                                                impl_om_ins_or_assign_ent_ret, \
                                                impl_om_key, lazy_value);      \
            }                                                                  \
            else if (impl_om_ins_or_assign_ent.entry.stats                     \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct ccc_omap_elem impl_ins_ent_saved                        \
                    = *ccc_impl_omap_elem_in_slot(                             \
                        impl_om_ins_or_assign_ent.t,                           \
                        impl_om_ins_or_assign_ent.entry.e);                    \
                *((typeof(lazy_value) *)impl_om_ins_or_assign_ent.entry.e)     \
                    = lazy_value;                                              \
                *ccc_impl_omap_elem_in_slot(impl_om_ins_or_assign_ent.t,       \
                                            impl_om_ins_or_assign_ent.entry.e) \
                    = impl_ins_ent_saved;                                      \
                impl_om_ins_or_assign_ent_ret                                  \
                    = impl_om_ins_or_assign_ent.entry;                         \
                *((typeof(impl_om_key) *)ccc_impl_om_key_in_slot(              \
                    impl_ins_or_assign_map_ptr,                                \
                    impl_om_ins_or_assign_ent_ret.e))                          \
                    = impl_om_key;                                             \
            }                                                                  \
        }                                                                      \
        impl_om_ins_or_assign_ent_ret;                                         \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_ORDERED_MAP_H */
