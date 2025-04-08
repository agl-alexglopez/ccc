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
@brief The Ordered Multimap Interface

A multimap offers storage and retrieval by key. However, duplicate keys are
allowed to be stored in the map.

This multimap orders duplicate keys by a round robin scheme. This means the
oldest key-value inserted into the multimap will be the one found on any
queries or removed first when popped from the multimap. Therefore, this
multimap is equivalent to a double ended priority queue with round robin
fairness among duplicate key elements. There are helper functions to make this
use case simpler. The multimap is a self-optimizing data structure and
therefore does not offer read-only searching. The runtime for all search,
insert, and remove operations is amortized O(lg N) and may not meet the
requirements of realtime systems.

This container offers pointer stability. Also, if the container is not
permitted to allocate all insertion code assumes that the user has allocated
memory appropriately for the element to be inserted; it will not allocate or
free in this case. If allocation is permitted upon initialization the container
will manage the memory as expected on insert or erase operations as defined
by the interface; memory is allocated for insertions and freed for removals.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define ORDERED_MULTIMAP_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_ORDERED_MULTIMAP_H
#define CCC_ORDERED_MULTIMAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "impl/impl_ordered_multimap.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A container for membership testing by key field, allowing duplicate
keys.
@warning it is undefined behavior to use an uninitialized container.

A ordered multimap may be stored on the stack, heap, or data segment. It can
be initialized at compile time or runtime.*/
typedef struct ccc_ommap_ ccc_ordered_multimap;

/** @brief The intrusive element for the user defined type stored in the
multimap.

The ordered multimap element can occupy a single field anywhere in the user
struct. Note that if allocation is not permitted, insertions functions accepting
this type as an argument assume it to exist in pre-allocated memory that will
exist with the appropriate lifetime and scope for the user's needs; the
container does not allocate or free in this case. */
typedef struct ccc_ommap_elem_ ccc_ommap_elem;

/** @brief The container specific type to support the Entry Interface.

An Entry Interface offers efficient conditional searching, saving multiple
searches. Entries are views of Vacant or Occupied multimap elements allowing
further operations to be performed once they are obtained without a second
search, insert, or remove query. */
typedef union ccc_ommap_entry_ ccc_ommap_entry;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a ordered multimap of the user specified type.
@param [in] omm_name the name of the ordered multimap being initialized.
@param [in] any_struct_name the struct the user intends to store.
@param [in] ommap_elem_field the name of the field with the intrusive element.
@param [in] key_field the name of the field used as the multimap key.
@param [in] key_cmp_fn the ccc_key_cmp_fn (types.h) used to compare the key to
the current stored element under considertion during a map operation.
@param [in] alloc_fn the ccc_alloc_fn (types.h) used to allocate nodes or NULL.
@param [in] aux any aux data needed for compare, print, or destruction.
@return the initialized ordered multimap. Use this initializer on the right
hand side of the variable at compile or run time
(e.g. ccc_ordered_multimap m = ccc_omm_init(...);) */
#define ccc_omm_init(omm_name, any_struct_name, ommap_elem_field, key_field,   \
                     key_cmp_fn, alloc_fn, aux)                                \
    ccc_impl_omm_init(omm_name, any_struct_name, ommap_elem_field, key_field,  \
                      key_cmp_fn, alloc_fn, aux)

/**@}*/

/**@name Membership Interface
Test membership or obtain references to stored user types directly. */
/**@{*/

/** @brief Returns the membership of key in the multimap. Amortized O(lg N).
@param [in] mm a pointer to the multimap.
@param [in] key a pointer to the key to be searched.
@return true if the multimap contains at least one entry at key, else false.
Error if mm or key is NULL. */
[[nodiscard]] ccc_tribool ccc_omm_contains(ccc_ordered_multimap *mm,
                                           void const *key);

/** @brief Returns a reference to the user type stored at key. Amortized O(lg
N).
@param [in] mm a pointer to the multimap.
@param [in] key a pointer to the key to be searched.
@return a reference to the oldest existing user type at key, NULL if absent. */
[[nodiscard]] void *ccc_omm_get_key_val(ccc_ordered_multimap *mm,
                                        void const *key);

/**@}*/

