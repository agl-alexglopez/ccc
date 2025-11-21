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
@brief The Singly Linked List Interface

A singly linked list is well suited for list or stack structures that only need
access to the front or most recently added elements. When compared to a doubly
linked list, the memory overhead per node is smaller but some operations will
have O(N) runtime implications when compared to a similar operation in a doubly
linked list. Review function documentation when unsure of the runtime of an
singly_linked_list operation.

This container offers pointer stability. Also, if the container is not
permitted to allocate all insertion code assumes that the user has allocated
memory appropriately for the element to be inserted; it will not allocate or
free in this case. If allocation is permitted upon initialization the container
will manage the memory as expected on insert or erase operations as defined
by the interface. In this case memory is allocated for insertions and freed for
removals.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `CCC_` prefix. */
#ifndef CCC_SINGLY_LINKED_LIST_H
#define CCC_SINGLY_LINKED_LIST_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "private/private_singly_linked_list.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A low overhead front tracking container with efficient push and pop.

A singly linked list may be stored in the stack, heap, or data segment. Once
Initialized it is passed by reference to all functions. A singly linked list
can be initialized at compile time or runtime. */
typedef struct CCC_Singly_linked_list CCC_Singly_linked_list;

/** @brief A singly linked list intrusive element to embedded in a user type.

It can be used in an allocating or non allocating container. If allocation is
prohibited the container assumes the element is wrapped in pre-allocated
memory with the appropriate lifetime and scope for the user's needs; the
container does not allocate or free in this case. If allocation is allowed
the container will handle copying the data wrapping the element to allocations
and deallocating when necessary. */
typedef struct CCC_Singly_linked_list_node CCC_Singly_linked_list_node;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a singly linked list at compile or runtime.
@param[in] struct_name the user type wrapping the intrusive singly_linked_list
elem.
@param[in] type_intruder_field the name of the field in the user type storing
the intrusive list elem.
@param[in] compare a comparison function for searching or sorting the list.
@param[in] allocate an allocation function if allocation is allowed.
@param[in] context_data a pointer to any context data needed for comparison or
destruction.
@return a stuct initializer for the singly linked list to be assigned
(e.g. CCC_Singly_linked_list l = CCC_singly_linked_list_initialize(...);). */
#define CCC_singly_linked_list_initialize(struct_name, type_intruder_field,    \
                                          compare, allocate, context_data)     \
    CCC_private_singly_linked_list_initialize(                                 \
        struct_name, type_intruder_field, compare, allocate, context_data)

/**@}*/

/** @name Insert and Remove Interface
Add or remove elements from the container. */
/**@{*/

/** @brief Push the type wrapping type_intruder to the front of the list. O(1).
@param[in] list a pointer to the singly linked list.
@param[in] type_intruder a pointer to the intrusive handle in the user type.
@return a pointer to the inserted element or NULL if allocation failed.

Note that if allocation is not allowed the container assumes the memory wrapping
elem has been allocated appropriately and with the correct lifetime by the user.

If allocation is allowed the provided element is copied to a new allocation. */
[[nodiscard]] void *
CCC_singly_linked_list_push_front(CCC_Singly_linked_list *list,
                                  CCC_Singly_linked_list_node *type_intruder);

/** @brief Write a compound literal directly to allocated memory at the front.
O(1).
@param[in] list_pointer a pointer to the singly linked list.
@param[in] type_compound_literal a compound literal containing the elements to
be written to a newly allocated node.
@return a reference to the element pushed to the front or NULL if allocation
failed.

Note that it only makes sense to use this method when the container is given
allocation permission. Otherwise NULL is returned due to an inability for the
container to allocate memory. */
#define CCC_singly_linked_list_emplace_front(list_pointer,                     \
                                             type_compound_literal...)         \
    CCC_private_singly_linked_list_emplace_front(list_pointer,                 \
                                                 type_compound_literal)

/** @brief Pop the front element from the list. O(1).
@param[in] list a pointer to the singly linked list.
@return ok if the list is non-empty and the pop is successful. An input error
is returned if list is NULL or the list is empty. */
CCC_Result CCC_singly_linked_list_pop_front(CCC_Singly_linked_list *list);

