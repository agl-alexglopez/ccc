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
@brief The Handle Bounded Map Interface

A bounded map offers insertion, removal, and searching with a strict bound of
`O(log(N))` time. The map is pointer stable. This map is suitable for realtime
applications if resizing can be well controlled. Insert operations may cause
resizing if allocation is allowed. Searching is a thread-safe read-only
operation. Balancing modifications only occur upon insertion or removal.

The handle variant of the bounded map promises contiguous storage and random
access if needed. Handles are stable and the user can use them to refer to an
element until that element is removed from the map. Handles remain valid even if
resizing of the table, insertions of other elements, or removals of other
elements occur. Active user elements may not be contiguous from index [0, N)
where N is the size of map; there may be gaps between active elements in the
Buffer and it is only guaranteed that N elements are stored between index [0,
Capacity).

All elements in the map track their relationships via indices in the buffer.
Therefore, this data structure can be relocated, copied, serialized, or written
to disk and all internal data structure references will remain valid. Insertion
may invoke an O(N) operation if resizing occurs. Finally, if allocation is
prohibited upon initialization and the user intends to store a fixed size N
nodes in the map N + 1 capacity is needed for the sentinel node in the buffer.

All interface functions accept `void *` references to either the key or the full
type the user is storing in the map. Therefore, it is important for the user to
be aware if they are passing a reference to the key or the full type depending
on the function requirements.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define HANDLE_BOUNDED_MAP_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `CCC_` prefix. */
#ifndef CCC_HANDLE_BOUNDED_MAP_H
#define CCC_HANDLE_BOUNDED_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "private/private_handle_bounded_map.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A handle bounded map offers O(lg N) search and erase, and
amortized O(lg N) insert.
@warning it is undefined behavior to access an uninitialized container.

A handle bounded map can be initialized on the stack, heap, or data
segment at runtime or compile time.*/
typedef struct CCC_Handle_bounded_map CCC_Handle_bounded_map;

/** @brief A container specific handle used to implement the Handle Interface.
@warning it is undefined behavior to access an uninitialized container.

The Handle Interface offers efficient search and subsequent insertion, deletion,
or value update based on the needs of the user. Handles obtained via the Handle
Interface are stable until the user removes the element at the provided handle.
Insertions and deletions of other elements do not affect handle stability.
Resizing of the table does not affect handle stability. */
typedef union CCC_Handle_bounded_map_handle_wrap CCC_Handle_bounded_map_handle;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Declare a fixed size map type for use in the stack, heap, or data
segment. Does not return a value.
@param[in] fixed_map_type_name the user chosen name of the fixed sized map.
@param[in] type_name the type the user plans to store in the map. It
may have a key and value field as well as any additional fields. For set-like
behavior, wrap a field in a struct/union (e.g. `union int_node {int e;};`).
@param[in] capacity the desired number of user accessible nodes.
@warning the map will use one slot of the specified capacity for a sentinel
node. This is not important to the user unless an exact allocation count is
needed in which case 1 should be added to desired capacity.

Once the location for the fixed size map is chosen--stack, heap, or data
segment--provide a pointer to the map for the initialization macro.

```
struct Val
{
    int key;
    int val;
};
CCC_handle_bounded_map_declare_fixed_map(small_fixed_map, struct Val,
64); static map static_map =
handle_bounded_map_initialize(
    &(static small_fixed_map){},
    struct Val,
    key,
    handle_bounded_map_key_order,
    NULL,
    NULL,
    handle_bounded_map_fixed_capacity(small_fixed_map)
);
```

Similarly, a fixed size map can be used on the stack.

```
struct Val
{
    int key;
    int val;
};
CCC_handle_bounded_map_declare_fixed_map(small_fixed_map, struct Val,
64); int main(void)
{
    map static_map =
handle_bounded_map_initialize(
        &(small_fixed_map){},
        struct Val,
        key,
        handle_bounded_map_key_order,
        NULL,
        NULL,
        handle_bounded_map_fixed_capacity(small_fixed_map)
    );
    return 0;
}
```

The CCC_handle_bounded_map_fixed_capacity macro can be used to obtain
the previously provided capacity when declaring the fixed map type. Finally, one
could allocate a fixed size map on the heap; however, it is usually better to
initialize a dynamic map and use the CCC_handle_bounded_map_reserve
function for such a use case.

This macro is not needed when a dynamic resizing map is needed. For dynamic
maps, simply pass NULL and 0 capacity to the initialization macro along with the
desired allocation function. */
#define CCC_handle_bounded_map_declare_fixed_map(fixed_map_type_name,          \
                                                 type_name, capacity)          \
    CCC_private_handle_bounded_map_declare_fixed_map(fixed_map_type_name,      \
                                                     type_name, capacity)

/** @brief Obtain the capacity previously chosen for the fixed size map type.
@param[in] fixed_map_type_name the name of a previously declared map.
@return the size_t capacity previously specified for this type by user. */
#define CCC_handle_bounded_map_fixed_capacity(fixed_map_type_name)             \
    CCC_private_handle_bounded_map_fixed_capacity(fixed_map_type_name)

