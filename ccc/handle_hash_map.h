/** @file
@brief The Handle Hash Map Interface

A Handle Hash Map stores elements by hash value and allows the user to retrieve
them by key in amortized O(1) while offering handle stability. A handle is an
index into a slot of the table where the user data is originally placed upon
insertion. It is guaranteed to remain in the same slot until deletion, even if
the table is resized by subsequent insertions or deletions of other elements
occur. This comes at a slight space and implementation complexity cost when
compared to the standard flat hash map offered in the collection, especially
during resizing operations. However, it is more beneficial for large structs and
fixed table sizes to use this version. The benefits are that when the handles
exposed in the interface are saved by the user they offer the similar guarantees
as pointer stability except with the benefits of tightly grouped data in one
array accessed via index.

For containers in this collection the user may have a variety of memory sources
backing the containers. This container aims to be an equivalent stand in for
std::unordered_map, absl::node_hash_map, or manually managing pointers in a flat
hash map under the constraints of the C Container Collection. Instead of forcing
the user to manage separate allocations for nodes that need to remain in the
same location, this container will ensure any inserted element remains at the
same index in the table allowing complex container compositions and any
underlying source of memory specified at compile time or runtime. This container
therefore exposes an interface that mainly returns stable handle indices and
these should be what the user stores and accesses when needed. Only expose the
underlying pointer to data with the provided access function when needed and
store the handle for all other purposes.

A handle hash map requires the user to provide a struct with known key and
handle hash element fields as well as a hash function and key comparator
function. The hash function should be well tailored to the key being stored in
the table to prevent collisions. Currently, the handle hash map does not offer
any default hash functions or hash strengthening algorithms so strong hash
functions should be obtained by the user for the data set.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define HANDLE_HASH_MAP_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_HANDLE_HASH_MAP_H
#define CCC_HANDLE_HASH_MAP_H

/** @cond */
#include <stdbool.h>
#include <stddef.h>
/** @endcond */

#include "impl/impl_handle_hash_map.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A container for storing key-value structures defined by the user in
a contiguous buffer.

A handle hash map can be initialized on the stack, heap, or data segment at
runtime or compile time. */
typedef struct ccc_hhmap_ ccc_handle_hash_map;

/** @brief An intrusive element for a user provided type.

Because the hash map is flat, data is always copied from the provided type into
the table. */
typedef struct ccc_hhmap_elem_ ccc_hhmap_elem;

/** @brief A container specific handle used to implement the Handle Interface.

The Handle Interface offers efficient search and subsequent insertion, deletion,
or value update based on the needs of the user. */
typedef union ccc_hhmap_handle_ ccc_hhmap_handle;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a map with a buffer of types at compile time or runtime.
@param [in] memory_ptr the pointer to the backing buffer array of user types.
May be NULL if the user provides a allocation function. The buffer will be
interpreted in units of type size that the user intends to store.
buffer is provided and an allocation function is given.
@param [in] hhash_elem_field the name of the hhmap_elem field.
@param [in] key_field the field of the struct used for key storage.
resizing is allowed.
@param [in] hash_fn the ccc_hash_fn function the user desires for the table.
@param [in] key_eq_fn the ccc_key_eq_fn the user intends to use.
@param [in] alloc_fn the allocation function for resizing or NULL if no
@param [in] aux_data auxiliary data that is needed for hashing or comparison.
@param [in] capacity the starting capacity of the provided buffer or 0 if no
@return the handle hash map directly initialized on the right hand side of the
equality operator (i.e. ccc_handle_hash_map fh = ccc_hhm_init(...);) */
#define ccc_hhm_init(memory_ptr, hhash_elem_field, key_field, hash_fn,         \
                     key_eq_fn, alloc_fn, aux_data, capacity)                  \
    ccc_impl_hhm_init(memory_ptr, hhash_elem_field, key_field, hash_fn,        \
                      key_eq_fn, alloc_fn, aux_data, capacity)