/** @brief Inserts splice element after pos. O(N).
@param[in] position_list the list to which position belongs.
@param[in] type_intruder_position the position after which splice will be
inserted.
@param[in] splice_list the list to which splice belongs.
@param[in] type_intruder_splice the element to be moved before pos.
@return ok if the operations is successful. An input error is provided if any
input pointers are NULL.

Note that position_list and splice_singly_linked_list may be the
same or different lists and the invariants of each or the same list will be
maintained by the function. */
CCC_Result CCC_singly_linked_list_splice(
    CCC_Singly_linked_list *position_list,
    CCC_Singly_linked_list_node *type_intruder_position,
    CCC_Singly_linked_list *splice_list,
    CCC_Singly_linked_list_node *type_intruder_splice);

/** @brief Inserts the `[begin, end)` of spliced elements after pos. `O(N)`.
@param[in] position_list the list to which position belongs.
@param[in] type_intruder_position the position after which the range will be
inserted.
@param[in] to_cut_list the list to which the range belongs.
@param[in] type_intruder_to_cut_begin the start of the range.
@param[in] type_intruder_to_cut_exclusive_end the exclusive end of the range.
@return OK if the operations is successful. An input error is provided if any
input pointers are NULL.
@warning position must not be inside of the range `[begin, end)` if
position_list is the same list as splice_singly_linked_list.

Note that position_list and splice_singly_linked_list may be the
same or different lists and the invariants of each or the same list will be
maintained by the function. */
CCC_Result CCC_singly_linked_list_splice_range(
    CCC_Singly_linked_list *position_list,
    CCC_Singly_linked_list_node *type_intruder_position,
    CCC_Singly_linked_list *to_cut_list,
    CCC_Singly_linked_list_node *type_intruder_to_cut_begin,
    CCC_Singly_linked_list_node *type_intruder_to_cut_exclusive_end);

/** @brief Erases type_intruder from the list returning the following element.
O(N).
@param[in] list a pointer to the singly linked list.
@param[in] type_intruder a handle to the intrusive element known to be in the
list.
@return a pointer to the element following type_intruder in the list or NULL if
the list is empty or any bad input is provided to the function.
@warning type_intruder must be in the list.

Note that if allocation permission is given to the container it will free the
element. Otherwise, it is the user's responsibility to free the type wrapping
elem. */
void *CCC_singly_linked_list_erase(CCC_Singly_linked_list *list,
                                   CCC_Singly_linked_list_node *type_intruder);

/** @brief Erases a range from the list returning the element after end. O(N).
@param[in] list a pointer to the singly linked list.
@param[in] type_intruder_begin the start of the range in the list.
@param[in] type_intruder_end the exclusive end of the range in the list.
@return a pointer to the element following the range in the list or NULL if the
list is empty or any bad input is provided to the function.
@warning the provided range must be in the list.

Note that if allocation permission is given to the container it will free the
elements in the range. Otherwise, it is the user's responsibility to free the
types wrapping the range of elements. */
void *CCC_singly_linked_list_erase_range(
    CCC_Singly_linked_list *list,
    CCC_Singly_linked_list_node *type_intruder_begin,
    CCC_Singly_linked_list_node *type_intruder_end);

/** @brief Extracts an element from the list without freeing it. O(N).
@param[in] list a pointer to the singly linked list.
@param[in] type_intruder a handle to the intrusive element known to be in the
list.
@return a pointer to the element following type_intruder in the list.

Note that regardless of allocation permission this method will not free the
type wrapping elem. It only removes it from the list. */
void *
CCC_singly_linked_list_extract(CCC_Singly_linked_list *list,
                               CCC_Singly_linked_list_node *type_intruder);

/** @brief Extracts a range of elements from the list without freeing them.
O(N).
@param[in] list a pointer to the singly linked list.
@param[in] type_intruder_begin the start of the range in the list.
@param[in] type_intruder_end the exclusive end of the range in the list.
@return a pointer to the element following the range of elements in the list.

Note that the range remains in tact and can be iterated as one would iterate
a normal list. However, insertions and removals from a range are not possible
as they no longer belong to any list. */
void *CCC_singly_linked_list_extract_range(
    CCC_Singly_linked_list *list,
    CCC_Singly_linked_list_node *type_intruder_begin,
    CCC_Singly_linked_list_node *type_intruder_end);

/**@}*/

