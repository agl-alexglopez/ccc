/** @file
@brief The Realtime Ordered Map Interface */
#ifndef CCC_REALTIME_ORDERED_MAP_H
#define CCC_REALTIME_ORDERED_MAP_H

#include <stddef.h>

#include "impl_realtime_ordered_map.h"
#include "types.h"

/** @brief A container for amortized O(lg N) search, insert, and erase and
pointer stability.

This map offers a strict runtime bound of O(lg N) which is helpful in realtime
environments. Also, searching is a thread-safe read-only operation. Balancing
modifications only occur upon insert or remove. */
typedef struct ccc_romap_ ccc_realtime_ordered_map;

/** @brief The intrusive element of the user defined struct being stored in the
map.

It can be used in an allocating or non allocating container. If allocation is
prohibited the container assumes the element is wrapped in pre-allocated
memory with the appropriate lifetime and scope for the user's needs; the
container does not allocate or free in this case. If allocation is allowed
the container will handle copying the data wrapping the element to allocations
and deallocating when necessary. */
typedef struct ccc_romap_elem_ ccc_romap_elem;

/** @brief A container specific entry used to implement the Entry API.

The Entry API offers efficient search and subsequent insertion, deletion, or
value update based on the needs of the user. */
typedef union ccc_romap_entry_ ccc_romap_entry;

/** @brief Initializes the ordered map at runtime or compile time.
@param [in] rom_name the name of the ordered map being initialized.
@param [in] struct_name the user type wrapping the intrusive element.
@param [in] rom_elem_field the name of the intrusive map elem field.
@param [in] key_elem_field the name of the field in user type used as key.
@param [in] alloc_fn the allocation function or NULL if allocation is banned.
@param [in] key_cmp_fn the key comparison function (see types.h).
@param [in] aux_data a pointer to any auxiliary data for comparison or
destruction.
@return the struct initialized ordered map for direct assignment
(i.e. ccc_realtime_ordered_map m = ccc_rom_init(...);). */
#define ccc_rom_init(rom_name, struct_name, rom_elem_field, key_elem_field,    \
                     alloc_fn, key_cmp_fn, aux_data)                           \
    ccc_impl_rom_init(rom_name, struct_name, rom_elem_field, key_elem_field,   \
                      alloc_fn, key_cmp_fn, aux_data)

/*=================    Membership and Retrieval    ==========================*/

/** @brief Searches the map for the presence of key.
@param [in] rom the map to be searched.
@param [in] key pointer to the key matching the key type of the user struct.
@return true if the struct containing key is stored, false if not. */
[[nodiscard]] bool ccc_rom_contains(ccc_realtime_ordered_map const *rom,
                                    void const *key);

/** @brief Returns a reference into the map at entry key.
@param [in] rom the ordered map to search.
@param [in] key the key to search matching stored key type.
@return a view of the map entry if it is present, else NULL. */
[[nodiscard]] void *ccc_rom_get_key_val(ccc_realtime_ordered_map const *rom,
                                        void const *key);

/*======================      Entry API    ==================================*/

/** @brief Invariantly inserts the key value wrapping key_val_handle.
@param [in] rom the pointer to the ordered map.
@param [in] key_val_handle the handle to the user type wrapping map elem.
@param [in] tmp handle to space for swapping in the old value, if present. The
same user type stored in the map should wrap tmp.
@return an entry. If Vacant, no prior element with key existed and the type
wrapping tmp remains unchanged. If Occupied the old value is written
to the type wrapping tmp and may be unwrapped to view. If more space
is needed but allocation fails or has been forbidden, an insert error is set.

Note that this function may write to the struct containing tmp and wraps it in
an entry to provide information about the old value. */
[[nodiscard]] ccc_entry ccc_rom_insert(ccc_realtime_ordered_map *rom,
                                       ccc_romap_elem *key_val_handle,
                                       ccc_romap_elem *tmp);

