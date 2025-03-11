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
typedef union ccc_range_ ccc_range;

/** @brief The result of a rrange query on iterable containers.

A rrange provides a view all elements that fit the equals rrange criteria
of search-by-key containers. Use the provided range iteration functions in
this header to iterate from beginning to end in reverse order relative to
the containers default ordering. */
typedef union ccc_rrange_ ccc_rrange;

/** @brief An Occupied or Vacant position in a searchable container.

A entry is the basis for more complex container specific Entry Interface for
all search-by-key containers. An entry is returned from various operations
to provide both a reference to data and any auxiliary status that is
important for the user. An entry can be Occupied or Vacant. See individual
headers for containers that return this type for its meaning in context. */
typedef union ccc_entry_ ccc_entry;

/** @brief The status monitoring and entry state once it is obtained.

To manage safe and efficient views into associative containers entries use
status flags internally. The provided functions in the Entry Interface for
each container are sufficient to obtain the needed status. However if more
information is needed, the status can be passed to the ccc_entry_status_msg()
function for detailed string messages regarding the entry status. This may
be helpful for debugging or logging. */
typedef enum ccc_entry_status_ ccc_entry_status;

/** @brief An Occupied or Vacant handle to a flat searchable container entry.

A handle uses the same semantics as an entry. However, the wrapped value is
a ccc_handle_i index. When this type is returned the container interface is
promising that this element will remain at the returned handle index until the
element is removed by the user. This is similar to pointer stability but offers
a stronger guarantee that will hold even if the underlying container is
resized. */
typedef union ccc_handle_ ccc_handle;

/** @brief A stable index to user data in a container that uses a flat array as
the underlying storage method.

User data at a handle position in an array remains valid until that element is
removed from the container. This means that resizing of the underlying array
may occur, but the handle index remains valid regardless.

This is similar to pointer stability except that pointers would not remain valid
when the underlying array is resized; a handle remains valid because it is an
index not a pointer. */
typedef size_t ccc_handle_i;

/** @brief The status monitoring and handle state once it is obtained.

To manage safe and efficient views into associative containers entries use
status flags internally. The provided functions in the Handle Interface for
each container are sufficient to obtain the needed status. However if more
information is needed, the status can be passed to the ccc_entry_status_msg()
function for detailed string messages regarding the handle status. This may
be helpful for debugging or logging. */
typedef enum ccc_entry_status_ ccc_handle_status;

/** @brief A three state boolean to allow for an error state. Error is -1, False
is 0, and True is 1.

Some containers conceptually take or return a boolean value as part of their
operations. However, booleans cannot indicate errors and this library offers
no errno or C++ throw-like behavior. Therefore, a three state value can offer
additional information while still maintaining the truthy and falsey bool
behavior one would normally expect. The chosen values also allow the user to
implement Three-valued Logic if desired as follows:

LOGIC      IMPLEMENTATION
NOT(A)   = NEG(A)
AND(A,B) = MIN(A,B)
OR(A,B)  = MAX(A,B)
XOR(A,B) = MIN(MAX(A,B), NEG(MIN(A,B)))

When interacting with multiple containers the user is free to implement this
logic for the values they return. However, a three branch coding pattern usually
suffice: `if (result < 0) {} else if (result) {} else {}`. */
typedef enum : int8_t
{
    /** Intended value if CCC_FALSE or CCC_TRUE could not be returned. */
    CCC_BOOL_ERR = -1,
    /** Equivalent to boolean false, guaranteed to be falsey aka 0. */
    CCC_FALSE,
    /** Equivalent to boolean true, guaranteed to be truthy aka 1. */
    CCC_TRUE,
} ccc_tribool;

/** @brief A result of actions on containers.

A result indicates the status of the requested operation. Each container
provides status messages according to the result type returned from a operation
that uses this type. */
typedef enum
{
    /** The operation has occurred without error. */
    CCC_OK = 0,
    /** Memory is needed but the container lacks allocation permission. */
    CCC_NO_ALLOC,
    /** The container has allocation permission, but allocation failed. */
    CCC_MEM_ERR,
    /** Bad arguments have been provided to an operation. */
    CCC_INPUT_ERR,
    /** Internal helper, never returned to user. Always last result. */
    CCC_RESULTS_SIZE,
} ccc_result;

