#ifndef CCC_FLAT_HASH_H
#define CCC_FLAT_HASH_H

#include "impl_flat_hash.h"

typedef struct
{
    ccc_hash_value impl;
} ccc_fhash_elem;

typedef struct
{
    struct ccc_impl_flat_hash impl;
} ccc_flat_hash;

ccc_result ccc_fhash_init(ccc_flat_hash *, ccc_buf *, size_t hash_elem_offset,
                          ccc_hash_fn *, ccc_eq_fn *, void *aux);
bool ccc_fhash_empty(ccc_flat_hash const *);
bool ccc_fhash_contains(ccc_flat_hash const *, void const *);
void const *ccc_fhash_find(ccc_flat_hash const *, void const *);
ccc_result ccc_fhash_insert(ccc_flat_hash *, void const *);
ccc_result ccc_fhash_erase(ccc_flat_hash *, void const *);
size_t ccc_fhash_size(ccc_flat_hash const *);

void const *ccc_fhash_begin(ccc_flat_hash const *);
void const *ccc_fhash_next(ccc_flat_hash const *, void const *iter);
void const *ccc_fhash_end(ccc_flat_hash const *);

#endif /* CCC_FLAT_HASH_H */
