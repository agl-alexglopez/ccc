#ifndef CCC_IMPL_REALTIME_ORDERED_MAP_H
#define CCC_IMPL_REALTIME_ORDERED_MAP_H

#include "impl_types.h"
#include "types.h"

#include <stddef.h>
#include <stdint.h>

typedef struct ccc_rtom_elem_
{
    struct ccc_rtom_elem_ *branch_[2];
    struct ccc_rtom_elem_ *parent_;
    uint8_t parity_;
} ccc_rtom_elem_;

struct ccc_rtom_
{
    struct ccc_rtom_elem_ *root_;
    struct ccc_rtom_elem_ end_;
    size_t sz_;
    size_t key_offset_;
    size_t node_elem_offset_;
    size_t elem_sz_;
    ccc_alloc_fn *alloc_;
    ccc_key_cmp_fn *cmp_;
    void *aux_;
};

struct ccc_rtom_entry_
{
    struct ccc_rtom_ *rom_;
    ccc_threeway_cmp last_cmp_;
    struct ccc_entry_ entry_;
};

#define ccc_impl_rom_init(struct_name, node_elem_field, key_elem_field,        \
                          map_name, alloc_fn, key_cmp_fn, aux_data)            \
    {                                                                          \
        {                                                                      \
            .root_ = &(map_name).impl_.end_,                                   \
            .end_ = {.parity_ = 1,                                             \
                     .parent_ = &(map_name).impl_.end_,                        \
                     .branch_                                                  \
                     = {&(map_name).impl_.end_, &(map_name).impl_.end_}},      \
            .sz_ = 0, .key_offset_ = offsetof(struct_name, key_elem_field),    \
            .node_elem_offset_ = offsetof(struct_name, node_elem_field),       \
            .elem_sz_ = sizeof(struct_name), .alloc_ = (alloc_fn),             \
            .cmp_ = (key_cmp_fn), .aux_ = (aux_data),                          \
        }                                                                      \
    }

void *ccc_impl_rom_key_from_node(struct ccc_rtom_ const *rom,
                                 struct ccc_rtom_elem_ const *elem);
void *ccc_impl_rom_key_in_slot(struct ccc_rtom_ const *rom, void const *slot);
struct ccc_rtom_elem_ *ccc_impl_rom_elem_in_slot(struct ccc_rtom_ const *rom,
                                                 void const *slot);
struct ccc_rtom_entry_ ccc_impl_rom_entry(struct ccc_rtom_ const *rom,
                                          void const *key);
void *ccc_impl_rom_insert(struct ccc_rtom_ *rom, struct ccc_rtom_elem_ *parent,
                          ccc_threeway_cmp last_cmp,
                          struct ccc_rtom_elem_ *out_handle);

/*==================   Helper Macros for Repeated Logic     =================*/

#define ccc_impl_rom_new(realtime_ordered_map_entry)                           \
    ({                                                                         \
        void *rom_ins_alloc_ret_ = NULL;                                       \
        if ((realtime_ordered_map_entry)->rom_->alloc_)                        \
        {                                                                      \
            rom_ins_alloc_ret_                                                 \
                = (realtime_ordered_map_entry)                                 \
                      ->rom_->alloc_(                                          \
                          NULL, (realtime_ordered_map_entry)->rom_->elem_sz_); \
        }                                                                      \
        rom_ins_alloc_ret_;                                                    \
    })

#define ccc_impl_rom_insert_key_val(realtime_ordered_map_entry, new_mem,       \
                                    lazy_key_value...)                         \
    ({                                                                         \
        if (new_mem)                                                           \
        {                                                                      \
            *new_mem = lazy_key_value;                                         \
            new_mem = ccc_impl_rom_insert(                                     \
                (realtime_ordered_map_entry)->rom_,                            \
                ccc_impl_rom_elem_in_slot(                                     \
                    (realtime_ordered_map_entry)->rom_,                        \
                    (realtime_ordered_map_entry)->entry_.e_),                  \
                (realtime_ordered_map_entry)->last_cmp_,                       \
                ccc_impl_rom_elem_in_slot((realtime_ordered_map_entry)->rom_,  \
                                          new_mem));                           \
        }                                                                      \
    })