/** @brief Initializes the map at runtime or compile time.
@param[in] memory_pointer a pointer to the contiguous user types or ((T
*)NULL).
@param[in] type_name the name of the user type stored in the map.
@param[in] type_intruder_field the name of the field in user type used as key.
@param[in] compare the key comparison function (see types.h).
@param[in] allocate the allocation function or NULL if allocation is banned.
@param[in] context_data a pointer to any context data for comparison or
destruction.
@param[in] capacity the capacity at data_pointer or 0.
@return the struct initialized bounded map for direct assignment
(i.e. CCC_Handle_bounded_map m =
CCC_handle_bounded_map_initialize(...);). */
#define CCC_handle_bounded_map_initialize(memory_pointer, type_name,           \
                                          type_intruder_field, compare,        \
                                          allocate, context_data, capacity)    \
    CCC_private_handle_bounded_map_initialize(                                 \
        memory_pointer, type_name, type_intruder_field, compare, allocate,     \
        context_data, capacity)

/** @brief Copy the map at source to destination.
@param[in] destination the initialized destination for the copy of the source
map.
@param[in] source the initialized source of the map.
@param[in] allocate the allocation function to resize destination or NULL.
@return the result of the copy operation. If the destination capacity is less
than the source capacity and no allocation function is provided an input error
is returned. If resizing is required and resizing of destination fails a memory
error is returned.
@note destination must have capacity greater than or equal to source. If
destination capacity is less than source, an allocation function must be
provided with the allocate argument.

Note that there are two ways to copy data from source to destination: provide
sufficient memory and pass NULL as allocate, or allow the copy function to take
care of allocation for the copy.

Manual memory management with no allocation function provided.

```
#define HANDLE_BOUNDED_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
CCC_handle_bounded_map_declare_fixed_map(small_fixed_map, struct Val,
64); static map source =
handle_bounded_map_initialize(
    &(static small_fixed_map){},
    struct Val,
    key,
    handle_bounded_map_key_order,
    NULL,
    NULL,
    handle_bounded_map_fixed_capacity(small_fixed_map)
);
insert_rand_vals(&source);
static map destination = handle_bounded_map_initialize(
    &(static small_fixed_map){},
    struct Val,
    key,
    handle_bounded_map_key_order,
    NULL,
    NULL,
    handle_bounded_map_fixed_capacity(small_fixed_map)
);
CCC_Result res = handle_bounded_map_copy(&destination, &source, NULL);
```

The above requires destination capacity be greater than or equal to source
capacity. Here is memory management handed over to the copy function.

```
#define HANDLE_BOUNDED_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
static Handle_adaptive_map source
    = handle_bounded_map_initialize(NULL, struct Val, key, key_order,
std_allocate, NULL, 0); insert_rand_vals(&source); static Handle_adaptive_map
destination = handle_bounded_map_initialize(NULL, struct Val, key, key_order,
std_allocate, NULL, 0); CCC_Result res = handle_bounded_map_copy(&destination,
&source, std_allocate);
```

The above allows destination to have a capacity less than that of the source as
long as copy has been provided an allocation function to resize destination.
Note that this would still work if copying to a destination that the user wants
as a fixed size map.

```
#define HANDLE_BOUNDED_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
static Handle_adaptive_map source
    = handle_bounded_map_initialize(NULL, struct Val, key, key_order,
std_allocate, NULL, 0); insert_rand_vals(&source); static Handle_adaptive_map
destination = handle_bounded_map_initialize(NULL, struct Val, key, key_order,
NULL, NULL, 0); CCC_Result res = handle_bounded_map_copy(&destination, &source,
std_allocate);
```

The above sets up destination with fixed size while source is a dynamic map.
Because an allocation function is provided, the destination is resized once for
the copy and retains its fixed size after the copy is complete. This would
require the user to manually free the underlying Buffer at destination
eventually if this method is used. Usually it is better to allocate the memory
explicitly before the copy if copying between maps without allocation
permission.

These options allow users to stay consistent across containers with their
memory management strategies. */
CCC_Result CCC_handle_bounded_map_copy(CCC_Handle_bounded_map *destination,
                                       CCC_Handle_bounded_map const *source,
                                       CCC_Allocator *allocate);

/** @brief Reserves space for at least to_add more elements.
@param[in] map a pointer to the handle bounded map.
@param[in] to_add the number of elements to add to the current size.
@param[in] allocate the allocation function to use to reserve memory.
@return the result of the reservation. OK if successful, otherwise an error
status is returned.
@note see the CCC_handle_bounded_map_clear_and_free_reserve function if
this function is being used for a one-time dynamic reservation.

This function can be used for a dynamic map with or
without allocation permission. If the map has allocation
permission, it will reserve the required space and later resize if more space is
needed.

If the map has been initialized with no allocation
permission and no memory this function can serve as a one-time reservation. This
is helpful when a fixed size is needed but that size is only known dynamically
at runtime. To free the map in such a case see the
CCC_handle_bounded_map_clear_and_free_reserve function. */
CCC_Result CCC_handle_bounded_map_reserve(CCC_Handle_bounded_map *map,
                                          size_t to_add,
                                          CCC_Allocator *allocate);

