#ifndef CCC_ORDERED_MULTIMAP_H
#define CCC_ORDERED_MULTIMAP_H

#include "impl_ordered_multimap.h"
#include "impl_tree.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

/** An ordered multimap allows for membership testing by key field, but
insertion allows for multiple keys of the same value to exist in the multimap.
This multimap orders duplicate keys by a round robin scheme. This means the
oldest key-value inserted into the multimap will be the one found on any
queries or removed first when popped from the multimap. Therefore, this
multimap is equivalent to a double ended priority queue with round robin
fairness among duplicate key elements. There are helper functions to make this
use case simpler. The multimap is a self-optimizing data structure and
therefore does not offer read-only searching. The runtime for all search,
insert, and remove operations is amortized O(lgN) and may not meet the
requirements of realtime systems. */
typedef union
{
    struct ccc_tree_ impl_;
} ccc_ordered_multimap;

/** The intrusive element that must occupy a field in the struct the user
intends to track in the set. The ordered multimap element can occupy a single
field anywhere in the user struct. */
typedef union
{
    struct ccc_node_ impl_;
} ccc_omm_elem;

/** The container specific type to support the Entry API. An Entry API offers
efficient conditional searching, saving multiple searches. Entries are views
of Vacant or Occupied multimap elements allowing further operations to be
performed once they are obtained without a second search, insert, or remove
query. */
typedef union
{
    struct ccc_tree_entry_ impl_;
} ccc_o_mm_entry;

/** @brief Initialize a ordered multimap of the user specified type.
@param [in] omm_name the name of the ordered multimap being initialized.
@param [in] user_struct_name the struct the user intends to store.
@param [in] omm_elem_field the name of the field with the intrusive element.
@param [in] key_field the name of the field used as the multimap key.
@param [in] alloc_fn the ccc_alloc_fn (types.h) used to allocate nodes or NULL.
@param [in] key_cmp_fn the ccc_key_cmp_fn (types.h) used to compare the key to
the current stored element under considertion during a map operation.
@param [in] aux any aux data needed for compare, print, or destruction.
@return the initialized ordered multimap. Use this initializer on the right
hand side of the variable at compile or run time
(e.g. ccc_ordered_multimap m = ccc_omm_init(...);) */
#define ccc_omm_init(omm_name, user_struct_name, omm_elem_field, key_field,    \
                     alloc_fn, key_cmp_fn, aux)                                \
    ccc_impl_omm_init(omm_name, user_struct_name, omm_elem_field, key_field,   \
                      alloc_fn, key_cmp_fn, aux)

/*=======================    Lazy Construction   ============================*/

/** @brief Modify the ordered multimap entry with a modification function
requiring auxiliary data. If auxiliary data is passed as a function call, it
will only execute if the entry is occupied.
@param [in] ordered_multimap_entry_ptr the address of the multimap entry.
@param [in] mod_fn the ccc_update_fn used to update a stored value.
@param [in] aux_data the rvalue that this operation will construct and pass to
the modification function if the entry is occupied.
@return a pointer to a new entry. This is a compound literal reference, not a
pointer that requires any manual memory management.

Note that keys should not be modified by the modify operation, only values or
other struct members. */
#define ccc_omm_and_modify_w(ordered_multimap_entry_ptr, mod_fn,               \
                             lazy_aux_data...)                                 \
    &(ccc_o_mm_entry)                                                          \
    {                                                                          \
        ccc_impl_omm_and_modify_w(ordered_multimap_entry_ptr, mod_fn,          \
                                  lazy_aux_data)                               \
    }

/** @brief Insert an initial key value into the multimap if none is present,
otherwise return the oldest user type stored at the specified key.
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

/** @brief Invariantly writes the specified compound literal directly to the
existing or newly allocated entry.
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

/** @brief Inserts a new key-value into the multimap only if none exists.
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
the first entry or overwriting the oldest existing entry at key.
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

/*=========================    Membership   =================================*/

/** @brief Returns the membership of key in the multimap.
@param [in] mm a pointer to the multimap.
@param [in] key a pointer to the key to be searched.
@return true if the multimap contains at least one entry at key, else false. */
[[nodiscard]] bool ccc_omm_contains(ccc_ordered_multimap *mm, void const *key);

/** @brief Returns a reference to the user type stored at key.
@param [in] mm a pointer to the multimap.
@param [in] key a pointer to the key to be searched.
@return a reference to the oldest existing user type at key, NULL if absent. */
[[nodiscard]] void *ccc_omm_get_key_val(ccc_ordered_multimap *mm,
                                        void const *key);

/*=========================    Entry API    =================================*/

