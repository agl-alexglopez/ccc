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
@brief The Flat Hash Map Interface

A Flat Hash Map stores elements in a contiguous buffer and allows the user to
retrieve them by key in amortized O(1). Elements in the table may be copied and
moved, especially when rehashing occurs, so no pointer stability is available in
this implementation.

A flat hash map requires the user to provide a pointer to the map, a type, a key
field, a hash function, and a key three way comparator function. The hasher
should be well tailored to the key being stored in the table to prevent
collisions. Good variety in the upper bits of the hashed value will also result
in faster performance. Currently, the flat hash map does not offer any default
hash functions or hash strengthening algorithms so strong hash functions should
be obtained by the user for the data set.

The current implementation will seek to use the best platform specific SIMD or
SRMD instructions available. However, if for any reason the user wishes to use
the most portable Single Register Multiple Data fallback implementation, there
are many options.

If building this library separately to include it's library file, add the
flag to the build (and read INSTALL.md for more details).

```
cmake --preset=clang-release -DCCC_FLAT_HASH_MAP_PORTABLE
```

If an install location other than the release folder is desired don't forget
to add the install prefix.

```
cmake --preset=clang-release -DCCC_FLAT_HASH_MAP_PORTABLE
-DCMAKE_INSTALL_PREFIX=/my/path/
```

If this library is being built as part of your project then define the flag
as part of your configuration.

```
cmake --preset=my-preset -DCCC_FLAT_HASH_MAP_PORTABLE
```

Or, add the flag to your `CMakePresets.json`.

```
"cacheVariables": {
    "CCC_FLAT_HASH_MAP_PORTABLE": "ON"
}
```

Or, enable on a per target basis in your `CMakeLists.txt`.

```
target_compile_definitions(my_target PRIVATE CCC_FLAT_HASH_MAP_PORTABLE)
```

Or finally, just define it before including the flat hash map header.

```
#define CCC_FLAT_HASH_MAP_PORTABLE
#include "ccc/flat_hash_map.h"
```

To shorten names in the interface, define the following preprocessor directive
at the top of your file. The `CCC_` prefix may then be omitted for only this
container.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#include "ccc/flat_hash_map.h"
```

All types and functions can then be written without the `CCC_` prefix. */
#ifndef CCC_FLAT_HASH_MAP_H
#define CCC_FLAT_HASH_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "private/private_flat_hash_map.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A container for storing key-value structures defined by the user in
a contiguous buffer.

A flat hash map can be initialized on the stack, heap, or data segment at
runtime or compile time. */
typedef struct CCC_Flat_hash_map CCC_Flat_hash_map;

/** @brief A container specific entry used to implement the Entry Interface.

The Entry Interface offers efficient search and subsequent insertion, deletion,
or value update based on the needs of the user. */
typedef union CCC_Flat_hash_map_entry_wrap CCC_Flat_hash_map_entry;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. When a fixed
size map is required that will not have allocation permission, the user must
declare the type name and size of the map they will use. */
/**@{*/

/** @brief Declare a fixed size map type for use in the stack, heap, or data
segment. Does not return a value.
@param[in] fixed_map_type_name the user chosen name of the fixed sized map.
@param[in] type_name the type the user plans to store in the map. It
may have a key and value field as well as any additional fields. For set-like
behavior, wrap a field in a struct/union (e.g. `union int_node {int e;};`).
@param[in] capacity the power of two capacity for the map.
@warning the capacity must be a power of two greater than 8 or 16, depending on
the platform (e.g. 16, 32, 64, etc.).

Once the location for the fixed size map is chosen--stack, heap, or data
segment--provide a pointer to the map for the initialization macro.

```
struct Val
{
    int key;
    int val;
};
CCC_flat_hash_map_declare_fixed_map(Small_fixed_map, struct Val, 64);
static Flat_hash_map static_map = flat_hash_map_initialize(
    &(static Small_fixed_map){},
    struct Val,
    key,
    Flat_hash_map_int_to_u64,
    flat_hash_map_id_order,
    NULL,
    NULL,
    flat_hash_map_fixed_capacity(Small_fixed_map)
);
```

Similarly, a fixed size map can be used on the stack.

```
struct Val
{
    int key;
    int val;
};
CCC_flat_hash_map_declare_fixed_map(Small_fixed_map, struct Val, 64);
int main(void)
{
    Flat_hash_map static_map = flat_hash_map_initialize(
        &(Small_fixed_map){},
        struct Val,
        key,
        flat_hash_map_int_to_u64,
        flat_hash_map_id_order,
        NULL,
        NULL,
        flat_hash_map_fixed_capacity(Small_fixed_map)
    );
    return 0;
}
```

The CCC_flat_hash_map_fixed_capacity macro can be used to obtain the previously
provided capacity when declaring the fixed map type. Finally, one could allocate
a fixed size map on the heap; however, it is usually better to initialize a
dynamic map and use the CCC_flat_hash_map_reserve function for such a use case.

This macro is not needed when a dynamic resizing flat hash map is needed. For
dynamic maps, simply pass NULL and 0 capacity to the initialization macro. */
#define CCC_flat_hash_map_declare_fixed_map(fixed_map_type_name, type_name,    \
                                            capacity)                          \
    CCC_private_flat_hash_map_declare_fixed_map(fixed_map_type_name,           \
                                                type_name, capacity)

/** @brief Obtain the capacity previously chosen for the fixed size map type.
@param[in] fixed_map_type_name the name of a previously declared map.
@return the size_t capacity. This is the full capacity without any load factor
restrictions. */
#define CCC_flat_hash_map_fixed_capacity(fixed_map_type_name)                  \
    CCC_private_flat_hash_map_fixed_capacity(fixed_map_type_name)

/** @brief Initialize a map with a Buffer of types at compile time or runtime.
@param[in] map_pointer a pointer to a fixed map allocation or NULL.
@param[in] type_name the name of the user defined type stored in the map.
@param[in] key_field the field of the struct used for key storage.
@param[in] hash the CCC_Key_hasher function provided by the user.
@param[in] compare the CCC_Key_comparator the user intends to
use.
@param[in] allocate the allocation function for resizing or NULL if no
resizing is allowed.
@param[in] context_data context data that is needed for hashing or comparison.
@param[in] capacity the capacity of a fixed size map or 0.
@return the flat hash map directly initialized on the right hand side of the
equality operator (i.e. CCC_Flat_hash_map map =
CCC_flat_hash_map_initialize(...);)
@note if a dynamic resizing map is required provide NULL as the map_pointer.

Initialize a static fixed size hash map at compile time that has
no allocation permission or context data needed.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
flat_hash_map_declare_fixed_map(Small_fixed_map, struct Val, 64);
static Flat_hash_map static_map = flat_hash_map_initialize(
    &(static Small_fixed_map){},
    struct Val,
    key,
    flat_hash_map_int_to_u64,
    flat_hash_map_key_order,
    NULL,
    NULL,
    flat_hash_map_fixed_capacity(Small_fixed_map)
);
```

