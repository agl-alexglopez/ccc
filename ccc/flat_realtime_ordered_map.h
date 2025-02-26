/** @file
@brief The Flat Realtime Ordered Map Interface

A flat realtime ordered map offers storage and retrieval by key. This map is
suitable for realtime applications if resizing can be well controlled. Insert
operations may cause resizing if allocation is allowed.

The flat variant of the ordered map promises contiguous storage and random
access if needed. Also, all elements in the map track their relationships via
indices in the buffer. Therefore, this data structure can be relocated, copied,
serialized, or written to disk and all internal data structure references will
remain valid. Insertion may invoke an O(N) operation if resizing occurs.
Finally, if allocation is prohibited upon initialization and the user intends
to store a fixed size N nodes in the map N + 1 capacity is needed for the
sentinel node in the buffer.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_FLAT_REALTIME_ORDERED_MAP_H
#define CCC_FLAT_REALTIME_ORDERED_MAP_H

/** @cond */
#include <stdbool.h>
#include <stddef.h>
/** @endcond */

#include "impl/impl_flat_realtime_ordered_map.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A flat realtime ordered map offers O(lg N) search and erase, and
amortized O(lg N) insert.
@warning it is undefined behavior to access an uninitialized container.

A flat realtime ordered map can be initialized on the stack, heap, or data
segment at runtime or compile time.*/
typedef struct ccc_fromap_ ccc_flat_realtime_ordered_map;

/** @brief The intrusive element for the user defined struct being stored in the
map

Because the map is flat data is always copied from the user type to map. */
typedef struct ccc_fromap_elem_ ccc_fromap_elem;

/** @brief A container specific entry used to implement the Entry Interface.
@warning it is undefined behavior to access an uninitialized container.

The Entry Interface offers efficient search and subsequent insertion, deletion,
or value update based on the needs of the user. */
typedef union ccc_fromap_entry_ ccc_fromap_entry;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initializes the map at runtime or compile time.
@param [in] memory_ptr a pointer to the contiguous user types or ((T *)NULL).
@param [in] frm_elem_field the name of the intrusive map elem field.
@param [in] key_elem_field the name of the field in user type used as key.
@param [in] key_cmp_fn the key comparison function (see types.h).
@param [in] alloc_fn the allocation function or NULL if allocation is banned.
@param [in] aux_data a pointer to any auxiliary data for comparison or
destruction.
@param [in] capacity the capacity at mem_ptr or 0 if ((T *)NULL).
@return the struct initialized ordered map for direct assignment
(i.e. ccc_flat_realtime_ordered_map m = ccc_frm_init(...);). */
#define ccc_frm_init(memory_ptr, frm_elem_field, key_elem_field, key_cmp_fn,   \
                     alloc_fn, aux_data, capacity)                             \
    ccc_impl_frm_init(memory_ptr, frm_elem_field, key_elem_field, key_cmp_fn,  \
                      alloc_fn, aux_data, capacity)

/** @brief Copy the map at source to destination.
@param [in] dst the initialized destination for the copy of the src map.
@param [in] src the initialized source of the map.
@param [in] fn the allocation function to resize dst or NULL.
@return the result of the copy operation. If the destination capacity is less
than the source capacity and no allocation function is provided an input error
is returned. If resizing is required and resizing of dst fails a memory error
is returned.
@note dst must have capacity greater than or equal to src. If dst capacity is
less than src, an allocation function must be provided with the fn argument.

Note that there are two ways to copy data from source to destination: provide
sufficient memory and pass NULL as fn, or allow the copy function to take care
of allocation for the copy.

Manual memory management with no allocation function provided.

```
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
struct val
{
    fromap_elem e;
    int key;
    int val;
};
static flat_ordered_map src
    = frm_init((static struct val[11]){}, e, key, key_cmp, NULL, NULL, 11);
insert_rand_vals(&src);
static flat_ordered_map dst
    = frm_init((static struct val[13]){}, e, key, key_cmp, NULL, NULL, 13);
ccc_result res = frm_copy(&dst, &src, NULL);
```

The above requires dst capacity be greater than or equal to src capacity. Here
is memory management handed over to the copy function.

```
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
struct val
{
    fromap_elem e;
    int key;
    int val;
};
static flat_ordered_map src
    = frm_init((struct val *)NULL, e, key, key_cmp, std_alloc, NULL, 0);
insert_rand_vals(&src);
static flat_ordered_map dst
    = frm_init((struct val *)NULL, e, key, key_cmp, std_alloc, NULL, 0);
ccc_result res = frm_copy(&dst, &src, std_alloc);
```

The above allows dst to have a capacity less than that of the src as long as
copy has been provided an allocation function to resize dst. Note that this
would still work if copying to a destination that the user wants as a fixed
size map.

```
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
struct val
{
    fromap_elem e;
    int key;
    int val;
};
static flat_ordered_map src
    = frm_init((struct val *)NULL, e, key, key_cmp, std_alloc, NULL, 0);
insert_rand_vals(&src);
static flat_ordered_map dst
    = frm_init((struct val *)NULL, e, key, key_cmp, NULL, NULL, 0);
ccc_result res = frm_copy(&dst, &src, std_alloc);
```

The above sets up dst with fixed size while src is a dynamic map. Because an
allocation function is provided, the dst is resized once for the copy and
retains its fixed size after the copy is complete. This would require the user
to manually free the underlying buffer at dst eventually if this method is used.
Usually it is better to allocate the memory explicitly before the copy if
copying between maps without allocation permission.

These options allow users to stay consistent across containers with their
memory management strategies. */
ccc_result ccc_frm_copy(ccc_flat_realtime_ordered_map *dst,
                        ccc_flat_realtime_ordered_map const *src,
                        ccc_alloc_fn *fn);

