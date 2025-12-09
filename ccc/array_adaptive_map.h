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
@brief The Array Adaptive Map Interface

A array adaptive map is a contiguously stored map offering storage and
retrieval by key. Because the data structure is self-optimizing it is not a
suitable map in a realtime environment where strict runtime bounds are needed.
Also, searching the map is not a const thread-safe operation as indicated by the
function signatures. The map is optimized upon every new search in attempt to
adapt to the usage pattern. In many cases the self-optimizing structure
of the map may be beneficial when considering non-uniform access patterns. In
the best case, repeated searches of the same value yield an `O(1)` access and
many other frequently searched values will remain close to the root of the map.

The array version of the adaptive map promises contiguous storage and random
access if needed. Handles remain valid until an element is removed even if other
elements are inserted, other elements are removed, or resizing occurs. All
elements in the map track their relationships to one another via indices in the
array. Therefore, this data structure can be relocated, copied, serialized, or
written to disk and all internal data structure references will remain valid.
Insertion may invoke an O(N) operation if resizing occurs. Finally, if
allocation is prohibited upon initialization, and the user provides a capacity
of `N` upon initialization, one slot will be used for a sentinel node. The user
available capacity is `N - 1`.

All interface functions accept `void *` references to either the key or the full
type the user is storing in the map. Therefore, it is important for the user to
be aware if they are passing a reference to the key or the full type depending
on the function requirements.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define ARRAY_ADAPTIVE_MAP_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `CCC_` prefix. */
#ifndef CCC_ARRAY_ADAPTIVE_MAP_H
#define CCC_ARRAY_ADAPTIVE_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "private/private_array_adaptive_map.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A self-optimizing data structure offering amortized O(lg N) search,
insert, and erase.
@warning it is undefined behavior to access an uninitialized container.

An array adaptive map can be initialized on the stack, heap, or data segment at
runtime or compile time.*/
typedef struct CCC_Array_adaptive_map CCC_Array_adaptive_map;

