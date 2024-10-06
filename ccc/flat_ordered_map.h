#ifndef CCC_FLAT_ORDERED_MAP_H
#define CCC_FLAT_ORDERED_MAP_H

#include "impl_flat_ordered_map.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct ccc_fom_ ccc_flat_ordered_map;

typedef struct ccc_fom_elem_ ccc_f_om_elem;

typedef union
{
    struct ccc_fom_entry_ impl_;
} ccc_f_om_entry;

#define ccc_fom_init(mem_ptr, capacity, map_elem_field, key_elem_field,        \
                     alloc_fn, key_cmp, aux)                                   \
    ccc_impl_fom_init(mem_ptr, capacity, map_elem_field, key_elem_field,       \
                      alloc_fn, key_cmp, aux)

#define ccc_fom_and_modify_w(flat_ordered_map_entry_ptr, mod_fn, aux_data...)  \
    &(ccc_f_om_entry)                                                          \
    {                                                                          \
        ccc_impl_fom_and_modify_w(flat_ordered_map_entry_ptr, mod_fn,          \
                                  aux_data)                                    \
    }

#define ccc_fom_or_insert_w(flat_ordered_map_entry_ptr, lazy_key_value...)     \
    ccc_impl_fom_or_insert_w(flat_ordered_map_entry_ptr, lazy_key_value)

#define ccc_fom_insert_entry_w(flat_ordered_map_entry_ptr, lazy_key_value...)  \
    ccc_impl_fom_insert_entry_w(flat_ordered_map_entry_ptr, lazy_key_value)

#define ccc_fom_try_insert_w(flat_ordered_map_ptr, key, lazy_value...)         \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_fom_try_insert_w(flat_ordered_map_ptr, key, lazy_value)       \
    }

#define ccc_fom_insert_or_assign_w(flat_ordered_map_ptr, key, lazy_value...)   \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_fom_insert_or_assign_w(flat_ordered_map_ptr, key, lazy_value) \
    }

bool ccc_fom_contains(ccc_flat_ordered_map *, void const *key);
void *ccc_fom_get_key_val(ccc_flat_ordered_map *, void const *key);

/*===========================   Entry API   =================================*/

#define ccc_fom_insert_vr(flat_ordered_map_ptr, out_handle_ptr, tmp_ptr)       \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fom_insert((flat_ordered_map_ptr), (out_handle_ptr), (tmp_ptr))    \
            .impl_                                                             \
    }

#define ccc_fom_try_insert_vr(flat_ordered_map_ptr, out_handle_ptr)            \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fom_try_insert((flat_ordered_map_ptr), (out_handle_ptr)).impl_     \
    }

#define ccc_fom_remove_vr(flat_ordered_map_ptr, out_handle_ptr)                \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fom_remove((flat_ordered_map_ptr), (out_handle_ptr)).impl_         \
    }

#define ccc_fom_remove_entry_vr(flat_ordered_map_entry_ptr)                    \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fom_remove_entry((flat_ordered_map_entry_ptr)).impl_               \
    }

ccc_entry ccc_fom_insert(ccc_flat_ordered_map *, ccc_f_om_elem *out_handle,
                         void *tmp);
ccc_entry ccc_fom_try_insert(ccc_flat_ordered_map *,
                             ccc_f_om_elem *key_val_handle);
ccc_entry ccc_fom_insert_or_assign(ccc_flat_ordered_map *,
                                   ccc_f_om_elem *key_val_handle);
ccc_entry ccc_fom_remove(ccc_flat_ordered_map *, ccc_f_om_elem *out_handle);
ccc_entry ccc_fom_remove_entry(ccc_f_om_entry *e);

/* Standard Entry API. */

#define ccc_fom_entry_vr(flat_ordered_map_ptr, key_ptr)                        \
    &(ccc_f_om_entry)                                                          \
    {                                                                          \
        ccc_fom_entry((flat_ordered_map_ptr), (key_ptr)).impl_                 \
    }

ccc_f_om_entry ccc_fom_entry(ccc_flat_ordered_map *fom, void const *key);

ccc_f_om_entry *ccc_fom_and_modify(ccc_f_om_entry *e, ccc_update_fn *fn);

ccc_f_om_entry *ccc_fom_and_modify_aux(ccc_f_om_entry *e, ccc_update_fn *fn,
                                       void *aux);

void *ccc_fom_or_insert(ccc_f_om_entry const *e, ccc_f_om_elem *elem);

void *ccc_fom_insert_entry(ccc_f_om_entry const *e, ccc_f_om_elem *elem);

void *ccc_fom_unwrap(ccc_f_om_entry const *e);
bool ccc_fom_insert_error(ccc_f_om_entry const *e);
bool ccc_fom_occupied(ccc_f_om_entry const *e);

