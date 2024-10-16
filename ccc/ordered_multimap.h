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
bool ccc_omm_contains(ccc_ordered_multimap *mm, void const *key);

/** @brief Returns a reference to the user type stored at key.
@param [in] mm a pointer to the multimap.
@param [in] key a pointer to the key to be searched.
@return a reference to the oldest existing user type at key, NULL if absent. */
void *ccc_omm_get_key_val(ccc_ordered_multimap *mm, void const *key);

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
ccc_entry ccc_omm_insert(ccc_ordered_multimap *mm,
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
ccc_entry ccc_omm_try_insert(ccc_ordered_multimap *mm,
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
ccc_entry ccc_omm_insert_or_assign(ccc_ordered_multimap *mm,
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
ccc_entry ccc_omm_remove(ccc_ordered_multimap *mm, ccc_omm_elem *out_handle);

/* Standard Entry API. */

/** @brief Return a compound literal reference to the entry generated from a
search. No manual management of a compound literal reference is necessary.
@param [in] ordered_multimap_ptr a pointer to the multimap.
@param [in] key_ptr a ponter to the key to be searched.
@return a compound literal reference to a container specific entry associated
with the enclosing scope.

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
ccc_o_mm_entry ccc_omm_entry(ccc_ordered_multimap *mm, void const *key);

/** @brief Return a reference to the provided entry modified with fn if
Occupied.
@param [in] e a pointer to the container specific entry.
@param [in] fn the update function to modify the type in the entry.
@return a reference to the same entry provided. The update function will be
called on the entry with NULL as the auxiliary argument if the entry is
Occupied, otherwise the function is not called. If either arguments to the
function are NULL, NULL is returned. */
ccc_o_mm_entry *ccc_omm_and_modify(ccc_o_mm_entry *e, ccc_update_fn *fn);

/** @brief Return a reference to the provided entry modified with fn and
auxiliary data aux if Occupied.
@param [in] e a pointer to the container specific entry.
@param [in] fn the update function to modify the type in the entry.
@param [in] aux a pointer to auxiliary data needed for the modification.
@return a reference to the same entry provided. The update function will be
called on the entry with aux as the auxiliary argument if the entry is
Occupied, otherwise the function is not called. If any arguments to the
function are NULL, NULL is returned. */
ccc_o_mm_entry *ccc_omm_and_modify_aux(ccc_o_mm_entry *e, ccc_update_fn *fn,
                                       void *aux);

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
void *ccc_omm_or_insert(ccc_o_mm_entry const *e, ccc_omm_elem *key_val_handle);

void *ccc_omm_insert_entry(ccc_o_mm_entry const *e,
                           ccc_omm_elem *key_val_handle);

ccc_entry ccc_omm_remove_entry(ccc_o_mm_entry *e);

void *ccc_omm_unwrap(ccc_o_mm_entry const *e);
bool ccc_omm_insert_error(ccc_o_mm_entry const *e);
bool ccc_omm_occupied(ccc_o_mm_entry const *e);

/*===================    Priority Queue Helpers    ==========================*/

ccc_result ccc_omm_pop_max(ccc_ordered_multimap *mm);

ccc_result ccc_omm_pop_min(ccc_ordered_multimap *mm);

void *ccc_omm_max(ccc_ordered_multimap *mm);

void *ccc_omm_min(ccc_ordered_multimap *mm);

bool ccc_omm_is_max(ccc_ordered_multimap *mm, ccc_omm_elem const *);

bool ccc_omm_is_min(ccc_ordered_multimap *mm, ccc_omm_elem const *);

void *ccc_omm_erase(ccc_ordered_multimap *mm, ccc_omm_elem *);

bool ccc_omm_update(ccc_ordered_multimap *mm, ccc_omm_elem *, ccc_update_fn *,
                    void *);

bool ccc_omm_increase(ccc_ordered_multimap *mm, ccc_omm_elem *, ccc_update_fn *,
                      void *);

bool ccc_omm_decrease(ccc_ordered_multimap *mm, ccc_omm_elem *, ccc_update_fn *,
                      void *);

/*===========================   Iterators   =================================*/

#define ccc_omm_equal_range_r(ordered_multimap_ptr, begin_and_end_key_ptrs...) \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_omm_equal_range(ordered_multimap_ptr, begin_and_end_key_ptrs)      \
            .impl_                                                             \
    }

#define ccc_omm_equal_rrange_r(ordered_multimap_ptr,                           \
                               rbegin_and_rend_key_ptrs...)                    \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_omm_equal_rrange(ordered_multimap_ptr, rbegin_and_rend_key_ptrs)   \
            .impl_                                                             \
    }

ccc_range ccc_omm_equal_range(ccc_ordered_multimap *mm, void const *begin_key,
                              void const *end_key);

ccc_rrange ccc_omm_equal_rrange(ccc_ordered_multimap *mm,
                                void const *rbegin_key, void const *end_key);

void *ccc_omm_begin(ccc_ordered_multimap const *mm);

void *ccc_omm_rbegin(ccc_ordered_multimap const *mm);

void *ccc_omm_next(ccc_ordered_multimap const *mm, ccc_omm_elem const *);

void *ccc_omm_rnext(ccc_ordered_multimap const *mm, ccc_omm_elem const *);

void *ccc_omm_end(ccc_ordered_multimap const *mm);

void *ccc_omm_rend(ccc_ordered_multimap const *mm);

void ccc_omm_clear(ccc_ordered_multimap *mm, ccc_destructor_fn *destructor);

/*===========================     Getters   =================================*/

bool ccc_omm_is_empty(ccc_ordered_multimap const *mm);

size_t ccc_omm_size(ccc_ordered_multimap const *mm);

ccc_result ccc_omm_print(ccc_ordered_multimap const *mm, ccc_print_fn *);

bool ccc_omm_validate(ccc_ordered_multimap const *mm);

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
#    define omm_erase(args...) ccc_omm_erase(args)
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
#    define omm_print(args...) ccc_omm_print(args)
#    define omm_validate(args...) ccc_omm_validate(args)
#endif /* ORDERED_MULTIMAP_USING_NAMESPACE_CCC */

#endif /* CCC_ORDERED_MULTIMAP_H */