/** @brief A container specific handle used to implement the Handle Interface.

The Handle Interface offers efficient search and subsequent insertion, deletion,
or value update based on the needs of the user. */
typedef union CCC_Array_adaptive_map_handle_wrap CCC_Array_adaptive_map_handle;

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
CCC_array_adaptive_map_declare_fixed_map(Small_fixed_map, struct Val, 64);
static Array_adaptive_map static_map = array_adaptive_map_initialize(
    &(static Small_fixed_map){},
    struct Val,
    key,
    array_adaptive_map_key_order,
    NULL,
    NULL,
    array_adaptive_map_fixed_capacity(Small_fixed_map)
);
```

Similarly, a fixed size map can be used on the stack.

```
struct Val
{
    int key;
    int val;
};
CCC_array_adaptive_map_declare_fixed_map(Small_fixed_map, struct Val, 64);
int main(void)
{
    Array_adaptive_map map = array_adaptive_map_initialize(
        &(Small_fixed_map){},
        struct Val,
        key,
        array_adaptive_map_key_order,
        NULL,
        NULL,
        array_adaptive_map_fixed_capacity(Small_fixed_map)
    );
    return 0;
}
```

The CCC_array_adaptive_map_fixed_capacity macro can be used to obtain the
previously provided capacity when declaring the fixed map type. Finally, one
could allocate a fixed size map on the heap; however, it is usually better to
initialize a dynamic map and use the CCC_array_adaptive_map_reserve function
for such a use case.

This macro is not needed when a dynamic resizing map is needed. For dynamic
maps, pass NULL and 0 capacity to the initialization macro along with the
desired allocation function. */
#define CCC_array_adaptive_map_declare_fixed_map(fixed_map_type_name,          \
                                                 type_name, capacity)          \
    CCC_private_array_adaptive_map_declare_fixed_map(fixed_map_type_name,      \
                                                     type_name, capacity)

/** @brief Obtain the capacity previously chosen for the fixed size map type.
@param[in] fixed_map_type_name the name of a previously declared map.
@return the size_t capacity previously specified for this type by user. */
#define CCC_array_adaptive_map_fixed_capacity(fixed_map_type_name)             \
    CCC_private_array_adaptive_map_fixed_capacity(fixed_map_type_name)

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
@return the struct initialized adaptive map for direct assignment
(i.e. CCC_Array_adaptive_map m = CCC_array_adaptive_map_initialize(...);). */
#define CCC_array_adaptive_map_initialize(memory_pointer, type_name,           \
                                          type_intruder_field, compare,        \
                                          allocate, context_data, capacity)    \
    CCC_private_array_adaptive_map_initialize(                                 \
        memory_pointer, type_name, type_intruder_field, compare, allocate,     \
        context_data, capacity)

/** @brief Initialize a dynamic map at runtime from an initializer list.
@param[in] type_key_field the field of the struct used for key storage.
@param[in] compare the CCC_Key_comparator the user intends to use.
@param[in] allocate the required allocation function.
@param[in] context_data context data that is needed for hashing or comparison.
@param[in] optional_capacity optionally specify the capacity of the map if
different from the size of the compound literal array initializer. If the
capacity is greater than the size of the compound literal array initializer, it
is respected and the capacity is reserved. If the capacity is less than the size
of the compound array initializer, the compound literal array initializer size
is set as the capacity. Therefore, 0 is valid if one is not concerned with the
size of the underlying reservation.
@param[in] type_compound_literal_array a list of key value pairs of the type
intended to be stored in the map, using array compound literal initialization
syntax (e.g `(struct my_type[]){{.k = 0, .v 0}, {.k = 1, .v = 1}}`).
@return the  map directly initialized on the right hand side of the equality
operator (i.e. CCC_Array_adaptive_map map = CCC_array_adaptive_map_from(...);)
@warning An allocation function is required. This initializer is only available
for dynamic maps.
@warning When duplicate keys appear in the initializer list, the last occurrence
replaces earlier ones by value (all fields are overwritten).
@warning If initialization fails all subsequent queries, insertions, or
removals will indicate the error: either memory related or lack of an
allocation function provided.

Initialize a dynamic map at run time. This example requires no context data for
initialization.

```
#define ARRAY_ADAPTIVE_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
int
main(void)
{
    Array_adaptive_map static_map = array_adaptive_map_from(
        key,
        array_adaptive_map_key_order,
        std_allocate,
        NULL,
        0,
        (struct Val[]) {
            {.key = 1, .val = 1},
            {.key = 2, .val = 2},
            {.key = 3, .val = 3},
        },
    );
    return 0;
}
```

Only dynamic maps may be initialized this way due the inability of the map
map to protect its invariants from user error at compile time. */
#define CCC_array_adaptive_map_from(type_key_field, compare, allocate,         \
                                    context_data, optional_capacity,           \
                                    type_compound_literal_array...)            \
    CCC_private_array_adaptive_map_from(type_key_field, compare, allocate,     \
                                        context_data, optional_capacity,       \
                                        type_compound_literal_array)

/** @brief Initialize a dynamic map at runtime with at least the specified
capacity.
@param[in] type_name the name of the type being stored in the map.
@param[in] type_key_field the field of the struct used for key storage.
@param[in] compare the CCC_Key_comparator the user intends to use.
@param[in] allocate the required allocation function.
@param[in] context_data context data that is needed for comparison.
@param[in] capacity the desired capacity for the map. A capacity of 0 results
in an argument error and is a no-op after the map is initialized empty.
@return the map directly initialized on the right hand side of the equality
operator (i.e. CCC_Array_adaptive_map map =
CCC_array_adaptive_map_with_capacity(...);)
@warning An allocation function is required. This initializer is only available
for dynamic maps.
@warning If initialization fails all subsequent queries, insertions, or
removals will indicate the error: either memory related or lack of an
allocation function provided.

Initialize a dynamic map at run time. This example requires no context
data for initialization.

```
#define ARRAY_ADAPTIVE_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
int
main(void)
{
    Array_adaptive_map map = array_adaptive_map_with_capacity(
        struct Val,
        key,
        array_adaptive_map_key_order,
        std_allocate,
        NULL,
        4096
    );
    return 0;
}
```

Only dynamic maps may be initialized this way as it simply combines the steps
of initialization and reservation. */
#define CCC_array_adaptive_map_with_capacity(                                  \
    type_name, type_key_field, compare, allocate, context_data, capacity)      \
    CCC_private_array_adaptive_map_with_capacity(                              \
        type_name, type_key_field, compare, allocate, context_data, capacity)

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
provided with the fn argument.

Note that there are two ways to copy data from source to destination: provide
sufficient memory and pass NULL as fn, or allow the copy function to take care
of allocation for the copy.

Manual memory management with no allocation function provided.

```
#define ARRAY_ADAPTIVE_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
CCC_array_adaptive_map_declare_fixed_map(Small_fixed_map, struct Val, 64);
static Array_tree_map source = array_adaptive_map_initialize(
    &(static Small_fixed_map){},
    struct Val,
    key,
    array_adaptive_map_key_order,
    NULL,
    NULL,
    array_adaptive_map_fixed_capacity(Small_fixed_map)
);
insert_rand_vals(&source);
static Array_tree_map destination = array_adaptive_map_initialize(
    &(static Small_fixed_map){},
    struct Val,
    key,
    array_adaptive_map_key_order,
    NULL,
    NULL,
    array_adaptive_map_fixed_capacity(Small_fixed_map)
);
CCC_Result res = array_adaptive_map_copy(&destination, &source, NULL);
```

The above requires destination capacity be greater than or equal to source
capacity. Here is memory management handed over to the copy function.

```
#define ARRAY_ADAPTIVE_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
static Array_adaptive_map source = array_adaptive_map_initialize(
    NULL,
    struct Val,
    key,
    key_order,
    std_allocate,
    NULL,
    0
);
insert_rand_vals(&source);
static Array_adaptive_map destination = array_adaptive_map_initialize(
    NULL,
    struct Val,
    key,
    key_order,
    std_allocate,
    NULL,
    0
);
CCC_Result res = array_adaptive_map_copy(&destination, &source, std_allocate);
```

The above allows destination to have a capacity less than that of the source as
long as copy has been provided an allocation function to resize destination.
Note that this would still work if copying to a destination that the user wants
as a fixed size map.

```
#define ARRAY_ADAPTIVE_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
static Array_adaptive_map source = array_adaptive_map_initialize(
    NULL,
    struct Val,
    key,
    key_order,
    std_allocate,
    NULL,
    0
);
insert_rand_vals(&source);
static Array_adaptive_map destination = array_adaptive_map_initialize(
    NULL,
    struct Val,
    key,
    key_order,
    NULL,
    NULL,
    0
);
CCC_Result res
    = array_adaptive_map_copy(&destination, &source, std_allocate);
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
CCC_Result CCC_array_adaptive_map_copy(CCC_Array_adaptive_map *destination,
                                       CCC_Array_adaptive_map const *source,
                                       CCC_Allocator *allocate);

