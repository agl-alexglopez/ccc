#ifndef CCC_IMPL_REALTIME_ORDERED_MAP_H
#define CCC_IMPL_REALTIME_ORDERED_MAP_H

#include "types.h"

#include <stddef.h>
#include <stdint.h>

#define CCC_ROM_EMPTY 0x0
#define CCC_ROM_ENTRY_VACANT 0x0
#define CCC_ROM_ENTRY_OCCUPIED 0x1
#define CCC_ROM_ENTRY_INSERT_ERROR 0x2
#define CCC_ROM_ENTRY_SEARCH_ERROR 0x4
#define CCC_ROM_ENTRY_NULL 0x8
#define CCC_ROM_ENTRY_DELETE_ERROR 0x10

typedef struct ccc_rtom_elem_
{
    struct ccc_rtom_elem_ *link[2];
    struct ccc_rtom_elem_ *parent;
    uint8_t parity;
} ccc_rtom_elem_;

struct ccc_rtom_
{
    struct ccc_rtom_elem_ *root;
    struct ccc_rtom_elem_ end;
    size_t sz;
    size_t key_offset;
    size_t node_elem_offset;
    size_t elem_sz;
    ccc_alloc_fn *alloc;
    ccc_key_cmp_fn *cmp;
    void *aux;
};

struct ccc_rtom_entry_
{
    struct ccc_rtom_ *rom;
    ccc_threeway_cmp last_cmp;
    ccc_entry entry;
};

