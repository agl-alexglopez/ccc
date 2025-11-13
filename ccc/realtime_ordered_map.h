/** @cond
Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
@endcond */
/** @file
@brief The Realtime Ordered Map Interface

A realtime ordered map offers storage and retrieval by key. This map offers
pointer stability and a strict runtime bound of O(lg N) which is helpful in
realtime environments. Also, searching is a thread-safe read-only operation.
Balancing modifications only occur upon insertion or removal.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `CCC_` prefix. */
#ifndef CCC_REALTIME_ORDERED_MAP_H
#define CCC_REALTIME_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "private/private_realtime_ordered_map.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A container for amortized O(lg N) search, insert, erase, ranges, and
pointer stability.
@warning it is undefined behavior to access an uninitialized container.

A realtime ordered map can be initialized on the stack, heap, or data segment at
runtime or compile time.*/
typedef struct CCC_Realtime_ordered_map CCC_Realtime_ordered_map;

/** @brief The intrusive element of the user defined struct being stored in the
map.

It can be used in an allocating or non allocating container. If allocation is
prohibited the container assumes the element is wrapped in pre-allocated
memory with the appropriate lifetime and scope for the user's needs; the
container does not allocate or free in this case. If allocation is allowed
the container will handle copying the data wrapping the element to allocations
and deallocating when necessary. */
typedef struct CCC_Realtime_ordered_map_node CCC_Realtime_ordered_map_node;

/** @brief A container specific entry used to implement the Entry Interface.

The Entry Interface offers efficient search and subsequent insertion, deletion,
or value update based on the needs of the user. */
typedef union CCC_Realtime_ordered_map_entry CCC_Realtime_ordered_map_entry;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initializes the ordered map at runtime or compile time.
@param [in] realtime_ordered_map_name the name of the ordered map being
initialized.
@param [in] struct_name the user type wrapping the intrusive element.
@param [in] realtime_ordered_map_node_field the name of the intrusive map elem
field.
@param [in] key_node_field the name of the field in user type used as key.
@param [in] key_order_fn the key comparison function (see types.h).
@param [in] allocate the allocation function or NULL if allocation is banned.
@param [in] context_data a pointer to any context data for comparison or
destruction.
@return the struct initialized ordered map for direct assignment
(i.e. CCC_Realtime_ordered_map m = CCC_realtime_ordered_map_initialize(...);).
*/
#define CCC_realtime_ordered_map_initialize(                                   \
    realtime_ordered_map_name, struct_name, realtime_ordered_map_node_field,   \
    key_node_field, key_order_fn, allocate, context_data)                      \
    CCC_private_realtime_ordered_map_initialize(                               \
        realtime_ordered_map_name, struct_name,                                \
        realtime_ordered_map_node_field, key_node_field, key_order_fn,         \
        allocate, context_data)

/**@}*/

/**@name Membership Interface
Test membership or obtain references to stored user types directly. */
/**@{*/

/** @brief Searches the map for the presence of key.
@param [in] rom the map to be searched.
@param [in] key pointer to the key matching the key type of the user struct.
@return true if the struct containing key is stored, false if not. Error if rom
or key is NULL.*/
[[nodiscard]] CCC_Tribool
CCC_realtime_ordered_map_contains(CCC_Realtime_ordered_map const *rom,
                                  void const *key);

/** @brief Returns a reference into the map at entry key.
@param [in] rom the ordered map to search.
@param [in] key the key to search matching stored key type.
@return a view of the map entry if it is present, else NULL. */
[[nodiscard]] void *
CCC_realtime_ordered_map_get_key_val(CCC_Realtime_ordered_map const *rom,
                                     void const *key);

/**@}*/

/** @name Entry Interface
Obtain and operate on container entries for efficient queries when non-trivial
control flow is needed. */
/**@{*/

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
[[nodiscard]] CCC_Entry CCC_realtime_ordered_map_swap_entry(
    CCC_Realtime_ordered_map *rom,
    CCC_Realtime_ordered_map_node *key_val_handle,
    CCC_Realtime_ordered_map_node *tmp);

