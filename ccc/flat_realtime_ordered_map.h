#ifndef FLAT_REALTIME_ORDERED_MAP_H
#define FLAT_REALTIME_ORDERED_MAP_H

#include "impl_flat_realtime_ordered_map.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct ccc_frm_elem_ ccc_frtm_elem;

typedef struct ccc_frm_ ccc_flat_realtime_ordered_map;

typedef union
{
    struct ccc_frm_entry_ impl_;
} ccc_frtm_entry;

#define ccc_frm_init(memory_ptr, capacity, node_elem_field, key_elem_field,    \
                     alloc_fn, key_cmp_fn, aux_data)                           \
    ccc_impl_frm_init(memory_ptr, capacity, node_elem_field, key_elem_field,   \
                      alloc_fn, key_cmp_fn, aux_data)

#define ccc_frm_and_modify_w(flat_realtime_ordered_map_entry_ptr, mod_fn,      \
                             aux_data...)                                      \
    &(ccc_rtom_entry)                                                          \
    {                                                                          \
        ccc_impl_frm_and_modify_w(flat_realtime_ordered_map_entry_ptr, mod_fn, \
                                  aux_data)                                    \
    }

#define ccc_frm_or_insert_w(flat_realtime_ordered_map_entry_ptr,               \
                            lazy_key_value...)                                 \
    ccc_impl_frm_or_insert_w(flat_realtime_ordered_map_entry_ptr,              \
                             lazy_key_value)

#define ccc_frm_insert_entry_w(flat_realtime_ordered_map_entry_ptr,            \
                               lazy_key_value...)                              \
    ccc_impl_frm_insert_entry_w(flat_realtime_ordered_map_entry_ptr,           \
                                lazy_key_value)

#define ccc_frm_try_insert_w(flat_realtime_ordered_map_ptr, key,               \
                             lazy_value...)                                    \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_frm_try_insert_w(flat_realtime_ordered_map_ptr, key,          \
                                  lazy_value)                                  \
    }

#define ccc_frm_insert_or_assign_w(flat_realtime_ordered_map_ptr, key,         \
                                   lazy_value...)                              \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_frm_insert_or_assign_w(flat_realtime_ordered_map_ptr, key,    \
                                        lazy_value)                            \
    }

/*=================      Membership and Retrieval    ========================*/

bool ccc_frm_contains(ccc_flat_realtime_ordered_map const *frm,
                      void const *key);
void *ccc_frm_get_key_val(ccc_flat_realtime_ordered_map const *frm,
                          void const *key);

/*======================      Entry API    ==================================*/

#define ccc_frm_entry_vr(flat_realtime_ordered_map_ptr, key_ptr)               \
    &(ccc_frtm_entry)                                                          \
    {                                                                          \
        ccc_frm_entry((flat_realtime_ordered_map_ptr), (key_ptr)).impl_        \
    }

#define ccc_frm_insert_vr(flat_realtime_ordered_map_ptr, out_handle_ptr)       \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_frm_insert((flat_realtime_ordered_map_ptr), (out_handle_ptr))      \
            .impl_                                                             \
    }

#define ccc_frm_try_insert_vr(flat_realtime_ordered_map_ptr, out_handle_ptr)   \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_frm_try_insert((flat_realtime_ordered_map_ptr), (out_handle_ptr))  \
            .impl_                                                             \
    }

#define ccc_frm_remove_vr(flat_realtime_ordered_map_ptr, out_handle_ptr)       \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_frm_remove((flat_realtime_ordered_map_ptr), (out_handle_ptr))      \
            .impl_                                                             \
    }

#define ccc_frm_remove_entry_vr(flat_realtime_ordered_map_entry_ptr)           \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_frm_remove_entry((flat_realtime_ordered_map_entry_ptr)).impl_      \
    }

ccc_entry ccc_frm_insert(ccc_flat_realtime_ordered_map *frm,
                         ccc_frtm_elem *out_handle);

ccc_entry ccc_frm_try_insert(ccc_flat_realtime_ordered_map *frm,
                             ccc_frtm_elem *key_val_handle);

ccc_entry ccc_frm_insert_or_assign(ccc_flat_realtime_ordered_map *frm,
                                   ccc_frtm_elem *key_val_handle);

ccc_frtm_entry ccc_frm_entry(ccc_flat_realtime_ordered_map const *frm,
                             void const *key);

ccc_entry ccc_frm_remove(ccc_flat_realtime_ordered_map *frm,
                         ccc_frtm_elem *out_handle);

ccc_entry ccc_frm_remove_entry(ccc_frtm_entry const *e);

ccc_frtm_entry *ccc_frm_and_modify(ccc_frtm_entry *e, ccc_update_fn *fn);

ccc_frtm_entry *ccc_frm_and_modify_aux(ccc_frtm_entry *e, ccc_update_fn *fn,
                                       void *aux);