/** @brief Copy the map at source to destination.
@param [in] dst the initialized destination for the copy of the src map.
@param [in] src the initialized source of the map.
@param [in] fn the optional allocation function if resizing is needed.
@return the result of the copy operation. If the destination capacity is less
than the source capacity and no allocation function is provided an input error
is returned. If resizing is required and resizing of dst fails a memory error
is returned.
@note dst must have capacity greater than or equal to src. If dst capacity is
less than src, an allocation function must be provided with the fn argument.
@warning the stable handles to user data in src will not remain the same as
those in dst if dst has a capacity greater than src. However, after the initial
copy to dst the handles in dst are now stable at their current positions.

Note that there are two ways to copy data from source to destination: provide
sufficient memory and pass NULL as fn, or allow the copy function to take care
of allocation for the copy.

Manual memory management with no allocation function provided.

```
#define handle_hash_MAP_USING_NAMESPACE_CCC
struct val
{
    hhmap_elem e;
    int key;
    int val;
};
static handle_hash_map src
    = hhm_init((static struct val[11]){}, e, key, hhmap_int_to_u64,
               hhmap_id_eq, NULL, NULL, 11);
insert_rand_vals(&src);
static handle_hash_map dst
    = hhm_init((static struct val[13]){}, e, key, hhmap_int_to_u64,
               hhmap_id_eq, NULL, NULL, 13);
ccc_result res = hhm_copy(&dst, &src, NULL);
```

The above requires dst capacity be greater than or equal to src capacity. Here
is memory management handed over to the copy function.

```
#define handle_hash_MAP_USING_NAMESPACE_CCC
struct val
{
    hhmap_elem e;
    int key;
    int val;
};
static handle_hash_map src
    = hhm_init((struct val*)NULL, e, key, hhmap_int_to_u64, hhmap_id_eq,
               NULL, 0);
insert_rand_vals(&src);
static handle_hash_map dst
    = hhm_init((struct val*)NULL, e, key, hhmap_int_to_u64, hhmap_id_eq,
               NULL, NULL, 0);
ccc_result res = hhm_copy(&dst, &src, std_alloc);
```

The above allows dst to have a capacity less than that of the src as long as
copy has been provided an allocation function to resize dst. Note that this
would still work if copying to a destination that the user wants as a fixed
size map.

```
#define handle_hash_MAP_USING_NAMESPACE_CCC
struct val
{
    hhmap_elem e;
    int key;
    int val;
};
static handle_hash_map src
    = hhm_init((struct val*)NULL, e, key, hhmap_int_to_u64, hhmap_id_eq,
               NULL, NULL, 0);
insert_rand_vals(&src);
static handle_hash_map dst
    = hhm_init((struct val*)NULL, e, key, hhmap_int_to_u64, hhmap_id_eq,
               NULL, NULL, 0);
ccc_result res = hhm_copy(&dst, &src, std_alloc);
```

The above sets up dst with fixed size while src is a dynamic map. Because an
allocation function is provided, the dst is resized once for the copy and
retains its fixed size after the copy is complete. This would require the user
to manually free the underlying buffer at dst eventually if this method is used.
Usually it is better to allocate the memory explicitly before the copy if
copying between maps without allocation permission.

These options allow users to stay consistent across containers with their
memory management strategies. */
ccc_result ccc_hhm_copy(ccc_handle_hash_map *dst,
                        ccc_handle_hash_map const *src, ccc_alloc_fn *fn);

/**@}*/

/**@name Membership Interface
Test membership or obtain references to stored user types directly. */
/**@{*/

/** @brief Returns a reference to the user data at the provided handle.
@param [in] h a pointer to the map.
@param [in] i the stable handle obtained by the user.
@return a pointer to the user type stored at the specified handle or NULL if
an out of range handle or handle representing no data is provided.
@warning this function can only check if the handle value is in range. If a
handle represents a slot that has been taken by a new element because the old
one has been removed that new element data will be returned.
@warning do not try to access data in the table manually with a handle. Always
use this provided interface function when a reference to data is needed. */
[[nodiscard]] void *ccc_hhm_at(ccc_handle_hash_map const *h, ccc_handle_i i);

/** @brief Returns a reference to the user type in the table at the handle.
@param [in] handle_hash_map_ptr a pointer to the map.
@param [in] type_name name of the user type stored in each slot of the map.
@param [in] handle_i the index handle obtained from previous map operations.
@return a reference to the handle at handle in the map as the type the user has
stored in the map. */
#define ccc_hhm_as(handle_hash_map_ptr, type_name, handle_i...)                \
    ccc_impl_hhm_as(handle_hash_map_ptr, type_name, handle_i)

