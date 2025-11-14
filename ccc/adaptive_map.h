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
@brief The Adaptive Map Interface

An adaptive map offers storage and retrieval of elements sorted on the user
specified key. Because the data structure is self-optimizing it is not a
suitable map when strict sub-linear runtime bounds are needed. Also, searching
the map is not a const thread-safe operation as indicated by the function
signatures. The map is optimized upon every new search attempting to adapt to
the usage pattern. In many cases the self-optimizing structure of the map can be
beneficial when considering non-uniform access patterns. In the best case,
repeated searches of the same value yield an O(1) access and many other
frequently searched values will be obtained in constant time.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define ADAPTIVE_MAP_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `CCC_` prefix. */
#ifndef CCC_ADAPTIVE_MAP_H
#define CCC_ADAPTIVE_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "private/private_adaptive_map.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A self-optimizing data structure offering amortized O(lg N) search,
insert, and erase and pointer stability.
@warning it is undefined behavior to access an uninitialized container.

An adaptive map can be initialized on the stack, heap, or data segment at
runtime or compile time.*/
typedef struct CCC_Adaptive_map CCC_Adaptive_map;

/** @brief The intrusive element for the user defined struct being stored in the
map.

Note that if allocation is not permitted, insertions functions accepting this
type as an argument assume it to exist in pre-allocated memory that will exist
with the appropriate lifetime and scope for the user's needs; the container does
not allocate or free in this case. */
typedef struct CCC_Adaptive_map_node CCC_Adaptive_map_node;

/** @brief A container specific entry used to implement the Entry Interface.

The Entry Interface offers efficient search and subsequent insertion, deletion,
or value update based on the needs of the user. */
typedef union CCC_Adaptive_map_entry_wrap CCC_Adaptive_map_entry;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initializes the adaptive map at runtime or compile time.
@param[in] map_name the name of the adaptive map being initialized.
@param[in] struct_name the user type wrapping the intrusive element.
@param[in] type_intruder_field the name of the intrusive map element field.
@param[in] key_node_field the name of the field in user type used as key.
@param[in] key_order the key comparison function (see types.h).
@param[in] allocate the allocation function or NULL if allocation is banned.
@param[in] context a pointer to any context data for comparison or destruction.
@return the struct initialized adaptive map for direct assignment
(i.e. CCC_Adaptive_map m = CCC_adaptive_map_initialize(...);). */
#define CCC_adaptive_map_initialize(map_name, struct_name,                     \
                                    type_intruder_field, key_node_field,       \
                                    key_order, allocate, context)              \
    CCC_private_adaptive_map_initialize(map_name, struct_name,                 \
                                        type_intruder_field, key_node_field,   \
                                        key_order, allocate, context)

/**@}*/

/**@name Membership Interface
Test membership or obtain references to stored user types directly. */
/**@{*/

/** @brief Searches the map for the presence of key.
@param[in] map the map to be searched.
@param[in] key pointer to the key matching the key type of the user struct.
@return true if the struct containing key is stored, false if not. Error if map
or key is NULL. */
[[nodiscard]] CCC_Tribool CCC_adaptive_map_contains(CCC_Adaptive_map *map,
                                                    void const *key);

/** @brief Returns a reference into the map at entry key.
@param[in] map the adaptive map to search.
@param[in] key the key to search matching stored key type.
@return a view of the map entry if it is present, else NULL. */
[[nodiscard]] void *CCC_adaptive_map_get_key_value(CCC_Adaptive_map *map,
                                                   void const *key);

/**@}*/

/** @name Entry Interface
Obtain and operate on container entries for efficient queries when non-trivial
control flow is needed. */
/**@{*/

/** @brief Invariantly inserts the key value wrapping type_intruder.
@param[in] map the pointer to the adaptive map.
@param[in] type_intruder the handle to the user type wrapping map node.
@param[in] temp_intruder handle to space for swapping in the old value, if
present. The user must provide this additional type (e.g. `&(my_type){}`).
@return an entry. If Vacant, no prior element with key existed and the type
wrapping swap_intruder remains unchanged. If Occupied the old value is written
to the type wrapping swap_intruder and may be unwrapped to view. If more space
is needed but allocation fails or has been forbidden, an insert error is set.

Note that this function may write to the struct containing temp_intruder and
wraps it in an entry to provide information about the old value. */
[[nodiscard]] CCC_Entry
CCC_adaptive_map_swap_entry(CCC_Adaptive_map *map,
                            CCC_Adaptive_map_node *type_intruder,
                            CCC_Adaptive_map_node *temp_intruder);