/** @name Entry Interface
Obtain and operate on container entries for efficient queries when non-trivial
control flow is needed. */
/**@{*/

/** @brief Returns an entry pointing to the newly inserted element and a status
indicating if the map has already been Occupied at the given key. Amortized
O(lg N).
@param [in] mm a pointer to the multimap.
@param [in] key_val_handle a handle to the new key value to be inserted.
@return an entry that can be unwrapped to view the inserted element. The status
will be Occupied if this element is a duplicate added to a duplicate list or
Vacant if this key is the first of its value inserted into the multimap. If the
element cannot be added due to an allocator error, an insert error is set.

Note that if allocation has been prohibited the address of the key_val_handle
is used directly. This means the container assumes the memory provided for the
user type containing key_val_handle has been allocated with appropriate lifetime
by the user, for the user's intended use case. */
[[nodiscard]] ccc_entry ccc_omm_swap_entry(ccc_ordered_multimap *mm,
                                           ccc_ommap_elem *key_val_handle);

/** @brief Inserts a new key-value into the multimap only if none exists.
Amortized O(lg N).
@param [in] mm a pointer to the multimap
@param [in] key_val_handle a pointer to the intrusive element in the user type.
@return an entry of the user type in the map. The status is Occupied if this
entry shows the oldest existing entry at key, or Vacant if no prior entry
existed and this is the first insertion at key.

Note that if allocation has been prohibited the address of the key_val_handle
is used directly. This means the container assumes the memory provided for the
user type containing key_val_handle has been allocated with appropriate lifetime
by the user, for the user's intended use case. */
[[nodiscard]] ccc_entry ccc_omm_try_insert(ccc_ordered_multimap *mm,
                                           ccc_ommap_elem *key_val_handle);

/** @brief Inserts a new key-value into the multimap only if none exists.
Amortized O(lg N).
@param [in] ordered_multimap_ptr a pointer to the multimap
@param [in] key the direct key r-value to be searched.
@param [in] lazy_value the compound literal for the type to be directly written
to a new allocation if an entry does not already exist at key.
@return a compound literal reference to the entry in the map. The status is
Occupied if this entry shows the oldest existing entry at key, or Vacant if
no prior entry existed and this is the first insertion at key.

Note that only the value, and any other supplementary fields, need be specified
in the struct compound literal as this method ensures the struct key and
searched key match. */
#define ccc_omm_try_insert_w(ordered_multimap_ptr, key, lazy_value...)         \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_omm_try_insert_w(ordered_multimap_ptr, key, lazy_value)       \
    }

/** @brief Invariantly inserts the key value pair into the multimap either as
the first entry or overwriting the oldest existing entry at key. Amortized
O(lg N).
@param [in] mm a pointer to the multimap
@param [in] key_val_handle a pointer to the intrusive element in the user type.
@return an entry of the user type in the map. The status is Occupied if this
entry shows the oldest existing entry at key with the newly written value, or
Vacant if no prior entry existed and this is the first insertion at key.

Note that if allocation has been prohibited the address of the key_val_handle
is used directly. This means the container assumes the memory provided for the
user type containing key_val_handle has been allocated with appropriate lifetime
by the user, for the user's intended use case. */
[[nodiscard]] ccc_entry
ccc_omm_insert_or_assign(ccc_ordered_multimap *mm,
                         ccc_ommap_elem *key_val_handle);

/** @brief Invariantly inserts the key value pair into the multimap either as
the first entry or overwriting the oldest existing entry at key. Amortized
O(lg N).
@param [in] ordered_multimap_ptr a pointer to the multimap
@param [in] key the direct key r-value to be searched.
@param [in] lazy_value the compound literal for the type to be directly written
to the existing or newly allocated map entry.
@return a compound literal reference to the entry in the map. The status is
Occupied if this entry shows the oldest existing entry at key with the newly
written value, or Vacant if no prior entry existed and this is the first
insertion at key.

Note that only the value, and any other supplementary fields, need to be
specified in the struct compound literal as this method ensures the struct key
and searched key match. */
#define ccc_omm_insert_or_assign_w(ordered_multimap_ptr, key, lazy_value...)   \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_omm_insert_or_assign_w(ordered_multimap_ptr, key, lazy_value) \
    }