/** @brief Searches the table for the presence of key.
@param [in] h the handle hash table to be searched.
@param [in] key pointer to the key matching the key type of the user struct.
@return true if the struct containing key is stored, false if not. */
[[nodiscard]] bool ccc_hhm_contains(ccc_handle_hash_map *h, void const *key);

/** @brief Returns a handle to the element stored at key if present.
@param [in] h the handle hash map to search.
@param [in] key the key to search matching stored key type.
@return a non-zero handle if present, otherwise 0 (falsey). */
[[nodiscard]] ccc_handle_i ccc_hhm_get_key_val(ccc_handle_hash_map *h,
                                               void const *key);

/**@}*/

/** @name Handle Interface
Obtain and operate on container entries for efficient queries when non-trivial
control flow is needed. A handle is a stable index to data in the table. For
the handle hash map a valid handle will always be non-zero. This allows for
the user to rely on truthy/falsey logic if needed: this is similar to valid
pointers vs the NULL pointer. */
/**@{*/

/** @brief Invariantly inserts the key value wrapping out_handle.
@param [in] h the pointer to the handle hash map.
@param [out] out_handle the handle to the user type wrapping hhash elem.
@return a handle. If Vacant, no prior element with key existed and the type
wrapping out_handle remains unchanged. If Occupied the old value is written
to the type wrapping out_handle. If more space is needed but allocation fails or
has been forbidden, an insert error is set. Unwrap to view the current table
element.

@note this function may write to the struct containing the second parameter. */
[[nodiscard]] ccc_handle ccc_hhm_insert(ccc_handle_hash_map *h,
                                        ccc_hhmap_elem *out_handle);

/** @brief Invariantly inserts the key value wrapping out_handle_ptr.
@param [in] handle_hash_map_ptr the pointer to the handle hash map.
@param [out] out_handle_ptr the handle to the user type wrapping hhash elem.
@return a compound literal handle to the handle. If Vacant, no prior element
with key existed and the type wrapping out_handle_ptr remains unchanged. If
Occupied the old value is written to the type wrapping out_handle_ptr. If more
space is needed but allocation fails or has been forbidden, an insert error is
set.

@note this function may write to the struct containing the second parameter. */
#define ccc_hhm_insert_r(handle_hash_map_ptr, out_handle_ptr)                  \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_hhm_insert((handle_hash_map_ptr), (out_handle_ptr)).impl_          \
    }

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing out_handle provided by the user.
@param [in] h the pointer to the handle hash map.
@param [out] out_handle the handle to the user type wrapping hhash elem.
@return a handle with a status indicating if the element searched existed and
has been removed from the table. Unwrapping will result in NULL. If an old
element existed it is copied to the struct wrapping out_handle.

Note that this function may write to the struct containing the second parameter
and wraps it in a handle to provide information about the old value. */
[[nodiscard]] ccc_handle ccc_hhm_remove(ccc_handle_hash_map *h,
                                        ccc_hhmap_elem *out_handle);

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing out_handle_ptr provided by the user.
@param [in] handle_hash_map_ptr the pointer to the handle hash map.
@param [out] out_handle_ptr the handle to the user type wrapping hhash elem.
@return a compound literal handle handle with a status indicating if the
element searched existed and has been removed from the table. Unwrapping will
result in NULL. If an old element existed it is copied to the struct wrapping
out_handle_ptr.

Note that this function may write to the struct containing the second parameter
and wraps it in a handle to provide information about the old value. */
#define ccc_hhm_remove_r(handle_hash_map_ptr, out_handle_ptr)                  \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_hhm_remove((handle_hash_map_ptr), (out_handle_ptr)).impl_          \
    }

