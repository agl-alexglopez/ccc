#ifndef CCC_ORDERED_MAP_H
#define CCC_ORDERED_MAP_H

#include "impl_ordered_map.h"
#include "impl_tree.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

/** A ordered map is a self-optimizing data structure offering amortized
O(lg N) search, insert, and erase and pointer stability. Because the data
structure is self-optimizing it is not suitable in a realtime environment where
strict runtime bounds are needed. Also, searching the map is not a const
thread-safe operation as indicated by the function signatures. The map is
optimized upon every new search. However, in many cases the self-optimizing
structure of the map can be beneficial when considering non-uniform access
cases. In the best case, repeated searches of the same value yield an O(1)
access and many other frequently searched values will remain close to the root
of the map. */
typedef union
{
    struct ccc_tree_ impl_;
} ccc_ordered_map;

/** A map element is the intrusive element of the user defined struct being
stored in the map for key value access. */
typedef union
{
    ccc_node_ impl_;
} ccc_omap_elem;

/** A container specific entry used to implement the Entry API. The Entry API
 offers efficient search and subsequent insertion, deletion, or value update
 based on the needs of the user. */
typedef union
{
    struct ccc_tree_entry_ impl_;
} ccc_omap_entry;

#define ccc_om_init(struct_name, set_elem_field, key_elem_field, set_name,     \
                    alloc_fn, key_cmp, aux)                                    \
    ccc_impl_om_init(struct_name, set_elem_field, key_elem_field, set_name,    \
                     alloc_fn, key_cmp, aux)

/*=======================    Lazy Construction   ============================*/

#define ccc_om_and_modify_w(ordered_map_entry_ptr, mod_fn, aux_data...)        \
    &(ccc_omap_entry)                                                          \
    {                                                                          \
        ccc_impl_om_and_modify_w(ordered_map_entry_ptr, mod_fn, aux_data)      \
    }

#define ccc_om_or_insert_w(ordered_map_entry_ptr, lazy_key_value...)           \
    ccc_impl_om_or_insert_w(ordered_map_entry_ptr, lazy_key_value)

#define ccc_om_insert_entry_w(ordered_map_entry_ptr, lazy_key_value...)        \
    ccc_impl_om_insert_entry_w(ordered_map_entry_ptr, lazy_key_value)

#define ccc_om_try_insert_w(ordered_map_ptr, key, lazy_value...)               \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_om_try_insert_w(ordered_map_ptr, key, lazy_value)             \
    }

#define ccc_om_insert_or_assign_w(ordered_map_ptr, key, lazy_value...)         \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_om_insert_or_assign_w(ordered_map_ptr, key, lazy_value)       \
    }

/*=========================   Membership    =================================*/

bool ccc_om_contains(ccc_ordered_map *, void const *key);

void *ccc_om_get_key_val(ccc_ordered_map *s, void const *key);

/*===========================   Entry API   =================================*/

/* Retain access to old values in the map. See types.h for ccc_entry. */

#define ccc_om_insert_r(ordered_map_ptr, out_handle_ptr, tmp_ptr)              \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_om_insert((ordered_map_ptr), (out_handle_ptr), (tmp_ptr)).impl_    \
    }

#define ccc_om_try_insert_r(ordered_map_ptr, out_handle_ptr)                   \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_om_try_insert((ordered_map_ptr), (out_handle_ptr)).impl_           \
    }

#define ccc_om_remove_r(ordered_map_ptr, out_handle_ptr)                       \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_om_remove((ordered_map_ptr), (out_handle_ptr)).impl_               \
    }

#define ccc_om_remove_entry_r(ordered_map_entry_ptr)                           \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_om_remove_entry((ordered_map_entry_ptr)).impl_                     \
    }

ccc_entry ccc_om_insert(ccc_ordered_map *, ccc_omap_elem *key_val_handle,
                        ccc_omap_elem *tmp);

ccc_entry ccc_om_try_insert(ccc_ordered_map *, ccc_omap_elem *key_val_handle);

ccc_entry ccc_om_insert_or_assign(ccc_ordered_map *,
                                  ccc_omap_elem *key_val_handle);

ccc_entry ccc_om_remove(ccc_ordered_map *, ccc_omap_elem *out_handle);

/* Standard Entry API. */

#define ccc_om_entry_r(ordered_map_ptr, key_ptr)                               \
    &(ccc_omap_entry)                                                          \
    {                                                                          \
        ccc_om_entry((ordered_map_ptr), (key_ptr)).impl_                       \
    }

ccc_omap_entry ccc_om_entry(ccc_ordered_map *s, void const *key);

ccc_omap_entry *ccc_om_and_modify(ccc_omap_entry *e, ccc_update_fn *fn);

