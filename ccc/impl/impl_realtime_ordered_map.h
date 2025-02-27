#ifndef CCC_IMPL_REALTIME_ORDERED_MAP_H
#define CCC_IMPL_REALTIME_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../types.h"
#include "impl_types.h"

/** @private */
typedef struct ccc_romap_elem_
{
    struct ccc_romap_elem_ *branch_[2];
    struct ccc_romap_elem_ *parent_;
    uint8_t parity_;
} ccc_romap_elem_;

/** @private */
struct ccc_romap_
{
    struct ccc_romap_elem_ *root_;
    struct ccc_romap_elem_ end_;
    size_t sz_;
    size_t key_offset_;
    size_t node_elem_offset_;
    size_t elem_sz_;
    ccc_alloc_fn *alloc_;
    ccc_key_cmp_fn *cmp_;
    void *aux_;
};

/** @private */
struct ccc_rtree_entry_
{
    struct ccc_romap_ *rom_;
    ccc_threeway_cmp last_cmp_;
    struct ccc_ent_ entry_;
};

/** @private */
union ccc_romap_entry_
{
    struct ccc_rtree_entry_ impl_;
};

#define ccc_impl_rom_init(map_name, struct_name, node_elem_field,              \
                          key_elem_field, key_cmp_fn, alloc_fn, aux_data)      \
    {                                                                          \
        .root_ = &(map_name).end_,                                             \
        .end_ = {.parity_ = 1,                                                 \
                 .parent_ = &(map_name).end_,                                  \
                 .branch_ = {&(map_name).end_, &(map_name).end_}},             \
        .sz_ = 0,                                                              \
        .key_offset_ = offsetof(struct_name, key_elem_field),                  \
        .node_elem_offset_ = offsetof(struct_name, node_elem_field),           \
        .elem_sz_ = sizeof(struct_name),                                       \
        .alloc_ = (alloc_fn),                                                  \
        .cmp_ = (key_cmp_fn),                                                  \
        .aux_ = (aux_data),                                                    \
    }

void *ccc_impl_rom_key_from_node(struct ccc_romap_ const *rom,
                                 struct ccc_romap_elem_ const *elem);
void *ccc_impl_rom_key_in_slot(struct ccc_romap_ const *rom, void const *slot);
struct ccc_romap_elem_ *
ccc_impl_romap_elem_in_slot(struct ccc_romap_ const *rom, void const *slot);
struct ccc_rtree_entry_ ccc_impl_rom_entry(struct ccc_romap_ const *rom,
                                           void const *key);
void *ccc_impl_rom_insert(struct ccc_romap_ *rom,
                          struct ccc_romap_elem_ *parent,
                          ccc_threeway_cmp last_cmp,
                          struct ccc_romap_elem_ *out_handle);

/* NOLINTBEGIN(readability-identifier-naming) */

/*==================   Helper Macros for Repeated Logic     =================*/

#define ccc_impl_rom_new(realtime_ordered_map_entry)                           \
    (__extension__({                                                           \
        void *rom_ins_alloc_ret_ = NULL;                                       \
        if ((realtime_ordered_map_entry)->rom_->alloc_)                        \
        {                                                                      \
            rom_ins_alloc_ret_                                                 \
                = (realtime_ordered_map_entry)                                 \
                      ->rom_->alloc_(                                          \
                          NULL, (realtime_ordered_map_entry)->rom_->elem_sz_,  \
                          (realtime_ordered_map_entry)->rom_->aux_);           \
        }                                                                      \
        rom_ins_alloc_ret_;                                                    \
    }))

#define ccc_impl_rom_insert_key_val(realtime_ordered_map_entry, new_mem,       \
                                    lazy_key_value...)                         \
    (__extension__({                                                           \
        if (new_mem)                                                           \
        {                                                                      \
            *new_mem = lazy_key_value;                                         \
            new_mem = ccc_impl_rom_insert(                                     \
                (realtime_ordered_map_entry)->rom_,                            \
                ccc_impl_romap_elem_in_slot(                                   \
                    (realtime_ordered_map_entry)->rom_,                        \
                    (realtime_ordered_map_entry)->entry_.e_),                  \
                (realtime_ordered_map_entry)->last_cmp_,                       \
                ccc_impl_romap_elem_in_slot(                                   \
                    (realtime_ordered_map_entry)->rom_, new_mem));             \
        }                                                                      \
    }))

