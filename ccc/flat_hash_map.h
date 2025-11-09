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

A Flat Hash Map stores elements by hash value and allows the user to retrieve
them by key in amortized O(1). Elements in the table may be copied and moved,
especially when rehashing occurs, so no pointer stability is available in this
implementation.

A flat hash map requires the user to provide a pointer to the map, a type, a key
field, a hash function, and a key three way comparison function. The hash
function should be well tailored to the key being stored in the table to prevent
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
cmake --preset=clang-rel -DCCC_FHM_PORTABLE
```

If an install location other than the release folder is desired don't forget
to add the install prefix.

```
cmake --preset=clang-rel -DCCC_FHM_PORTABLE -DCMAKE_INSTALL_PREFIX=/my/path/
```

If this library is being built as part of your project then define the flag
as part of your configuration.

```
cmake --preset=my-preset -DCCC_FHM_PORTABLE
```

Or, add the flag to your `CMakePresets.json`.

```
"cacheVariables": {
    "CCC_FHM_PORTABLE": "ON"
}
```

Or, enable on a per target basis in your `CMakeLists.txt`.

```
target_compile_definitions(my_target PRIVATE CCC_FHM_PORTABLE)
```

Or finally, just define it before including the flat hash map header.

```
#define CCC_FHM_PORTABLE
#include "ccc/flat_hash_map.h"
```

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#include "ccc/flat_hash_map.h"
```

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_FLAT_HASH_MAP_H
#define CCC_FLAT_HASH_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "impl/impl_flat_hash_map.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A container for storing key-value structures defined by the user in
a contiguous buffer.

A flat hash map can be initialized on the stack, heap, or data segment at
runtime or compile time. */
typedef struct ccc_fhmap ccc_flat_hash_map;

/** @brief A container specific entry used to implement the Entry Interface.

The Entry Interface offers efficient search and subsequent insertion, deletion,
or value update based on the needs of the user. */
typedef union ccc_fhmap_entry ccc_fhmap_entry;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. When a fixed
size map is required that will not have allocation permission, the user must
declare the type name and size of the map they will use. */
/**@{*/

/** @brief Declare a fixed size map type for use in the stack, heap, or data
segment. Does not return a value.
@param [in] fixed_map_type_name the user chosen name of the fixed sized map.
@param [in] key_val_type_name the type the user plans to store in the map. It
may have a key and value field as well as any additional fields. For set-like
behavior, wrap a field in a struct/union (e.g. `union int_elem {int e;};`).
@param [in] capacity the power of two capacity for the map.
@warning the capacity must be a power of two greater than 8 or 16, depending on
the platform (e.g. 16, 32, 64, etc.).

Once the location for the fixed size map is chosen--stack, heap, or data
segment--provide a pointer to the map for the initialization macro.

```
struct val
{
    int key;
    int val;
};
ccc_fhm_declare_fixed_map(small_fixed_map, struct val, 64);
static flat_hash_map static_fh = fhm_init(
    &(static small_fixed_map){},
    struct val,
    key,
    fhmap_int_to_u64,
    fhmap_id_cmp,
    NULL,
    NULL,
    fhm_fixed_capacity(small_fixed_map)
);
```

Similarly, a fixed size map can be used on the stack.

```
struct val
{
    int key;
    int val;
};
ccc_fhm_declare_fixed_map(small_fixed_map, struct val, 64);
int main(void)
{
    flat_hash_map static_fh = fhm_init(
        &(small_fixed_map){},
        struct val,
        key,
        fhmap_int_to_u64,
        fhmap_id_cmp,
        NULL,
        NULL,
        fhm_fixed_capacity(small_fixed_map)
    );
    return 0;
}
```

The ccc_fhm_fixed_capacity macro can be used to obtain the previously provided
capacity when declaring the fixed map type. Finally, one could allocate a fixed
size map on the heap; however, it is usually better to initialize a dynamic
map and use the ccc_fhm_reserve function for such a use case.

This macro is not needed when a dynamic resizing flat hash map is needed. For
dynamic maps, simply pass NULL and 0 capacity to the initialization macro. */
#define ccc_fhm_declare_fixed_map(fixed_map_type_name, key_val_type_name,      \
                                  capacity)                                    \
    ccc_impl_fhm_declare_fixed_map(fixed_map_type_name, key_val_type_name,     \
                                   capacity)

