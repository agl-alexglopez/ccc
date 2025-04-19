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
#ifndef CCC_IMPL_REALTIME_ORDERED_MAP_H
#define CCC_IMPL_REALTIME_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../types.h"
#include "impl_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private */
struct ccc_romap_elem
{
    struct ccc_romap_elem *branch[2];
    struct ccc_romap_elem *parent;
    uint8_t parity;
};

/** @private */
struct ccc_romap
{
    struct ccc_romap_elem *root;
    struct ccc_romap_elem end;
    size_t sz;
    size_t key_offset;
    size_t node_elem_offset;
    size_t elem_sz;
    ccc_any_alloc_fn *alloc;
    ccc_any_key_cmp_fn *cmp;
    void *aux;
};

/** @private */
struct ccc_rtree_entry
{
    struct ccc_romap *rom;
    ccc_threeway_cmp last_cmp;
    struct ccc_ent entry;
};

/** @private */
union ccc_romap_entry
{
    struct ccc_rtree_entry impl;
};

/*=========================   Private Interface  ============================*/

/** @private */
void *ccc_impl_rom_key_in_slot(struct ccc_romap const *rom, void const *slot);
/** @private */
struct ccc_romap_elem *ccc_impl_romap_elem_in_slot(struct ccc_romap const *rom,
                                                   void const *slot);
/** @private */
struct ccc_rtree_entry ccc_impl_rom_entry(struct ccc_romap const *rom,
                                          void const *key);
/** @private */
void *ccc_impl_rom_insert(struct ccc_romap *rom, struct ccc_romap_elem *parent,
                          ccc_threeway_cmp last_cmp,
                          struct ccc_romap_elem *out_handle);

/*==========================   Initialization     ===========================*/

/** @private */
#define ccc_impl_rom_init(impl_map_name, impl_struct_name,                     \
                          impl_node_elem_field, impl_key_elem_field,           \
                          impl_key_cmp_fn, impl_alloc_fn, impl_aux_data)       \
    {                                                                          \
        .root = &(impl_map_name).end,                                          \
        .end = {.parity = 1,                                                   \
                .parent = &(impl_map_name).end,                                \
                .branch = {&(impl_map_name).end, &(impl_map_name).end}},       \
        .sz = 0,                                                               \
        .key_offset = offsetof(impl_struct_name, impl_key_elem_field),         \
        .node_elem_offset = offsetof(impl_struct_name, impl_node_elem_field),  \
        .elem_sz = sizeof(impl_struct_name),                                   \
        .alloc = (impl_alloc_fn),                                              \
        .cmp = (impl_key_cmp_fn),                                              \
        .aux = (impl_aux_data),                                                \
    }

/*==================   Helper Macros for Repeated Logic     =================*/

/** @private */
#define ccc_impl_rom_new(realtime_ordered_map_entry)                           \
    (__extension__({                                                           \
        void *impl_rom_ins_alloc_ret = NULL;                                   \
        if ((realtime_ordered_map_entry)->rom->alloc)                          \
        {                                                                      \
            impl_rom_ins_alloc_ret                                             \
                = (realtime_ordered_map_entry)                                 \
                      ->rom->alloc(NULL,                                       \
                                   (realtime_ordered_map_entry)->rom->elem_sz, \
                                   (realtime_ordered_map_entry)->rom->aux);    \
        }                                                                      \
        impl_rom_ins_alloc_ret;                                                \
    }))

/** @private */
#define ccc_impl_rom_insert_key_val(realtime_ordered_map_entry, new_mem,       \
                                    lazy_key_value...)                         \
    (__extension__({                                                           \
        if (new_mem)                                                           \
        {                                                                      \
            *new_mem = lazy_key_value;                                         \
            new_mem = ccc_impl_rom_insert(                                     \
                (realtime_ordered_map_entry)->rom,                             \
                ccc_impl_romap_elem_in_slot(                                   \
                    (realtime_ordered_map_entry)->rom,                         \
                    (realtime_ordered_map_entry)->entry.e),                    \
                (realtime_ordered_map_entry)->last_cmp,                        \
                ccc_impl_romap_elem_in_slot((realtime_ordered_map_entry)->rom, \
                                            new_mem));                         \
        }                                                                      \
    }))