/**@}*/

/**@name Membership Interface
Test membership or obtain references to stored user types directly. */
/**@{*/

/** @brief Searches the map for the presence of key.
@param [in] frm the map to be searched.
@param [in] key pointer to the key matching the key type of the user struct.
@return true if the struct containing key is stored, false if not. */
[[nodiscard]] bool ccc_frm_contains(ccc_flat_realtime_ordered_map const *frm,
                                    void const *key);

/** @brief Returns a reference into the map at entry key.
@param [in] frm the ordered map to search.
@param [in] key the key to search matching stored key type.
@return a view of the map entry if it is present, else NULL. */
[[nodiscard]] void *
ccc_frm_get_key_val(ccc_flat_realtime_ordered_map const *frm, void const *key);

/**@}*/

/** @name Entry Interface
Obtain and operate on container entries for efficient queries when non-trivial
control flow is needed. */
/**@{*/

/** @brief Invariantly inserts the key value wrapping key_val_handle.
@param [in] frm the pointer to the ordered map.
@param [out] out_handle the handle to the user type wrapping map elem.
@return an entry. If Vacant, no prior element with key existed and the type
wrapping out_handle remains unchanged. If Occupied the old value is written
to the type wrapping out_handle and may be unwrapped to view. If more space
is needed but allocation fails or has been forbidden, an insert error is set.

Note that this function may write to the struct containing out_handle and wraps
it in an entry to provide information about the old value. */
[[nodiscard]] ccc_entry ccc_frm_insert(ccc_flat_realtime_ordered_map *frm,
                                       ccc_fromap_elem *out_handle);

/** @brief Invariantly inserts the key value wrapping key_val_handle.
@param [in] flat_realtime_ordered_map_ptr the pointer to the ordered map.
@param [out] out_handle_ptr the handle to the user type wrapping map elem.
@return a compound literal reference to an entry. If Vacant, no prior element
with key existed and the type wrapping out_handle remains unchanged. If Occupied
the old value is written to the type wrapping out_handle and may be unwrapped to
view. If more space is needed but allocation fails or has been forbidden, an
insert error is set.

Note that this function may write to the struct containing out_handle and wraps
it in an entry to provide information about the old value. */
#define ccc_frm_insert_r(flat_realtime_ordered_map_ptr, out_handle_ptr)        \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_frm_insert((flat_realtime_ordered_map_ptr), (out_handle_ptr))      \
            .impl_                                                             \
    }

/** @brief Attempts to insert the key value wrapping key_val_handle.
@param [in] frm the pointer to the map.
@param [in] key_val_handle the handle to the user type wrapping map elem.
@return an entry. If Occupied, the entry contains a reference to the key value
user type in the map and may be unwrapped. If Vacant the entry contains a
reference to the newly inserted entry in the map. If more space is needed but
allocation fails, an insert error is set. */
[[nodiscard]] ccc_entry ccc_frm_try_insert(ccc_flat_realtime_ordered_map *frm,
                                           ccc_fromap_elem *key_val_handle);

