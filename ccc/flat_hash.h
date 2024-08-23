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

/** Inserts the specified key and value into the hash table invariantly.
@param[in] h the flat hash table being queried.
@param [in] key the key being queried for insertion.
@param [in] val_handle the handle to the struct being inserted. If a prior
entry exists, It's content will be written to the encasing struct.
@return an empty entry indicates no prior value stored in the table while
an occupied entry contains the prior entry if it existed that is now stored
in the struct wrapping the val_handle provided.

The hash elem handle must point to the embedded handle within the same struct
type the user is storing in the table or the behavior is undefined.

If the key did not exist in the table, an empty entry is returned and any
get methods on it will yeild NULL/false. If a prior entry existed, the
old entry stored in the table slot is swapped into the struct containing
val_handle and the old table slot is overwritten with the new intended
insertion. The old value can be viewed with any entry get methods.

If an insertion error occurs, due to a table resizing failure, a NULL and
vacant entry is returned. Get methods will yeild false/NULL and the
insertion error checking function will evaluate to true.

Example:

    struct val
    {
        int id;
        int val;
        ccc_fhash_elem e;
    };
    ...initializations have already occured
    struct val v = {.id = 99, .val = 137};
    ccc_fhash_entry e = ccc_fh_insert(&h, &v.id, &v.e);
    if (ccc_fh_insert_error(e))
    {
        ...handle inability of table to find new memory
    }
    struct val *old_val = ccc_fh_get(e);
    if (old_val)
    {
        ...examine old value
    } */
ccc_fhash_entry ccc_fh_insert(ccc_fhash *h, void *key,
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