/**@}*/

/**@name Membership Interface
Test membership or obtain references to stored user types directly. */
/**@{*/

/** @brief Returns a reference to the user data at the provided handle.
@param[in] handle a pointer to the map.
@param[in] index the stable handle obtained by the user.
@return a pointer to the user type stored at the specified handle or NULL if
an out of range handle or handle representing no data is provided.
@warning this function can only check if the handle value is in range. If a
handle represents a slot that has been taken by a new element because the old
one has been removed that new element data will be returned.
@warning do not try to access data in the table manually with a handle. Always
use this provided interface function when a reference to data is needed. */
[[nodiscard]] void *
CCC_handle_bounded_map_at(CCC_Handle_bounded_map const *handle,
                          CCC_Handle_index index);

/** @brief Returns a reference to the user type in the table at the handle.
@param[in] Handle_bounded_map_pointer a pointer to the map.
@param[in] type_name name of the user type stored in each slot of the map.
@param[in] handle_index the index handle obtained from previous map operations.
@return a reference to the handle at handle in the map as the type the user has
stored in the map. */
#define CCC_handle_bounded_map_as(Handle_bounded_map_pointer, type_name,       \
                                  handle_index...)                             \
    CCC_private_handle_bounded_map_as(Handle_bounded_map_pointer, type_name,   \
                                      handle_index)

/** @brief Searches the map for the presence of key.
@param[in] map the map to be searched.
@param[in] key pointer to the key matching the key type of the user struct.
@return true if the struct containing key is stored, false if not. Error if
handle_bounded_map or key is NULL. */
[[nodiscard]] CCC_Tribool
CCC_handle_bounded_map_contains(CCC_Handle_bounded_map const *map,
                                void const *key);

/** @brief Returns a reference into the map at handle key.
@param[in] map the bounded map to search.
@param[in] key the key to search matching stored key type.
@return a view of the map handle if it is present, else NULL. */
[[nodiscard]] CCC_Handle_index
CCC_handle_bounded_map_get_key_value(CCC_Handle_bounded_map const *map,
                                     void const *key);

/**@}*/

/** @name Handle Interface
Obtain and operate on container entries for efficient queries when non-trivial
control flow is needed. */
/**@{*/

/** @brief Invariantly inserts the key value in type_output.
@param[in] map the pointer to the bounded map.
@param[out] type_output the type user type map elem.
@return a type element in the table. If Vacant, no prior element with
key existed and the type key value type remains unchanged. If Occupied the
old value is written to the type key value type. If more space is needed
but allocation fails or has been forbidden, an insert error is set.

Note that this function may write to the provided user type struct. */
[[nodiscard]] CCC_Handle
CCC_handle_bounded_map_swap_handle(CCC_Handle_bounded_map *map,
                                   void *type_output);

/** @brief Invariantly inserts the key value in type_output_pointer.
@param[in] Handle_bounded_map_pointer the pointer to the bounded map.
@param[out] type_output_pointer type user type map elem.
@return a compound literal reference to a type element in the table. If
Vacant, no prior element with key existed and the type key value type
remains unchanged. If Occupied the old value is written to the type wrapping
key value type. If more space is needed but allocation fails or has been
forbidden, an insert error is set.

Note that this function may write to the provided user type struct. */
#define CCC_handle_bounded_map_swap_handle_wrap(Handle_bounded_map_pointer,    \
                                                type_output_pointer)           \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_handle_bounded_map_swap_handle((Handle_bounded_map_pointer),       \
                                           (type_output_pointer))              \
            .private                                                           \
    }

/** @brief Attempts to insert the key value in type.
@param[in] map the pointer to the map.
@param[in] type the type user type map elem.
@return a handle. If Occupied, the handle contains a reference to the key value
user type in the map and may be unwrapped. If Vacant the handle contains a
reference to the newly inserted handle in the map. If more space is needed but
allocation fails, an insert error is set. */
[[nodiscard]] CCC_Handle
CCC_handle_bounded_map_try_insert(CCC_Handle_bounded_map *map,
                                  void const *type);

/** @brief Attempts to insert the key value type_pointer.
@param[in] Handle_bounded_map_pointer the pointer to the map.
@param[in] type_pointer the type user type map elem.
@return a compound literal reference to a handle. If Occupied, the handle
contains a reference to the key value user type in the map and may be unwrapped.
If Vacant the handle contains a reference to the newly inserted handle in the
map. If more space is needed but allocation fails an insert error is set. */
#define CCC_handle_bounded_map_try_insert_wrap(Handle_bounded_map_pointer,     \
                                               type_pointer)                   \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_handle_bounded_map_try_insert((Handle_bounded_map_pointer),        \
                                          (type_pointer))                      \
            .private                                                           \
    }