/** @brief Obtain the capacity previously chosen for the fixed size map type.
@param [in] fixed_map_type_name the name of a previously declared map.
@return the size_t capacity. This is the full capacity without any load factor
restrictions. */
#define ccc_fhm_fixed_capacity(fixed_map_type_name)                            \
    ccc_impl_fhm_fixed_capacity(fixed_map_type_name)

/** @brief Initialize a map with a buffer of types at compile time or runtime.
@param [in] map_ptr a pointer to a fixed map allocation or NULL.
@param [in] any_type_name the name of the user defined type stored in the map.
@param [in] key_field the field of the struct used for key storage.
@param [in] hash_fn the ccc_any_key_hash_fn function the user desires for the
table.
@param [in] key_cmp_fn the ccc_any_key_cmp_fn the user intends to use.
@param [in] alloc_fn the allocation function for resizing or NULL if no
resizing is allowed.
@param [in] aux_data auxiliary data that is needed for hashing or comparison.
@param [in] capacity the capacity of a fixed size map or 0.
@return the flat hash map directly initialized on the right hand side of the
equality operator (i.e. ccc_flat_hash_map fh = ccc_fhm_init(...);)
@note if a dynamic resizing map is required provide NULL as the map_ptr.

Initialize a static fixed size hash map at compile time that has
no allocation permission or auxiliary data needed.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct val
{
    int key;
    int val;
};
fhm_declare_fixed_map(small_fixed_map, struct val, 64);
static flat_hash_map static_fh = fhm_init(
    &(static small_fixed_map){},
    struct val,
    key,
    fhmap_int_to_u64,
    fhmap_key_cmp,
    NULL,
    NULL,
    fhm_fixed_capacity(small_fixed_map)
);
```

Initialize a dynamic hash table at compile time with allocation permission and
no auxiliary data. Use the same type as the previous example.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct val
{
    int key;
    int val;
};
static flat_hash_map static_fh = fhm_init(
    NULL,
    struct val,
    key,
    fhmap_int_to_u64,
    fhmap_key_cmp,
    std_alloc,
    NULL,
    0
);
```

Initialization at runtime is also possible. Stack-based or dynamic maps are
identical to the provided examples. Omit `static` in a runtime context. */
#define ccc_fhm_init(map_ptr, any_type_name, key_field, hash_fn, key_cmp_fn,   \
                     alloc_fn, aux_data, capacity)                             \
    ccc_impl_fhm_init(map_ptr, any_type_name, key_field, hash_fn, key_cmp_fn,  \
                      alloc_fn, aux_data, capacity)

