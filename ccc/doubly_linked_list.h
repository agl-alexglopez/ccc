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
@brief The Doubly Linked List Interface

A doubly linked list offers efficient push, pop, extract, and erase operations
for elements stored in the list. Notably, for single elements the list can
offer O(1) push front/back, pop front/back, and removal of elements in
arbitrary positions in the list. The cost of this efficiency is higher memory
footprint.

This container offers pointer stability. Also, if the container is not
permitted to allocate all insertion code assumes that the user has allocated
memory appropriately for the element to be inserted; it will not allocate or
free in this case. If allocation is permitted upon initialization the container
will manage the memory as expected on insert or erase operations as defined
by the interface; memory is allocated for insertions and freed for removals.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `CCC_` prefix. */
#ifndef CCC_DOUBLY_LINKED_LIST_H
#define CCC_DOUBLY_LINKED_LIST_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "private/private_doubly_linked_list.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A container offering bidirectional, insert, removal, and iteration.
@warning it is undefined behavior to use an uninitialized container.

A doubly linked list may be stored in the stack, heap, or data segment. Once
initialized it is passed by reference to all functions. A doubly linked list
can be initialized at compile time or runtime. */
typedef struct CCC_Doubly_linked_list CCC_Doubly_linked_list;

/** @brief A doubly linked list intrusive element to embedded in a user type.

It can be used in an allocating or non allocating container. If allocation is
prohibited the container assumes the element is wrapped in pre-allocated
memory with the appropriate lifetime and scope for the user's needs; the
container does not allocate or free in this case. If allocation is allowed
the container will handle copying the data wrapping the element to allocations
and deallocating when necessary. */
typedef struct CCC_Doubly_linked_list_node CCC_Doubly_linked_list_node;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a doubly linked list with its l-value name, type
containing the Doubly_linked_list elems, the field of the doubly_linked_list
elem, allocation function, compare function and any context data needed
for comparison, printing, or destructors.
@param[in] struct_name the type containing the intrusive doubly_linked_list
element.
@param[in] type_intruder_field name of the Doubly_linked_list element in the
containing type.
@param[in] compare the CCC_Type_comparator used to compare list
elements.
@param[in] context_data any context data that will be needed for
comparison, printing, or destruction of elements.
@param[in] allocate the optional allocation function or NULL.
@return the initialized list. Assign to the list directly on the right hand
side of an equality operator. Initialization can occur at runtime or compile
time (e.g. CCC_doubly_linked list = CCC_doubly_linked_list_initialize(...);). */
#define CCC_doubly_linked_list_initialize(struct_name, type_intruder_field,    \
                                          compare, allocate, context_data)     \
    CCC_private_doubly_linked_list_initialize(                                 \
        struct_name, type_intruder_field, compare, allocate, context_data)

/** @brief Initialize a doubly linked list at runtime from a compound literal
array.
@param[in] type_intruder_field the name of the field intruding on user's type.
@param[in] compare the comparison function for the user type.
@param[in] allocate the allocation function required for construction.
@param[in] destroy the optional destructor to run if insertion fails.
@param[in] context_data context data needed for comparison or destruction.
@param[in] compound_literal_array the array of user types to insert into the
map (e.g. (struct My_type[]){ {.val = 1}, {.val = 2}}).
@return the initialized doubly linked list on the right side of an equality
operator (e.g. CCC_Doubly_linked_list list = CCC_doubly_linked_list_from(...);)
@note Elements in the initializer list are pushed back into the list in the
order they appear. Therefore, the order of the doubly linked list will mirror
the order of the elements in the compound literal array. */
#define CCC_doubly_linked_list_from(type_intruder_field, compare, allocate,    \
                                    destroy, context_data,                     \
                                    compound_literal_array...)                 \
    CCC_private_doubly_linked_list_from(type_intruder_field, compare,          \
                                        allocate, destroy, context_data,       \
                                        compound_literal_array)

/**@}*/

/** @name Insert and Remove Interface
Add or remove elements from the doubly linked list. */
/**@{*/

/** @brief  writes contents of type initializer directly to allocated memory at
the back of the list. O(1).
@param[in] list_pointer the address of the doubly linked list.
@param[in] type_compound_literal the r-value initializer of the type to be
inserted in the list. This should match the type containing Doubly_linked_list
elements as a struct member for this list.
@return a reference to the inserted element or NULL if allocation is not
allowed or fails.

Note that it does not make sense to use this method if the list has been
initialized without an allocation function. If the user does not allow
allocation, the contents of new elements to be inserted has been determined by
the user prior to any inserts into the list. */
#define CCC_doubly_linked_list_emplace_back(list_pointer,                      \
                                            type_compound_literal...)          \
    CCC_private_doubly_linked_list_emplace_back(list_pointer,                  \
                                                type_compound_literal)