#define ccc_impl_rom_insert_and_copy_key(                                      \
    rom_insert_entry, rom_insert_entry_ret, key, lazy_value...)                \
    ({                                                                         \
        typeof(lazy_value) *rom_new_ins_base_                                  \
            = ccc_impl_rom_new((&rom_insert_entry));                           \
        rom_insert_entry_ret = (struct ccc_entry_){                            \
            .e_ = rom_new_ins_base_,                                           \
            .stats_ = (CCC_ENTRY_INSERT_ERROR | CCC_ENTRY_CONTAINS_NULL),      \
        };                                                                     \
        if (rom_new_ins_base_)                                                 \
        {                                                                      \
            *((typeof(lazy_value) *)rom_new_ins_base_) = lazy_value;           \
            *((typeof(key) *)ccc_impl_rom_key_in_slot(rom_insert_entry.rom_,   \
                                                      rom_new_ins_base_))      \
                = key;                                                         \
            (void)ccc_impl_rom_insert(                                         \
                rom_insert_entry.rom_,                                         \
                ccc_impl_rom_elem_in_slot(rom_insert_entry.rom_,               \
                                          rom_insert_entry.entry_.e_),         \
                rom_insert_entry.last_cmp_,                                    \
                ccc_impl_rom_elem_in_slot(rom_insert_entry.rom_,               \
                                          rom_new_ins_base_));                 \
        }                                                                      \
    })

/*==================     Core Macro Implementations     =====================*/

#define ccc_impl_rom_and_modify_w(realtime_ordered_map_entry, mod_fn,          \
                                  aux_data...)                                 \
    ({                                                                         \
        struct ccc_rtom_entry_ rom_mod_ent_                                    \
            = (realtime_ordered_map_entry)->impl_;                             \
        if (rom_mod_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)                   \
        {                                                                      \
            __auto_type rom_aux_data_ = aux_data;                              \
            (mod_fn)(&(ccc_update){.container = (void *const)e.entry,          \
                                   .aux = &rom_aux_data_});                    \
        }                                                                      \
        rom_mod_ent_;                                                          \
    })

#define ccc_impl_rom_or_insert_w(realtime_ordered_map_entry,                   \
                                 lazy_key_value...)                            \
    ({                                                                         \
        struct ccc_rtom_entry_ *rom_or_ins_ent_                                \
            = &(realtime_ordered_map_entry)->impl_;                            \
        typeof(lazy_key_value) *rom_or_ins_ret_ = NULL;                        \
        if (rom_or_ins_ent_->entry_.stats_ == CCC_ENTRY_OCCUPIED)              \
        {                                                                      \
            rom_or_ins_ret_ = rom_or_ins_ent_->entry_.e_;                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            rom_or_ins_ret_ = ccc_impl_rom_new(rom_or_ins_ent_);               \
            ccc_impl_rom_insert_key_val(rom_or_ins_ent_, rom_or_ins_ret_,      \
                                        lazy_key_value);                       \
        }                                                                      \
        rom_or_ins_ret_;                                                       \
    })

#define ccc_impl_rom_insert_entry_w(realtime_ordered_map_entry,                \
                                    lazy_key_value...)                         \
    ({                                                                         \
        struct ccc_rtom_entry_ *rom_ins_ent_                                   \
            = &(realtime_ordered_map_entry)->impl_;                            \
        typeof(lazy_key_value) *rom_ins_ent_ret_ = NULL;                       \
        if (!(rom_ins_ent_->entry_.stats_ & CCC_ENTRY_OCCUPIED))               \
        {                                                                      \
            rom_ins_ent_ret_ = ccc_impl_rom_new(rom_ins_ent_);                 \
            ccc_impl_rom_insert_key_val(rom_ins_ent_, rom_ins_ent_ret_,        \
                                        lazy_key_value);                       \
        }                                                                      \
        else if (rom_ins_ent_->entry_.stats_ == CCC_ENTRY_OCCUPIED)            \
        {                                                                      \
            struct ccc_rtom_elem_ ins_ent_saved_ = *ccc_impl_rom_elem_in_slot( \
                rom_ins_ent_->rom_, rom_ins_ent_->entry_.e_);                  \
            *((typeof(rom_ins_ent_ret_))rom_ins_ent_->entry_.e_)               \
                = lazy_key_value;                                              \
            *ccc_impl_rom_elem_in_slot(rom_ins_ent_->rom_,                     \
                                       rom_ins_ent_->entry_.e_)                \
                = ins_ent_saved_;                                              \
            rom_ins_ent_ret_ = rom_ins_ent_->entry_.e_;                        \
        }                                                                      \
        rom_ins_ent_ret_;                                                      \
    })