/** @brief Invariantly inserts the key value wrapping the provided intruder.
@param[in] map_pointer the pointer to the adaptive map.
@param[in] type_intruder_pointer the handle to the user type wrapping this
handle.
@param[in] temp_intruder_pointer handle to space for swapping in the old value,
if present. The same user type stored in the map should wrap temp_intruder.
@return a compound literal reference to an entry. If Vacant, no prior element
with key existed and the type wrapping temp_intruder remains unchanged. If
Occupied the old value is written to the type wrapping temp_intruder and may be
unwrapped to view. If more space is needed but allocation fails or has been
forbidden, an insert error is set.

Note that this function may write to the struct containing temp_intruder and
wraps it in an entry to provide information about the old value. */
#define CCC_adaptive_map_swap_entry_wrap(map_pointer, type_intruder_pointer,   \
                                         temp_intruder_pointer)                \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_adaptive_map_swap_entry((map_pointer), (type_intruder_pointer),    \
                                    (temp_intruder_pointer))                   \
            .private                                                           \
    }

/** @brief Attempts to insert the key value wrapping type_intruder.
@param[in] map the pointer to the map.
@param[in] type_intruder the handle to the user type wrapping map node.
@return an entry. If Occupied, the entry contains a reference to the key value
user type in the map and may be unwrapped. If Vacant the entry contains a
reference to the newly inserted entry in the map. If more space is needed but
allocation fails, an insert error is set. */
[[nodiscard]] CCC_Entry
CCC_adaptive_map_try_insert(CCC_Adaptive_map *map,
                            CCC_Adaptive_map_node *type_intruder);

/** @brief Attempts to insert the key value wrapping type_intruder.
@param[in] map_pointer the pointer to the map.
@param[in] type_intruder_pointer the handle to the user type wrapping map
node.
@return a compound literal reference to an entry. If Occupied, the entry
contains a reference to the key value user type in the map and may be unwrapped.
If Vacant the entry contains a reference to the newly inserted entry in the map.
If more space is needed but allocation fails or has been forbidden, an insert
error is set. */
#define CCC_adaptive_map_try_insert_wrap(map_pointer, type_intruder_pointer)   \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_adaptive_map_try_insert((map_pointer), (type_intruder_pointer))    \
            .private                                                           \
    }

/** @brief lazily insert compound_literal_type into the map at key if key is
absent.
@param[in] map_pointer a pointer to the map.
@param[in] key the direct key r-value.
@param[in] compound_literal_type the compound literal specifying the value.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define CCC_adaptive_map_try_insert_with(map_pointer, key,                     \
                                         compound_literal_type...)             \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_private_adaptive_map_try_insert_with(map_pointer, key,             \
                                                 compound_literal_type)        \
    }

/** @brief Invariantly inserts or overwrites a user struct into the map.
@param[in] map a pointer to the flat hash map.
@param[in] type_intruder the handle to the wrapping user struct key value.
@return an entry. If Occupied an entry was overwritten by the new key value. If
Vacant no prior map entry existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
[[nodiscard]] CCC_Entry
CCC_adaptive_map_insert_or_assign(CCC_Adaptive_map *map,
                                  CCC_Adaptive_map_node *type_intruder);

/** @brief Inserts a new key value pair or overwrites the existing entry.
@param[in] map_pointer the pointer to the flat hash map.
@param[in] key the key to be searched in the map.
@param[in] compound_literal_type the compound literal to insert or use for
overwrite.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define CCC_adaptive_map_insert_or_assign_with(map_pointer, key,               \
                                               compound_literal_type...)       \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_private_adaptive_map_insert_or_assign_with(map_pointer, key,       \
                                                       compound_literal_type)  \
    }

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing type_output_intruder provided by the user.
@param[in] map the pointer to the adaptive map.
@param[out] type_output_intruder the handle to the user type wrapping map node.
@return the removed entry. If Occupied it may be unwrapped to obtain the old key
value pair. If Vacant the key value pair was not stored in the map. If bad input
is provided an input error is set.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value.

If allocation has been prohibited upon initialization then the entry returned
contains the previously stored user type, if any, and nothing is written to
the type_output_intruder. It is then the user's responsibility to manage their
previously stored memory as they see fit. */
[[nodiscard]] CCC_Entry
CCC_adaptive_map_remove(CCC_Adaptive_map *map,
                        CCC_Adaptive_map_node *type_output_intruder);

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing type_output_intruder provided by the user.
@param[in] map_pointer the pointer to the adaptive map.
@param[out] type_output_intruder_pointer the handle to the user type wrapping
map node.
@return a compound literal reference to the removed entry. If Occupied it may be
unwrapped to obtain the old key value pair. If Vacant the key value pair was not
stored in the map. If bad input is provided an input error is set.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value.