/** @brief Reserves space for at least to_add more elements.
@param[in] map a pointer to the array adaptive map.
@param[in] to_add the number of elements to add to the current size.
@param[in] allocate the allocation function to use to reserve memory.
@return the result of the reservation. OK if successful, otherwise an error
status is returned.
@note see the CCC_array_adaptive_map_clear_and_free_reserve function if this
function is being used for a one-time dynamic reservation.

This function can be used for a dynamic array_adaptive_map with or without
allocation permission. If the array_adaptive_map has allocation permission, it
will reserve the required space and later resize if more space is needed.

If the array_adaptive_map has been initialized with no allocation permission
and no memory this function can serve as a one-time reservation. This is helpful
when a fixed size is needed but that size is only known dynamically at runtime.
To free the array_adaptive_map in such a case see the
CCC_array_adaptive_map_clear_and_free_reserve function. */
CCC_Result CCC_array_adaptive_map_reserve(CCC_Array_adaptive_map *map,
                                          size_t to_add,
                                          CCC_Allocator *allocate);

/**@}*/

/**@name Membership Interface
Test membership or obtain references to stored user types directly. */
/**@{*/

/** @brief Returns a reference to the user data at the provided handle.
@param[in] map a pointer to the map.
@param[in] index the stable handle obtained by the user.
@return a pointer to the user type stored at the specified handle or NULL if
an out of range handle or handle representing no data is provided.
@warning this function can only check if the handle value is in range. If a
handle represents a slot that has been taken by a new element because the
old one has been removed that new element data will be returned.
@warning do not try to access data in the table manually with a handle.
Always use this provided interface function when a reference to data is
needed. */
[[nodiscard]] void *CCC_array_adaptive_map_at(CCC_Array_adaptive_map const *map,
                                              CCC_Handle_index index);

/** @brief Returns a reference to the user type in the table at the handle.
@param[in] map_pointer a pointer to the map.
@param[in] type_name name of the user type stored in each slot of the map.
@param[in] array_index the index handle obtained from previous map operations.
@return a reference to the handle at handle in the map as the type the user has
stored in the map. */
#define CCC_array_adaptive_map_as(map_pointer, type_name, array_index...)      \
    CCC_private_array_adaptive_map_as(map_pointer, type_name, array_index)

/** @brief Searches the map for the presence of key.
@param[in] map the map to be searched.
@param[in] key pointer to the key matching the key type of the user struct.
@return true if the struct containing key is stored, false if not. Error if
array_adaptive_map or key is NULL. */
[[nodiscard]] CCC_Tribool
CCC_array_adaptive_map_contains(CCC_Array_adaptive_map *map, void const *key);

/** @brief Returns a reference into the map at handle key.
@param[in] map the adaptive map to search.
@param[in] key the key to search matching stored key type.
@return a view of the map handle if it is present, else NULL. */
[[nodiscard]] CCC_Handle_index
CCC_array_adaptive_map_get_key_value(CCC_Array_adaptive_map *map,
                                     void const *key);

/**@}*/

/** @name Handle Interface
Obtain and operate on container handles for efficient queries when non-trivial
control flow is needed. */
/**@{*/

/** @brief Invariantly inserts the key value wrapping type.
@param[in] map the pointer to the adaptive map.
@param[out] type_output the handle to the user type wrapping map elem.
@return a handle. If Vacant, no prior element with key existed and the type
wrapping type_output remains unchanged. If Occupied the old value is written
to the type wrapping type_output and may be unwrapped to view. If more space
is needed but allocation fails or has been forbidden, an insert error is set.

Note that this function may write to the struct containing type_output and
wraps it in a handle to provide information about the old value. */
[[nodiscard]] CCC_Handle
CCC_array_adaptive_map_swap_handle(CCC_Array_adaptive_map *map,
                                   void *type_output);

/** @brief Invariantly inserts the key value wrapping type.
@param[in] map_pointer the pointer to the adaptive map.
@param[out] type_output_pointer the handle to the user type wrapping map
elem.
@return a compound literal reference to a handle. If Vacant, no prior element
with key existed and the type wrapping type_output remains unchanged. If
Occupied the old value is written to the type wrapping type_output and may be
unwrapped to view. If more space is needed but allocation fails or has been
forbidden, an insert error is set.

Note that this function may write to the struct containing type_output and
wraps it in a handle to provide information about the old value. */
#define CCC_array_adaptive_map_swap_handle_wrap(map_pointer,                   \
                                                type_output_pointer)           \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_array_adaptive_map_swap_handle((map_pointer),                      \
                                           (type_output_pointer))              \
            .private                                                           \
    }

