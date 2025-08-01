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

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_DOUBLY_LINKED_LIST_H
#define CCC_DOUBLY_LINKED_LIST_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "impl/impl_doubly_linked_list.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A container offering bidirectional, insert, removal, and iteration.
@warning it is undefined behavior to use an uninitialized container.

A doubly linked list may be stored in the stack, heap, or data segment. Once
initialized it is passed by reference to all functions. A doubly linked list
can be initialized at compile time or runtime. */
typedef struct ccc_dll ccc_doubly_linked_list;

/** @brief A doubly linked list intrusive element to embedded in a user type.

It can be used in an allocating or non allocating container. If allocation is
prohibited the container assumes the element is wrapped in pre-allocated
memory with the appropriate lifetime and scope for the user's needs; the
container does not allocate or free in this case. If allocation is allowed
the container will handle copying the data wrapping the element to allocations
and deallocating when necessary. */
typedef struct ccc_dll_elem ccc_dll_elem;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a doubly linked list with its l-value name, type
containing the dll elems, the field of the dll elem, allocation function,
compare function and any auxilliary data needed for comparison, printing, or
destructors.
@param [in] list_name the name of the list being initialized.
@param [in] struct_name the type containing the intrusive dll element.
@param [in] list_elem_field name of the dll element in the containing type.
@param [in] cmp_fn the ccc_any_type_cmp_fn used to compare list elements.
@param [in] aux_data any auxilliary data that will be needed for comparison,
printing, or destruction of elements.
@param [in] alloc_fn the optional allocation function or NULL.
@return the initialized list. Assign to the list directly on the right hand
side of an equality operator. Initialization can occur at runtime or compile
time (e.g. ccc_doubly_linked l = ccc_dll_init(...);). */
#define ccc_dll_init(list_name, struct_name, list_elem_field, cmp_fn,          \
                     alloc_fn, aux_data)                                       \
    ccc_impl_dll_init(list_name, struct_name, list_elem_field, cmp_fn,         \
                      alloc_fn, aux_data)

/**@}*/

/** @name Insert and Remove Interface
Add or remove elements from the doubly linked list. */
/**@{*/

/** @brief  writes contents of type initializer directly to allocated memory at
the back of the list. O(1).
@param [in] list_ptr the address of the doubly linked list.
@param [in] type_initializer the r-value initializer of the type to be inserted
in the list. This should match the type containing dll elements as a struct
member for this list.
@return a reference to the inserted element or NULL if allocation is not
allowed or fails.

Note that it does not make sense to use this method if the list has been
initialized without an allocation function. If the user does not allow
allocation, the contents of new elements to be inserted has been determined by
the user prior to any inserts into the list. */
#define ccc_dll_emplace_back(list_ptr, type_initializer...)                    \
    ccc_impl_dll_emplace_back(list_ptr, type_initializer)

/** @brief  writes contents of type initializer directly to allocated memory at
the front of the list. O(1).
@param [in] list_ptr the address of the doubly linked list.
@param [in] type_initializer the r-value initializer of the type to be inserted
in the list. This should match the type containing dll elements as a struct
member for this list.
@return a reference to the inserted element or NULL if allocation is not
allowed or fails.

Note that it does not make sense to use this method if the list has been
initialized without an allocation function. If the user does not allow
allocation, the contents of new elements to be inserted has been determined by
the user prior to any inserts into the list. */
#define ccc_dll_emplace_front(list_ptr, type_initializer...)                   \
    ccc_impl_dll_emplace_front(list_ptr, type_initializer)

/** @brief Push user type wrapping elem to the front of the list. O(1).
@param [in] l a pointer to the doubly linked list.
@param [in] elem a pointer to the list element.
@return a pointer to the element inserted or NULL if bad input is provided
or allocation fails. */
[[nodiscard]] void *ccc_dll_push_front(ccc_doubly_linked_list *l,
                                       ccc_dll_elem *elem);

/** @brief Push user type wrapping elem to the back of the list. O(1).
@param [in] l a pointer to the doubly linked list.
@param [in] elem a pointer to the list element.
@return a pointer to the element inserted or NULL if bad input is provided
or allocation fails. */
[[nodiscard]] void *ccc_dll_push_back(ccc_doubly_linked_list *l,
                                      ccc_dll_elem *elem);

