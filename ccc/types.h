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

#include "private/private_types.h"

/** @name Container Types
Types used across many containers. */
/**@{*/

/** @brief The result of a range query on iterable containers.

A range provides a view all elements that fit the equals range criteria
of search-by-key containers. Use the provided range iteration functions in
this header to iterate from beginning to end in forward order relative to
the containers default ordering. */
typedef union CCC_Range_wrap CCC_Range;

/** @brief The result of a range_reverse query on iterable containers.

A range_reverse provides a view all elements that fit the equals range_reverse
criteria of search-by-key containers. Use the provided range iteration functions
in this header to iterate from beginning to end in reverse order relative to the
containers default ordering. */
typedef union CCC_Range_reverse_wrap CCC_Range_reverse;

/** @brief The result of a range query on iterable containers.

A range provides a view all elements that fit the equals range criteria
of search-by-key containers. Use the provided range iteration functions in
this header to iterate from beginning to end in forward order relative to
the containers default ordering. */
typedef union CCC_Handle_range_wrap CCC_Handle_range;

/** @brief The result of a range_reverse query on iterable containers.

A range_reverse provides a view all elements that fit the equals range_reverse
criteria of search-by-key containers. Use the provided range iteration functions
in this header to iterate from beginning to end in reverse order relative to the
containers default ordering. */
typedef union CCC_Handle_range_reverse_wrap CCC_Handle_range_reverse;

/** @brief An Occupied or Vacant position in a searchable container.

A entry is the basis for more complex container specific Entry Interface for
all search-by-key containers. An entry is returned from various operations
to provide both a reference to data and any context status that is
important for the user. An entry can be Occupied or Vacant. See individual
headers for containers that return this type for its meaning in context. */
typedef union CCC_Entry_wrap CCC_Entry;

/** @brief The status monitoring and entry state once it is obtained.

To manage safe and efficient views into associative containers entries use
status flags internally. The provided functions in the Entry Interface for
each container are sufficient to obtain the needed status. However if more
information is needed, the status can be passed to the
CCC_entry_status_message() function for detailed string messages regarding the
entry status. This may be helpful for debugging or logging. */
typedef enum CCC_Entry_status CCC_Entry_status;

/** @brief An Occupied or Vacant handle to a flat searchable container entry.

A handle uses the same semantics as an entry. However, the wrapped value is
a CCC_Handle_index index. When this type is returned the container interface is
promising that this element will remain at the returned handle index until the
element is removed by the user. This is similar to pointer stability but offers
a stronger guarantee that will hold even if the underlying container is
resized. */
typedef union CCC_Handle_wrap CCC_Handle;

/** @brief A stable index to user data in a container that uses a flat array as
the underlying storage method.

User data at a handle position in an array remains valid until that element is
removed from the container. This means that resizing of the underlying array
may occur, but the handle index remains valid regardless.

This is similar to pointer stability except that pointers would not remain valid
when the underlying array is resized; a handle remains valid because it is an
index not a pointer. */
typedef size_t CCC_Handle_index;

/** @brief The status monitoring and handle state once it is obtained.

To manage safe and efficient views into associative containers entries use
status flags internally. The provided functions in the Handle Interface for
each container are sufficient to obtain the needed status. However if more
information is needed, the status can be passed to the
CCC_entry_status_message() function for detailed string messages regarding the
handle status. This may be helpful for debugging or logging. */
typedef enum CCC_Entry_status CCC_Handle_status;

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
} CCC_Tribool;

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
    CCC_RESULT_NO_ALLOCATION_FUNCTION,
    /** The container has allocation permission, but allocation failed. */
    CCC_RESULT_ALLOCATOR_ERROR,
    /** Bad arguments have been provided and operation returned early. */
    CCC_RESULT_ARGUMENT_ERROR,
    /** Internal helper, never returned to user. Always last result. */
    CCC_PRIVATE_RESULT_COUNT,
} CCC_Result;

