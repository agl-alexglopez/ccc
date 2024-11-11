#ifndef CCC_REALTIME_ORDERED_MAP_H
#define CCC_REALTIME_ORDERED_MAP_H

#include <stddef.h>

#include "impl_realtime_ordered_map.h"
#include "types.h"

/** A realtime ordered map offers amortized O(lg N) search, insert, and erase
and pointer stability. This map offers a strict runtime bound of O(lg N) which
is helpful in realtime environments. Also, searching is a thread-safe read-only
operation. Balancing modifications only occur upon insert or remove. */
typedef struct ccc_romap_ ccc_realtime_ordered_map;

/** A map element is the intrusive element of the user defined struct being
stored in the map for key value access. */
typedef struct ccc_romap_elem_ ccc_romap_elem;

/** A container specific entry used to implement the Entry API. The Entry API
 offers efficient search and subsequent insertion, deletion, or value update
 based on the needs of the user. */
typedef union
{
    struct ccc_romap_entry_ impl_;
} ccc_romap_entry;

#define ccc_rom_init(map_name, struct_name, node_elem_field, key_elem_field,   \
                     alloc_fn, key_cmp_fn, aux_data)                           \
    ccc_impl_rom_init(map_name, struct_name, node_elem_field, key_elem_field,  \
                      alloc_fn, key_cmp_fn, aux_data)

/*=================    Membership and Retrieval    ==========================*/

bool ccc_rom_contains(ccc_realtime_ordered_map const *rom, void const *key);

void *ccc_rom_get_key_val(ccc_realtime_ordered_map const *rom, void const *key);

/*======================      Entry API    ==================================*/

#define ccc_rom_insert_r(realtime_ordered_map_ptr, out_handle_ptr, tmp_ptr)    \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_rom_insert((realtime_ordered_map_ptr), (out_handle_ptr),           \
                       (tmp_ptr))                                              \
            .impl_                                                             \
    }

ccc_entry ccc_rom_insert(ccc_realtime_ordered_map *rom,
                         ccc_romap_elem *key_val_handle, ccc_romap_elem *tmp);

ccc_entry ccc_rom_try_insert(ccc_realtime_ordered_map *rom,
                             ccc_romap_elem *key_val_handle);

#define ccc_rom_try_insert_r(realtime_ordered_map_ptr, out_handle_ptr)         \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_rom_try_insert((realtime_ordered_map_ptr), (out_handle_ptr)).impl_ \
    }

#define ccc_rom_try_insert_w(realtime_ordered_map_ptr, key, lazy_value...)     \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_rom_try_insert_w(realtime_ordered_map_ptr, key, lazy_value)   \
    }

ccc_entry ccc_rom_insert_or_assign(ccc_realtime_ordered_map *rom,
                                   ccc_romap_elem *key_val_handle);

#define ccc_rom_insert_or_assign_w(realtime_ordered_map_ptr, key,              \
                                   lazy_value...)                              \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_rom_insert_or_assign_w(realtime_ordered_map_ptr, key,         \
                                        lazy_value)                            \
    }

ccc_entry ccc_rom_remove(ccc_realtime_ordered_map *rom,
                         ccc_romap_elem *out_handle);

#define ccc_rom_remove_r(realtime_ordered_map_ptr, out_handle_ptr)             \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_rom_remove((realtime_ordered_map_ptr), (out_handle_ptr)).impl_     \
    }

ccc_entry ccc_rom_remove_entry(ccc_romap_entry const *e);

#define ccc_rom_remove_entry_r(realtime_ordered_map_entry_ptr)                 \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_rom_remove_entry((realtime_ordered_map_entry_ptr)).impl_           \
    }

ccc_romap_entry *ccc_rom_and_modify(ccc_romap_entry *e, ccc_update_fn *fn);

ccc_romap_entry *ccc_rom_and_modify_aux(ccc_romap_entry *e, ccc_update_fn *fn,
                                        void *aux);

#define ccc_rom_and_modify_w(realtime_ordered_map_entry_ptr, mod_fn,           \
                             aux_data...)                                      \
    &(ccc_romap_entry)                                                         \
    {                                                                          \
        ccc_impl_rom_and_modify_w(realtime_ordered_map_entry_ptr, mod_fn,      \
                                  aux_data)                                    \
    }

ccc_romap_entry ccc_rom_entry(ccc_realtime_ordered_map const *rom,
                              void const *key);

#define ccc_rom_entry_r(realtime_ordered_map_ptr, key_ptr)                     \
    &(ccc_romap_entry)                                                         \
    {                                                                          \
        ccc_rom_entry((realtime_ordered_map_ptr), (key_ptr)).impl_             \
    }

void *ccc_rom_or_insert(ccc_romap_entry const *e, ccc_romap_elem *elem);

#define ccc_rom_or_insert_w(realtime_ordered_map_entry_ptr, lazy_key_value...) \
    ccc_impl_rom_or_insert_w(realtime_ordered_map_entry_ptr, lazy_key_value)

void *ccc_rom_insert_entry(ccc_romap_entry const *e, ccc_romap_elem *elem);