/** @brief  writes contents of type initializer directly to allocated memory at
the front of the list. O(1).
@param[in] list_pointer the address of the doubly linked list.
@param[in] type_compound_literal the r-value initializer of the type to be
inserted in the list. This should match the type containing Doubly_linked_list
elements as a struct member for this list.
@return a reference to the inserted element or NULL if allocation is not
allowed or fails.

Note that it does not make sense to use this method if the list has been
initialized without an allocation function. If the user does not allow
allocation, the contents of new elements to be inserted has been determined by
the user prior to any inserts into the list. */
#define CCC_doubly_linked_list_emplace_front(list_pointer,                     \
                                             type_compound_literal...)         \
    CCC_private_doubly_linked_list_emplace_front(list_pointer,                 \
                                                 type_compound_literal)

/** @brief Push user type wrapping type_intruder to the front of the list. O(1).
@param[in] list a pointer to the doubly linked list.
@param[in] type_intruder a pointer to the list element.
@return a pointer to the element inserted or NULL if bad input is provided
or allocation fails. */
[[nodiscard]] void *
CCC_doubly_linked_list_push_front(CCC_Doubly_linked_list *list,
                                  CCC_Doubly_linked_list_node *type_intruder);

/** @brief Push user type wrapping type_intruder to the back of the list. O(1).
@param[in] list a pointer to the doubly linked list.
@param[in] type_intruder a pointer to the list element.
@return a pointer to the element inserted or NULL if bad input is provided
or allocation fails. */
[[nodiscard]] void *
CCC_doubly_linked_list_push_back(CCC_Doubly_linked_list *list,
                                 CCC_Doubly_linked_list_node *type_intruder);

/** @brief Insert user type wrapping type_intruder before position_node. O(1).
@param[in] list a pointer to the doubly linked list.
@param[in] position_node a pointer to the list element before which
type_intruder inserts.
@param[in] type_intruder a pointer to the list element.
@return a pointer to the element inserted or NULL if bad input is provided
or allocation fails. */
[[nodiscard]] void *
CCC_doubly_linked_list_insert(CCC_Doubly_linked_list *list,
                              CCC_Doubly_linked_list_node *position_node,
                              CCC_Doubly_linked_list_node *type_intruder);

/** @brief Pop the user type at the front of the list. O(1).
@param[in] list a pointer to the doubly linked list.
@return an ok result if the pop was successful or an error if bad input is
provided or the list is empty.*/
CCC_Result CCC_doubly_linked_list_pop_front(CCC_Doubly_linked_list *list);

/** @brief Pop the user type at the back of the list. O(1).
@param[in] list a pointer to the doubly linked list.
@return an ok result if the pop was successful or an error if bad input is
provided or the list is empty.*/
CCC_Result CCC_doubly_linked_list_pop_back(CCC_Doubly_linked_list *list);

/** @brief Returns the element following an extracted element from the list
without deallocating regardless of allocation permission provided to the
container. O(1).
@param[in] list a pointer to the doubly linked list.
@param[in] type_intruder the handle of an element known to be in the list.
@return a reference to the element in the list following type_intruder or NULL
if the element is the last. NULL is returned if bad input is provided or the
type_intruder is not in the list. */
void *
CCC_doubly_linked_list_extract(CCC_Doubly_linked_list *list,
                               CCC_Doubly_linked_list_node *type_intruder);

/** @brief Returns the element following an erased element from the list. O(1).
@param[in] list a pointer to the doubly linked list.
@param[in] type_intruder the handle of an element known to be in the list.
@return a reference to the element in the list following type_intruder or NULL
if the element is the last. NULL is returned if bad input is provided or the
type_intruder is not in the list. */
void *CCC_doubly_linked_list_erase(CCC_Doubly_linked_list *list,
                                   CCC_Doubly_linked_list_node *type_intruder);