/** @brief Attempts to insert the key value wrapping key_val_handle
@param [in] h the pointer to the handle hash map.
@param [in] key_val_handle the handle to the user type wrapping hhash elem.
@return a handle. If Occupied, the handle contains a handle to the key value
user type in the table and may be unwrapped. If Vacant the handle contains a
handle to the newly inserted element in the table. If more space is needed but
allocation fails or has been forbidden, an insert error is set. */
[[nodiscard]] ccc_handle ccc_hhm_try_insert(ccc_handle_hash_map *h,
                                            ccc_hhmap_elem *key_val_handle);

/** @brief Attempts to insert the key value wrapping key_val_handle_ptr.
@param [in] handle_hash_map_ptr the pointer to the handle hash map.
@param [in] key_val_handle_ptr the handle to the user type wrapping hhash elem.
@return a compound literal handle to the handle. If Occupied, the handle
contains a handle to the key value user type in the table and may be
unwrapped. If Vacant the handle contains a handle to the newly inserted
element in the table. If more space is needed but allocation fails or has been
forbidden, an insert error is set. */
#define ccc_hhm_try_insert_r(handle_hash_map_ptr, key_val_handle_ptr)          \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_hhm_try_insert((handle_hash_map_ptr), (key_val_handle_ptr)).impl_  \
    }

/** @brief lazily insert lazy_value into the map at key if key is absent.
@param [in] handle_hash_map_ptr a pointer to the handle hash map.
@param [in] key the direct key r-value.
@param [in] lazy_value the compound literal specifying the value.
@return a compound literal handle to the handle of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.
@warning ensure the key type matches the type stored in table as the key. For
example, if the key is of type `int` and a `size_t` is passed as the variable to
the key argument, adjacent bytes of the struct will be overwritten.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define ccc_hhm_try_insert_w(handle_hash_map_ptr, key, lazy_value...)          \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_impl_hhm_try_insert_w(handle_hash_map_ptr, key, lazy_value)        \
    }

/** @brief Invariantly inserts or overwrites a user struct into the table.
@param [in] h a pointer to the handle hash map.
@param [in] key_val_handle the handle to the wrapping user struct key value.
@return a handle to the current table element. If Occupied a handle was
overwritten by the new key value. If Vacant no prior table handle existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
[[nodiscard]] ccc_handle
ccc_hhm_insert_or_assign(ccc_handle_hash_map *h,
                         ccc_hhmap_elem *key_val_handle);

/** @brief Invariantly inserts or overwrites a user struct into the table.
@param [in] handle_hash_map_ptr a pointer to the handle hash map.
@param [in] key_val_handle_ptr the handle to the wrapping user struct key value.
@return a compound literal handle to the current table element. If Occupied a
handle was overwritten by the new key value. If Vacant no prior table handle
existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
#define ccc_hhm_insert_or_assign_r(handle_hash_map_ptr, key_val_handle_ptr)    \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_hhm_insert_or_assign((handle_hash_map_ptr), (key_val_handle))      \
            .impl_                                                             \
    }

/** @brief Inserts a new key value pair or overwrites the existing handle.
@param [in] handle_hash_map_ptr the pointer to the handle hash map.
@param [in] key the key to be searched in the table.
@param [in] lazy_value the compound literal to insert or use for overwrite.
@return a compound literal handle to the handle of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define ccc_hhm_insert_or_assign_w(handle_hash_map_ptr, key, lazy_value...)    \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_impl_hhm_insert_or_assign_w(handle_hash_map_ptr, key, lazy_value)  \
    }

/** @brief Obtains a handle for the provided key in the table for future use.
@param [in] h the hash table to be searched.
@param [in] key the key used to search the table matching the stored key type.
@return a specialized hash handle for use with other functions in the Handle
Interface.
@warning the contents of a handle should not be examined or modified. Use the
provided functions, only.

A handle is a search result that provides either an Occupied or Vacant element
in the table. An occupied handle signifies that the search was successful. A
Vacant handle means the search was not successful but we now have a handle to
where in the table such an element should be inserted.

A handle is rarely useful on its own. It should be passed in a functional style
to subsequent calls in the Handle Interface.*/
[[nodiscard]] ccc_hhmap_handle ccc_hhm_handle(ccc_handle_hash_map *h,
                                              void const *key);