/** @brief Invariantly inserts the key value wrapping key_val_handle_ptr.
@param [in] Realtime_ordered_map_ptr the pointer to the ordered map.
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
#define CCC_realtime_ordered_map_swap_entry_r(Realtime_ordered_map_ptr,        \
                                              key_val_handle_ptr, tmp_ptr)     \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_realtime_ordered_map_swap_entry((Realtime_ordered_map_ptr),        \
                                            (key_val_handle_ptr), (tmp_ptr))   \
            .private                                                           \
    }

/** @brief Attempts to insert the key value wrapping key_val_handle.
@param [in] rom the pointer to the map.
@param [in] key_val_handle the handle to the user type wrapping map elem.
@return an entry. If Occupied, the entry contains a reference to the key value
user type in the map and may be unwrapped. If Vacant the entry contains a
reference to the newly inserted entry in the map. If more space is needed but
allocation fails, an insert error is set. */
[[nodiscard]] CCC_Entry CCC_realtime_ordered_map_try_insert(
    CCC_Realtime_ordered_map *rom,
    CCC_Realtime_ordered_map_node *key_val_handle);

/** @brief Attempts to insert the key value wrapping key_val_handle.
@param [in] Realtime_ordered_map_ptr the pointer to the map.
@param [in] key_val_handle_ptr the handle to the user type wrapping map elem.
@return a compound literal reference to an entry. If Occupied, the entry
contains a reference to the key value user type in the map and may be unwrapped.
If Vacant the entry contains a reference to the newly inserted entry in the map.
If more space is needed but allocation fails or has been forbidden, an insert
error is set. */
#define CCC_realtime_ordered_map_try_insert_r(Realtime_ordered_map_ptr,        \
                                              key_val_handle_ptr)              \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_realtime_ordered_map_try_insert((Realtime_ordered_map_ptr),        \
                                            (key_val_handle_ptr))              \
            .private                                                           \
    }

/** @brief lazily insert lazy_value into the map at key if key is absent.
@param [in] Realtime_ordered_map_ptr a pointer to the map.
@param [in] key the direct key r-value.
@param [in] lazy_value the compound literal specifying the value.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define CCC_realtime_ordered_map_try_insert_w(Realtime_ordered_map_ptr, key,   \
                                              lazy_value...)                   \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_private_realtime_ordered_map_try_insert_w(                         \
            Realtime_ordered_map_ptr, key, lazy_value)                         \
    }

/** @brief Invariantly inserts or overwrites a user struct into the map.
@param [in] rom a pointer to the flat hash map.
@param [in] key_val_handle the handle to the wrapping user struct key value.
@return an entry. If Occupied an entry was overwritten by the new key value. If
Vacant no prior map entry existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
[[nodiscard]] CCC_Entry CCC_realtime_ordered_map_insert_or_assign(
    CCC_Realtime_ordered_map *rom,
    CCC_Realtime_ordered_map_node *key_val_handle);

/** @brief Inserts a new key value pair or overwrites the existing entry.
@param [in] Realtime_ordered_map_ptr the pointer to the flat hash map.
@param [in] key the key to be searched in the map.
@param [in] lazy_value the compound literal to insert or use for overwrite.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define CCC_realtime_ordered_map_insert_or_assign_w(Realtime_ordered_map_ptr,  \
                                                    key, lazy_value...)        \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_private_realtime_ordered_map_insert_or_assign_w(                   \
            Realtime_ordered_map_ptr, key, lazy_value)                         \
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
[[nodiscard]] CCC_Entry
CCC_realtime_ordered_map_remove(CCC_Realtime_ordered_map *rom,
                                CCC_Realtime_ordered_map_node *out_handle);

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing out_handle_ptr provided by the user.
@param [in] Realtime_ordered_map_ptr the pointer to the ordered map.
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
#define CCC_realtime_ordered_map_remove_r(Realtime_ordered_map_ptr,            \
                                          out_handle_ptr)                      \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_realtime_ordered_map_remove((Realtime_ordered_map_ptr),            \
                                        (out_handle_ptr))                      \
            .private                                                           \
    }

/** @brief Obtains an entry for the provided key in the map for future use.
@param [in] rom the map to be searched.
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
[[nodiscard]] CCC_Realtime_ordered_map_entry
CCC_realtime_ordered_map_entry(CCC_Realtime_ordered_map const *rom,
                               void const *key);

/** @brief Obtains an entry for the provided key in the map for future use.
@param [in] Realtime_ordered_map_ptr the map to be searched.
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
#define CCC_realtime_ordered_map_entry_r(Realtime_ordered_map_ptr, key_ptr)    \
    &(CCC_Realtime_ordered_map_entry)                                          \
    {                                                                          \
        CCC_realtime_ordered_map_entry((Realtime_ordered_map_ptr), (key_ptr))  \
            .private                                                           \
    }

/** @brief Modifies the provided entry if it is Occupied.
@param [in] e the entry obtained from an entry function or macro.
@param [in] fn an update function in which the context argument is unused.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function is intended to make the function chaining in the Entry Interface
more succinct if the entry will be modified in place based on its own value
without the need of the context argument a CCC_Type_updater can provide.
*/
[[nodiscard]] CCC_Realtime_ordered_map_entry *
CCC_realtime_ordered_map_and_modify(CCC_Realtime_ordered_map_entry *e,
                                    CCC_Type_updater *fn);