/** @brief Returns the element following an extracted range of elements from the
list. O(N).
@param[in] list a pointer to the doubly linked list.
@param[in] type_intruder_begin the handle of an element known to be in the list
at the start of the range.
@param[in] type_intruder_end the handle of an element known to be in the list at
the end of the range following type_intruder_begin.
@return a reference to the element in the list following type_intruder_end or
NULL if the element is the last. NULL is returned if bad input is provided or
the type_intruder is not in the list.

Note that if the user does not permit the container to allocate they may iterate
through the extracted range in the same way one iterates through a normal list
using the iterator function. If allocation is allowed, all elements from
type_intruder_begin to type_intruder_end will be erased and references
invalidated. */
void *CCC_doubly_linked_list_erase_range(
    CCC_Doubly_linked_list *list,
    CCC_Doubly_linked_list_node *type_intruder_begin,
    CCC_Doubly_linked_list_node *type_intruder_end);

/** @brief Returns the element following an extracted range of elements from the
list without deallocating regardless of allocation permission provided to the
container. O(N).
@param[in] list a pointer to the doubly linked list.
@param[in] type_intruder_begin the handle of an element known to be in the list
at the start of the range.
@param[in] type_intruder_end the handle of an element known to be in the list at
the end of the range following type_intruder_begin.
@return a reference to the element in the list following type_intruder_end or
NULL if the element is the last. NULL is returned if bad input is provided or
the type_intruder is not in the list.

Note that the user may iterate through the extracted range in the same way one
iterates through a normal list using the iterator function. */
void *CCC_doubly_linked_list_extract_range(
    CCC_Doubly_linked_list *list,
    CCC_Doubly_linked_list_node *type_intruder_begin,
    CCC_Doubly_linked_list_node *type_intruder_end);

/** @brief Repositions to_cut before pos. Only list pointers are modified. O(1).
@param[in] position_doubly_linked_list the list to which position belongs.
@param[in] type_intruder_position the position before which to_cut will be
moved.
@param[in] to_cut_doubly_linked_list the list to which to_cut belongs.
@param[in] type_intruder_to_cut the element to cut.
@return ok if the splice is successful or an error if bad input is provided. */
CCC_Result CCC_doubly_linked_list_splice(
    CCC_Doubly_linked_list *position_doubly_linked_list,
    CCC_Doubly_linked_list_node *type_intruder_position,
    CCC_Doubly_linked_list *to_cut_doubly_linked_list,
    CCC_Doubly_linked_list_node *type_intruder_to_cut);

/** @brief Splices the list to cut before the specified position. The range
being cut is exclusive from [start, end), meaning the final element provided is
not move. This is an O(N) operation.
@param[in] position_doubly_linked_list the list to which position belongs.
@param[in] type_intruder_position the position before which the list is moved.
@param[in] to_cut_doubly_linked_list the list to which the range belongs.
@param[in] type_intruder_to_cut_begin the start of the list to splice.
@param[in] type_intruder_to_cut_exclusive_end the exclusive end of the list to
splice, not included in the splice operation.
@return OK if the splice is successful or an error if bad input is provided. */
CCC_Result CCC_doubly_linked_list_splice_range(
    CCC_Doubly_linked_list *position_doubly_linked_list,
    CCC_Doubly_linked_list_node *type_intruder_position,
    CCC_Doubly_linked_list *to_cut_doubly_linked_list,
    CCC_Doubly_linked_list_node *type_intruder_to_cut_begin,
    CCC_Doubly_linked_list_node *type_intruder_to_cut_exclusive_end);

/**@}*/

/** @name Sorting Interface
Sort the container. */
/**@{*/

/** @brief Sorts the doubly linked list in non-decreasing order as defined by
the provided comparison function. `O(N * log(N))` time, `O(1)` space.
@param[in] doubly_linked_list a pointer to the doubly linked list to sort.
@return the result of the sort, usually OK. An arg error if doubly_linked_list
is null. */
CCC_Result
CCC_doubly_linked_list_sort(CCC_Doubly_linked_list *doubly_linked_list);

/** @brief Inserts type_intruder in sorted position according to the
non-decreasing order of the list determined by the user provided comparison
function. `O(1)`.
@param[in] doubly_linked_list a pointer to the doubly linked list.
@param[in] type_intruder a pointer to the element to be inserted in order.
@return a pointer to the element that has been inserted or NULL if allocation
is required and has failed.
@warning this function assumes the list is sorted.

If a non-increasing order is desired, return opposite results from the user
comparison function. If an element is CCC_ORDER_LESSERERS return
CCC_ORDER_GREATER and vice versa. If elements are equal, return CCC_ORDER_EQUAL.
*/
void *CCC_doubly_linked_list_insert_sorted(
    CCC_Doubly_linked_list *doubly_linked_list,
    CCC_Doubly_linked_list_node *type_intruder);