/** @brief Removes the entry specified at key of the type containing out_handle
preserving the old value if possible. Amortized O(lg N).
@param [in] mm a pointer to the multimap.
@param [in] out_handle the pointer to the intrusive element in the user type.
@return an entry indicating if one of the elements stored at key has been
removed. The status is Occupied if at least one element at key existed and
was removed, or Vacant if no element existed at key. If the container has
been given permission to allocate, the oldest user type stored at key will
be written to the struct containing out_handle; the original data has been
freed. If allocation has been denied the container will return the user
struct directly and the user must unwrap and free their type themselves. */
[[nodiscard]] ccc_entry ccc_omm_remove(ccc_ordered_multimap *mm,
                                       ccc_ommap_elem *out_handle);

/** @brief Return a container specific entry for the given search for key.
Amortized O(lg N).
@param [in] mm a pointer to the multimap.
@param [in] key a pointer to the key to be searched.
@return a container specific entry for status, unwrapping, or further Entry
Interface operations. Occupied indicates at least one user type with key exists
and can be unwrapped to view. Vacant indicates no user type at key exists. */
[[nodiscard]] ccc_ommap_entry ccc_omm_entry(ccc_ordered_multimap *mm,
                                            void const *key);

/** @brief Return a compound literal reference to the entry generated from a
search. No manual management of a compound literal reference is necessary.
Amortized O(lg N).
@param [in] ordered_multimap_ptr a pointer to the multimap.
@param [in] key_ptr a ponter to the key to be searched.
@return a compound literal reference to a container specific entry associated
with the enclosing scope. This reference is always non-NULL.

Note this is useful for nested calls where an entry pointer is requested by
further operations in the Entry Interface, avoiding uneccessary intermediate
values and references (e.g. struct val *v = or_insert(entry_r(...), ...)); */
#define ccc_omm_entry_r(ordered_multimap_ptr, key_ptr)                         \
    &(ccc_ommap_entry)                                                         \
    {                                                                          \
        ccc_omm_entry((ordered_multimap_ptr), (key_ptr)).impl_                 \
    }

/** @brief Return a reference to the provided entry modified with fn if
Occupied.
@param [in] e a pointer to the container specific entry.
@param [in] fn the update function to modify the type in the entry.
@return a reference to the same entry provided. The update function will be
called on the entry with NULL as the auxiliary argument if the entry is
Occupied, otherwise the function is not called. If either arguments to the
function are NULL, NULL is returned. */
[[nodiscard]] ccc_ommap_entry *ccc_omm_and_modify(ccc_ommap_entry *e,
                                                  ccc_update_fn *fn);

/** @brief Return a reference to the provided entry modified with fn and
auxiliary data aux if Occupied.
@param [in] e a pointer to the container specific entry.
@param [in] fn the update function to modify the type in the entry.
@param [in] aux a pointer to auxiliary data needed for the modification.
@return a reference to the same entry provided. The update function will be
called on the entry with aux as the auxiliary argument if the entry is
Occupied, otherwise the function is not called. If any arguments to the
function are NULL, NULL is returned. */
[[nodiscard]] ccc_ommap_entry *
ccc_omm_and_modify_aux(ccc_ommap_entry *e, ccc_update_fn *fn, void *aux);

/** @brief Modify an Occupied entry with a closure over user type T.
@param [in] ordered_multimap_entry_ptr the address of the multimap entry.
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
#define ORDERED_MULTIMAP_USING_NAMESPACE_CCC
// Increment the key k if found otherwise do nothing.
omm_entry *e = omm_and_modify_w(entry_r(&omm, &k), word, T->cnt++;);

// Increment the key k if found otherwise insert a default value.
word *w = omm_or_insert_w(omm_and_modify_w(entry_r(&omm, &k), word,
                                           { T->cnt++; }),
                          (word){.key = k, .cnt = 1});
```


Note that any code written is only evaluated if the entry is Occupied and the
container can deliver the user type T. This means any function calls are lazily
evaluated in the closure scope. */
#define ccc_omm_and_modify_w(ordered_multimap_entry_ptr, type_name,            \
                             closure_over_T...)                                \
    &(ccc_ommap_entry)                                                         \
    {                                                                          \
        ccc_impl_omm_and_modify_w(ordered_multimap_entry_ptr, type_name,       \
                                  closure_over_T)                              \
    }

