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
@brief The C Container Collection Fundamental Types

All containers make use of the fundamental types defined here. The purpose of
these types is to aid the user in writing correct callback functions, allow
clear error handling, and present a consistent interface to users across
containers. If allocation permission is given to containers be sure to review
the allocation function interface. */
#ifndef CCC_TYPES_H
#define CCC_TYPES_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "impl/impl_types.h"

/** @name Container Types
Types used across many containers. */
/**@{*/

/** @brief The result of a range query on iterable containers.

A range provides a view all elements that fit the equals range criteria
of search-by-key containers. Use the provided range iteration functions in
this header to iterate from beginning to end in forward order relative to
the containers default ordering. */
typedef union CCC_range CCC_range;

/** @brief The result of a rrange query on iterable containers.

A rrange provides a view all elements that fit the equals rrange criteria
of search-by-key containers. Use the provided range iteration functions in
this header to iterate from beginning to end in reverse order relative to
the containers default ordering. */
typedef union CCC_rrange CCC_rrange;

/** @brief An Occupied or Vacant position in a searchable container.

A entry is the basis for more complex container specific Entry Interface for
all search-by-key containers. An entry is returned from various operations
to provide both a reference to data and any auxiliary status that is
important for the user. An entry can be Occupied or Vacant. See individual
headers for containers that return this type for its meaning in context. */
typedef union CCC_entry CCC_entry;

/** @brief The status monitoring and entry state once it is obtained.

To manage safe and efficient views into associative containers entries use
status flags internally. The provided functions in the Entry Interface for
each container are sufficient to obtain the needed status. However if more
information is needed, the status can be passed to the CCC_entry_status_msg()
function for detailed string messages regarding the entry status. This may
be helpful for debugging or logging. */
typedef enum CCC_entry_status CCC_entry_status;

/** @brief An Occupied or Vacant handle to a flat searchable container entry.

A handle uses the same semantics as an entry. However, the wrapped value is
a CCC_handle_i index. When this type is returned the container interface is
promising that this element will remain at the returned handle index until the
element is removed by the user. This is similar to pointer stability but offers
a stronger guarantee that will hold even if the underlying container is
resized. */
typedef union CCC_handle CCC_handle;

/** @brief A stable index to user data in a container that uses a flat array as
the underlying storage method.

User data at a handle position in an array remains valid until that element is
removed from the container. This means that resizing of the underlying array
may occur, but the handle index remains valid regardless.

This is similar to pointer stability except that pointers would not remain valid
when the underlying array is resized; a handle remains valid because it is an
index not a pointer. */
typedef size_t CCC_handle_i;

/** @brief The status monitoring and handle state once it is obtained.

To manage safe and efficient views into associative containers entries use
status flags internally. The provided functions in the Handle Interface for
each container are sufficient to obtain the needed status. However if more
information is needed, the status can be passed to the CCC_entry_status_msg()
function for detailed string messages regarding the handle status. This may
be helpful for debugging or logging. */
typedef enum CCC_entry_status CCC_handle_status;

/** @brief A three state boolean to allow for an error state. Error is -1, False
is 0, and True is 1.

Some containers conceptually take or return a boolean value as part of their
operations. However, booleans cannot indicate errors and this library offers
no errno or C++ throw-like behavior. Therefore, a three state value can offer
additional information while still maintaining the truthy and falsey bool
behavior one would normally expect.

A third branch can be added while otherwise using simple true(1) and false(0).
`if (result == CCC_TRIBOOL_ERROR) {} else if (result) {} else {}`. */
typedef enum : int8_t
{
    /** Intended value if CCC_FALSE or CCC_TRUE could not be returned. */
    CCC_TRIBOOL_ERROR = -1,
    /** Equivalent to boolean false, guaranteed to be falsey aka 0. */
    CCC_FALSE,
    /** Equivalent to boolean true, guaranteed to be truthy aka 1. */
    CCC_TRUE,
} CCC_tribool;

