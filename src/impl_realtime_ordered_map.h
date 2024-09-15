#ifndef CCC_IMPL_REALTIME_ORDERED_MAP_H
#define CCC_IMPL_REALTIME_ORDERED_MAP_H

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

#define CCC_IMPL_ROM_INIT(struct_name, node_elem_field, key_elem_field,        \
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
struct ccc_rtom_elem_ *ccc_impl_rom_elem_in_slot(struct ccc_rtom_ const *rom,
                                                 void const *slot);
struct ccc_rtom_entry_ ccc_impl_rom_entry(struct ccc_rtom_ const *rom,
                                          void const *key);
void *ccc_impl_rom_insert(struct ccc_rtom_ *rom, struct ccc_rtom_elem_ *parent,
                          ccc_threeway_cmp last_cmp,
                          struct ccc_rtom_elem_ *out_handle);

#define CCC_IMPL_ROM_ENTRY(realtime_ordered_map_ptr, key...)                   \
    ({                                                                         \
        __auto_type rom_ent_key_ = (key);                                      \
        struct ccc_rtom_entry_ rom_ent_ = ccc_impl_rom_entry(                  \
            &(realtime_ordered_map_ptr)->impl_, &rom_ent_key_);                \
        rom_ent_;                                                              \
    })

#define CCC_IMPL_ROM_GET_KEY_VAL(realtime_ordered_map_ptr, key...)             \
    ({                                                                         \
        __auto_type rom_get_ent_key_ = key;                                    \
        struct ccc_rtom_entry_ rom_get_ent_ = ccc_impl_rom_entry(              \
            (realtime_ordered_map_ptr), &rom_get_ent_key_);                    \
        void *const rom_get_res_ = NULL;                                       \
        if (rom_get_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)                   \
        {                                                                      \
            rom_get_res_ = rom_get_ent_.entry_.e_;                             \
        }                                                                      \
        rom_get_res_;                                                          \
    })

#define CCC_IMPL_ROM_AND_MODIFY(realtime_ordered_map_entry, mod_fn)            \
    ({                                                                         \
        struct ccc_tree_entry_ rom_mod_ent_                                    \
            = (realtime_ordered_map_entry)->impl_;                             \
        if (rom_mod_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)                   \
        {                                                                      \
            (mod_fn)(&(ccc_update){.container = (void *const)e.entry,          \
                                   .aux = NULL});                              \
        }                                                                      \
        rom_mod_ent_;                                                          \
    })

#define CCC_IMPL_ROM_AND_MODIFY_W(realtime_ordered_map_entry, mod_fn,          \
                                  aux_data)                                    \
    ({                                                                         \
        struct ccc_tree_entry_ rom_mod_ent_                                    \
            = (realtime_ordered_map_entry)->impl_;                             \
        if (rom_mod_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)                   \
        {                                                                      \
            __auto_type rom_aux_data_ = (aux_data);                            \
            (mod_fn)(&(ccc_update){.container = (void *const)e.entry,          \
                                   .aux = &rom_aux_data_});                    \
        }                                                                      \
        rom_mod_ent_;                                                          \
    })

#define CCC_IMPL_ROM_NEW_INSERT(realtime_ordered_map_entry, key_value...)      \
    ({                                                                         \
        void *rom_ins_alloc_ret_ = NULL;                                       \
        if ((realtime_ordered_map_entry)->rom_->alloc_)                        \
        {                                                                      \
            rom_ins_alloc_ret_                                                 \
                = (realtime_ordered_map_entry)                                 \
                      ->rom_->alloc_(                                          \
                          NULL, (realtime_ordered_map_entry)->rom_->elem_sz_); \
            if (rom_ins_alloc_ret_)                                            \
            {                                                                  \
                *((typeof(key_value) *)rom_ins_alloc_ret_) = key_value;        \
                rom_ins_alloc_ret_ = ccc_impl_rom_insert(                      \
                    (realtime_ordered_map_entry)->rom_,                        \
                    (realtime_ordered_map_entry)->entry_.e_,                   \
                    (realtime_ordered_map_entry)->last_cmp_,                   \
                    ccc_impl_rom_elem_in_slot(                                 \
                        (realtime_ordered_map_entry)->rom_,                    \
                        rom_ins_alloc_ret_));                                  \
            }                                                                  \
        }                                                                      \
        rom_ins_alloc_ret_;                                                    \
    })

#define CCC_IMPL_ROM_INSERT_ENTRY(realtime_ordered_map_entry, key_value...)    \
    ({                                                                         \
        struct ccc_rtom_entry_ *rom_ins_ent_                                   \
            = &(realtime_ordered_map_entry)->impl_;                            \
        typeof(key_value) *rom_ins_ent_ret_ = NULL;                            \
        if (!(rom_ins_ent_->entry_.stats_ & CCC_ENTRY_OCCUPIED))               \
        {                                                                      \
            rom_ins_ent_ret_                                                   \
                = CCC_IMPL_ROM_NEW_INSERT(rom_ins_ent_, key_value);            \
        }                                                                      \
        else if (rom_ins_ent_->entry_.stats_ == CCC_ENTRY_OCCUPIED)            \
        {                                                                      \
            struct ccc_rtom_elem_ ins_ent_saved_ = *ccc_impl_rom_elem_in_slot( \
                rom_ins_ent_->rom_, rom_ins_ent_->entry_.e_);                  \
            *((typeof(key_value) *)rom_ins_ent_->entry_.e_) = key_value;       \
            *ccc_impl_rom_elem_in_slot(rom_ins_ent_->rom_,                     \
                                       rom_ins_ent_->entry_.e_)                \
                = ins_ent_saved_;                                              \
            rom_ins_ent_ret_ = rom_ins_ent_->entry_.e_;                        \
        }                                                                      \
        rom_ins_ent_ret_;                                                      \
    })

#define CCC_IMPL_ROM_OR_INSERT(realtime_ordered_map_entry, key_value...)       \
    ({                                                                         \
        struct ccc_rtom_entry_ *rom_or_ins_ent_                                \
            = &(realtime_ordered_map_entry)->impl_;                            \
        typeof(key_value) *rom_or_ins_ret_ = NULL;                             \
        if (rom_or_ins_ent_->entry_.stats_ == CCC_ENTRY_OCCUPIED)              \
        {                                                                      \
            rom_or_ins_ret_ = rom_or_ins_ent_->entry_.e_;                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            rom_or_ins_ret_                                                    \
                = CCC_IMPL_ROM_NEW_INSERT(rom_or_ins_ent_, key_value);         \
        }                                                                      \
        rom_or_ins_ret_;                                                       \
    })

#endif /* CCC_IMPL_REALTIME_ORDERED_MAP_H */