/** @brief Modifies the provided entry if it is Occupied.
@param [in] e the entry obtained from an entry function or macro.
@param [in] fn an update function that requires context data.
@param [in] context context data required for the update.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function makes full use of a CCC_Type_updater capability, meaning a
complete CCC_update object will be passed to the update function callback. */
[[nodiscard]] CCC_Realtime_ordered_map_entry *
CCC_realtime_ordered_map_and_modify_context(CCC_Realtime_ordered_map_entry *e,
                                            CCC_Type_updater *fn,
                                            void *context);

/** @brief Modify an Occupied entry with a closure over user type T.
@param [in] Realtime_ordered_map_entry_ptr a pointer to the obtained entry.
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
#define REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
// Increment the key k if found otherwise do nothing.
realtime_ordered_map_entry *e = realtime_ordered_map_and_modify_w(entry_r(&rom,
&k), word, T->cnt++;);

// Increment the key k if found otherwise insert a default value.
word *w =
realtime_ordered_map_or_insert_w(realtime_ordered_map_and_modify_w(entry_r(&rom,
&k), word, { T->cnt++; }), (word){.key = k, .cnt = 1});
```

Note that any code written is only evaluated if the entry is Occupied and the
container can deliver the user type T. This means any function calls are lazily
evaluated in the closure scope. */
#define CCC_realtime_ordered_map_and_modify_w(Realtime_ordered_map_entry_ptr,  \
                                              type_name, closure_over_T...)    \
    &(CCC_Realtime_ordered_map_entry)                                          \
    {                                                                          \
        CCC_private_realtime_ordered_map_and_modify_w(                         \
            Realtime_ordered_map_entry_ptr, type_name, closure_over_T)         \
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
[[nodiscard]] void *
CCC_realtime_ordered_map_or_insert(CCC_Realtime_ordered_map_entry const *e,
                                   CCC_Realtime_ordered_map_node *elem);

/** @brief Lazily insert the desired key value into the entry if it is Vacant.
@param [in] Realtime_ordered_map_entry_ptr a pointer to the obtained entry.
@param [in] lazy_key_value the compound literal to construct in place if the
entry is Vacant.
@return a reference to the unwrapped user type in the entry, either the
unmodified reference if the entry was Occupied or the newly inserted element
if the entry was Vacant. NULL is returned if resizing is required but fails or
is not allowed.

Note that if the compound literal uses any function calls to generate values
or other data, such functions will not be called if the entry is Occupied. */
#define CCC_realtime_ordered_map_or_insert_w(Realtime_ordered_map_entry_ptr,   \
                                             lazy_key_value...)                \
    CCC_private_realtime_ordered_map_or_insert_w(                              \
        Realtime_ordered_map_entry_ptr, lazy_key_value)

/** @brief Inserts the provided entry invariantly.
@param [in] e the entry returned from a call obtaining an entry.
@param [in] elem a handle to the struct the user intends to insert.
@return a pointer to the inserted element or NULL upon allocation failure.

This method can be used when the old value in the map does not need to
be preserved. See the regular insert method if the old value is of interest. */
[[nodiscard]] void *
CCC_realtime_ordered_map_insert_entry(CCC_Realtime_ordered_map_entry const *e,
                                      CCC_Realtime_ordered_map_node *elem);

/** @brief Write the contents of the compound literal lazy_key_value to a node.
@param [in] Realtime_ordered_map_entry_ptr a pointer to the obtained entry.
@param [in] lazy_key_value the compound literal to write to a new slot.
@return a reference to the newly inserted or overwritten user type. NULL is
returned if allocation failed or is not allowed when required. */
#define CCC_realtime_ordered_map_insert_entry_w(                               \
    Realtime_ordered_map_entry_ptr, lazy_key_value...)                         \
    CCC_private_realtime_ordered_map_insert_entry_w(                           \
        Realtime_ordered_map_entry_ptr, lazy_key_value)