#define ccc_impl_rom_insert_and_copy_key(                                      \
    rom_insert_entry, rom_insert_entry_ret, key, lazy_value...)                \
    (__extension__({                                                           \
        typeof(lazy_value) *rom_new_ins_base_                                  \
            = ccc_impl_rom_new((&rom_insert_entry));                           \
        rom_insert_entry_ret = (struct ccc_ent_){                              \
            .e_ = rom_new_ins_base_,                                           \
            .stats_ = CCC_INSERT_ERROR,                                        \
        };                                                                     \
        if (rom_new_ins_base_)                                                 \
        {                                                                      \
            *rom_new_ins_base_ = lazy_value;                                   \
            *((typeof(key) *)ccc_impl_rom_key_in_slot(rom_insert_entry.rom_,   \
                                                      rom_new_ins_base_))      \
                = key;                                                         \
            (void)ccc_impl_rom_insert(                                         \
                rom_insert_entry.rom_,                                         \
                ccc_impl_romap_elem_in_slot(rom_insert_entry.rom_,             \
                                            rom_insert_entry.entry_.e_),       \
                rom_insert_entry.last_cmp_,                                    \
                ccc_impl_romap_elem_in_slot(rom_insert_entry.rom_,             \
                                            rom_new_ins_base_));               \
        }                                                                      \
    }))

/*==================     Core Macro Implementations     =====================*/

#define ccc_impl_rom_and_modify_w(realtime_ordered_map_entry_ptr, type_name,   \
                                  closure_over_T...)                           \
    (__extension__({                                                           \
        __auto_type rom_ent_ptr_ = (realtime_ordered_map_entry_ptr);           \
        struct ccc_rtree_entry_ rom_mod_ent_                                   \
            = {.entry_ = {.stats_ = CCC_INPUT_ERROR}};                         \
        if (rom_ent_ptr_)                                                      \
        {                                                                      \
            rom_mod_ent_ = rom_ent_ptr_->impl_;                                \
            if (rom_mod_ent_.entry_.stats_ & CCC_OCCUPIED)                     \
            {                                                                  \
                type_name *const T = rom_mod_ent_.entry_.e_;                   \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        rom_mod_ent_;                                                          \
    }))

#define ccc_impl_rom_or_insert_w(realtime_ordered_map_entry_ptr,               \
                                 lazy_key_value...)                            \
    (__extension__({                                                           \
        __auto_type or_ins_entry_ptr_ = (realtime_ordered_map_entry_ptr);      \
        typeof(lazy_key_value) *rom_or_ins_ret_ = NULL;                        \
        if (or_ins_entry_ptr_)                                                 \
        {                                                                      \
            struct ccc_rtree_entry_ *rom_or_ins_ent_                           \
                = &or_ins_entry_ptr_->impl_;                                   \
            if (rom_or_ins_ent_->entry_.stats_ == CCC_OCCUPIED)                \
            {                                                                  \
                rom_or_ins_ret_ = rom_or_ins_ent_->entry_.e_;                  \
            }                                                                  \
            else                                                               \
            {                                                                  \
                rom_or_ins_ret_ = ccc_impl_rom_new(rom_or_ins_ent_);           \
                ccc_impl_rom_insert_key_val(rom_or_ins_ent_, rom_or_ins_ret_,  \
                                            lazy_key_value);                   \
            }                                                                  \
        }                                                                      \
        rom_or_ins_ret_;                                                       \
    }))

#define ccc_impl_rom_insert_entry_w(realtime_ordered_map_entry_ptr,            \
                                    lazy_key_value...)                         \
    (__extension__({                                                           \
        __auto_type ins_entry_ptr_ = (realtime_ordered_map_entry_ptr);         \
        typeof(lazy_key_value) *rom_ins_ent_ret_ = NULL;                       \
        if (ins_entry_ptr_)                                                    \
        {                                                                      \
            struct ccc_rtree_entry_ *rom_ins_ent_ = &ins_entry_ptr_->impl_;    \
            if (!(rom_ins_ent_->entry_.stats_ & CCC_OCCUPIED))                 \
            {                                                                  \
                rom_ins_ent_ret_ = ccc_impl_rom_new(rom_ins_ent_);             \
                ccc_impl_rom_insert_key_val(rom_ins_ent_, rom_ins_ent_ret_,    \
                                            lazy_key_value);                   \
            }                                                                  \
            else if (rom_ins_ent_->entry_.stats_ == CCC_OCCUPIED)              \
            {                                                                  \
                struct ccc_romap_elem_ ins_ent_saved_                          \
                    = *ccc_impl_romap_elem_in_slot(rom_ins_ent_->rom_,         \
                                                   rom_ins_ent_->entry_.e_);   \
                *((typeof(rom_ins_ent_ret_))rom_ins_ent_->entry_.e_)           \
                    = lazy_key_value;                                          \
                *ccc_impl_romap_elem_in_slot(rom_ins_ent_->rom_,               \
                                             rom_ins_ent_->entry_.e_)          \
                    = ins_ent_saved_;                                          \
                rom_ins_ent_ret_ = rom_ins_ent_->entry_.e_;                    \
            }                                                                  \
        }                                                                      \
        rom_ins_ent_ret_;                                                      \
    }))

