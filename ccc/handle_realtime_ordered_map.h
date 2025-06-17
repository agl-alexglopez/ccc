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
@brief The Handle Realtime Ordered Map Interface

A handle realtime ordered map offers storage and retrieval by key. This map is
suitable for realtime applications if resizing can be well controlled. Insert
operations may cause resizing if allocation is allowed.

The handle variant of the ordered map promises contiguous storage and random
access if needed. Handles are stable and the user can use them to refer to an
element until that element is removed from the map. Handles remain valid even if
resizing of the table, insertions of other elements, or removals of other
elements occur. Active user elements may not be contiguous from index [0, N)
where N is the size of map; there may be gaps between active elements in the
buffer and it is only guaranteed that N elements are stored between index [0,
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
#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_HANDLE_REALTIME_ORDERED_MAP_H
#define CCC_HANDLE_REALTIME_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "impl/impl_handle_realtime_ordered_map.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A handle realtime ordered map offers O(lg N) search and erase, and
amortized O(lg N) insert.
@warning it is undefined behavior to access an uninitialized container.

A handle realtime ordered map can be initialized on the stack, heap, or data
segment at runtime or compile time.*/
typedef struct ccc_hromap ccc_handle_realtime_ordered_map;

/** @brief A container specific handle used to implement the Handle Interface.
@warning it is undefined behavior to access an uninitialized container.

The Handle Interface offers efficient search and subsequent insertion, deletion,
or value update based on the needs of the user. Handles obtained via the Handle
Interface are stable until the user removes the element at the provided handle.
Insertions and deletions of other elements do not affect handle stability.
Resizing of the table does not affect handle stability. */
typedef union ccc_hromap_handle ccc_hromap_handle;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Declare a fixed size map type for use in the stack, heap, or data
segment. Does not return a value.
@param [in] fixed_map_type_name the user chosen name of the fixed sized map.
@param [in] key_val_type_name the type the user plans to store in the map. It
may have a key and value field as well as any additional fields. For set-like
behavior, wrap a field in a struct/union (e.g. `union int_elem {int e;};`).
@param [in] capacity the desired number of user accessible nodes.
@warning the map will use one slot of the specified capacity for a sentinel
node. This is not important to the user unless an exact allocation count is
needed in which case 1 should be added to desired capacity.

Once the location for the fixed size map is chosen--stack, heap, or data
segment--provide a pointer to the map for the initialization macro.

```
struct val
{
    int key;
    int val;
};
ccc_hrm_declare_fixed_map(small_fixed_map, struct val, 64);
static handle_realtime_ordered_map static_map = hrm_init(
    &(static small_fixed_map){},
    struct val,
    key,
    hrmap_key_cmp,
    NULL,
    NULL,
    hrm_fixed_capacity(small_fixed_map)
);
```

Similarly, a fixed size map can be used on the stack.

```
struct val
{
    int key;
    int val;
};
ccc_hrm_declare_fixed_map(small_fixed_map, struct val, 64);
int main(void)
{
    flat_hash_map static_fh = hrm_init(
        &(small_fixed_map){},
        struct val,
        key,
        hrmap_key_cmp,
        NULL,
        NULL,
        hrm_fixed_capacity(small_fixed_map)
    );
    return 0;
}
```

The ccc_hrm_fixed_capacity macro can be used to obtain the previously provided
capacity when declaring the fixed map type. Finally, one could allocate a fixed
size map on the heap; however, it is usually better to initialize a dynamic
map and use the ccc_hrm_reserve function for such a use case.

This macro is not needed when a dynamic resizing map is needed. For dynamic
maps, simply pass NULL and 0 capacity to the initialization macro along with the
desired allocation function. */
#define ccc_hrm_declare_fixed_map(fixed_map_type_name, key_val_type_name,      \
                                  capacity)                                    \
    ccc_impl_hrm_declare_fixed_map(fixed_map_type_name, key_val_type_name,     \
                                   capacity)

/** @brief Obtain the capacity previously chosen for the fixed size map type.
@param [in] fixed_map_type_name the name of a previously declared map.
@return the size_t capacity previously specified for this type by user. */
#define ccc_hrm_fixed_capacity(fixed_map_type_name)                            \
    ccc_impl_hrm_fixed_capacity(fixed_map_type_name)

