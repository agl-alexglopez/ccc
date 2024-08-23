#ifndef CCC_MACROS_H
#define CCC_MACROS_H

#include "impl_flat_hash.h"

#define DEFAULT_ERR                                                            \
    ({                                                                         \
        ccc_result _input_err_res_ = CCC_INPUT_ERR;                            \
        _input_err_res_;                                                       \
    })

#define ENTRY(container_ptr, key)                                              \
    _Generic((container_ptr),                                                  \
        ccc_fhash                                                              \
            *: (ccc_fhash_entry){CCC_IMPL_FH_ENTRY((container_ptr), (key))},   \
        default: DEFAULT_ERR)

#define OR_INSERT_WITH(entry_copy, struct_key_value_initializer...)            \
    _Generic((entry_copy),                                                     \
        ccc_fhash_entry: CCC_IMPL_FH_OR_INSERT_WITH(                           \
                 (entry_copy), (struct_key_value_initializer)),                \
        default: DEFAULT_ERR)

#define AND_MODIFY(entry_copy, mod_fn)                                         \
    _Generic((entry_copy),                                                     \
        ccc_fhash_entry: (ccc_fhash_entry){CCC_IMPL_FH_AND_MODIFY(             \
            (entry_copy), (mod_fn))},                                          \
        default: DEFAULT_ERR)

#define AND_MODIFY_WITH(entry_copy, mod_fn, aux)                               \
    _Generic((entry_copy),                                                     \
        ccc_fhash_entry: (ccc_fhash_entry){CCC_IMPL_FH_AND_MODIFY_WITH(        \
            (entry_copy), (mod_fn), (aux))},                                   \
        default: DEFAULT_ERR)

#endif /* CCC_MACROS_H */
