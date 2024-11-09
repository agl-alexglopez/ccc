#ifndef CCC_FLAT_HASH_MAP_H
#define CCC_FLAT_HASH_MAP_H

#include <stddef.h>

#include "impl_flat_hash_map.h"
#include "types.h"

/** A flat hash map stores key-value structures defined by the user in
a contiguous buffer. Elements are then retrievable by key in amortized
O(1). Elements in the table may be copied and moved so no pointer
stability is available in this implementation. If pointer stability
is needed, store and hash pointers to those elements in this table.

A flat hash map requires the user to provide a struct with known key
and flat hash element fields as well as a hash function and key
comparator function. The hash function should be well tailored to
the key being stored in the table to prevent collisions. Currently,
the flat hash map does not offer any default hash functions or hash
strengthening algorithms so good hash functions should be used. */
typedef struct ccc_fhm_ ccc_flat_hash_map;

/** A flat hash element is an intrusive element that should be inserted into the
struct used to store the key and value for the user. */
typedef struct ccc_fhm_elem_ ccc_fh_map_elem;

/** A flat hash map entry is the container specific entry used to implement the
 Entry API. The Entry API offers efficient search and subsequent insertion,
 deletion, or value update based on the needs of the user. */
typedef union
{
    struct ccc_fhm_entry_ impl_;
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
@param [in] aux auxilliary data that is needed for hashing or comparison.
@return this macro "returns" a value, a ccc_result to indicate if
initialization is successful or a failure. */
#define ccc_fhm_init(fhash_ptr, memory_ptr, capacity, key_field,               \
                     fhash_elem_field, alloc_fn, hash_fn, key_eq_fn, aux)      \
    ccc_impl_fhm_init(fhash_ptr, memory_ptr, capacity, key_field,              \
                      fhash_elem_field, alloc_fn, hash_fn, key_eq_fn, aux)

/** @brief modify the value stored in the map entry with a modification
function and lazily constructed auxiliary data.
@param [in] flat_hash_map_entry_ptr a pointer to the obtained entry.
@param [in] mod_fn the function used to modify the entry value.
@param [in] aux lazily constructed auxiliary data for mod_fn.
@return a compound literal reference to the modified entry if it was occupied
or a vacant entry if it was vacant.

Note that if aux is a function call to generate a value it will only be called
if the entry is occupied and thus able to be modified. */
#define ccc_fhm_and_modify_w(flat_hash_map_entry_ptr, mod_fn, aux...)          \
    &(ccc_fh_map_entry)                                                        \
    {                                                                          \
        ccc_impl_fhm_and_modify_w(flat_hash_map_entry_ptr, mod_fn, aux)        \
    }

/** @brief lazily insert the desired key value into the entry if it is Vacant.
@param [in] flat_hash_map_entry_ptr a pointer to the obtained entry.
@param [in] lazy_key_value the compound literal to construct in place if the
entry is Vacant.
@return a reference to the unwrapped user type in the entry, either the
unmodified reference if the entry was Occupied or the newly inserted element
if the entry was Vacant. NULL is returned if resizing is required but fails or
is not allowed.

Note that if the compound literal uses any function calls to generate values
or other data, such functions will not be called if the entry is Occupied. */
#define ccc_fhm_or_insert_w(flat_hash_map_entry_ptr, lazy_key_value...)        \
    ccc_impl_fhm_or_insert_w(flat_hash_map_entry_ptr, lazy_key_value)

/** @brief write the contents of the compound literal lazy_key_value to a slot.
@param [in] flat_hash_map_entry_ptr a pointer to the obtained entry.
@param [in] lazy_key_value the compound literal to write to a new slot.
@return a reference to the newly inserted or overwritten user type. NULL is
returned if resizing is required but fails or is not allowed. */
#define ccc_fhm_insert_entry_w(flat_hash_map_entry_ptr, lazy_key_value...)     \
    ccc_impl_fhm_insert_entry_w(flat_hash_map_entry_ptr, lazy_key_value)