/** @brief Initializes the map at runtime or compile time.
@param [in] memory_ptr a pointer to the contiguous user types or ((T *)NULL).
@param [in] any_type_name the name of the user type stored in the map.
@param [in] key_elem_field the name of the field in user type used as key.
@param [in] key_cmp_fn the key comparison function (see types.h).
@param [in] alloc_fn the allocation function or NULL if allocation is banned.
@param [in] aux_data a pointer to any auxiliary data for comparison or
destruction.
@param [in] capacity the capacity at mem_ptr or 0.
@return the struct initialized ordered map for direct assignment
(i.e. ccc_handle_realtime_ordered_map m = ccc_hrm_init(...);). */
#define ccc_hrm_init(memory_ptr, any_type_name, key_elem_field, key_cmp_fn,    \
                     alloc_fn, aux_data, capacity)                             \
    ccc_impl_hrm_init(memory_ptr, any_type_name, key_elem_field, key_cmp_fn,   \
                      alloc_fn, aux_data, capacity)

/** @brief Copy the map at source to destination.
@param [in] dst the initialized destination for the copy of the src map.
@param [in] src the initialized source of the map.
@param [in] fn the allocation function to resize dst or NULL.
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
#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
struct val
{
    int key;
    int val;
};
ccc_hrm_declare_fixed_map(small_fixed_map, struct val, 64);
static handle_realtime_ordered_map src = hrm_init(
    &(static small_fixed_map){},
    struct val,
    key,
    hrmap_key_cmp,
    NULL,
    NULL,
    hrm_fixed_capacity(small_fixed_map)
);
insert_rand_vals(&src);
static handle_realtime_ordered_map dst = hrm_init(
    &(static small_fixed_map){},
    struct val,
    key,
    hrmap_key_cmp,
    NULL,
    NULL,
    hrm_fixed_capacity(small_fixed_map)
);
ccc_result res = hrm_copy(&dst, &src, NULL);
```

The above requires dst capacity be greater than or equal to src capacity. Here
is memory management handed over to the copy function.

```
#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
struct val
{
    int key;
    int val;
};
static handle_ordered_map src
    = hrm_init(NULL, struct val, key, key_cmp, std_alloc, NULL, 0);
insert_rand_vals(&src);
static handle_ordered_map dst
    = hrm_init(NULL, struct val, key, key_cmp, std_alloc, NULL, 0);
ccc_result res = hrm_copy(&dst, &src, std_alloc);
```

The above allows dst to have a capacity less than that of the src as long as
copy has been provided an allocation function to resize dst. Note that this
would still work if copying to a destination that the user wants as a fixed
size map.

```
#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
struct val
{
    int key;
    int val;
};
static handle_ordered_map src
    = hrm_init(NULL, struct val, key, key_cmp, std_alloc, NULL, 0);
insert_rand_vals(&src);
static handle_ordered_map dst
    = hrm_init(NULL, struct val, key, key_cmp, NULL, NULL, 0);
ccc_result res = hrm_copy(&dst, &src, std_alloc);
```

The above sets up dst with fixed size while src is a dynamic map. Because an
allocation function is provided, the dst is resized once for the copy and
retains its fixed size after the copy is complete. This would require the user
to manually free the underlying buffer at dst eventually if this method is used.
Usually it is better to allocate the memory explicitly before the copy if
copying between maps without allocation permission.

These options allow users to stay consistent across containers with their
memory management strategies. */
ccc_result ccc_hrm_copy(ccc_handle_realtime_ordered_map *dst,
                        ccc_handle_realtime_ordered_map const *src,
                        ccc_any_alloc_fn *fn);

/** @brief Reserves space for at least to_add more elements.
@param [in] hrm a pointer to the handle realtime ordered map.
@param [in] to_add the number of elements to add to the current size.
@param [in] fn the allocation function to use to reserve memory.
@return the result of the reservation. OK if successful, otherwise an error
status is returned.
@note see the ccc_hrm_clear_and_free_reserve function if this function is
being used for a one-time dynamic reservation.

This function can be used for a dynamic hrm with or without allocation
permission. If the hrm has allocation permission, it will reserve the required
space and later resize if more space is needed.

If the hrm has been initialized with no allocation permission and no memory
this function can serve as a one-time reservation. This is helpful when a fixed
size is needed but that size is only known dynamically at runtime. To free the
hrm in such a case see the ccc_hrm_clear_and_free_reserve function. */
ccc_result ccc_hrm_reserve(ccc_handle_realtime_ordered_map *hrm, size_t to_add,
                           ccc_any_alloc_fn *fn);

/**@}*/

/**@name Membership Interface
Test membership or obtain references to stored user types directly. */
/**@{*/

/** @brief Returns a reference to the user data at the provided handle.
@param [in] h a pointer to the map.
@param [in] i the stable handle obtained by the user.
@return a pointer to the user type stored at the specified handle or NULL if
an out of range handle or handle representing no data is provided.
@warning this function can only check if the handle value is in range. If a
handle represents a slot that has been taken by a new element because the old
one has been removed that new element data will be returned.
@warning do not try to access data in the table manually with a handle. Always
use this provided interface function when a reference to data is needed. */
[[nodiscard]] void *ccc_hrm_at(ccc_handle_realtime_ordered_map const *h,
                               ccc_handle_i i);