/** @brief A three-way comparison for comparison functions.

A C style three way comparison value (e.g. ((a > b) - (a < b))). CCC_LES if
left hand side is less than right hand side, CCC_EQL if they are equal, and
CCC_GRT if left hand side is greater than right hand side. */
typedef enum
{
    /** The left hand side is less than the right hand side. */
    CCC_LES = -1,
    /** The left hand side and right hand side are equal. */
    CCC_EQL,
    /** The left hand side is greater than the right hand side. */
    CCC_GRT,
    /** Comparison is not possible or other error has occurred. */
    CCC_CMP_ERR,
} ccc_threeway_cmp;

/** @brief An element comparison helper.

This type helps the user define the comparison callback function, if the
container takes a standard element comparison function, and helps avoid
swappable argument errors. User type LHS is considered the left hand side and
user type RHS is the right hand side when considering three-way comparison
return values. Aux data is a reference to any auxiliary data provided upon
container initialization. */
typedef struct
{
    /** The left hand side for a three-way comparison operation. */
    void const *const user_type_lhs;
    /** The right hand side for a three-way comparison operation. */
    void const *const user_type_rhs;
    /** A reference to aux data provided to container on initialization. */
    void *aux;
} ccc_cmp;

/** @brief A key comparison helper to avoid argument swapping.

The key is considered the left hand side of the operation if three-way
comparison is needed. Left and right do not matter if equality is needed. */
typedef struct
{
    /** Key matching the key field of the provided type to the container. */
    void const *const key_lhs;
    /** The complete user type stored in the container. */
    void const *const user_type_rhs;
    /** A reference to aux data provided to the container on initialization. */
    void *aux;
} ccc_key_cmp;

/** @brief A reference to a user type within the container.

This is to help users define callback functions that act on each node in a
container. For example, a destruct function will use this type. */
typedef struct
{
    /** The user type being stored in the container. */
    void *user_type;
    /** A reference to aux data provided to the container on initialization. */
    void *aux;
} ccc_user_type;

/** @brief A read only reference to a key type matching the key field type used
for hash containers.

A reference to any auxiliary data is also provided. This the struct one can use
to hash their values with their hash function. */
typedef struct
{
    /** A reference to the same type used for keys in the container. */
    void const *const user_key;
    /** A reference to aux data provided to the container on initialization. */
    void *aux;
} ccc_user_key;

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
typedef void *ccc_alloc_fn(void *ptr, size_t size, void *aux);

/** @brief A callback function for comparing two elements in a container.

A three-way comparison return value is expected and the two containers being
compared are guaranteed to be non-NULL and pointing to the base of the user type
stored in the container. Aux may be NULL if no aux is provided on
initialization. */
typedef ccc_threeway_cmp ccc_cmp_fn(ccc_cmp);

/** @brief A callback function for modifying an element in the container.

A reference to the container type and any aux data provided on initialization
is available. The container pointer points to the base of the user type and is
not NULL. Aux may be NULL if no aux is provided on initialization. An update
function is used when a container Interface exposes functions to modify the key
or value used to determine sorted order of elements in the container. */
typedef void ccc_update_fn(ccc_user_type);

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
typedef void ccc_destructor_fn(ccc_user_type);

/** @brief A callback function to determining equality between two stored keys.

The function should return CCC_TRUE if the key and key field in the user type
are equivalent, else CCC_FALSE.
@note a callback need not return CCC_BOOL_ERR as the container code always
provides data to the arguments of the function invariantly. */
typedef ccc_tribool ccc_key_eq_fn(ccc_key_cmp);

/** @brief A callback function for three-way comparing two stored keys.

The key is considered the left hand side of the comparison. The function should
return CCC_LES if the key is less than the key in key field of user type,
CCC_EQL if equal, and CCC_GRT if greater. */
typedef ccc_threeway_cmp ccc_key_cmp_fn(ccc_key_cmp);