/** @brief Insert an initial key value into the multimap if none is present,
otherwise return the oldest user type stored at the specified key. O(1).
@param [in] e a pointer to the multimap entry.
@param [in] key_val_handle a pointer to the intrusive element in the user type.
@return a pointer to the user type stored in the map either existing or newly
inserted. If NULL is returned, an allocator error has occured when allocation
was allowed for the container.

Note that if allocation has been prohibited the address of the key_val_handle
is used directly. This means the container assumes the memory provided for the
user type containing key_val_handle has been allocated with appropriate lifetime
by the user, for the user's intended use case. */
[[nodiscard]] void *ccc_omm_or_insert(ccc_ommap_entry const *e,
                                      ccc_ommap_elem *key_val_handle);

/** @brief Insert an initial key value into the multimap if none is present,
otherwise return the oldest user type stored at the specified key. O(1).
@param [in] ordered_multimap_entry_ptr the address of the multimap entry.
@param [in] lazy_key_value the compound literal of the user struct stored in
the map.
@return a pointer to the user type stored in the map either existing or newly
inserted. If NULL is returned, an allocator error has occured or allocation
was disallowed on initialization to prevent inserting a new element.
@warning the key in the lazy_key_value compound literal must match the key
used for the initial entry generation.

Note that it only makes sense to use this method when the container is
permitted to allocate memory. */
#define ccc_omm_or_insert_w(ordered_multimap_entry_ptr, lazy_key_value...)     \
    ccc_impl_omm_or_insert_w(ordered_multimap_entry_ptr, lazy_key_value)

/** @brief Invariantly writes the specified key value directly to the existing
or newly allocated entry. O(1).
@param [in] e a pointer to the multimap entry.
@param [in] key_val_handle a pointer to the intrusive element in the user type.
@return a pointer to the user type written to the existing map entry or newly
inserted. NULL is returned if allocation is permitted but the allocator
encounters an error.

Note that if allocation has been prohibited the address of the key_val_handle
is used directly. This means the container assumes the memory provided for the
user type containing key_val_handle has been allocated with appropriate lifetime
by the user, for the user's intended use case. */
[[nodiscard]] void *ccc_omm_insert_entry(ccc_ommap_entry const *e,
                                         ccc_ommap_elem *key_val_handle);

/** @brief Invariantly writes the specified compound literal directly to the
existing or newly allocated entry. O(1).
@param [in] ordered_multimap_entry_ptr the address of the multimap entry.
@param [in] lazy_key_value the compound literal that is constructed directly
at the existing or newly allocated memory in the container.
@return a pointer to the user type written to the existing map entry or newly
inserted. If NULL is returned, an allocator error has occured or allocation
was disallowed on initialization to prevent inserting a new element
@warning the key in the lazy_key_value compound literal must match the key
used for the initial entry generation.

Note that it only makes sense to use this method when the container is
permitted to allocate memory. */
#define ccc_omm_insert_entry_w(ordered_multimap_entry_ptr, lazy_key_value...)  \
    ccc_impl_omm_insert_entry_w(ordered_multimap_entry_ptr, lazy_key_value)

/** @brief Removes the entry if it is Occupied. O(1).
@param [in] e a pointer to the multimap entry.
@return an entry indicating the status of the removal. If the entry was Vacant,
a Vacant entry with NULL is returned. If the entry is Occupied and allocation
is permitted, the stored user type is freed, the entry points to NULL, and the
status indicates the entry was Occupied but contains NULL. If allocation is
prohibited the entry is removed from the map and returned to be unwrapped and
freed by the user. */
[[nodiscard]] ccc_entry ccc_omm_remove_entry(ccc_ommap_entry *e);

/** @brief Indicates if an entry is Occupied or Vacant.
@param [in] e a pointer to the multimap entry.
@return true if the entry is Occupied, false if it is Vacant. Error if e is
NULL. */
[[nodiscard]] ccc_tribool ccc_omm_occupied(ccc_ommap_entry const *e);

/** @brief Unwraps the provided entry. An Occupied entry will point to the user
type stored in the map, a Vacant entry will be NULL.
@param [in] e a pointer to the multimap entry.
@return a pointer to the user type if Occupied, otherwise NULL. */
[[nodiscard]] void *ccc_omm_unwrap(ccc_ommap_entry const *e);