/** @brief Remove the entry from the map if Occupied.
@param [in] e a pointer to the map entry.
@return an entry containing NULL or a reference to the old entry. If Occupied an
entry in the map existed and was removed. If Vacant, no prior entry existed to
be removed.

Note that if allocation is permitted the old element is freed and the entry
will contain a NULL reference. If allocation is prohibited the entry can be
unwrapped to obtain the old user struct stored in the map and the user may
free or use as needed. */
[[nodiscard]] CCC_Entry
CCC_realtime_ordered_map_remove_entry(CCC_Realtime_ordered_map_entry const *e);

/** @brief Remove the entry from the map if Occupied.
@param [in] Realtime_ordered_map_entry_ptr a pointer to the map entry.
@return a compound literal reference to an entry containing NULL or a reference
to the old entry. If Occupied an entry in the map existed and was removed. If
Vacant, no prior entry existed to be removed.

Note that if allocation is permitted the old element is freed and the entry
will contain a NULL reference. If allocation is prohibited the entry can be
unwrapped to obtain the old user struct stored in the map and the user may
free or use as needed. */
#define CCC_realtime_ordered_map_remove_entry_r(                               \
    Realtime_ordered_map_entry_ptr)                                            \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_realtime_ordered_map_remove_entry(                                 \
            (Realtime_ordered_map_entry_ptr))                                  \
            .private                                                           \
    }

/** @brief Unwraps the provided entry to obtain a view into the map element.
@param [in] e the entry from a query to the map via function or macro.
@return a view into the table entry if one is present, or NULL. */
[[nodiscard]] void *
CCC_realtime_ordered_map_unwrap(CCC_Realtime_ordered_map_entry const *e);

/** @brief Returns the Vacant or Occupied status of the entry.
@param [in] e the entry from a query to the map via function or macro.
@return true if the entry is occupied, false if not. Error if e is NULL. */
[[nodiscard]] CCC_Tribool
CCC_realtime_ordered_map_insert_error(CCC_Realtime_ordered_map_entry const *e);

/** @brief Provides the status of the entry should an insertion follow.
@param [in] e the entry from a query to the table via function or macro.
@return true if an entry obtained from an insertion attempt failed to insert
due to an allocation failure when allocation success was expected. Error if e is
NULL. */
[[nodiscard]] CCC_Tribool
CCC_realtime_ordered_map_occupied(CCC_Realtime_ordered_map_entry const *e);

/** @brief Obtain the entry status from a container entry.
@param [in] e a pointer to the entry.
@return the status stored in the entry after the required action on the
container completes. If e is NULL an entry input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned.

Note that this function can be useful for debugging or if more detailed
messages are needed for logging purposes. See CCC_Entry_status_msg() in
ccc/types.h for more information on detailed entry statuses. */
[[nodiscard]] CCC_Entry_status
CCC_realtime_ordered_map_entry_status(CCC_Realtime_ordered_map_entry const *e);

/**@}*/

/** @name Deallocation Interface
Deallocate the container. */
/**@{*/

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
CCC_Result CCC_realtime_ordered_map_clear(CCC_Realtime_ordered_map *rom,
                                          CCC_Type_destructor *destructor);

/**@}*/

/** @name Iterator Interface
Obtain and manage iterators over the container. */
/**@{*/

/** @brief Return an iterable range of values from [begin_key, end_key).
Amortized O(lg N).
@param [in] rom a pointer to the map.
@param [in] begin_key a pointer to the key intended as the start of the range.
@param [in] end_key a pointer to the key intended as the end of the range.
@return a range containing the first element NOT LESS than the begin_key and
the first element GREATER than end_key.

Note that due to the variety of values that can be returned in the range, using
the provided range iteration functions from types.h is recommended for example:

for (struct Val *i = range_begin(&range);
     i != range_end(&range);
     i = next(&omm, &i->elem))
{}

This avoids any possible errors in handling an end range element that is in the
map versus the end map sentinel. */
[[nodiscard]] CCC_Range
CCC_realtime_ordered_map_equal_range(CCC_Realtime_ordered_map const *rom,
                                     void const *begin_key,
                                     void const *end_key);