/** @brief lazily insert type_compound_literal into the map at key if key is
absent.
@param[in] Handle_bounded_map_pointer a pointer to the map.
@param[in] key the direct key r-value.
@param[in] type_compound_literal the compound literal specifying the value.
@return a compound literal reference to the handle of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Behavior in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define CCC_handle_bounded_map_try_insert_with(Handle_bounded_map_pointer,     \
                                               key, type_compound_literal...)  \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_private_handle_bounded_map_try_insert_with(                        \
            Handle_bounded_map_pointer, key, type_compound_literal)            \
    }

/** @brief Invariantly inserts or overwrites a user struct into the map.
@param[in] map a pointer to the handle hash map.
@param[in] type the type user struct key value.
@return a handle. If Occupied a handle was overwritten by the new key value.
If Vacant no prior map handle existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
[[nodiscard]] CCC_Handle
CCC_handle_bounded_map_insert_or_assign(CCC_Handle_bounded_map *map,
                                        void const *type);

/** @brief Inserts a new key value pair or overwrites the existing handle.
@param[in] Handle_bounded_map_pointer the pointer to the handle hash map.
@param[in] key the key to be searched in the map.
@param[in] type_compound_literal the compound literal to insert or use for
overwrite.
@return a compound literal reference to the handle of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unin any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define CCC_handle_bounded_map_insert_or_assign_with(                          \
    Handle_bounded_map_pointer, key, type_compound_literal...)                 \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_private_handle_bounded_map_insert_or_assign_with(                  \
            Handle_bounded_map_pointer, key, type_compound_literal)            \
    }

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing type_output provided by the user.
@param[in] map the pointer to the bounded map.
@param[out] type_output the type user type map elem.
@return the removed handle. If Occupied key value type holds the old key value
pair. If Vacant the key value pair was not stored in the map. If bad input is
provided an input error is set.

Note that this function may write to the user type struct. */
[[nodiscard]] CCC_Handle
CCC_handle_bounded_map_remove(CCC_Handle_bounded_map *map, void *type_output);

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing key value type provided by the user.
@param[in] Handle_bounded_map_pointer the pointer to the bounded map.
@param[out] type_output_pointer type user type map elem.
@return a compound literal reference to the removed handle. If Occupied
type_output_pointer holds the old key value pair.. If Vacant the key
value pair was not stored in the map. If bad input is provided an input error is
set.

Note that this function may write to the user type struct. */
#define CCC_handle_bounded_map_remove_wrap(Handle_bounded_map_pointer,         \
                                           type_output_pointer)                \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_handle_bounded_map_remove((Handle_bounded_map_pointer),            \
                                      (type_output_pointer))                   \
            .private                                                           \
    }

/** @brief Obtains a handle for the provided key in the map for future use.
@param[in] map the map to be searched.
@param[in] key the key used to search the map matching the stored key type.
@return a specialized handle for use with other functions in the Handle
Interface.
@warning the contents of a handle should not be examined or modified. Use the
provided functions, only.

A handle is a search result that provides either an Occupied or Vacant handle
in the map. An occupied handle signifies that the search was successful. A
Vacant handle means the search was not successful but a handle is gained to
where in the map such an element should be inserted.

A handle is rarely useful on its own. It should be passed in a functional style
to subsequent calls in the Handle Interface. */
[[nodiscard]] CCC_Handle_bounded_map_handle
CCC_handle_bounded_map_handle(CCC_Handle_bounded_map const *map,
                              void const *key);

/** @brief Obtains a handle for the provided key in the map for future use.
@param[in] Handle_bounded_map_pointer the map to be searched.
@param[in] key_pointer the key used to search the map matching the stored key
type.
@return a compound literal reference to a specialized handle for use with other
functions in the Handle Interface.
@warning the contents of a handle should not be examined or modified. Use the
provided functions, only.

A handle is a search result that provides either an Occupied or Vacant handle
in the map. An occupied handle signifies that the search was successful. A
Vacant handle means the search was not successful but a handle is gained to
where in the map such an element should be inserted.

A handle is rarely useful on its own. It should be passed in a functional style
to subsequent calls in the Handle Interface. */
#define CCC_handle_bounded_map_handle_wrap(Handle_bounded_map_pointer,         \
                                           key_pointer)                        \
    &(CCC_Handle_bounded_map_handle)                                           \
    {                                                                          \
        CCC_handle_bounded_map_handle((Handle_bounded_map_pointer),            \
                                      (key_pointer))                           \
            .private                                                           \
    }

/** @brief Modifies the provided handle if it is Occupied.
@param[in] handle the handle obtained from a handle function or macro.
@param[in] modify an update function in which the context argument is unused.
@return the updated handle if it was Occupied or the unmodified vacant handle.

This function is intended to make the function chaining in the Handle Interface
more succinct if the handle will be modified in place based on its own value
without the need of the context argument a CCC_Type_modifier can provide.
*/
[[nodiscard]] CCC_Handle_bounded_map_handle *
CCC_handle_bounded_map_and_modify(CCC_Handle_bounded_map_handle *handle,
                                  CCC_Type_modifier *modify);