/** @brief Attempts to insert the key value wrapping type.
@param[in] map the pointer to the map.
@param[in] type the handle to the user type wrapping map elem.
@return a handle. If Occupied, the handle contains a reference to the key value
user type in the map and may be unwrapped. If Vacant the handle contains a
reference to the newly inserted handle in the map. If more space is needed but
allocation fails, an insert error is set. */
[[nodiscard]] CCC_Handle
CCC_array_adaptive_map_try_insert(CCC_Array_adaptive_map *map,
                                  void const *type);

/** @brief Attempts to insert the key value wrapping type.
@param[in] map_pointer the pointer to the map.
@param[in] type_pointer the handle to the user type wrapping map elem.
@return a compound literal reference to a handle. If Occupied, the handle
contains a reference to the key value user type in the map and may be unwrapped.
If Vacant the handle contains a reference to the newly inserted handle in the
map. If more space is needed but allocation fails an insert error is set. */
#define CCC_array_adaptive_map_try_insert_wrap(map_pointer, type_pointer)      \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_array_adaptive_map_try_insert((map_pointer), (type_pointer))       \
            .private                                                           \
    }

/** @brief lazily insert type_compound_literal into the map at key if key is
absent.
@param[in] map_pointer a pointer to the map.
@param[in] key the direct key r-value.
@param[in] type_compound_literal the compound literal specifying the value.
@return a compound literal reference to the handle of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define CCC_array_adaptive_map_try_insert_with(map_pointer, key,               \
                                               type_compound_literal...)       \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_private_array_adaptive_map_try_insert_with(map_pointer, key,       \
                                                       type_compound_literal)  \
    }

/** @brief Invariantly inserts or overwrites a user struct into the map.
@param[in] map a pointer to the handle hash map.
@param[in] type the handle to the wrapping user struct key value.
@return a handle. If Occupied a handle was overwritten by the new key value.
If Vacant no prior map handle existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
[[nodiscard]] CCC_Handle
CCC_array_adaptive_map_insert_or_assign(CCC_Array_adaptive_map *map,
                                        void const *type);

/** @brief Inserts a new key value pair or overwrites the existing handle.
@param[in] map_pointer the pointer to the handle hash map.
@param[in] key the key to be searched in the map.
@param[in] type_compound_literal the compound literal to insert or use for
overwrite.
@return a compound literal reference to the handle of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define CCC_array_adaptive_map_insert_or_assign_with(map_pointer, key,         \
                                                     type_compound_literal...) \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_private_array_adaptive_map_insert_or_assign_with(                  \
            map_pointer, key, type_compound_literal)                           \
    }

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing type_output provided by the user.
@param[in] map the pointer to the adaptive map.
@param[out] type_output the handle to the user type wrapping map elem.
@return the removed handle. If Occupied the struct containing type_output
holds the old value. If Vacant the key value pair was not stored in the map. If
bad input is provided an input error is set.

Note that this function may write to the struct containing the second parameter
and wraps it in a handle to provide information about the old value. */
[[nodiscard]] CCC_Handle
CCC_array_adaptive_map_remove_key_value(CCC_Array_adaptive_map *map,
                                        void *type_output);

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing type_output provided by the user.
@param[in] map_pointer the pointer to the adaptive map.
@param[out] type_output_pointer the handle to the user type wrapping map
elem.
@return a compound literal reference to a handle. If Occupied the struct
containing type_output holds the old value. If Vacant the key value pair was
not stored in the map. If bad input is provided an input error is set.

Note that this function may write to the struct containing the second parameter
and wraps it in a handle to provide information about the old value. */
#define CCC_array_adaptive_map_remove_key_value_wrap(map_pointer,              \
                                                     type_output_pointer)      \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_array_adaptive_map_remove_key_value((map_pointer),                 \
                                                (type_output_pointer))         \
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
[[nodiscard]] CCC_Array_adaptive_map_handle
CCC_array_adaptive_map_handle(CCC_Array_adaptive_map *map, void const *key);

/** @brief Obtains a handle for the provided key in the map for future use.
@param[in] array_pointer the map to be searched.
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
#define CCC_array_adaptive_map_handle_wrap(array_pointer, key_pointer)         \
    &(CCC_Array_adaptive_map_handle)                                           \
    {                                                                          \
        CCC_array_adaptive_map_handle((array_pointer), (key_pointer)).private  \
    }

/** @brief Modifies the provided handle if it is Occupied.
@param[in] handle the handle obtained from a handle function or macro.
@param[in] modify an update function in which the context argument is unused.
@return the updated handle if it was Occupied or the unmodified vacant handle.

This function is intended to make the function chaining in the Handle Interface
more succinct if the handle will be modified in place based on its own value
without the need of the context argument a CCC_Type_modifier can provide.
*/
[[nodiscard]] CCC_Array_adaptive_map_handle *
CCC_array_adaptive_map_and_modify(CCC_Array_adaptive_map_handle *handle,
                                  CCC_Type_modifier *modify);