/** @brief Returns true if the list is sorted in non-decreasing order according
to the user provided comparison function.
@param[in] doubly_linked_list a pointer to the singly linked list.
@return CCC_TRUE if the list is sorted CCC_FALSE if not. Error if
doubly_linked_list is NULL.

If a non-increasing order is desired, return opposite results from the user
comparison function. If an element is CCC_ORDER_LESSER return CCC_ORDER_GREATER
and vice versa. If elements are equal, return CCC_ORDER_EQUAL. */
CCC_Tribool CCC_doubly_linked_list_is_sorted(
    CCC_Doubly_linked_list const *doubly_linked_list);

/**@}*/

/** @name Deallocation Interface
Deallocate the container. */
/**@{*/

/** @brief Clear the contents of the list freeing elements, if given allocation
permission. O(N).
@param[in] list a pointer to the doubly linked list.
@param[in] destroy a destructor function to run on each element.
@return ok if the clearing was a success or an input error if list or destroy is
NULL.

Note that if the list is initialized with allocation permission it will free
elements for the user and the destructor function should only perform context
cleanup, otherwise a double free will occur.

If the list has not been given allocation permission the user should free
the list elements with the destructor if they wish to do so. The implementation
ensures the function is called after the element is removed. Otherwise, the user
must manage their elements at their discretion after the list is emptied in
this function. */
CCC_Result CCC_doubly_linked_list_clear(CCC_Doubly_linked_list *list,
                                        CCC_Type_destructor *destroy);

/**@}*/

/** @name Iteration Interface
Iterate through the doubly linked list. */
/**@{*/

/** @brief Return the user type at the start of the list or NULL if empty. O(1).
@param[in] list a pointer to the doubly linked list.
@return a pointer to the user type or NULL if empty or bad input. */
[[nodiscard]] void *
CCC_doubly_linked_list_begin(CCC_Doubly_linked_list const *list);

/** @brief Return the user type at the end of the list or NULL if empty. O(1).
@param[in] list a pointer to the doubly linked list.
@return a pointer to the user type or NULL if empty or bad input. */
[[nodiscard]] void *
CCC_doubly_linked_list_reverse_begin(CCC_Doubly_linked_list const *list);

/** @brief Return the user type following the element known to be in the list.
O(1).
@param[in] list a pointer to the doubly linked list.
@param[in] type_intruder a handle to the list element known to be in the list.
@return a pointer to the element following type_intruder or NULL if no elements
follow or bad input is provided. */
[[nodiscard]] void *
CCC_doubly_linked_list_next(CCC_Doubly_linked_list const *list,
                            CCC_Doubly_linked_list_node const *type_intruder);

/** @brief Return the user type following the element known to be in the list
moving from back to front. O(1).
@param[in] list a pointer to the doubly linked list.
@param[in] type_intruder a handle to the list element known to be in the list.
@return a pointer to the element following type_intruder from back to front or
NULL if no elements follow or bad input is provided. */
[[nodiscard]] void *CCC_doubly_linked_list_reverse_next(
    CCC_Doubly_linked_list const *list,
    CCC_Doubly_linked_list_node const *type_intruder);

/** @brief Return the end sentinel with no accessible fields. O(1).
@param[in] list a pointer to the doubly linked list.
@return a pointer to the end sentinel with no accessible fields. */
[[nodiscard]] void *
CCC_doubly_linked_list_end(CCC_Doubly_linked_list const *list);

/** @brief Return the start sentinel with no accessible fields. O(1).
@param[in] list a pointer to the doubly linked list.
@return a pointer to the start sentinel with no accessible fields. */
[[nodiscard]] void *
CCC_doubly_linked_list_reverse_end(CCC_Doubly_linked_list const *list);

/**@}*/

/** @name State Interface
Obtain state from the doubly linked list. */
/**@{*/

/** @brief Returns the user type at the front of the list. O(1).
@param[in] list a pointer to the doubly linked list.
@return a pointer to the user type at the front of the list. NULL if empty. */
[[nodiscard]] void *
CCC_doubly_linked_list_front(CCC_Doubly_linked_list const *list);

/** @brief Returns the user type at the back of the list. O(1).
@param[in] list a pointer to the doubly linked list.
@return a pointer to the user type at the back of the list. NULL if empty. */
[[nodiscard]] void *
CCC_doubly_linked_list_back(CCC_Doubly_linked_list const *list);

/** @brief Return a handle to the list element at the front of the list which
may be the sentinel. O(1).
@param[in] list a pointer to the doubly linked list.
@return a pointer to the list element at the beginning of the list which may be
the sentinel but will not be NULL unless a NULL pointer is provided as l. */
[[nodiscard]] CCC_Doubly_linked_list_node *
CCC_doubly_linked_list_node_begin(CCC_Doubly_linked_list const *list);