/** @brief Invariantly inserts the key value wrapping key_val_handle_ptr.
@param [in] realtime_ordered_map_ptr the pointer to the ordered map.
@param [in] key_val_handle_ptr the handle to the user type wrapping map elem.
@param [in] tmp_ptr handle to space for swapping in the old value, if present.
The same user type stored in the map should wrap tmp_ptr.
@return a compound literal reference to an entry. If Vacant, no prior element
with key existed and the type wrapping tmp_ptr remains unchanged. If Occupied
the old value is written to the type wrapping tmp_ptr and may be unwrapped to
view. If more space is needed but allocation fails or has been forbidden, an
insert error is set.

Note that this function may write to the struct containing tmp_ptr and wraps it
in an entry to provide information about the old value. */
#define ccc_rom_insert_r(realtime_ordered_map_ptr, key_val_handle_ptr,         \
                         tmp_ptr)                                              \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_rom_insert((realtime_ordered_map_ptr), (key_val_handle_ptr),       \
                       (tmp_ptr))                                              \
            .impl_                                                             \
    }

/** @brief Attempts to insert the key value wrapping key_val_handle.
@param [in] rom the pointer to the map.
@param [in] key_val_handle the handle to the user type wrapping map elem.
@return an entry. If Occupied, the entry contains a reference to the key value
user type in the map and may be unwrapped. If Vacant the entry contains a
reference to the newly inserted entry in the map. If more space is needed but
allocation fails, an insert error is set. */
[[nodiscard]] ccc_entry ccc_rom_try_insert(ccc_realtime_ordered_map *rom,
                                           ccc_romap_elem *key_val_handle);

/** @brief Attempts to insert the key value wrapping key_val_handle.
@param [in] realtime_ordered_map_ptr the pointer to the map.
@param [in] key_val_handle_ptr the handle to the user type wrapping map elem.
@return a compound literal reference to an entry. If Occupied, the entry
contains a reference to the key value user type in the map and may be unwrapped.
If Vacant the entry contains a reference to the newly inserted entry in the map.
If more space is needed but allocation fails or has been forbidden, an insert
error is set. */
#define ccc_rom_try_insert_r(realtime_ordered_map_ptr, key_val_handle_ptr)     \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_rom_try_insert((realtime_ordered_map_ptr), (key_val_handle_ptr))   \
            .impl_                                                             \
    }

/** @brief lazily insert lazy_value into the map at key if key is absent.
@param [in] realtime_ordered_map_ptr a pointer to the map.
@param [in] key the direct key r-value.
@param [in] lazy_value the compound literal specifying the value.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define ccc_rom_try_insert_w(realtime_ordered_map_ptr, key, lazy_value...)     \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_rom_try_insert_w(realtime_ordered_map_ptr, key, lazy_value)   \
    }

/** @brief Invariantly inserts or overwrites a user struct into the map.
@param [in] rom a pointer to the flat hash map.
@param [in] key_val_handle the handle to the wrapping user struct key value.
@return an entry. If Occupied an entry was overwritten by the new key value. If
Vacant no prior map entry existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
[[nodiscard]] ccc_entry
ccc_rom_insert_or_assign(ccc_realtime_ordered_map *rom,
                         ccc_romap_elem *key_val_handle);

/** @brief Inserts a new key value pair or overwrites the existing entry.
@param [in] realtime_ordered_map_ptr the pointer to the flat hash map.
@param [in] key the key to be searched in the map.
@param [in] lazy_value the compound literal to insert or use for overwrite.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define ccc_rom_insert_or_assign_w(realtime_ordered_map_ptr, key,              \
                                   lazy_value...)                              \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_rom_insert_or_assign_w(realtime_ordered_map_ptr, key,         \
                                        lazy_value)                            \
    }

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing out_handle provided by the user.
@param [in] rom the pointer to the ordered map.
@param [out] out_handle the handle to the user type wrapping map elem.
@return the removed entry. If Occupied it may be unwrapped to obtain the old key
value pair. If Vacant the key value pair was not stored in the map. If bad input
is provided an input error is set.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value.

If allocation has been prohibited upon initialization then the entry returned
contains the previously stored user type, if any, and nothing is written to
the out_handle. It is then the user's responsibility to manage their previously
stored memory as they see fit. */
[[nodiscard]] ccc_entry ccc_rom_remove(ccc_realtime_ordered_map *rom,
                                       ccc_romap_elem *out_handle);

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing out_handle_ptr provided by the user.
@param [in] realtime_ordered_map_ptr the pointer to the ordered map.
@param [out] out_handle_ptr the handle to the user type wrapping map elem.
@return a compound literal reference to the removed entry. If Occupied it may be
unwrapped to obtain the old key value pair. If Vacant the key value pair was not
stored in the map. If bad input is provided an input error is set.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value.