/** @brief Modifies the provided handle if it is Occupied.
@param[in] handle the handle obtained from a handle function or macro.
@param[in] modify an update function that requires context data.
@param[in] context context data required for the update.
@return the updated handle if it was Occupied or the unmodified vacant handle.

This function makes full use of a CCC_Type_modifier capability, meaning a
complete CCC_update object will be passed to the update function callback. */
[[nodiscard]] CCC_Handle_bounded_map_handle *
CCC_handle_bounded_map_and_modify_context(CCC_Handle_bounded_map_handle *handle,
                                          CCC_Type_modifier *modify,
                                          void *context);

/** @brief Modify an Occupied handle with a closure over user type T.
@param[in] map_handle_pointer a pointer to the obtained
handle.
@param[in] type_name the name of the user type stored in the container.
@param[in] closure_over_T the code to be run on the reference to user type T,
if Occupied. This may be a semicolon separated list of statements to execute on
T or a section of code wrapped in braces {code here} which may be preferred
for formatting.
@return a compound literal reference to the modified handle if it was occupied
or a vacant handle if it was vacant.
@note T is a reference to the user type stored in the handle guaranteed to be
non-NULL if the closure executes.

```
#define HANDLE_BOUNDED_MAP_USING_NAMESPACE_CCC
handle_bounded_map_handle *e =
handle_bounded_map_and_modify_with(handle_wrap(&handle_bounded_map,
&k), word, T->cnt++;); Handle_i w =
handle_bounded_map_or_insert_with(handle_bounded_map_and_modify_with(handle_wrap(&handle_bounded_map,
&k), word, { T->cnt++; }), (word){.key = k, .cnt = 1});
```

Note that any code written is only evaluated if the handle is Occupied and the
container can deliver the user type T. This means any function calls are lazily
evaluated in the closure scope. */
#define CCC_handle_bounded_map_and_modify_with(map_handle_pointer, type_name,  \
                                               closure_over_T...)              \
    &(CCC_Handle_bounded_map_handle)                                           \
    {                                                                          \
        CCC_private_handle_bounded_map_and_modify_with(                        \
            map_handle_pointer, type_name, closure_over_T)                     \
    }

/** @brief Inserts the provided user type if the handle is Vacant.
@param[in] handle the handle obtained via function or macro call.
@param[in] type the type struct to be inserted to a Vacant handle.
@return a pointer to handle in the map invariantly. NULL on error.

Because this functions takes a handle and inserts if it is Vacant, the only
reason NULL shall be returned is when an insertion error occurs, usually due to
a user struct allocation failure.

If no allocation is permitted, this function assumes the user struct wrapping
elem has been allocated with the appropriate lifetime and scope by the user. */
[[nodiscard]] CCC_Handle_index
CCC_handle_bounded_map_or_insert(CCC_Handle_bounded_map_handle const *handle,
                                 void const *type);

/** @brief Lazily insert the desired key value into the handle if it is Vacant.
@param[in] map_handle_pointer a pointer to the obtained
handle.
@param[in] type_compound_literal the compound literal to construct in place if
the handle is Vacant.
@return a reference to the unwrapped user type in the handle, either the
unmodified reference if the handle was Occupied or the newly inserted element
if the handle was Vacant. NULL is returned if resizing is required but fails or
is not allowed.

Note that if the compound literal uses any function calls to generate values
or other data, such functions will not be called if the handle is Occupied. */
#define CCC_handle_bounded_map_or_insert_with(map_handle_pointer,              \
                                              type_compound_literal...)        \
    CCC_private_handle_bounded_map_or_insert_with(map_handle_pointer,          \
                                                  type_compound_literal)

/** @brief Inserts the provided user type invariantly.
@param[in] handle the handle returned from a call obtaining a handle.
@param[in] type a type struct the user intends to insert.
@return a pointer to the inserted element or NULL upon allocation failure.

This method can be used when the old value in the map does not need to
be preserved. See the regular insert method if the old value is of interest. */
[[nodiscard]] CCC_Handle_index CCC_handle_bounded_map_insert_handle(
    CCC_Handle_bounded_map_handle const *handle, void const *type);

/** @brief Write the contents of the compound literal type_compound_literal to a
node.
@param[in] map_handle_pointer a pointer to the obtained
handle.
@param[in] type_compound_literal the compound literal to write to a new slot.
@return a reference to the newly inserted or overwritten user type. NULL is
returned if allocation failed or is not allowed when required. */
#define CCC_handle_bounded_map_insert_handle_with(map_handle_pointer,          \
                                                  type_compound_literal...)    \
    CCC_private_handle_bounded_map_insert_handle_with(map_handle_pointer,      \
                                                      type_compound_literal)