/** @brief A result of actions on containers.

A result indicates the status of the requested operation. Each container
provides status messages according to the result type returned from a operation
that uses this type. */
typedef enum : uint8_t
{
    /** The operation has occurred successfully. */
    CCC_RESULT_OK = 0,
    /** An operation ran but could not return the intended result. */
    CCC_RESULT_FAIL,
    /** Memory is needed but the container lacks allocation permission. */
    CCC_RESULT_NO_ALLOC,
    /** The container has allocation permission, but allocation failed. */
    CCC_RESULT_MEM_ERROR,
    /** Bad arguments have been provided and operation returned early. */
    CCC_RESULT_ARG_ERROR,
    /** Internal helper, never returned to user. Always last result. */
    CCC_RESULT_COUNT,
} CCC_result;

/** @brief A three-way comparison for comparison functions.

A C style three way comparison value (e.g. ((a > b) - (a < b))). CCC_LES if
left hand side is less than right hand side, CCC_EQL if they are equal, and
CCC_GRT if left hand side is greater than right hand side. */
typedef enum : int8_t
{
    /** The left hand side is less than the right hand side. */
    CCC_LES = -1,
    /** The left hand side and right hand side are equal. */
    CCC_EQL,
    /** The left hand side is greater than the right hand side. */
    CCC_GRT,
    /** Comparison is not possible or other error has occurred. */
    CCC_CMP_ERROR,
} CCC_threeway_cmp;

/** @brief A type for returning an unsigned integer from a container for
counting. Intended to count sizes, capacities, and 0-based indices.

Access the fields of this struct directly to check for errors and then use the
returned index. If an error has occurred, the index is invalid. An error will be
indicated by any non-zero value in the error field.

```
CCC_ucount res = CCC_bs_first_trailing_one(&my_bitset);
if (res.error)
{
    // handle errors...
}
(void)CCC_bs_set(&my_bitset, res.count, CCC_TRUE);
```

Full string explanations of the exact CCC_result error types can be provided via
the CCC_result_msg function if the enum itself does not provide sufficient
explanation. */
typedef struct
{
    /** The error that occurred indicated by a status. 0 (falsey) means OK. */
    CCC_result error;
    /** The count returned by the operation. */
    size_t count;
} CCC_ucount;

/** @brief An element comparison helper.

This type helps the user define the comparison callback function, if the
container takes a standard element comparison function, and helps avoid
swappable argument errors. Any type LHS is considered the left hand side and
any type RHS is the right hand side when considering three-way comparison
return values. Aux data is a reference to any auxiliary data provided upon
container initialization. */
typedef struct
{
    /** The left hand side for a three-way comparison operation. */
    void const *const any_type_lhs;
    /** The right hand side for a three-way comparison operation. */
    void const *const any_type_rhs;
    /** A reference to aux data provided to container on initialization. */
    void *aux;
} CCC_any_type_cmp;

/** @brief A key comparison helper to avoid argument swapping.

The key is considered the left hand side of the operation if three-way
comparison is needed. Note a comparison is taking place between the key on the
left hand side and the complete user type on the right hand side. This means the
right hand side will need to manually access its key field.

```
int const *const my_key_lhs = cmp.any_key_lhs;
struct key_val const *const my_type_rhs  = cmp.any_type_rhs;
return (*my_key_lhs > my_type_rhs->key) - (*my_key_lhs < my_type_rhs->key);
```

Notice that the user type must access its key field of its struct. Comparison
must happen this way to support searching by key in associative containers
rather than by entire user struct. Only needing to provide a key can save
significant memory for a search depending on the size of the user type. */
typedef struct
{
    /** Key matching the key field of the provided type to the container. */
    void const *const any_key_lhs;
    /** The complete user type stored in the container. */
    void const *const any_type_rhs;
    /** A reference to aux data provided to the container on initialization. */
    void *aux;
} CCC_any_key_cmp;

/** @brief A reference to a user type within the container.

This is to help users define callback functions that act on each node in a
container. For example, a destruct function will use this type. */
typedef struct
{
    /** The user type being stored in the container. */
    void *any_type;
    /** A reference to aux data provided to the container on initialization. */
    void *aux;
} CCC_any_type;

/** @brief A read only reference to a key type matching the key field type used
for hash containers.

A reference to any auxiliary data is also provided. This the struct one can use
to hash their values with their hash function. */
typedef struct
{
    /** A reference to the same type used for keys in the container. */
    void const *const any_key;
    /** A reference to aux data provided to the container on initialization. */
    void *aux;
} CCC_any_key;