/** @brief Initialize a dynamic map at runtime.
@param [in] key_field the field of the struct used for key storage.
@param [in] hash_fn the ccc_any_key_hash_fn function the user desires for the
table.
@param [in] key_cmp_fn the ccc_any_key_cmp_fn the user intends to use.
@param [in] alloc_fn the required allocation function.
@param [in] aux_data auxiliary data that is needed for hashing or comparison.
@param [in] initializer_list a list of key value pairs of the type intended to
be stored in the map, using array compound literal initialization syntax (e.g
`(struct my_type[]){{.k = 0, .v 0}, {.k = 1, .v = 1}}`).
@return the flat hash map directly initialized on the right hand side of the
equality operator (i.e. ccc_flat_hash_map fh = ccc_fhm_init_from(...);)
@warning An allocation function is required. This initializer is only available
for dynamic maps.
@warning If elements with duplicate keys are inserted, the earlier entry is
overwritten completely by all fields of the newer element.

Initialize a dynamic hash table at run time. This example requires no auxiliary
data for initialization.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct val
{
    int key;
    int val;
};
int
main(void)
{
    flat_hash_map static_fh = fhm_from(
        key,
        fhmap_int_to_u64,
        fhmap_key_cmp,
        std_alloc,
        NULL,
        (struct val[]) {
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
#define ccc_fhm_from(key_field, hash_fn, key_cmp_fn, alloc_fn, aux_data,       \
                     initializer_list...)                                      \
    ccc_impl_fhm_from(key_field, hash_fn, key_cmp_fn, alloc_fn, aux_data,      \
                      initializer_list)

/** @brief Copy the map at source to destination.
@param [in] dst the initialized destination for the copy of the src map.
@param [in] src the initialized source of the map.
@param [in] fn the optional allocation function if resizing is needed.
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
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct val
{
    int key;
    int val;
};
fhm_declare_fixed_map(small_fixed_map, struct val, 64);
flat_hash_map src = fhm_init(
    &(static small_fixed_map){},
    struct val,
    key,
    fhmap_int_to_u64,
    fhmap_key_cmp,
    NULL,
    NULL,
    ccc_fhm_fixed_capacity(small_fixed_map)
);
insert_rand_vals(&src);
flat_hash_map dst = fhm_init(
    &(static small_fixed_map){},
    struct val,
    key,
    fhmap_int_to_u64,
    fhmap_key_cmp,
    NULL,
    NULL,
    ccc_fhm_fixed_capacity(small_fixed_map)
);
ccc_result res = fhm_copy(&dst, &src, NULL);
```

The above requires dst capacity be greater than or equal to src capacity. Here
is memory management handed over to the copy function.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct val
{
    int key;
    int val;
};
flat_hash_map src = fhm_init(
    NULL,
    struct val,
    key,
    fhmap_int_to_u64,
    fhmap_key_cmp,
    std_alloc,
    NULL,
    0
);
insert_rand_vals(&src);
flat_hash_map dst = fhm_init(
    NULL,
    struct val,
    key,
    fhmap_int_to_u64,
    fhmap_key_cmp,
    std_alloc,
    NULL,
    0
);
ccc_result res = fhm_copy(&dst, &src, std_alloc);
```

The above allows dst to have a capacity less than that of the src as long as
copy has been provided an allocation function to resize dst. Note that this
would still work if copying to a destination that the user wants as a fixed
size map.

```
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct val
{
    int key;
    int val;
};
flat_hash_map src = fhm_init(
    NULL,
    struct val,
    key,
    fhmap_int_to_u64,
    fhmap_key_cmp,
    std_alloc,
    NULL,
    0
);
insert_rand_vals(&src);
flat_hash_map dst = fhm_init(
    NULL,
    struct val,
    key,
    fhmap_int_to_u64,
    fhmap_key_cmp,
    NULL,
    NULL,
    0
);
ccc_result res = fhm_copy(&dst, &src, std_alloc);
```

The above sets up dst with fixed size while src is a dynamic map. Because an
allocation function is provided, the dst is resized once for the copy and
retains its fixed size after the copy is complete. This would require the user
to manually free the underlying buffer at dst eventually if this method is used.
Usually it is better to allocate the memory with the reserve function if fixed
size dynamic maps are required.

These options allow users to stay consistent across containers with their
memory management strategies. */
ccc_result ccc_fhm_copy(ccc_flat_hash_map *dst, ccc_flat_hash_map const *src,
                        ccc_any_alloc_fn *fn);

/** @brief Reserve space required to add a specified number of elements to the
map. If the current capacity is sufficient, do nothing.
@param [in] h a pointer to the hash map.
@param [in] to_add the number of elements to add to the map.
@param [in] fn the required allocation function that can be used for resizing.
Such a function is provided to cover the case where the user wants a fixed size
map but cannot know the size needed until runtime. In this case, the function
needs to be provided to allow the single resizing to occur. Any auxiliary data
provided upon initialization will be passed to the allocation function when
called.
@return the result of the reserving operation, OK if successful or an error
code to indicate the specific failure.

If the map has already been initialized with allocation permission simply
provide the same function that was passed upon initialization. */
ccc_result ccc_fhm_reserve(ccc_flat_hash_map *h, size_t to_add,
                           ccc_any_alloc_fn *fn);

/**@}*/

/**@name Membership Interface
Test membership or obtain references to stored user types directly. */
/**@{*/

/** @brief Searches the table for the presence of key.
@param [in] h the flat hash table to be searched.
@param [in] key pointer to the key matching the key type of the user struct.
@return true if the struct containing key is stored, false if not. Error if h or
key is NULL. */
[[nodiscard]] ccc_tribool ccc_fhm_contains(ccc_flat_hash_map const *h,
                                           void const *key);

/** @brief Returns a reference into the table at entry key.
@param [in] h the flat hash map to search.
@param [in] key the key to search matching stored key type.
@return a view of the table entry if it is present, else NULL. */
[[nodiscard]] void *ccc_fhm_get_key_val(ccc_flat_hash_map const *h,
                                        void const *key);

/**@}*/

/** @name Entry Interface
Obtain and operate on container entries for efficient queries when non-trivial
control flow is needed. */
/**@{*/

