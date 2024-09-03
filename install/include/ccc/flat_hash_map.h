#ifndef CCC_FLAT_HASH_MAP_H
#define CCC_FLAT_HASH_MAP_H

#include "impl_flat_hash_map.h"
#include "types.h"

typedef struct
{
    struct ccc_impl_fhash_elem impl;
} ccc_fh_map_elem;

typedef struct
{
    struct ccc_impl_flat_hash impl;
} ccc_flat_hash_map;

typedef struct
{
    struct ccc_impl_fhash_entry impl;
} ccc_fh_map_entry;

/** @brief the initialization helper macro for a hash table. Must be called
at runtime.
@param [in] fhash_ptr the pointer to the flat_hash object.
@param [in] memory_ptr the pointer to the backing buffer array. May be NULL
if the user provides a reallocation function. The buffer will be interpreted
in units of type size that the user intends to store.
@param [in] capacity the starting capacity of the provided buffer or 0 if no
buffer is provided and a reallocation function is given.
@param [in] struct_name the name of the struct type the user stores in the
hash table.
@param [in] key_field the field of the struct used for key storage.
@param [in] fhash_elem_field the name of the field holding the fhash_elem
handle.
@param [in] alloc_fn the reallocation function for resizing or NULL if no
resizing is allowed.
@param [in] hash_fn the ccc_hash_fn function the user desires for the table.
@param [in] key_eq_fn the ccc_key_eq_fn the user intends to use.
@param [in] aux auxilliary data that is needed for key comparison.
@return this macro "returns" a value, a ccc_result to indicate if
initialization is successful or a failure. */
#define CCC_FHM_INIT(fhash_ptr, memory_ptr, capacity, struct_name, key_field,  \
                     fhash_elem_field, alloc_fn, hash_fn, key_eq_fn, aux)      \
    CCC_IMPL_FHM_INIT(fhash_ptr, memory_ptr, capacity, struct_name, key_field, \
                      fhash_elem_field, alloc_fn, hash_fn, key_eq_fn, aux)

#define FHM_GET(fhash_ptr, key) CCC_IMPL_FHM_GET(fhash_ptr, key)

#define FHM_GET_MUT(fhash_ptr, key) CCC_IMPL_FHM_GET_MUT(fhash_ptr, key)

#define FHM_ENTRY(fhash_ptr, key)                                              \
    (ccc_fh_map_entry)                                                         \
    {                                                                          \
        CCC_IMPL_FHM_ENTRY(fhash_ptr, key)                                     \
    }

#define FHM_UNWRAP(entry_copy) CCC_IMPL_FHM_UNWRAP(entry_copy)

#define FHM_UNWRAP_MUT(entry_copy) CCC_IMPL_FHM_UNWRAP_MUT(entry_copy)

#define FHM_AND_MODIFY(entry_copy, mod_fn)                                     \
    (ccc_fh_map_entry)                                                         \
    {                                                                          \
        CCC_IMPL_FHM_AND_MODIFY(entry_copy, mod_fn)                            \
    }

#define FHM_AND_MODIFY_W(entry_copy, mod_fn, aux)                              \
    (ccc_fh_map_entry)                                                         \
    {                                                                          \
        CCC_IMPL_FHM_AND_MODIFY_W(entry_copy, mod_fn, aux)                     \
    }

#define FHM_INSERT_ENTRY(entry_copy, key_val_struct...)                        \
    CCC_IMPL_FHM_INSERT_ENTRY(entry_copy, key_val_struct)

#define FHM_OR_INSERT(entry_copy, key_val_struct...)                           \
    CCC_IMPL_FHM_OR_INSERT(entry_copy, key_val_struct)

/** @brief Searches the table for the presence of key.
@param [in] h the flat hash table to be searched.
@param [in] key pointer to the key matching the key type of the user struct.
@return true if the struct containing key is stored, false if not. */
bool ccc_fhm_contains(ccc_flat_hash_map *h, void const *key);

/** @brief Inserts the specified key and value into the hash table invariantly.
@param [in] h the flat hash table being queried.
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
ccc_fh_map_entry ccc_fhm_insert(ccc_flat_hash_map *h,
                                ccc_fh_map_elem *out_handle);

/** @brief Removes the entry stored at key, writing stored value to output.
@param [in] h the hash table to query.
@param [in] key the key used for the query matching stored key type.
@param [in] the handle to the struct that will be returned from this function.
@return a pointer to the struct wrapping out_handle if a value was present,
NULL if no entry occupied the table at the provided key.
@warning this function's side effect is overwriting the provided struct with
the previous hash table entry if one existed.

This function should be used when one wishes to preserve the old value if
one is present. If such behavior is not needed see the entry API. */
void *ccc_fhm_remove(ccc_flat_hash_map *h, ccc_fh_map_elem *out_handle);