Initialize a dynamic hash table at compile time with allocation permission and
no context data. Use the same type as the previous example.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
static Flat_hash_map static_map = flat_hash_map_initialize(
    NULL,
    struct Val,
    key,
    flat_hash_map_int_to_u64,
    flat_hash_map_key_order,
    std_allocate,
    NULL,
    0
);
```

Initialization at runtime is also possible. Stack-based or dynamic maps are
identical to the provided examples. Omit `static` in a runtime context. */
#define CCC_flat_hash_map_initialize(map_pointer, type_name, key_field, hash,  \
                                     compare, allocate, context_data,          \
                                     capacity)                                 \
    CCC_private_flat_hash_map_initialize(map_pointer, type_name, key_field,    \
                                         hash, compare, allocate,              \
                                         context_data, capacity)

/** @brief Initialize a dynamic map at runtime from an initializer list.
@param[in] key_field the field of the struct used for key storage.
@param[in] hash the CCC_Key_hasher function provided by the user.
@param[in] compare the CCC_Key_comparator the user intends to
use.
@param[in] allocate the required allocation function.
@param[in] context_data context data that is needed for hashing or comparison.
@param[in] optional_capacity optionally specify the capacity of the map if
different from the size of the compound literal array initializer. If the
capacity is greater than the size of the compound literal array initializer, it
is respected and the capacity is reserved. If the capacity is less than the size
of the compound array initializer, the compound literal array initializer size
is set as the capacity. Therefore, 0 is valid if one is not concerned with the
size of the underlying reservation.
@param[in] array_compound_literal a list of key value pairs of the type
intended to be stored in the map, using array compound literal initialization
syntax (e.g `(struct My_type[]){{.k = 0, .v 0}, {.k = 1, .v = 1}}`).
@return the flat hash map directly initialized on the right hand side of the
equality operator (i.e. CCC_Flat_hash_map map = CCC_flat_hash_map_from(...);)
@warning An allocation function is required. This initializer is only available
for dynamic maps.
@warning When duplicate keys appear in the initializer list, the last occurrence
replaces earlier ones by value (all fields are overwritten).
@warning If initialization fails, the map will be returned empty. All subsequent
queries, insertions, or removals will indicate the error: either memory related
or lack of an allocation function provided.

Initialize a dynamic hash table at run time. This example requires no context
data for initialization.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
int
main(void)
{
    Flat_hash_map map = flat_hash_map_from(
        key,
        flat_hash_map_int_to_u64,
        flat_hash_map_key_order,
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

Only dynamic maps may be initialized this way due the inability of the hash
map to protect its invariants from user error at compile time. */
#define CCC_flat_hash_map_from(key_field, hash, compare, allocate,             \
                               context_data, optional_capacity,                \
                               array_compound_literal...)                      \
    CCC_private_flat_hash_map_from(key_field, hash, compare, allocate,         \
                                   context_data, optional_capacity,            \
                                   array_compound_literal)

/** @brief Initialize a dynamic map at runtime with at least the specified
capacity.
@param[in] type_name the name of the type being stored in the map.
@param[in] key_field the field of the struct used for key storage.
@param[in] hash the CCC_Key_hasher function provided by the user.
@param[in] compare the CCC_Key_comparator the user intends to
use.
@param[in] allocate the required allocation function.
@param[in] context_data context data that is needed for hashing or comparison.
@param[in] capacity the desired capacity for the map. A capacity of 0 results
in an argument error and is a no-op after the map is initialized empty.
@return the flat hash map directly initialized on the right hand side of the
equality operator (i.e. CCC_Flat_hash_map map =
CCC_flat_hash_map_with_capacity(...);)
@warning An allocation function is required. This initializer is only available
for dynamic maps.
@warning If initialization fails all subsequent queries, insertions, or
removals will indicate the error: either memory related or lack of an
allocation function provided.

Initialize a dynamic hash table at run time. This example requires no context
data for initialization.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
int
main(void)
{
    Flat_hash_map map = flat_hash_map_with_capacity(
        struct Val,
        key,
        flat_hash_map_int_to_u64,
        flat_hash_map_key_order,
        std_allocate,
        NULL,
        4096
    );
    return 0;
}
```