#define ccc_impl_rom_try_insert_w(realtime_ordered_map_ptr, key,               \
                                  lazy_value...)                               \
    ({                                                                         \
        __auto_type rom_key_ = (key);                                          \
        struct ccc_rtom_entry_ rom_try_ins_ent_ = ccc_impl_rom_entry(          \
            &(realtime_ordered_map_ptr)->impl_, &rom_key_);                    \
        struct ccc_entry_ rom_try_ins_ent_ret_ = {};                           \
        if (!(rom_try_ins_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED))            \
        {                                                                      \
            ccc_impl_rom_insert_and_copy_key(                                  \
                rom_try_ins_ent_, rom_try_ins_ent_ret_, key, lazy_value);      \
        }                                                                      \
        else if (rom_try_ins_ent_.entry_.stats_ == CCC_ENTRY_OCCUPIED)         \
        {                                                                      \
            rom_try_ins_ent_ret_ = rom_try_ins_ent_.entry_;                    \
        }                                                                      \
        rom_try_ins_ent_ret_;                                                  \
    })

#define ccc_impl_rom_insert_or_assign_w(realtime_ordered_map_ptr, key,         \
                                        lazy_value...)                         \
    ({                                                                         \
        __auto_type rom_key_ = (key);                                          \
        struct ccc_rtom_ *rtom_ptr_ = &(realtime_ordered_map_ptr)->impl_;      \
        struct ccc_rtom_entry_ rom_ins_or_assign_ent_                          \
            = ccc_impl_rom_entry(rtom_ptr_, &rom_key_);                        \
        struct ccc_entry_ rom_ins_or_assign_ent_ret_ = {};                     \
        if (!(rom_ins_or_assign_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED))      \
        {                                                                      \
            ccc_impl_rom_insert_and_copy_key(rom_ins_or_assign_ent_,           \
                                             rom_ins_or_assign_ent_ret_, key,  \
                                             lazy_value);                      \
        }                                                                      \
        else if (rom_ins_or_assign_ent_.entry_.stats_ == CCC_ENTRY_OCCUPIED)   \
        {                                                                      \
            struct ccc_rtom_elem_ ins_ent_saved_ = *ccc_impl_rom_elem_in_slot( \
                rom_ins_or_assign_ent_.rom_,                                   \
                rom_ins_or_assign_ent_.entry_.e_);                             \
            *((typeof(lazy_value) *)rom_ins_or_assign_ent_.entry_.e_)          \
                = lazy_value;                                                  \
            *ccc_impl_rom_elem_in_slot(rom_ins_or_assign_ent_.rom_,            \
                                       rom_ins_or_assign_ent_.entry_.e_)       \
                = ins_ent_saved_;                                              \
            rom_ins_or_assign_ent_ret_ = rom_ins_or_assign_ent_.entry_;        \
            *((typeof(rom_key_) *)ccc_impl_rom_key_in_slot(                    \
                rtom_ptr_, rom_ins_or_assign_ent_ret_.e_))                     \
                = rom_key_;                                                    \
        }                                                                      \
        rom_ins_or_assign_ent_ret_;                                            \
    })

#endif /* CCC_IMPL_REALTIME_realtime_ORDERED_MAP_H */