If allocation has been prohibited upon initialization then the entry returned
contains the previously stored user type, if any, and nothing is written to
the type_output_intruder. It is then the user's responsibility to manage their
previously stored memory as they see fit. */
#define CCC_adaptive_map_remove_wrap(map_pointer,                              \
                                     type_output_intruder_pointer)             \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_adaptive_map_remove((map_pointer), (type_output_intruder_pointer)) \
            .private                                                           \
    }

/** @brief Obtains an entry for the provided key in the map for future use.
@param[in] map the map to be searched.
@param[in] key the key used to search the map matching the stored key type.
@return a specialized entry for use with other functions in the Entry Interface.
@warning the contents of an entry should not be examined or modified. Use the
provided functions, only.

An entry is a search result that provides either an Occupied or Vacant entry
in the map. An occupied entry signifies that the search was successful. A
Vacant entry means the search was not successful but a handle is gained to
where in the map such an element should be inserted.

An entry is rarely useful on its own. It should be passed in a functional style
to subsequent calls in the Entry Interface. */
[[nodiscard]] CCC_Adaptive_map_entry
CCC_adaptive_map_entry(CCC_Adaptive_map *map, void const *key);

/** @brief Obtains an entry for the provided key in the map for future use.
@param[in] map_pointer the map to be searched.
@param[in] key_pointer the key used to search the map matching the stored key
type.
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
#define CCC_adaptive_map_entry_wrap(map_pointer, key_pointer)                  \
    &(CCC_Adaptive_map_entry)                                                  \
    {                                                                          \
        CCC_adaptive_map_entry((map_pointer), (key_pointer)).private           \
    }

/** @brief Modifies the provided entry if it is Occupied.
@param[in] entry the entry obtained from an entry function or macro.
@param[in] modify an update function in which the context argument is unused.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function is intended to make the function chaining in the Entry Interface
more succinct if the entry will be modified in place based on its own value
without the need of the context argument a CCC_Type_modifier can provide.
*/
[[nodiscard]] CCC_Adaptive_map_entry *
CCC_adaptive_map_and_modify(CCC_Adaptive_map_entry *entry,
                            CCC_Type_modifier *modify);

/** @brief Modifies the provided entry if it is Occupied.
@param[in] entry the entry obtained from an entry function or macro.
@param[in] fn an update function that requires context data.
@param[in] context context data required for the update.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function makes full use of a CCC_Type_modifier capability, meaning a
complete CCC_update object will be passed to the update function callback. */
[[nodiscard]] CCC_Adaptive_map_entry *
CCC_adaptive_map_and_modify_context(CCC_Adaptive_map_entry *entry,
                                    CCC_Type_modifier *fn, void *context);

/** @brief Modify an Occupied entry with a closure over user type T.
@param[in] adaptive_map_entry_pointer a pointer to the obtained entry.
@param[in] type_name the name of the user type stored in the container.
@param[in] closure_over_T the code to be run on the reference to user type T,
if Occupied. This may be a semicolon separated list of statements to execute on
T or a section of code wrapped in braces {code here} which may be preferred
for formatting.
@return a compound literal reference to the modified entry if it was occupied
or a vacant entry if it was vacant.
@note T is a reference to the user type stored in the entry guaranteed to be
non-NULL if the closure executes.

```
#define ADAPTIVE_MAP_USING_NAMESPACE_CCC
map_entry *entry = adaptive_map_and_modify_with(entry_wrap(&map, &k), word,
T->cnt++;); word *w =
adaptive_map_or_insert_with(adaptive_map_and_modify_with(entry_wrap(&map, &k),
word, { T->cnt++; }), (word){.key = k, .cnt = 1});
```

Note that any code written is only evaluated if the entry is Occupied and the
container can deliver the user type T. This means any function calls are lazily
evaluated in the closure scope. */
#define CCC_adaptive_map_and_modify_with(adaptive_map_entry_pointer,           \
                                         type_name, closure_over_T...)         \
    &(CCC_Adaptive_map_entry)                                                  \
    {                                                                          \
        CCC_private_adaptive_map_and_modify_with(adaptive_map_entry_pointer,   \
                                                 type_name, closure_over_T)    \
    }