/** @brief Returns a reference to the user type in the table at the handle.
@param [in] handle_realtime_ordered_map_ptr a pointer to the map.
@param [in] type_name name of the user type stored in each slot of the map.
@param [in] handle_i the index handle obtained from previous map operations.
@return a reference to the handle at handle in the map as the type the user has
stored in the map. */
#define ccc_hrm_as(handle_realtime_ordered_map_ptr, type_name, handle_i...)    \
    ccc_impl_hrm_as(handle_realtime_ordered_map_ptr, type_name, handle_i)

/** @brief Searches the map for the presence of key.
@param [in] hrm the map to be searched.
@param [in] key pointer to the key matching the key type of the user struct.
@return true if the struct containing key is stored, false if not. Error if hrm
or key is NULL. */
[[nodiscard]] ccc_tribool
ccc_hrm_contains(ccc_handle_realtime_ordered_map const *hrm, void const *key);

/** @brief Returns a reference into the map at handle key.
@param [in] hrm the ordered map to search.
@param [in] key the key to search matching stored key type.
@return a view of the map handle if it is present, else NULL. */
[[nodiscard]] ccc_handle_i
ccc_hrm_get_key_val(ccc_handle_realtime_ordered_map const *hrm,
                    void const *key);

/**@}*/

/** @name Handle Interface
Obtain and operate on container entries for efficient queries when non-trivial
control flow is needed. */
/**@{*/

/** @brief Invariantly inserts the key value in key_val_type_output.
@param [in] hrm the pointer to the ordered map.
@param [out] key_val_type_output the type user type map elem.
@return a type element in the table. If Vacant, no prior element with
key existed and the type key value type remains unchanged. If Occupied the
old value is written to the type key value type. If more space is needed
but allocation fails or has been forbidden, an insert error is set.

Note that this function may write to the provided user type struct. */
[[nodiscard]] ccc_handle
ccc_hrm_swap_handle(ccc_handle_realtime_ordered_map *hrm,
                    void *key_val_type_output);

/** @brief Invariantly inserts the key value in key_val_type_output_ptr.
@param [in] handle_realtime_ordered_map_ptr the pointer to the ordered map.
@param [out] key_val_type_output_ptr type user type map elem.
@return a compound literal reference to a type element in the table. If
Vacant, no prior element with key existed and the type key value type
remains unchanged. If Occupied the old value is written to the type wrapping
key value type. If more space is needed but allocation fails or has been
forbidden, an insert error is set.

Note that this function may write to the provided user type struct. */
#define ccc_hrm_swap_handle_r(handle_realtime_ordered_map_ptr,                 \
                              key_val_type_output_ptr)                         \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_hrm_swap_handle((handle_realtime_ordered_map_ptr),                 \
                            (key_val_type_output_ptr))                         \
            .impl                                                              \
    }

/** @brief Attempts to insert the key value in key_val_type.
@param [in] hrm the pointer to the map.
@param [in] key_val_type the type user type map elem.
@return a handle. If Occupied, the handle contains a reference to the key value
user type in the map and may be unwrapped. If Vacant the handle contains a
reference to the newly inserted handle in the map. If more space is needed but
allocation fails, an insert error is set. */
[[nodiscard]] ccc_handle
ccc_hrm_try_insert(ccc_handle_realtime_ordered_map *hrm,
                   void const *key_val_type);

/** @brief Attempts to insert the key value key_val_type_ptr.
@param [in] handle_realtime_ordered_map_ptr the pointer to the map.
@param [in] key_val_type_ptr the type user type map elem.
@return a compound literal reference to a handle. If Occupied, the handle
contains a reference to the key value user type in the map and may be unwrapped.
If Vacant the handle contains a reference to the newly inserted handle in the
map. If more space is needed but allocation fails an insert error is set. */
#define ccc_hrm_try_insert_r(handle_realtime_ordered_map_ptr,                  \
                             key_val_type_ptr)                                 \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_hrm_try_insert((handle_realtime_ordered_map_ptr),                  \
                           (key_val_type_ptr))                                 \
            .impl                                                              \
    }