/** @brief Remove the handle from the map if Occupied.
@param[in] handle a pointer to the map handle.
@return a handle containing NULL or a reference to the old handle. If Occupied
a handle in the map existed and was removed. If Vacant, no prior handle existed
to be removed.
@warning the reference to the removed handle is invalidated upon any further
insertions. */
CCC_Handle CCC_handle_bounded_map_remove_handle(
    CCC_Handle_bounded_map_handle const *handle);

/** @brief Remove the handle from the map if Occupied.
@param[in] map_handle_pointer a pointer to the map handle.
@return a compound literal reference to a handle containing NULL or a reference
to the old handle. If Occupied a handle in the map existed and was removed. If
Vacant, no prior handle existed to be removed.

Note that the reference to the removed handle is invalidated upon any further
insertions. */
#define CCC_handle_bounded_map_remove_handle_wrap(map_handle_pointer)          \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_handle_bounded_map_remove_handle((map_handle_pointer)).private     \
    }

/** @brief Unwraps the provided handle to obtain a view into the map element.
@param[in] handle the handle from a query to the map via function or macro.
@return a view into the table handle if one is present, or NULL. */
[[nodiscard]] CCC_Handle_index
CCC_handle_bounded_map_unwrap(CCC_Handle_bounded_map_handle const *handle);

/** @brief Returns the Vacant or Occupied status of the handle.
@param[in] handle the handle from a query to the map via function or macro.
@return true if the handle is occupied, false if not. Error if handle is NULL.
*/
[[nodiscard]] CCC_Tribool
CCC_handle_bounded_map_occupied(CCC_Handle_bounded_map_handle const *handle);

/** @brief Provides the status of the handle should an insertion follow.
@param[in] handle the handle from a query to the table via function or macro.
@return true if a handle obtained from an insertion attempt failed to insert
due to an allocation failure when allocation success was expected. Error if
handle is NULL. */
[[nodiscard]] CCC_Tribool CCC_handle_bounded_map_insert_error(
    CCC_Handle_bounded_map_handle const *handle);

/** @brief Obtain the handle status from a container handle.
@param[in] handle a pointer to the handle.
@return the status stored in the handle after the required action on the
container completes. If handle is NULL a handle input error is returned so
ensure e is non-NULL to avoid an inaccurate status returned.

Note that this function can be useful for debugging or if more detailed
messages are needed for logging purposes. See CCC_handle_status_message() in
ccc/types.h for more information on detailed handle statuses. */
[[nodiscard]] CCC_Handle_status CCC_handle_bounded_map_handle_status(
    CCC_Handle_bounded_map_handle const *handle);

/**@}*/

/** @name Deallocation Interface
Deallocate the container. */
/**@{*/

/** @brief Frees all slots in the map for use without affecting capacity.
@param[in] map the map to be cleared.
@param[in] destroy the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the map before their slots are
forfeit.

If NULL is passed as the destructor function time is O(1), else O(size). */
CCC_Result CCC_handle_bounded_map_clear(CCC_Handle_bounded_map *map,
                                        CCC_Type_destructor *destroy);

/** @brief Frees all slots in the map and frees the underlying buffer.
@param[in] map the map to be cleared.
@param[in] destroy the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the map before their slots are
forfeit.
@return the result of free operation. If no allocate function is provided it is
an error to attempt to free the Buffer and a memory error is returned.
Otherwise, an OK result is returned.

If NULL is passed as the destructor function time is O(1), else O(size). */
CCC_Result CCC_handle_bounded_map_clear_and_free(CCC_Handle_bounded_map *map,
                                                 CCC_Type_destructor *destroy);

/** @brief Frees all slots in the map and frees the
underlying Buffer that was previously dynamically reserved with the reserve
function.
@param[in] map the map to be cleared.
@param[in] destroy the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the handle_bounded_map
before their slots are dropped.
@param[in] allocate the required allocation function to provide to a
dynamically reserved handle_bounded_map. Any context data provided upon
initialization will be passed to the allocation function when called.
@return the result of free operation. OK if success, or an error status to
indicate the error.
@warning It is an error to call this function on a handle_bounded_map
that was not reserved with the provided CCC_Allocator. The
handle_bounded_map must have existing memory to free.

This function covers the edge case of reserving a dynamic capacity for a
handle_bounded_map at runtime but denying the
handle_bounded_map allocation permission to resize. This can help
prevent a map from growing unbounded. The user in this
case knows the map does not have allocation permission
and therefore no further memory will be dedicated to the
handle_bounded_map.

However, to free the map in such a case this function
must be used because the map has no ability to free
itself. Just as the allocation function is required to reserve memory so to is
it required to free memory.

This function will work normally if called on a map with
allocation permission however the normal
CCC_handle_bounded_map_clear_and_free is sufficient for that use case.
*/
CCC_Result
CCC_handle_bounded_map_clear_and_free_reserve(CCC_Handle_bounded_map *map,
                                              CCC_Type_destructor *destroy,
                                              CCC_Allocator *allocate);

/**@}*/