If allocation has been prohibited upon initialization then the entry returned
contains the previously stored user type, if any, and nothing is written to
the out_handle_ptr. It is then the user's responsibility to manage their
previously stored memory as they see fit. */
#define ccc_rom_remove_r(realtime_ordered_map_ptr, out_handle_ptr)             \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_rom_remove((realtime_ordered_map_ptr), (out_handle_ptr)).impl_     \
    }

/** @brief Obtains an entry for the provided key in the map for future use.
@param [in] rom the map to be searched.
@param [in] key the key used to search the map matching the stored key type.
@return a specialized entry for use with other functions in the Entry API.
@warning the contents of an entry should not be examined or modified. Use the
provided functions, only.

An entry is a search result that provides either an Occupied or Vacant entry
in the map. An occupied entry signifies that the search was successful. A
Vacant entry means the search was not successful but a handle is gained to
where in the map such an element should be inserted.

An entry is rarely useful on its own. It should be passed in a functional style
to subsequent calls in the Entry API. */
[[nodiscard]] ccc_romap_entry ccc_rom_entry(ccc_realtime_ordered_map const *rom,
                                            void const *key);

/** @brief Obtains an entry for the provided key in the map for future use.
@param [in] realtime_ordered_map_ptr the map to be searched.
@param [in] key_ptr the key used to search the map matching the stored key type.
@return a compound literal reference to a specialized entry for use with other
functions in the Entry API.
@warning the contents of an entry should not be examined or modified. Use the
provided functions, only.

An entry is a search result that provides either an Occupied or Vacant entry
in the map. An occupied entry signifies that the search was successful. A
Vacant entry means the search was not successful but a handle is gained to
where in the map such an element should be inserted.

An entry is rarely useful on its own. It should be passed in a functional style
to subsequent calls in the Entry API. */
#define ccc_rom_entry_r(realtime_ordered_map_ptr, key_ptr)                     \
    &(ccc_romap_entry)                                                         \
    {                                                                          \
        ccc_rom_entry((realtime_ordered_map_ptr), (key_ptr)).impl_             \
    }

/** @brief Modifies the provided entry if it is Occupied.
@param [in] e the entry obtained from an entry function or macro.
@param [in] fn an update function in which the auxiliary argument is unused.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function is intended to make the function chaining in the Entry API more
succinct if the entry will be modified in place based on its own value without
the need of the auxiliary argument a ccc_update_fn can provide. */
[[nodiscard]] ccc_romap_entry *ccc_rom_and_modify(ccc_romap_entry *e,
                                                  ccc_update_fn *fn);

/** @brief Modifies the provided entry if it is Occupied.
@param [in] e the entry obtained from an entry function or macro.
@param [in] fn an update function that requires auxiliary data.
@param [in] aux auxiliary data required for the update.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function makes full use of a ccc_update_fn capability, meaning a complete
ccc_update object will be passed to the update function callback. */
[[nodiscard]] ccc_romap_entry *
ccc_rom_and_modify_aux(ccc_romap_entry *e, ccc_update_fn *fn, void *aux);