/** @brief Returns an entry pointing to the newly inserted element and a status
indicating if the map has already been Occupied at the given key.
@param [in] mm a pointer to the multimap.
@param [in] e a handle to the new key value to be inserted.
@return an entry that can be unwrapped to view the inserted element. The status
will be Occupied if this element is a duplicate added to a duplicate list or
Vacant if this key is the first of its value inserted into the multimap. If the
element cannot be added due to an allocator error, an insert error is set.

Note that if allocation has been prohibited the address of the key_val_handle
is used directly. This means the container assumes the memory provided for the
user type containing key_val_handle has been allocated with appropriate lifetime
by the user, for the user's intended use case. */
[[nodiscard]] ccc_entry ccc_omm_insert(ccc_ordered_multimap *mm,
                                       ccc_omm_elem *key_val_handle);

/** @brief Inserts a new key-value into the multimap only if none exists.
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
                                           ccc_omm_elem *key_val_handle);

/** @brief Invariantly inserts the key value pair into the multimap either as
the first entry or overwriting the oldest existing entry at key.
@param [in] mm a pointer to the multimap
@param [in] key_val_handle a pointer to the intrusive element in the user type.
@return an entry of the user type in the map. The status is Occupied if this
entry shows the oldest existing entry at key with the newly written value, or
Vacant if no prior entry existed and this is the first insertion at key.

Note that if allocation has been prohibited the address of the key_val_handle
is used directly. This means the container assumes the memory provided for the
user type containing key_val_handle has been allocated with appropriate lifetime
by the user, for the user's intended use case. */
[[nodiscard]] ccc_entry ccc_omm_insert_or_assign(ccc_ordered_multimap *mm,
                                                 ccc_omm_elem *key_val_handle);

/** @brief Removes the entry specified at key of the type containing out_handle
preserving the old value if possible.
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
                                       ccc_omm_elem *out_handle);

/* Standard Entry API. */

/** @brief Return a compound literal reference to the entry generated from a
search. No manual management of a compound literal reference is necessary.
@param [in] ordered_multimap_ptr a pointer to the multimap.
@param [in] key_ptr a ponter to the key to be searched.
@return a compound literal reference to a container specific entry associated
with the enclosing scope. This reference is always non-NULL.

Note this is useful for nested calls where an entry pointer is requested by
further operations in the Entry API, avoiding uneccessary intermediate values
and references (e.g. struct val *v = or_insert(entry_r(...), ...)); */
#define ccc_omm_entry_r(ordered_multimap_ptr, key_ptr)                         \
    &(ccc_o_mm_entry)                                                          \
    {                                                                          \
        ccc_omm_entry((ordered_multimap_ptr), (key_ptr)).impl_                 \
    }

/** @brief Return a container specific entry for the given search for key.
@param [in] mm a pointer to the multimap.
@param [in] key a pointer to the key to be searched.
@return a container specific entry for status, unwrapping, or further Entry API
operations. Occupied indicates at least one user type with key exists and can
be unwrapped to view. Vacant indicates no user type at key exists. */
[[nodiscard]] ccc_o_mm_entry ccc_omm_entry(ccc_ordered_multimap *mm,
                                           void const *key);