/** @brief Modifies the provided handle if it is Occupied.
@param[in] handle the handle obtained from a handle function or macro.
@param[in] modify an update function that requires context data.
@param[in] context context data required for the update.
@return the updated handle if it was Occupied or the unmodified vacant handle.

This function makes full use of a CCC_Type_modifier capability, meaning a
complete CCC_update object will be passed to the update function callback. */
[[nodiscard]] CCC_Array_adaptive_map_handle *
CCC_array_adaptive_map_and_modify_context(CCC_Array_adaptive_map_handle *handle,
                                          CCC_Type_modifier *modify,
                                          void *context);

/** @brief Modify an Occupied handle with a closure over user type T.
@param[in] array_pointer a pointer to the obtained handle.
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
#define ARRAY_ADAPTIVE_MAP_USING_NAMESPACE_CCC
// Increment the count if found otherwise do nothing.
Array_adaptive_map_handle *e =
    array_adaptive_map_and_modify_with(
        handle_wrap(&array_adaptive_map, &k),
        Word,
        T->cnt++;
    );
// Increment the count if found otherwise insert a default value.
Handle_index w =
    array_adaptive_map_or_insert_with(
        array_adaptive_map_and_modify_with(
            handle_wrap(&array_adaptive_map, &k),
            Word,
            { T->cnt++; }
        ),
        (Word){.key = k, .cnt = 1}
    );
```

Note that any code written is only evaluated if the handle is Occupied and the
container can deliver the user type T. This means any function calls are lazily
evaluated in the closure scope. */
#define CCC_array_adaptive_map_and_modify_with(array_pointer, type_name,       \
                                               closure_over_T...)              \
    &(CCC_Array_adaptive_map_handle)                                           \
    {                                                                          \
        CCC_private_array_adaptive_map_and_modify_with(                        \
            array_pointer, type_name, closure_over_T)                          \
    }

/** @brief Inserts the struct with user type if the handle is Vacant.
@param[in] handle the handle obtained via function or macro call.
@param[in] type handle to the struct to be inserted to Vacant handle.
@return a pointer to handle in the map invariantly. NULL on error.

Because this functions takes a handle and inserts if it is Vacant, the only
reason NULL shall be returned is when an insertion error occurs, usually due to
a user struct allocation failure.

If no allocation is permitted, this function assumes the user struct wrapping
elem has been allocated with the appropriate lifetime and scope by the user. */
[[nodiscard]] CCC_Handle_index
CCC_array_adaptive_map_or_insert(CCC_Array_adaptive_map_handle const *handle,
                                 void const *type);

/** @brief Lazily insert the desired key value into the handle if it is Vacant.
@param[in] array_pointer a pointer to the obtained handle.
@param[in] type_compound_literal the compound literal to construct in place if
the handle is Vacant.
@return a reference to the unwrapped user type in the handle, either the
unmodified reference if the handle was Occupied or the newly inserted element
if the handle was Vacant. NULL is returned if resizing is required but fails or
is not allowed.

Note that if the compound literal uses any function calls to generate values
or other data, such functions will not be called if the handle is Occupied. */
#define CCC_array_adaptive_map_or_insert_with(array_pointer,                   \
                                              type_compound_literal...)        \
    CCC_private_array_adaptive_map_or_insert_with(array_pointer,               \
                                                  type_compound_literal)

/** @brief Inserts the provided handle invariantly.
@param[in] handle the handle returned from a call obtaining a handle.
@param[in] type a handle to the struct the user intends to insert.
@return a pointer to the inserted element or NULL upon allocation failure.

This method can be used when the old value in the map does not need to
be preserved. See the regular insert method if the old value is of interest. */
[[nodiscard]] CCC_Handle_index CCC_array_adaptive_map_insert_handle(
    CCC_Array_adaptive_map_handle const *handle, void const *type);

/** @brief Write the contents of the compound literal type_compound_literal to a
node.
@param[in] array_pointer a pointer to the obtained handle.
@param[in] type_compound_literal the compound literal to write to a new slot.
@return a reference to the newly inserted or overwritten user type. NULL is
returned if allocation failed or is not allowed when required. */
#define CCC_array_adaptive_map_insert_array_with(array_pointer,                \
                                                 type_compound_literal...)     \
    CCC_private_array_adaptive_map_insert_array_with(array_pointer,            \
                                                     type_compound_literal)

/** @brief Remove the handle from the map if Occupied.
@param[in] handle a pointer to the map handle.
@return a handle containing no valid reference but information about removed
element. If Occupied a handle in the map existed and was removed. If Vacant, no
prior handle existed to be removed. */
[[nodiscard]] CCC_Handle
CCC_array_adaptive_map_remove_handle(CCC_Array_adaptive_map_handle *handle);

/** @brief Remove the handle from the map if Occupied.
@param[in] array_pointer pointer to the map handle.
@return a compound literal reference containing no valid reference but
information about the old handle. If Occupied a handle in the map existed and
was removed. If Vacant, no prior handle existed to be removed. */
#define CCC_array_adaptive_map_remove_handle_wrap(array_pointer)               \
    &(CCC_Handle)                                                              \
    {                                                                          \
        CCC_array_adaptive_map_remove_handle((array_pointer)).private          \
    }

