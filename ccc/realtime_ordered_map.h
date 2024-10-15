#ifndef CCC_REALTIME_ORDERED_MAP_H
#define CCC_REALTIME_ORDERED_MAP_H

#include <stddef.h>

#include "impl_realtime_ordered_map.h"
#include "types.h"

typedef struct ccc_rtom_elem_ ccc_rtom_elem;

typedef struct ccc_rtom_ ccc_realtime_ordered_map;

typedef union
{
    struct ccc_rtom_entry_ impl_;
} ccc_rtom_entry;

#define ccc_rom_init(struct_name, node_elem_field, key_elem_field, map_name,   \
                     alloc_fn, key_cmp_fn, aux_data)                           \
    ccc_impl_rom_init(struct_name, node_elem_field, key_elem_field, map_name,  \
                      alloc_fn, key_cmp_fn, aux_data)

#define ccc_rom_and_modify_w(realtime_ordered_map_entry_ptr, mod_fn,           \
                             aux_data...)                                      \
    &(ccc_rtom_entry)                                                          \
    {                                                                          \
        ccc_impl_rom_and_modify_w(realtime_ordered_map_entry_ptr, mod_fn,      \
                                  aux_data)                                    \
    }

#define ccc_rom_or_insert_w(realtime_ordered_map_entry_ptr, lazy_key_value...) \
    ccc_impl_rom_or_insert_w(realtime_ordered_map_entry_ptr, lazy_key_value)

#define ccc_rom_insert_entry_w(realtime_ordered_map_entry_ptr,                 \
                               lazy_key_value...)                              \
    ccc_impl_rom_insert_entry_w(realtime_ordered_map_entry_ptr, lazy_key_value)

#define ccc_rom_try_insert_w(realtime_ordered_map_ptr, key, lazy_value...)     \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_rom_try_insert_w(realtime_ordered_map_ptr, key, lazy_value)   \
    }

#define ccc_rom_insert_or_assign_w(realtime_ordered_map_ptr, key,              \
                                   lazy_value...)                              \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_rom_insert_or_assign_w(realtime_ordered_map_ptr, key,         \
                                        lazy_value)                            \
    }

/*=================    Membership and Retrieval    ==========================*/

bool ccc_rom_contains(ccc_realtime_ordered_map const *rom, void const *key);

void *ccc_rom_get_key_val(ccc_realtime_ordered_map const *rom, void const *key);

/*======================      Entry API    ==================================*/

#define ccc_rom_entry_vr(realtime_ordered_map_ptr, key_ptr)                    \
    &(ccc_rtom_entry)                                                          \
    {                                                                          \
        ccc_rom_entry((realtime_ordered_map_ptr), (key_ptr)).impl_             \
    }

#define ccc_rom_insert_vr(realtime_ordered_map_ptr, out_handle_ptr, tmp_ptr)   \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_rom_insert((realtime_ordered_map_ptr), (out_handle_ptr),           \
                       (tmp_ptr))                                              \
            .impl_                                                             \
    }

#define ccc_rom_try_insert_vr(realtime_ordered_map_ptr, out_handle_ptr)        \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_rom_try_insert((realtime_ordered_map_ptr), (out_handle_ptr)).impl_ \
    }

#define ccc_rom_remove_vr(realtime_ordered_map_ptr, out_handle_ptr)            \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_rom_remove((realtime_ordered_map_ptr), (out_handle_ptr)).impl_     \
    }

#define ccc_rom_remove_entry_vr(realtime_ordered_map_entry_ptr)                \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_rom_remove_entry((realtime_ordered_map_entry_ptr)).impl_           \
    }

ccc_entry ccc_rom_insert(ccc_realtime_ordered_map *rom,
                         ccc_rtom_elem *out_handle, void *tmp);

ccc_entry ccc_rom_try_insert(ccc_realtime_ordered_map *rom,
                             ccc_rtom_elem *key_val_handle);

ccc_entry ccc_rom_insert_or_assign(ccc_realtime_ordered_map *rom,
                                   ccc_rtom_elem *key_val_handle);

ccc_entry ccc_rom_remove(ccc_realtime_ordered_map *rom,
                         ccc_rtom_elem *out_handle);

ccc_entry ccc_rom_remove_entry(ccc_rtom_entry const *e);

ccc_rtom_entry *ccc_rom_and_modify(ccc_rtom_entry *e, ccc_update_fn *fn);