/** @brief Inserts the struct with handle type_intruder if the entry is Vacant.
@param[in] entry the entry obtained via function or macro call.
@param[in] type_intruder the handle to the struct to be inserted to a Vacant
entry.
@return a pointer to entry in the map invariantly. NULL on error.

Because this functions takes an entry and inserts if it is Vacant, the only
reason NULL shall be returned is when an insertion error occurs, usually due to
a user struct allocation failure.

If no allocation is permitted, this function assumes the user struct wrapping
type_intruder has been allocated with the appropriate lifetime and scope by the
user. */
[[nodiscard]] void *
CCC_adaptive_map_or_insert(CCC_Adaptive_map_entry const *entry,
                           CCC_Adaptive_map_node *type_intruder);

/** @brief Lazily insert the desired key value into the entry if it is Vacant.
@param[in] adaptive_map_entry_pointer a pointer to the obtained entry.
@param[in] type_compound_literal the compound literal to construct in place if
the entry is Vacant.
@return a reference to the unwrapped user type in the entry, either the
unmodified reference if the entry was Occupied or the newly inserted element
if the entry was Vacant. NULL is returned if resizing is required but fails or
is not allowed.

Note that if the compound literal uses any function calls to generate values
or other data, such functions will not be called if the entry is Occupied. */
#define CCC_adaptive_map_or_insert_with(adaptive_map_entry_pointer,            \
                                        type_compound_literal...)              \
    CCC_private_adaptive_map_or_insert_with(adaptive_map_entry_pointer,        \
                                            type_compound_literal)

/** @brief Inserts the provided entry invariantly.
@param[in] entry the entry returned from a call obtaining an entry.
@param[in] type_intruder a handle to the struct the user intends to insert.
@return a pointer to the inserted element or NULL upon allocation failure.

This method can be used when the old value in the map does not need to
be preserved. See the regular insert method if the old value is of interest. */
[[nodiscard]] void *
CCC_adaptive_map_insert_entry(CCC_Adaptive_map_entry const *entry,
                              CCC_Adaptive_map_node *type_intruder);

/** @brief Write the contents of the compound literal type_compound_literal to a
node.
@param[in] adaptive_map_entry_pointer a pointer to the obtained entry.
@param[in] type_compound_literal the compound literal to write to a new slot.
@return a reference to the newly inserted or overwritten user type. NULL is
returned if allocation failed or is not allowed when required. */
#define CCC_adaptive_map_insert_entry_with(adaptive_map_entry_pointer,         \
                                           type_compound_literal...)           \
    CCC_private_adaptive_map_insert_entry_with(adaptive_map_entry_pointer,     \
                                               type_compound_literal)

/** @brief Remove the entry from the map if Occupied.
@param[in] entry a pointer to the map entry.
@return an entry containing NULL or a reference to the old entry. If Occupied an
entry in the map existed and was removed. If Vacant, no prior entry existed to
be removed.

Note that if allocation is permitted the old element is freed and the entry
will contain a NULL reference. If allocation is prohibited the entry can be
unwrapped to obtain the old user struct stored in the map and the user may
free or use as needed. */
[[nodiscard]] CCC_Entry
CCC_adaptive_map_remove_entry(CCC_Adaptive_map_entry *entry);