/** @brief Insert user type wrapping elem before pos_elem. O(1).
@param [in] l a pointer to the doubly linked list.
@param [in] pos_elem a pointer to the list element before which elem inserts.
@param [in] elem a pointer to the list element.
@return a pointer to the element inserted or NULL if bad input is provided
or allocation fails. */
[[nodiscard]] void *ccc_dll_insert(ccc_doubly_linked_list *l,
                                   ccc_dll_elem *pos_elem, ccc_dll_elem *elem);

/** @brief Pop the user type at the front of the list. O(1).
@param [in] l a pointer to the doubly linked list.
@return an ok result if the pop was successful or an error if bad input is
provided or the list is empty.*/
ccc_result ccc_dll_pop_front(ccc_doubly_linked_list *l);

/** @brief Pop the user type at the back of the list. O(1).
@param [in] l a pointer to the doubly linked list.
@return an ok result if the pop was successful or an error if bad input is
provided or the list is empty.*/
ccc_result ccc_dll_pop_back(ccc_doubly_linked_list *l);

/** @brief Returns the element following an extracted element from the list
without deallocating regardless of allocation permission provided to the
container. O(1).
@param [in] l a pointer to the doubly linked list.
@param [in] elem the handle of an element known to be in the list.
@return a reference to the element in the list following elem or NULL if the
element is the last. NULL is returned if bad input is provided or the elem is
not in the list. */
void *ccc_dll_extract(ccc_doubly_linked_list *l, ccc_dll_elem *elem);

/** @brief Returns the element following an erased element from the list. O(1).
@param [in] l a pointer to the doubly linked list.
@param [in] elem the handle of an element known to be in the list.
@return a reference to the element in the list following elem or NULL if the
element is the last. NULL is returned if bad input is provided or the elem is
not in the list. */
void *ccc_dll_erase(ccc_doubly_linked_list *l, ccc_dll_elem *elem);

/** @brief Returns the element following an extracted range of elements from the
list. O(N).
@param [in] l a pointer to the doubly linked list.
@param [in] elem_begin the handle of an element known to be in the list at the
start of the range.
@param [in] elem_end the handle of an element known to be in the list at the
end of the range following elem_begin.
@return a reference to the element in the list following elem_end or NULL if the
element is the last. NULL is returned if bad input is provided or the elem is
not in the list.

Note that if the user does not permit the container to allocate they may iterate
through the extracted range in the same way one iterates through a normal list
using the iterator function. If allocation is allowed, all elements from
elem_begin to elem_end will be erased and references invalidated. */
void *ccc_dll_erase_range(ccc_doubly_linked_list *l, ccc_dll_elem *elem_begin,
                          ccc_dll_elem *elem_end);

/** @brief Returns the element following an extracted range of elements from the
list without deallocating regardless of allocation permission provided to the
container. O(N).
@param [in] l a pointer to the doubly linked list.
@param [in] elem_begin the handle of an element known to be in the list at the
start of the range.
@param [in] elem_end the handle of an element known to be in the list at the
end of the range following elem_begin.
@return a reference to the element in the list following elem_end or NULL if the
element is the last. NULL is returned if bad input is provided or the elem is
not in the list.

Note that the user may iterate through the extracted range in the same way one
iterates through a normal list using the iterator function. */
void *ccc_dll_extract_range(ccc_doubly_linked_list *l, ccc_dll_elem *elem_begin,
                            ccc_dll_elem *elem_end);

/** @brief Repositions to_cut before pos. Only list pointers are modified. O(1).
@param [in] pos_dll the list to which pos belongs.
@param [in] pos the position before which to_cut will be moved.
@param [in] to_cut_dll the list to which to_cut belongs.
@param [in] to_cut the element to cut.
@return ok if the splice is successful or an error if bad input is provided. */
ccc_result ccc_dll_splice(ccc_doubly_linked_list *pos_dll, ccc_dll_elem *pos,
                          ccc_doubly_linked_list *to_cut_dll,
                          ccc_dll_elem *to_cut);

/** @brief Repositions begin to end before pos. Only list pointers are modified
O(N).
@param [in] pos_dll the list to which pos belongs.
@param [in] pos the position before which to_cut will be moved.
@param [in] to_cut_dll the list to which the range belongs.
@param [in] begin the start of the list to splice.
@param [in] end the end of the list to splice.
@return ok if the splice is successful or an error if bad input is provided. */
ccc_result ccc_dll_splice_range(ccc_doubly_linked_list *pos_dll,
                                ccc_dll_elem *pos,
                                ccc_doubly_linked_list *to_cut_dll,
                                ccc_dll_elem *begin, ccc_dll_elem *end);

/**@}*/

/** @name Sorting Interface
Sort the container. */
/**@{*/