/** @brief Obtains an entry for the provided key in the table for future use.
@param [in] h the hash table to be searched.
@param [in] key the key used to search the table matching the stored key type.
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
[[nodiscard]] ccc_fhmap_entry ccc_fhm_entry(ccc_flat_hash_map *h,
                                            void const *key);

/** @brief Obtains an entry for the provided key in the table for future use.
@param [in] map_ptr the hash table to be searched.
@param [in] key_ptr the key used to search the table matching the stored key
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
#define ccc_fhm_entry_r(map_ptr, key_ptr)                                      \
    &(ccc_fhmap_entry)                                                         \
    {                                                                          \
        ccc_fhm_entry(map_ptr, key_ptr).impl                                   \
    }

/** @brief Modifies the provided entry if it is Occupied.
@param [in] e the entry obtained from an entry function or macro.
@param [in] fn an update function in which the auxiliary argument is unused.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function is intended to make the function chaining in the Entry Interface
more succinct if the entry will be modified in place based on its own value
without the need of the auxiliary argument a ccc_any_type_update_fn can provide.
*/
[[nodiscard]] ccc_fhmap_entry *ccc_fhm_and_modify(ccc_fhmap_entry *e,
                                                  ccc_any_type_update_fn *fn);

/** @brief Modifies the provided entry if it is Occupied.
@param [in] e the entry obtained from an entry function or macro.
@param [in] fn an update function that requires auxiliary data.
@param [in] aux auxiliary data required for the update.
@return the updated entry if it was Occupied or the unmodified vacant entry.

This function makes full use of a ccc_any_type_update_fn capability, meaning a
complete ccc_update object will be passed to the update function callback. */
[[nodiscard]] ccc_fhmap_entry *
ccc_fhm_and_modify_aux(ccc_fhmap_entry *e, ccc_any_type_update_fn *fn,
                       void *aux);

/** @brief Modify an Occupied entry with a closure over user type T.
@param [in] map_entry_ptr a pointer to the obtained entry.
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
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
fhm_entry *e = fhm_and_modify_w(entry_r(&fhm, &k), word, T->cnt++;);

word *w = fhm_or_insert_w(fhm_and_modify_w(entry_r(&fhm, &k), word,
                                           { T->cnt++; }),
                          (word){.key = k, .cnt = 1});
```

Note that any code written is only evaluated if the entry is Occupied and the
container can deliver the user type T. This means any function calls are lazily
evaluated in the closure scope. */
#define ccc_fhm_and_modify_w(map_entry_ptr, type_name, closure_over_T...)      \
    &(ccc_fhmap_entry)                                                         \
    {                                                                          \
        ccc_impl_fhm_and_modify_w(map_entry_ptr, type_name, closure_over_T)    \
    }

/** @brief Inserts the struct with handle elem if the entry is Vacant.
@param [in] e the entry obtained via function or macro call.
@param [in] key_val_type the complete key and value type to be inserted.
@return a pointer to entry in the table invariantly. NULL on error.

Because this functions takes an entry and inserts if it is Vacant, the only
reason NULL shall be returned is when an insertion error will occur, usually
due to a resizing memory error. This can happen if the table is not allowed
to resize because no allocation function is provided. */
[[nodiscard]] void *ccc_fhm_or_insert(ccc_fhmap_entry const *e,
                                      void const *key_val_type);

/** @brief lazily insert the desired key value into the entry if it is Vacant.
@param [in] map_entry_ptr a pointer to the obtained entry.
@param [in] lazy_key_value the compound literal to construct in place if the
entry is Vacant.
@return a reference to the unwrapped user type in the entry, either the
unmodified reference if the entry was Occupied or the newly inserted element
if the entry was Vacant. NULL is returned if resizing is required but fails or
is not allowed.

Note that if the compound literal uses any function calls to generate values
or other data, such functions will not be called if the entry is Occupied. */
#define ccc_fhm_or_insert_w(map_entry_ptr, lazy_key_value...)                  \
    ccc_impl_fhm_or_insert_w(map_entry_ptr, lazy_key_value)

/** @brief Inserts the provided entry invariantly.
@param [in] e the entry returned from a call obtaining an entry.
@param [in] key_val_type the complete key and value type to be inserted.
@return a pointer to the inserted element or NULL upon a memory error in which
the load factor would be exceeded when no allocation policy is defined or
resizing failed to find more memory.

This method can be used when the old value in the table does not need to
be preserved. See the regular insert method if the old value is of interest.
If an error occurs during the insertion process due to memory limitations
or a search error NULL is returned. Otherwise insertion should not fail. */
[[nodiscard]] void *ccc_fhm_insert_entry(ccc_fhmap_entry const *e,
                                         void const *key_val_type);