/** @brief A callback function to hash the key type used in a container.

A reference to any aux data provided on initialization is also available.
Return the complete hash value as determined by the user hashing algorithm. */
typedef uint64_t ccc_hash_fn(ccc_user_key to_hash);

/**@}*/

/** @name Entry Interface
The generic interface for associative container entries. */
/**@{*/

/** @brief Determine if an entry is Occupied in the container.
@param [in] e the pointer to the entry obtained from a container.
@return true if Occupied false if Vacant. Error if e is NULL. */
ccc_tribool ccc_entry_occupied(ccc_entry const *e);

/** @brief Determine if an insertion error has occurred when a function that
attempts to insert a value in a container is used.
@param [in] e the pointer to the entry obtained from a container insert.
@return true if an insertion error occurred usually meaning a insertion should
have occurred but the container did not have permission to allocate new memory
or allocation failed. Error if e is NULL. */
ccc_tribool ccc_entry_insert_error(ccc_entry const *e);

/** @brief Determine if an input error has occurred for a function that
generates an entry.
@param [in] e the pointer to the entry obtained from a container function.
@return true if an input error occurred usually meaning an invalid argument such
as a NULL pointer was provided to a function. Error if e is NULL. */
ccc_tribool ccc_entry_input_error(ccc_entry const *e);

/** @brief Unwraps the provided entry providing a reference to the user type
obtained from the operation that provides the entry.
@param [in] e the pointer to the entry obtained from an operation.
@return a reference to the user type stored in the Occupied entry or NULL if
the entry is Vacant or otherwise cannot be viewed.

The expected return value from unwrapping a value will change depending on the
container from which the entry is obtained. Read the documentation for the
container being used to understand what to expect from this function once an
entry is obtained. */
void *ccc_entry_unwrap(ccc_entry const *e);

/** @brief Determine if an handle is Occupied in the container.
@param [in] e the pointer to the handle obtained from a container.
@return true if Occupied false if Vacant. Error if e is NULL. */
ccc_tribool ccc_handle_occupied(ccc_handle const *e);

/** @brief Determine if an insertion error has occurred when a function that
attempts to insert a value in a container is used.
@param [in] e the pointer to the handle obtained from a container insert.
@return true if an insertion error occurred usually meaning a insertion should
have occurred but the container did not have permission to allocate new memory
or allocation failed. Error if e is NULL. */
ccc_tribool ccc_handle_insert_error(ccc_handle const *e);

/** @brief Determine if an input error has occurred for a function that
generates an handle.
@param [in] e the pointer to the handle obtained from a container function.
@return true if an input error occurred usually meaning an invalid argument such
as a NULL pointer was provided to a function. Error if e is NULL. */
ccc_tribool ccc_handle_input_error(ccc_handle const *e);

/** @brief Unwraps the provided handle providing a reference to the user type
obtained from the operation that provides the handle.
@param [in] e the pointer to the handle obtained from an operation.
@return a reference to the user type stored in the Occupied handle or NULL if
the handle is Vacant or otherwise cannot be viewed.

The expected return value from unwrapping a value will change depending on the
container from which the handle is obtained. Read the documentation for the
container being used to understand what to expect from this function once an
handle is obtained. */
ccc_handle_i ccc_handle_unwrap(ccc_handle const *e);

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
void *ccc_begin_range(ccc_range const *r);

/** @brief Obtain a reference to the end user element stored in a
container in the provided range.
@param [in] r a pointer to the range.
@return a reference to the user type stored in the container that serves as
the end of the range.

Note the end of a range may be equivalent to the beginning or NULL. Functions
that obtain ranges treat the end as an exclusive bound and therefore it is
undefined to access this element. */
void *ccc_end_range(ccc_range const *r);

/** @brief Obtain a reference to the reverse beginning user element stored in a
container in the provided range.
@param [in] r a pointer to the range.
@return a reference to the user type stored in the container that serves as
the reverse beginning of the range.

Note the reverse beginning of a range may be equivalent to the reverse end or
NULL. */
void *ccc_rbegin_rrange(ccc_rrange const *r);