Only dynamic maps may be initialized this way as it simply combines the steps
of initialization and reservation. */
#define CCC_flat_hash_map_with_capacity(type_name, key_field, hash, compare,   \
                                        allocate, context_data, capacity)      \
    CCC_private_flat_hash_map_with_capacity(                                   \
        type_name, key_field, hash, compare, allocate, context_data, capacity)

/** @brief Copy the map at source to destination.
@param[in] destination the initialized destination for the copy of the source
map.
@param[in] source the initialized source of the map.
@param[in] allocate the optional allocation function if resizing is needed.
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
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
flat_hash_map_declare_fixed_map(Small_fixed_map, struct Val, 64);
Flat_hash_map source = flat_hash_map_initialize(
    &(static Small_fixed_map){},
    struct Val,
    key,
    flat_hash_map_int_to_u64,
    flat_hash_map_key_order,
    NULL,
    NULL,
    CCC_flat_hash_map_fixed_capacity(Small_fixed_map)
);
insert_rand_vals(&source);
Flat_hash_map destination = flat_hash_map_initialize(
    &(static Small_fixed_map){},
    struct Val,
    key,
    flat_hash_map_int_to_u64,
    flat_hash_map_key_order,
    NULL,
    NULL,
    CCC_flat_hash_map_fixed_capacity(Small_fixed_map)
);
CCC_Result res = flat_hash_map_copy(&destination, &source, NULL);
```

The above requires destination capacity be greater than or equal to source
capacity. Here is memory management handed over to the copy function.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
Flat_hash_map source = flat_hash_map_initialize(
    NULL,
    struct Val,
    key,
    flat_hash_map_int_to_u64,
    flat_hash_map_key_order,
    std_allocate,
    NULL,
    0
);
insert_rand_vals(&source);
Flat_hash_map destination = flat_hash_map_initialize(
    NULL,
    struct Val,
    key,
    flat_hash_map_int_to_u64,
    flat_hash_map_key_order,
    std_allocate,
    NULL,
    0
);
CCC_Result res = flat_hash_map_copy(&destination, &source, std_allocate);
```

The above allows destination to have a capacity less than that of the source as
long as copy has been provided an allocation function to resize destination.
Note that this would still work if copying to a destination that the user wants
as a fixed size map.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
Flat_hash_map source = flat_hash_map_initialize(
    NULL,
    struct Val,
    key,
    flat_hash_map_int_to_u64,
    flat_hash_map_key_order,
    std_allocate,
    NULL,
    0
);
insert_rand_vals(&source);
Flat_hash_map destination = flat_hash_map_initialize(
    NULL,
    struct Val,
    key,
    flat_hash_map_int_to_u64,
    flat_hash_map_key_order,
    NULL,
    NULL,
    0
);
CCC_Result res = flat_hash_map_copy(&destination, &source, std_allocate);
```

The above sets up destination with fixed size while source is a dynamic map.
Because an allocation function is provided, the destination is resized once for
the copy and retains its fixed size after the copy is complete. This would
require the user to manually free the underlying Buffer at destination
eventually if this method is used. Usually it is better to allocate the memory
with the reserve function if maps with one-time allocation requirements are
used.

These options allow users to stay consistent across containers with their
memory management strategies. */
CCC_Result CCC_flat_hash_map_copy(CCC_Flat_hash_map *destination,
                                  CCC_Flat_hash_map const *source,
                                  CCC_Allocator *allocate);

/** @brief Reserve space required to add a specified number of elements to the
map. If the current capacity is sufficient, do nothing.
@param[in] map a pointer to the hash map.
@param[in] to_add the number of elements to add to the map.
@param[in] allocate the required allocation function that can be used for
resizing. Such a function is provided to cover the case where the user wants a
fixed size map but cannot know the size needed until runtime. In this case, the
function needs to be provided to allow the single resizing to occur. Any context
data provided upon initialization will be passed to the allocation function when
called.
@return the result of the reserving operation, OK if successful or an error
code to indicate the specific failure.

If the map has already been initialized with allocation permission simply
provide the same function that was passed upon initialization. */
CCC_Result CCC_flat_hash_map_reserve(CCC_Flat_hash_map *map, size_t to_add,
                                     CCC_Allocator *allocate);

/**@}*/