/** @name Sorting Interface
Sort the container. */
/**@{*/

/** @brief Sorts the singly linked list in non-decreasing order as defined by
the provided comparison function. `O(N * log(N))` time, `O(1)` space.
@param[in] list a pointer to the singly linked list to sort.
@return the result of the sort, usually OK. An arg error if singly_linked_list
is null. */
CCC_Result CCC_singly_linked_list_sort(CCC_Singly_linked_list *list);

/** @brief Inserts e in sorted position according to the non-decreasing order
of the list determined by the user provided comparison function.
@param[in] list a pointer to the singly linked list.
@param[in] type_intruder a pointer to the element to be inserted in order.
@return a pointer to the element that has been inserted or NULL if allocation
is required and has failed.
@warning this function assumes the list is sorted.

If a non-increasing order is desired, return opposite results from the user
comparison function. If an element is CCC_ORDER_LESSER return CCC_ORDER_GREATER
and vice versa. If elements are equal, return CCC_ORDER_EQUAL. */
void *CCC_singly_linked_list_insert_sorted(
    CCC_Singly_linked_list *list, CCC_Singly_linked_list_node *type_intruder);

/** @brief Returns true if the list is sorted in non-decreasing order according
to the user provided comparison function.
@param[in] list a pointer to the singly linked list.
@return CCC_TRUE if the list is sorted CCC_FALSE if not. Error if
singly_linked_list is NULL.

If a non-increasing order is desired, return opposite results from the user
comparison function. If an element is CCC_ORDER_LESSER return CCC_ORDER_GREATER
and vice versa. If elements are equal, return CCC_ORDER_EQUAL. */
CCC_Tribool
CCC_singly_linked_list_is_sorted(CCC_Singly_linked_list const *list);

/**@}*/

/** @name Deallocation Interface
Deallocate the container. */
/**@{*/

/** @brief Clears the list freeing memory if needed. O(N).
@param[in] list a pointer to the singly linked list.
@param[in] destroy a destructor function or NULL if not needed.
@return ok if the clear succeeded or an input error if list or destroy are NULL.

Note that if allocation is allowed, the container will free the user types
wrapping each intrusive element in the list after calling destroy. Therefore,
destroy should not free memory if the container has been given allocation
permission. It should only perform other necessary cleanup or state management.

If allocation is not allowed destroy may free memory or not as the user sees
fit. The user is responsible for managing the memory that wraps each intrusive
handle as the elements are simply removed from the list. */
CCC_Result CCC_singly_linked_list_clear(CCC_Singly_linked_list *list,
                                        CCC_Type_destructor *destroy);

/**@}*/

/** @name Iteration Interface
Iterate through the doubly linked list. */
/**@{*/

/** @brief Return the user type at the front of the list. O(1).
@param[in] list a pointer to the singly linked list.
@return a pointer to the user type at the start of the list or the end sentinel.
NULL is returned if list is NULL. */
[[nodiscard]] void *
CCC_singly_linked_list_begin(CCC_Singly_linked_list const *list);

/** @brief Return the list node type at the front of the list. O(1).
@param[in] list a pointer to the singly linked list.
@return a pointer to the list node type at the start of the list or NULL if
empty. */
[[nodiscard]] void *
CCC_singly_linked_list_node_begin(CCC_Singly_linked_list const *list);

/** @brief Return the list node type at the front of the list. O(1).
@param[in] list a pointer to the singly linked list.
@return a pointer to the list node type at the start of the list or NULL if
empty. */
[[nodiscard]] void *
CCC_singly_linked_list_node_before_begin(CCC_Singly_linked_list const *list);

/** @brief Return the sentinel at the end of the list. Do not access sentinel.
O(1).
@param[in] list a pointer to the singly linked list.
@return a pointer to the sentinel at the end of the list. It is undefined to
access the sentinel. NULL is returned if list is NULL.  */
[[nodiscard]] void *
CCC_singly_linked_list_end(CCC_Singly_linked_list const *list);

/** @brief Return the user type following type_intruder in the list. O(1).
@param[in] list a pointer to the singly linked list.
@param[in] type_intruder a pointer to the intrusive handle known to be in the
list.
@return the user type following type_intruder or the end sentinel if none
follow. NULL is returned if list or type_intruder are NULL. */
[[nodiscard]] void *
CCC_singly_linked_list_next(CCC_Singly_linked_list const *list,
                            CCC_Singly_linked_list_node const *type_intruder);

