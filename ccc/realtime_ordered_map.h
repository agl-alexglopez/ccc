#ifndef CCC_REALTIME_ORDERED_MAP_H
#define CCC_REALTIME_ORDERED_MAP_H

#include "impl_realtime_ordered_map.h"

typedef struct
{
    struct ccc_impl_r_om_elem impl;
} ccc_r_om_elem;

typedef struct
{
    struct ccc_impl_realtime_ordered_map impl;
} ccc_realtime_ordered_map;

typedef struct
{
    struct ccc_impl_r_om_entry impl;
} ccc_r_om_entry;

#define CCC_ROM_INIT(struct_name, node_elem_field, key_elem_field, map_name,   \
                     alloc_fn, key_cmp_fn, aux_data)                           \
    (ccc_realtime_ordered_map)                                                 \
        CCC_IMPL_ROM_INIT(struct_name, node_elem_field, key_elem_field,        \
                          map_name, alloc_fn, key_cmp_fn, aux_data)

bool ccc_rom_contains(ccc_realtime_ordered_map const *rom, void const *key);
void const *ccc_rom_get(ccc_realtime_ordered_map const *rom, void const *key);
void *ccc_rom_get_mut(ccc_realtime_ordered_map const *rom, void const *key);

ccc_r_om_entry ccc_rom_insert(ccc_realtime_ordered_map *rom,
                              ccc_r_om_elem *out_handle);
void *ccc_rom_remove(ccc_realtime_ordered_map *rom, ccc_r_om_elem *out_handle);

ccc_r_om_entry ccc_rom_entry(ccc_realtime_ordered_map const *rom,
                             void const *key);

void *ccc_rom_insert_entry(ccc_r_om_entry e, ccc_r_om_elem *elem);

bool ccc_rom_remove_entry(ccc_r_om_entry e);

void const *ccc_rom_unwrap(ccc_r_om_entry e);
void *ccc_rom_unwrap_mut(ccc_r_om_entry e);

void *ccc_rom_begin(ccc_realtime_ordered_map const *rom);
void *ccc_rom_next(ccc_realtime_ordered_map *rom, ccc_r_om_elem const *);

size_t ccc_rom_size(ccc_realtime_ordered_map const *rom);
bool ccc_rom_empty(ccc_realtime_ordered_map const *rom);

bool ccc_rom_validate(ccc_realtime_ordered_map const *rom);
void ccc_rom_print(ccc_realtime_ordered_map const *rom, ccc_print_fn *fn);

void *ccc_rom_root(ccc_realtime_ordered_map const *rom);

#endif /* CCC_REALTIME_ORDERED_MAP_H */