/** @brief lazily insert lazy_value into the map at key if key is absent.
@param [in] handle_realtime_ordered_map_ptr a pointer to the map.
@param [in] key the direct key r-value.
@param [in] lazy_value the compound literal specifying the value.
@return a compound literal reference to the handle of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unin any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define ccc_hrm_try_insert_w(handle_realtime_ordered_map_ptr, key,             \
                             lazy_value...)                                    \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_impl_hrm_try_insert_w(handle_realtime_ordered_map_ptr, key,        \
                                  lazy_value)                                  \
    }

/** @brief Invariantly inserts or overwrites a user struct into the map.
@param [in] hrm a pointer to the handle hash map.
@param [in] key_val_type the type user struct key value.
@return a handle. If Occupied a handle was overwritten by the new key value.
If Vacant no prior map handle existed.

Note that this function can be used when the old user type is not needed but
the information regarding its presence is helpful. */
[[nodiscard]] ccc_handle
ccc_hrm_insert_or_assign(ccc_handle_realtime_ordered_map *hrm,
                         void const *key_val_type);

/** @brief Inserts a new key value pair or overwrites the existing handle.
@param [in] handle_realtime_ordered_map_ptr the pointer to the handle hash map.
@param [in] key the key to be searched in the map.
@param [in] lazy_value the compound literal to insert or use for overwrite.
@return a compound literal reference to the handle of the existing or newly
inserted value. Occupied indicates the key existed, Vacant indicates the key
was absent. Unin any case provides the current value unless an error
occurs that prevents insertion. An insertion error will flag such a case.

Note that for brevity and convenience the user need not write the key to the
lazy value compound literal as well. This function ensures the key in the
compound literal matches the searched key. */
#define ccc_hrm_insert_or_assign_w(handle_realtime_ordered_map_ptr, key,       \
                                   lazy_value...)                              \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_impl_hrm_insert_or_assign_w(handle_realtime_ordered_map_ptr, key,  \
                                        lazy_value)                            \
    }

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing key_val_type_output provided by the user.
@param [in] hrm the pointer to the ordered map.
@param [out] key_val_type_output the type user type map elem.
@return the removed handle. If Occupied key value type holds the old key value
pair. If Vacant the key value pair was not stored in the map. If bad input is
provided an input error is set.

Note that this function may write to the user type struct. */
[[nodiscard]] ccc_handle ccc_hrm_remove(ccc_handle_realtime_ordered_map *hrm,
                                        void *key_val_type_output);

/** @brief Removes the key value in the map storing the old value, if present,
in the struct containing key value type provided by the user.
@param [in] handle_realtime_ordered_map_ptr the pointer to the ordered map.
@param [out] key_val_type_output_ptr type user type map elem.
@return a compound literal reference to the removed handle. If Occupied
key_val_type_output_ptr holds the old key value pair.. If Vacant the key value
pair was not stored in the map. If bad input is provided an input error is set.

Note that this function may write to the user type struct. */
#define ccc_hrm_remove_r(handle_realtime_ordered_map_ptr,                      \
                         key_val_type_output_ptr)                              \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_hrm_remove((handle_realtime_ordered_map_ptr),                      \
                       (key_val_type_output_ptr))                              \
            .impl                                                              \
    }

/** @brief Obtains a handle for the provided key in the map for future use.
@param [in] hrm the map to be searched.
@param [in] key the key used to search the map matching the stored key type.
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
[[nodiscard]] ccc_hromap_handle
ccc_hrm_handle(ccc_handle_realtime_ordered_map const *hrm, void const *key);

/** @brief Obtains a handle for the provided key in the map for future use.
@param [in] handle_realtime_ordered_map_ptr the map to be searched.
@param [in] key_ptr the key used to search the map matching the stored key type.
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
#define ccc_hrm_handle_r(handle_realtime_ordered_map_ptr, key_ptr)             \
    &(ccc_hromap_handle)                                                       \
    {                                                                          \
        ccc_hrm_handle((handle_realtime_ordered_map_ptr), (key_ptr)).impl      \
    }

/** @brief Modifies the provided handle if it is Occupied.
@param [in] h the handle obtained from a handle function or macro.
@param [in] fn an update function in which the auxiliary argument is unused.
@return the updated handle if it was Occupied or the unmodified vacant handle.

This function is intended to make the function chaining in the Handle Interface
more succinct if the handle will be modified in place based on its own value
without the need of the auxiliary argument a ccc_any_type_update_fn can provide.
*/
[[nodiscard]] ccc_hromap_handle *ccc_hrm_and_modify(ccc_hromap_handle *h,
                                                    ccc_any_type_update_fn *fn);

/** @brief Modifies the provided handle if it is Occupied.
@param [in] h the handle obtained from a handle function or macro.
@param [in] fn an update function that requires auxiliary data.
@param [in] aux auxiliary data required for the update.
@return the updated handle if it was Occupied or the unmodified vacant handle.

This function makes full use of a ccc_any_type_update_fn capability, meaning a
complete ccc_update object will be passed to the update function callback. */
[[nodiscard]] ccc_hromap_handle *
ccc_hrm_and_modify_aux(ccc_hromap_handle *h, ccc_any_type_update_fn *fn,
                       void *aux);