/** @brief modify the value stored in the map entry with a modification
function and lazily constructed auxiliary data.
@param [in] realtime_ordered_map_entry_ptr a pointer to the obtained entry.
@param [in] mod_fn the function used to modify the entry value.
@param [in] aux_data lazily constructed auxiliary data for mod_fn.
@return a compound literal reference to the modified entry if it was occupied
or a vacant entry if it was vacant.

Note that if aux is a function call to generate a value it will only be called
if the entry is occupied and thus able to be modified. */
#define ccc_rom_and_modify_w(realtime_ordered_map_entry_ptr, mod_fn,           \
                             aux_data...)                                      \
    &(ccc_romap_entry)                                                         \
    {                                                                          \
        ccc_impl_rom_and_modify_w(realtime_ordered_map_entry_ptr, mod_fn,      \
                                  aux_data)                                    \
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
[[nodiscard]] void *ccc_rom_or_insert(ccc_romap_entry const *e,
                                      ccc_romap_elem *elem);

/** @brief Lazily insert the desired key value into the entry if it is Vacant.
@param [in] realtime_ordered_map_entry_ptr a pointer to the obtained entry.
@param [in] lazy_key_value the compound literal to construct in place if the
entry is Vacant.
@return a reference to the unwrapped user type in the entry, either the
unmodified reference if the entry was Occupied or the newly inserted element
if the entry was Vacant. NULL is returned if resizing is required but fails or
is not allowed.

Note that if the compound literal uses any function calls to generate values
or other data, such functions will not be called if the entry is Occupied. */
#define ccc_rom_or_insert_w(realtime_ordered_map_entry_ptr, lazy_key_value...) \
    ccc_impl_rom_or_insert_w(realtime_ordered_map_entry_ptr, lazy_key_value)

/** @brief Inserts the provided entry invariantly.
@param [in] e the entry returned from a call obtaining an entry.
@param [in] elem a handle to the struct the user intends to insert.
@return a pointer to the inserted element or NULL upon allocation failure.

This method can be used when the old value in the map does not need to
be preserved. See the regular insert method if the old value is of interest. */
[[nodiscard]] void *ccc_rom_insert_entry(ccc_romap_entry const *e,
                                         ccc_romap_elem *elem);

/** @brief Write the contents of the compound literal lazy_key_value to a node.
@param [in] realtime_ordered_map_entry_ptr a pointer to the obtained entry.
@param [in] lazy_key_value the compound literal to write to a new slot.
@return a reference to the newly inserted or overwritten user type. NULL is
returned if allocation failed or is not allowed when required. */
#define ccc_rom_insert_entry_w(realtime_ordered_map_entry_ptr,                 \
                               lazy_key_value...)                              \
    ccc_impl_rom_insert_entry_w(realtime_ordered_map_entry_ptr, lazy_key_value)

/** @brief Remove the entry from the map if Occupied.
@param [in] e a pointer to the map entry.
@return an entry containing NULL or a reference to the old entry. If Occupied an
entry in the map existed and was removed. If Vacant, no prior entry existed to
be removed.

Note that if allocation is permitted the old element is freed and the entry
will contain a NULL reference. If allocation is prohibited the entry can be
unwrapped to obtain the old user struct stored in the map and the user may
free or use as needed. */
[[nodiscard]] ccc_entry ccc_rom_remove_entry(ccc_romap_entry const *e);

/** @brief Remove the entry from the map if Occupied.
@param [in] realtime_ordered_map_entry_ptr a pointer to the map entry.
@return a compound literal reference to an entry containing NULL or a reference
to the old entry. If Occupied an entry in the map existed and was removed. If
Vacant, no prior entry existed to be removed.

Note that if allocation is permitted the old element is freed and the entry
will contain a NULL reference. If allocation is prohibited the entry can be
unwrapped to obtain the old user struct stored in the map and the user may
free or use as needed. */
#define ccc_rom_remove_entry_r(realtime_ordered_map_entry_ptr)                 \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_rom_remove_entry((realtime_ordered_map_entry_ptr)).impl_           \
    }

/** @brief Unwraps the provided entry to obtain a view into the map element.
@param [in] e the entry from a query to the map via function or macro.
@return a view into the table entry if one is present, or NULL. */
[[nodiscard]] void *ccc_rom_unwrap(ccc_romap_entry const *e);

/** @brief Returns the Vacant or Occupied status of the entry.
@param [in] e the entry from a query to the map via function or macro.
@return true if the entry is occupied, false if not. */
[[nodiscard]] bool ccc_rom_insert_error(ccc_romap_entry const *e);

/** @brief Provides the status of the entry should an insertion follow.
@param [in] e the entry from a query to the table via function or macro.
@return true if an entry obtained from an insertion attempt failed to insert
due to an allocation failure when allocation success was expected. */
[[nodiscard]] bool ccc_rom_occupied(ccc_romap_entry const *e);

/*======================      Iteration    ==================================*/

/** @brief Return an iterable range of values from [begin_key, end_key).
Amortized O(lg N).
@param [in] rom a pointer to the map.
@param [in] begin_key a pointer to the key intended as the start of the range.
@param [in] end_key a pointer to the key intended as the end of the range.
@return a range containing the first element NOT LESS than the begin_key and
the first element GREATER than end_key.

Note that due to the variety of values that can be returned in the range, using
the provided range iteration functions from types.h is recommended for example:

for (struct val *i = range_begin(&range);
     i != end_range(&range);
     i = next(&omm, &i->elem))
{}

This avoids any possible errors in handling an end range element that is in the
map versus the end map sentinel. */
[[nodiscard]] ccc_range ccc_rom_equal_range(ccc_realtime_ordered_map const *rom,
                                            void const *begin_key,
                                            void const *end_key);

