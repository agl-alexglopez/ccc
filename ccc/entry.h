#ifndef CCC_ENTRY_H
#define CCC_ENTRY_H

#include "impl_flat_hash.h"

#define ENTRY(container_ptr, key)                                              \
    _Generic((container_ptr),                                                  \
        ccc_fhash                                                              \
            *: (ccc_fhash_entry){CCC_IMPL_FH_ENTRY((container_ptr), (key))})

#define AND_MODIFY(entry, mod_fn)                                              \
    _Generic((entry),                                                          \
        ccc_fhash_entry: (ccc_fhash_entry){                                    \
            CCC_IMPL_FH_AND_MODIFY((entry), (mod_fn))})

#define AND_MODIFY_WITH(entry, mod_fn, aux)                                    \
    _Generic((entry),                                                          \
        ccc_fhash_entry: (ccc_fhash_entry){                                    \
            CCC_IMPL_FH_AND_MODIFY_WITH((entry), (mod_fn), (aux))})

#define INSERT_ENTRY(entry, struct_key_value_initializer...)                   \
    _Generic((entry),                                                          \
        ccc_fhash_entry: CCC_IMPL_FH_INSERT_ENTRY(                             \
                 (entry), (struct_key_value_initializer)))

#define OR_INSERT(entry, struct_key_value_initializer...)                      \
    _Generic((entry),                                                          \
        ccc_fhash_entry: CCC_IMPL_FH_OR_INSERT(                                \
                 (entry), (struct_key_value_initializer)))

#endif /* CCC_ENTRY_H */