void *ccc_frm_or_insert(ccc_frtm_entry const *e, ccc_frtm_elem *elem);
void *ccc_frm_insert_entry(ccc_frtm_entry const *e, ccc_frtm_elem *elem);

void *ccc_frm_unwrap(ccc_frtm_entry const *e);
bool ccc_frm_insert_error(ccc_frtm_entry const *e);
bool ccc_frm_occupied(ccc_frtm_entry const *e);

/*======================      Iteration    ==================================*/

#define ccc_frm_equal_range_vr(flat_realtime_ordered_map_ptr,                  \
                               begin_and_end_key_ptrs...)                      \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_frm_equal_range((flat_realtime_ordered_map_ptr),                   \
                            (begin_and_end_key_ptrs))                          \
            .impl_                                                             \
    }

#define ccc_frm_equal_rrange_vr(flat_realtime_ordered_map_ptr,                 \
                                rbegin_and_rend_key_ptrs...)                   \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_frm_equal_rrange((flat_realtime_ordered_map_ptr),                  \
                             (rbegin_and_rend_key_ptrs))                       \
            .impl_                                                             \
    }

ccc_range ccc_frm_equal_range(ccc_flat_realtime_ordered_map const *frm,
                              void const *begin_key, void const *end_key);
ccc_rrange ccc_frm_equal_rrange(ccc_flat_realtime_ordered_map const *frm,
                                void const *rbegin_key, void const *rend_key);

void *ccc_frm_begin(ccc_flat_realtime_ordered_map const *frm);
void *ccc_frm_rbegin(ccc_flat_realtime_ordered_map const *frm);

void *ccc_frm_next(ccc_flat_realtime_ordered_map const *frm,
                   ccc_frtm_elem const *);
void *ccc_frm_rnext(ccc_flat_realtime_ordered_map const *frm,
                    ccc_frtm_elem const *);
void *ccc_frm_end(ccc_flat_realtime_ordered_map const *frm);
void *ccc_frm_rend(ccc_flat_realtime_ordered_map const *frm);

/*======================       Cleanup     ==================================*/

ccc_result ccc_frm_clear(ccc_flat_realtime_ordered_map *frm,
                         ccc_destructor_fn *fn);
ccc_result ccc_frm_clear_and_free(ccc_flat_realtime_ordered_map *frm,
                                  ccc_destructor_fn *fn);

/*======================       Getters     ==================================*/

bool ccc_frm_is_empty(ccc_flat_realtime_ordered_map const *frm);
size_t ccc_frm_size(ccc_flat_realtime_ordered_map const *frm);
bool ccc_frm_validate(ccc_flat_realtime_ordered_map const *frm);
void *ccc_frm_root(ccc_flat_realtime_ordered_map const *frm);
ccc_result ccc_frm_print(ccc_flat_realtime_ordered_map const *frm,
                         ccc_print_fn *fn);

/*======================      Namespace    ==================================*/

#ifdef FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
typedef ccc_frtm_elem frtm_elem;
typedef ccc_flat_realtime_ordered_map flat_realtime_ordered_map;
typedef ccc_frtm_entry frtm_entry;
#    define frm_and_modify_w(args...) ccc_frm_and_modify_w(args)
#    define frm_or_insert_w(args...) ccc_frm_or_insert_w(args)
#    define frm_insert_entry_w(args...) ccc_frm_insert_entry_w(args)
#    define frm_try_insert_w(args...) ccc_frm_try_insert_w(args)
#    define frm_insert_or_assign_w(args...) ccc_frm_insert_or_assign_w(args)
#    define frm_init(args...) ccc_frm_init(args)
#    define frm_contains(args...) ccc_frm_contains(args)
#    define frm_get_key_val(args...) ccc_frm_get_key_val(args)
#    define frm_insert(args...) ccc_frm_insert(args)
#    define frm_begin(args...) ccc_frm_begin(args)
#    define frm_rbegin(args...) ccc_frm_rbegin(args)
#    define frm_next(args...) ccc_frm_next(args)
#    define frm_rnext(args...) ccc_frm_rnext(args)
#    define frm_end(args...) ccc_frm_end(args)
#    define frm_rend(args...) ccc_frm_rend(args)
#    define frm_root(args...) ccc_frm_root(args)
#    define frm_root(args...) ccc_frm_root(args)
#    define frm_is_empty(args...) ccc_frm_is_empty(args)
#    define frm_size(args...) ccc_frm_size(args)
#    define frm_clear(args...) ccc_frm_clear(args)
#    define frm_clear_and_free(args...) ccc_frm_clear_and_free(args)
#    define frm_validate(args...) ccc_frm_validate(args)
#    define frm_print(args...) ccc_frm_print(args)
#endif /* FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC */

#endif /* FLAT_REALTIME_ORDERED_MAP_H */