/** @brief Remove the entry from the map if Occupied.
@param[in] adaptive_map_entry_pointer a pointer to the map entry.
@return a compound literal reference to an entry containing NULL or a reference
to the old entry. If Occupied an entry in the map existed and was removed. If
Vacant, no prior entry existed to be removed.

Note that if allocation is permitted the old element is freed and the entry
will contain a NULL reference. If allocation is prohibited the entry can be
unwrapped to obtain the old user struct stored in the map and the user may
free or use as needed. */
#define CCC_adaptive_map_remove_entry_wrap(adaptive_map_entry_pointer)         \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_adaptive_map_remove_entry((adaptive_map_entry_pointer)).private    \
    }

/** @brief Unwraps the provided entry to obtain a view into the map element.
@param[in] entry the entry from a query to the map via function or macro.
@return a view into the table entry if one is present, or NULL. */
[[nodiscard]] void *
CCC_adaptive_map_unwrap(CCC_Adaptive_map_entry const *entry);

/** @brief Returns the Vacant or Occupied status of the entry.
@param[in] entry the entry from a query to the map via function or macro.
@return true if the entry is occupied, false if not. Error if entry is NULL. */
[[nodiscard]] CCC_Tribool
CCC_adaptive_map_occupied(CCC_Adaptive_map_entry const *entry);

/** @brief Provides the status of the entry should an insertion follow.
@param[in] entry the entry from a query to the table via function or macro.
@return true if an entry obtained from an insertion attempt failed to insert
due to an allocation failure when allocation success was expected. Error if
entry is NULL. */
[[nodiscard]] CCC_Tribool
CCC_adaptive_map_insert_error(CCC_Adaptive_map_entry const *entry);

/** @brief Obtain the entry status from a container entry.
@param[in] entry a pointer to the entry.
@return the status stored in the entry after the required action on the
container completes. If entry is NULL an entry input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned.

Note that this function can be useful for debugging or if more detailed
messages are needed for logging purposes. See CCC_entry_status_message() in
ccc/types.h for more information on detailed entry statuses. */
[[nodiscard]] CCC_Entry_status
CCC_adaptive_map_entry_status(CCC_Adaptive_map_entry const *entry);

/**@}*/

/** @name Iterator Interface
Obtain and manage iterators over the container. */
/**@{*/

/** @brief Return an iterable range of values from [begin_key, end_key).
Amortized O(lg N).
@param[in] map a pointer to the map.
@param[in] begin_key a pointer to the key intended as the start of the range.
@param[in] end_key a pointer to the key intended as the end of the range.
@return a range containing the first element NOT LESS than the begin_key and
the first element GREATER than end_key.

Note that due to the variety of values that can be returned in the range, using
the provided range iteration functions from types.h is recommended for example:

```
for (struct Val *i = range_begin(&range);
     i != range_end(&range);
     i = next(&map, &i->type_intruder))
{}
```

This avoids any possible errors in handling an end range element that is in the
map versus the end map sentinel. */
[[nodiscard]] CCC_Range CCC_adaptive_map_equal_range(CCC_Adaptive_map *map,
                                                     void const *begin_key,
                                                     void const *end_key);

/** @brief Returns a compound literal reference to the desired range. Amortized
O(lg N).
@param[in] map_pointer a pointer to the map.
@param[in] begin_and_end_key_pointers two pointers, one to the start of the
range and a second to the end of the range.
@return a compound literal reference to the produced range associated with the
enclosing scope. This reference is always non-NULL. */
#define CCC_adaptive_map_equal_range_wrap(map_pointer,                         \
                                          begin_and_end_key_pointers...)       \
    &(CCC_Range)                                                               \
    {                                                                          \
        CCC_adaptive_map_equal_range(map_pointer, begin_and_end_key_pointers)  \
            .private                                                           \
    }

/** @brief Return an iterable range_reverse of values from [begin_key, end_key).
Amortized O(lg N).
@param[in] map a pointer to the map.
@param[in] reverse_begin_key a pointer to the key intended as the start of the
range_reverse.
@param[in] reverse_end_key a pointer to the key intended as the end of the
range_reverse.
@return a range_reverse containing the first element NOT GREATER than the
begin_key and the first element LESS than reverse_end_key.

Note that due to the variety of values that can be returned in the
range_reverse, using the provided range_reverse iteration functions from types.h
is recommended for example:

```
for (struct Val *i = range_reverse_begin(&range_reverse);
     i != range_reverse_end(&range_reverse);
     i = reverse_next(&map, &i->type_intruder))
{}
```

This avoids any possible errors in handling an reverse_end range_reverse element
that is in the map versus the end map sentinel. */
[[nodiscard]] CCC_Range_reverse
CCC_adaptive_map_equal_range_reverse(CCC_Adaptive_map *map,
                                     void const *reverse_begin_key,
                                     void const *reverse_end_key);

