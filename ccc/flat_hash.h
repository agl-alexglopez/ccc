#ifndef CCC_FLAT_HASH_H
#define CCC_FLAT_HASH_H

#include "impl_flat_hash.h"
#include "types.h"
/* All containers transitively include the ENTRY API NOLINTNEXTLINE */
#include "entry.h"

typedef struct
{
    struct ccc_impl_fh_elem impl;
} ccc_fhash_elem;

typedef struct
{
    struct ccc_impl_fhash impl;
} ccc_fhash;

typedef struct
{
    struct ccc_impl_fh_entry impl;
} ccc_fhash_entry;

ccc_result ccc_fh_init(ccc_fhash *, ccc_buf *, size_t hash_elem_offset,
                       ccc_hash_fn *, ccc_key_cmp_fn *, void *aux);
bool ccc_fh_empty(ccc_fhash const *);
size_t ccc_fh_size(ccc_fhash const *);

bool ccc_fh_contains(ccc_fhash *, void const *key);

ccc_fhash_entry ccc_fh_insert(ccc_fhash *, void *key,
                              ccc_fhash_elem *val_handle);

ccc_fhash_entry ccc_fh_entry(ccc_fhash *, void const *key);
ccc_fhash_entry ccc_fh_and_modify(ccc_fhash_entry, ccc_update_fn *);
ccc_fhash_entry ccc_fh_and_modify_with(ccc_fhash_entry, ccc_update_fn *,
                                       void *aux);

void *ccc_fh_or_insert(ccc_fhash_entry, ccc_fhash_elem *elem);
void *ccc_fh_and_erase(ccc_fhash_entry, ccc_fhash_elem *elem);
void const *ccc_fh_get(ccc_fhash_entry);
void *ccc_fh_get_mut(ccc_fhash_entry);
bool ccc_fh_occupied(ccc_fhash_entry);
bool ccc_fh_insert_error(ccc_fhash_entry);

void const *ccc_fh_begin(ccc_fhash const *);
void const *ccc_fh_next(ccc_fhash const *, ccc_fhash_elem const *iter);
void const *ccc_fh_end(ccc_fhash const *);

#endif /* CCC_FLAT_HASH_H */