/** @brief Indicates if an insertion error occurs.
@param [in] e a pointer to the multimap entry.
@return true if an insertion error occured preventing completing of an Entry
Interface series of operations. Error if e is NULL.

Note that this will most commonly occur if the container is permitted to
allocate but the allocation has failed. */
[[nodiscard]] ccc_tribool ccc_omm_insert_error(ccc_ommap_entry const *e);

/** @brief Indicates if a function used to generate the provided entry
encountered bad arguments that prevented the operation of the function.
@param [in] e a pointer to the multimap entry.
@return true if bad function arguments were provided, otherwise false. Error if
e is NULL.

Note bad arguments usually mean NULL pointers were passed to functions expecting
non-NULL arguments. */
[[nodiscard]] ccc_tribool ccc_omm_input_error(ccc_ommap_entry const *e);

/** @brief Obtain the entry status from a container entry.
@param [in] e a pointer to the entry.
@return the status stored in the entry after the required action on the
container completes. If e is NULL an entry input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned.

Note that this function can be useful for debugging or if more detailed
messages are needed for logging purposes. See ccc_entry_status_msg() in
ccc/types.h for more information on detailed entry statuses. */
[[nodiscard]] ccc_entry_status ccc_omm_entry_status(ccc_ommap_entry const *e);

/**@}*/

/**@name Push and Pop Interface
An interface to enhance the priority queue capabilities of a multimap. */
/**@{*/

/** @brief Pops the oldest maximum key value user type from the map. Elements
are stored in ascending order, smallest as defined by the comparison function is
min and largest is max. Amortized O(lg N).
@param [in] mm the pointer to the multimap.
@return the status of the pop operation. If NULL pointer is provided or the
map is empty a bad input result is returned otherwise ok.

Note that continual pop max operations emptying a full queue with few to no
intervening search or insert operations is a good use case for this container
due to its self optimization. */
ccc_result ccc_omm_pop_max(ccc_ordered_multimap *mm);

/** @brief Pops the oldest minimum element from the map. Elements are stored
in ascending order, smallest as defined by the comparison function is min and
largest is max. Amortized O(lg N).
@param [in] mm the pointer to the multimap.
@return the status of the pop operation. If NULL pointer is provided or the
map is empty a bad input result is returned otherwise ok.

Note that continual pop min operations emptying a full queue with few to no
intervening search or insert operations is a good use case for this container
due to its self optimization. */
ccc_result ccc_omm_pop_min(ccc_ordered_multimap *mm);

/** @brief Returns a reference to the oldest maximum key value user type from
the map. Elements are stored in ascending order, smallest as defined by the
comparison function is min and largest is max. Amortized O(lg N).
@param [in] mm the pointer to the multimap.
@return the oldest maximum key value user type in the map.

Note that because the map is self optimizing, a search for the maximum element
followed by a pop of the maximum element results in one amortized O(lg N) search
followed by one O(1) pop. If there are duplicate max keys stored in the map, all
subsequent max search and pop operations are O(1) until duplicates are exhausted
and if no intervening search, insert, or erase operations occur for non-max
keys. */
[[nodiscard]] void *ccc_omm_max(ccc_ordered_multimap *mm);

/** @brief Returns a reference to the oldest minimum key value user type from
the map. Elements are stored in ascending order, smallest as defined by the
comparison function is min and largest is max. Amortized O(lg N).
@param [in] mm the pointer to the multimap.
@return the oldest minimum key value user type in the map.

Note that because the map is self optimizing, a search for the minimum element
followed by a pop of the minimum element results in one amortized O(lg N) search
followed by one O(1) pop. If there are duplicate min keys stored in the map, all
subsequent min search and pop operations are O(1) until duplicates are exhausted
and if no intervening search, insert, or erase operations occur for non-min
keys. */
[[nodiscard]] void *ccc_omm_min(ccc_ordered_multimap *mm);