/** @brief Returns a compound literal reference to the desired range_reverse.
Amortized O(lg N).
@param[in] map_pointer a pointer to the map.
@param[in] reverse_begin_and_reverse_end_key_pointers two pointers, one to the
start of the range_reverse and a second to the end of the range_reverse.
@return a compound literal reference to the produced range_reverse associated
with the enclosing scope. This reference is always non-NULL. */
#define CCC_adaptive_map_equal_range_reverse_wrap(                             \
    map_pointer, reverse_begin_and_reverse_end_key_pointers...)                \
    &(CCC_Range_reverse)                                                       \
    {                                                                          \
        CCC_adaptive_map_equal_range_reverse(                                  \
            map_pointer, reverse_begin_and_reverse_end_key_pointers)           \
            .private                                                           \
    }

/** @brief Return the start of an inorder traversal of the map. Amortized
O(lg N).
@param[in] map a pointer to the map.
@return the oldest minimum element of the map. */
[[nodiscard]] void *CCC_adaptive_map_begin(CCC_Adaptive_map const *map);

/** @brief Return the start of a reverse inorder traversal of the map.
Amortized O(lg N).
@param[in] map a pointer to the map.
@return the oldest maximum element of the map. */
[[nodiscard]] void *CCC_adaptive_map_reverse_begin(CCC_Adaptive_map const *map);

/** @brief Return the next element in an inorder traversal of the map. O(1).
@param[in] map a pointer to the map.
@param[in] iterator_intruder a pointer to the intrusive map element of the
current iterator.
@return the next user type stored in the map in an inorder traversal. */
[[nodiscard]] void *
CCC_adaptive_map_next(CCC_Adaptive_map const *map,
                      CCC_Adaptive_map_node const *iterator_intruder);

/** @brief Return the reverse_next element in a reverse inorder traversal of the
map. O(1).
@param[in] map a pointer to the map.
@param[in] iterator_intruder a pointer to the intrusive map element of the
current iterator.
@return the reverse_next user type stored in the map in a reverse inorder
traversal. */
[[nodiscard]] void *
CCC_adaptive_map_reverse_next(CCC_Adaptive_map const *map,
                              CCC_Adaptive_map_node const *iterator_intruder);

/** @brief Return the end of an inorder traversal of the map. O(1).
@param[in] map a pointer to the map.
@return the newest maximum element of the map. */
[[nodiscard]] void *CCC_adaptive_map_end(CCC_Adaptive_map const *map);

/** @brief Return the reverse_end of a reverse inorder traversal of the map.
O(1).
@param[in] map a pointer to the map.
@return the newest minimum element of the map. */
[[nodiscard]] void *CCC_adaptive_map_reverse_end(CCC_Adaptive_map const *map);

/**@}*/

/** @name Deallocation Interface
Destroy the container. */
/**@{*/

/** @brief Pops every element from the map calling destructor if destructor is
non-NULL. O(N).
@param[in] map a pointer to the map.
@param[in] destructor a destructor function if required. NULL if unneeded.
@return an input error if map points to NULL otherwise OK.

Note that if the map has been given permission to allocate, the destructor will
be called on each element before it uses the provided allocator to free the
element. Therefore, the destructor should not free the element or a double free
will occur.

If the container has not been given allocation permission, then the destructor
may free elements or not depending on how and when the user wishes to free
elements of the map according to their own memory management schemes. */
CCC_Result CCC_adaptive_map_clear(CCC_Adaptive_map *map,
                                  CCC_Type_destructor *destructor);

/**@}*/

/** @name State Interface
Obtain the container state. */
/**@{*/

/** @brief Returns the size status of the map.
@param[in] map the map.
@return true if empty else false. Error if map is NULL. */
[[nodiscard]] CCC_Tribool
CCC_adaptive_map_is_empty(CCC_Adaptive_map const *map);

/** @brief Returns the count of occupied map nodes.
@param[in] map the map.
@return the size or an argument error is set if map is NULL. */
[[nodiscard]] CCC_Count CCC_adaptive_map_count(CCC_Adaptive_map const *map);