/** @brief Obtains a handle for the provided key in the table for future use.
@param [in] handle_hash_map_ptr the hash table to be searched.
@param [in] key_ptr the key used to search the table matching the stored key
type.
@return a compound literal handle to a specialized hash handle for use with
other functions in the Handle Interface.
@warning the contents of a handle should not be examined or modified. Use the
provided functions, only.

A handle is a search result that provides either an Occupied or Vacant handle
in the table. An occupied handle signifies that the search was successful. A
Vacant handle means the search was not successful but we now have a handle to
where in the table such an element should be inserted.

A handle is most often passed in a functional style to subsequent calls in the
Handle Interface.*/
#define ccc_hhm_handle_r(handle_hash_map_ptr, key_ptr)                         \
    &(ccc_hhmap_handle)                                                        \
    {                                                                          \
        ccc_hhm_handle((handle_hash_map_ptr), (key_ptr)).impl_                 \
    }

/** @brief Modifies the provided handle if it is Occupied.
@param [in] e the handle obtained from a handle function or macro.
@param [in] fn an update function in which the auxiliary argument is unused.
@return the updated handle if it was Occupied or the unmodified vacant handle.

This function is intended to make the function chaining in the Handle Interface
more succinct if the handle will be modified in place based on its own value
without the need of the auxiliary argument a ccc_update_fn can provide. */
[[nodiscard]] ccc_hhmap_handle *ccc_hhm_and_modify(ccc_hhmap_handle *e,
                                                   ccc_update_fn *fn);

/** @brief Modifies the provided handle if it is Occupied.
@param [in] e the handle obtained from a handle function or macro.
@param [in] fn an update function that requires auxiliary data.
@param [in] aux auxiliary data required for the update.
@return the updated handle if it was Occupied or the unmodified vacant handle.

This function makes full use of a ccc_update_fn capability, meaning a complete
ccc_update object will be passed to the update function callback. */
[[nodiscard]] ccc_hhmap_handle *
ccc_hhm_and_modify_aux(ccc_hhmap_handle *e, ccc_update_fn *fn, void *aux);

/** @brief Modify an Occupied handle with a closure over user type T.
@param [in] handle_hash_map_handle_ptr a pointer to the obtained handle.
@param [in] type_name the name of the user type stored in the container.
@param [in] closure_over_T the code to be run on the handle to user type T,
if Occupied. This may be a semicolon separated list of statements to execute on
T or a section of code wrapped in braces {code here} which may be preferred
for formatting.
@return a compound literal handle to the modified handle if it was occupied
or a vacant handle if it was vacant.
@note T is a handle to the user type stored in the handle guaranteed to be
non-NULL if the closure executes.

```
#define handle_hash_MAP_USING_NAMESPACE_CCC
// Increment the key k if found otherwise do nothing.
hhmap_handle *e = hhm_and_modify_w(handle_r(&hhm, &k), word, T->cnt++;);

// Increment the key k if found otherwise insert a default value.
handle_i w = hhm_or_insert_w(hhm_and_modify_w(handle_r(&hhm, &k), word,
                                              { T->cnt++; }),
                             (word){.key = k, .cnt = 1});
```

Note that any code written is only evaluated if the handle is Occupied and the
container can deliver the user type T. This means any function calls are lazily
evaluated in the closure scope. */
#define ccc_hhm_and_modify_w(handle_hash_map_handle_ptr, type_name,            \
                             closure_over_T...)                                \
    &(ccc_hhmap_handle)                                                        \
    {                                                                          \
        ccc_impl_hhm_and_modify_w(handle_hash_map_handle_ptr, type_name,       \
                                  closure_over_T)                              \
    }

/** @brief Inserts the struct with handle elem if the handle is Vacant.
@param [in] e the handle obtained via function or macro call.
@param [in] elem the handle to the struct to be inserted to a Vacant handle.
@return a non-zero handle index to handle in the table invariantly. 0 (falsey)
on error.

Because this functions takes a handle and inserts if it is Vacant, the only
reason 0 shall be returned is when an insertion error will occur, usually
due to a resizing memory error. This can happen if the table is not allowed
to resize because no allocation function is provided. */
[[nodiscard]] ccc_handle_i ccc_hhm_or_insert(ccc_hhmap_handle const *e,
                                             ccc_hhmap_elem *elem);