/** @brief write the contents of the compound literal lazy_key_value to a slot.
@param [in] map_entry_ptr a pointer to the obtained entry.
@param [in] lazy_key_value the compound literal to write to a new slot.
@return a reference to the newly inserted or overwritten user type. NULL is
returned if resizing is required but fails or is not allowed. */
#define ccc_fhm_insert_entry_w(map_entry_ptr, lazy_key_value...)               \
    ccc_impl_fhm_insert_entry_w(map_entry_ptr, lazy_key_value)

/** @brief Invariantly inserts the key value wrapping out_handle.
@param [in] h the pointer to the flat hash map.
@param [out] key_val_type_output the complete key and value type to be inserted.
@return an entry. If Vacant, no prior element with key existed and entry may be
unwrapped to view the new insertion in the table. If Occupied the old value is
written to key_val_type_output and may be unwrapped to view. If more space is
needed but allocation fails or has been forbidden, an insert error is set.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value. */
[[nodiscard]]
ccc_entry ccc_fhm_swap_entry(ccc_flat_hash_map *h, void *key_val_type_output);

/** @brief Invariantly inserts the key value wrapping out_handle.
@param [in] map_ptr the pointer to the flat hash map.
@param [out] key_val_type_ptr the complete key and value type to be inserted.
@return a compound literal reference to an entry. If Vacant, no prior element
with key existed and entry may be unwrapped to view the new insertion in the
table. If Occupied the old value is written to key_val_type_output and may be
unwrapped to view. If more space is needed but allocation fails or has been
forbidden, an insert error is set.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value. */
#define ccc_fhm_swap_entry_r(map_ptr, key_val_type_ptr)                        \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_swap_entry(map_ptr, key_val_type_ptr).impl                     \
    }

/** @brief Remove the entry from the table if Occupied.
@param [in] e a pointer to the table entry.
@return an entry containing NULL. If Occupied an entry in the table existed and
was removed. If Vacant, no prior entry existed to be removed. */
[[nodiscard]] ccc_entry ccc_fhm_remove_entry(ccc_fhmap_entry const *e);

/** @brief Remove the entry from the table if Occupied.
@param [in] map_entry_ptr a pointer to the table entry.
@return a compound liter to an entry containing NULL. If Occupied an entry in
the table existed and was removed. If Vacant, no prior entry existed to be
removed. */
#define ccc_fhm_remove_entry_r(map_entry_ptr)                                  \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_remove_entry(map_entry_ptr).impl                               \
    }

/** @brief Attempts to insert the key value wrapping key_val_handle
@param [in] h the pointer to the flat hash map.
@param [in] key_val_type the complete key and value type to be inserted.
@return an entry. If Occupied, the entry contains a reference to the key value
user type in the table and may be unwrapped. If Vacant the entry contains a
reference to the newly inserted entry in the table. If more space is needed but
allocation fails or has been forbidden, an insert error is set.
@warning because this function returns a reference to a user type in the table
any subsequent insertions or deletions invalidate this reference. */
[[nodiscard]]
ccc_entry ccc_fhm_try_insert(ccc_flat_hash_map *h, void const *key_val_type);

/** @brief Attempts to insert the key value wrapping key_val_handle_ptr.
@param [in] map_ptr the pointer to the flat hash map.
@param [in] key_val_type_ptr the complete key and value type to be inserted.
@return a compound literal reference to the entry. If Occupied, the entry
contains a reference to the key value user type in the table and may be
unwrapped. If Vacant the entry contains a reference to the newly inserted
entry in the table. If more space is needed but allocation fails or has been
forbidden, an insert error is set.
@warning because this function returns a reference to a user type in the table
any subsequent insertions or deletions invalidate this reference. */
#define ccc_fhm_try_insert_r(map_ptr, key_val_type_ptr)                        \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_try_insert(map_ptr, key_val_type_ptr).impl                     \
    }

