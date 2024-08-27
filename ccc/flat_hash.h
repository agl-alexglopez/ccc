#ifndef CCC_FLAT_HASH_H
#define CCC_FLAT_HASH_H

#include "impl_flat_hash.h"
#include "types.h"
/* All containers transitively include the ENTRY API */
#include "entry.h" /* NOLINT */

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

#define CCC_FH_INIT(fhash_ptr, memory_ptr, capacity, struct_name, key_field,   \
                    fhash_elem_field, realloc_fn, hash_fn, key_cmp_fn, aux)    \
    CCC_IMPL_FH_INIT(fhash_ptr, memory_ptr, capacity, struct_name, key_field,  \
                     fhash_elem_field, realloc_fn, hash_fn, key_cmp_fn, aux)

size_t ccc_fh_next_prime(size_t);
void *ccc_fh_buf_base(ccc_fhash const *);
size_t ccc_fh_capacity(ccc_fhash const *);
void ccc_fh_clear(ccc_fhash *, ccc_destructor_fn *);
ccc_result ccc_fh_clear_and_free(ccc_fhash *, ccc_destructor_fn *);

bool ccc_fh_empty(ccc_fhash const *);
size_t ccc_fh_size(ccc_fhash const *);

bool ccc_fh_contains(ccc_fhash *, void const *key);

/** @brief Inserts the specified key and value into the hash table invariantly.
@param[in] h the flat hash table being queried.
@param [in] key the key being queried for insertion matching stored key type.
@param [in] out_handle the handle to the struct inserted with the value.
If a prior entry exists, It's content will be written to this container.
@return an empty entry indicates no prior value was stored in the table. An
occupied entry now points to the new value in the table. The old value
has been written to the out_handle containing struct.
@warning this function's side effect is overwriting the provided struct with
the previous hash table entry if one existed.

The hash elem handle must point to the embedded handle within the same struct
type the user is storing in the table or the behavior is undefined.

If the key did not exist in the table, an empty entry is returned and any
get methods on it will yeild NULL/false. If a prior entry existed, the
old entry from the table slot is swapped into the struct containing
val_handle and the old table slot is overwritten with the new intended
insertion. The new value in the table is returned as the entry. If such copy
behavior is not needed consider using the entry api.

If an insertion error occurs, due to a table resizing failure, a NULL and
vacant entry is returned. Get methods will yeild false/NULL and the
insertion error checking function will evaluate to true. */
ccc_fhash_entry ccc_fh_insert(ccc_fhash *h, void *key,
                              ccc_fhash_elem *out_handle);

/** @brief Removes the entry stored at key, writing stored value to output.
@param[in] h the hash table to query.
@param[in] key the key used for the query matching stored key type.
@param[in] the handle to the struct that will be returned from this function.
@return a pointer to the struct wrapping out_handle if a value was present,
NULL if no entry occupied the table at the provided key.

This function should be used when one wishes to preserve the old value if
one is present. If such behavior is not needed see the entry API. */
void *ccc_fh_remove(ccc_fhash *h, void *key, ccc_fhash_elem *out_handle);

/** @brief Inserts the provided entry invariantly.
@param[in] e the entry returned from a call obtaining an entry.
@param[in] elem a handle to the struct the user intends to insert.
@return a pointer to the inserted element in the table or NULL upon error.

This method can be used when the old value in the table does not need to
be preserved. See the regular insert method if the old value is of interest.
If an error occurs during the insertion process due to memory limitations
or a search error NULL is returned. Otherwise insertion should not fail. */
void *ccc_fh_insert_entry(ccc_fhash_entry e, ccc_fhash_elem *elem);

/** @brief Removes the provided entry if it is Occupied.
@param[in] e the entry to be removed.
@return true if e was Occupied and now has been removed, false if vacant.

This method does nothing to help preserve the old value if one was present. If
preserving the old value is of interest see the remove method. */
bool ccc_fh_remove_entry(ccc_fhash_entry e);

ccc_fhash_entry ccc_fh_entry(ccc_fhash *, void const *key);
ccc_fhash_entry ccc_fh_and_modify(ccc_fhash_entry, ccc_update_fn *);
ccc_fhash_entry ccc_fh_and_modify_with(ccc_fhash_entry, ccc_update_fn *,
                                       void *aux);
void *ccc_fh_or_insert(ccc_fhash_entry, ccc_fhash_elem *elem);
void const *ccc_fh_get(ccc_fhash_entry);
void *ccc_fh_get_mut(ccc_fhash_entry);
bool ccc_fh_occupied(ccc_fhash_entry);
bool ccc_fh_insert_error(ccc_fhash_entry);

void const *ccc_fh_begin(ccc_fhash const *);
void const *ccc_fh_next(ccc_fhash const *, ccc_fhash_elem const *iter);
void const *ccc_fh_end(ccc_fhash const *);

void ccc_fh_print(ccc_fhash const *, ccc_print_fn *);

#endif /* CCC_FLAT_HASH_H */
