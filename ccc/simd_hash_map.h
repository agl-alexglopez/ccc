#ifndef CCC_SIMD_HASH_MAP_H
#define CCC_SIMD_HASH_MAP_H

#include "impl/impl_simd_hash_map.h"
#include "types.h"

typedef union ccc_shmap_entry_ ccc_shmap_entry;
typedef struct ccc_shmap_ ccc_simd_hash_map;

#define ccc_shm_declare_fixed_map(fixed_map_type_name, key_val_type_name,      \
                                  capacity)                                    \
    ccc_impl_shm_declare_fixed_map(fixed_map_type_name, key_val_type_name,     \
                                   capacity)

#define ccc_shm_init(data_ptr, meta_ptr, key_field, hash_fn, key_eq_fn,        \
                     alloc_fn, aux_data, capacity)                             \
    ccc_impl_shm_init(data_ptr, meta_ptr, key_field, hash_fn, key_eq_fn,       \
                      alloc_fn, aux_data, capacity)

[[nodiscard]] ccc_shmap_entry ccc_shm_entry(ccc_simd_hash_map *h,
                                            void const *key);
[[nodiscard]] ccc_entry ccc_shm_insert_entry(ccc_shmap_entry *h,
                                             void const *key_val_type);

#endif /* CCC_SIMD_HASH_MAP_H */