/** @brief Returns a read only reference into the table at entry key.
@param [in] h the flat hash map to search.
@param [in]*key the key to search matching stored key type.
@return a read only view of the table entry if it is present, else NULL. */
void const *ccc_fhm_get(ccc_flat_hash_map *h, void const *key);

/** @brief Returns a reference into the table at entry key.
@param [in] h the flat hash map to search.
@param [in]*key the key to search matching stored key type.
@return a view of the table entry if it is present, else NULL. */
void *ccc_fhm_get_mut(ccc_flat_hash_map *h, void const *key);

/*========================    Entry API    ==================================*/

/** @brief Obtains an entry for the provided key in the table for future use.
@param [in] h the hash table to be searched.
@param [in] key the key used to search the table matching the stored key type.
@return a specialized hash entry for use with other functions in the Entry API.
@warning the contents of an entry should not be examined or modified. Use the
provided functions, only.

An entry is a search result that provides either an Occupied or Vacant entry
in the table. An occupied entry signifies that the search was successful. A
Vacant entry means the search was not successful but we now have a handle to
where in the table such an element should be inserted.

An entry is rarely useful on its own. It should be passed in a functional style
to subsequent calls in the Entry API.*/
ccc_fh_map_entry ccc_fhm_entry(ccc_flat_hash_map *h, void const *key);

/** @brief Modifies the provided entry if it is Occupied.
@param [in] e the entry obtained from an entry function or macro.
@param [in] fn an update function in which the auxilliary argument is unused.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function is intended to make the function chaining in the Entry API more
succinct if the entry will be modified in place based on its own value without
the need of the auxilliary argument a ccc_update_fn can provide. */
ccc_fh_map_entry ccc_fhm_and_modify(ccc_fh_map_entry e, ccc_update_fn *fn);

/** @brief Modifies the provided entry if it is Occupied.
@param [in] e the entry obtained from an entry function or macro.
@param [in] fn an update function that requires auxilliary data.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function makes full use of a ccc_update_fn capability, meaning a complete
ccc_update object will be passed to the update function callback. */
ccc_fh_map_entry ccc_fhm_and_modify_with(ccc_fh_map_entry e, ccc_update_fn *fn,
                                         void *aux);

/** @brief Inserts the struct with handle elem if the entry is Vacant.
@param [in] e the entry obtained via function or macro call.
@param [in] elem the handle to the struct to be inserted to a Vacant entry.
@return a pointer to entry in the table invariantly. NULL on error.

Because this functions takes an entry and inserts if it is Vacant, the only
reason NULL shall be returned is when an insertion error will occur, usually
due to a resizing memory error. This can happen if the table is not allowed
to resize because no reallocation function is provided. */
void *ccc_fhm_or_insert(ccc_fh_map_entry e, ccc_fh_map_elem *elem);

/** @brief Inserts the provided entry invariantly.
@param [in] e the entry returned from a call obtaining an entry.
@param [in] elem a handle to the struct the user intends to insert.
@return a pointer to the inserted element or NULL upon a memory error in which
the load factor would be exceeded when no reallocation policy is defined or
resizing failed to find more memory.

This method can be used when the old value in the table does not need to
be preserved. See the regular insert method if the old value is of interest.
If an error occurs during the insertion process due to memory limitations
or a search error NULL is returned. Otherwise insertion should not fail. */
void *ccc_fhm_insert_entry(ccc_fh_map_entry e, ccc_fh_map_elem *elem);

/** @brief Removes the provided entry if it is Occupied.
@param [in] e the entry to be removed.
@return true if e was Occupied and now has been removed, false if vacant.

This method does nothing to help preserve the old value if one was present. If
preserving the old value is of interest see the remove method. */
bool ccc_fhm_remove_entry(ccc_fh_map_entry e);

/** @brief Unwraps the provided entry to obtain a view into the table element.
@param [in] e the entry from a query to the table via function or macro.
@return an immutable view into the table entry if one is present, or NULL. */
void const *ccc_fhm_unwrap(ccc_fh_map_entry e);