ccc_rtom_entry *ccc_rom_and_modify_aux(ccc_rtom_entry *e, ccc_update_fn *fn,
                                       void *aux);

ccc_rtom_entry ccc_rom_entry(ccc_realtime_ordered_map const *rom,
                             void const *key);

void *ccc_rom_or_insert(ccc_rtom_entry const *e, ccc_rtom_elem *elem);

void *ccc_rom_insert_entry(ccc_rtom_entry const *e, ccc_rtom_elem *elem);

void *ccc_rom_unwrap(ccc_rtom_entry const *e);
bool ccc_rom_insert_error(ccc_rtom_entry const *e);
bool ccc_rom_occupied(ccc_rtom_entry const *e);

/*======================      Iteration    ==================================*/

#define ccc_rom_equal_range_vr(realtime_ordered_map_ptr,                       \
                               begin_and_end_key_ptrs...)                      \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_rom_equal_range((realtime_ordered_map_ptr),                        \
                            (begin_and_end_key_ptrs))                          \
            .impl_                                                             \
    }

#define ccc_rom_equal_rrange_vr(realtime_ordered_map_ptr,                      \
                                rbegin_and_rend_key_ptrs...)                   \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_rom_equal_rrange((realtime_ordered_map_ptr),                       \
                             (rbegin_and_rend_key_ptrs))                       \
            .impl_                                                             \
    }

ccc_range ccc_rom_equal_range(ccc_realtime_ordered_map const *rom,
                              void const *begin_key, void const *end_key);
ccc_rrange ccc_rom_equal_rrange(ccc_realtime_ordered_map const *rom,
                                void const *rbegin_key, void const *rend_key);

void *ccc_rom_begin(ccc_realtime_ordered_map const *rom);
void *ccc_rom_next(ccc_realtime_ordered_map const *rom, ccc_rtom_elem const *);

void *ccc_rom_rbegin(ccc_realtime_ordered_map const *rom);
void *ccc_rom_rnext(ccc_realtime_ordered_map const *rom, ccc_rtom_elem const *);

void *ccc_rom_end(ccc_realtime_ordered_map const *rom);
void *ccc_rom_rend(ccc_realtime_ordered_map const *rom);

/*======================       Getters     ==================================*/

size_t ccc_rom_size(ccc_realtime_ordered_map const *rom);
bool ccc_rom_is_empty(ccc_realtime_ordered_map const *rom);

/*=================           Cleanup               =========================*/

ccc_result ccc_rom_clear(ccc_realtime_ordered_map *rom,
                         ccc_destructor_fn *destructor);

/*=================     Utilities and Validation    =========================*/

ccc_result ccc_rom_print(ccc_realtime_ordered_map const *rom, ccc_print_fn *fn);

bool ccc_rom_validate(ccc_realtime_ordered_map const *rom);

void *ccc_rom_root(ccc_realtime_ordered_map const *rom);

#ifdef REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
typedef ccc_rtom_elem rtom_elem;
typedef ccc_realtime_ordered_map realtime_ordered_map;
typedef ccc_rtom_entry rtom_entry;
#    define rom_init(args...) ccc_rom_init(args)
#    define rom_and_modify_w(args...) ccc_rom_and_modify_w(args)
#    define rom_or_insert_w(args...) ccc_rom_or_insert_w(args)
#    define rom_insert_entry_w(args...) ccc_rom_insert_entry_w(args)
#    define rom_try_insert_w(args...) ccc_rom_try_insert_w(args)
#    define rom_insert_or_assign_w(args...) ccc_rom_insert_or_assign_w(args)
#    define rom_insert_vr(args...) ccc_rom_insert_vr(args)
#    define rom_remove_vr(args...) ccc_rom_remove_vr(args)
#    define rom_remove_entry_vr(args...) ccc_rom_remove_entry_vr(args)
#    define rom_entry_vr(args...) ccc_rom_entry_vr(args)
#    define rom_and_modify_vr(args...) ccc_rom_and_modify_vr(args)
#    define rom_and_modify_aux_vr(args...) ccc_rom_and_modify_aux_vr(args)
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
#    define rom_print(args...) ccc_rom_print(args)
#    define rom_validate(args...) ccc_rom_validate(args)
#    define rom_root(args...) ccc_rom_root(args)
#endif

#endif /* CCC_REALTIME_ORDERED_MAP_H */