/** @brief Unwraps the provided handle to obtain a view into the map element.
@param[in] handle the handle from a query to the map via function or macro.
@return a view into the table handle if one is present, or NULL. */
[[nodiscard]] CCC_Handle_index
CCC_array_adaptive_map_unwrap(CCC_Array_adaptive_map_handle const *handle);

/** @brief Returns the Vacant or Occupied status of the handle.
@param[in] handle the handle from a query to the map via function or macro.
@return true if the handle is occupied, false if not. Error if h is NULL. */
[[nodiscard]] CCC_Tribool
CCC_array_adaptive_map_occupied(CCC_Array_adaptive_map_handle const *handle);

/** @brief Provides the status of the handle should an insertion follow.
@param[in] handle the handle from a query to the table via function or macro.
@return true if a handle obtained from an insertion attempt failed to insert
due to an allocation failure when allocation success was expected. Error if h is
NULL. */
[[nodiscard]] CCC_Tribool CCC_array_adaptive_map_insert_error(
    CCC_Array_adaptive_map_handle const *handle);

/** @brief Obtain the handle status from a container handle.
@param[in] handle a pointer to the handle.
@return the status stored in the handle after the required action on the
container completes. If h is NULL a handle input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned.

Note that this function can be useful for debugging or if more detailed
messages are needed for logging purposes. See CCC_handle_status_message() in
ccc/types.h for more information on detailed handle statuses. */
[[nodiscard]] CCC_Handle_status CCC_array_adaptive_map_handle_status(
    CCC_Array_adaptive_map_handle const *handle);

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
the provided range iteration functions from types.h or traits.h is recommended
for example:

```
for (CCC_Handle_index index = range_begin(&range);
     index != range_end(&range);
     index = next(&map, index))
{}
```

This avoids any possible errors in handling an end range element that is in the
map versus the end map sentinel. */
[[nodiscard]] CCC_Handle_range
CCC_array_adaptive_map_equal_range(CCC_Array_adaptive_map *map,
                                   void const *begin_key, void const *end_key);

/** @brief Returns a compound literal reference to the desired range. Amortized
O(lg N).
@param[in] map_pointer a pointer to the map.
@param[in] begin_and_end_key_pointers pointers to the begin and end of the
range.
@return a compound literal reference to the produced range associated with the
enclosing scope. This reference is always be valid. */
#define CCC_array_adaptive_map_equal_range_wrap(map_pointer,                   \
                                                begin_and_end_key_pointers...) \
    &(CCC_Handle_range)                                                        \
    {                                                                          \
        CCC_array_adaptive_map_equal_range(map_pointer,                        \
                                           begin_and_end_key_pointers)         \
            .private                                                           \
    }

/** @brief Return an iterable range_reverse of values from [reverse_begin_key,
end_key). Amortized O(lg N).
@param[in] map a pointer to the map.
@param[in] reverse_begin_key a pointer to the key intended as the start of the
range_reverse.
@param[in] reverse_end_key a pointer to the key intended as the end of the
range_reverse.
@return a range_reverse containing the first element NOT GREATER than the
begin_key and the first element LESS than reverse_end_key.

Note that due to the variety of values that can be returned in the
range_reverse, using the provided range_reverse iteration functions from types.h
or traits.h is recommended for example:

```
for (CCC_Handle_index index = range_reverse_begin(&range);
     index != range_reverse_end(&range);
     index = next(&map, index))
{}
```

This avoids any possible errors in handling an reverse_end range_reverse element
that is in the map versus the end map sentinel. */
[[nodiscard]] CCC_Handle_range_reverse
CCC_array_adaptive_map_equal_range_reverse(CCC_Array_adaptive_map *map,
                                           void const *reverse_begin_key,
                                           void const *reverse_end_key);

/** @brief Returns a compound literal reference to the desired range_reverse.
Amortized O(lg N).
@param[in] map_pointer a pointer to the map.
@param[in] reverse_begin_and_reverse_end_key_pointers pointers to the
reverse_begin and reverse_end of the range.
@return a compound literal reference to the produced range_reverse associated
with the enclosing scope. This reference is always valid. */
#define CCC_array_adaptive_map_equal_range_reverse_wrap(                       \
    map_pointer, reverse_begin_and_reverse_end_key_pointers...)                \
    &(CCC_Handle_range_reverse)                                                \
    {                                                                          \
        CCC_array_adaptive_map_equal_range_reverse(                            \
            map_pointer, reverse_begin_and_reverse_end_key_pointers)           \
            .private                                                           \
    }

/** @brief Return the start of an inorder traversal of the map. Amortized
O(lg N).
@param[in] map a pointer to the map.
@return a handle for the minimum element of the map. */
[[nodiscard]] CCC_Handle_index
CCC_array_adaptive_map_begin(CCC_Array_adaptive_map const *map);

/** @brief Return the start of a reverse inorder traversal of the map.
Amortized O(lg N).
@param[in] map a pointer to the map.
@return a handle for the maximum element of the map. */
[[nodiscard]] CCC_Handle_index
CCC_array_adaptive_map_reverse_begin(CCC_Array_adaptive_map const *map);