/** @brief Attempts to insert the key value wrapping key_val_handle.
@param [in] flat_realtime_ordered_map_ptr the pointer to the map.
@param [in] key_val_handle_ptr the handle to the user type wrapping map elem.
@return a compound literal reference to an entry. If Occupied, the entry
contains a reference to the key value user type in the map and may be unwrapped.
If Vacant the entry contains a reference to the newly inserted entry in the map.
If more space is needed but allocation fails an insert error is set. */
#define ccc_frm_try_insert_r(flat_realtime_ordered_map_ptr,                    \
                             key_val_handle_ptr)                               \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_frm_try_insert((flat_realtime_ordered_map_ptr),                    \
                           (key_val_handle_ptr))                               \
            .impl_                                                             \
    }

/** @brief lazily insert lazy_value into the map at key if key is absent.
@param [in] flat_realtime_ordered_map_ptr a pointer to the map.
@param [in] key the direct key r-value.
@param [in] lazy_value the compound literal specifying the value.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define ccc_frm_try_insert_w(flat_realtime_ordered_map_ptr, key,               \
                             lazy_value...)                                    \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_frm_try_insert_w(flat_realtime_ordered_map_ptr, key,          \
                                  lazy_value)                                  \
    }

/** @brief Invariantly inserts or overwrites a user struct into the map.
@param [in] frm a pointer to the flat hash map.
@param [in] key_val_handle the handle to the wrapping user struct key value.
@return an entry. If Occupied an entry was overwritten by the new key value. If
Vacant no prior map entry existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
[[nodiscard]] ccc_entry
ccc_frm_insert_or_assign(ccc_flat_realtime_ordered_map *frm,
                         ccc_fromap_elem *key_val_handle);

/** @brief Inserts a new key value pair or overwrites the existing entry.
@param [in] flat_realtime_ordered_map_ptr the pointer to the flat hash map.
@param [in] key the key to be searched in the map.
@param [in] lazy_value the compound literal to insert or use for overwrite.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define ccc_frm_insert_or_assign_w(flat_realtime_ordered_map_ptr, key,         \
                                   lazy_value...)                              \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_frm_insert_or_assign_w(flat_realtime_ordered_map_ptr, key,    \
                                        lazy_value)                            \
    }

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing out_handle provided by the user.
@param [in] frm the pointer to the ordered map.
@param [out] out_handle the handle to the user type wrapping map elem.
@return the removed entry. If Occupied it may be unwrapped to obtain the old key
value pair. If Vacant the key value pair was not stored in the map. If bad input
is provided an input error is set.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value. */
[[nodiscard]] ccc_entry ccc_frm_remove(ccc_flat_realtime_ordered_map *frm,
                                       ccc_fromap_elem *out_handle);

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing out_handle provided by the user.
@param [in] flat_realtime_ordered_map_ptr the pointer to the ordered map.
@param [out] out_handle_ptr the handle to the user type wrapping map elem.
@return a compound literal reference to the removed entry. If Occupied it may be
unwrapped to obtain the old key value pair. If Vacant the key value pair was not
stored in the map. If bad input is provided an input error is set.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value. */
#define ccc_frm_remove_r(flat_realtime_ordered_map_ptr, out_handle_ptr)        \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_frm_remove((flat_realtime_ordered_map_ptr), (out_handle_ptr))      \
            .impl_                                                             \
    }

/** @brief Obtains an entry for the provided key in the map for future use.
@param [in] frm the map to be searched.
@param [in] key the key used to search the map matching the stored key type.
@return a specialized entry for use with other functions in the Entry Interface.
@warning the contents of an entry should not be examined or modified. Use the
provided functions, only.

An entry is a search result that provides either an Occupied or Vacant entry
in the map. An occupied entry signifies that the search was successful. A
Vacant entry means the search was not successful but a handle is gained to
where in the map such an element should be inserted.

An entry is rarely useful on its own. It should be passed in a functional style
to subsequent calls in the Entry Interface. */
[[nodiscard]] ccc_fromap_entry
ccc_frm_entry(ccc_flat_realtime_ordered_map const *frm, void const *key);