/** @brief Returns a compound literal reference to the desired range. Amortized
O(lg N).
@param [in] realtime_ordered_map_ptr a pointer to the map.
@param [in] begin_and_end_key_ptrs two pointers, the first to the start of the
range the second to the end of the range.
@return a compound literal reference to the produced range associated with the
enclosing scope. This reference is always non-NULL. */
#define ccc_rom_equal_range_r(realtime_ordered_map_ptr,                        \
                              begin_and_end_key_ptrs...)                       \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_rom_equal_range((realtime_ordered_map_ptr),                        \
                            (begin_and_end_key_ptrs))                          \
            .impl_                                                             \
    }

/** @brief Return an iterable rrange of values from [begin_key, end_key).
Amortized O(lg N).
@param [in] rom a pointer to the map.
@param [in] rbegin_key a pointer to the key intended as the start of the rrange.
@param [in] rend_key a pointer to the key intended as the end of the rrange.
@return a rrange containing the first element NOT GREATER than the begin_key and
the first element LESS than rend_key.

Note that due to the variety of values that can be returned in the rrange, using
the provided rrange iteration functions from types.h is recommended for example:

for (struct val *i = rrange_begin(&rrange);
     i != rend_rrange(&rrange);
     i = rnext(&omm, &i->elem))
{}

This avoids any possible errors in handling an rend rrange element that is in
the map versus the end map sentinel. */
[[nodiscard]] ccc_rrange
ccc_rom_equal_rrange(ccc_realtime_ordered_map const *rom,
                     void const *rbegin_key, void const *rend_key);

/** @brief Returns a compound literal reference to the desired rrange. Amortized
O(lg N).
@param [in] realtime_ordered_map_ptr a pointer to the map.
@param [in] rbegin_and_rend_key_ptrs two pointers, the first to the start of the
rrange the second to the end of the rrange.
@return a compound literal reference to the produced rrange associated with the
enclosing scope. This reference is always non-NULL. */
#define ccc_rom_equal_rrange_r(realtime_ordered_map_ptr,                       \
                               rbegin_and_rend_key_ptrs...)                    \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_rom_equal_rrange((realtime_ordered_map_ptr),                       \
                             (rbegin_and_rend_key_ptrs))                       \
            .impl_                                                             \
    }

/** @brief Return the start of an inorder traversal of the map. Amortized
O(lg N).
@param [in] rom a pointer to the map.
@return the oldest minimum element of the map. */
[[nodiscard]] void *ccc_rom_begin(ccc_realtime_ordered_map const *rom);

/** @brief Return the start of a reverse inorder traversal of the map.
Amortized O(lg N).
@param [in] rom a pointer to the map.
@return the oldest maximum element of the map. */
[[nodiscard]] void *ccc_rom_rbegin(ccc_realtime_ordered_map const *rom);

/** @brief Return the next element in an inorder traversal of the map. O(1).
@param [in] rom a pointer to the map.
@param [in] iter_handle a pointer to the intrusive map element of the
current iterator.
@return the next user type stored in the map in an inorder traversal. */
[[nodiscard]] void *ccc_rom_next(ccc_realtime_ordered_map const *rom,
                                 ccc_romap_elem const *iter_handle);

/** @brief Return the rnext element in a reverse inorder traversal of the map.
O(1).
@param [in] rom a pointer to the map.
@param [in] iter_handle a pointer to the intrusive map element of the
current iterator.
@return the rnext user type stored in the map in a reverse inorder traversal. */
[[nodiscard]] void *ccc_rom_rnext(ccc_realtime_ordered_map const *rom,
                                  ccc_romap_elem const *iter_handle);

/** @brief Return the end of an inorder traversal of the map. O(1).
@param [in] rom a pointer to the map.
@return the newest maximum element of the map. */
[[nodiscard]] void *ccc_rom_end(ccc_realtime_ordered_map const *rom);