/** @brief An allocation function at the core of all containers.

An allocation function implements the following behavior, where ptr is pointer
to memory, size is number of bytes to allocate, and aux is a reference to any
supplementary information required for allocation, deallocation, or
reallocation. Aux is passed to a container upon its initialization and the
programmer may choose how to best utilize this reference (more on aux later).

- If NULL is provided with a size of 0, NULL is returned.
- If NULL is provided with a non-zero size, new memory is allocated/returned.
- If ptr is non-NULL it has been previously allocated by the alloc function.
- If ptr is non-NULL with non-zero size, ptr is resized to at least size
  size. The pointer returned is NULL if resizing fails. Upon success, the
  pointer returned might not be equal to the pointer provided.
- If ptr is non-NULL and size is 0, ptr is freed and NULL is returned.

One may be tempted to use realloc to check all of these boxes but realloc is
implementation defined on some of these points. So, the aux parameter also
discourages users from providing realloc. For example, one solution using the
standard library allocator might be implemented as follows (aux is not needed):

```
void *
std_alloc(void *const ptr, size_t const size, void *)
{
    if (!ptr && !size)
    {
        return NULL;
    }
    if (!ptr)
    {
        return malloc(size);
    }
    if (!size)
    {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, size);
}
```

However, the above example is only useful if the standard library allocator
is used. Any allocator that implements the required behavior is sufficient.
For example programs that utilize the aux parameter, see the sample programs.
Using custom arena allocators or container compositions are cases when aux is
needed. */
typedef void *CCC_any_alloc_fn(void *ptr, size_t size, void *aux);

/** @brief A callback function for comparing two elements in a container.

A three-way comparison return value is expected and the two containers being
compared are guaranteed to be non-NULL and pointing to the base of the user type
stored in the container. Aux may be NULL if no aux is provided on
initialization. */
typedef CCC_threeway_cmp CCC_any_type_cmp_fn(CCC_any_type_cmp);

/** @brief A callback function for modifying an element in the container.

A reference to the container type and any aux data provided on initialization
is available. The container pointer points to the base of the user type and is
not NULL. Aux may be NULL if no aux is provided on initialization. An update
function is used when a container Interface exposes functions to modify the key
or value used to determine sorted order of elements in the container. */
typedef void CCC_any_type_update_fn(CCC_any_type);

/** @brief A callback function for destroying an element in the container.

A reference to the container type and any aux data provided on initialization
is available. The container pointer points to the base of the user type and is
not NULL. Aux may be NULL if no aux is provided on initialization. A destructor
function is used to act on each element of the container when it is being
emptied and destroyed. The function will be called on each type after it
removed from the container and before it is freed by the container, if
allocation permission is provided to the container. Therefore, if the user
has given permission to the container to allocate memory they can assume the
container will free each element with the provided allocation function; this
function can be used for any other program state to be maintained before the
container frees. If the user has not given permission to the container to
allocate memory, this a good function in which to free each element, if
desired; any program state can be maintained then the element can be freed by
the user in this function as the final step. */
typedef void CCC_any_type_destructor_fn(CCC_any_type);

/** @brief A callback function for three-way comparing two stored keys.

The key is considered the left hand side of the comparison. The function should
return CCC_LES if the key is less than the key in key field of user type,
CCC_EQL if equal, and CCC_GRT if greater. */
typedef CCC_threeway_cmp CCC_any_key_cmp_fn(CCC_any_key_cmp);

/** @brief A callback function to hash the key type used in a container.

A reference to any aux data provided on initialization is also available.
Return the complete hash value as determined by the user hashing algorithm. */
typedef uint64_t CCC_any_key_hash_fn(CCC_any_key to_hash);

/**@}*/

/** @name Entry Interface
The generic interface for associative container entries. */
/**@{*/

/** @brief Determine if an entry is Occupied in the container.
@param [in] e the pointer to the entry obtained from a container.
@return true if Occupied false if Vacant. Error if e is NULL. */
CCC_tribool CCC_entry_occupied(CCC_entry const *e);

/** @brief Determine if an insertion error has occurred when a function that
attempts to insert a value in a container is used.
@param [in] e the pointer to the entry obtained from a container insert.
@return true if an insertion error occurred usually meaning a insertion should
have occurred but the container did not have permission to allocate new memory
or allocation failed. Error if e is NULL. */
CCC_tribool CCC_entry_insert_error(CCC_entry const *e);