/** @brief Modify an Occupied handle with a closure over user type T.
@param [in] handle_realtime_ordered_map_handle_ptr a pointer to the obtained
handle.
@param [in] type_name the name of the user type stored in the container.
@param [in] closure_over_T the code to be run on the reference to user type T,
if Occupied. This may be a semicolon separated list of statements to execute on
T or a section of code wrapped in braces {code here} which may be preferred
for formatting.
@return a compound literal reference to the modified handle if it was occupied
or a vacant handle if it was vacant.
@note T is a reference to the user type stored in the handle guaranteed to be
non-NULL if the closure executes.

```
#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
hrm_handle *e = hrm_and_modify_w(handle_r(&hrm, &k), word, T->cnt++;);
handle_i w = hrm_or_insert_w(hrm_and_modify_w(handle_r(&hrm, &k), word,
                                              { T->cnt++; }),
                             (word){.key = k, .cnt = 1});
```

Note that any code written is only evaluated if the handle is Occupied and the
container can deliver the user type T. This means any function calls are lazily
evaluated in the closure scope. */
#define ccc_hrm_and_modify_w(handle_realtime_ordered_map_handle_ptr,           \
                             type_name, closure_over_T...)                     \
    &(ccc_hromap_handle)                                                       \
    {                                                                          \
        ccc_impl_hrm_and_modify_w(handle_realtime_ordered_map_handle_ptr,      \
                                  type_name, closure_over_T)                   \
    }

/** @brief Inserts the provided user type if the handle is Vacant.
@param [in] h the handle obtained via function or macro call.
@param [in] key_val_type the type struct to be inserted to a Vacant handle.
@return a pointer to handle in the map invariantly. NULL on error.

Because this functions takes a handle and inserts if it is Vacant, the only
reason NULL shall be returned is when an insertion error occurs, usually due to
a user struct allocation failure.

If no allocation is permitted, this function assumes the user struct wrapping
elem has been allocated with the appropriate lifetime and scope by the user. */
[[nodiscard]] ccc_handle_i ccc_hrm_or_insert(ccc_hromap_handle const *h,
                                             void const *key_val_type);

/** @brief Lazily insert the desired key value into the handle if it is Vacant.
@param [in] handle_realtime_ordered_map_handle_ptr a pointer to the obtained
handle.
@param [in] lazy_key_value the compound literal to construct in place if the
handle is Vacant.
@return a reference to the unwrapped user type in the handle, either the
unmodified reference if the handle was Occupied or the newly inserted element
if the handle was Vacant. NULL is returned if resizing is required but fails or
is not allowed.

Note that if the compound literal uses any function calls to generate values
or other data, such functions will not be called if the handle is Occupied. */
#define ccc_hrm_or_insert_w(handle_realtime_ordered_map_handle_ptr,            \
                            lazy_key_value...)                                 \
    ccc_impl_hrm_or_insert_w(handle_realtime_ordered_map_handle_ptr,           \
                             lazy_key_value)

/** @brief Inserts the provided user type invariantly.
@param [in] h the handle returned from a call obtaining a handle.
@param [in] key_val_type a type struct the user intends to insert.
@return a pointer to the inserted element or NULL upon allocation failure.

This method can be used when the old value in the map does not need to
be preserved. See the regular insert method if the old value is of interest. */
[[nodiscard]] ccc_handle_i ccc_hrm_insert_handle(ccc_hromap_handle const *h,
                                                 void const *key_val_type);

/** @brief Write the contents of the compound literal lazy_key_value to a node.
@param [in] handle_realtime_ordered_map_handle_ptr a pointer to the obtained
handle.
@param [in] lazy_key_value the compound literal to write to a new slot.
@return a reference to the newly inserted or overwritten user type. NULL is
returned if allocation failed or is not allowed when required. */
#define ccc_hrm_insert_handle_w(handle_realtime_ordered_map_handle_ptr,        \
                                lazy_key_value...)                             \
    ccc_impl_hrm_insert_handle_w(handle_realtime_ordered_map_handle_ptr,       \
                                 lazy_key_value)

/** @brief Remove the handle from the map if Occupied.
@param [in] h a pointer to the map handle.
@return a handle containing NULL or a reference to the old handle. If Occupied
a handle in the map existed and was removed. If Vacant, no prior handle existed
to be removed.
@warning the reference to the removed handle is invalidated upon any further
insertions. */
ccc_handle ccc_hrm_remove_handle(ccc_hromap_handle const *h);