/**@name Membership Interface
Test membership or obtain references to stored user types directly. */
/**@{*/

/** @brief Searches the table for the presence of key.
@param[in] map the flat hash table to be searched.
@param[in] key pointer to the key matching the key type of the user struct.
@return true if the struct containing key is stored, false if not. Error if map
or key is NULL. */
[[nodiscard]] CCC_Tribool
CCC_flat_hash_map_contains(CCC_Flat_hash_map const *map, void const *key);

/** @brief Returns a reference into the table at entry key.
@param[in] map the flat hash map to search.
@param[in] key the key to search matching stored key type.
@return a view of the table entry if it is present, else NULL. */
[[nodiscard]] void *
CCC_flat_hash_map_get_key_value(CCC_Flat_hash_map const *map, void const *key);

/**@}*/

/** @name Entry Interface
Obtain and operate on container entries for efficient queries when non-trivial
control flow is needed. */
/**@{*/

/** @brief Obtains an entry for the provided key in the table for future use.
@param[in] map the hash table to be searched.
@param[in] key the key used to search the table matching the stored key type.
@return a specialized hash entry for use with other functions in the Entry
Interface.
@warning the contents of an entry should not be examined or modified. Use the
provided functions, only.

An entry is a search result that provides either an Occupied or Vacant entry
in the table. An occupied entry signifies that the search was successful. A
Vacant entry means the search was not successful but we now have a handle to
where in the table such an element should be inserted.

An entry is rarely useful on its own. It should be passed in a functional style
to subsequent calls in the Entry Interface.*/
[[nodiscard]] CCC_Flat_hash_map_entry
CCC_flat_hash_map_entry(CCC_Flat_hash_map *map, void const *key);

/** @brief Obtains an entry for the provided key in the table for future use.
@param[in] map_pointer the hash table to be searched.
@param[in] key_pointer the key used to search the table matching the stored key
type.
@return a compound literal reference to a specialized hash entry for use with
other functions in the Entry Interface.
@warning the contents of an entry should not be examined or modified. Use the
provided functions, only.

An entry is a search result that provides either an Occupied or Vacant entry
in the table. An occupied entry signifies that the search was successful. A
Vacant entry means the search was not successful but we now have a handle to
where in the table such an element should be inserted.

An entry is most often passed in a functional style to subsequent calls in the
Entry Interface.*/
#define CCC_flat_hash_map_entry_wrap(map_pointer, key_pointer)                 \
    &(CCC_Flat_hash_map_entry)                                                 \
    {                                                                          \
        CCC_flat_hash_map_entry(map_pointer, key_pointer).private              \
    }

/** @brief Modifies the provided entry if it is Occupied.
@param[in] entry the entry obtained from an entry function or macro.
@param[in] modify an update function in which the context argument is unused.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function is intended to make the function chaining in the Entry Interface
more succinct if the entry will be modified in place based on its own value
without the need of the context argument a CCC_Type_modifier can provide.
*/
[[nodiscard]] CCC_Flat_hash_map_entry *
CCC_flat_hash_map_and_modify(CCC_Flat_hash_map_entry *entry,
                             CCC_Type_modifier *modify);

/** @brief Modifies the provided entry if it is Occupied.
@param[in] entry the entry obtained from an entry function or macro.
@param[in] modify an update function that requires context data.
@param[in] context context data required for the update.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function makes full use of a CCC_Type_modifier capability, meaning a
complete CCC_update object will be passed to the update function callback. */
[[nodiscard]] CCC_Flat_hash_map_entry *
CCC_flat_hash_map_and_modify_context(CCC_Flat_hash_map_entry *entry,
                                     CCC_Type_modifier *modify, void *context);

/** @brief Modify an Occupied entry with a closure over user type T.
@param[in] map_entry_pointer a pointer to the obtained entry.
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
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
// Increment the count if found otherwise do nothing.
Flat_hash_map_entry *entry =
    flat_hash_map_and_modify_with(
        entry_wrap(&map, &k),
        Word,
        T->cnt++;
    );
// Increment the count if found otherwise insert a default value.
Word *w =
    flat_hash_map_or_insert_with(
        flat_hash_map_and_modify_with(
            entry_wrap(&flat_hash_map, &k),
            Word,
            { T->cnt++; }
        ),
        (Word){.key = k, .cnt = 1}
    );
```

Note that any code written is only evaluated if the entry is Occupied and the
container can deliver the user type T. This means any function calls are lazily
evaluated in the closure scope. */
#define CCC_flat_hash_map_and_modify_with(map_entry_pointer, type_name,        \
                                          closure_over_T...)                   \
    &(CCC_Flat_hash_map_entry)                                                 \
    {                                                                          \
        CCC_private_flat_hash_map_and_modify_with(map_entry_pointer,           \
                                                  type_name, closure_over_T)   \
    }

/** @brief Inserts the struct with handle elem if the entry is Vacant.
@param[in] entry the entry obtained via function or macro call.
@param[in] type the complete key and value type to be inserted.
@return a pointer to entry in the table invariantly. NULL on error.

Because this functions takes an entry and inserts if it is Vacant, the only
reason NULL shall be returned is when an insertion error will occur, usually
due to a resizing memory error. This can happen if the table is not allowed
to resize because no allocation function is provided. */
[[nodiscard]] void *
CCC_flat_hash_map_or_insert(CCC_Flat_hash_map_entry const *entry,
                            void const *type);