/** @brief A three-way comparison for comparison functions.

A C style three way comparison value (e.g. ((a > b) - (a < b))).
CCC_ORDER_LESSER if left hand side is less than right hand side, CCC_ORDER_EQUAL
if they are equal, and CCC_ORDER_GREATER if left hand side is greater than right
hand side.
*/
typedef enum : int8_t
{
    /** The left hand side is less than the right hand side. */
    CCC_ORDER_LESSER = -1,
    /** The left hand side and right hand side are equal. */
    CCC_ORDER_EQUAL,
    /** The left hand side is greater than the right hand side. */
    CCC_ORDER_GREATER,
    /** Comparison is not possible or other error has occurred. */
    CCC_ORDER_ERROR,
} CCC_Order;

/** @brief A type for returning an unsigned integer from a container for
counting. Intended to count sizes, capacities, and 0-based indices.

Access the fields of this struct directly to check for errors and then use the
returned index. If an error has occurred, the index is invalid. An error will be
indicated by any non-zero value in the error field.

```
CCC_Count res = CCC_bitset_first_trailing_one(&my_bitset);
if (res.error)
{
    // handle errors...
}
(void)CCC_bitset_set(&my_bitset, res.count, CCC_TRUE);
```

Full string explanations of the exact CCC_Result error types can be provided via
the CCC_result_message function if the enum itself does not provide sufficient
explanation. */
typedef struct
{
    /** The error that occurred indicated by a status. 0 (falsey) means OK. */
    CCC_Result error;
    /** The count returned by the operation. */
    size_t count;
} CCC_Count;

/** @brief An element comparison helper.

This type helps the user define the comparison callback function, if the
container takes a standard element comparison function, and helps avoid
swappable argument errors. Any type LHS is considered the left hand side and
any type RHS is the right hand side when considering three-way comparison
return values. Context data is a reference to any context data provided upon
container initialization. */
typedef struct
{
    /** The left hand side for a three-way comparison operation. */
    void const *const type_left;
    /** The right hand side for a three-way comparison operation. */
    void const *const type_right;
    /** A reference to context data provided to container on initialization. */
    void *context;
} CCC_Type_comparator_context;

/** @brief A key comparison helper to avoid argument swapping.

The key is considered the left hand side of the operation if three-way
comparison is needed. Note a comparison is taking place between the key on the
left hand side and the complete user type on the right hand side. This means the
right hand side will need to manually access its key field.

```
int const *const my_key_left = order.key_left;
struct key_val const *const my_type_right  = order.type_right;
return (*my_key_left > my_type_right->key) - (*my_key_left <
my_type_right->key);
```

Notice that the user type must access its key field of its struct. Comparison
must happen this way to support searching by key in associative containers
rather than by entire user struct. Only needing to provide a key can save
significant memory for a search depending on the size of the user type. */
typedef struct
{
    /** Key matching the key field of the provided type to the container. */
    void const *const key_left;
    /** The complete user type stored in the container. */
    void const *const type_right;
    /** A reference to context data provided to the container on initialization.
     */
    void *context;
} CCC_Key_comparator_context;

/** @brief A reference to a user type within the container.

This is to help users define callback functions that act on each node in a
container. For example, a destruct function will use this type. */
typedef struct
{
    /** The user type being stored in the container. */
    void *type;
    /** A reference to context data provided to the container on initialization.
     */
    void *context;
} CCC_Type_context;

/** @brief A read only reference to a key type matching the key field type used
for hash containers.

A reference to any context data is also provided. This the struct one can use
to hash their values with their hash function. */
typedef struct
{
    /** A reference to the same type used for keys in the container. */
    void const *const key;
    /** A reference to context data provided to the container on initialization.
     */
    void *context;
} CCC_Key_context;