/** @brief lazily insert lazy_value into the map at key if key is absent.
@param [in] map_ptr a pointer to the flat hash map.
@param [in] key the direct key r-value.
@param [in] lazy_value the compound literal specifying the value.
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
#define ccc_fhm_try_insert_w(map_ptr, key, lazy_value...)                      \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_fhm_try_insert_w(map_ptr, key, lazy_value)                    \
    }

/** @brief Invariantly inserts or overwrites a user struct into the table.
@param [in] h a pointer to the flat hash map.
@param [in] key_val_type the complete key and value type to be inserted.
@return an entry. If Occupied an entry was overwritten by the new key value. If
Vacant no prior table entry existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
[[nodiscard]]
ccc_entry ccc_fhm_insert_or_assign(ccc_flat_hash_map *h,
                                   void const *key_val_type);

/** @brief Invariantly inserts or overwrites a user struct into the table.
@param [in] map_ptr a pointer to the flat hash map.
@param [in] key_val_type_ptr the complete key and value type to be inserted.
@return a compound literal reference to the entry. If Occupied an entry was
overwritten by the new key value. If Vacant no prior table entry existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
#define ccc_fhm_insert_or_assign_r(map_ptr, key_val_type_ptr)                  \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_insert_or_assign(map_ptr, key_val_type_ptr).impl               \
    }

/** @brief Inserts a new key value pair or overwrites the existing entry.
@param [in] map_ptr the pointer to the flat hash map.
@param [in] key the key to be searched in the table.
@param [in] lazy_value the compound literal specifying the value.
@return a compound literal reference to the entry of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unwrapping in any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define ccc_fhm_insert_or_assign_w(map_ptr, key, lazy_value...)                \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_fhm_insert_or_assign_w(map_ptr, key, lazy_value)              \
    }

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing out_handle provided by the user.
@param [in] h the pointer to the flat hash map.
@param [out] key_val_type_output the complete key and value type to be removed
@return the removed entry. If Occupied it may be unwrapped to obtain the old key
value pair. If Vacant the key value pair was not stored in the map. If bad input
is provided an input error is set. If a previously stored value is returned it
is safe to keep and modify this reference because the data has been written
to the provided space.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value. */
[[nodiscard]] ccc_entry ccc_fhm_remove(ccc_flat_hash_map *h,
                                       void *key_val_type_output);

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing out_handle_ptr provided by the user.
@param [in] map_ptr the pointer to the flat hash map.
@param [out] key_val_type_output_ptr the complete key and value type to be
removed
@return a compound literal reference to the removed entry. If Occupied it may
be unwrapped to obtain the old key value pair. If Vacant the key value pair
was not stored in the map. If bad input is provided an input error is set. If a
previously stored value is returned it is safe to keep and modify this reference
because the data has been written to the provided space.

Note that this function may write to the struct containing the second parameter
and wraps it in an entry to provide information about the old value. */
#define ccc_fhm_remove_r(map_ptr, key_val_type_output_ptr)                     \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_remove(map_ptr, key_val_type_output_ptr).impl                  \
    }

/** @brief Unwraps the provided entry to obtain a view into the table element.
@param [in] e the entry from a query to the table via function or macro.
@return an view into the table entry if one is present, or NULL. */
[[nodiscard]] void *ccc_fhm_unwrap(ccc_fhmap_entry const *e);

/** @brief Returns the Vacant or Occupied status of the entry.
@param [in] e the entry from a query to the table via function or macro.
@return true if the entry is occupied, false if not. Error if e is NULL. */
[[nodiscard]] ccc_tribool ccc_fhm_occupied(ccc_fhmap_entry const *e);

/** @brief Provides the status of the entry should an insertion follow.
@param [in] e the entry from a query to the table via function or macro.
@return true if the next insertion of a new element will cause an error. Error
if e is null.

Table resizing occurs upon calls to entry functions/macros or when trying
to insert a new element directly. This is to provide stable entries from the
time they are obtained to the time they are used in functions they are passed
to (i.e. the idiomatic or_insert(entry(...)...)).

However, if a Vacant entry is returned and then a subsequent insertion function
is used, it will not work if resizing has failed and the return of those
functions will indicate such a failure. One can also confirm an insertion error
will occur from an entry with this function. For example, leaving this function
in an assert for debug builds can be a helpful sanity check. */
[[nodiscard]] ccc_tribool ccc_fhm_insert_error(ccc_fhmap_entry const *e);