/** @brief Obtains an entry for the provided key in the map for future use.
@param [in] flat_realtime_ordered_map_ptr the map to be searched.
@param [in] key_ptr the key used to search the map matching the stored key type.
@return a compound literal reference to a specialized entry for use with other
functions in the Entry Interface.
@warning the contents of an entry should not be examined or modified. Use the
provided functions, only.

An entry is a search result that provides either an Occupied or Vacant entry
in the map. An occupied entry signifies that the search was successful. A
Vacant entry means the search was not successful but a handle is gained to
where in the map such an element should be inserted.

An entry is rarely useful on its own. It should be passed in a functional style
to subsequent calls in the Entry Interface. */
#define ccc_frm_entry_r(flat_realtime_ordered_map_ptr, key_ptr)                \
    &(ccc_fromap_entry)                                                        \
    {                                                                          \
        ccc_frm_entry((flat_realtime_ordered_map_ptr), (key_ptr)).impl_        \
    }

/** @brief Modifies the provided entry if it is Occupied.
@param [in] e the entry obtained from an entry function or macro.
@param [in] fn an update function in which the auxiliary argument is unused.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function is intended to make the function chaining in the Entry Interface
more succinct if the entry will be modified in place based on its own value
without the need of the auxiliary argument a ccc_update_fn can provide. */
[[nodiscard]] ccc_fromap_entry *ccc_frm_and_modify(ccc_fromap_entry *e,
                                                   ccc_update_fn *fn);

/** @brief Modifies the provided entry if it is Occupied.
@param [in] e the entry obtained from an entry function or macro.
@param [in] fn an update function that requires auxiliary data.
@param [in] aux auxiliary data required for the update.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function makes full use of a ccc_update_fn capability, meaning a complete
ccc_update object will be passed to the update function callback. */
[[nodiscard]] ccc_fromap_entry *
ccc_frm_and_modify_aux(ccc_fromap_entry *e, ccc_update_fn *fn, void *aux);

/** @brief Modify an Occupied entry with a closure over user type T.
@param [in] flat_realtime_ordered_map_entry_ptr a pointer to the obtained entry.
@param [in] type_name the name of the user type stored in the container.
@param [in] closure_over_T the code to be run on the reference to user type T,
if Occupied. This may be a semicolon separated list of statements to execute on
T or a section of code wrapped in braces {code here} which may be preferred
for formatting.
@return a compound literal reference to the modified entry if it was occupied
or a vacant entry if it was vacant.
@note T is a reference to the user type stored in the entry guaranteed to be
non-NULL if the closure executes.

```
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
// Increment the key k if found otherwise do nothing.
frm_entry *e = frm_and_modify_w(entry_r(&frm, &k), word, T->cnt++;);

// Increment the key k if found otherwise insert a default value.
word *w = frm_or_insert_w(frm_and_modify_w(entry_r(&frm, &k), word,
                                           { T->cnt++; }),
                          (word){.key = k, .cnt = 1});
```

Note that any code written is only evaluated if the entry is Occupied and the
container can deliver the user type T. This means any function calls are lazily
evaluated in the closure scope. */
#define ccc_frm_and_modify_w(flat_realtime_ordered_map_entry_ptr, type_name,   \
                             closure_over_T...)                                \
    &(ccc_fromap_entry)                                                        \
    {                                                                          \
        ccc_impl_frm_and_modify_w(flat_realtime_ordered_map_entry_ptr,         \
                                  type_name, closure_over_T)                   \
    }

/** @brief Inserts the struct with handle elem if the entry is Vacant.
@param [in] e the entry obtained via function or macro call.
@param [in] elem the handle to the struct to be inserted to a Vacant entry.
@return a pointer to entry in the map invariantly. NULL on error.

Because this functions takes an entry and inserts if it is Vacant, the only
reason NULL shall be returned is when an insertion error occurs, usually due to
a user struct allocation failure.

If no allocation is permitted, this function assumes the user struct wrapping
elem has been allocated with the appropriate lifetime and scope by the user. */
[[nodiscard]] void *ccc_frm_or_insert(ccc_fromap_entry const *e,
                                      ccc_fromap_elem *elem);

/** @brief Lazily insert the desired key value into the entry if it is Vacant.
@param [in] flat_realtime_ordered_map_entry_ptr a pointer to the obtained entry.
@param [in] lazy_key_value the compound literal to construct in place if the
entry is Vacant.
@return a reference to the unwrapped user type in the entry, either the
unmodified reference if the entry was Occupied or the newly inserted element
if the entry was Vacant. NULL is returned if resizing is required but fails or
is not allowed.

Note that if the compound literal uses any function calls to generate values
or other data, such functions will not be called if the entry is Occupied. */
#define ccc_frm_or_insert_w(flat_realtime_ordered_map_entry_ptr,               \
                            lazy_key_value...)                                 \
    ccc_impl_frm_or_insert_w(flat_realtime_ordered_map_entry_ptr,              \
                             lazy_key_value)

