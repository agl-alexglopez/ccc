/** @file
@brief The Doubly Linked List Interface

A doubly linked list offers efficient push, pop, extract, and erase operations
for elements stored in the list. Notably, for single elements the list can
offer O(1) push front/back, pop front/back, and removal of elements in
arbitrary positions in the list. The cost of this efficiency is higher memory
footprint. */
#ifndef CCC_DOUBLY_LINKED_LIST_H
#define CCC_DOUBLY_LINKED_LIST_H

#include <stddef.h>

#include "impl_doubly_linked_list.h"
#include "types.h"

/** @brief A container offering bidirectional, insert, removal, and iteration.
@warning it is undefined behavior to use an uninitialized doubly linked list.

A doubly linked list may be stored in the stack, heap, or data segment. Once
Initialized it is passed by reference to all functions. A doubly linked list
can be initialized at compile time or runtime. */
typedef struct ccc_dll_ ccc_doubly_linked_list;

/** @brief A doubly linked list intrusive element to embedded in a user type.

It can be used in an allocating or non allocating container. If allocation is
prohibited the container assumes the element is wrapped in pre-allocated
memory with the appropriate lifetime and scope for the user's needs; the
container does not allocate or free in this case. If allocation is allowed
the container will handle copying the data wrapping the element to allocations
and deallocating when necessary. */
typedef struct ccc_dll_elem_ ccc_dll_elem;

/** @brief Initialize a doubly linked list with its l-value name, type
containing the dll elems, the field of the dll elem, allocation function,
compare function and any auxilliary data needed for comparison, printing, or
destructors.
@param [in] list_name the name of the list being initialized.
@param [in] struct_name the type containing the intrusive dll element.
@param [in] list_elem_field name of the dll element in the containing type.
@param [in] alloc_fn the optional allocation function or NULL.
@param [in] cmp_fn the ccc_cmp_fn used to compare list elements.
@param [in] aux_data any auxilliary data that will be needed for comparison,
printing, or destruction of elements.
@return the initialized list. Assign to the list directly on the right hand
side of an equality operator. Initialization can occur at runtime or compile
time (e.g. ccc_doubly_linked l = ccc_dll_init(...);). */
#define ccc_dll_init(list_name, struct_name, list_elem_field, alloc_fn,        \
                     cmp_fn, aux_data)                                         \
    ccc_impl_dll_init(list_name, struct_name, list_elem_field, alloc_fn,       \
                      cmp_fn, aux_data)

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

/** @brief Returns the user type at the front of the list. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the user type at the front of the list. NULL if empty. */
[[nodiscard]] void *ccc_dll_front(ccc_doubly_linked_list const *l);

/** @brief Returns the user type at the back of the list. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the user type at the back of the list. NULL if empty. */
[[nodiscard]] void *ccc_dll_back(ccc_doubly_linked_list const *l);

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
@param [in] pos_sll the list to which pos belongs.
@param [in] pos the position before which to_cut will be moved.
@param [in] to_cut_sll the list to which to_cut belongs.
@param [in] to_cut the element to cut.
@return ok if the splice is successful or an error if bad input is provided. */
ccc_result ccc_dll_splice(ccc_doubly_linked_list *pos_sll, ccc_dll_elem *pos,
                          ccc_doubly_linked_list *to_cut_sll,
                          ccc_dll_elem *to_cut);

/** @brief Repositions begin to end before pos. Only list pointers are modified
O(N).
@param [in] pos_sll the list to which pos belongs.
@param [in] pos the position before which to_cut will be moved.
@param [in] to_cut_sll the list to which the range belongs.
@param [in] begin the start of the list to splice.
@param [in] end the end of the list to splice.
@return ok if the splice is successful or an error if bad input is provided. */
ccc_result ccc_dll_splice_range(ccc_doubly_linked_list *pos_sll,
                                ccc_dll_elem *pos,
                                ccc_doubly_linked_list *to_cut_sll,
                                ccc_dll_elem *begin, ccc_dll_elem *end);

/** @brief Return the user type at the start of the list or NULL if empty. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the user type or NULL if empty or bad input. */
[[nodiscard]] void *ccc_dll_begin(ccc_doubly_linked_list const *l);

/** @brief Return the user type following the element known to be in the list.
O(1).
@param [in] l a pointer to the doubly linked list.
@param [in] elem a handle to the list element known to be in the list.
@return a pointer to the element following elem or NULL if no elements follow
or bad input is provided. */
[[nodiscard]] void *ccc_dll_next(ccc_doubly_linked_list const *l,
                                 ccc_dll_elem const *elem);

/** @brief Return the user type at the end of the list or NULL if empty. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the user type or NULL if empty or bad input. */
[[nodiscard]] void *ccc_dll_rbegin(ccc_doubly_linked_list const *l);

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

/** @brief Return a handle to the sentinel at the back of the list. O(1).
@param [in] l a pointer to the doubly linked list.
@return a pointer to the sentinel at the end of the list which will not be NULL
unless a NULL pointer is provided as l. */
[[nodiscard]] ccc_dll_elem *
ccc_dll_end_sentinel(ccc_doubly_linked_list const *l);

/** @brief Return the size of the list. O(1).
@param [in] l a pointer to the doubly linked list.
@return the size of the list or 0 if empty or l is NULL. */
[[nodiscard]] size_t ccc_dll_size(ccc_doubly_linked_list const *l);

/** @brief Return if the size of the list is equal to 0. O(1).
@param [in] l a pointer to the doubly linked list.
@return true if the size is 0 or l is NULL, else false. */
[[nodiscard]] bool ccc_dll_is_empty(ccc_doubly_linked_list const *l);

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
ccc_result ccc_dll_clear(ccc_doubly_linked_list *l, ccc_destructor_fn *fn);

/** @brief Validates internal state of the list.
@param [in] l a pointer to the doubly linked list.
@return true if invariants hold, false if not. */
[[nodiscard]] bool ccc_dll_validate(ccc_doubly_linked_list const *l);

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
#    define dll_size(args...) ccc_dll_size(args)
#    define dll_is_empty(args...) ccc_dll_is_empty(args)
#    define dll_clear(args...) ccc_dll_clear(args)
#    define dll_validate(args...) ccc_dll_validate(args)
#endif /* DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC */

#endif /* CCC_LIST_H */