void ccc_fom_clear(ccc_flat_ordered_map *frm, ccc_destructor_fn *fn);
ccc_result ccc_fom_clear_and_free(ccc_flat_ordered_map *frm,
                                  ccc_destructor_fn *fn);

/*===========================   Iterators   =================================*/

void *ccc_fom_begin(ccc_flat_ordered_map const *);
void *ccc_fom_rbegin(ccc_flat_ordered_map const *);
void *ccc_fom_end(ccc_flat_ordered_map const *);
void *ccc_fom_rend(ccc_flat_ordered_map const *);
void *ccc_fom_next(ccc_flat_ordered_map const *, ccc_f_om_elem const *);
void *ccc_fom_rnext(ccc_flat_ordered_map const *, ccc_f_om_elem const *);
ccc_range ccc_fom_equal_range(ccc_flat_ordered_map *fom, void const *begin_key,
                              void const *end_key);
ccc_rrange ccc_fom_equal_rrange(ccc_flat_ordered_map *fom,
                                void const *rbegin_key, void const *end_key);
void *ccc_fom_root(ccc_flat_ordered_map const *);

/*===========================    Getters    =================================*/

size_t ccc_fom_size(ccc_flat_ordered_map const *);
bool ccc_fom_empty(ccc_flat_ordered_map const *);
bool ccc_fom_validate(ccc_flat_ordered_map const *);
void ccc_fom_print(ccc_flat_ordered_map const *fom, ccc_print_fn *fn);

/*===========================    Namespace  =================================*/

#ifdef FLAT_ORDERED_MAP_USING_NAMESPACE_CCC
typedef ccc_flat_ordered_map flat_ordered_map;
typedef ccc_f_om_elem f_om_elem;
typedef ccc_f_om_entry f_om_entry;
#    define fom_and_modify_w(args...) ccc_fom_and_modify_w(args)
#    define fom_or_insert_w(args...) ccc_fom_or_insert_w(args)
#    define fom_insert_entry_w(args...) ccc_fom_insert_entry_w(args)
#    define fom_try_insert_w(args...) ccc_fom_try_insert_w(args)
#    define fom_insert_or_assign_w(args...) ccc_fom_insert_or_assign_w(args)
#    define fom_init(args...) ccc_fom_init(args)
#    define fom_contains(args...) ccc_fom_contains(args)
#    define fom_get_key_val(args...) ccc_fom_get_key_val(args)
#    define fom_insert_vr(args...) ccc_fom_insert_vr(args)
#    define fom_try_insert_vr(args...) ccc_fom_try_insert_vr(args)
#    define fom_remove_vr(args...) ccc_fom_remove_vr(args)
#    define fom_remove_entry_vr(args...) ccc_fom_remove_entry_vr(args)
#    define fom_insert(args...) ccc_fom_insert(args)
#    define fom_try_insert(args...) ccc_fom_try_insert(args)
#    define fom_insert_or_assign(args...) ccc_fom_insert_or_assign(args)
#    define fom_remove(args...) ccc_fom_remove(args)
#    define fom_remove_entry(args...) ccc_fom_remove_entry(args)
#    define fom_entry_vr(args...) ccc_fom_entry_vr(args)
#    define fom_entry(args...) ccc_fom_entry(args)
#    define fom_and_modify(args...) ccc_fom_and_modify(args)
#    define fom_and_modify_aux(args...) ccc_fom_and_modify_aux(args)
#    define fom_or_insert(args...) ccc_fom_or_insert(args)
#    define fom_insert_entry(args...) ccc_fom_insert_entry(args)
#    define fom_unwrap(args...) ccc_fom_unwrap(args)
#    define fom_insert_error(args...) ccc_fom_insert_error(args)
#    define fom_occupied(args...) ccc_fom_occupied(args)
#    define fom_clear(args...) ccc_fom_clear(args)
#    define fom_clear_and_free(args...) ccc_fom_clear_and_free(args)
#    define fom_begin(args...) ccc_fom_begin(args)
#    define fom_rbegin(args...) ccc_fom_rbegin(args)
#    define fom_end(args...) ccc_fom_end(args)
#    define fom_rend(args...) ccc_fom_rend(args)
#    define fom_next(args...) ccc_fom_next(args)
#    define fom_rnext(args...) ccc_fom_rnext(args)
#    define fom_root(args...) ccc_fom_root(args)
#    define fom_size(args...) ccc_fom_size(args)
#    define fom_empty(args...) ccc_fom_empty(args)
#    define fom_validate(args...) ccc_fom_validate(args)
#    define fom_print(args...) ccc_fom_print(args)
#endif /* FLAT_ORDERED_MAP_USING_NAMESPACE_CCC */

#endif /* CCC_FLAT_ORDERED_MAP_H */
