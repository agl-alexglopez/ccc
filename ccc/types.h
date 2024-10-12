/** This file contains definitions and functions for types used across all
containers in the C Container Collection Library. */
#ifndef CCC_TYPES_H
#define CCC_TYPES_H

#include "impl_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** A range provides a view all elements that fit the equals range criteria
of search-by-key containers. Use the provided range iteration functions in
this header to iterate from beginning to end in forward order relative to
the containers default ordering. */
typedef union
{
    struct ccc_range_ impl_;
} ccc_range;

/** A rrange provides a view all elements that fit the equals rrange criteria
of search-by-key containers. Use the provided range iteration functions in
this header to iterate from beginning to end in reverse order relative to
the containers default ordering. */
typedef union
{
    struct ccc_range_ impl_;
} ccc_rrange;

/** A entry is the basis for more complex container specific Entry API's for
all search-by-key containers. An entry is returned from various operations
to provide both a reference to data and any auxilliary status that is
important for the user. An entry can be Occupied or Vacant. See individual
headers for containers that return this type for its meaning in context. */
typedef union
{
    struct ccc_entry_ impl_;
} ccc_entry;

/** A result indicates the status of the requested operation. Each container
provides status messages according to the result type returned from a operation
that uses this type. */
typedef enum
{
    /** The operation has occured without error. */
    CCC_OK = 0,
    /** Memory is needed but the container lacks allocation permission. */
    CCC_NO_REALLOC,
    /** The container has allocation permission, but allocation failed. */
    CCC_MEM_ERR,
    /** Bad arguments have been provided to an operation. */
    CCC_INPUT_ERR,
    /** The operation has had no effect, nothing executes. */
    CCC_NOP,
} ccc_result;

/** A C style threeway comparison value (e.g. ((a > b) - (a < b))). CCC_LES if
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
    /** Comparison is not possible or other error has occured. */
    CCC_CMP_ERR,
} ccc_threeway_cmp;

/** An element comparison helper. This type helps the user define the
comparison callback function, if the container takes a standard element
comparison function, and helps avoid swappable argument errors. Use type
lhs is considered the left hand side and user type rhs is the right hand side
when considering threeway comparison return values. Aux data is a reference
to any auxilliary data provided upon container initialization. */
typedef struct
{
    /** The left hand side for a threeway comparison operation. */
    void const *const user_type_lhs;
    /** The right hand side for a threeway comparison operation. */
    void const *const user_type_rhs;
    /** A reference to aux data provided to container on initialization. */
    void *aux;
} ccc_cmp;

/** A key comparison helper to avoid argument swapping. The key is considered
the left hand side of the operation if threeway comparison is needed. Left
and right do not matter if simply equality is needed. */
typedef struct
{
    /** Key matching the key field of the provided type to the container. */
    void const *const key_lhs;
    /** The complete user type stored in the container. */
    void const *const user_type_rhs;
    /** A reference to aux data provided to the container on initialization. */
    void *aux;
} ccc_key_cmp;

/** A read only reference to a user type within the container. This is to help
users define callback functions that act on each node in a container. For
example, a printer function will use this type. */
typedef struct
{
    /** The user type being stored in the container. */
    void const *const user_type;
    /** A reference to aux data provided to the container on initialization. */
    void *aux;
} ccc_user_type;

/** A reference to a user type within the container. This is to help users
define callback functions that act on each node in a container. For example, a
destruct function will use this type. */
typedef struct
{
    /** The user type being stored in the container. */
    void *user_type;
    /** A reference to aux data provided to the container on initialization. */
    void *aux;
} ccc_user_type_mut;

/** A read only reference to a key type matching the key field type used
for hash containers. A reference to any auxilliary data is also provided.
This the struct one can use to hash their values with their hash function. */
typedef struct
{
    /** A reference to the same type used for keys in the container. */
    void const *const user_key;
    /** A reference to aux data provided to the container on initialization. */
    void *aux;
} ccc_user_key;

/** An allocation function is at the core of this library and all containers.
An allocation function implements the following API, where void * is pointer
to memory and size_t is number of bytes to allocate.
- If NULL is provided with a size of 0, NULL is returned.
- If NULL is provided with a non-zero size new memory is allocated/returned.
- If void * is non-NULL it has been previously allocated by the alloc fn.
- If void * is non-NULL with non-zero size void * is resized to at least size
  bytes. The pointer returned is NULL if resizing fails. Upon success the
  pointer returned may not be equal to the pointer provided.
- If void * is non-NULL and size is 0 memory is freed and NULL is returned.
A function that implements such behavior on many platforms is realloc. If one
is not sure that realloc implements all such behaviors, wrap it in a helper
function. For example, one solution might be implemented as follows:

void *
alloc_fn(void *const mem, size_t const bytes)
{
    if (!mem && !bytes)
    {
        return NULL;
    }
    if (!mem)
    {
        return malloc(bytes);
    }
    if (!bytes)
    {
        free(mem);
        return NULL;
    }
    return realloc(mem, bytes);
}

However, the above example is only useful if the standard library allocator
is used. Any allocator that implements the required behavior is sufficient. */
typedef void *ccc_alloc_fn(void *, size_t);