#define ccc_rom_insert_entry_w(realtime_ordered_map_entry_ptr,                 \
                               lazy_key_value...)                              \
    ccc_impl_rom_insert_entry_w(realtime_ordered_map_entry_ptr, lazy_key_value)

void *ccc_rom_unwrap(ccc_romap_entry const *e);

bool ccc_rom_insert_error(ccc_romap_entry const *e);

bool ccc_rom_occupied(ccc_romap_entry const *e);

/*======================      Iteration    ==================================*/

ccc_range ccc_rom_equal_range(ccc_realtime_ordered_map const *rom,
                              void const *begin_key, void const *end_key);

#define ccc_rom_equal_range_r(realtime_ordered_map_ptr,                        \
                              begin_and_end_key_ptrs...)                       \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_rom_equal_range((realtime_ordered_map_ptr),                        \
                            (begin_and_end_key_ptrs))                          \
            .impl_                                                             \
    }

ccc_rrange ccc_rom_equal_rrange(ccc_realtime_ordered_map const *rom,
                                void const *rbegin_key, void const *rend_key);

#define ccc_rom_equal_rrange_r(realtime_ordered_map_ptr,                       \
                               rbegin_and_rend_key_ptrs...)                    \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_rom_equal_rrange((realtime_ordered_map_ptr),                       \
                             (rbegin_and_rend_key_ptrs))                       \
            .impl_                                                             \
    }

void *ccc_rom_begin(ccc_realtime_ordered_map const *rom);

void *ccc_rom_next(ccc_realtime_ordered_map const *rom, ccc_romap_elem const *);

void *ccc_rom_rbegin(ccc_realtime_ordered_map const *rom);

void *ccc_rom_rnext(ccc_realtime_ordered_map const *rom,
                    ccc_romap_elem const *);

void *ccc_rom_end(ccc_realtime_ordered_map const *rom);

void *ccc_rom_rend(ccc_realtime_ordered_map const *rom);

ccc_result ccc_rom_clear(ccc_realtime_ordered_map *rom,
                         ccc_destructor_fn *destructor);

/*======================       Getters     ==================================*/

size_t ccc_rom_size(ccc_realtime_ordered_map const *rom);

bool ccc_rom_is_empty(ccc_realtime_ordered_map const *rom);

bool ccc_rom_validate(ccc_realtime_ordered_map const *rom);

#ifdef REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
typedef ccc_romap_elem romap_elem;
typedef ccc_realtime_ordered_map realtime_ordered_map;
typedef ccc_romap_entry romap_entry;
#    define rom_init(args...) ccc_rom_init(args)
#    define rom_and_modify_w(args...) ccc_rom_and_modify_w(args)
#    define rom_or_insert_w(args...) ccc_rom_or_insert_w(args)
#    define rom_insert_entry_w(args...) ccc_rom_insert_entry_w(args)
#    define rom_try_insert_w(args...) ccc_rom_try_insert_w(args)
#    define rom_insert_or_assign_w(args...) ccc_rom_insert_or_assign_w(args)
#    define rom_insert_r(args...) ccc_rom_insert_r(args)
#    define rom_remove_r(args...) ccc_rom_remove_r(args)
#    define rom_remove_entry_r(args...) ccc_rom_remove_entry_r(args)
#    define rom_entry_r(args...) ccc_rom_entry_r(args)
#    define rom_and_modify_r(args...) ccc_rom_and_modify_r(args)
#    define rom_and_modify_aux_r(args...) ccc_rom_and_modify_aux_r(args)
#    define rom_contains(args...) ccc_rom_contains(args)
#    define rom_get_key_val(args...) ccc_rom_get_key_val(args)
#    define rom_get_mut(args...) ccc_rom_get_mut(args)
#    define rom_insert(args...) ccc_rom_insert(args)
#    define rom_remove(args...) ccc_rom_remove(args)
#    define rom_entry(args...) ccc_rom_entry(args)
#    define rom_remove_entry(args...) ccc_rom_remove_entry(args)
#    define rom_or_insert(args...) ccc_rom_or_insert(args)
#    define rom_insert_entry(args...) ccc_rom_insert_entry(args)
#    define rom_unwrap(args...) ccc_rom_unwrap(args)
#    define rom_unwrap_mut(args...) ccc_rom_unwrap_mut(args)
#    define rom_begin(args...) ccc_rom_begin(args)
#    define rom_next(args...) ccc_rom_next(args)
#    define rom_rbegin(args...) ccc_rom_rbegin(args)
#    define rom_rnext(args...) ccc_rom_rnext(args)
#    define rom_end(args...) ccc_rom_end(args)
#    define rom_rend(args...) ccc_rom_rend(args)
#    define rom_size(args...) ccc_rom_size(args)
#    define rom_is_empty(args...) ccc_rom_is_empty(args)
#    define rom_clear(args...) ccc_rom_clear(args)
#    define rom_validate(args...) ccc_rom_validate(args)
#endif

#endif /* CCC_REALTIME_ORDERED_MAP_H */