/** @brief lazily insert the desired key value into the handle if it is Vacant.
@param [in] handle_hash_map_handle_ptr a pointer to the obtained handle.
@param [in] lazy_key_value the compound literal to construct in place if the
handle is Vacant.
@return a non-zero handle index to the unwrapped user type in the handle, either
the unmodified handle if the handle was Occupied or the newly inserted element
if the handle was Vacant. 0 is returned if resizing is required but fails or
is not allowed.

Note that if the compound literal uses any function calls to generate values
or other data, such functions will not be called if the handle is Occupied. */
#define ccc_hhm_or_insert_w(handle_hash_map_handle_ptr, lazy_key_value...)     \
    ccc_impl_hhm_or_insert_w(handle_hash_map_handle_ptr, lazy_key_value)

/** @brief Inserts the provided handle invariantly.
@param [in] e the handle returned from a call obtaining a handle.
@param [in] elem a handle to the struct the user intends to insert.
@return a non-zero handle index to the inserted element or 0 upon a memory
error in which the load factor would be exceeded when no allocation policy is
defined or resizing failed to find more memory.

This method can be used when the old value in the table does not need to
be preserved. See the regular insert method if the old value is of interest.
If an error occurs during the insertion process due to memory limitations
or a search error 0 is returned. Otherwise insertion should not fail. */
[[nodiscard]] ccc_handle_i ccc_hhm_insert_handle(ccc_hhmap_handle const *e,
                                                 ccc_hhmap_elem *elem);

/** @brief write the contents of the compound literal lazy_key_value to a slot.
@param [in] handle_hash_map_handle_ptr a pointer to the obtained handle.
@param [in] lazy_key_value the compound literal to write to a new slot.
@return a non-zero handle to the newly inserted or overwritten user type. 0 is
returned if resizing is required but fails or is not allowed. */
#define ccc_hhm_insert_handle_w(handle_hash_map_handle_ptr, lazy_key_value...) \
    ccc_impl_hhm_insert_handle_w(handle_hash_map_handle_ptr, lazy_key_value)

/** @brief Remove the handle from the table if Occupied.
@param [in] e a pointer to the table handle.
@return a handle containing 0. If Occupied a handle in the table existed
and was removed. If Vacant, no prior handle existed to be removed.

If the old table element is needed see the remove method. */
[[nodiscard]] ccc_handle ccc_hhm_remove_handle(ccc_hhmap_handle const *e);

/** @brief Remove the handle from the table if Occupied.
@param [in] handle_hash_map_handle_ptr a pointer to the table handle.
@return a handle containing 0. If Occupied a handle in the table existed
and was removed. If Vacant, no prior handle existed to be removed.

If the old table element is needed see the remove method. */
#define ccc_hhm_remove_handle_r(handle_hash_map_handle_ptr)                    \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_hhm_remove_handle((handle_hash_map_handle_ptr)).impl_              \
    }

/** @brief Unwraps the provided handle to obtain a handle index.
@param [in] e the handle from a query to the table via function or macro.
@return a non-zero handle index if the table element is Occupied, otherwise 0
(falsey). */
[[nodiscard]] ccc_handle_i ccc_hhm_unwrap(ccc_hhmap_handle const *e);

/** @brief Returns the Vacant or Occupied status of the handle.
@param [in] e the handle from a query to the table via function or macro.
@return true if the handle is occupied, false if not. */
[[nodiscard]] bool ccc_hhm_occupied(ccc_hhmap_handle const *e);

/** @brief Provides the status of the handle should an insertion follow.
@param [in] e the handle from a query to the table via function or macro.
@return true if the next insertion of a new element will cause an error.

Table resizing occurs upon calls to handle functions/macros or when trying
to insert a new element directly. This is to provide stable entries from the
time they are obtained to the time they are used in functions they are passed
to (i.e. the idiomatic or_insert(handle(...)...)).

However, if a Vacant handle is returned and then a subsequent insertion function
is used, it will not work if resizing has failed and the return of those
functions will indicate such a failure. One can also confirm an insertion error
will occur from a handle with this function. For example, leaving this function
in an assert for debug builds can be a helpful sanity check if the heap should
correctly resize by default and errors are not usually expected. */
[[nodiscard]] bool ccc_hhm_insert_error(ccc_hhmap_handle const *e);