/** @brief Extracts a user type known to be stored in the map with
key_val_handle as an element currently in use by the map. O(1).
@param [in] mm the pointer to the multimap.
@param [in] key_val_handle a pointer to the intrusive element embedded in a
user type that the user knows is currently in the map.
@return a reference to the extracted element. NULL is returned if it is possible
to prove the key_val_handle is not tracked by the map or the map is empty.

Note that the element that is extracted is not freed, even if allocation
permission is given to the container. It is the user's responsibility to free
the element that has been extracted. */
[[nodiscard]] void *ccc_omm_extract(ccc_ordered_multimap *mm,
                                    ccc_ommap_elem *key_val_handle);

/** @brief Updates an element key that is currently tracked directly as a
member of the map. Amortized O(lg N).
@param [in] mm the pointer to the multimap.
@param [in] key_val_handle a pointer to the intrusive element embedded in a
user type that the user knows is currently in the map.
@param [in] fn the function used to update an element key in the map.
@param [in] aux any auxiliary data needed for the update. Usually a new value
but NULL is possible if aux is not needed.
@return true if the key update was successful. Error is returned if bad
arguments are provided or it can be deduced that key_val_handle is not a member
of the container. */
[[nodiscard]] ccc_tribool ccc_omm_update(ccc_ordered_multimap *mm,
                                         ccc_ommap_elem *key_val_handle,
                                         ccc_update_fn *fn, void *aux);

/** @brief Increases an element key that is currently tracked directly as a
member of the map. Amortized O(lg N).
@param [in] mm the pointer to the multimap.
@param [in] key_val_handle a pointer to the intrusive element embedded in a
user type that the user knows is currently in the map.
@param [in] fn the function used to increase an element key in the map.
@param [in] aux any auxiliary data needed for the key increase. Usually a new
value but NULL is possible if aux is not needed.
@return true if the key increase was successful. Error is returned if bad
arguments are provided or it can be deduced that key_val_handle is not a member
of the container. */
[[nodiscard]] ccc_tribool ccc_omm_increase(ccc_ordered_multimap *mm,
                                           ccc_ommap_elem *key_val_handle,
                                           ccc_update_fn *fn, void *aux);

/** @brief Decreases an element key that is currently tracked directly as a
member of the map. Amortized O(lg N).
@param [in] mm the pointer to the multimap.
@param [in] key_val_handle a pointer to the intrusive element embedded in a
user type that the user knows is currently in the map.
@param [in] fn the function used to decrease an element key in the map.
@param [in] aux any auxiliary data needed for the key decrease. Usually a new
value but NULL is possible if aux is not needed.
@return true if the key decrease was successful. Error is returned if bad
arguments are provided or it can be deduced that key_val_handle is not a member
of the container. */
[[nodiscard]] ccc_tribool ccc_omm_decrease(ccc_ordered_multimap *mm,
                                           ccc_ommap_elem *key_val_handle,
                                           ccc_update_fn *fn, void *aux);

/**@}*/

/** @name Deallocation Interface
Deallocate the container. */
/**@{*/

/** @brief Pops every element from the map calling destructor if destructor is
non-NULL. O(N).
@param [in] mm a pointer to the multimap.
@param [in] destructor a destructor function if required. NULL if unneeded.
@return an input error if mm points to NULL otherwise ok.

Note that if the multimap has been given permission to allocate, the destructor
will be called on each element before it uses the provided allocator to free
the element. Therefore, the destructor should not free the element or a double
free will occur.

If the container has not been given allocation permission, then the destructor
may free elements or not depending on how and when the user wishes to free
elements of the map according to their own memory management schemes. */
ccc_result ccc_omm_clear(ccc_ordered_multimap *mm,
                         ccc_destructor_fn *destructor);

/**@}*/

/** @name Iterator Interface
Obtain and manage iterators over the container. */
/**@{*/

/** @brief Return an iterable range of values from [begin_key, end_key).
Amortized O(lg N).
@param [in] mm a pointer to the multimap.
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
[[nodiscard]] ccc_range ccc_omm_equal_range(ccc_ordered_multimap *mm,
                                            void const *begin_key,
                                            void const *end_key);

/** @brief Returns a compound literal reference to the desired range. Amortized
O(lg N).
@param [in] ordered_multimap_ptr a pointer to the multimap.
@param [in] begin_and_end_key_ptrs two pointers, one to the beginning of the
range and one to the end of the range.
@return a compound literal reference to the produced range associated with the
enclosing scope. This reference is always non-NULL. */
#define ccc_omm_equal_range_r(ordered_multimap_ptr, begin_and_end_key_ptrs...) \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_omm_equal_range(ordered_multimap_ptr, begin_and_end_key_ptrs)      \
            .impl_                                                             \
    }