/** @brief Sorts the doubly linked list in non-decreasing order as defined by
the provided comparison function. `O(N * log(N))` time, `O(1)` space.
@param [in] dll a pointer to the doubly linked list to sort.
@return the result of the sort, usually OK. An arg error if dll is null. */
ccc_result ccc_dll_sort(ccc_doubly_linked_list *dll);

/** @brief Inserts e in sorted position according to the non-decreasing order
of the list determined by the user provided comparison function. `O(1)`.
@param [in] dll a pointer to the doubly linked list.
@param [in] e a pointer to the element to be inserted in order.
@return a pointer to the element that has been inserted or NULL if allocation
is required and has failed.
@warning this function assumes the list is sorted.

If a non-increasing order is desired, return opposite results from the user
comparison function. If an element is CCC_LES return CCC_GRT and vice versa.
If elements are equal, return CCC_EQL. */
void *ccc_dll_insert_sorted(ccc_doubly_linked_list *dll, ccc_dll_elem *e);

/** @brief Returns true if the list is sorted in non-decreasing order according
to the user provided comparison function.
@param [in] dll a pointer to the singly linked list.
@return CCC_TRUE if the list is sorted CCC_FALSE if not. Error if dll is NULL.

If a non-increasing order is desired, return opposite results from the user
comparison function. If an element is CCC_LES return CCC_GRT and vice versa.
If elements are equal, return CCC_EQL. */
ccc_tribool ccc_dll_is_sorted(ccc_doubly_linked_list const *dll);

/**@}*/

/** @name Deallocation Interface
Deallocate the container. */
/**@{*/

/** @brief Clear the contents of the list freeing elements, if given allocation
permission. O(N).
@param [in] l a pointer to the doubly linked list.
@param [in] fn a destructor function to run on each element.
@return ok if the clearing was a success or an input error if l or fn is NULL.

Note that if the list is initialized with allocation permission it will free
elements for the user and the destructor function should only perform auxiliary
cleanup, otherwise a double free will occur.

If the list has not been given allocation permission the user should free
the list elements with the destructor if they wish to do so. The implementation
ensures the function is called after the element is removed. Otherwise, the user
must manage their elements at their discretion after the list is emptied in
this function. */
ccc_result ccc_dll_clear(ccc_doubly_linked_list *l,
                         ccc_any_type_destructor_fn *fn);

/**@}*/

/** @name Iteration Interface
Iterate through the doubly linked list. */
/**@{*/

/** @brief Return the user type at the start of the list or NULL if empty. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the user type or NULL if empty or bad input. */
[[nodiscard]] void *ccc_dll_begin(ccc_doubly_linked_list const *l);

/** @brief Return the user type at the end of the list or NULL if empty. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the user type or NULL if empty or bad input. */
[[nodiscard]] void *ccc_dll_rbegin(ccc_doubly_linked_list const *l);

/** @brief Return the user type following the element known to be in the list.
O(1).
@param [in] l a pointer to the doubly linked list.
@param [in] elem a handle to the list element known to be in the list.
@return a pointer to the element following elem or NULL if no elements follow
or bad input is provided. */
[[nodiscard]] void *ccc_dll_next(ccc_doubly_linked_list const *l,
                                 ccc_dll_elem const *elem);

/** @brief Return the user type following the element known to be in the list
moving from back to front. O(1).
@param [in] l a pointer to the doubly linked list.
@param [in] elem a handle to the list element known to be in the list.
@return a pointer to the element following elem from back to front or NULL if no
elements follow or bad input is provided. */
[[nodiscard]] void *ccc_dll_rnext(ccc_doubly_linked_list const *l,
                                  ccc_dll_elem const *elem);

/** @brief Return the end sentinel with no accessible fields. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the end sentinel with no accessible fields. */
[[nodiscard]] void *ccc_dll_end(ccc_doubly_linked_list const *l);

/** @brief Return the start sentinel with no accessible fields. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the start sentinel with no accessible fields. */
[[nodiscard]] void *ccc_dll_rend(ccc_doubly_linked_list const *l);

/**@}*/

/** @name State Interface
Obtain state from the doubly linked list. */
/**@{*/

/** @brief Returns the user type at the front of the list. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the user type at the front of the list. NULL if empty. */
[[nodiscard]] void *ccc_dll_front(ccc_doubly_linked_list const *l);

/** @brief Returns the user type at the back of the list. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the user type at the back of the list. NULL if empty. */
[[nodiscard]] void *ccc_dll_back(ccc_doubly_linked_list const *l);

