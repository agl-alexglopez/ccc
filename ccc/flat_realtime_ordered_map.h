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

#define ccc_frm_init(memory_ptr, capacity, struct_name, node_elem_field,       \
                     key_elem_field, alloc_fn, key_cmp_fn, aux_data)           \
    ccc_impl_frm_init(memory_ptr, capacity, struct_name, node_elem_field,      \
                      key_elem_field, alloc_fn, key_cmp_fn, aux_data)

/*=================      Membership and Retrieval    ========================*/

bool ccc_frm_contains(ccc_flat_realtime_ordered_map const *frm,
                      void const *key);
void *ccc_frm_get_key_val(ccc_flat_realtime_ordered_map const *frm,
                          void const *key);

/*======================      Entry API    ==================================*/

#define ccc_frm_insert_vr(flat_realtime_ordered_map_ptr, out_handle_ptr,       \
                          tmp_ptr)                                             \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_frm_insert((flat_realtime_ordered_map_ptr), (out_handle_ptr),      \
                       (tmp_ptr))                                              \
            .impl_                                                             \
    }

ccc_entry ccc_frm_insert(ccc_flat_realtime_ordered_map *frm,
                         ccc_frtm_elem *out_handle, void *tmp);

ccc_entry ccc_frm_try_insert(ccc_flat_realtime_ordered_map *rom,
                             ccc_frtm_elem *key_val_handle);

ccc_entry ccc_frm_insert_or_assign(ccc_flat_realtime_ordered_map *rom,
                                   ccc_frtm_elem *key_val_handle);

/*======================      Iteration    ==================================*/

void *ccc_frm_begin(ccc_flat_realtime_ordered_map const *frm);
void *ccc_frm_rbegin(ccc_flat_realtime_ordered_map const *frm);

void *ccc_frm_next(ccc_flat_realtime_ordered_map const *frm,
                   ccc_frtm_elem const *);
void *ccc_frm_rnext(ccc_flat_realtime_ordered_map const *frm,
                    ccc_frtm_elem const *);
void *ccc_frm_end(ccc_flat_realtime_ordered_map const *frm);
void *ccc_frm_rend(ccc_flat_realtime_ordered_map const *frm);

/*======================       Getters     ==================================*/

bool ccc_frm_empty(ccc_flat_realtime_ordered_map const *frm);
size_t ccc_frm_size(ccc_flat_realtime_ordered_map const *frm);
bool ccc_frm_validate(ccc_flat_realtime_ordered_map const *frm);
void *ccc_frm_root(ccc_flat_realtime_ordered_map const *frm);
void ccc_frm_print(ccc_flat_realtime_ordered_map const *frm, ccc_print_fn *fn);

/*======================      Namespace    ==================================*/

#ifdef FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
typedef ccc_frtm_elem frtm_elem;
typedef ccc_flat_realtime_ordered_map flat_realtime_ordered_map;
typedef ccc_frtm_entry frtm_entry;
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
#    define frm_empty(args...) ccc_frm_empty(args)
#    define frm_size(args...) ccc_frm_size(args)
#    define frm_validate(args...) ccc_frm_validate(args)
#    define frm_print(args...) ccc_frm_print(args)
#endif /* FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC */

#endif /* FLAT_REALTIME_ORDERED_MAP_H */