/** @brief Return an iterable rrange of values from [begin_key, end_key).
Amortized O(lg N).
@param [in] mm a pointer to the multimap.
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
[[nodiscard]] ccc_rrange ccc_omm_equal_rrange(ccc_ordered_multimap *mm,
                                              void const *rbegin_key,
                                              void const *rend_key);

/** @brief Returns a compound literal reference to the desired rrange. Amortized
O(lg N).
@param [in] ordered_multimap_ptr a pointer to the multimap.
@param [in] rbegin_and_rend_key_ptrs two pointers, one to the beginning of the
rrange and one to the end of the rrange.
@return a compound literal reference to the produced rrange associated with the
enclosing scope. This reference is always non-NULL. */
#define ccc_omm_equal_rrange_r(ordered_multimap_ptr,                           \
                               rbegin_and_rend_key_ptrs...)                    \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_omm_equal_rrange(ordered_multimap_ptr, rbegin_and_rend_key_ptrs)   \
            .impl_                                                             \
    }

/** @brief Return the start of an inorder traversal of the multimap. Amortized
O(lg N).
@param [in] mm a pointer to the multimap.
@return the oldest minimum element of the map.

Note that duplicate keys will be traversed starting at the oldest element in
round robin order and ending at the newest before progressing to the next
key of stored in the multimap. */
[[nodiscard]] void *ccc_omm_begin(ccc_ordered_multimap const *mm);

/** @brief Return the start of a reverse inorder traversal of the multimap.
Amortized O(lg N).
@param [in] mm a pointer to the multimap.
@return the oldest maximum element of the map.

Note that duplicate keys will be traversed starting at the oldest element in
round robin order and ending at the newest before progressing to the next
key of stored in the multimap. */
[[nodiscard]] void *ccc_omm_rbegin(ccc_ordered_multimap const *mm);

/** @brief Return the next element in an inorder traversal of the multimap.
O(1).
@param [in] mm a pointer to the multimap.
@param [in] iter_handle a pointer to the intrusive multimap element of the
current iterator.
@return the next user type stored in the map in an inorder traversal.

Note that duplicate keys will be traversed starting at the oldest element in
round robin order and ending at the newest before progressing to the next
key of stored in the multimap. */
[[nodiscard]] void *ccc_omm_next(ccc_ordered_multimap const *mm,
                                 ccc_ommap_elem const *iter_handle);

/** @brief Return the rnext element in a reverse inorder traversal of the
multimap. O(1).
@param [in] mm a pointer to the multimap.
@param [in] iter_handle a pointer to the intrusive multimap element of the
current iterator.
@return the rnext user type stored in the map in a reverse inorder traversal.

Note that duplicate keys will be traversed starting at the oldest element in
round robin order and ending at the newest before progressing to the rnext
key of stored in the multimap. */
[[nodiscard]] void *ccc_omm_rnext(ccc_ordered_multimap const *mm,
                                  ccc_ommap_elem const *iter_handle);

/** @brief Return the end of an inorder traversal of the multimap. O(1).
@param [in] mm a pointer to the multimap.
@return the newest maximum element of the map.

Note that duplicate keys will be traversed starting at the oldest element in
round robin order and ending at the newest before progressing to the next
key of stored in the multimap. */
[[nodiscard]] void *ccc_omm_end(ccc_ordered_multimap const *mm);

/** @brief Return the rend of a reverse inorder traversal of the multimap. O(1).
@param [in] mm a pointer to the multimap.
@return the newest minimum element of the map.

Note that duplicate keys will be traversed starting at the oldest element in
round robin order and ending at the newest before progressing to the next
key of stored in the multimap. */
[[nodiscard]] void *ccc_omm_rend(ccc_ordered_multimap const *mm);

/**@}*/

/** @name State Interface
Obtain the container state. */
/**@{*/

/** @brief Returns true if the multimap is empty. O(1).
@param [in] mm a pointer to the multimap.
@return true if empty, false if mm is not empty. Error if mm is NULL. */
[[nodiscard]] ccc_tribool ccc_omm_is_empty(ccc_ordered_multimap const *mm);