/** @name Iterator Interface
Obtain and manage iterators over the container. */
/**@{*/

/** @brief Return an iterable range of values from [begin_key, end_key). O(lgN).
@param[in] map a pointer to the map.
@param[in] begin_key a pointer to the key intended as the start of the range.
@param[in] end_key a pointer to the key intended as the end of the range.
@return a range containing the first element NOT LESS than the begin_key and
the first element GREATER than end_key.

Note that due to the variety of values that can be returned in the range, using
the provided range iteration functions from types.h is recommended for example:

```
for (struct Val *i = range_begin(&range);
     index != range_end(&range);
     index = next(&handle_bounded_map, i))
{}
```

This avoids any possible errors in handling an end range element that is in the
map versus the end map sentinel. */
[[nodiscard]] CCC_Range
CCC_handle_bounded_map_equal_range(CCC_Handle_bounded_map const *map,
                                   void const *begin_key, void const *end_key);

/** @brief Returns a compound literal reference to the desired range. O(lg N).
@param[in] Handle_bounded_map_pointer a pointer to the map.
@param[in] begin_and_end_key_pointers two pointers, the first to the start of
the range the second to the end of the range.
@return a compound literal reference to the produced range associated with the
enclosing scope. This reference is always non-NULL. */
#define CCC_handle_bounded_map_equal_range_wrap(Handle_bounded_map_pointer,    \
                                                begin_and_end_key_pointers...) \
    &(CCC_Range)                                                               \
    {                                                                          \
        CCC_handle_bounded_map_equal_range((Handle_bounded_map_pointer),       \
                                           (begin_and_end_key_pointers))       \
            .private                                                           \
    }

/** @brief Return an iterable range_reverse of values from [begin_key, end_key).
O(lg N).
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
for (struct Val *iterator = range_reverse_begin(&range_reverse);
     iterator != range_reverse_end(&range_reverse);
     iterator = reverse_next(&fom, i))
{}
```

This avoids any possible errors in handling an reverse_end range_reverse element
that is in the map versus the end map sentinel. */
[[nodiscard]] CCC_Range_reverse
CCC_handle_bounded_map_equal_range_reverse(CCC_Handle_bounded_map const *map,
                                           void const *reverse_begin_key,
                                           void const *reverse_end_key);

/** @brief Returns a compound literal reference to the desired range_reverse.
O(lg N).
@param[in] Handle_bounded_map_pointer a pointer to the map.
@param[in] reverse_begin_and_reverse_end_key_pointers two pointers, the first
to the start of the range_reverse the second to the end of the range_reverse.
@return a compound literal reference to the produced range_reverse associated
with the enclosing scope. This reference is always non-NULL. */
#define CCC_handle_bounded_map_equal_range_reverse_wrap(                       \
    Handle_bounded_map_pointer, reverse_begin_and_reverse_end_key_pointers...) \
    &(CCC_Range_reverse)                                                       \
    {                                                                          \
        CCC_handle_bounded_map_equal_range_reverse(                            \
            (Handle_bounded_map_pointer),                                      \
            (reverse_begin_and_reverse_end_key_pointers))                      \
            .private                                                           \
    }

/** @brief Return the start of an inorder traversal of the map. O(lg N).
@param[in] map a pointer to the map.
@return the oldest minimum element of the map. */
[[nodiscard]] void *
CCC_handle_bounded_map_begin(CCC_Handle_bounded_map const *map);

/** @brief Return the start of a reverse inorder traversal of the map. O(lg N).
@param[in] map a pointer to the map.
@return the oldest maximum element of the map. */
[[nodiscard]] void *
CCC_handle_bounded_map_reverse_begin(CCC_Handle_bounded_map const *map);

/** @brief Return the next element in an inorder traversal of the map. O(1).
@param[in] map a pointer to the map.
@param[in] type_iterator a pointer to the intrusive map element of the
current iterator.
@return the next user type stored in the map in an inorder traversal. */
[[nodiscard]] void *
CCC_handle_bounded_map_next(CCC_Handle_bounded_map const *map,
                            void const *type_iterator);

/** @brief Return the reverse_next element in a reverse inorder traversal of the
map. O(1).
@param[in] map a pointer to the map.
@param[in] type_iterator a pointer to the intrusive map element of the
current iterator.
@return the reverse_next user type stored in the map in a reverse inorder
traversal. */
[[nodiscard]] void *
CCC_handle_bounded_map_reverse_next(CCC_Handle_bounded_map const *map,
                                    void const *type_iterator);

/** @brief Return the end of an inorder traversal of the map. O(1).
@param[in] map a pointer to the map.
@return the newest maximum element of the map. */
[[nodiscard]] void *
CCC_handle_bounded_map_end(CCC_Handle_bounded_map const *map);

/** @brief Return the reverse_end of a reverse inorder traversal of the map.
O(1).
@param[in] map a pointer to the map.
@return the newest minimum element of the map. */
[[nodiscard]] void *
CCC_handle_bounded_map_reverse_end(CCC_Handle_bounded_map const *map);