/** @private */
#define ccc_impl_rom_insert_and_copy_key(                                      \
    rom_insert_entry, rom_insert_entry_ret, key, lazy_value...)                \
    (__extension__({                                                           \
        typeof(lazy_value) *impl_rom_new_ins_base                              \
            = ccc_impl_rom_new((&rom_insert_entry));                           \
        rom_insert_entry_ret = (struct ccc_ent){                               \
            .e = impl_rom_new_ins_base,                                        \
            .stats = CCC_ENTRY_INSERT_ERROR,                                   \
        };                                                                     \
        if (impl_rom_new_ins_base)                                             \
        {                                                                      \
            *impl_rom_new_ins_base = lazy_value;                               \
            *((typeof(key) *)ccc_impl_rom_key_in_slot(rom_insert_entry.rom,    \
                                                      impl_rom_new_ins_base))  \
                = key;                                                         \
            (void)ccc_impl_rom_insert(                                         \
                rom_insert_entry.rom,                                          \
                ccc_impl_romap_elem_in_slot(rom_insert_entry.rom,              \
                                            rom_insert_entry.entry.e),         \
                rom_insert_entry.last_cmp,                                     \
                ccc_impl_romap_elem_in_slot(rom_insert_entry.rom,              \
                                            impl_rom_new_ins_base));           \
        }                                                                      \
    }))

/*==================     Core Macro Implementations     =====================*/

/** @private */
#define ccc_impl_rom_and_modify_w(realtime_ordered_map_entry_ptr, type_name,   \
                                  closure_over_T...)                           \
    (__extension__({                                                           \
        __auto_type impl_rom_ent_ptr = (realtime_ordered_map_entry_ptr);       \
        struct ccc_rtree_entry impl_rom_mod_ent                                \
            = {.entry = {.stats = CCC_ENTRY_ARG_ERROR}};                       \
        if (impl_rom_ent_ptr)                                                  \
        {                                                                      \
            impl_rom_mod_ent = impl_rom_ent_ptr->impl;                         \
            if (impl_rom_mod_ent.entry.stats & CCC_ENTRY_OCCUPIED)             \
            {                                                                  \
                type_name *const T = impl_rom_mod_ent.entry.e;                 \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_rom_mod_ent;                                                      \
    }))

/** @private */
#define ccc_impl_rom_or_insert_w(realtime_ordered_map_entry_ptr,               \
                                 lazy_key_value...)                            \
    (__extension__({                                                           \
        __auto_type impl_or_ins_entry_ptr = (realtime_ordered_map_entry_ptr);  \
        typeof(lazy_key_value) *impl_rom_or_ins_ret = NULL;                    \
        if (impl_or_ins_entry_ptr)                                             \
        {                                                                      \
            struct ccc_rtree_entry *impl_rom_or_ins_ent                        \
                = &impl_or_ins_entry_ptr->impl;                                \
            if (impl_rom_or_ins_ent->entry.stats == CCC_ENTRY_OCCUPIED)        \
            {                                                                  \
                impl_rom_or_ins_ret = impl_rom_or_ins_ent->entry.e;            \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_rom_or_ins_ret = ccc_impl_rom_new(impl_rom_or_ins_ent);   \
                ccc_impl_rom_insert_key_val(                                   \
                    impl_rom_or_ins_ent, impl_rom_or_ins_ret, lazy_key_value); \
            }                                                                  \
        }                                                                      \
        impl_rom_or_ins_ret;                                                   \
    }))

/** @private */
#define ccc_impl_rom_insert_entry_w(realtime_ordered_map_entry_ptr,            \
                                    lazy_key_value...)                         \
    (__extension__({                                                           \
        __auto_type impl_ins_entry_ptr = (realtime_ordered_map_entry_ptr);     \
        typeof(lazy_key_value) *impl_rom_ins_ent_ret = NULL;                   \
        if (impl_ins_entry_ptr)                                                \
        {                                                                      \
            struct ccc_rtree_entry *impl_rom_ins_ent                           \
                = &impl_ins_entry_ptr->impl;                                   \
            if (!(impl_rom_ins_ent->entry.stats & CCC_ENTRY_OCCUPIED))         \
            {                                                                  \
                impl_rom_ins_ent_ret = ccc_impl_rom_new(impl_rom_ins_ent);     \
                ccc_impl_rom_insert_key_val(                                   \
                    impl_rom_ins_ent, impl_rom_ins_ent_ret, lazy_key_value);   \
            }                                                                  \
            else if (impl_rom_ins_ent->entry.stats == CCC_ENTRY_OCCUPIED)      \
            {                                                                  \
                struct ccc_romap_elem impl_ins_ent_saved                       \
                    = *ccc_impl_romap_elem_in_slot(impl_rom_ins_ent->rom,      \
                                                   impl_rom_ins_ent->entry.e); \
                *((typeof(impl_rom_ins_ent_ret))impl_rom_ins_ent->entry.e)     \
                    = lazy_key_value;                                          \
                *ccc_impl_romap_elem_in_slot(impl_rom_ins_ent->rom,            \
                                             impl_rom_ins_ent->entry.e)        \
                    = impl_ins_ent_saved;                                      \
                impl_rom_ins_ent_ret = impl_rom_ins_ent->entry.e;              \
            }                                                                  \
        }                                                                      \
        impl_rom_ins_ent_ret;                                                  \
    }))