/** @brief lazily insert the desired key value into the entry if it is Vacant.
@param[in] map_entry_pointer a pointer to the obtained entry.
@param[in] type_compound_literal the compound literal to construct in place if
the entry is Vacant.
@return a reference to the unwrapped user type in the entry, either the
unmodified reference if the entry was Occupied or the newly inserted element
if the entry was Vacant. NULL is returned if resizing is required but fails or
is not allowed.

Note that if the compound literal uses any function calls to generate values
or other data, such functions will not be called if the entry is Occupied. */
#define CCC_flat_hash_map_or_insert_with(map_entry_pointer,                    \
                                         type_compound_literal...)             \
    CCC_private_flat_hash_map_or_insert_with(map_entry_pointer,                \
                                             type_compound_literal)

/** @brief Inserts the provided entry invariantly.
@param[in] entry the entry returned from a call obtaining an entry.
@param[in] type the complete key and value type to be inserted.
@return a pointer to the inserted element or NULL upon a memory error in which
the load factor would be exceeded when no allocation policy is defined or
resizing failed to find more memory.

This method can be used when the old value in the table does not need to
be preserved. See the regular insert method if the old value is of interest.
If an error occurs during the insertion process due to memory limitations
or a search error NULL is returned. Otherwise insertion should not fail. */
[[nodiscard]] void *
CCC_flat_hash_map_insert_entry(CCC_Flat_hash_map_entry const *entry,
                               void const *type);

/** @brief write the contents of the compound literal type_compound_literal to a
slot.
@param[in] map_entry_pointer a pointer to the obtained entry.
@param[in] type_compound_literal the compound literal to write to a new slot.
@return a reference to the newly inserted or overwritten user type. NULL is
returned if resizing is required but fails or is not allowed. */
#define CCC_flat_hash_map_insert_entry_with(map_entry_pointer,                 \
                                            type_compound_literal...)          \
    CCC_private_flat_hash_map_insert_entry_with(map_entry_pointer,             \
                                                type_compound_literal)

/** @brief Invariantly inserts the key value wrapping out_handle.
@param[in] map the pointer to the flat hash map.
@param[out] type_output the complete key and value type to be inserted.
@return an entry. If Vacant, no prior element with key existed and entry may be
unwrapped to view the new insertion in the table. If Occupied the old value is
written to type_output and may be unwrapped to view. If more space is
needed but allocation fails or has been forbidden, an insert error is set.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value. */
[[nodiscard]]
CCC_Entry CCC_flat_hash_map_swap_entry(CCC_Flat_hash_map *map,
                                       void *type_output);

/** @brief Invariantly inserts the key value wrapping out_handle.
@param[in] map_pointer the pointer to the flat hash map.
@param[out] type_pointer the complete key and value type to be
inserted.
@return a compound literal reference to an entry. If Vacant, no prior element
with key existed and entry may be unwrapped to view the new insertion in the
table. If Occupied the old value is written to type_output and may be
unwrapped to view. If more space is needed but allocation fails or has been
forbidden, an insert error is set.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value. */
#define CCC_flat_hash_map_swap_entry_wrap(map_pointer, type_pointer)           \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_flat_hash_map_swap_entry(map_pointer, type_pointer).private        \
    }

/** @brief Remove the entry from the table if Occupied.
@param[in] entry a pointer to the table entry.
@return an entry containing NULL. If Occupied an entry in the table existed and
was removed. If Vacant, no prior entry existed to be removed. */
[[nodiscard]] CCC_Entry
CCC_flat_hash_map_remove_entry(CCC_Flat_hash_map_entry const *entry);

/** @brief Remove the entry from the table if Occupied.
@param[in] map_entry_pointer a pointer to the table entry.
@return a compound liter to an entry containing NULL. If Occupied an entry in
the table existed and was removed. If Vacant, no prior entry existed to be
removed. */
#define CCC_flat_hash_map_remove_entry_wrap(map_entry_pointer)                 \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_flat_hash_map_remove_entry(map_entry_pointer).private              \
    }

/** @brief Attempts to insert the key value wrapping key_val_handle
@param[in] map the pointer to the flat hash map.
@param[in] type the complete key and value type to be inserted.
@return an entry. If Occupied, the entry contains a reference to the key value
user type in the table and may be unwrapped. If Vacant the entry contains a
reference to the newly inserted entry in the table. If more space is needed but
allocation fails or has been forbidden, an insert error is set.
@warning because this function returns a reference to a user type in the table
any subsequent insertions or deletions invalidate this reference. */
[[nodiscard]]
CCC_Entry CCC_flat_hash_map_try_insert(CCC_Flat_hash_map *map,
                                       void const *type);

