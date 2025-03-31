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

[[nodiscard]] ccc_tribool ccc_fhm_contains(ccc_flat_hash_map const *h,
                                           void const *key);
[[nodiscard]] ccc_fhmap_entry ccc_fhm_entry(ccc_flat_hash_map *h,
                                            void const *key);
[[nodiscard]] void *ccc_fhm_insert_entry(ccc_fhmap_entry const *e,
                                         void const *key_val_type);
[[nodiscard]] ccc_entry ccc_fhm_remove_entry(ccc_fhmap_entry const *e);

#endif /* CCC_FLAT_HASH_MAP_H */