/** @brief lazily insert lazy_value into the map at key if key is absent.
@param [in] flat_hash_map_ptr a pointer to the flat hash map.
@param [in] key the direct key r-value.
@param [in] lazy_value the compound literal specifying the value.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define ccc_fhm_try_insert_w(flat_hash_map_ptr, key, lazy_value...)            \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_fhm_try_insert_w(flat_hash_map_ptr, key, lazy_value)          \
    }

/** @brief Inserts a new key value pair or overwrites the existing entry.
@param [in] flat_hash_map_ptr the pointer to the flat hash map.
@param [in] lazy_value the compound literal to insert or use for overwrite.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define ccc_fhm_insert_or_assign_w(flat_hash_map_ptr, key, lazy_value...)      \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_fhm_insert_or_assign_w(flat_hash_map_ptr, key, lazy_value)    \
    }

/** @brief Searches the table for the presence of key.
@param [in] h the flat hash table to be searched.
@param [in] key pointer to the key matching the key type of the user struct.
@return true if the struct containing key is stored, false if not. */
bool ccc_fhm_contains(ccc_flat_hash_map *h, void const *key);

/** @brief Returns a reference into the table at entry key.
@param [in] h the flat hash map to search.
@param [in] key the key to search matching stored key type.
@return a view of the table entry if it is present, else NULL. */
void *ccc_fhm_get_key_val(ccc_flat_hash_map *h, void const *key);

/*========================    Entry API    ==================================*/

/* Preserve old values from stored in the map. See types.h for more. */

#define ccc_fhm_remove_r(flat_hash_map_ptr, out_handle_ptr)                    \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_remove((flat_hash_map_ptr), (out_handle_ptr)).impl_            \
    }

#define ccc_fhm_insert_r(flat_hash_map_ptr, out_handle_ptr)                    \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_insert((flat_hash_map_ptr), (out_handle_ptr)).impl_            \
    }

#define ccc_fhm_try_insert_r(flat_hash_map_ptr, out_handle_ptr)                \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_try_insert((flat_hash_map_ptr), (out_handle_ptr)).impl_        \
    }

#define ccc_fhm_remove_entry_r(flat_hash_map_entry_ptr)                        \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_remove_entry((flat_hash_map_entry_ptr)).impl_                  \
    }

/** TODO */
ccc_entry ccc_fhm_remove(ccc_flat_hash_map *h, ccc_fh_map_elem *out_handle);

/** TODO */
ccc_entry ccc_fhm_insert(ccc_flat_hash_map *h, ccc_fh_map_elem *out_handle);

/** TODO */
ccc_entry ccc_fhm_try_insert(ccc_flat_hash_map *h,
                             ccc_fh_map_elem *key_val_handle);

/** TODO */
ccc_entry ccc_fhm_insert_or_assign(ccc_flat_hash_map *h,
                                   ccc_fh_map_elem *key_val_handle);

/** TODO */
ccc_entry ccc_fhm_remove_entry(ccc_fh_map_entry const *e);

/* Standard Entry API */

#define ccc_fhm_entry_r(flat_hash_map_ptr, key_ptr)                            \
    &(ccc_fh_map_entry)                                                        \
    {                                                                          \
        ccc_fhm_entry((flat_hash_map_ptr), (key_ptr)).impl_                    \
    }

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
ccc_fh_map_entry *ccc_fhm_and_modify(ccc_fh_map_entry *e, ccc_update_fn *fn);

/** @brief Modifies the provided entry if it is Occupied.
@param [in] e the entry obtained from an entry function or macro.
@param [in] fn an update function that requires auxilliary data.
@param [in] aux auxilliary data required for the update.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function makes full use of a ccc_update_fn capability, meaning a complete
ccc_update object will be passed to the update function callback. */
ccc_fh_map_entry *ccc_fhm_and_modify_aux(ccc_fh_map_entry *e, ccc_update_fn *fn,
                                         void *aux);

/** @brief Inserts the struct with handle elem if the entry is Vacant.
@param [in] e the entry obtained via function or macro call.
@param [in] elem the handle to the struct to be inserted to a Vacant entry.
@return a pointer to entry in the table invariantly. NULL on error.

Because this functions takes an entry and inserts if it is Vacant, the only
reason NULL shall be returned is when an insertion error will occur, usually
due to a resizing memory error. This can happen if the table is not allowed
to resize because no reallocation function is provided. */
void *ccc_fhm_or_insert(ccc_fh_map_entry const *e, ccc_fh_map_elem *elem);

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
void *ccc_fhm_insert_entry(ccc_fh_map_entry const *e, ccc_fh_map_elem *elem);

/** @brief Unwraps the provided entry to obtain a view into the table element.
@param [in] e the entry from a query to the table via function or macro.
@return an immutable view into the table entry if one is present, or NULL. */
void *ccc_fhm_unwrap(ccc_fh_map_entry const *e);

