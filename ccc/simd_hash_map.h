#ifndef CCC_SIMD_HASH_MAP_H
#define CCC_SIMD_HASH_MAP_H

#include "impl/impl_simd_hash_map.h"

#define ccc_shm_declare_fixed_map(fixed_map_type_name, key_val_type_name,      \
                                  capacity)                                    \
    ccc_impl_shm_declare_fixed_map(fixed_map_type_name, key_val_type_name,     \
                                   capacity)

#endif /* CCC_SIMD_HASH_MAP_H */