/** A callback function for comparing two elements in a container. A threeway
comparison return value is expected and the two containers being compared
are guaranteed to be non-null and pointing to the base of the user type stored
in the container. Aux may be NULL if no aux is provided on initialization. */
typedef ccc_threeway_cmp ccc_cmp_fn(ccc_cmp);

/** A callback function for printing an individual element in the container.
A read-only reference to the container type and any aux data provided on
initialization is available. The container pointer points to the base of the
user type and is not NULL. Aux may be NULL if no aux is provided on
initialization. */
typedef void ccc_print_fn(ccc_user_type);

/** A callback function for modifying an individual element in the container.
A reference to the container type and any aux data provided on initialization
is available. The container pointer points to the base of the user type and is
not NULL. Aux may be NULL if no aux is provided on initialization. An update
function is used when a container api exposes functions to modify the key
or value used to determine sorted order of elements in the container. */
typedef void ccc_update_fn(ccc_user_type_mut);

/** A callback function for modifying an individual element in the container.
A reference to the container type and any aux data provided on initialization
is available. The container pointer points to the base of the user type and is
not NULL. Aux may be NULL if no aux is provided on initialization. A destructor
function is used to act on each element of the container when it is being
emptied and destroyed. The function will be called on each type after it
removed fromt the container and before it is freed by the container, if
allocation permission is provided to the container. Therefore, if the user
has given permission to the container to allocate memory they can assume the
container will free each element with the provided allocation function; this
function can be used for any other program state to be maintained before the
container frees. If the user has not given permission to the container to
allocate memory, this a good function in which to free each element, if
desired; any program state can be maintained then the element can be freed by
the user in this function as the final step. */
typedef void ccc_destructor_fn(ccc_user_type_mut);

/** A callback function for determining equality between a key and the key
field of a user type stored in the container. The function should return
true if the key and key field in the user type are equivalent, else false. */
typedef bool ccc_key_eq_fn(ccc_key_cmp);

/** A callback function for comparing a key to the key field in a user type
stored in the container. The key is considered the left hand side of the
comparison. The function should return CCC_LES if the key is less than the
key in key field of user type, CCC_EQL if equal, and CCC_GRT if greater. */
typedef ccc_threeway_cmp ccc_key_cmp_fn(ccc_key_cmp);

/** A callback function to hash the key type used in a container. A reference to
any aux data provided on initialization is also available. Return the complete
hash value as determined by the user hashing algorithm. */
typedef uint64_t ccc_hash_fn(ccc_user_key to_hash);

/** @brief Determine if an entry is Occupied in the container.
@param [in] e the pointer to the entry obtained from a container.
@return true if Occupied false if Vacant. */
bool ccc_entry_occupied(ccc_entry const *e);

/** @brief Determine if an insertion error has occured when a function that
attempts to insert a value in a container is used.
@param [in] e the pointer to the entry obtained from a container insert.
@return true if an insertion error occured usually meaning a insertion should
have occured but the container did not have permission to allocate new memory
or allocation failed. */
bool ccc_entry_insert_error(ccc_entry const *e);

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

/** Define this directive at the top of a translation unit if shorter names are
desired. By default the ccc prefix is used to avoid namespace clashes. */
#ifdef TYPES_USING_NAMESPACE_CCC
typedef ccc_range range;
typedef ccc_rrange rrange;
typedef ccc_entry entry;
typedef ccc_result result;
typedef ccc_threeway_cmp threeway_cmp;
typedef ccc_cmp cmp;
typedef ccc_key_cmp key_cmp;
typedef ccc_user_type user_type;
typedef ccc_user_key user_key;
typedef ccc_alloc_fn alloc_fn;
typedef ccc_cmp_fn cmp_fn;
typedef ccc_print_fn print_fn;
typedef ccc_update_fn update_fn;
typedef ccc_destructor_fn destructor_fn;
typedef ccc_key_eq_fn key_eq_fn;
typedef ccc_key_cmp_fn key_cmp_fn;
typedef ccc_hash_fn hash_fn;
#    define entry_occupied(entry_ptr) ccc_entry_occupied(entry_ptr)
#    define entry_insert_error(entry_ptr) ccc_entry_insert_error(entry_ptr)
#    define entry_unwrap(entry_ptr) ccc_entry_unwrap(entry_ptr)
#    define begin_range(range_ptr) ccc_begin_range(range_ptr)
#    define end_range(range_ptr) ccc_end_range(range_ptr)
#    define rbegin_rrange(range_ptr) ccc_rbegin_rrange(range_ptr)
#    define rend_rrange(range_ptr) ccc_rend_rrange(range_ptr)
#endif /* TYPES_USING_NAMESPACE_CCC */

#endif /* CCC_TYPES_H */