/** @brief Return the next element in an inorder traversal of the map. O(1).
@param[in] map a pointer to the map.
@param[in] iterator pointer to the current iterator user type.
@return a handle for the next user type stored in the map in an inorder
traversal. */
[[nodiscard]] CCC_Handle_index
CCC_array_adaptive_map_next(CCC_Array_adaptive_map const *map,
                            CCC_Handle_index iterator);

/** @brief Return the reverse_next element in a reverse inorder traversal of the
map. O(1).
@param[in] map a pointer to the map.
@param[in] iterator pointer to the current iterator user type.
@return a handle for the reverse_next user type stored in the map in a reverse
inorder traversal. */
[[nodiscard]] CCC_Handle_index
CCC_array_adaptive_map_reverse_next(CCC_Array_adaptive_map const *map,
                                    CCC_Handle_index iterator);

/** @brief Return the end of an inorder traversal of the map. O(1).
@param[in] map a pointer to the map.
@return a handle for the maximum element of the map. */
[[nodiscard]] CCC_Handle_index
CCC_array_adaptive_map_end(CCC_Array_adaptive_map const *map);

/** @brief Return the reverse_end of a reverse inorder traversal of the map.
O(1).
@param[in] map a pointer to the map.
@return a handle for the minimum element of the map. */
[[nodiscard]] CCC_Handle_index
CCC_array_adaptive_map_reverse_end(CCC_Array_adaptive_map const *map);

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
CCC_Result CCC_array_adaptive_map_clear(CCC_Array_adaptive_map *map,
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
CCC_Result CCC_array_adaptive_map_clear_and_free(CCC_Array_adaptive_map *map,
                                                 CCC_Type_destructor *destroy);

/** @brief Frees all slots in the array_adaptive_map and frees the underlying
Buffer that was previously dynamically reserved with the reserve function.
@param[in] map the map to be cleared.
@param[in] destroy the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the array_adaptive_map before their
slots are dropped.
@param[in] allocate the required allocation function to provide to a
dynamically reserved array_adaptive_map. Any context data provided upon
initialization will be passed to the allocation function when called.
@return the result of free operation. OK if success, or an error status to
indicate the error.
@warning It is an error to call this function on a array_adaptive_map that was
not reserved with the provided CCC_Allocator. The array_adaptive_map must have
existing memory to free.

This function covers the edge case of reserving a dynamic capacity for a
array_adaptive_map at runtime but denying the array_adaptive_map allocation
permission to resize. This can help prevent a array_adaptive_map from growing
untree. The user in this case knows the array_adaptive_map does not have
allocation permission and therefore no further memory will be dedicated to the
array_adaptive_map.

However, to free the array_adaptive_map in such a case this function must be
used because the array_adaptive_map has no ability to free itself. Just as the
allocation function is required to reserve memory so to is it required to free
memory.

This function will work normally if called on a array_adaptive_map with
allocation permission however the normal CCC_array_adaptive_map_clear_and_free
is sufficient for that use case. */
CCC_Result
CCC_array_adaptive_map_clear_and_free_reserve(CCC_Array_adaptive_map *map,
                                              CCC_Type_destructor *destroy,
                                              CCC_Allocator *allocate);

/**@}*/

/** @name State Interface
Obtain the container state. */
/**@{*/

/** @brief Returns the count of map occupied slots.
@param[in] map the map.
@return the size of the map or an argument error is set if array_adaptive_map
is NULL. */
[[nodiscard]] CCC_Count
CCC_array_adaptive_map_count(CCC_Array_adaptive_map const *map);

/** @brief Returns the capacity of the map representing total possible slots.
@param[in] map the map.
@return the capacity or an argument error is set if array_adaptive_map is NULL.
*/
[[nodiscard]] CCC_Count
CCC_array_adaptive_map_capacity(CCC_Array_adaptive_map const *map);

/** @brief Returns the size status of the map.
@param[in] map the map.
@return true if empty else false. Error if array_adaptive_map is NULL. */
[[nodiscard]] CCC_Tribool
CCC_array_adaptive_map_is_empty(CCC_Array_adaptive_map const *map);

/** @brief Validation of invariants for the map.
@param[in] map the map to validate.
@return true if all invariants hold, false if corruption occurs. Error if
array_adaptive_mape is NULL.  */
[[nodiscard]] CCC_Tribool
CCC_array_adaptive_map_validate(CCC_Array_adaptive_map const *map);

/**@}*/

/** Define this preprocessor directive if shorter names are helpful. Ensure
 no namespace clashes occur before shortening. */
#ifdef ARRAY_ADAPTIVE_MAP_USING_NAMESPACE_CCC
typedef CCC_Array_adaptive_map Array_adaptive_map;
typedef CCC_Array_adaptive_map_handle Array_adaptive_map_handle;
#    define array_adaptive_map_declare_fixed_map(args...)                      \
        CCC_array_adaptive_map_declare_fixed_map(args)
#    define array_adaptive_map_fixed_capacity(args...)                         \
        CCC_array_adaptive_map_fixed_capacity(args)
#    define array_adaptive_map_initialize(args...)                             \
        CCC_array_adaptive_map_initialize(args)
#    define array_adaptive_map_from(args...) CCC_array_adaptive_map_from(args)
#    define array_adaptive_map_with_capacity(args...)                          \
        CCC_array_adaptive_map_with_capacity(args)
#    define array_adaptive_map_at(args...) CCC_array_adaptive_map_at(args)
#    define array_adaptive_map_as(args...) CCC_array_adaptive_map_as(args)
#    define array_adaptive_map_and_modify_with(args...)                        \
        CCC_array_adaptive_map_and_modify_with(args)
#    define array_adaptive_map_or_insert_with(args...)                         \
        CCC_array_adaptive_map_or_insert_with(args)
#    define array_adaptive_map_insert_array_with(args...)                      \
        CCC_array_adaptive_map_insert_array_with(args)
#    define array_adaptive_map_try_insert_with(args...)                        \
        CCC_array_adaptive_map_try_insert_with(args)
#    define array_adaptive_map_insert_or_assign_with(args...)                  \
        CCC_array_adaptive_map_insert_or_assign_with(args)
#    define array_adaptive_map_copy(args...) CCC_array_adaptive_map_copy(args)
#    define array_adaptive_map_reserve(args...)                                \
        CCC_array_adaptive_map_reserve(args)
#    define array_adaptive_map_contains(args...)                               \
        CCC_array_adaptive_map_contains(args)
#    define array_adaptive_map_get_key_value(args...)                          \
        CCC_array_adaptive_map_get_key_value(args)
#    define array_adaptive_map_swap_handle_wrap(args...)                       \
        CCC_array_adaptive_map_swap_handle_wrap(args)
#    define array_adaptive_map_try_insert_wrap(args...)                        \
        CCC_array_adaptive_map_try_insert_wrap(args)
#    define array_adaptive_map_remove_key_value_wrap(args...)                  \
        CCC_array_adaptive_map_remove_key_value_wrap(args)
#    define array_adaptive_map_remove_handle_wrap(args...)                     \
        CCC_array_adaptive_map_remove_handle_wrap(args)
#    define array_adaptive_map_swap_handle(args...)                            \
        CCC_array_adaptive_map_swap_handle(args)
#    define array_adaptive_map_try_insert(args...)                             \
        CCC_array_adaptive_map_try_insert(args)
#    define array_adaptive_map_insert_or_assign(args...)                       \
        CCC_array_adaptive_map_insert_or_assign(args)
#    define array_adaptive_map_remove_key_value(args...)                       \
        CCC_array_adaptive_map_remove_key_value(args)
#    define array_adaptive_map_remove_handle(args...)                          \
        CCC_array_adaptive_map_remove_handle(args)
#    define array_adaptive_map_handle_wrap(args...)                            \
        CCC_array_adaptive_map_handle_wrap(args)
#    define array_adaptive_map_handle(args...)                                 \
        CCC_array_adaptive_map_handle(args)
#    define array_adaptive_map_and_modify(args...)                             \
        CCC_array_adaptive_map_and_modify(args)
#    define array_adaptive_map_and_modify_context(args...)                     \
        CCC_array_adaptive_map_and_modify_context(args)
#    define array_adaptive_map_or_insert(args...)                              \
        CCC_array_adaptive_map_or_insert(args)
#    define array_adaptive_map_insert_handle(args...)                          \
        CCC_array_adaptive_map_insert_handle(args)
#    define array_adaptive_map_unwrap(args...)                                 \
        CCC_array_adaptive_map_unwrap(args)
#    define array_adaptive_map_insert_error(args...)                           \
        CCC_array_adaptive_map_insert_error(args)
#    define array_adaptive_map_occupied(args...)                               \
        CCC_array_adaptive_map_occupied(args)
#    define array_adaptive_map_clear(args...) CCC_array_adaptive_map_clear(args)
#    define array_adaptive_map_clear_and_free(args...)                         \
        CCC_array_adaptive_map_clear_and_free(args)
#    define array_adaptive_map_clear_and_free_reserve(args...)                 \
        CCC_array_adaptive_map_clear_and_free_reserve(args)
#    define array_adaptive_map_begin(args...) CCC_array_adaptive_map_begin(args)
#    define array_adaptive_map_reverse_begin(args...)                          \
        CCC_array_adaptive_map_reverse_begin(args)
#    define array_adaptive_map_end(args...) CCC_array_adaptive_map_end(args)
#    define array_adaptive_map_reverse_end(args...)                            \
        CCC_array_adaptive_map_reverse_end(args)
#    define array_adaptive_map_next(args...) CCC_array_adaptive_map_next(args)
#    define array_adaptive_map_reverse_next(args...)                           \
        CCC_array_adaptive_map_reverse_next(args)
#    define array_adaptive_map_count(args...) CCC_array_adaptive_map_count(args)
#    define array_adaptive_map_capacity(args...)                               \
        CCC_array_adaptive_map_capacity(args)
#    define array_adaptive_map_is_empty(args...)                               \
        CCC_array_adaptive_map_is_empty(args)
#    define array_adaptive_map_validate(args...)                               \
        CCC_array_adaptive_map_validate(args)
#endif /* ARRAY_ADAPTIVE_MAP_USING_NAMESPACE_CCC */

#endif /* CCC_ARRAY_ADAPTIVE_MAP_H */
