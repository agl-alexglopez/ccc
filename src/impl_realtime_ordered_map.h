#ifndef CCC_IMPL_REALTIME_ORDERED_MAP_H
#define CCC_IMPL_REALTIME_ORDERED_MAP_H

#include "types.h"

#include <stddef.h>
#include <stdint.h>

#define CCC_ROM_EMPTY ((uint64_t)0)
#define CCC_ROM_ENTRY_VACANT ((uint8_t)0x0)
#define CCC_ROM_ENTRY_OCCUPIED ((uint8_t)0x1)
#define CCC_ROM_ENTRY_INSERT_ERROR ((uint8_t)0x2)
#define CCC_ROM_ENTRY_SEARCH_ERROR ((uint8_t)0x4)
#define CCC_ROM_ENTRY_NULL ((uint8_t)0x8)
#define CCC_ROM_ENTRY_DELETE_ERROR ((uint8_t)0x10)

typedef struct ccc_impl_r_om_elem
{
    struct ccc_impl_r_om_elem *link[2];
    struct ccc_impl_r_om_elem *parent;
    uint8_t parity;
} ccc_impl_r_om_elem;

struct ccc_impl_realtime_ordered_map
{
    struct ccc_impl_r_om_elem *root;
    struct ccc_impl_r_om_elem end;
    size_t sz;
    size_t key_offset;
    size_t node_elem_offset;
    size_t elem_sz;
    ccc_alloc_fn *alloc;
    ccc_key_cmp_fn *cmp;
    void *aux;
};

struct ccc_impl_r_om_entry
{
    struct ccc_impl_realtime_ordered_map *rom;
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

void *
ccc_impl_rom_key_from_node(struct ccc_impl_realtime_ordered_map const *rom,
                           struct ccc_impl_r_om_elem const *elem);
struct ccc_impl_r_om_elem *
ccc_impl_rom_elem_in_slot(struct ccc_impl_realtime_ordered_map const *rom,
                          void const *slot);

#endif /* CCC_IMPL_REALTIME_ORDERED_MAP_H */