/** @brief A bundle of arguments to pass to the user-implemented Allocator
function interface. This ensures clarity in inputs and expected outputs to
an allocator function the user wishes to use for managing containers.
Additional context can be provided for more complex allocation schemes. */
typedef struct
{
    /** The input to the allocation function. NULL or previously allocated. */
    void *input;
    /** The bytes being requested from the allocator. 0 is a free request. */
    size_t bytes;
    /** Additional state to pass to the allocator to help manage memory. */
    void *context;
} CCC_Allocator_context;

/** @brief An allocation function at the core of all containers.

An allocation function implements the following behavior, when it has been
passed an allocator context. Context is passed to a container upon its
initialization and the programmer may choose how to best utilize this reference
(more on context later).

- If input is NULL and bytes 0, NULL is returned.
- If input is NULL with non-zero bytes, new memory is allocated/returned.
- If input is non-NULL it has been previously allocated by the Allocator.
- If input is non-NULL with non-zero size, input is resized to at least bytes
  size. The pointer returned is NULL if resizing fails. Upon success, the
  pointer returned might not be equal to the pointer provided.
- If input is non-NULL and size is 0, input is freed and NULL is returned.

One may be tempted to use realloc to check all of these boxes but realloc is
implementation defined on some of these points. So, the context parameter also
discourages users from providing realloc. For example, one solution using the
standard library allocator might be implemented as follows (context is not
needed):

```
void *
std_allocate(CCC_Allocator_context const context)
{
    if (!context.input && !context.bytes)
    {
        return NULL;
    }
    if (!context.input)
    {
        return malloc(context.bytes);
    }
    if (!context.bytes)
    {
        free(context.input);
        return NULL;
    }
    return realloc(context.input, context.bytes);
}
```

However, the above example is only useful if the standard library allocator
is used. Any allocator that implements the required behavior is sufficient.
For example programs that utilize the context parameter, see the sample
programs. Using custom arena allocators or container compositions are cases when
context is needed. */
typedef void *CCC_Allocator(CCC_Allocator_context);

/** @brief A callback function for comparing two elements in a container.

A three-way comparison return value is expected and the two containers being
compared are guaranteed to be non-NULL and pointing to the base of the user type
stored in the container. Context may be NULL if no context is provided on
initialization. */
typedef CCC_Order CCC_Type_comparator(CCC_Type_comparator_context);

/** @brief A callback function for modifying an element in the container.

A reference to the container type and any context data provided on
initialization is available. The container pointer points to the base of the
user type and is not NULL. Context may be NULL if no context is provided on
initialization. An update function is used when a container Interface exposes
functions to modify the key or value used to determine sorted order of elements
in the container. */
typedef void CCC_Type_modifier(CCC_Type_context);

/** @brief A callback function for destroying an element in the container.

A reference to the container type and any context data provided on
initialization is available. The container pointer points to the base of the
user type and is not NULL. Context may be NULL if no context is provided on
initialization. A destructor function is used to act on each element of the
container when it is being emptied and destroyed. The function will be called on
each type after it removed from the container and before it is freed by the
container, if allocation permission is provided to the container. Therefore, if
the user has given permission to the container to allocate memory they can
assume the container will free each element with the provided allocation
function; this function can be used for any other program state to be maintained
before the container frees. If the user has not given permission to the
container to allocate memory, this a good function in which to free each
element, if desired; any program state can be maintained then the element can be
freed by the user in this function as the final step. */
typedef void CCC_Type_destructor(CCC_Type_context);

/** @brief A callback function for three-way comparing two stored keys.

The key is considered the left hand side of the comparison. The function should
return CCC_ORDER_LESSER if the key is less than the key in key field of user
type, CCC_ORDER_EQUAL if equal, and CCC_ORDER_GREATER if greater. */
typedef CCC_Order CCC_Key_comparator(CCC_Key_comparator_context);

/** @brief A callback function to hash the key type used in a container.

A reference to any context data provided on initialization is also available.
Return the complete hash value as determined by the user hashing algorithm. */
typedef uint64_t CCC_Key_hasher(CCC_Key_context);

/**@}*/

/** @name Entry Interface
The generic interface for associative container entries. */
/**@{*/