/** @brief Inserts the provided entry invariantly.
@param [in] e the entry returned from a call obtaining an entry.
@param [in] elem a handle to the struct the user intends to insert.
@return a pointer to the inserted element or NULL upon allocation failure.

This method can be used when the old value in the map does not need to
be preserved. See the regular insert method if the old value is of interest. */
[[nodiscard]] void *ccc_frm_insert_entry(ccc_fromap_entry const *e,
                                         ccc_fromap_elem *elem);

/** @brief Write the contents of the compound literal lazy_key_value to a node.
@param [in] flat_realtime_ordered_map_entry_ptr a pointer to the obtained entry.
@param [in] lazy_key_value the compound literal to write to a new slot.
@return a reference to the newly inserted or overwritten user type. NULL is
returned if allocation failed or is not allowed when required. */
#define ccc_frm_insert_entry_w(flat_realtime_ordered_map_entry_ptr,            \
                               lazy_key_value...)                              \
    ccc_impl_frm_insert_entry_w(flat_realtime_ordered_map_entry_ptr,           \
                                lazy_key_value)

/** @brief Remove the entry from the map if Occupied.
@param [in] e a pointer to the map entry.
@return an entry containing NULL or a reference to the old entry. If Occupied an
entry in the map existed and was removed. If Vacant, no prior entry existed to
be removed.
@warning the reference to the removed entry is invalidated upon any further
insertions. */
ccc_entry ccc_frm_remove_entry(ccc_fromap_entry const *e);

/** @brief Remove the entry from the map if Occupied.
@param [in] flat_realtime_ordered_map_entry_ptr a pointer to the map entry.
@return a compound literal reference to an entry containing NULL or a reference
to the old entry. If Occupied an entry in the map existed and was removed. If
Vacant, no prior entry existed to be removed.

Note that the reference to the removed entry is invalidated upon any further
insertions. */
#define ccc_frm_remove_entry_r(flat_realtime_ordered_map_entry_ptr)            \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_frm_remove_entry((flat_realtime_ordered_map_entry_ptr)).impl_      \
    }

/** @brief Unwraps the provided entry to obtain a view into the map element.
@param [in] e the entry from a query to the map via function or macro.
@return a view into the table entry if one is present, or NULL. */
[[nodiscard]] void *ccc_frm_unwrap(ccc_fromap_entry const *e);

/** @brief Returns the Vacant or Occupied status of the entry.
@param [in] e the entry from a query to the map via function or macro.
@return true if the entry is occupied, false if not. */
[[nodiscard]] bool ccc_frm_occupied(ccc_fromap_entry const *e);

/** @brief Provides the status of the entry should an insertion follow.
@param [in] e the entry from a query to the table via function or macro.
@return true if an entry obtained from an insertion attempt failed to insert
due to an allocation failure when allocation success was expected. */
[[nodiscard]] bool ccc_frm_insert_error(ccc_fromap_entry const *e);

/** @brief Obtain the entry status from a container entry.
@param [in] e a pointer to the entry.
@return the status stored in the entry after the required action on the
container completes. If e is NULL an entry input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned.

Note that this function can be useful for debugging or if more detailed
messages are needed for logging purposes. See ccc_entry_status_msg() in
ccc/types.h for more information on detailed entry statuses. */
[[nodiscard]] ccc_entry_status ccc_frm_entry_status(ccc_fromap_entry const *e);

/**@}*/

/** @name Deallocation Interface
Deallocate the container. */
/**@{*/

/** @brief Frees all slots in the map for use without affecting capacity.
@param [in] frm the map to be cleared.
@param [in] fn the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the map before their slots are
forfeit.

If NULL is passed as the destructor function time is O(1), else O(size). */
ccc_result ccc_frm_clear(ccc_flat_realtime_ordered_map *frm,
                         ccc_destructor_fn *fn);

/** @brief Frees all slots in the map and frees the underlying buffer.
@param [in] frm the map to be cleared.
@param [in] fn the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the map before their slots are
forfeit.
@return the result of free operation. If no alloc function is provided it is
an error to attempt to free the buffer and a memory error is returned.
Otherwise, an OK result is returned.

If NULL is passed as the destructor function time is O(1), else O(size). */
ccc_result ccc_frm_clear_and_free(ccc_flat_realtime_ordered_map *frm,
                                  ccc_destructor_fn *fn);