/** @brief Obtain the handle status from a container handle.
@param [in] e a pointer to the handle.
@return the status stored in the handle after the required action on the
container completes. If e is NULL a handle input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned.

Note that this function can be useful for debugging or if more detailed
messages are needed for logging purposes. See ccc_handle_status_msg() in
ccc/types.h for more information on detailed handle statuses. */
[[nodiscard]] ccc_handle_status
ccc_hhm_handle_status(ccc_hhmap_handle const *e);

/**@}*/

/** @name Deallocation Interface
Destroy the container. */
/**@{*/

/** @brief Frees all slots in the table for use without affecting capacity.
@param [in] h the table to be cleared.
@param [in] fn the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the table before their slots are
forfeit.

If NULL is passed as the destructor function time is O(1), else O(capacity). */
ccc_result ccc_hhm_clear(ccc_handle_hash_map *h, ccc_destructor_fn *fn);

/** @brief Frees all slots in the table and frees the underlying buffer.
@param [in] h the table to be cleared.
@param [in] fn the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the table before their slots are
forfeit.
@return the result of free operation. If no alloc function is provided it is
an error to attempt to free the buffer and a memory error is returned.
Otherwise, an OK result is returned. */
ccc_result ccc_hhm_clear_and_free(ccc_handle_hash_map *h,
                                  ccc_destructor_fn *fn);

/**@}*/

/** @name Iterator Interface
Obtain and manage iterators over the container. */
/**@{*/

/** @brief Obtains a handle to the first element in the table.
@param [in] h the table to iterate through.
@return a container specific handle that interface functions will accept.
@warning erasing or inserting during iteration may result in repeating or
unexpected iteration orders.

Iteration starts from index 0 by capacity of the table so iteration order is
not obvious to the user, nor should any specific order be relied on. */
[[nodiscard]] ccc_hhmap_handle ccc_hhm_begin(ccc_handle_hash_map const *h);

/** @brief Advances the iterator to the next occupied table handle.
@param [out] iter the previous handle that acts as an iterator.
@return ok if the handle is successfully updated to represent the next element
or an error if iter is NULL.
@warning erasing or inserting during iteration may result in repeating or
unexpected iteration orders, but the index remains valid for the table. */
ccc_result ccc_hhm_next(ccc_hhmap_handle *iter);

/** @brief Check if the current handle iterator has reached the end.
@param [in] iter a pointer to the current handle iterator.
@return true if the handle iterator has reached the end of the table and
iteration should stop, false if the iterator is valid and iteration should
continue.
@warning if iter has reached the end unwrapping it will result in 0 or invalid
handles and NULL references. */
[[nodiscard]] bool ccc_hhm_end(ccc_hhmap_handle const *iter);

/**@}*/

/** @name State Interface
Obtain the container state. */
/**@{*/

/** @brief Returns the size status of the table.
@param [in] h the hash table.
@return true if empty else false. */
[[nodiscard]] bool ccc_hhm_is_empty(ccc_handle_hash_map const *h);

/** @brief Returns the size of the table.
@param [in] h the hash table.
@return the size_t size. */
[[nodiscard]] size_t ccc_hhm_size(ccc_handle_hash_map const *h);

/** @brief Helper to find a prime number if needed.
@param [in] n the input that may or may not be prime.
@return the next larger prime number.

It is possible to use this hash table without an allocator by providing the
buffer to be used for the underlying storage and preventing allocation.
If such a backing store is used it would be best to ensure it is a prime number
size to mitigate hash collisions. */
[[nodiscard]] size_t ccc_hhm_next_prime(size_t n);

/** @brief Return the full capacity of the backing storage.
@param [in] h the hash table.
@return the capacity. */
[[nodiscard]] size_t ccc_hhm_capacity(ccc_handle_hash_map const *h);

/** @brief Return a reference to the base of backing array. O(1).
@param [in] h a pointer to the map.
@return a reference to the base of the backing array.
@note the reference is to the base of the backing array at index 0 with no
consideration for the organization of map.
@warning it is the users responsibility to ensure that access to any data is
within the capacity of the backing buffer. */
[[nodiscard]] void *ccc_hhm_data(ccc_handle_hash_map const *h);