/** @brief Attempts to insert the key value wrapping key_val_array_pointer.
@param[in] map_pointer the pointer to the flat hash map.
@param[in] type_pointer the complete key and value type to be inserted.
@return a compound literal reference to the entry. If Occupied, the entry
contains a reference to the key value user type in the table and may be
unwrapped. If Vacant the entry contains a reference to the newly inserted
entry in the table. If more space is needed but allocation fails or has been
forbidden, an insert error is set.
@warning because this function returns a reference to a user type in the table
any subsequent insertions or deletions invalidate this reference. */
#define CCC_flat_hash_map_try_insert_wrap(map_pointer, type_pointer)           \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_flat_hash_map_try_insert(map_pointer, type_pointer).private        \
    }

/** @brief lazily insert type_compound_literal into the map at key if key is
absent.
@param[in] map_pointer a pointer to the flat hash map.
@param[in] key the direct key r-value.
@param[in] type_compound_literal the compound literal specifying the value.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.
@warning ensure the key type matches the type stored in table as your key. For
example, if your key is of type `int` and you pass a `size_t` variable to the
key argument, you will overwrite adjacent bytes of your struct.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define CCC_flat_hash_map_try_insert_with(map_pointer, key,                    \
                                          type_compound_literal...)            \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_private_flat_hash_map_try_insert_with(map_pointer, key,            \
                                                  type_compound_literal)       \
    }

/** @brief Invariantly inserts or overwrites a user struct into the table.
@param[in] map a pointer to the flat hash map.
@param[in] type the complete key and value type to be inserted.
@return an entry. If Occupied an entry was overwritten by the new key value. If
Vacant no prior table entry existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
[[nodiscard]]
CCC_Entry CCC_flat_hash_map_insert_or_assign(CCC_Flat_hash_map *map,
                                             void const *type);

/** @brief Invariantly inserts or overwrites a user struct into the table.
@param[in] map_pointer a pointer to the flat hash map.
@param[in] type_pointer the complete key and value type to be inserted.
@return a compound literal reference to the entry. If Occupied an entry was
overwritten by the new key value. If Vacant no prior table entry existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
#define CCC_flat_hash_map_insert_or_assign_wrap(map_pointer, type_pointer)     \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_flat_hash_map_insert_or_assign(map_pointer, type_pointer).private  \
    }

/** @brief Inserts a new key value pair or overwrites the existing entry.
@param[in] map_pointer the pointer to the flat hash map.
@param[in] key the key to be searched in the table.
@param[in] type_compound_literal the compound literal specifying the value.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define CCC_flat_hash_map_insert_or_assign_with(map_pointer, key,              \
                                                type_compound_literal...)      \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_private_flat_hash_map_insert_or_assign_with(map_pointer, key,      \
                                                        type_compound_literal) \
    }

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing out_handle provided by the user.
@param[in] map the pointer to the flat hash map.
@param[out] type_output the complete key and value type to be removed
@return the removed entry. If Occupied it may be unwrapped to obtain the old key
value pair. If Vacant the key value pair was not stored in the map. If bad input
is provided an input error is set. If a previously stored value is returned it
is safe to keep and modify this reference because the data has been written
to the provided space.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value. */
[[nodiscard]] CCC_Entry CCC_flat_hash_map_remove(CCC_Flat_hash_map *map,
                                                 void *type_output);

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing out_array_pointer provided by the user.
@param[in] map_pointer the pointer to the flat hash map.
@param[out] type_output_pointer the complete key and value type to be
removed
@return a compound literal reference to the removed entry. If Occupied it may
be unwrapped to obtain the old key value pair. If Vacant the key value pair
was not stored in the map. If bad input is provided an input error is set. If a
previously stored value is returned it is safe to keep and modify this reference
because the data has been written to the provided space.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value. */
#define CCC_flat_hash_map_remove_wrap(map_pointer, type_output_pointer)        \
    &(CCC_Entry)                                                               \
    {                                                                          \
        CCC_flat_hash_map_remove(map_pointer, type_output_pointer).private     \
    }

/** @brief Unwraps the provided entry to obtain a view into the table element.
@param[in] entry the entry from a query to the table via function or macro.
@return an view into the table entry if one is present, or NULL. */
[[nodiscard]] void *
CCC_flat_hash_map_unwrap(CCC_Flat_hash_map_entry const *entry);

/** @brief Returns the Vacant or Occupied status of the entry.
@param[in] entry the entry from a query to the table via function or macro.
@return true if the entry is occupied, false if not. Error if entry is NULL. */
[[nodiscard]] CCC_Tribool
CCC_flat_hash_map_occupied(CCC_Flat_hash_map_entry const *entry);

/** @brief Provides the status of the entry should an insertion follow.
@param[in] entry the entry from a query to the table via function or macro.
@return true if the next insertion of a new element will cause an error. Error
if entry is null.

Table resizing occurs upon calls to entry functions/macros or when trying
to insert a new element directly. This is to provide stable entries from the
time they are obtained to the time they are used in functions they are passed
to (i.e. the idiomatic or_insert(entry(...)...)).

However, if a Vacant entry is returned and then a subsequent insertion function
is used, it will not work if resizing has failed and the return of those
functions will indicate such a failure. One can also confirm an insertion error
will occur from an entry with this function. For example, leaving this function
in an assert for debug builds can be a helpful sanity check. */
[[nodiscard]] CCC_Tribool
CCC_flat_hash_map_insert_error(CCC_Flat_hash_map_entry const *entry);