/**@}*/

/** @name State Interface
Obtain the container state. */
/**@{*/

/** @brief Returns the size status of the map.
@param[in] map the map.
@return true if empty else false. Error if map is NULL.
*/
[[nodiscard]] CCC_Tribool
CCC_handle_bounded_map_is_empty(CCC_Handle_bounded_map const *map);

/** @brief Returns the count of map occupied slots.
@param[in] map the map.
@return the size of the map or an argument error is set if
handle_bounded_map is NULL. */
[[nodiscard]] CCC_Count
CCC_handle_bounded_map_count(CCC_Handle_bounded_map const *map);

/** @brief Returns the capacity of the map representing total available slots.
@param[in] map the map.
@return the capacity or an argument error is set if handle_bounded_map
is NULL. */
[[nodiscard]] CCC_Count
CCC_handle_bounded_map_capacity(CCC_Handle_bounded_map const *map);

/** @brief Validation of invariants for the map.
@param[in] map the map to validate.
@return true if all invariants hold, false if corruption occurs. Error if
handle_bounded_map is NULL. */
[[nodiscard]] CCC_Tribool
CCC_handle_bounded_map_validate(CCC_Handle_bounded_map const *map);

/**@}*/

/** Define this preprocessor directive if shorter names are helpful. Ensure
 no namespace clashes occur before shortening. */
#ifdef HANDLE_BOUNDED_MAP_USING_NAMESPACE_CCC
typedef CCC_Handle_bounded_map Handle_bounded_map;
typedef CCC_Handle_bounded_map_handle Handle_bounded_map_handle;
#    define handle_bounded_map_declare_fixed_map(args...)                      \
        CCC_handle_bounded_map_declare_fixed_map(args)
#    define handle_bounded_map_initialize(args...)                             \
        CCC_handle_bounded_map_initialize(args)
#    define handle_bounded_map_fixed_capacity(args...)                         \
        CCC_handle_bounded_map_fixed_capacity(args)
#    define handle_bounded_map_copy(args...) CCC_handle_bounded_map_copy(args)
#    define handle_bounded_map_reserve(args...)                                \
        CCC_handle_bounded_map_reserve(args)
#    define handle_bounded_map_at(args...) CCC_handle_bounded_map_at(args)
#    define handle_bounded_map_as(args...) CCC_handle_bounded_map_as(args)
#    define handle_bounded_map_and_modify_with(args...)                        \
        CCC_handle_bounded_map_and_modify_with(args)
#    define handle_bounded_map_or_insert_with(args...)                         \
        CCC_handle_bounded_map_or_insert_with(args)
#    define handle_bounded_map_insert_handle_with(args...)                     \
        CCC_handle_bounded_map_insert_handle_with(args)
#    define handle_bounded_map_try_insert_with(args...)                        \
        CCC_handle_bounded_map_try_insert_with(args)
#    define handle_bounded_map_insert_or_assign_with(args...)                  \
        CCC_handle_bounded_map_insert_or_assign_with(args)
#    define handle_bounded_map_contains(args...)                               \
        CCC_handle_bounded_map_contains(args)
#    define handle_bounded_map_get_key_value(args...)                          \
        CCC_handle_bounded_map_get_key_value(args)
#    define handle_bounded_map_swap_handle(args...)                            \
        CCC_handle_bounded_map_swap_handle(args)
#    define handle_bounded_map_swap_handle_wrap(args...)                       \
        CCC_handle_bounded_map_swap_handle_wrap(args)
#    define handle_bounded_map_begin(args...) CCC_handle_bounded_map_begin(args)
#    define handle_bounded_map_reverse_begin(args...)                          \
        CCC_handle_bounded_map_reverse_begin(args)
#    define handle_bounded_map_next(args...) CCC_handle_bounded_map_next(args)
#    define handle_bounded_map_reverse_next(args...)                           \
        CCC_handle_bounded_map_reverse_next(args)
#    define handle_bounded_map_end(args...) CCC_handle_bounded_map_end(args)
#    define handle_bounded_map_reverse_end(args...)                            \
        CCC_handle_bounded_map_reverse_end(args)
#    define handle_bounded_map_is_empty(args...)                               \
        CCC_handle_bounded_map_is_empty(args)
#    define handle_bounded_map_count(args...) CCC_handle_bounded_map_count(args)
#    define handle_bounded_map_clear(args...) CCC_handle_bounded_map_clear(args)
#    define handle_bounded_map_clear_and_free(args...)                         \
        CCC_handle_bounded_map_clear_and_free(args)
#    define handle_bounded_map_clear_and_free_reserve(args...)                 \
        CCC_handle_bounded_map_clear_and_free_reserve(args)
#    define handle_bounded_map_validate(args...)                               \
        CCC_handle_bounded_map_validate(args)
#endif /* HANDLE_BOUNDED_MAP_USING_NAMESPACE_CCC */

#endif /* CCC_HANDLE_BOUNDED_MAP_H */