/** @brief Obtain a reference to the reverse end user element stored in a
container in the provided range.
@param [in] r a pointer to the range.
@return a reference to the user type stored in the container that serves as
the reverse end of the range.

Note the reverse end of a range may be equivalent to the reverse beginning or
NULL. Functions that obtain ranges treat the reverse end as an exclusive bound
and therefore it is undefined to access this element. */
void *ccc_rend_rrange(ccc_rrange const *r);

/**@}*/

/** @name Status Interface
Functions for obtaining more descriptive status information. */
/**@{*/

/** @brief Obtain a string message with a description of the error returned
from a container operation, possible causes, and possible fixes to such error.
@param [in] res the result obtained from a container operation.
@return a string message of the result. A CCC_OK result is an empty string,
the falsey NULL terminator. All other results have a string message.

These messages can be used for logging or to help with debugging by providing
more information for why such a result might be obtained from a container. */
char const *ccc_result_msg(ccc_result res);

/** @brief Obtain the entry status from a generic entry.
@param [in] e a pointer to the entry.
@return the status stored in the entry after the required action on the
container completes. If e is NULL an entry input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned. */
ccc_entry_status ccc_get_entry_status(ccc_entry const *e);

/** @brief Obtain the handle status from a generic handle.
@param [in] e a pointer to the handle.
@return the status stored in the handle after the required action on the
container completes. If e is NULL an handle input error is returned so ensure
e is non-NULL to avoid an inaccurate status returned. */
ccc_handle_status ccc_get_handle_status(ccc_handle const *e);

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
char const *ccc_entry_status_msg(ccc_entry_status status);

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
char const *ccc_handle_status_msg(ccc_handle_status status);

/**@}*/

/** Define this directive at the top of a translation unit if shorter names are
desired. By default the ccc prefix is used to avoid namespace clashes. */
#ifdef TYPES_USING_NAMESPACE_CCC
typedef ccc_range range;
typedef ccc_rrange rrange;
typedef ccc_entry entry;
typedef ccc_handle handle;
typedef ccc_handle_i handle_i;
typedef ccc_result result;
typedef ccc_threeway_cmp threeway_cmp;
typedef ccc_cmp cmp;
typedef ccc_key_cmp key_cmp;
typedef ccc_user_type user_type;
typedef ccc_user_key user_key;
typedef ccc_alloc_fn alloc_fn;
typedef ccc_cmp_fn cmp_fn;
typedef ccc_update_fn update_fn;
typedef ccc_destructor_fn destructor_fn;
typedef ccc_key_eq_fn key_eq_fn;
typedef ccc_key_cmp_fn key_cmp_fn;
typedef ccc_hash_fn hash_fn;
#    define entry_occupied(entry_ptr) ccc_entry_occupied(entry_ptr)
#    define entry_insert_error(entry_ptr) ccc_entry_insert_error(entry_ptr)
#    define entry_unwrap(entry_ptr) ccc_entry_unwrap(entry_ptr)
#    define get_entry_status(entry_ptr) ccc_get_entry_status(entry_ptr)
#    define entry_status_msg(status) ccc_entry_status_msg(status)
#    define handle_occupied(handle_ptr) ccc_handle_occupied(handle_ptr)
#    define handle_insert_error(handle_ptr) ccc_handle_insert_error(handle_ptr)
#    define handle_unwrap(handle_ptr) ccc_handle_unwrap(handle_ptr)
#    define get_handle_status(handle_ptr) ccc_get_handle_status(handle_ptr)
#    define handle_status_msg(status) ccc_handle_status_msg(status)
#    define begin_range(range_ptr) ccc_begin_range(range_ptr)
#    define end_range(range_ptr) ccc_end_range(range_ptr)
#    define rbegin_rrange(range_ptr) ccc_rbegin_rrange(range_ptr)
#    define rend_rrange(range_ptr) ccc_rend_rrange(range_ptr)
#    define result_msg(res) ccc_result_msg(res)
#endif /* TYPES_USING_NAMESPACE_CCC */

#endif /* CCC_TYPES_H */
