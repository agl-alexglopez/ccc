#ifndef CCC_FLAT_HASH_H
#define CCC_FLAT_HASH_H

#include "impl_flat_hash.h"
#include "types.h"

typedef struct
{
    struct ccc_impl_fhash_elem impl;
} ccc_fhash_elem;

typedef struct
{
    struct ccc_impl_flat_hash impl;
} ccc_flat_hash;

typedef struct
{
    struct ccc_impl_fhash_entry impl;
} ccc_flat_hash_entry;

#define CCC_FHASH_ENTRY(fhash_ptr, struct_name, struct_key_initializer...)     \
    (ccc_flat_hash_entry)                                                      \
        CCC_IMPL_FHASH_ENTRY((fhash_ptr), struct_name, struct_key_initializer)

#define CCC_FHASH_OR_INSERT(entry_copy, struct_name,                           \
                            struct_key_value_initializer...)                   \
    CCC_IMPL_FHASH_OR_INSERT(entry_copy, struct_name,                          \
                             struct_key_value_initializer)

ccc_result ccc_fhash_init(ccc_flat_hash *, ccc_buf *, size_t hash_elem_offset,
                          ccc_hash_fn *, ccc_eq_fn *, void *aux);
bool ccc_fhash_empty(ccc_flat_hash const *);
bool ccc_fhash_contains(ccc_flat_hash const *, ccc_fhash_elem const *);
void const *ccc_fhash_find(ccc_flat_hash const *, ccc_fhash_elem const *);
ccc_result ccc_fhash_erase(ccc_flat_hash *, ccc_fhash_elem const *);
size_t ccc_fhash_size(ccc_flat_hash const *);

ccc_flat_hash_entry ccc_fhash_entry(ccc_flat_hash *, ccc_fhash_elem *);
void *ccc_fhash_or_insert(ccc_flat_hash_entry, ccc_fhash_elem *);
void *ccc_fhash_and_erase(ccc_flat_hash_entry, ccc_fhash_elem *);

void const *ccc_fhash_begin(ccc_flat_hash const *);
void const *ccc_fhash_next(ccc_flat_hash const *, ccc_fhash_elem const *iter);
void const *ccc_fhash_end(ccc_flat_hash const *);

#endif /* CCC_FLAT_HASH_H */