/** @brief Returns a compound literal reference to the desired range. Amortized
O(lg N).
@param [in] Realtime_ordered_map_ptr a pointer to the map.
@param [in] begin_and_end_key_ptrs two pointers, the first to the start of the
range the second to the end of the range.
@return a compound literal reference to the produced range associated with the
enclosing scope. This reference is always non-NULL. */
#define CCC_realtime_ordered_map_equal_range_r(Realtime_ordered_map_ptr,       \
                                               begin_and_end_key_ptrs...)      \
    &(CCC_Range)                                                               \
    {                                                                          \
        CCC_realtime_ordered_map_equal_range((Realtime_ordered_map_ptr),       \
                                             (begin_and_end_key_ptrs))         \
            .private                                                           \
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

for (struct Val *i = rrange_begin(&rrange);
     i != rrange_rend(&rrange);
     i = rnext(&omm, &i->elem))
{}

This avoids any possible errors in handling an rend rrange element that is in
the map versus the end map sentinel. */
[[nodiscard]] CCC_Reverse_range
CCC_realtime_ordered_map_equal_rrange(CCC_Realtime_ordered_map const *rom,
                                      void const *rbegin_key,
                                      void const *rend_key);

/** @brief Returns a compound literal reference to the desired rrange. Amortized
O(lg N).
@param [in] Realtime_ordered_map_ptr a pointer to the map.
@param [in] rbegin_and_rend_key_ptrs two pointers, the first to the start of the
rrange the second to the end of the rrange.
@return a compound literal reference to the produced rrange associated with the
enclosing scope. This reference is always non-NULL. */
#define CCC_realtime_ordered_map_equal_rrange_r(Realtime_ordered_map_ptr,      \
                                                rbegin_and_rend_key_ptrs...)   \
    &(CCC_Reverse_range)                                                       \
    {                                                                          \
        CCC_realtime_ordered_map_equal_rrange((Realtime_ordered_map_ptr),      \
                                              (rbegin_and_rend_key_ptrs))      \
            .private                                                           \
    }

/** @brief Return the start of an inorder traversal of the map. Amortized
O(lg N).
@param [in] rom a pointer to the map.
@return the oldest minimum element of the map. */
[[nodiscard]] void *
CCC_realtime_ordered_map_begin(CCC_Realtime_ordered_map const *rom);

/** @brief Return the start of a reverse inorder traversal of the map.
Amortized O(lg N).
@param [in] rom a pointer to the map.
@return the oldest maximum element of the map. */
[[nodiscard]] void *
CCC_realtime_ordered_map_rbegin(CCC_Realtime_ordered_map const *rom);

/** @brief Return the next element in an inorder traversal of the map. O(1).
@param [in] rom a pointer to the map.
@param [in] iter_handle a pointer to the intrusive map element of the
current iterator.
@return the next user type stored in the map in an inorder traversal. */
[[nodiscard]] void *
CCC_realtime_ordered_map_next(CCC_Realtime_ordered_map const *rom,
                              CCC_Realtime_ordered_map_node const *iter_handle);

/** @brief Return the rnext element in a reverse inorder traversal of the map.
O(1).
@param [in] rom a pointer to the map.
@param [in] iter_handle a pointer to the intrusive map element of the
current iterator.
@return the rnext user type stored in the map in a reverse inorder traversal. */
[[nodiscard]] void *CCC_realtime_ordered_map_rnext(
    CCC_Realtime_ordered_map const *rom,
    CCC_Realtime_ordered_map_node const *iter_handle);

/** @brief Return the end of an inorder traversal of the map. O(1).
@param [in] rom a pointer to the map.
@return the newest maximum element of the map. */
[[nodiscard]] void *
CCC_realtime_ordered_map_end(CCC_Realtime_ordered_map const *rom);

/** @brief Return the rend of a reverse inorder traversal of the map. O(1).
@param [in] rom a pointer to the map.
@return the newest minimum element of the map. */
[[nodiscard]] void *
CCC_realtime_ordered_map_rend(CCC_Realtime_ordered_map const *rom);

/**@}*/

/** @name State Interface
Obtain the container state. */
/**@{*/

/** @brief Returns the count of map occupied nodes.
@param [in] rom the map.
@return the size or an argument is set if rom is NULL. */
[[nodiscard]] CCC_Count
CCC_realtime_ordered_map_count(CCC_Realtime_ordered_map const *rom);

/** @brief Returns the size status of the map.
@param [in] rom the map.
@return true if empty else false. Error if rom is NULL. */
[[nodiscard]] CCC_Tribool
CCC_realtime_ordered_map_is_empty(CCC_Realtime_ordered_map const *rom);

/** @brief Validation of invariants for the map.
@param [in] rom the map to validate.
@return true if all invariants hold, false if corruption occurs. Error if rom is
NULL. */
[[nodiscard]] CCC_Tribool
CCC_realtime_ordered_map_validate(CCC_Realtime_ordered_map const *rom);