/** @brief Obtain the entry status from a container entry.
@param [in] e a pointer to the entry.
@return the status stored in the entry after the required action on the
container completes. If e is NULL an entry input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned.

Note that this function can be useful for debugging or if more detailed
messages are needed for logging purposes. See ccc_entry_status_msg() in
ccc/types.h for more information on detailed entry statuses. */
[[nodiscard]] ccc_entry_status ccc_fhm_entry_status(ccc_fhmap_entry const *e);

/**@}*/

/** @name Deallocation Interface
Destroy the container. */
/**@{*/

/** @brief Frees all slots in the table for use without affecting capacity.
@param [in] h the table to be cleared.
@param [in] fn the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the table before their slots are
forfeit.

If NULL is passed as the destructor function time is O(1), else O(capacity). */
ccc_result ccc_fhm_clear(ccc_flat_hash_map *h, ccc_any_type_destructor_fn *fn);

/** @brief Frees all slots in the table and frees the underlying buffer.
@param [in] h the table to be cleared.
@param [in] fn the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the table before their slots are
forfeit.
@return the result of free operation. If no alloc function is provided it is
an error to attempt to free the buffer and a memory error is returned.
Otherwise, an OK result is returned. */
ccc_result ccc_fhm_clear_and_free(ccc_flat_hash_map *h,
                                  ccc_any_type_destructor_fn *fn);

/** @brief Frees all slots in the table and frees the underlying buffer that was
previously dynamically reserved with the reserve function.
@param [in] h the table to be cleared.
@param [in] destructor the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the table before their slots are
forfeit.
@param [in] alloc the required allocation function to provide to a dynamically
reserved map. Any auxiliary data provided upon initialization will be passed to
the allocation function when called.
@return the result of free operation. CCC_RESULT_OK if success, or an error
status to indicate the error.
@warning It is an error to call this function on a map that was not reserved
with the provided ccc_any_alloc_fn. The map must have existing memory to free.

This function covers the edge case of reserving a dynamic capacity for a map
at runtime but denying the map allocation permission to resize. This can help
prevent a map from growing unbounded due to internal decisions about rehashes
and resizing. The user in this case knows the map does not have allocation
permission and therefore no further memory will be dedicated to the map.

However, to free the map in such a case this function must be used because the
map has no ability to free itself. Just as the allocation function is required
to reserve memory so to is it required to free memory.

This function will work normally if called on a map with allocation permission
however the normal ccc_fhm_clear_and_free is sufficient for that use case. */
ccc_result
ccc_fhm_clear_and_free_reserve(ccc_flat_hash_map *h,
                               ccc_any_type_destructor_fn *destructor,
                               ccc_any_alloc_fn *alloc);

/**@}*/

/** @name Iterator Interface
Obtain and manage iterators over the container. */
/**@{*/

/** @brief Obtains a pointer to the first element in the table.
@param [in] h the table to iterate through.
@return a pointer that can be cast directly to the user type that is stored.
@warning erasing or inserting during iteration may invalidate iterators if
resizing occurs which would lead to undefined behavior. O(capacity).

Iteration starts from index 0 by capacity of the table so iteration order is
not obvious to the user, nor should any specific order be relied on. */
[[nodiscard]] void *ccc_fhm_begin(ccc_flat_hash_map const *h);

/** @brief Advances the iterator to the next occupied table slot.
@param [in] h the table being iterated upon.
@param [in] key_val_type_iter the previous iterator.
@return a pointer that can be cast directly to the user type that is stored.
@warning erasing or inserting during iteration may invalidate iterators if
resizing occurs which would lead to undefined behavior. O(capacity). */
[[nodiscard]] void *ccc_fhm_next(ccc_flat_hash_map const *h,
                                 void const *key_val_type_iter);

/** @brief Check the current iterator against the end for loop termination.
@param [in] h the table being iterated upon.
@return the end address of the hash table.
@warning It is undefined behavior to access or modify the end address. */
[[nodiscard]] void *ccc_fhm_end(ccc_flat_hash_map const *h);

/**@}*/

/** @name State Interface
Obtain the container state. */
/**@{*/

/** @brief Returns the size status of the table.
@param [in] h the hash table.
@return true if empty else false. Error if h is NULL. */
[[nodiscard]] ccc_tribool ccc_fhm_is_empty(ccc_flat_hash_map const *h);

/** @brief Returns the count of table occupied slots.
@param [in] h the hash table.
@return the size of the map or an argument error is set if h is NULL. */
[[nodiscard]] ccc_ucount ccc_fhm_count(ccc_flat_hash_map const *h);