/** @brief Validation of invariants for the hash table.
@param [in] h the table to validate.
@return true if all invariants hold, false if corruption occurs. */
[[nodiscard]] bool ccc_hhm_validate(ccc_handle_hash_map const *h);

/**@}*/

/** Define this preprocessor directive if shorter names are helpful. Ensure
 no namespace clashes occur before shortening. */
#ifdef HANDLE_HASH_MAP_USING_NAMESPACE_CCC
typedef ccc_handle_hash_map handle_hash_map;
typedef ccc_hhmap_elem hhmap_elem;
typedef ccc_hhmap_handle hhmap_handle;
#    define hhm_init(args...) ccc_hhm_init(args)
#    define hhm_copy(args...) ccc_hhm_copy(args)
#    define hhm_contains(args...) ccc_hhm_contains(args)
#    define hhm_get_key_val(args...) ccc_hhm_get_key_val(args)
#    define hhm_at(args...) ccc_hhm_at(args)
#    define hhm_as(args...) ccc_hhm_as(args)
#    define hhm_insert(args...) ccc_hhm_insert(args)
#    define hhm_insert_r(args...) ccc_hhm_insert_r(args)
#    define hhm_remove(args...) ccc_hhm_remove(args)
#    define hhm_remove_r(args...) ccc_hhm_remove_r(args)
#    define hhm_try_insert(args...) ccc_hhm_try_insert(args)
#    define hhm_try_insert_r(args...) ccc_hhm_try_insert_r(args)
#    define hhm_try_insert_w(args...) ccc_hhm_try_insert_w(args)
#    define hhm_insert_or_assign(args...) ccc_hhm_insert_or_assign(args)
#    define hhm_insert_or_assign_r(args...) ccc_hhm_insert_or_assign_r(args)
#    define hhm_insert_or_assign_w(args...) ccc_hhm_insert_or_assign_w(args)
#    define hhm_handle(args...) ccc_hhm_handle(args)
#    define hhm_handle_r(args...) ccc_hhm_handle_r(args)
#    define hhm_and_modify(args...) ccc_hhm_and_modify(args)
#    define hhm_and_modify_aux(args...) ccc_hhm_and_modify_aux(args)
#    define hhm_and_modify_w(args...) ccc_hhm_and_modify_w(args)
#    define hhm_or_insert(args...) ccc_hhm_or_insert(args)
#    define hhm_or_insert_w(args...) ccc_hhm_or_insert_w(args)
#    define hhm_insert_handle(args...) ccc_hhm_insert_handle(args)
#    define hhm_insert_handle_w(args...) ccc_hhm_insert_handle_w(args)
#    define hhm_remove_handle(args...) ccc_hhm_remove_handle(args)
#    define hhm_remove_handle_r(args...) ccc_hhm_remove_handle_r(args)
#    define hhm_unwrap(args...) ccc_hhm_unwrap(args)
#    define hhm_occupied(args...) ccc_hhm_occupied(args)
#    define hhm_insert_error(args...) ccc_hhm_insert_error(args)
#    define hhm_handle_status(args...) ccc_hhm_handle_status(args)
#    define hhm_clear(args...) ccc_hhm_clear(args)
#    define hhm_clear_and_free(args...) ccc_hhm_clear_and_free(args)
#    define hhm_begin(args...) ccc_hhm_begin(args)
#    define hhm_next(args...) ccc_hhm_next(args)
#    define hhm_end(args...) ccc_hhm_end(args)
#    define hhm_is_empty(args...) ccc_hhm_is_empty(args)
#    define hhm_size(args...) ccc_hhm_size(args)
#    define hhm_next_prime(args...) ccc_hhm_next_prime(args)
#    define hhm_capacity(args...) ccc_hhm_capacity(args)
#    define hhm_data(args...) ccc_hhm_data(args)
#    define hhm_validate(args...) ccc_hhm_validate(args)
#endif /* HANDLE_HASH_MAP_USING_NAMESPACE_CCC */

#endif /* CCC_HANDLE_HASH_MAP_H */