ccc_omap_entry *ccc_om_and_modify_aux(ccc_omap_entry *e, ccc_update_fn *fn,
                                      void *aux);

void *ccc_om_or_insert(ccc_omap_entry const *e, ccc_omap_elem *elem);

void *ccc_om_insert_entry(ccc_omap_entry const *e, ccc_omap_elem *elem);

ccc_entry ccc_om_remove_entry(ccc_omap_entry *e);

void *ccc_om_unwrap(ccc_omap_entry const *e);
bool ccc_om_insert_error(ccc_omap_entry const *e);
bool ccc_om_occupied(ccc_omap_entry const *e);

/*===========================   Iterators   =================================*/

#define ccc_om_equal_range_r(ordered_map_ptr, begin_and_end_key_ptrs...)       \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_om_equal_range(ordered_map_ptr, begin_and_end_key_ptrs).impl_      \
    }

#define ccc_om_equal_rrange_r(ordered_map_ptr, rbegin_and_rend_key_ptrs...)    \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_om_equal_rrange(ordered_map_ptr, rbegin_and_rend_key_ptrs).impl_   \
    }

ccc_range ccc_om_equal_range(ccc_ordered_map *, void const *begin_key,
                             void const *end_key);

ccc_rrange ccc_om_equal_rrange(ccc_ordered_map *, void const *rbegin_key,
                               void const *end_key);

void *ccc_om_begin(ccc_ordered_map const *);

void *ccc_om_rbegin(ccc_ordered_map const *);

void *ccc_om_end(ccc_ordered_map const *);

void *ccc_om_rend(ccc_ordered_map const *);

void *ccc_om_next(ccc_ordered_map const *, ccc_omap_elem const *);

void *ccc_om_rnext(ccc_ordered_map const *, ccc_omap_elem const *);

void *ccc_om_root(ccc_ordered_map const *);

ccc_result ccc_om_clear(ccc_ordered_map *, ccc_destructor_fn *destructor);

/*===========================     Getters   =================================*/

bool ccc_om_is_empty(ccc_ordered_map const *);

size_t ccc_om_size(ccc_ordered_map const *);

bool ccc_om_validate(ccc_ordered_map const *);

#ifdef ORDERED_MAP_USING_NAMESPACE_CCC
typedef ccc_omap_elem omap_elem;
typedef ccc_ordered_map ordered_map;
typedef ccc_omap_entry omap_entry;
#    define om_init(args...) ccc_om_init(args)
#    define om_and_modify_w(args...) ccc_om_and_modify_w(args)
#    define om_or_insert_w(args...) ccc_om_or_insert_w(args)
#    define om_insert_entry_w(args...) ccc_om_insert_entry_w(args)
#    define om_try_insert_w(args...) ccc_om_try_insert_w(args)
#    define om_insert_or_assign_w(args...) ccc_om_insert_or_assign_w(args)
#    define om_insert_r(args...) ccc_om_insert_r(args)
#    define om_remove_r(args...) ccc_om_remove_r(args)
#    define om_remove_entry_r(args...) ccc_om_remove_entry_r(args)
#    define om_entry_r(args...) ccc_om_entry_r(args)
#    define om_and_modify_r(args...) ccc_om_and_modify_r(args)
#    define om_and_modify_aux_r(args...) ccc_om_and_modify_aux_r(args)
#    define om_contains(args...) ccc_om_contains(args)
#    define om_get_key_val(args...) ccc_om_get_key_val(args)
#    define om_get_mut(args...) ccc_om_get_mut(args)
#    define om_insert(args...) ccc_om_insert(args)
#    define om_remove(args...) ccc_om_remove(args)
#    define om_entry(args...) ccc_om_entry(args)
#    define om_remove_entry(args...) ccc_om_remove_entry(args)
#    define om_or_insert(args...) ccc_om_or_insert(args)
#    define om_insert_entry(args...) ccc_om_insert_entry(args)
#    define om_unwrap(args...) ccc_om_unwrap(args)
#    define om_unwrap_mut(args...) ccc_om_unwrap_mut(args)
#    define om_begin(args...) ccc_om_begin(args)
#    define om_next(args...) ccc_om_next(args)
#    define om_rbegin(args...) ccc_om_rbegin(args)
#    define om_rnext(args...) ccc_om_rnext(args)
#    define om_end(args...) ccc_om_end(args)
#    define om_rend(args...) ccc_om_rend(args)
#    define om_size(args...) ccc_om_size(args)
#    define om_is_empty(args...) ccc_om_is_empty(args)
#    define om_clear(args...) ccc_om_clear(args)
#    define om_validate(args...) ccc_om_validate(args)
#    define om_root(args...) ccc_om_root(args)
#endif

#endif /* CCC_ORDERED_MAP_H */