/** @brief Return a reference to the provided entry modified with fn if
Occupied.
@param [in] e a pointer to the container specific entry.
@param [in] fn the update function to modify the type in the entry.
@return a reference to the same entry provided. The update function will be
called on the entry with NULL as the auxiliary argument if the entry is
Occupied, otherwise the function is not called. If either arguments to the
function are NULL, NULL is returned. */
[[nodiscard]] ccc_o_mm_entry *ccc_omm_and_modify(ccc_o_mm_entry *e,
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
[[nodiscard]] ccc_o_mm_entry *
ccc_omm_and_modify_aux(ccc_o_mm_entry *e, ccc_update_fn *fn, void *aux);

/** @brief Insert an initial key value into the multimap if none is present,
otherwise return the oldest user type stored at the specified key.
@param [in] e a pointer to the multimap entry.
@param [in] key_val_handle a pointer to the intrusive element in the user type.
@return a pointer to the user type stored in the map either existing or newly
inserted. If NULL is returned, an allocator error has occured when allocation
was allowed for the container.

Note that if allocation has been prohibited the address of the key_val_handle
is used directly. This means the container assumes the memory provided for the
user type containing key_val_handle has been allocated with appropriate lifetime
by the user, for the user's intended use case. */
[[nodiscard]] void *ccc_omm_or_insert(ccc_o_mm_entry const *e,
                                      ccc_omm_elem *key_val_handle);

/** @brief Invariantly writes the specified key value directly to the existing
or newly allocated entry.
@param [in] e a pointer to the multimap entry.
@param [in] key_val_handle a pointer to the intrusive element in the user type.
@return a pointer to the user type written to the existing map entry or newly
inserted. NULL is returned if allocation is permitted but the allocator
encounters an error.

Note that if allocation has been prohibited the address of the key_val_handle
is used directly. This means the container assumes the memory provided for the
user type containing key_val_handle has been allocated with appropriate lifetime
by the user, for the user's intended use case. */
[[nodiscard]] void *ccc_omm_insert_entry(ccc_o_mm_entry const *e,
                                         ccc_omm_elem *key_val_handle);

/** @brief Removes the entry if it is Occupied.
@param [in] e a pointer to the multimap entry.
@return an entry indicating the status of the removal. If the entry was Vacant,
a Vacant entry with NULL is returned. If the entry is Occupied and allocation
is permitted, the stored user type is freed, the entry points to NULL, and the
status indicates the entry was Occupied but contains NULL. If allocation is
prohibited the entry is removed from the map and returned to be unwrapped and
freed by the user. */
[[nodiscard]] ccc_entry ccc_omm_remove_entry(ccc_o_mm_entry *e);

/** @brief Indicates if an entry is Occupied or Vacant.
@param [in] e a pointer to the multimap entry.
@return true if the entry is Occupied, false if it is Vacant. */
[[nodiscard]] bool ccc_omm_occupied(ccc_o_mm_entry const *e);

/** @brief Unwraps the provided entry. An Occupied entry will point to the user
type stored in the map, a Vacant entry will be NULL.
@param [in] e a pointer to the multimap entry.
@return a pointer to the user type if Occupied, otherwise NULL. */
[[nodiscard]] void *ccc_omm_unwrap(ccc_o_mm_entry const *e);

/** @brief Indicates if an insertion error occurs.
@param [in] e a pointer to the multimap entry.
@return true if an insertion error occured preventing completing of an Entry
API series of operations.

Note that this will most commonly occur if the container is permitted to
allocate but the allocation has failed. */
[[nodiscard]] bool ccc_omm_insert_error(ccc_o_mm_entry const *e);

/** @brief Indicates if a function used to generate the provided entry
encountered bad arguments that prevented the operation of the function.
@param [in] e a pointer to the multimap entry.
@return true if bad function arguments were provided, otherwise false.

Note bad arguments usually mean NULL pointers were passed to functions expecting
non-NULL arguments. */
[[nodiscard]] bool ccc_omm_input_error(ccc_o_mm_entry const *e);

/*===================    Priority Queue Helpers    ==========================*/

/** @brief Pops the oldest maximum key value user type from the map. Elements
are stored in ascending order, smallest as defined by the comparison function is
min and largest is max.
@param [in] mm the pointer to the multimap.
@return the status of the pop operation. If NULL pointer is provided or the
map is empty a bad input result is returned otherwise ok.

Note that continual pop max operations emptying a full queue with few to no
intervening search or insert operations is a good use case for this container
due to its self optimization. */
ccc_result ccc_omm_pop_max(ccc_ordered_multimap *mm);

/** @brief Pops the oldest minimum element from the map. Elements are stored
in ascending order, smallest as defined by the comparison function is min and
largest is max.
@param [in] mm the pointer to the multimap.
@return the status of the pop operation. If NULL pointer is provided or the
map is empty a bad input result is returned otherwise ok.

Note that continual pop min operations emptying a full queue with few to no
intervening search or insert operations is a good use case for this container
due to its self optimization. */
ccc_result ccc_omm_pop_min(ccc_ordered_multimap *mm);

/** @brief Returns a reference to the oldest maximum key value user type from
the map. Elements are stored in ascending order, smallest as defined by the
comparison function is min and largest is max.
@param [in] mm the pointer to the multimap.
@return the oldest maximum key value user type in the map.

Note that because the map is self optimizing, a search for the maximum element
followed by a pop of the maximum element results in one amortized O(lgN) search
followed by one O(1) pop. If there are duplicate max keys stored in the map, all
subsequent max search and pop operations are O(1) until duplicates are exhausted
and if no intervening search, insert, or erase operations occur for non-max
keys. */
[[nodiscard]] void *ccc_omm_max(ccc_ordered_multimap *mm);

/** @brief Returns a reference to the oldest minimum key value user type from
the map. Elements are stored in ascending order, smallest as defined by the
comparison function is min and largest is max.
@param [in] mm the pointer to the multimap.
@return the oldest minimum key value user type in the map.

Note that because the map is self optimizing, a search for the minimum element
followed by a pop of the minimum element results in one amortized O(lgN) search
followed by one O(1) pop. If there are duplicate min keys stored in the map, all
subsequent min search and pop operations are O(1) until duplicates are exhausted
and if no intervening search, insert, or erase operations occur for non-min
keys. */
[[nodiscard]] void *ccc_omm_min(ccc_ordered_multimap *mm);

/** @brief Extracts a user type known to be stored in the map with
key_val_handle as an element currently in use by the map.
@param [in] mm the pointer to the multimap.
@param [in] key_val_handle a pointer to the intrusive element embedded in a
user type that the user knows is currently in the map.
@return a reference to the extracted element. NULL is returned if it is possible
to prove the key_val_handle is not tracked by the map or the map is empty.

Note that the element that is extracted is not freed, even if allocation
permission is given to the container. It is the user's responsibility to free
the element that has been extracted. */
[[nodiscard]] void *ccc_omm_extract(ccc_ordered_multimap *mm,
                                    ccc_omm_elem *key_val_handle);

/** @brief Updates an element key that is currently tracked directly as a
member of the map.
@param [in] mm the pointer to the multimap.
@param [in] key_val_handle a pointer to the intrusive element embedded in a
user type that the user knows is currently in the map.
@param [in] fn the function used to update an element key in the map.
@param [in] aux any auxiliary data needed for the update. Usually a new value
but NULL is possible if aux is not needed.
@return true if the key update was successful, false if bad arguments are
provided, it is possible to prove the key_val_handle is not tracked by the map,
or the map is empty. */
[[nodiscard]] bool ccc_omm_update(ccc_ordered_multimap *mm,
                                  ccc_omm_elem *key_val_handle,
                                  ccc_update_fn *fn, void *aux);

/** @brief Increases an element key that is currently tracked directly as a
member of the map.
@param [in] mm the pointer to the multimap.
@param [in] key_val_handle a pointer to the intrusive element embedded in a
user type that the user knows is currently in the map.
@param [in] fn the function used to increase an element key in the map.
@param [in] aux any auxiliary data needed for the key increase. Usually a new
value but NULL is possible if aux is not needed.
@return true if the key increase was successful, false if bad arguments are
provided, it is possible to prove the key_val_handle is not tracked by the map,
or the map is empty. */
[[nodiscard]] bool ccc_omm_increase(ccc_ordered_multimap *mm,
                                    ccc_omm_elem *key_val_handle,
                                    ccc_update_fn *fn, void *aux);

/** @brief Decreases an element key that is currently tracked directly as a
member of the map.
@param [in] mm the pointer to the multimap.
@param [in] key_val_handle a pointer to the intrusive element embedded in a
user type that the user knows is currently in the map.
@param [in] fn the function used to decrease an element key in the map.
@param [in] aux any auxiliary data needed for the key decrease. Usually a new
value but NULL is possible if aux is not needed.
@return true if the key decrease was successful, false if bad arguments are
provided, it is possible to prove the key_val_handle is not tracked by the map,
or the map is empty. */
[[nodiscard]] bool ccc_omm_decrease(ccc_ordered_multimap *mm,
                                    ccc_omm_elem *key_val_handle,
                                    ccc_update_fn *fn, void *aux);

/*===========================   Iterators   =================================*/

/** @brief Returns a compound literal reference to the desired range.
@param [in] ordered_multimap_ptr a pointer to the multimap.
@param [in] begin_key_ptr a pointer to the key that marks the start of the
range.
@param [in] end_key_ptr a pointer to the key that marks the end of the range.
@return a compound literal reference to the produced range associated with the
enclosing scope. This reference is always non-NULL. */
#define ccc_omm_equal_range_r(ordered_multimap_ptr, begin_and_end_key_ptrs...) \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_omm_equal_range(ordered_multimap_ptr, begin_and_end_key_ptrs)      \
            .impl_                                                             \
    }