/**@}*/

/** @name Iterator Interface
Obtain and manage iterators over the container. */
/**@{*/

/** @brief Return an iterable range of values from [begin_key, end_key).
O(lg N).
@param [in] frm a pointer to the map.
@param [in] begin_key a pointer to the key intended as the start of the range.
@param [in] end_key a pointer to the key intended as the end of the range.
@return a range containing the first element NOT LESS than the begin_key and
the first element GREATER than end_key.

Note that due to the variety of values that can be returned in the range, using
the provided range iteration functions from types.h is recommended for example:

for (struct val *i = range_begin(&range);
     i != end_range(&range);
     i = next(&frm, &i->elem))
{}

This avoids any possible errors in handling an end range element that is in the
map versus the end map sentinel. */
[[nodiscard]] ccc_range
ccc_frm_equal_range(ccc_flat_realtime_ordered_map const *frm,
                    void const *begin_key, void const *end_key);

/** @brief Returns a compound literal reference to the desired range. O(lg N).
@param [in] flat_realtime_ordered_map_ptr a pointer to the map.
@param [in] begin_and_end_key_ptrs two pointers, the first to the start of the
range the second to the end of the range.
@return a compound literal reference to the produced range associated with the
enclosing scope. This reference is always non-NULL. */
#define ccc_frm_equal_range_r(flat_realtime_ordered_map_ptr,                   \
                              begin_and_end_key_ptrs...)                       \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_frm_equal_range((flat_realtime_ordered_map_ptr),                   \
                            (begin_and_end_key_ptrs))                          \
            .impl_                                                             \
    }

/** @brief Return an iterable rrange of values from [begin_key, end_key).
O(lg N).
@param [in] frm a pointer to the map.
@param [in] rbegin_key a pointer to the key intended as the start of the rrange.
@param [in] rend_key a pointer to the key intended as the end of the rrange.
@return a rrange containing the first element NOT GREATER than the begin_key and
the first element LESS than rend_key.

Note that due to the variety of values that can be returned in the rrange, using
the provided rrange iteration functions from types.h is recommended for example:

for (struct val *i = rrange_begin(&rrange);
     i != rend_rrange(&rrange);
     i = rnext(&fom, &i->elem))
{}

This avoids any possible errors in handling an rend rrange element that is in
the map versus the end map sentinel. */
[[nodiscard]] ccc_rrange
ccc_frm_equal_rrange(ccc_flat_realtime_ordered_map const *frm,
                     void const *rbegin_key, void const *rend_key);

/** @brief Returns a compound literal reference to the desired rrange.
O(lg N).
@param [in] flat_realtime_ordered_map_ptr a pointer to the map.
@param [in] rbegin_and_rend_key_ptrs two pointers, the first to the start of the
rrange the second to the end of the rrange.
@return a compound literal reference to the produced rrange associated with the
enclosing scope. This reference is always non-NULL. */
#define ccc_frm_equal_rrange_r(flat_realtime_ordered_map_ptr,                  \
                               rbegin_and_rend_key_ptrs...)                    \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_frm_equal_rrange((flat_realtime_ordered_map_ptr),                  \
                             (rbegin_and_rend_key_ptrs))                       \
            .impl_                                                             \
    }

/** @brief Return the start of an inorder traversal of the map. O(lg N).
@param [in] frm a pointer to the map.
@return the oldest minimum element of the map. */
[[nodiscard]] void *ccc_frm_begin(ccc_flat_realtime_ordered_map const *frm);

/** @brief Return the start of a reverse inorder traversal of the map. O(lg N).
@param [in] frm a pointer to the map.
@return the oldest maximum element of the map. */
[[nodiscard]] void *ccc_frm_rbegin(ccc_flat_realtime_ordered_map const *frm);

/** @brief Return the next element in an inorder traversal of the map. O(1).
@param [in] frm a pointer to the map.
@param [in] iter_handle a pointer to the intrusive map element of the
current iterator.
@return the next user type stored in the map in an inorder traversal. */
[[nodiscard]] void *ccc_frm_next(ccc_flat_realtime_ordered_map const *frm,
                                 ccc_fromap_elem const *iter_handle);