#define CCC_IMPL_ROM_INIT(struct_name, node_elem_field, key_elem_field,        \
                          map_name, alloc_fn, key_cmp_fn, aux_data)            \
    {                                                                          \
        {                                                                      \
            .root = &(map_name).impl.end,                                      \
            .end = {.link = {&(map_name).impl.end, &(map_name).impl.end},      \
                    .parent = &(map_name).impl.end,                            \
                    .parity = 1},                                              \
            .sz = 0, .key_offset = offsetof(struct_name, key_elem_field),      \
            .node_elem_offset = offsetof(struct_name, node_elem_field),        \
            .elem_sz = sizeof(struct_name), .alloc = (alloc_fn),               \
            .cmp = (key_cmp_fn), .aux = (aux_data),                            \
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

#define CCC_IMPL_ROM_ENTRY(rom_ptr, key...)                                    \
    ({                                                                         \
        __auto_type rom_ent_key_ = (key);                                      \
        struct ccc_rtom_entry_ rom_ent_                                        \
            = ccc_impl_rom_entry(&(rom_ptr)->impl, &rom_ent_key_);             \
        rom_ent_;                                                              \
    })

#define CCC_IMPL_ROM_GET(rom_ptr, key...)                                      \
    ({                                                                         \
        __auto_type rom_get_ent_key_ = key;                                    \
        struct ccc_rtom_entry_ rom_get_ent_                                    \
            = ccc_impl_rom_entry((rom_ptr), &rom_get_ent_key_);                \
        void const *const rom_get_res_ = NULL;                                 \
        if (rom_get_ent_.entry.status & CCC_ROM_ENTRY_OCCUPIED)                \
        {                                                                      \
            rom_get_res_ = rom_get_ent_.entry.entry;                           \
        }                                                                      \
        rom_get_res_;                                                          \
    })

#define CCC_IMPL_ROM_GET_MUT(rom_ptr, key...)                                  \
    (void *)CCC_IMPL_ROM_GET(rom_ptr, key)

#define CCC_IMPL_ROM_AND_MODIFY(rom_entry, mod_fn)                             \
    ({                                                                         \
        struct ccc_om_entry_ rom_mod_ent_ = (rom_entry).impl;                  \
        if (rom_mod_ent_.entry.status & CCC_ROM_ENTRY_OCCUPIED)                \
        {                                                                      \
            (mod_fn)(                                                          \
                (ccc_update){.container = (void *const)e.entry, .aux = NULL}); \
        }                                                                      \
        rom_mod_ent_;                                                          \
    })

#define CCC_IMPL_ROM_AND_MODIFY_W(rom_entry, mod_fn, aux_data)                 \
    ({                                                                         \
        struct ccc_om_entry_ rom_mod_ent_ = (rom_entry).impl;                  \
        if (rom_mod_ent_.entry.status & CCC_ROM_ENTRY_OCCUPIED)                \
        {                                                                      \
            __auto_type rom_aux_data_ = (aux_data);                            \
            (mod_fn)((ccc_update){.container = (void *const)e.entry,           \
                                  .aux = &rom_aux_data_});                     \
        }                                                                      \
        rom_mod_ent_;                                                          \
    })

#define CCC_IMPL_ROM_NEW_INSERT(entry_copy, key_val...)                        \
    ({                                                                         \
        void *rom_ins_alloc_ret_ = NULL;                                       \
        if ((entry_copy).rom->alloc)                                           \
        {                                                                      \
            rom_ins_alloc_ret_                                                 \
                = (entry_copy).rom->alloc(NULL, (entry_copy).rom->elem_sz);    \
            if (rom_ins_alloc_ret_)                                            \
            {                                                                  \
                *((typeof(key_val) *)rom_ins_alloc_ret_) = key_val;            \
                rom_ins_alloc_ret_ = ccc_impl_rom_insert(                      \
                    (entry_copy).rom, (entry_copy).entry.entry,                \
                    (entry_copy).last_cmp,                                     \
                    ccc_impl_rom_elem_in_slot((entry_copy).rom,                \
                                              rom_ins_alloc_ret_));            \
            }                                                                  \
        }                                                                      \
        rom_ins_alloc_ret_;                                                    \
    })

#define CCC_IMPL_ROM_INSERT_ENTRY(rom_entry, key_value...)                     \
    ({                                                                         \
        struct ccc_rtom_entry_ rom_ins_ent_ = (rom_entry).impl;                \
        typeof(key_value) *rom_ins_ent_ret_ = NULL;                            \
        if (!(rom_ins_ent_.entry.status & CCC_ROM_ENTRY_OCCUPIED))             \
        {                                                                      \
            rom_ins_ent_ret_                                                   \
                = CCC_IMPL_ROM_NEW_INSERT(rom_ins_ent_, key_value);            \
        }                                                                      \
        else if (rom_ins_ent_.entry.status == CCC_ROM_ENTRY_OCCUPIED)          \
        {                                                                      \
            struct ccc_rtom_elem_ ins_ent_saved_ = *ccc_impl_rom_elem_in_slot( \
                rom_ins_ent_.rom, rom_ins_ent_.entry.entry);                   \
            *((typeof(key_value) *)rom_ins_ent_.entry.entry) = key_value;      \
            *ccc_impl_rom_elem_in_slot(rom_ins_ent_.rom,                       \
                                       rom_ins_ent_.entry.entry)               \
                = ins_ent_saved_;                                              \
            rom_ins_ent_ret_ = rom_ins_ent_.entry.entry;                       \
        }                                                                      \
        rom_ins_ent_ret_;                                                      \
    })

#define CCC_IMPL_ROM_OR_INSERT(entry_copy, key_val...)                         \
    ({                                                                         \
        struct ccc_rtom_entry_ rom_or_ins_ent_ = (entry_copy).impl;            \
        typeof(key_val) *rom_or_ins_ret_ = NULL;                               \
        if (rom_or_ins_ent_.entry.status == CCC_ROM_ENTRY_OCCUPIED)            \
        {                                                                      \
            rom_or_ins_ret_ = rom_or_ins_ent_.entry.entry;                     \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            rom_or_ins_ret_                                                    \
                = CCC_IMPL_ROM_NEW_INSERT(rom_or_ins_ent_, key_val);           \
        }                                                                      \
        rom_or_ins_ret_;                                                       \
    })

#endif /* CCC_IMPL_REALTIME_ORDERED_MAP_H */