/** @private */
#define ccc_impl_rom_try_insert_w(realtime_ordered_map_ptr, key,               \
                                  lazy_value...)                               \
    (__extension__({                                                           \
        struct ccc_romap *const impl_try_ins_map_ptr                           \
            = (realtime_ordered_map_ptr);                                      \
        struct ccc_ent impl_rom_try_ins_ent_ret                                \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_try_ins_map_ptr)                                              \
        {                                                                      \
            __auto_type impl_rom_key = (key);                                  \
            struct ccc_rtree_entry impl_rom_try_ins_ent = ccc_impl_rom_entry(  \
                impl_try_ins_map_ptr, (void *)&impl_rom_key);                  \
            if (!(impl_rom_try_ins_ent.entry.stats & CCC_ENTRY_OCCUPIED))      \
            {                                                                  \
                ccc_impl_rom_insert_and_copy_key(impl_rom_try_ins_ent,         \
                                                 impl_rom_try_ins_ent_ret,     \
                                                 impl_rom_key, lazy_value);    \
            }                                                                  \
            else if (impl_rom_try_ins_ent.entry.stats == CCC_ENTRY_OCCUPIED)   \
            {                                                                  \
                impl_rom_try_ins_ent_ret = impl_rom_try_ins_ent.entry;         \
            }                                                                  \
        }                                                                      \
        impl_rom_try_ins_ent_ret;                                              \
    }))

/** @private */
#define ccc_impl_rom_insert_or_assign_w(realtime_ordered_map_ptr, key,         \
                                        lazy_value...)                         \
    (__extension__({                                                           \
        struct ccc_romap *const impl_ins_or_assign_map_ptr                     \
            = (realtime_ordered_map_ptr);                                      \
        struct ccc_ent impl_rom_ins_or_assign_ent_ret                          \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_ins_or_assign_map_ptr)                                        \
        {                                                                      \
            __auto_type impl_rom_key = (key);                                  \
            struct ccc_rtree_entry impl_rom_ins_or_assign_ent                  \
                = ccc_impl_rom_entry(impl_ins_or_assign_map_ptr,               \
                                     (void *)&impl_rom_key);                   \
            if (!(impl_rom_ins_or_assign_ent.entry.stats                       \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                ccc_impl_rom_insert_and_copy_key(                              \
                    impl_rom_ins_or_assign_ent,                                \
                    impl_rom_ins_or_assign_ent_ret, impl_rom_key, lazy_value); \
            }                                                                  \
            else if (impl_rom_ins_or_assign_ent.entry.stats                    \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct ccc_romap_elem impl_ins_ent_saved                       \
                    = *ccc_impl_romap_elem_in_slot(                            \
                        impl_rom_ins_or_assign_ent.rom,                        \
                        impl_rom_ins_or_assign_ent.entry.e);                   \
                *((typeof(lazy_value) *)impl_rom_ins_or_assign_ent.entry.e)    \
                    = lazy_value;                                              \
                *ccc_impl_romap_elem_in_slot(                                  \
                    impl_rom_ins_or_assign_ent.rom,                            \
                    impl_rom_ins_or_assign_ent.entry.e)                        \
                    = impl_ins_ent_saved;                                      \
                impl_rom_ins_or_assign_ent_ret                                 \
                    = impl_rom_ins_or_assign_ent.entry;                        \
                *((typeof(impl_rom_key) *)ccc_impl_rom_key_in_slot(            \
                    impl_rom_ins_or_assign_ent.rom,                            \
                    impl_rom_ins_or_assign_ent_ret.e))                         \
                    = impl_rom_key;                                            \
            }                                                                  \
        }                                                                      \
        impl_rom_ins_or_assign_ent_ret;                                        \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_REALTIME__ORDERED_MAP_H */