/** @brief Obtain the entry status from a container entry.
@param[in] entry a pointer to the entry.
@return the status stored in the entry after the required action on the
container completes. If entry is NULL an entry input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned.

Note that this function can be useful for debugging or if more detailed
messages are needed for logging purposes. See CCC_entry_status_message() in
ccc/types.h for more information on detailed entry statuses. */
[[nodiscard]] CCC_Entry_status
CCC_flat_hash_map_entry_status(CCC_Flat_hash_map_entry const *entry);

/**@}*/

/** @name Deallocation Interface
Destroy the container. */
/**@{*/

/** @brief Frees all slots in the table for use without affecting capacity.
@param[in] map the table to be cleared.
@param[in] destroy the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the table before their slots are
forfeit.

If NULL is passed as the destructor function time is O(1), else O(capacity). */
CCC_Result CCC_flat_hash_map_clear(CCC_Flat_hash_map *map,
                                   CCC_Type_destructor *destroy);

/** @brief Frees all slots in the table and frees the underlying buffer.
@param[in] map the table to be cleared.
@param[in] destroy the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the table before their slots are
forfeit.
@return the result of free operation. If no allocate function is provided it is
an error to attempt to free the Buffer and a memory error is returned.
Otherwise, an OK result is returned. */
CCC_Result CCC_flat_hash_map_clear_and_free(CCC_Flat_hash_map *map,
                                            CCC_Type_destructor *destroy);

/** @brief Frees all slots in the table and frees the underlying Buffer that was
previously dynamically reserved with the reserve function.
@param[in] map the table to be cleared.
@param[in] destroy the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the table before their slots are
forfeit.
@param[in] allocate the required allocation function to provide to a
dynamically reserved map. Any context data provided upon initialization will be
passed to the allocation function when called.
@return the result of free operation. CCC_RESULT_OK if success, or an error
status to indicate the error.
@warning It is an error to call this function on a map that was not reserved
with the provided CCC_Allocator. The map must have existing memory to free.

This function covers the edge case of reserving a dynamic capacity for a map
at runtime but denying the map allocation permission to resize. This can help
prevent a map from growing unbounded due to internal decisions about rehashes
and resizing. The user in this case knows the map does not have allocation
permission and therefore no further memory will be dedicated to the map.

However, to free the map in such a case this function must be used because the
map has no ability to free itself. Just as the allocation function is required
to reserve memory so to is it required to free memory.

This function will work normally if called on a map with allocation permission
however the normal CCC_flat_hash_map_clear_and_free is sufficient for that use
case. */
CCC_Result
CCC_flat_hash_map_clear_and_free_reserve(CCC_Flat_hash_map *map,
                                         CCC_Type_destructor *destroy,
                                         CCC_Allocator *allocate);

/**@}*/

/** @name Iterator Interface
Obtain and manage iterators over the container. */
/**@{*/

/** @brief Obtains a pointer to the first element in the table.
@param[in] map the table to iterate through.
@return a pointer that can be cast directly to the user type that is stored.
@warning erasing or inserting during iteration may invalidate iterators if
resizing occurs which would lead to undefined behavior. O(capacity).

Iteration starts from index 0 by capacity of the table so iteration order is
not obvious to the user, nor should any specific order be relied on. */
[[nodiscard]] void *CCC_flat_hash_map_begin(CCC_Flat_hash_map const *map);

/** @brief Advances the iterator to the next occupied table slot.
@param[in] map the table being iterated upon.
@param[in] type_iterator the previous iterator.
@return a pointer that can be cast directly to the user type that is stored.
@warning erasing or inserting during iteration may invalidate iterators if
resizing occurs which would lead to undefined behavior. O(capacity). */
[[nodiscard]] void *CCC_flat_hash_map_next(CCC_Flat_hash_map const *map,
                                           void const *type_iterator);

/** @brief Check the current iterator against the end for loop termination.
@param[in] map the table being iterated upon.
@return the end address of the hash table.
@warning It is undefined behavior to access or modify the end address. */
[[nodiscard]] void *CCC_flat_hash_map_end(CCC_Flat_hash_map const *map);

/**@}*/

/** @name State Interface
Obtain the container state. */
/**@{*/

/** @brief Returns the size status of the table.
@param[in] map the hash table.
@return true if empty else false. Error if map is NULL. */
[[nodiscard]] CCC_Tribool
CCC_flat_hash_map_is_empty(CCC_Flat_hash_map const *map);

/** @brief Returns the count of table occupied slots.
@param[in] map the hash table.
@return the size of the map or an argument error is set if map is NULL. */
[[nodiscard]] CCC_Count CCC_flat_hash_map_count(CCC_Flat_hash_map const *map);

/** @brief Return the full capacity of the backing storage.
@param[in] map the hash table.
@return the capacity of the map or an argument error is set if map is NULL. */
[[nodiscard]] CCC_Count
CCC_flat_hash_map_capacity(CCC_Flat_hash_map const *map);

/** @brief Validation of invariants for the hash table.
@param[in] map the table to validate.
@return true if all invariants hold, false if corruption occurs. Error if map is
NULL. */
[[nodiscard]] CCC_Tribool
CCC_flat_hash_map_validate(CCC_Flat_hash_map const *map);

