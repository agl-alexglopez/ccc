#ifndef CCC_ENTRY_H
#define CCC_ENTRY_H

#include "impl_flat_hash.h"

#define ENTRY(container_ptr, key)                                              \
    _Generic((container_ptr),                                                  \
        ccc_fhash                                                              \
            *: (ccc_fhash_entry){CCC_IMPL_FH_ENTRY((container_ptr), (key))})

#define AND_MODIFY(entry_copy, mod_fn)                                         \
    _Generic((entry_copy),                                                     \
        ccc_fhash_entry: (ccc_fhash_entry){                                    \
            CCC_IMPL_FH_AND_MODIFY((entry_copy), (mod_fn))})

#define AND_MODIFY_WITH(entry_copy, mod_fn, aux)                               \
    _Generic((entry_copy),                                                     \
        ccc_fhash_entry: (ccc_fhash_entry){                                    \
            CCC_IMPL_FH_AND_MODIFY_WITH((entry_copy), (mod_fn), (aux))})

#define OR_INSERT_WITH(entry_copy, struct_key_value_initializer...)            \
    _Generic((entry_copy),                                                     \
        ccc_fhash_entry: CCC_IMPL_FH_OR_INSERT_WITH(                           \
                 (entry_copy), (struct_key_value_initializer)))

#endif /* CCC_ENTRY_H */