/** @brief Returns a compound literal reference to the desired rrange.
@param [in] ordered_multimap_ptr a pointer to the multimap.
@param [in] begin_key_ptr a pointer to the key that marks the start of the
rrange.
@param [in] end_key_ptr a pointer to the key that marks the end of the rrange.
@return a compound literal reference to the produced rrange associated with the
enclosing scope. This reference is always non-NULL. */
#define ccc_omm_equal_rrange_r(ordered_multimap_ptr,                           \
                               rbegin_and_rend_key_ptrs...)                    \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_omm_equal_rrange(ordered_multimap_ptr, rbegin_and_rend_key_ptrs)   \
            .impl_                                                             \
    }

/** @brief Return an iterable range of values from [begin_key, end_key).
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

/** @brief Return an iterable rrange of values from [begin_key, end_key).
@param [in] mm a pointer to the multimap.
@param [in] begin_key a pointer to the key intended as the start of the rrange.
@param [in] end_key a pointer to the key intended as the end of the rrange.
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

/** @brief Return the start of an inorder traversal of the multimap.
@param [in] mm a pointer to the multimap.
@return the oldest minimum element of the map.

Note that duplicate keys will be traversed starting at the oldest element in
round robin order and ending at the newest before progressing to the next
key of stored in the multimap. */
[[nodiscard]] void *ccc_omm_begin(ccc_ordered_multimap const *mm);