/** @brief Return a handle to the list element at the front of the list which
may be the sentinel. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the list element at the beginning of the list which may be
the sentinel but will not be NULL unless a NULL pointer is provided as l. */
[[nodiscard]] ccc_dll_elem *ccc_dll_begin_elem(ccc_doubly_linked_list const *l);

/** @brief Return a handle to the list element at the back of the list which
may be the sentinel. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the list element at the end of the list which may be
the sentinel but will not be NULL unless a NULL pointer is provided as l. */
[[nodiscard]] ccc_dll_elem *ccc_dll_end_elem(ccc_doubly_linked_list const *l);

/** @brief Return a pointer to the sentinel node at the back of the list.
@param [in] l a pointer to the doubly linked list.
@return a pointer to the sentinel node that always points to the first and last
elements or itself. It will not be NULL unless sll pointer provided is NULL.

This function can be used when the user wishes to splice an element or range of
elements to the back of the list. Because the interface only allows the user
to splice an element or elements BEFORE a position having access to the sentinel
makes it possible to splice to the back of the list. */
[[nodiscard]] ccc_dll_elem *
ccc_dll_end_sentinel(ccc_doubly_linked_list const *l);

/** @brief Return the count of elements in the list. O(1).
@param [in] l a pointer to the doubly linked list.
@return the size of the list. An argument error is set if l is NULL. */
[[nodiscard]] ccc_ucount ccc_dll_count(ccc_doubly_linked_list const *l);

/** @brief Return if the size of the list is equal to 0. O(1).
@param [in] l a pointer to the doubly linked list.
@return true if the size is 0, else false. Error if l is NULL. */
[[nodiscard]] ccc_tribool ccc_dll_is_empty(ccc_doubly_linked_list const *l);

/** @brief Validates internal state of the list.
@param [in] l a pointer to the doubly linked list.
@return true if invariants hold, false if not. Error if l is NULL. */
[[nodiscard]] ccc_tribool ccc_dll_validate(ccc_doubly_linked_list const *l);

/**@}*/

/** Define this preprocessor directive before including this header if shorter
names are desired for the doubly linked list container. Ensure no namespace
clashes exist prior to name shortening. */
#ifdef DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC
typedef ccc_dll_elem dll_elem;
typedef ccc_doubly_linked_list doubly_linked_list;
#    define dll_init(args...) ccc_dll_init(args)
#    define dll_emplace_back(args...) ccc_dll_emplace_back(args)
#    define dll_emplace_front(args...) ccc_dll_emplace_front(args)
#    define dll_push_front(args...) ccc_dll_push_front(args)
#    define dll_push_back(args...) ccc_dll_push_back(args)
#    define dll_front(args...) ccc_dll_front(args)
#    define dll_back(args...) ccc_dll_back(args)
#    define dll_pop_front(args...) ccc_dll_pop_front(args)
#    define dll_pop_back(args...) ccc_dll_pop_back(args)
#    define dll_extract(args...) ccc_dll_extract(args)
#    define dll_extract_range(args...) ccc_dll_extract_range(args)
#    define dll_erase(args...) ccc_dll_erase(args)
#    define dll_erase_range(args...) ccc_dll_erase_range(args)
#    define dll_splice(args...) ccc_dll_splice(args)
#    define dll_splice_range(args...) ccc_dll_splice_range(args)
#    define dll_sort(args...) ccc_dll_sort(args)
#    define dll_insert_sorted(args...) ccc_dll_insert_sorted(args)
#    define dll_is_sorted(args...) ccc_dll_is_sorted(args)
#    define dll_begin(args...) ccc_dll_begin(args)
#    define dll_next(args...) ccc_dll_next(args)
#    define dll_rbegin(args...) ccc_dll_rbegin(args)
#    define dll_rnext(args...) ccc_dll_rnext(args)
#    define dll_end(args...) ccc_dll_end(args)
#    define dll_end_elem(args...) ccc_dll_end_elem(args)
#    define dll_rend(args...) ccc_dll_rend(args)
#    define dll_begin_elem(args...) ccc_dll_begin_elem(args)
#    define dll_end_elem(args...) ccc_dll_end_elem(args)
#    define dll_end_sentinel(args...) ccc_dll_end_sentinel(args)
#    define dll_count(args...) ccc_dll_count(args)
#    define dll_is_empty(args...) ccc_dll_is_empty(args)
#    define dll_clear(args...) ccc_dll_clear(args)
#    define dll_validate(args...) ccc_dll_validate(args)
#endif /* DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC */

#endif /* CCC_LIST_H */