/** @brief Remove the handle from the map if Occupied.
@param [in] handle_realtime_ordered_map_handle_ptr a pointer to the map handle.
@return a compound literal reference to a handle containing NULL or a reference
to the old handle. If Occupied a handle in the map existed and was removed. If
Vacant, no prior handle existed to be removed.

Note that the reference to the removed handle is invalidated upon any further
insertions. */
#define ccc_hrm_remove_handle_r(handle_realtime_ordered_map_handle_ptr)        \
    &(ccc_handle)                                                              \
    {                                                                          \
        ccc_hrm_remove_handle((handle_realtime_ordered_map_handle_ptr)).impl   \
    }

/** @brief Unwraps the provided handle to obtain a view into the map element.
@param [in] h the handle from a query to the map via function or macro.
@return a view into the table handle if one is present, or NULL. */
[[nodiscard]] ccc_handle_i ccc_hrm_unwrap(ccc_hromap_handle const *h);

/** @brief Returns the Vacant or Occupied status of the handle.
@param [in] h the handle from a query to the map via function or macro.
@return true if the handle is occupied, false if not. Error if h is NULL. */
[[nodiscard]] ccc_tribool ccc_hrm_occupied(ccc_hromap_handle const *h);

/** @brief Provides the status of the handle should an insertion follow.
@param [in] h the handle from a query to the table via function or macro.
@return true if a handle obtained from an insertion attempt failed to insert
due to an allocation failure when allocation success was expected. Error if h is
NULL. */
[[nodiscard]] ccc_tribool ccc_hrm_insert_error(ccc_hromap_handle const *h);

/** @brief Obtain the handle status from a container handle.
@param [in] h a pointer to the handle.
@return the status stored in the handle after the required action on the
container completes. If h is NULL a handle input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned.

Note that this function can be useful for debugging or if more detailed
messages are needed for logging purposes. See ccc_handle_status_msg() in
ccc/types.h for more information on detailed handle statuses. */
[[nodiscard]] ccc_handle_status
ccc_hrm_handle_status(ccc_hromap_handle const *h);

/**@}*/

/** @name Deallocation Interface
Deallocate the container. */
/**@{*/

/** @brief Frees all slots in the map for use without affecting capacity.
@param [in] hrm the map to be cleared.
@param [in] fn the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the map before their slots are
forfeit.

If NULL is passed as the destructor function time is O(1), else O(size). */
ccc_result ccc_hrm_clear(ccc_handle_realtime_ordered_map *hrm,
                         ccc_any_type_destructor_fn *fn);

/** @brief Frees all slots in the map and frees the underlying buffer.
@param [in] hrm the map to be cleared.
@param [in] fn the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the map before their slots are
forfeit.
@return the result of free operation. If no alloc function is provided it is
an error to attempt to free the buffer and a memory error is returned.
Otherwise, an OK result is returned.

If NULL is passed as the destructor function time is O(1), else O(size). */
ccc_result ccc_hrm_clear_and_free(ccc_handle_realtime_ordered_map *hrm,
                                  ccc_any_type_destructor_fn *fn);

/** @brief Frees all slots in the hrm and frees the underlying buffer that was
previously dynamically reserved with the reserve function.
@param [in] hrm the map to be cleared.
@param [in] destructor the destructor for each element. NULL can be passed if no
maintenance is required on the elements in the hrm before their slots are
dropped.
@param [in] alloc the required allocation function to provide to a dynamically
reserved hrm. Any auxiliary data provided upon initialization will be passed to
the allocation function when called.
@return the result of free operation. OK if success, or an error status to
indicate the error.
@warning It is an error to call this function on a hrm that was not reserved
with the provided ccc_any_alloc_fn. The hrm must have existing memory to free.

This function covers the edge case of reserving a dynamic capacity for a hrm
at runtime but denying the hrm allocation permission to resize. This can help
prevent a hrm from growing unbounded. The user in this case knows the hrm does
not have allocation permission and therefore no further memory will be dedicated
to the hrm.

However, to free the hrm in such a case this function must be used because the
hrm has no ability to free itself. Just as the allocation function is required
to reserve memory so to is it required to free memory.

This function will work normally if called on a hrm with allocation permission
however the normal ccc_hrm_clear_and_free is sufficient for that use case. */
ccc_result
ccc_hrm_clear_and_free_reserve(ccc_handle_realtime_ordered_map *hrm,
                               ccc_any_type_destructor_fn *destructor,
                               ccc_any_alloc_fn *alloc);

/**@}*/

/** @name Iterator Interface
Obtain and manage iterators over the container. */
/**@{*/