/** @brief Validation of invariants for the map.
@param[in] map the map to validate.
@return true if all invariants hold, false if corruption occurs. Error if map is
NULL. */
[[nodiscard]] CCC_Tribool
CCC_adaptive_map_validate(CCC_Adaptive_map const *map);

/**@}*/

/** Define this preprocessor directive if shorter names are helpful. Ensure
 no namespace clashes occur before shortening. */
#ifdef ADAPTIVE_MAP_USING_NAMESPACE_CCC
typedef CCC_Adaptive_map_node Adaptive_map_node;
typedef CCC_Adaptive_map Adaptive_map;
typedef CCC_Adaptive_map_entry Adaptive_map_entry;
#    define adaptive_map_initialize(args...) CCC_adaptive_map_initialize(args)
#    define adaptive_map_and_modify_with(args...)                              \
        CCC_adaptive_map_and_modify_with(args)
#    define adaptive_map_or_insert_with(args...)                               \
        CCC_adaptive_map_or_insert_with(args)
#    define adaptive_map_insert_entry_with(args...)                            \
        CCC_adaptive_map_insert_entry_with(args)
#    define adaptive_map_try_insert_with(args...)                              \
        CCC_adaptive_map_try_insert_with(args)
#    define adaptive_map_insert_or_assign_with(args...)                        \
        CCC_adaptive_map_insert_or_assign_with(args)
#    define adaptive_map_swap_entry_wrap(args...)                              \
        CCC_adaptive_map_swap_entry_wrap(args)
#    define adaptive_map_remove_wrap(args...) CCC_adaptive_map_remove_wrap(args)
#    define adaptive_map_remove_entry_wrap(args...)                            \
        CCC_adaptive_map_remove_entry_wrap(args)
#    define adaptive_map_entry_wrap(args...) CCC_adaptive_map_entry_wrap(args)
#    define adaptive_map_and_modify_wrap(args...)                              \
        CCC_adaptive_map_and_modify_wrap(args)
#    define adaptive_map_and_modify_context_wrap(args...)                      \
        CCC_adaptive_map_and_modify_context_wrap(args)
#    define adaptive_map_contains(args...) CCC_adaptive_map_contains(args)
#    define adaptive_map_get_key_value(args...)                                \
        CCC_adaptive_map_get_key_value(args)
#    define adaptive_map_get_mut(args...) CCC_adaptive_map_get_mut(args)
#    define adaptive_map_swap_entry(args...) CCC_adaptive_map_swap_entry(args)
#    define adaptive_map_remove(args...) CCC_adaptive_map_remove(args)
#    define adaptive_map_entry(args...) CCC_adaptive_map_entry(args)
#    define adaptive_map_remove_entry(args...)                                 \
        CCC_adaptive_map_remove_entry(args)
#    define adaptive_map_or_insert(args...) CCC_adaptive_map_or_insert(args)
#    define adaptive_map_insert_entry(args...)                                 \
        CCC_adaptive_map_insert_entry(args)
#    define adaptive_map_unwrap(args...) CCC_adaptive_map_unwrap(args)
#    define adaptive_map_unwrap_mut(args...) CCC_adaptive_map_unwrap_mut(args)
#    define adaptive_map_begin(args...) CCC_adaptive_map_begin(args)
#    define adaptive_map_next(args...) CCC_adaptive_map_next(args)
#    define adaptive_map_reverse_begin(args...)                                \
        CCC_adaptive_map_reverse_begin(args)
#    define adaptive_map_reverse_next(args...)                                 \
        CCC_adaptive_map_reverse_next(args)
#    define adaptive_map_end(args...) CCC_adaptive_map_end(args)
#    define adaptive_map_reverse_end(args...) CCC_adaptive_map_reverse_end(args)
#    define adaptive_map_count(args...) CCC_adaptive_map_count(args)
#    define adaptive_map_is_empty(args...) CCC_adaptive_map_is_empty(args)
#    define adaptive_map_clear(args...) CCC_adaptive_map_clear(args)
#    define adaptive_map_validate(args...) CCC_adaptive_map_validate(args)
#endif

#endif /* CCC_ADAPTIVE_MAP_H */