/**@}*/

/** @name State Interface
Obtain state from the doubly linked list. */
/**@{*/

/** @brief Return a pointer to the element at the front of the list. O(1).
@param[in] list a pointer to the singly linked list.
@return a reference to the front element or NULL if empty or singly_linked_list
is NULL. */
[[nodiscard]] void *
CCC_singly_linked_list_front(CCC_Singly_linked_list const *list);

/** @brief Return the count of nodes in the list. O(1).
@param[in] list a pointer to the singly linked list.
@return the size or an argument error is set if list is NULL. */
[[nodiscard]] CCC_Count
CCC_singly_linked_list_count(CCC_Singly_linked_list const *list);

/** @brief Return true if the list is empty. O(1).
@param[in] list a pointer to the singly linked list.
@return true if size is 0 otherwise false. Error returned if singly_linked_list
is NULL. */
[[nodiscard]] CCC_Tribool
CCC_singly_linked_list_is_empty(CCC_Singly_linked_list const *list);

/** @brief Returns true if the invariants of the list hold.
@param[in] list a pointer to the singly linked list.
@return true if list is valid, else false. Error returned if singly_linked_list
is NULL. */
[[nodiscard]] CCC_Tribool
CCC_singly_linked_list_validate(CCC_Singly_linked_list const *list);

/**@}*/

/** Define this preprocessor macro if shorter names are desired for the singly
linked list container. Check for namespace clashes before name shortening. */
#ifdef SINGLY_LINKED_LIST_USING_NAMESPACE_CCC
typedef CCC_Singly_linked_list_node singly_linked_list_node;
typedef CCC_Singly_linked_list Singly_linked_list;
#    define singly_linked_list_initialize(args...)                             \
        CCC_singly_linked_list_initialize(args)
#    define singly_linked_list_emplace_front(args...)                          \
        CCC_singly_linked_list_emplace_front(args)
#    define singly_linked_list_push_front(args...)                             \
        CCC_singly_linked_list_push_front(args)
#    define singly_linked_list_front(args...) CCC_singly_linked_list_front(args)
#    define singly_linked_list_pop_front(args...)                              \
        CCC_singly_linked_list_pop_front(args)
#    define singly_linked_list_splice(args...)                                 \
        CCC_singly_linked_list_splice(args)
#    define singly_linked_list_splice_range(args...)                           \
        CCC_singly_linked_list_splice_range(args)
#    define singly_linked_list_extract(args...)                                \
        CCC_singly_linked_list_extract(args)
#    define singly_linked_list_extract_range(args...)                          \
        CCC_singly_linked_list_extract_range(args)
#    define singly_linked_list_erase(args...) CCC_singly_linked_list_erase(args)
#    define singly_linked_list_erase_range(args...)                            \
        CCC_singly_linked_list_erase_range(args)
#    define singly_linked_list_sort(args...) CCC_singly_linked_list_sort(args)
#    define singly_linked_list_insert_sorted(args...)                          \
        CCC_singly_linked_list_insert_sorted(args)
#    define singly_linked_list_is_sorted(args...)                              \
        CCC_singly_linked_list_is_sorted(args)
#    define singly_linked_list_begin(args...) CCC_singly_linked_list_begin(args)
#    define singly_linked_list_node_begin(args...)                             \
        CCC_singly_linked_list_node_begin(args)
#    define singly_linked_list_node_before_begin(args...)                      \
        CCC_singly_linked_list_node_before_begin(args)
#    define singly_linked_list_end(args...) CCC_singly_linked_list_end(args)
#    define singly_linked_list_next(args...) CCC_singly_linked_list_next(args)
#    define singly_linked_list_count(args...) CCC_singly_linked_list_count(args)
#    define singly_linked_list_is_empty(args...)                               \
        CCC_singly_linked_list_is_empty(args)
#    define singly_linked_list_validate(args...)                               \
        CCC_singly_linked_list_validate(args)
#    define singly_linked_list_clear(args...) CCC_singly_linked_list_clear(args)
#endif /* SINGLY_LINKED_LIST_USING_NAMESPACE_CCC */

#endif /* CCC_FORWARD_LIST_H */