/** @brief Return the rnext element in a reverse inorder traversal of the map.
O(1).
@param [in] frm a pointer to the map.
@param [in] iter_handle a pointer to the intrusive map element of the
current iterator.
@return the rnext user type stored in the map in a reverse inorder traversal. */
[[nodiscard]] void *ccc_frm_rnext(ccc_flat_realtime_ordered_map const *frm,
                                  ccc_fromap_elem const *iter_handle);

/** @brief Return the end of an inorder traversal of the map. O(1).
@param [in] frm a pointer to the map.
@return the newest maximum element of the map. */
[[nodiscard]] void *ccc_frm_end(ccc_flat_realtime_ordered_map const *frm);

/** @brief Return the rend of a reverse inorder traversal of the map. O(1).
@param [in] frm a pointer to the map.
@return the newest minimum element of the map. */
[[nodiscard]] void *ccc_frm_rend(ccc_flat_realtime_ordered_map const *frm);

/**@}*/

/** @name State Interface
Obtain the container state. */
/**@{*/

/** @brief Returns the size status of the map.
@param [in] frm the map.
@return true if empty else false. */
[[nodiscard]] bool ccc_frm_is_empty(ccc_flat_realtime_ordered_map const *frm);

/** @brief Returns the size of the map
@param [in] frm the map.
@return the size_t size. */
[[nodiscard]] size_t ccc_frm_size(ccc_flat_realtime_ordered_map const *frm);

/** @brief Returns the capacity of the map
@param [in] frm the map.
@return the size_t capacity. */
[[nodiscard]] size_t ccc_frm_capacity(ccc_flat_realtime_ordered_map const *frm);

/** @brief Return a reference to the base of backing array. O(1).
@param [in] frm a pointer to the map.
@return a reference to the base of the backing array.
@note the reference is to the base of the backing array at index 0 with no
consideration for the organization of map. However, all nodes of the map
are guaranteed to be stored contiguously starting at index 1. Index 0 is
reserved for the sentinel node.
@warning it is the users responsibility to ensure that access to any data is
within the capacity of the backing buffer. */
[[nodiscard]] void *ccc_frm_data(ccc_flat_realtime_ordered_map const *frm);

/** @brief Validation of invariants for the map.
@param [in] frm the map to validate.
@return true if all invariants hold, false if corruption occurs. */
[[nodiscard]] bool ccc_frm_validate(ccc_flat_realtime_ordered_map const *frm);

/**@}*/

/** Define this preprocessor directive if shorter names are helpful. Ensure
 no namespace clashes occur before shortening. */
#ifdef FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
typedef ccc_fromap_elem fromap_elem;
typedef ccc_flat_realtime_ordered_map flat_realtime_ordered_map;
typedef ccc_fromap_entry fromap_entry;
#    define frm_and_modify_w(args...) ccc_frm_and_modify_w(args)
#    define frm_or_insert_w(args...) ccc_frm_or_insert_w(args)
#    define frm_insert_entry_w(args...) ccc_frm_insert_entry_w(args)
#    define frm_try_insert_w(args...) ccc_frm_try_insert_w(args)
#    define frm_insert_or_assign_w(args...) ccc_frm_insert_or_assign_w(args)
#    define frm_init(args...) ccc_frm_init(args)
#    define frm_copy(args...) ccc_frm_copy(args)
#    define frm_contains(args...) ccc_frm_contains(args)
#    define frm_get_key_val(args...) ccc_frm_get_key_val(args)
#    define frm_insert(args...) ccc_frm_insert(args)
#    define frm_begin(args...) ccc_frm_begin(args)
#    define frm_rbegin(args...) ccc_frm_rbegin(args)
#    define frm_next(args...) ccc_frm_next(args)
#    define frm_rnext(args...) ccc_frm_rnext(args)
#    define frm_end(args...) ccc_frm_end(args)
#    define frm_rend(args...) ccc_frm_rend(args)
#    define frm_data(args...) ccc_frm_data(args)
#    define frm_is_empty(args...) ccc_frm_is_empty(args)
#    define frm_size(args...) ccc_frm_size(args)
#    define frm_clear(args...) ccc_frm_clear(args)
#    define frm_clear_and_free(args...) ccc_frm_clear_and_free(args)
#    define frm_validate(args...) ccc_frm_validate(args)
#endif /* FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC */

#endif /* CCC_FLAT_REALTIME_ORDERED_MAP_H */