/** @brief Return an iterable range of values from [begin_key, end_key). O(lgN).
@param [in] hrm a pointer to the map.
@param [in] begin_key a pointer to the key intended as the start of the range.
@param [in] end_key a pointer to the key intended as the end of the range.
@return a range containing the first element NOT LESS than the begin_key and
the first element GREATER than end_key.

Note that due to the variety of values that can be returned in the range, using
the provided range iteration functions from types.h is recommended for example:

```
for (struct val *i = range_begin(&range);
     i != end_range(&range);
     i = next(&hrm, i))
{}
```

This avoids any possible errors in handling an end range element that is in the
map versus the end map sentinel. */
[[nodiscard]] ccc_range
ccc_hrm_equal_range(ccc_handle_realtime_ordered_map const *hrm,
                    void const *begin_key, void const *end_key);

/** @brief Returns a compound literal reference to the desired range. O(lg N).
@param [in] handle_realtime_ordered_map_ptr a pointer to the map.
@param [in] begin_and_end_key_ptrs two pointers, the first to the start of the
range the second to the end of the range.
@return a compound literal reference to the produced range associated with the
enclosing scope. This reference is always non-NULL. */
#define ccc_hrm_equal_range_r(handle_realtime_ordered_map_ptr,                 \
                              begin_and_end_key_ptrs...)                       \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_hrm_equal_range((handle_realtime_ordered_map_ptr),                 \
                            (begin_and_end_key_ptrs))                          \
            .impl                                                              \
    }

/** @brief Return an iterable rrange of values from [begin_key, end_key).
O(lg N).
@param [in] hrm a pointer to the map.
@param [in] rbegin_key a pointer to the key intended as the start of the rrange.
@param [in] rend_key a pointer to the key intended as the end of the rrange.
@return a rrange containing the first element NOT GREATER than the begin_key and
the first element LESS than rend_key.

Note that due to the variety of values that can be returned in the rrange, using
the provided rrange iteration functions from types.h is recommended for example:

```
for (struct val *i = rrange_begin(&rrange);
     i != rend_rrange(&rrange);
     i = rnext(&fom, i))
{}
```

This avoids any possible errors in handling an rend rrange element that is in
the map versus the end map sentinel. */
[[nodiscard]] ccc_rrange
ccc_hrm_equal_rrange(ccc_handle_realtime_ordered_map const *hrm,
                     void const *rbegin_key, void const *rend_key);

/** @brief Returns a compound literal reference to the desired rrange.
O(lg N).
@param [in] handle_realtime_ordered_map_ptr a pointer to the map.
@param [in] rbegin_and_rend_key_ptrs two pointers, the first to the start of the
rrange the second to the end of the rrange.
@return a compound literal reference to the produced rrange associated with the
enclosing scope. This reference is always non-NULL. */
#define ccc_hrm_equal_rrange_r(handle_realtime_ordered_map_ptr,                \
                               rbegin_and_rend_key_ptrs...)                    \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_hrm_equal_rrange((handle_realtime_ordered_map_ptr),                \
                             (rbegin_and_rend_key_ptrs))                       \
            .impl                                                              \
    }

/** @brief Return the start of an inorder traversal of the map. O(lg N).
@param [in] hrm a pointer to the map.
@return the oldest minimum element of the map. */
[[nodiscard]] void *ccc_hrm_begin(ccc_handle_realtime_ordered_map const *hrm);

/** @brief Return the start of a reverse inorder traversal of the map. O(lg N).
@param [in] hrm a pointer to the map.
@return the oldest maximum element of the map. */
[[nodiscard]] void *ccc_hrm_rbegin(ccc_handle_realtime_ordered_map const *hrm);

/** @brief Return the next element in an inorder traversal of the map. O(1).
@param [in] hrm a pointer to the map.
@param [in] key_val_type_iter a pointer to the intrusive map element of the
current iterator.
@return the next user type stored in the map in an inorder traversal. */
[[nodiscard]] void *ccc_hrm_next(ccc_handle_realtime_ordered_map const *hrm,
                                 void const *key_val_type_iter);

/** @brief Return the rnext element in a reverse inorder traversal of the map.
O(1).
@param [in] hrm a pointer to the map.
@param [in] key_val_type_iter a pointer to the intrusive map element of the
current iterator.
@return the rnext user type stored in the map in a reverse inorder traversal. */
[[nodiscard]] void *ccc_hrm_rnext(ccc_handle_realtime_ordered_map const *hrm,
                                  void const *key_val_type_iter);

/** @brief Return the end of an inorder traversal of the map. O(1).
@param [in] hrm a pointer to the map.
@return the newest maximum element of the map. */
[[nodiscard]] void *ccc_hrm_end(ccc_handle_realtime_ordered_map const *hrm);