/** @brief Determine if an entry is Occupied in the container.
@param[in] entry the pointer to the entry obtained from a container.
@return true if Occupied false if Vacant. Error if entry is NULL. */
CCC_Tribool CCC_entry_occupied(CCC_Entry const *entry);

/** @brief Determine if an insertion error has occurred when a function that
attempts to insert a value in a container is used.
@param[in] entry the pointer to the entry obtained from a container insert.
@return true if an insertion error occurred usually meaning a insertion should
have occurred but the container did not have permission to allocate new memory
or allocation failed. Error if entry is NULL. */
CCC_Tribool CCC_entry_insert_error(CCC_Entry const *entry);

/** @brief Determine if an input error has occurred for a function that
generates an entry.
@param[in] entry the pointer to the entry obtained from a container function.
@return true if an input error occurred usually meaning an invalid argument such
as a NULL pointer was provided to a function. Error if entry is NULL. */
CCC_Tribool CCC_entry_input_error(CCC_Entry const *entry);

/** @brief Unwraps the provided entry providing a reference to the user type
obtained from the operation that provides the entry.
@param[in] entry the pointer to the entry obtained from an operation.
@return a reference to the user type stored in the Occupied entry or NULL if
the entry is Vacant or otherwise cannot be viewed.

The expected return value from unwrapping a value will change depending on the
container from which the entry is obtained. Read the documentation for the
container being used to understand what to expect from this function once an
entry is obtained. */
void *CCC_entry_unwrap(CCC_Entry const *entry);

/** @brief Determine if an handle is Occupied in the container.
@param[in] handle the pointer to the handle obtained from a container.
@return true if Occupied false if Vacant. Error if handle is NULL. */
CCC_Tribool CCC_handle_occupied(CCC_Handle const *handle);

/** @brief Determine if an insertion error has occurred when a function that
attempts to insert a value in a container is used.
@param[in] handle the pointer to the handle obtained from a container insert.
@return true if an insertion error occurred usually meaning a insertion should
have occurred but the container did not have permission to allocate new memory
or allocation failed. Error if handle is NULL. */
CCC_Tribool CCC_handle_insert_error(CCC_Handle const *handle);

/** @brief Determine if an input error has occurred for a function that
generates an handle.
@param[in] handle the pointer to the handle obtained from a container function.
@return true if an input error occurred usually meaning an invalid argument such
as a NULL pointer was provided to a function. Error if handle is NULL. */
CCC_Tribool CCC_handle_input_error(CCC_Handle const *handle);

/** @brief Unwraps the provided handle providing a reference to the user type
obtained from the operation that provides the handle.
@param[in] handle the pointer to the handle obtained from an operation.
@return a reference to the user type stored in the Occupied handle or NULL if
the handle is Vacant or otherwise cannot be viewed.

The expected return value from unwrapping a value will change depending on the
container from which the handle is obtained. Read the documentation for the
container being used to understand what to expect from this function once an
handle is obtained. */
CCC_Handle_index CCC_handle_unwrap(CCC_Handle const *handle);

/**@}*/

/** @name Range Interface
The generic range interface for associative containers. */
/**@{*/

/** @brief Obtain a reference to the beginning user element stored in a
container in the provided range.
@param[in] range a pointer to the range.
@return a reference to the user type stored in the container that serves as
the beginning of the range.

Note the beginning of a range may be equivalent to the end or NULL. */
void *CCC_range_begin(CCC_Range const *range);

/** @brief Obtain a reference to the end user element stored in a
container in the provided range.
@param[in] range a pointer to the range.
@return a reference to the user type stored in the container that serves as
the end of the range.

Note the end of a range may be equivalent to the beginning or NULL. Functions
that obtain ranges treat the end as an exclusive bound and therefore it is
undefined to access this element. */
void *CCC_range_end(CCC_Range const *range);

