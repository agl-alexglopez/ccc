#ifndef CCC_REALTIME_ORDERED_MAP_H
#define CCC_REALTIME_ORDERED_MAP_H

#include "impl_realtime_ordered_map.h"

typedef struct
{
    struct ccc_rtom_elem_ impl;
} ccc_rtom_elem;

typedef struct
{
    struct ccc_rtom_ impl;
} ccc_realtime_ordered_map;

typedef struct
{
    struct ccc_rtom_entry_ impl;
} ccc_rtom_entry;

#define CCC_ROM_INIT(struct_name, node_elem_field, key_elem_field, map_name,   \
                     alloc_fn, key_cmp_fn, aux_data)                           \
    (ccc_realtime_ordered_map)                                                 \
        CCC_IMPL_ROM_INIT(struct_name, node_elem_field, key_elem_field,        \
                          map_name, alloc_fn, key_cmp_fn, aux_data)

#define CCC_ROM_ENTRY(entry, key...)                                           \
    (ccc_rtom_entry)                                                           \
    {                                                                          \
        CCC_IMPL_ROM_ENTRY(entry, key)                                         \
    }

#define CCC_ROM_GET(entry, key...) CCC_IMPL_ROM_GET(entry, key)

#define CCC_ROM_GET_MUT(entry, key...) CCC_IMPL_ROM_GET_MUT(entry, key)

#define CCC_ROM_AND_MODIFY(map_entry, mod_fn)                                  \
    (ccc_rtom_entry)                                                           \
    {                                                                          \
        CCC_IMPL_ROM_AND_MODIFY(map_entry, mod_fn)                             \
    }

#define CCC_ROM_AND_MODIFY_W(map_entry, mod_fn, aux_data)                      \
    (ccc_rtom_entry)                                                           \
    {                                                                          \
        CCC_IMPL_ROM_AND_MODIFY_WITH(map_entry, mod_fn, aux_data)              \
    }

#define CCC_ROM_OR_INSERT(entry, key_val...)                                   \
    CCC_IMPL_ROM_OR_INSERT(entry, key_val)

#define CCC_ROM_INSERT_ENTRY(map_entry, key_value...)                          \
    CCC_IMPL_ROM_INSERT_ENTRY(map_entry, key_value)

/*=================    Membership and Retrieval    ==========================*/

bool ccc_rom_contains(ccc_realtime_ordered_map const *rom, void const *key);

void const *ccc_rom_get(ccc_realtime_ordered_map const *rom, void const *key);

void *ccc_rom_get_mut(ccc_realtime_ordered_map const *rom, void const *key);

/*======================      Entry API    ==================================*/

ccc_entry ccc_rom_insert(ccc_realtime_ordered_map *rom,
                         ccc_rtom_elem *out_handle);

ccc_entry ccc_rom_remove(ccc_realtime_ordered_map *rom,
                         ccc_rtom_elem *out_handle);

ccc_rtom_entry ccc_rom_entry(ccc_realtime_ordered_map const *rom,
                             void const *key);

ccc_entry ccc_rom_remove_entry(ccc_rtom_entry e);

void *ccc_rom_or_insert(ccc_rtom_entry e, ccc_rtom_elem *elem);

void *ccc_rom_insert_entry(ccc_rtom_entry e, ccc_rtom_elem *elem);

void const *ccc_rom_unwrap(ccc_rtom_entry e);
void *ccc_rom_unwrap_mut(ccc_rtom_entry e);

/*======================      Iteration    ==================================*/

void *ccc_rom_begin(ccc_realtime_ordered_map const *rom);
void *ccc_rom_next(ccc_realtime_ordered_map *rom, ccc_rtom_elem const *);

void *ccc_rom_rbegin(ccc_realtime_ordered_map const *rom);
void *ccc_rom_rnext(ccc_realtime_ordered_map *rom, ccc_rtom_elem const *);

void *ccc_rom_end(ccc_realtime_ordered_map const *rom);
void *ccc_rom_rend(ccc_realtime_ordered_map const *rom);

/*======================       Getters     ==================================*/

size_t ccc_rom_size(ccc_realtime_ordered_map const *rom);
bool ccc_rom_empty(ccc_realtime_ordered_map const *rom);

/*=================           Cleanup               =========================*/

void ccc_rom_clear(ccc_realtime_ordered_map *rom,
                   ccc_destructor_fn *destructor);

ccc_result ccc_rom_clear_and_free(ccc_realtime_ordered_map *rom,
                                  ccc_destructor_fn *destructor);

/*=================     Utilities and Validation    =========================*/

void ccc_rom_print(ccc_realtime_ordered_map const *rom, ccc_print_fn *fn);

bool ccc_rom_validate(ccc_realtime_ordered_map const *rom);

void *ccc_rom_root(ccc_realtime_ordered_map const *rom);

#ifdef REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#    define rtom_elem ccc_rtom_elem
#    define realtime_ordered_map ccc_realtime_ordered_map
#    define rtom_entry ccc_rtom_entry
#    define ROM_INIT CCC_ROM_INIT
#    define ROM_ENTRY CCC_ROM_ENTRY
#    define ROM_GET CCC_ROM_GET
#    define ROM_GET_MUT CCC_ROM_GET_MUT
#    define ROM_AND_MODIFY CCC_ROM_AND_MODIFY
#    define ROM_AND_MODIFY_W CCC_ROM_AND_MODIFY_W
#    define ROM_OR_INSERT CCC_ROM_OR_INSERT
#    define ROM_INSERT_ENTRY CCC_ROM_INSERT_ENTRY
#    define rom_contains ccc_rom_contains
#    define rom_get ccc_rom_get
#    define rom_get_mut ccc_rom_get_mut
#    define rom_insert ccc_rom_insert
#    define rom_remove ccc_rom_remove
#    define rom_entry ccc_rom_entry
#    define rom_remove_entry ccc_rom_remove_entry
#    define rom_or_insert ccc_rom_or_insert
#    define rom_insert_entry ccc_rom_insert_entry
#    define rom_unwrap ccc_rom_unwrap
#    define rom_unwrap_mut ccc_rom_unwrap_mut
#    define rom_begin ccc_rom_begin
#    define rom_next ccc_rom_next
#    define rom_rbegin ccc_rom_rbegin
#    define rom_rnext ccc_rom_rnext
#    define rom_end ccc_rom_end
#    define rom_rend ccc_rom_rend
#    define rom_size ccc_rom_size
#    define rom_empty ccc_rom_empty
#    define rom_clear ccc_rom_clear
#    define rom_clear_and_free ccc_rom_clear_and_free
#    define rom_print ccc_rom_print
#    define rom_validate ccc_rom_validate
#    define rom_root ccc_rom_root
#endif

#endif /* CCC_REALTIME_ORDERED_MAP_H */
