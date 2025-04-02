#ifndef CCC_FLAT_HASH_MAP_H
#define CCC_FLAT_HASH_MAP_H

#include "impl/impl_flat_hash_map.h"
#include "types.h"

typedef union ccc_fhmap_entry_ ccc_fhmap_entry;
typedef struct ccc_fhmap_ ccc_flat_hash_map;

#define ccc_fhm_declare_fixed_map(fixed_map_type_name, key_val_type_name,      \
                                  capacity)                                    \
    ccc_impl_fhm_declare_fixed_map(fixed_map_type_name, key_val_type_name,     \
                                   capacity)

#define ccc_fhm_init(data_ptr, tag_ptr, key_field, hash_fn, key_eq_fn,         \
                     alloc_fn, aux_data, capacity)                             \
    ccc_impl_fhm_init(data_ptr, tag_ptr, key_field, hash_fn, key_eq_fn,        \
                      alloc_fn, aux_data, capacity)

ccc_result ccc_fhm_copy(ccc_flat_hash_map *dst, ccc_flat_hash_map const *src,
                        ccc_alloc_fn *fn);

[[nodiscard]] ccc_tribool ccc_fhm_contains(ccc_flat_hash_map const *h,
                                           void const *key);
[[nodiscard]] void *ccc_fhm_get_key_val(ccc_flat_hash_map const *h,
                                        void const *key);
[[nodiscard]] ccc_fhmap_entry ccc_fhm_entry(ccc_flat_hash_map *h,
                                            void const *key);
[[nodiscard]] void *ccc_fhm_or_insert(ccc_fhmap_entry const *e,
                                      void const *key_val_type);
[[nodiscard]] void *ccc_fhm_insert_entry(ccc_fhmap_entry const *e,
                                         void const *key_val_type);
[[nodiscard]] ccc_entry ccc_fhm_remove_entry(ccc_fhmap_entry const *e);
[[nodiscard]] ccc_fhmap_entry *ccc_fhm_and_modify(ccc_fhmap_entry *e,
                                                  ccc_update_fn *fn);
[[nodiscard]] ccc_fhmap_entry *
ccc_fhm_and_modify_aux(ccc_fhmap_entry *e, ccc_update_fn *fn, void *aux);
[[nodiscard]]
ccc_entry ccc_fhm_swap_entry(ccc_flat_hash_map *h, void *key_val_type_output);
[[nodiscard]]
ccc_entry ccc_fhm_try_insert(ccc_flat_hash_map *h, void *key_val_type);
[[nodiscard]]
ccc_entry ccc_fhm_insert_or_assign(ccc_flat_hash_map *h, void *key_val_type);
[[nodiscard]] ccc_entry ccc_fhm_remove(ccc_flat_hash_map *h,
                                       void *key_val_type_output);

[[nodiscard]] void *ccc_fhm_begin(ccc_flat_hash_map const *h);
[[nodiscard]] void *ccc_fhm_next(ccc_flat_hash_map const *h,
                                 void *key_val_type_iter);
[[nodiscard]] void *ccc_fhm_end(ccc_flat_hash_map const *h);

ccc_result ccc_fhm_clear(ccc_flat_hash_map *h, ccc_destructor_fn *fn);
ccc_result ccc_fhm_clear_and_free(ccc_flat_hash_map *h, ccc_destructor_fn *fn);

[[nodiscard]] void *ccc_fhm_unwrap(ccc_fhmap_entry const *e);
[[nodiscard]] ccc_tribool ccc_fhm_occupied(ccc_fhmap_entry const *e);
[[nodiscard]] ccc_tribool ccc_fhm_insert_error(ccc_fhmap_entry const *e);
[[nodiscard]] ccc_handle_status ccc_fhm_handle_status(ccc_fhmap_entry const *e);

[[nodiscard]] ccc_tribool ccc_fhm_is_empty(ccc_flat_hash_map const *h);
[[nodiscard]] ccc_ucount ccc_fhm_size(ccc_flat_hash_map const *h);
[[nodiscard]] ccc_ucount ccc_fhm_capacity(ccc_flat_hash_map const *h);
[[nodiscard]] void *ccc_fhm_data(ccc_flat_hash_map const *h);

#endif /* CCC_FLAT_HASH_MAP_H */