/** @brief Determine if an input error has occurred for a function that
generates an entry.
@param [in] e the pointer to the entry obtained from a container function.
@return true if an input error occurred usually meaning an invalid argument such
as a NULL pointer was provided to a function. Error if e is NULL. */
CCC_tribool CCC_entry_input_error(CCC_entry const *e);

/** @brief Unwraps the provided entry providing a reference to the user type
obtained from the operation that provides the entry.
@param [in] e the pointer to the entry obtained from an operation.
@return a reference to the user type stored in the Occupied entry or NULL if
the entry is Vacant or otherwise cannot be viewed.

The expected return value from unwrapping a value will change depending on the
container from which the entry is obtained. Read the documentation for the
container being used to understand what to expect from this function once an
entry is obtained. */
void *CCC_entry_unwrap(CCC_entry const *e);

/** @brief Determine if an handle is Occupied in the container.
@param [in] e the pointer to the handle obtained from a container.
@return true if Occupied false if Vacant. Error if e is NULL. */
CCC_tribool CCC_handle_occupied(CCC_handle const *e);

/** @brief Determine if an insertion error has occurred when a function that
attempts to insert a value in a container is used.
@param [in] e the pointer to the handle obtained from a container insert.
@return true if an insertion error occurred usually meaning a insertion should
have occurred but the container did not have permission to allocate new memory
or allocation failed. Error if e is NULL. */
CCC_tribool CCC_handle_insert_error(CCC_handle const *e);

/** @brief Determine if an input error has occurred for a function that
generates an handle.
@param [in] e the pointer to the handle obtained from a container function.
@return true if an input error occurred usually meaning an invalid argument such
as a NULL pointer was provided to a function. Error if e is NULL. */
CCC_tribool CCC_handle_input_error(CCC_handle const *e);

/** @brief Unwraps the provided handle providing a reference to the user type
obtained from the operation that provides the handle.
@param [in] e the pointer to the handle obtained from an operation.
@return a reference to the user type stored in the Occupied handle or NULL if
the handle is Vacant or otherwise cannot be viewed.

The expected return value from unwrapping a value will change depending on the
container from which the handle is obtained. Read the documentation for the
container being used to understand what to expect from this function once an
handle is obtained. */
CCC_handle_i CCC_handle_unwrap(CCC_handle const *e);

/**@}*/

/** @name Range Interface
The generic range interface for associative containers. */
/**@{*/

/** @brief Obtain a reference to the beginning user element stored in a
container in the provided range.
@param [in] r a pointer to the range.
@return a reference to the user type stored in the container that serves as
the beginning of the range.

Note the beginning of a range may be equivalent to the end or NULL. */
void *CCC_begin_range(CCC_range const *r);

/** @brief Obtain a reference to the end user element stored in a
container in the provided range.
@param [in] r a pointer to the range.
@return a reference to the user type stored in the container that serves as
the end of the range.

Note the end of a range may be equivalent to the beginning or NULL. Functions
that obtain ranges treat the end as an exclusive bound and therefore it is
undefined to access this element. */
void *CCC_end_range(CCC_range const *r);

/** @brief Obtain a reference to the reverse beginning user element stored in a
container in the provided range.
@param [in] r a pointer to the range.
@return a reference to the user type stored in the container that serves as
the reverse beginning of the range.

Note the reverse beginning of a range may be equivalent to the reverse end or
NULL. */
void *CCC_rbegin_rrange(CCC_rrange const *r);

/** @brief Obtain a reference to the reverse end user element stored in a
container in the provided range.
@param [in] r a pointer to the range.
@return a reference to the user type stored in the container that serves as
the reverse end of the range.

Note the reverse end of a range may be equivalent to the reverse beginning or
NULL. Functions that obtain ranges treat the reverse end as an exclusive bound
and therefore it is undefined to access this element. */
void *CCC_rend_rrange(CCC_rrange const *r);

/**@}*/

/** @name Status Interface
Functions for obtaining more descriptive status information. */
/**@{*/

/** @brief Obtain a string message with a description of the error returned
from a container operation, possible causes, and possible fixes to such error.
@param [in] res the result obtained from a container operation.
@return a string message of the result. A CCC_RESULT_OK result is an empty
string, the falsey NULL terminator. All other results have a string message.

These messages can be used for logging or to help with debugging by providing
more information for why such a result might be obtained from a container. */
char const *CCC_result_msg(CCC_result res);