/** @brief Return the start of a reverse inorder traversal of the multimap.
@param [in] mm a pointer to the multimap.
@return the oldest maximum element of the map.

Note that duplicate keys will be traversed starting at the oldest element in
round robin order and ending at the newest before progressing to the next
key of stored in the multimap. */
[[nodiscard]] void *ccc_omm_rbegin(ccc_ordered_multimap const *mm);

/** @brief Return the next element in an inorder traversal of the multimap.
@param [in] mm a pointer to the multimap.
@param [in] iter_handle a pointer to the intrusive multimap element of the
current iterator.
@return the next user type stored in the map in an inorder traversal.

Note that duplicate keys will be traversed starting at the oldest element in
round robin order and ending at the newest before progressing to the next
key of stored in the multimap. */
[[nodiscard]] void *ccc_omm_next(ccc_ordered_multimap const *mm,
                                 ccc_omm_elem const *iter_handle);

/** @brief Return the rnext element in a reverse inorder traversal of the
multimap.
@param [in] mm a pointer to the multimap.
@param [in] iter_handle a pointer to the intrusive multimap element of the
current iterator.
@return the rnext user type stored in the map in a reverse inorder traversal.

Note that duplicate keys will be traversed starting at the oldest element in
round robin order and ending at the newest before progressing to the rnext
key of stored in the multimap. */
[[nodiscard]] void *ccc_omm_rnext(ccc_ordered_multimap const *mm,
                                  ccc_omm_elem const *iter_handle);

/** @brief Return the end of an inorder traversal of the multimap.
@param [in] mm a pointer to the multimap.
@return the newest maximum element of the map.

Note that duplicate keys will be traversed starting at the oldest element in
round robin order and ending at the newest before progressing to the next
key of stored in the multimap. */
[[nodiscard]] void *ccc_omm_end(ccc_ordered_multimap const *mm);

/** @brief Return the rend of a reverse inorder traversal of the multimap.
@param [in] mm a pointer to the multimap.
@return the newest minimum element of the map.

Note that duplicate keys will be traversed starting at the oldest element in
round robin order and ending at the newest before progressing to the next
key of stored in the multimap. */
[[nodiscard]] void *ccc_omm_rend(ccc_ordered_multimap const *mm);

/** @brief Pops every element from the map calling destructor if destructor is
non-NULL.
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

/*===========================     Getters   =================================*/

/** @brief Returns true if the multimap is empty.
@param [in] mm a pointer to the multimap.
@return true if empty, false if mm is NULL or mm is empty. */
[[nodiscard]] bool ccc_omm_is_empty(ccc_ordered_multimap const *mm);

/** @brief Returns true if the multimap is empty.
@param [in] mm a pointer to the multimap.
@return the size of the container or 0 if empty or mm is NULL. */
[[nodiscard]] size_t ccc_omm_size(ccc_ordered_multimap const *mm);

/** @brief Returns true if the multimap is empty.
@param [in] mm a pointer to the multimap.
@return true if invariants of the data structure are preserved, else false. */
[[nodiscard]] bool ccc_omm_validate(ccc_ordered_multimap const *mm);

/** Use this preprocessor directive if shorter names are desired use the ccc
namespace to drop the ccc prefix from all types and methods. */
#ifdef ORDERED_MULTIMAP_USING_NAMESPACE_CCC
typedef ccc_omm_elem omm_elem;
typedef ccc_ordered_multimap ordered_multimap;
typedef ccc_o_mm_entry o_mm_entry;
#    define omm_and_modify_w(args...) ccc_omm_and_modify_w(args)
#    define omm_or_insert_w(args...) ccc_omm_or_insert_w(args)
#    define omm_insert_entry_w(args...) ccc_omm_insert_entry_w(args)
#    define omm_try_insert_w(args...) ccc_omm_try_insert_w(args)
#    define omm_insert_or_assign_w(args...) ccc_omm_insert_or_assign_w(args)
#    define omm_init(args...) ccc_omm_init(args)
#    define omm_insert(args...) ccc_omm_insert(args)
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