/** @brief Return the rend of a reverse inorder traversal of the map. O(1).
@param [in] hrm a pointer to the map.
@return the newest minimum element of the map. */
[[nodiscard]] void *ccc_hrm_rend(ccc_handle_realtime_ordered_map const *hrm);

/**@}*/

/** @name State Interface
Obtain the container state. */
/**@{*/

/** @brief Returns the size status of the map.
@param [in] hrm the map.
@return true if empty else false. Error if hrm is NULL. */
[[nodiscard]] ccc_tribool
ccc_hrm_is_empty(ccc_handle_realtime_ordered_map const *hrm);

/** @brief Returns the size of the map representing active slots.
@param [in] hrm the map.
@return the size of the map or an argument error is set if hrm is NULL. */
[[nodiscard]] ccc_ucount
ccc_hrm_size(ccc_handle_realtime_ordered_map const *hrm);

/** @brief Returns the capacity of the map representing total available slots.
@param [in] hrm the map.
@return the capacity or an argument error is set if hrm is NULL. */
[[nodiscard]] ccc_ucount
ccc_hrm_capacity(ccc_handle_realtime_ordered_map const *hrm);

/** @brief Return a reference to the base of backing array. O(1).
@param [in] hrm a pointer to the map.
@return a reference to the base of the backing array.
@note the reference is to the base of the backing array at index 0 with no
consideration for the organization of map and contains capacity elements.
@warning it is the users responsibility to ensure that access to any data is
within the capacity of the backing buffer. */
[[nodiscard]] void *ccc_hrm_data(ccc_handle_realtime_ordered_map const *hrm);

/** @brief Validation of invariants for the map.
@param [in] hrm the map to validate.
@return true if all invariants hold, false if corruption occurs. Error if hrm is
NULL. */
[[nodiscard]] ccc_tribool
ccc_hrm_validate(ccc_handle_realtime_ordered_map const *hrm);

/**@}*/

/** Define this preprocessor directive if shorter names are helpful. Ensure
 no namespace clashes occur before shortening. */
#ifdef HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
typedef ccc_handle_realtime_ordered_map handle_realtime_ordered_map;
typedef ccc_hromap_handle hromap_handle;
#    define hrm_declare_fixed_map(args...) ccc_hrm_declare_fixed_map(args)
#    define hrm_init(args...) ccc_hrm_init(args)
#    define hrm_fixed_capacity(args...) ccc_hrm_fixed_capacity(args)
#    define hrm_copy(args...) ccc_hrm_copy(args)
#    define hrm_reserve(args...) ccc_hrm_reserve(args)
#    define hrm_at(args...) ccc_hrm_at(args)
#    define hrm_as(args...) ccc_hrm_as(args)
#    define hrm_and_modify_w(args...) ccc_hrm_and_modify_w(args)
#    define hrm_or_insert_w(args...) ccc_hrm_or_insert_w(args)
#    define hrm_insert_handle_w(args...) ccc_hrm_insert_handle_w(args)
#    define hrm_try_insert_w(args...) ccc_hrm_try_insert_w(args)
#    define hrm_insert_or_assign_w(args...) ccc_hrm_insert_or_assign_w(args)
#    define hrm_contains(args...) ccc_hrm_contains(args)
#    define hrm_get_key_val(args...) ccc_hrm_get_key_val(args)
#    define hrm_swap_handle(args...) ccc_hrm_swap_handle(args)
#    define hrm_swap_handle_r(args...) ccc_hrm_swap_handle_r(args)
#    define hrm_begin(args...) ccc_hrm_begin(args)
#    define hrm_rbegin(args...) ccc_hrm_rbegin(args)
#    define hrm_next(args...) ccc_hrm_next(args)
#    define hrm_rnext(args...) ccc_hrm_rnext(args)
#    define hrm_end(args...) ccc_hrm_end(args)
#    define hrm_rend(args...) ccc_hrm_rend(args)
#    define hrm_data(args...) ccc_hrm_data(args)
#    define hrm_is_empty(args...) ccc_hrm_is_empty(args)
#    define hrm_size(args...) ccc_hrm_size(args)
#    define hrm_clear(args...) ccc_hrm_clear(args)
#    define hrm_clear_and_free(args...) ccc_hrm_clear_and_free(args)
#    define hrm_clear_and_free_reserve(args...)                                \
        ccc_hrm_clear_and_free_reserve(args)
#    define hrm_validate(args...) ccc_hrm_validate(args)
#endif /* HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC */

#endif /* CCC_HANDLE_REALTIME_ORDERED_MAP_H */