/** @brief Obtain a reference to the reverse beginning user element stored in a
container in the provided range.
@param[in] range a pointer to the range.
@return a reference to the user type stored in the container that serves as
the reverse beginning of the range.

Note the reverse beginning of a range may be equivalent to the reverse end or
NULL. */
void *CCC_range_reverse_begin(CCC_Range_reverse const *range);

/** @brief Obtain a reference to the reverse end user element stored in a
container in the provided range.
@param[in] range a pointer to the range.
@return a reference to the user type stored in the container that serves as
the reverse end of the range.

Note the reverse end of a range may be equivalent to the reverse beginning or
NULL. Functions that obtain ranges treat the reverse end as an exclusive bound
and therefore it is undefined to access this element. */
void *CCC_range_reverse_end(CCC_Range_reverse const *range);

/** @brief Obtain a handle to the beginning user element stored in a
container in the provided range.
@param[in] range a pointer to the range.
@return a handle to the user type stored in the container that serves as
the beginning of the range.

Note the beginning of a range may be equivalent to the end or NULL. */
CCC_Handle_index CCC_handle_range_begin(CCC_Handle_range const *range);

/** @brief Obtain a handle to the end user element stored in a
container in the provided range.
@param[in] range a pointer to the range.
@return a handle to the user type stored in the container that serves as
the end of the range.

Note the end of a range may be equivalent to the beginning or 0. Functions
that obtain ranges treat the end as an exclusive bound and therefore it is
undefined to access this element. */
CCC_Handle_index CCC_handle_range_end(CCC_Handle_range const *range);

/** @brief Obtain a handle to the reverse beginning user element stored in a
container in the provided range.
@param[in] range a pointer to the range.
@return a handle to the user type stored in the container that serves as
the reverse beginning of the range.

Note the reverse beginning of a range may be equivalent to the reverse end or
0. */
CCC_Handle_index
CCC_handle_range_reverse_begin(CCC_Handle_range_reverse const *range);

/** @brief Obtain a handle to the reverse end user element stored in a
container in the provided range.
@param[in] range a pointer to the range.
@return a handle to the user type stored in the container that serves as
the reverse end of the range.

Note the reverse end of a range may be equivalent to the reverse beginning or
0. Functions that obtain ranges treat the reverse end as an exclusive bound
and therefore it is undefined to access this element. */
CCC_Handle_index
CCC_handle_range_reverse_end(CCC_Handle_range_reverse const *range);

/**@}*/

/** @name Status Interface
Functions for obtaining more descriptive status information. */
/**@{*/

/** @brief Obtain a string message with a description of the error returned
from a container operation, possible causes, and possible fixes to such error.
@param[in] result the result obtained from a container operation.
@return a string message of the result. A CCC_RESULT_OK result is an empty
string, the falsey NULL terminator. All other results have a string message.

These messages can be used for logging or to help with debugging by providing
more information for why such a result might be obtained from a container. */
char const *CCC_result_message(CCC_Result result);

/** @brief Obtain the entry status from a generic entry.
@param[in] entry a pointer to the entry.
@return the status stored in the entry after the required action on the
container completes. If entry is NULL an entry input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned. */
CCC_Entry_status CCC_get_entry_status(CCC_Entry const *entry);

/** @brief Obtain the handle status from a generic handle.
@param[in] handle a pointer to the handle.
@return the status stored in the handle after the required action on the
container completes. If handle is NULL an handle input error is returned so
ensure e is non-NULL to avoid an inaccurate status returned. */
CCC_Handle_status CCC_get_handle_status(CCC_Handle const *handle);

/** @brief Obtain a string message with a description of the entry status.
@param[in] status the status obtained from an entry.
@return a string message with more detailed information regarding the status.

Note that status for an entry is relevant when it is first obtained and when
an action completes. Obtaining an entry can provide information on whether
the search yielded an Occupied or Vacant Entry or any errors that may have
occurred. If a function tries to complete an action like insertion or removal
the status can reflect if any errors occurred in this process as well. Usually,
the provided interface gives all the functions needed to check status but these
strings can be used when more details are required. */
char const *CCC_entry_status_message(CCC_Entry_status status);