/** @brief Returns the size of the multimap. O(1).
@param [in] mm a pointer to the multimap.
@return the size of the container or an argument error is set if mm is NULL. */
[[nodiscard]] ccc_ucount ccc_omm_size(ccc_ordered_multimap const *mm);

/** @brief Returns true if the multimap is empty.
@param [in] mm a pointer to the multimap.
@return true if invariants of the data structure are preserved, else false.
Error if mm is NULL. */
[[nodiscard]] ccc_tribool ccc_omm_validate(ccc_ordered_multimap const *mm);

/**@}*/

/** Use this preprocessor directive if shorter names are desired use the ccc
namespace to drop the ccc prefix from all types and methods. */
#ifdef ORDERED_MULTIMAP_USING_NAMESPACE_CCC
typedef ccc_ommap_elem ommap_elem;
typedef ccc_ordered_multimap ordered_multimap;
typedef ccc_ommap_entry ommap_entry;
#    define omm_and_modify_w(args...) ccc_omm_and_modify_w(args)
#    define omm_or_insert_w(args...) ccc_omm_or_insert_w(args)
#    define omm_insert_entry_w(args...) ccc_omm_insert_entry_w(args)
#    define omm_try_insert_w(args...) ccc_omm_try_insert_w(args)
#    define omm_insert_or_assign_w(args...) ccc_omm_insert_or_assign_w(args)
#    define omm_init(args...) ccc_omm_init(args)
#    define omm_swap_entry(args...) ccc_omm_swap_entry(args)
#    define omm_try_insert(args...) ccc_omm_try_insert(args)
#    define omm_insert_or_assign(args...) ccc_omm_insert_or_assign(args)
#    define omm_remove(args...) ccc_omm_remove(args)
#    define omm_remove_entry(args...) ccc_omm_remove_entry(args)
#    define omm_entry_r(args...) ccc_omm_entry_r(args)
#    define omm_entry(args...) ccc_omm_entry(args)
#    define omm_and_modify(args...) ccc_omm_and_modify(args)
#    define omm_and_modify_aux(args...) ccc_omm_and_modify_aux(args)
#    define omm_or_insert(args...) ccc_omm_or_insert(args)
#    define omm_insert_entry(args...) ccc_omm_insert_entry(args)
#    define omm_unwrap(args...) ccc_omm_unwrap(args)
#    define omm_insert_error(args...) ccc_omm_insert_error(args)
#    define omm_occupied(args...) ccc_omm_occupied(args)
#    define omm_clear(args...) ccc_omm_clear(args)
#    define omm_is_empty(args...) ccc_omm_is_empty(args)
#    define omm_size(args...) ccc_omm_size(args)
#    define omm_pop_max(args...) ccc_omm_pop_max(args)
#    define omm_pop_min(args...) ccc_omm_pop_min(args)
#    define omm_max(args...) ccc_omm_max(args)
#    define omm_min(args...) ccc_omm_min(args)
#    define omm_is_max(args...) ccc_omm_is_max(args)
#    define omm_is_min(args...) ccc_omm_is_min(args)
#    define omm_extract(args...) ccc_omm_extract(args)
#    define omm_update(args...) ccc_omm_update(args)
#    define omm_increase(args...) ccc_omm_increase(args)
#    define omm_decrease(args...) ccc_omm_decrease(args)
#    define omm_contains(args...) ccc_omm_contains(args)
#    define omm_begin(args...) ccc_omm_begin(args)
#    define omm_rbegin(args...) ccc_omm_rbegin(args)
#    define omm_next(args...) ccc_omm_next(args)
#    define omm_rnext(args...) ccc_omm_rnext(args)
#    define omm_end(args...) ccc_omm_end(args)
#    define omm_rend(args...) ccc_omm_rend(args)
#    define omm_equal_range(args...) ccc_omm_equal_range(args)
#    define omm_equal_rrange(args...) ccc_omm_equal_rrange(args)
#    define omm_validate(args...) ccc_omm_validate(args)
#endif /* ORDERED_MULTIMAP_USING_NAMESPACE_CCC */

#endif /* CCC_ORDERED_MULTIMAP_H */