/** @brief Returns the Vacant or Occupied status of the entry.
@param [in] e the entry from a query to the table via function or macro.
@return true if the entry is occupied, false if not. */
bool ccc_fhm_occupied(ccc_fh_map_entry const *e);

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
will occur from an entry with this function. For example, leaving this function
in an assert for debug builds can be a helpful sanity check if the heap should
correctly resize by default and errors are not usually expected. */
bool ccc_fhm_insert_error(ccc_fh_map_entry const *e);

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
bool ccc_fhm_is_empty(ccc_flat_hash_map const *h);

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
ccc_result ccc_fhm_clear(ccc_flat_hash_map *h, ccc_destructor_fn *fn);

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

/** @brief Validation of invariants for the hash table.
@param [in] h the table to validate.
@return true if all invariants hold, false if corruption occurs. */
bool ccc_fhm_validate(ccc_flat_hash_map const *h);

#ifdef FLAT_HASH_MAP_USING_NAMESPACE_CCC
typedef ccc_fh_map_elem fh_map_elem;
typedef ccc_flat_hash_map flat_hash_map;
typedef ccc_fh_map_entry fh_map_entry;
#    define fhm_init(args...) ccc_fhm_init(args)
#    define fhm_and_modify_w(args...) ccc_fhm_and_modify_w(args)
#    define fhm_or_insert_w(args...) ccc_fhm_or_insert_w(args)
#    define fhm_insert_entry_w(args...) ccc_fhm_insert_entry_w(args)
#    define fhm_insert_or_assign_w(args...) ccc_fhm_insert_or_assign_w(args)
#    define fhm_try_insert_w(args...) ccc_fhm_try_insert_w(args)
#    define fhm_contains(args...) ccc_fhm_contains(args)
#    define fhm_get_key_val(args...) ccc_fhm_get_key_val(args)
#    define fhm_get_mut(args...) ccc_fhm_get_mut(args)
#    define fhm_insert_r(args...) ccc_fhm_insert_r(args)
#    define fhm_remove_r(args...) ccc_fhm_remove_r(args)
#    define fhm_remove_entry_r(args...) ccc_fhm_remove_entry_r(args)
#    define fhm_remove(args...) ccc_fhm_remove(args)
#    define fhm_insert(args...) ccc_fhm_insert(args)
#    define fhm_remove_entry(args...) ccc_fhm_remove_entry(args)
#    define fhm_entry_r(args...) ccc_fhm_entry_r(args)
#    define fhm_entry(args...) ccc_fhm_entry(args)
#    define fhm_and_modify_r(args...) ccc_fhm_and_modify_r(args)
#    define fhm_and_modify(args...) ccc_fhm_and_modify(args)
#    define fhm_and_modify_aux_r(args...) ccc_fhm_and_modify_aux_r(args)
#    define fhm_and_modify_aux(args...) ccc_fhm_and_modify_aux(args)
#    define fhm_entry(args...) ccc_fhm_entry(args)
#    define fhm_and_modify(args...) ccc_fhm_and_modify(args)
#    define fhm_and_modify_aux(args...) ccc_fhm_and_modify_aux(args)
#    define fhm_or_insert(args...) ccc_fhm_or_insert(args)
#    define fhm_insert_entry(args...) ccc_fhm_insert_entry(args)
#    define fhm_unwrap(args...) ccc_fhm_unwrap(args)
#    define fhm_unwrap_mut(args...) ccc_fhm_unwrap_mut(args)
#    define fhm_occupied(args...) ccc_fhm_occupied(args)
#    define fhm_insert_error(args...) ccc_fhm_insert_error(args)
#    define fhm_begin(args...) ccc_fhm_begin(args)
#    define fhm_next(args...) ccc_fhm_next(args)
#    define fhm_end(args...) ccc_fhm_end(args)
#    define fhm_is_empty(args...) ccc_fhm_is_empty(args)
#    define fhm_size(args...) ccc_fhm_size(args)
#    define fhm_clear(args...) ccc_fhm_clear(args)
#    define fhm_clear_and_free(args...) ccc_fhm_clear_and_free(args)
#    define fhm_next_prime(args...) ccc_fhm_next_prime(args)
#    define fhm_capacity(args...) ccc_fhm_capacity(args)
#    define fhm_validate(args...) ccc_fhm_validate(args)
#endif

#endif /* CCC_FLAT_HASH_MAP_H */