/** @brief Return the full capacity of the backing storage.
@param [in] h the hash table.
@return the capacity of the map or an argument error is set if h is NULL. */
[[nodiscard]] ccc_ucount ccc_fhm_capacity(ccc_flat_hash_map const *h);

/** @brief Validation of invariants for the hash table.
@param [in] h the table to validate.
@return true if all invariants hold, false if corruption occurs. Error if h is
NULL. */
[[nodiscard]] ccc_tribool ccc_fhm_validate(ccc_flat_hash_map const *h);

/**@}*/

/** Define this preprocessor directive if shorter names are helpful. Ensure
 no namespace clashes occur before shortening. */
#ifdef FLAT_HASH_MAP_USING_NAMESPACE_CCC
typedef ccc_flat_hash_map flat_hash_map;
typedef ccc_fhmap_entry fhmap_entry;
#    define fhm_declare_fixed_map(args...) ccc_fhm_declare_fixed_map(args)
#    define fhm_fixed_capacity(args...) ccc_fhm_fixed_capacity(args)
#    define fhm_reserve(args...) ccc_fhm_reserve(args)
#    define fhm_init(args...) ccc_fhm_init(args)
#    define fhm_from(args...) ccc_fhm_from(args)
#    define fhm_copy(args...) ccc_fhm_copy(args)
#    define fhm_and_modify_w(args...) ccc_fhm_and_modify_w(args)
#    define fhm_or_insert_w(args...) ccc_fhm_or_insert_w(args)
#    define fhm_insert_entry_w(args...) ccc_fhm_insert_entry_w(args)
#    define fhm_try_insert_w(args...) ccc_fhm_try_insert_w(args)
#    define fhm_insert_or_assign_w(args...) ccc_fhm_insert_or_assign_w(args)
#    define fhm_contains(args...) ccc_fhm_contains(args)
#    define fhm_get_key_val(args...) ccc_fhm_get_key_val(args)
#    define fhm_remove_r(args...) ccc_fhm_remove_r(args)
#    define fhm_swap_entry_r(args...) ccc_fhm_swap_entry_r(args)
#    define fhm_try_insert_r(args...) ccc_fhm_try_insert_r(args)
#    define fhm_insert_or_assign_r(args...) ccc_fhm_insert_or_assign_r(args)
#    define fhm_remove_entry_r(args...) ccc_fhm_remove_entry_r(args)
#    define fhm_remove(args...) ccc_fhm_remove(args)
#    define fhm_swap_entry(args...) ccc_fhm_swap_entry(args)
#    define fhm_try_insert(args...) ccc_fhm_try_insert(args)
#    define fhm_insert_or_assign(args...) ccc_fhm_insert_or_assign(args)
#    define fhm_remove_entry(args...) ccc_fhm_remove_entry(args)
#    define fhm_entry_r(args...) ccc_fhm_entry_r(args)
#    define fhm_entry(args...) ccc_fhm_entry(args)
#    define fhm_and_modify(args...) ccc_fhm_and_modify(args)
#    define fhm_and_modify_aux(args...) ccc_fhm_and_modify_aux(args)
#    define fhm_or_insert(args...) ccc_fhm_or_insert(args)
#    define fhm_insert_entry(args...) ccc_fhm_insert_entry(args)
#    define fhm_unwrap(args...) ccc_fhm_unwrap(args)
#    define fhm_occupied(args...) ccc_fhm_occupied(args)
#    define fhm_insert_error(args...) ccc_fhm_insert_error(args)
#    define fhm_begin(args...) ccc_fhm_begin(args)
#    define fhm_next(args...) ccc_fhm_next(args)
#    define fhm_end(args...) ccc_fhm_end(args)
#    define fhm_is_empty(args...) ccc_fhm_is_empty(args)
#    define fhm_count(args...) ccc_fhm_count(args)
#    define fhm_clear(args...) ccc_fhm_clear(args)
#    define fhm_clear_and_free(args...) ccc_fhm_clear_and_free(args)
#    define fhm_clear_and_free_reserve(args...)                                \
        ccc_fhm_clear_and_free_reserve(args)
#    define fhm_capacity(args...) ccc_fhm_capacity(args)
#    define fhm_validate(args...) ccc_fhm_validate(args)
#endif

#endif /* CCC_FLAT_HASH_MAP_H */