/** @brief Unwraps the provided entry to obtain a view into the table element.
@param [in] e the entry from a query to the table via function or macro.
@return a mutable view into the table entry if one is present, or NULL. */
void *ccc_fhm_unwrap_mut(ccc_fh_map_entry e);

/** @brief Returns the Vacant or Occupied status of the entry.
@param [in] e the entry from a query to the table via function or macro.
@return true if the entry is occupied, false if not. */
bool ccc_fhm_occupied(ccc_fh_map_entry e);

/** @brief Provides the status of the entry should an insertion follow.
@param [in] e the entry from a query to the table via function or macro.
@return true if the next insertion of a new element will cause an error.

Table resizing occurs upon calls to entry functions/macros or when trying
to insert a new element directly. This is to provide stable entries from the
time they are obtained to the time they are used in functions they are passed
to (i.e. the idiomatic OR_INSERT(ENTRY(...)...)).

However, if a Vacant entry is returned and then a subsequent insertion function
is used, it will not work if resizing has failed and the return of those
functions will indicate such a failure. One can also confirm an insertion error
will occur from an entry with this function. */
bool ccc_fhm_insert_error(ccc_fh_map_entry e);

/*==============================   Iteration    =============================*/

/** @brief Obtains a pointer to the first element in the table.
@param [in] h the table to iterate through.
@return a pointer that can be cast directly to the user type that is stored.
@warning erasing or inserting during iteration may invalidate iterators if
resizing occurs which would lead to undefined behavior. O(capacity).

Iteration starts from index 0 by capacity of the table so iteration order is
not obvious to the user, nor should any specific order be relied on. */
void *ccc_fhm_begin(ccc_flat_hash_map const *h);

/** @brief Advances the iterator to the next occupied table slot.
@param [in] h the table being iterated upon.
@param [in] iter the previous iterator.
@return a pointer that can be cast directly to the user type that is stored.
@warning erasing or inserting during iteration may invalidate iterators if
resizing occurs which would lead to undefined behavior. O(capacity). */
void *ccc_fhm_next(ccc_flat_hash_map const *h, ccc_fh_map_elem const *iter);

/** @brief Check the current iterator against the end for loop termination.
@param [in] h the table being iterated upon.
@return the end address of the hash table.
@warning It is undefined behavior to access or modify the end address. */
void *ccc_fhm_end(ccc_flat_hash_map const *h);

/** @brief Returns the size status of the table.
@param [in] h the hash table.
@return true if empty else false. */
bool ccc_fhm_empty(ccc_flat_hash_map const *h);

/** @brief Returns the size of the table.
@param [in] h the hash table.
@return the size_t size. */
size_t ccc_fhm_size(ccc_flat_hash_map const *h);

/** @brief Frees all slots in the table for use without affecting capacity.
@param [in] h the table to be cleared.
@param [in] fn the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the table before their slots are
forfeit.

If NULL is passed as the destructor function time is O(1), else O(capacity). */
void ccc_fhm_clear(ccc_flat_hash_map *h, ccc_destructor_fn *fn);

/** @brief Frees all slots in the table and frees the underlying buffer.
@param [in] h the table to be cleared.
@param [in] fn the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the table before their slots are
forfeit.
@return the result of free operation. If no realloc function is provided it is
an error to attempt to free the buffer and a memory error is returned.
Otherwise, an OK result is returned. */
ccc_result ccc_fhm_clear_and_free(ccc_flat_hash_map *, ccc_destructor_fn *);

/** @brief Helper to find a prime number if needed.
@param [in] n the input that may or may not be prime.
@return the next larger prime number.

It is possible to use this hash table without an allocator by providing the
buffer to be used for the underlying storage and preventing reallocation.
If such a backing store is used it would be best to ensure it is a prime number
size to mitigate hash collisions. */
size_t ccc_fhm_next_prime(size_t n);

/** @brief Return the full capacity of the backing storage.
@param [in] h the hash table.
@return the capacity. */
size_t ccc_fhm_capacity(ccc_flat_hash_map const *h);

/** @brief Print all elements in the table as defined by the provided printer.
@param [in] h the hash table
@param [in] the printer function for the type stored by the user.

This function will only print the occupied slots in the table. O(capacity) */
void ccc_fhm_print(ccc_flat_hash_map const *h, ccc_print_fn *fn);

/** @brief Validation of invariants for the hash table.
@param [in] h the table to validate.
@return true if all invariants hold, false if corruption occurs. */
bool ccc_fhm_validate(ccc_flat_hash_map const *h);

#endif /* CCC_FLAT_HASH_MAP_H */