/** @brief Obtain the entry status from a generic entry.
@param [in] e a pointer to the entry.
@return the status stored in the entry after the required action on the
container completes. If e is NULL an entry input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned. */
CCC_entry_status CCC_get_entry_status(CCC_entry const *e);

/** @brief Obtain the handle status from a generic handle.
@param [in] e a pointer to the handle.
@return the status stored in the handle after the required action on the
container completes. If e is NULL an handle input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned. */
CCC_handle_status CCC_get_handle_status(CCC_handle const *e);

/** @brief Obtain a string message with a description of the entry status.
@param [in] status the status obtained from an entry.
@return a string message with more detailed information regarding the status.

Note that status for an entry is relevant when it is first obtained and when
an action completes. Obtaining an entry can provide information on whether
the search yielded an Occupied or Vacant Entry or any errors that may have
occurred. If a function tries to complete an action like insertion or removal
the status can reflect if any errors occurred in this process as well. Usually,
the provided interface gives all the functions needed to check status but these
strings can be used when more details are required. */
char const *CCC_entry_status_msg(CCC_entry_status status);

/** @brief Obtain a string message with a description of the handle status.
@param [in] status the status obtained from an handle.
@return a string message with more detailed information regarding the status.

Note that status for an handle is relevant when it is first obtained and when
an action completes. Obtaining an handle can provide information on whether
the search yielded an Occupied or Vacant handle or any errors that may have
occurred. If a function tries to complete an action like insertion or removal
the status can reflect if any errors occurred in this process as well. Usually,
the provided interface gives all the functions needed to check status but these
strings can be used when more details are required. */
char const *CCC_handle_status_msg(CCC_handle_status status);

/**@}*/

/** Define this directive at the top of a translation unit if shorter names are
desired. By default the ccc prefix is used to avoid namespace clashes. */
#ifdef TYPES_USING_NAMESPACE_CCC
typedef CCC_range range;
typedef CCC_rrange rrange;
typedef CCC_entry entry;
typedef CCC_handle handle;
typedef CCC_handle_i handle_i;
typedef CCC_result result;
typedef CCC_threeway_cmp threeway_cmp;
typedef CCC_any_type any_type;
typedef CCC_any_type_cmp any_type_cmp;
typedef CCC_any_key any_key;
typedef CCC_any_key_cmp any_key_cmp;
typedef CCC_any_alloc_fn any_alloc_fn;
typedef CCC_any_type_cmp_fn any_type_cmp_fn;
typedef CCC_any_type_update_fn any_type_update_fn;
typedef CCC_any_type_destructor_fn any_type_destructor_fn;
typedef CCC_any_key_cmp_fn any_key_cmp_fn;
typedef CCC_any_key_hash_fn any_key_hash_fn;
#    define entry_occupied(entry_ptr) CCC_entry_occupied(entry_ptr)
#    define entry_insert_error(entry_ptr) CCC_entry_insert_error(entry_ptr)
#    define entry_unwrap(entry_ptr) CCC_entry_unwrap(entry_ptr)
#    define get_entry_status(entry_ptr) CCC_get_entry_status(entry_ptr)
#    define entry_status_msg(status) CCC_entry_status_msg(status)
#    define handle_occupied(handle_ptr) CCC_handle_occupied(handle_ptr)
#    define handle_insert_error(handle_ptr) CCC_handle_insert_error(handle_ptr)
#    define handle_unwrap(handle_ptr) CCC_handle_unwrap(handle_ptr)
#    define get_handle_status(handle_ptr) CCC_get_handle_status(handle_ptr)
#    define handle_status_msg(status) CCC_handle_status_msg(status)
#    define begin_range(range_ptr) CCC_begin_range(range_ptr)
#    define end_range(range_ptr) CCC_end_range(range_ptr)
#    define rbegin_rrange(range_ptr) CCC_rbegin_rrange(range_ptr)
#    define rend_rrange(range_ptr) CCC_rend_rrange(range_ptr)
#    define result_msg(res) CCC_result_msg(res)
#endif /* TYPES_USING_NAMESPACE_CCC */

#endif /* CCC_TYPES_H */