/**@}*/

/** Define this preprocessor directive if shorter names are helpful. Ensure
 no namespace clashes occur before shortening. */
#ifdef FLAT_HASH_MAP_USING_NAMESPACE_CCC
typedef CCC_Flat_hash_map Flat_hash_map;
typedef CCC_Flat_hash_map_entry Flat_hash_map_entry;
#    define flat_hash_map_declare_fixed_map(args...)                           \
        CCC_flat_hash_map_declare_fixed_map(args)
#    define flat_hash_map_fixed_capacity(args...)                              \
        CCC_flat_hash_map_fixed_capacity(args)
#    define flat_hash_map_reserve(args...) CCC_flat_hash_map_reserve(args)
#    define flat_hash_map_initialize(args...) CCC_flat_hash_map_initialize(args)
#    define flat_hash_map_from(args...) CCC_flat_hash_map_from(args)
#    define flat_hash_map_with_capacity(args...)                               \
        CCC_flat_hash_map_with_capacity(args)
#    define flat_hash_map_copy(args...) CCC_flat_hash_map_copy(args)
#    define flat_hash_map_and_modify_with(args...)                             \
        CCC_flat_hash_map_and_modify_with(args)
#    define flat_hash_map_or_insert_with(args...)                              \
        CCC_flat_hash_map_or_insert_with(args)
#    define flat_hash_map_insert_entry_with(args...)                           \
        CCC_flat_hash_map_insert_entry_with(args)
#    define flat_hash_map_try_insert_with(args...)                             \
        CCC_flat_hash_map_try_insert_with(args)
#    define flat_hash_map_insert_or_assign_with(args...)                       \
        CCC_flat_hash_map_insert_or_assign_with(args)
#    define flat_hash_map_contains(args...) CCC_flat_hash_map_contains(args)
#    define flat_hash_map_get_key_value(args...)                               \
        CCC_flat_hash_map_get_key_value(args)
#    define flat_hash_map_remove_wrap(args...)                                 \
        CCC_flat_hash_map_remove_wrap(args)
#    define flat_hash_map_swap_entry_wrap(args...)                             \
        CCC_flat_hash_map_swap_entry_wrap(args)
#    define flat_hash_map_try_insert_wrap(args...)                             \
        CCC_flat_hash_map_try_insert_wrap(args)
#    define flat_hash_map_insert_or_assign_wrap(args...)                       \
        CCC_flat_hash_map_insert_or_assign_wrap(args)
#    define flat_hash_map_remove_entry_wrap(args...)                           \
        CCC_flat_hash_map_remove_entry_wrap(args)
#    define flat_hash_map_remove(args...) CCC_flat_hash_map_remove(args)
#    define flat_hash_map_swap_entry(args...) CCC_flat_hash_map_swap_entry(args)
#    define flat_hash_map_try_insert(args...) CCC_flat_hash_map_try_insert(args)
#    define flat_hash_map_insert_or_assign(args...)                            \
        CCC_flat_hash_map_insert_or_assign(args)
#    define flat_hash_map_remove_entry(args...)                                \
        CCC_flat_hash_map_remove_entry(args)
#    define flat_hash_map_entry_wrap(args...) CCC_flat_hash_map_entry_wrap(args)
#    define flat_hash_map_entry(args...) CCC_flat_hash_map_entry(args)
#    define flat_hash_map_and_modify(args...) CCC_flat_hash_map_and_modify(args)
#    define flat_hash_map_and_modify_context(args...)                          \
        CCC_flat_hash_map_and_modify_context(args)
#    define flat_hash_map_or_insert(args...) CCC_flat_hash_map_or_insert(args)
#    define flat_hash_map_insert_entry(args...)                                \
        CCC_flat_hash_map_insert_entry(args)
#    define flat_hash_map_unwrap(args...) CCC_flat_hash_map_unwrap(args)
#    define flat_hash_map_occupied(args...) CCC_flat_hash_map_occupied(args)
#    define flat_hash_map_insert_error(args...)                                \
        CCC_flat_hash_map_insert_error(args)
#    define flat_hash_map_begin(args...) CCC_flat_hash_map_begin(args)
#    define flat_hash_map_next(args...) CCC_flat_hash_map_next(args)
#    define flat_hash_map_end(args...) CCC_flat_hash_map_end(args)
#    define flat_hash_map_is_empty(args...) CCC_flat_hash_map_is_empty(args)
#    define flat_hash_map_count(args...) CCC_flat_hash_map_count(args)
#    define flat_hash_map_clear(args...) CCC_flat_hash_map_clear(args)
#    define flat_hash_map_clear_and_free(args...)                              \
        CCC_flat_hash_map_clear_and_free(args)
#    define flat_hash_map_clear_and_free_reserve(args...)                      \
        CCC_flat_hash_map_clear_and_free_reserve(args)
#    define flat_hash_map_capacity(args...) CCC_flat_hash_map_capacity(args)
#    define flat_hash_map_validate(args...) CCC_flat_hash_map_validate(args)
#endif

#endif /* CCC_FLAT_HASH_MAP_H */