#define ccc_impl_rom_try_insert_w(realtime_ordered_map_ptr, key,               \
                                  lazy_value...)                               \
    (__extension__({                                                           \
        struct ccc_romap_ *const try_ins_map_ptr_                              \
            = (realtime_ordered_map_ptr);                                      \
        struct ccc_ent_ rom_try_ins_ent_ret_ = {.stats_ = CCC_INPUT_ERROR};    \
        if (try_ins_map_ptr_)                                                  \
        {                                                                      \
            __auto_type rom_key_ = (key);                                      \
            struct ccc_rtree_entry_ rom_try_ins_ent_                           \
                = ccc_impl_rom_entry(try_ins_map_ptr_, (void *)&rom_key_);     \
            if (!(rom_try_ins_ent_.entry_.stats_ & CCC_OCCUPIED))              \
            {                                                                  \
                ccc_impl_rom_insert_and_copy_key(                              \
                    rom_try_ins_ent_, rom_try_ins_ent_ret_, key, lazy_value);  \
            }                                                                  \
            else if (rom_try_ins_ent_.entry_.stats_ == CCC_OCCUPIED)           \
            {                                                                  \
                rom_try_ins_ent_ret_ = rom_try_ins_ent_.entry_;                \
            }                                                                  \
        }                                                                      \
        rom_try_ins_ent_ret_;                                                  \
    }))

#define ccc_impl_rom_insert_or_assign_w(realtime_ordered_map_ptr, key,         \
                                        lazy_value...)                         \
    (__extension__({                                                           \
        struct ccc_romap_ *const ins_or_assign_map_ptr_                        \
            = (realtime_ordered_map_ptr);                                      \
        struct ccc_ent_ rom_ins_or_assign_ent_ret_                             \
            = {.stats_ = CCC_INPUT_ERROR};                                     \
        if (ins_or_assign_map_ptr_)                                            \
        {                                                                      \
            __auto_type rom_key_ = (key);                                      \
            struct ccc_rtree_entry_ rom_ins_or_assign_ent_                     \
                = ccc_impl_rom_entry(ins_or_assign_map_ptr_,                   \
                                     (void *)&rom_key_);                       \
            if (!(rom_ins_or_assign_ent_.entry_.stats_ & CCC_OCCUPIED))        \
            {                                                                  \
                ccc_impl_rom_insert_and_copy_key(rom_ins_or_assign_ent_,       \
                                                 rom_ins_or_assign_ent_ret_,   \
                                                 key, lazy_value);             \
            }                                                                  \
            else if (rom_ins_or_assign_ent_.entry_.stats_ == CCC_OCCUPIED)     \
            {                                                                  \
                struct ccc_romap_elem_ ins_ent_saved_                          \
                    = *ccc_impl_romap_elem_in_slot(                            \
                        rom_ins_or_assign_ent_.rom_,                           \
                        rom_ins_or_assign_ent_.entry_.e_);                     \
                *((typeof(lazy_value) *)rom_ins_or_assign_ent_.entry_.e_)      \
                    = lazy_value;                                              \
                *ccc_impl_romap_elem_in_slot(rom_ins_or_assign_ent_.rom_,      \
                                             rom_ins_or_assign_ent_.entry_.e_) \
                    = ins_ent_saved_;                                          \
                rom_ins_or_assign_ent_ret_ = rom_ins_or_assign_ent_.entry_;    \
                *((typeof(rom_key_) *)ccc_impl_rom_key_in_slot(                \
                    rom_ins_or_assign_ent_.rom_,                               \
                    rom_ins_or_assign_ent_ret_.e_))                            \
                    = rom_key_;                                                \
            }                                                                  \
        }                                                                      \
        rom_ins_or_assign_ent_ret_;                                            \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_REALTIME__ORDERED_MAP_H */