/** @brief Obtain a string message with a description of the handle status.
@param[in] status the status obtained from an handle.
@return a string message with more detailed information regarding the status.

Note that status for an handle is relevant when it is first obtained and when
an action completes. Obtaining an handle can provide information on whether
the search yielded an Occupied or Vacant handle or any errors that may have
occurred. If a function tries to complete an action like insertion or removal
the status can reflect if any errors occurred in this process as well. Usually,
the provided interface gives all the functions needed to check status but these
strings can be used when more details are required. */
char const *CCC_handle_status_message(CCC_Handle_status status);

/**@}*/

/** Define this directive at the top of a translation unit if shorter names are
desired. By default the ccc prefix is used to avoid namespace clashes. */
#ifdef TYPES_USING_NAMESPACE_CCC
typedef CCC_Range Range;
typedef CCC_Range_reverse Range_reverse;
typedef CCC_Handle_range Handle_range;
typedef CCC_Handle_range_reverse Handle_range_reverse;
typedef CCC_Entry Entry;
typedef CCC_Handle Handle;
typedef CCC_Handle_index Handle_index;
typedef CCC_Result Result;
typedef CCC_Order Order;
typedef CCC_Type_context Type_context;
typedef CCC_Type_comparator_context Type_comparator_context;
typedef CCC_Key_context Key_context;
typedef CCC_Key_comparator_context Key_comparator_context;
typedef CCC_Allocator Allocator;
typedef CCC_Type_comparator Type_comparator;
typedef CCC_Type_modifier Type_modifier;
typedef CCC_Type_destructor Type_destructor;
typedef CCC_Key_comparator Key_comparator;
typedef CCC_Key_hasher Key_hasher;
#    define entry_occupied(entry_pointer) CCC_entry_occupied(entry_pointer)
#    define entry_insert_error(entry_pointer)                                  \
        CCC_entry_insert_error(entry_pointer)
#    define entry_unwrap(entry_pointer) CCC_entry_unwrap(entry_pointer)
#    define get_entry_status(entry_pointer) CCC_get_entry_status(entry_pointer)
#    define entry_status_message(status) CCC_entry_status_message(status)
#    define handle_occupied(handle_pointer) CCC_handle_occupied(handle_pointer)
#    define handle_insert_error(handle_pointer)                                \
        CCC_handle_insert_error(handle_pointer)
#    define handle_unwrap(handle_pointer) CCC_handle_unwrap(handle_pointer)
#    define get_handle_status(handle_pointer)                                  \
        CCC_get_handle_status(handle_pointer)
#    define handle_status_message(status) CCC_handle_status_message(status)
#    ifndef range_begin
#        define range_begin(range_pointer) CCC_range_begin(range_pointer)
#    endif
#    ifndef range_end
#        define range_end(range_pointer) CCC_range_end(range_pointer)
#    endif
#    ifndef range_reverse_begin
#        define range_reverse_begin(range_pointer)                             \
            CCC_range_reverse_begin(range_pointer)
#    endif
#    ifndef range_reverse_end
#        define range_reverse_end(range_pointer)                               \
            CCC_range_reverse_end(range_pointer)
#    endif
#    define handle_range_begin(handle_range_pointer)                           \
        CCC_handle_range_begin(handle_range_pointer)
#    define handle_range_end(handle_range_pointer)                             \
        CCC_handle_range_end(handle_range_pointer)
#    define handle_range_reverse_begin(handle_range_pointer)                   \
        CCC_handle_range_reverse_begin(handle_range_pointer)
#    define handle_range_reverse_end(handle_range_pointer)                     \
        CCC_handle_range_reverse_end(handle_range_pointer)
#    define result_message(res) CCC_result_message(res)
#endif /* TYPES_USING_NAMESPACE_CCC */

#endif /* CCC_TYPES_H */