/**@}*/

/** Define this preprocessor directive if shorter names are helpful. Ensure
 no namespace clashes occur before shortening. */
#ifdef REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
typedef CCC_Realtime_ordered_map_node Realtime_ordered_map_node;
typedef CCC_Realtime_ordered_map Realtime_ordered_map;
typedef CCC_Realtime_ordered_map_entry Realtime_ordered_map_entry;
#    define realtime_ordered_map_initialize(args...)                           \
        CCC_realtime_ordered_map_initialize(args)
#    define realtime_ordered_map_and_modify_w(args...)                         \
        CCC_realtime_ordered_map_and_modify_w(args)
#    define realtime_ordered_map_or_insert_w(args...)                          \
        CCC_realtime_ordered_map_or_insert_w(args)
#    define realtime_ordered_map_insert_entry_w(args...)                       \
        CCC_realtime_ordered_map_insert_entry_w(args)
#    define realtime_ordered_map_try_insert_w(args...)                         \
        CCC_realtime_ordered_map_try_insert_w(args)
#    define realtime_ordered_map_insert_or_assign_w(args...)                   \
        CCC_realtime_ordered_map_insert_or_assign_w(args)
#    define realtime_ordered_map_swap_entry_r(args...)                         \
        CCC_realtime_ordered_map_swap_entry_r(args)
#    define realtime_ordered_map_remove_r(args...)                             \
        CCC_realtime_ordered_map_remove_r(args)
#    define realtime_ordered_map_remove_entry_r(args...)                       \
        CCC_realtime_ordered_map_remove_entry_r(args)
#    define realtime_ordered_map_entry_r(args...)                              \
        CCC_realtime_ordered_map_entry_r(args)
#    define realtime_ordered_map_and_modify_r(args...)                         \
        CCC_realtime_ordered_map_and_modify_r(args)
#    define realtime_ordered_map_and_modify_context_r(args...)                 \
        CCC_realtime_ordered_map_and_modify_context_r(args)
#    define realtime_ordered_map_contains(args...)                             \
        CCC_realtime_ordered_map_contains(args)
#    define realtime_ordered_map_get_key_val(args...)                          \
        CCC_realtime_ordered_map_get_key_val(args)
#    define realtime_ordered_map_get_mut(args...)                              \
        CCC_realtime_ordered_map_get_mut(args)
#    define realtime_ordered_map_swap_entry(args...)                           \
        CCC_realtime_ordered_map_swap_entry(args)
#    define realtime_ordered_map_remove(args...)                               \
        CCC_realtime_ordered_map_remove(args)
#    define realtime_ordered_map_entry(args...)                                \
        CCC_realtime_ordered_map_entry(args)
#    define realtime_ordered_map_remove_entry(args...)                         \
        CCC_realtime_ordered_map_remove_entry(args)
#    define realtime_ordered_map_or_insert(args...)                            \
        CCC_realtime_ordered_map_or_insert(args)
#    define realtime_ordered_map_insert_entry(args...)                         \
        CCC_realtime_ordered_map_insert_entry(args)
#    define realtime_ordered_map_unwrap(args...)                               \
        CCC_realtime_ordered_map_unwrap(args)
#    define realtime_ordered_map_unwrap_mut(args...)                           \
        CCC_realtime_ordered_map_unwrap_mut(args)
#    define realtime_ordered_map_begin(args...)                                \
        CCC_realtime_ordered_map_begin(args)
#    define realtime_ordered_map_next(args...)                                 \
        CCC_realtime_ordered_map_next(args)
#    define realtime_ordered_map_rbegin(args...)                               \
        CCC_realtime_ordered_map_rbegin(args)
#    define realtime_ordered_map_rnext(args...)                                \
        CCC_realtime_ordered_map_rnext(args)
#    define realtime_ordered_map_end(args...) CCC_realtime_ordered_map_end(args)
#    define realtime_ordered_map_rend(args...)                                 \
        CCC_realtime_ordered_map_rend(args)
#    define realtime_ordered_map_count(args...)                                \
        CCC_realtime_ordered_map_count(args)
#    define realtime_ordered_map_is_empty(args...)                             \
        CCC_realtime_ordered_map_is_empty(args)
#    define realtime_ordered_map_clear(args...)                                \
        CCC_realtime_ordered_map_clear(args)
#    define realtime_ordered_map_validate(args...)                             \
        CCC_realtime_ordered_map_validate(args)
#endif

#endif /* CCC_REALTIME_ORDERED_MAP_H */