/** @brief Return the count of elements in the list. O(1).
@param[in] list a pointer to the doubly linked list.
@return the size of the list. An argument error is set if list is NULL. */
[[nodiscard]] CCC_Count
CCC_doubly_linked_list_count(CCC_Doubly_linked_list const *list);

/** @brief Return if the size of the list is equal to 0. O(1).
@param[in] list a pointer to the doubly linked list.
@return true if the size is 0, else false. Error if list is NULL. */
[[nodiscard]] CCC_Tribool
CCC_doubly_linked_list_is_empty(CCC_Doubly_linked_list const *list);

/** @brief Validates internal state of the list.
@param[in] list a pointer to the doubly linked list.
@return true if invariants hold, false if not. Error if list is NULL. */
[[nodiscard]] CCC_Tribool
CCC_doubly_linked_list_validate(CCC_Doubly_linked_list const *list);

/**@}*/

/** Define this preprocessor directive before including this header if shorter
names are desired for the doubly linked list container. Ensure no namespace
clashes exist prior to name shortening. */
#ifdef DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC
typedef CCC_Doubly_linked_list_node Doubly_linked_list_node;
typedef CCC_Doubly_linked_list Doubly_linked_list;
#    define doubly_linked_list_initialize(args...)                             \
        CCC_doubly_linked_list_initialize(args)
#    define doubly_linked_list_emplace_back(args...)                           \
        CCC_doubly_linked_list_emplace_back(args)
#    define doubly_linked_list_emplace_front(args...)                          \
        CCC_doubly_linked_list_emplace_front(args)
#    define doubly_linked_list_push_front(args...)                             \
        CCC_doubly_linked_list_push_front(args)
#    define doubly_linked_list_push_back(args...)                              \
        CCC_doubly_linked_list_push_back(args)
#    define doubly_linked_list_front(args...) CCC_doubly_linked_list_front(args)
#    define doubly_linked_list_back(args...) CCC_doubly_linked_list_back(args)
#    define doubly_linked_list_pop_front(args...)                              \
        CCC_doubly_linked_list_pop_front(args)
#    define doubly_linked_list_pop_back(args...)                               \
        CCC_doubly_linked_list_pop_back(args)
#    define doubly_linked_list_extract(args...)                                \
        CCC_doubly_linked_list_extract(args)
#    define doubly_linked_list_extract_range(args...)                          \
        CCC_doubly_linked_list_extract_range(args)
#    define doubly_linked_list_erase(args...) CCC_doubly_linked_list_erase(args)
#    define doubly_linked_list_erase_range(args...)                            \
        CCC_doubly_linked_list_erase_range(args)
#    define doubly_linked_list_splice(args...)                                 \
        CCC_doubly_linked_list_splice(args)
#    define doubly_linked_list_splice_range(args...)                           \
        CCC_doubly_linked_list_splice_range(args)
#    define doubly_linked_list_sort(args...) CCC_doubly_linked_list_sort(args)
#    define doubly_linked_list_insert_sorted(args...)                          \
        CCC_doubly_linked_list_insert_sorted(args)
#    define doubly_linked_list_is_sorted(args...)                              \
        CCC_doubly_linked_list_is_sorted(args)
#    define doubly_linked_list_begin(args...) CCC_doubly_linked_list_begin(args)
#    define doubly_linked_list_next(args...) CCC_doubly_linked_list_next(args)
#    define doubly_linked_list_reverse_begin(args...)                          \
        CCC_doubly_linked_list_reverse_begin(args)
#    define doubly_linked_list_reverse_next(args...)                           \
        CCC_doubly_linked_list_reverse_next(args)
#    define doubly_linked_list_end(args...) CCC_doubly_linked_list_end(args)
#    define doubly_linked_list_reverse_end(args...)                            \
        CCC_doubly_linked_list_reverse_end(args)
#    define doubly_linked_list_node_begin(args...)                             \
        CCC_doubly_linked_list_node_begin(args)
#    define doubly_linked_list_count(args...) CCC_doubly_linked_list_count(args)
#    define doubly_linked_list_is_empty(args...)                               \
        CCC_doubly_linked_list_is_empty(args)
#    define doubly_linked_list_clear(args...) CCC_doubly_linked_list_clear(args)
#    define doubly_linked_list_validate(args...)                               \
        CCC_doubly_linked_list_validate(args)
#endif /* DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC */

#endif /* CCC_LIST_H */