/** @brief Return the rend of a reverse inorder traversal of the map. O(1).
@param [in] rom a pointer to the map.
@return the newest minimum element of the map. */
[[nodiscard]] void *ccc_rom_rend(ccc_realtime_ordered_map const *rom);

/** @brief Pops every element from the map calling destructor if destructor is
non-NULL. O(N).
@param [in] rom a pointer to the map.
@param [in] destructor a destructor function if required. NULL if unneeded.
@return an input error if rom points to NULL otherwise OK.

Note that if the map has been given permission to allocate, the destructor will
be called on each element before it uses the provided allocator to free the
element. Therefore, the destructor should not free the element or a double free
will occur.

If the container has not been given allocation permission, then the destructor
may free elements or not depending on how and when the user wishes to free
elements of the map according to their own memory management schemes. */
ccc_result ccc_rom_clear(ccc_realtime_ordered_map *rom,
                         ccc_destructor_fn *destructor);

/*======================       Getters     ==================================*/

/** @brief Returns the size of the map
@param [in] rom the map.
@return the size_t size. */
[[nodiscard]] size_t ccc_rom_size(ccc_realtime_ordered_map const *rom);

/** @brief Returns the size status of the map.
@param [in] rom the map.
@return true if empty else false. */
[[nodiscard]] bool ccc_rom_is_empty(ccc_realtime_ordered_map const *rom);

/** @brief Validation of invariants for the map.
@param [in] rom the map to validate.
@return true if all invariants hold, false if corruption occurs. */
[[nodiscard]] bool ccc_rom_validate(ccc_realtime_ordered_map const *rom);

/** Define this preprocessor directive if shorter names are helpful. Ensure
 no namespace clashes occur before shortening. */
#ifdef REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
typedef ccc_romap_elem romap_elem;
typedef ccc_realtime_ordered_map realtime_ordered_map;
typedef ccc_romap_entry romap_entry;
#    define rom_init(args...) ccc_rom_init(args)
#    define rom_and_modify_w(args...) ccc_rom_and_modify_w(args)
#    define rom_or_insert_w(args...) ccc_rom_or_insert_w(args)
#    define rom_insert_entry_w(args...) ccc_rom_insert_entry_w(args)
#    define rom_try_insert_w(args...) ccc_rom_try_insert_w(args)
#    define rom_insert_or_assign_w(args...) ccc_rom_insert_or_assign_w(args)
#    define rom_insert_r(args...) ccc_rom_insert_r(args)
#    define rom_remove_r(args...) ccc_rom_remove_r(args)
#    define rom_remove_entry_r(args...) ccc_rom_remove_entry_r(args)
#    define rom_entry_r(args...) ccc_rom_entry_r(args)
#    define rom_and_modify_r(args...) ccc_rom_and_modify_r(args)
#    define rom_and_modify_aux_r(args...) ccc_rom_and_modify_aux_r(args)
#    define rom_contains(args...) ccc_rom_contains(args)
#    define rom_get_key_val(args...) ccc_rom_get_key_val(args)
#    define rom_get_mut(args...) ccc_rom_get_mut(args)
#    define rom_insert(args...) ccc_rom_insert(args)
#    define rom_remove(args...) ccc_rom_remove(args)
#    define rom_entry(args...) ccc_rom_entry(args)
#    define rom_remove_entry(args...) ccc_rom_remove_entry(args)
#    define rom_or_insert(args...) ccc_rom_or_insert(args)
#    define rom_insert_entry(args...) ccc_rom_insert_entry(args)
#    define rom_unwrap(args...) ccc_rom_unwrap(args)
#    define rom_unwrap_mut(args...) ccc_rom_unwrap_mut(args)
#    define rom_begin(args...) ccc_rom_begin(args)
#    define rom_next(args...) ccc_rom_next(args)
#    define rom_rbegin(args...) ccc_rom_rbegin(args)
#    define rom_rnext(args...) ccc_rom_rnext(args)
#    define rom_end(args...) ccc_rom_end(args)
#    define rom_rend(args...) ccc_rom_rend(args)
#    define rom_size(args...) ccc_rom_size(args)
#    define rom_is_empty(args...) ccc_rom_is_empty(args)
#    define rom_clear(args...) ccc_rom_clear(args)
#    define rom_validate(args...) ccc_rom_validate(args)
#endif

#endif /* CCC_REALTIME_ORDERED_MAP_H */
