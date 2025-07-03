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

/** @private A WAVL node follows traditional balanced binary tree constructs
except for the rank field which can be simplified to an even/odd parity. */
struct ccc_romap_elem
{
    /** @private Children in an array to unite left and right cases. */
    struct ccc_romap_elem *branch[2];
    /** @private The parent node needed for iteration and rotation. */
    struct ccc_romap_elem *parent;
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
struct ccc_romap
{
    /** @private The root of the tree or the sentinel end if empty. */
    struct ccc_romap_elem *root;
    /** @private The end sentinel in the struct for fewer code branches. */
    struct ccc_romap_elem end;
    /** @private The count of stored nodes in the tree. */
    size_t count;
    /** @private The byte offset of the key in the user struct. */
    size_t key_offset;
    /** @private The byte offset of the intrusive element in the user struct. */
    size_t node_elem_offset;
    /** @private The size of the user struct holding the intruder. */
    size_t sizeof_type;
    /** @private An allocation function, if any. */
    ccc_any_alloc_fn *alloc;
    /** @private The comparison function for three way comparison. */
    ccc_any_key_cmp_fn *cmp;
    /** @private Auxiliary data, if any. */
    void *aux;
};

/** @private An entry is a way to store a node or the information needed to
insert a node without a second query. The user can then take different actions
depending on the Occupied or Vacant status of the entry. */
struct ccc_rtree_entry
{
    /** @private The tree associated with this query. */
    struct ccc_romap *rom;
    /** @private The result of the last comparison to find the user specified
    node. Equal if found or indicates the direction the node should be
    inserted from the parent we currently store in the entry. */
    ccc_threeway_cmp last_cmp;
    /** @private The stored node or it's parent if it does not exist. */
    struct ccc_ent entry;
};

/** @private Enable return by compound literal reference on the stack. Think
of this method as return by value but with the additional ability to pass by
pointer in a functional style. `fnB(&(union ccc_romap_entry){fnA().impl});` */
union ccc_romap_entry
{
    /** @private The field containing the entry struct. */
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
        .count = 0,                                                            \
        .key_offset = offsetof(impl_struct_name, impl_key_elem_field),         \
        .node_elem_offset = offsetof(impl_struct_name, impl_node_elem_field),  \
        .sizeof_type = sizeof(impl_struct_name),                               \
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
                      ->rom->alloc(                                            \
                          NULL,                                                \
                          (realtime_ordered_map_entry)->rom->sizeof_type,      \
                          (realtime_ordered_map_entry)->rom->aux);             \
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
            if (impl_or_ins_entry_ptr->impl.entry.stats == CCC_ENTRY_OCCUPIED) \
            {                                                                  \
                impl_rom_or_ins_ret = impl_or_ins_entry_ptr->impl.entry.e;     \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_rom_or_ins_ret                                            \
                    = ccc_impl_rom_new(&impl_or_ins_entry_ptr->impl);          \
                ccc_impl_rom_insert_key_val(&impl_or_ins_entry_ptr->impl,      \
                                            impl_rom_or_ins_ret,               \
                                            lazy_key_value);                   \
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
            if (!(impl_ins_entry_ptr->impl.entry.stats & CCC_ENTRY_OCCUPIED))  \
            {                                                                  \
                impl_rom_ins_ent_ret                                           \
                    = ccc_impl_rom_new(&impl_ins_entry_ptr->impl);             \
                ccc_impl_rom_insert_key_val(&impl_ins_entry_ptr->impl,         \
                                            impl_rom_ins_ent_ret,              \
                                            lazy_key_value);                   \
            }                                                                  \
            else if (impl_ins_entry_ptr->impl.entry.stats                      \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct ccc_romap_elem impl_ins_ent_saved                       \
                    = *ccc_impl_romap_elem_in_slot(                            \
                        impl_ins_entry_ptr->impl.rom,                          \
                        impl_ins_entry_ptr->impl.entry.e);                     \
                *((typeof(impl_rom_ins_ent_ret))                               \
                      impl_ins_entry_ptr->impl.entry.e)                        \
                    = lazy_key_value;                                          \
                *ccc_impl_romap_elem_in_slot(impl_ins_entry_ptr->impl.rom,     \
                                             impl_ins_entry_ptr->impl.entry.e) \
                    = impl_ins_ent_saved;                                      \
                impl_rom_ins_ent_ret = impl_ins_entry_ptr->impl.entry.e;       \
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
