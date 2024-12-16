/** @file
@brief The Singly Linked List Interface

A singly linked list is well suited for list or stack structures that only need
access to the front or most recently added elements. When compared to a doubly
linked list, the memory overhead per node is smaller but some operations will
have O(N) runtime implications when compared to a similar operation in a doubly
linked list. Review function documentation when unsure of the runtime of an sll
operation.

This container offers pointer stability. Also, if the container is not
permitted to allocate all insertion code assumes that the user has allocated
memory appropriately for the element to be inserted; it will not allocate or
free in this case. If allocation is permitted upon initialization the container
will manage the memory as expected on insert or erase operations as defined
by the interface; memory is allocated for insertions and freed for removals.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_SINGLY_LINKED_LIST_H
#define CCC_SINGLY_LINKED_LIST_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "impl/impl_singly_linked_list.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A low overhead front tracking container with efficient push and pop.

A singly linked list may be stored in the stack, heap, or data segment. Once
Initialized it is passed by reference to all functions. A singly linked list
can be initialized at compile time or runtime. */
typedef struct ccc_sll_ ccc_singly_linked_list;

/** @brief A singly linked list intrusive element to embedded in a user type.

It can be used in an allocating or non allocating container. If allocation is
prohibited the container assumes the element is wrapped in pre-allocated
memory with the appropriate lifetime and scope for the user's needs; the
container does not allocate or free in this case. If allocation is allowed
the container will handle copying the data wrapping the element to allocations
and deallocating when necessary. */
typedef struct ccc_sll_elem_ ccc_sll_elem;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a singly linked list at compile or runtime.
@param [in] list_name the name the user has chosen for the list.
@param [in] struct_name the user type wrapping the intrusive sll elem.
@param [in] list_elem_field the name of the field in the user type storing the
intrusive list elem.
@param [in] alloc_fn an allocation function if allocation is allowed.
@param [in] cmp_fn a comparison function for searching or sorting the list.
@param [in] aux_data a pointer to any auxiliary data needed for comparison or
destruction.
@return a stuct initializer for the singly linked list to be assigned
(e.g. ccc_singly_linked_list l = ccc_sll_init(...);). */
#define ccc_sll_init(list_name, struct_name, list_elem_field, alloc_fn,        \
                     cmp_fn, aux_data)                                         \
    ccc_impl_sll_init(list_name, struct_name, list_elem_field, alloc_fn,       \
                      cmp_fn, aux_data)

/**@}*/

/** @name Insert and Remove Interface
Add or remove elements from the container. */
/**@{*/

/** @brief Push the type wrapping elem to the front of the list. O(1).
@param [in] sll a pointer to the singly linked list.
@param [in] elem a pointer to the intrusive handle in the user type.
@return a pointer to the inserted element or NULL if allocation failed.

Note that if allocation is not allowed the container assumes the memory wrapping
elem has been allocated appropriately and with the correct lifetime by the user.

If allocation is allowed the provided element is copied to a new allocation. */
[[nodiscard]] void *ccc_sll_push_front(ccc_singly_linked_list *sll,
                                       ccc_sll_elem *elem);

/** @brief Write a compound literal directly to allocated memory at the front.
O(1).
@param [in] list_ptr a pointer to the singly linked list.
@param [in] struct_initializer a compound literal containing the elements to
be written to a newly allocated node.
@return a reference to the element pushed to the front or NULL if allocation
failed.

Note that it only makes sense to use this method when the container is given
allocation permission. Otherwise NULL is returned due to an inability for the
container to allocate memory. */
#define ccc_sll_emplace_front(list_ptr, struct_initializer...)                 \
    ccc_impl_sll_emplace_front(list_ptr, struct_initializer)

/** @brief Pop the front element from the list. O(1).
@param [in] sll a pointer to the singly linked list.
@return ok if the list is non-empty and the pop is successful. An input error
is returned if sll is NULL or the list is empty. */
ccc_result ccc_sll_pop_front(ccc_singly_linked_list *sll);

/** @brief Inserts splice element before pos. O(N).
@param [in] pos_sll the list to which pos belongs.
@param [in] pos the position before which splice will be inserted.
@param [in] splice_sll the list to which splice belongs.
@param [in] splice the element to be moved before pos.
@return ok if the operations is successful. An input error is provided if any
input pointers are NULL.

Note that pos_sll and splice_sll may be the same or different lists and the
invariants of each or the same list will be maintained by the function. */
ccc_result ccc_sll_splice(ccc_singly_linked_list *pos_sll, ccc_sll_elem *pos,
                          ccc_singly_linked_list *splice_sll,
                          ccc_sll_elem *splice);

/** @brief Inserts the range of spliced elements before pos. O(N).
@param [in] pos_sll the list to which pos belongs.
@param [in] pos the position before which the range will be inserted.
@param [in] splice_sll the list to which the range belongs.
@param [in] begin the start of the range.
@param [in] end the end of the range.
@return ok if the operations is successful. An input error is provided if any
input pointers are NULL.
@warning pos must not be inside of the range (begin, end) if pos_sll is the
same list as splice_sll.

Note that pos_sll and splice_sll may be the same or different lists and the
invariants of each or the same list will be maintained by the function. */
ccc_result ccc_sll_splice_range(ccc_singly_linked_list *pos_sll,
                                ccc_sll_elem *pos,
                                ccc_singly_linked_list *splice_sll,
                                ccc_sll_elem *begin, ccc_sll_elem *end);

/** @brief Erases elem from the list returning the following element. O(N).
@param [in] sll a pointer to the singly linked list.
@param [in] elem a handle to the intrusive element known to be in the list.
@return a pointer to the element following elem in the list or NULL if the
list is empty or any bad input is provided to the function.
@warning elem must be in the list.

Note that if allocation permission is given to the container it will free the
element. Otherwise, it is the user's responsibility to free the type wrapping
elem. */
void *ccc_sll_erase(ccc_singly_linked_list *sll, ccc_sll_elem *elem);

/** @brief Erases a range from the list returning the element after end. O(N).
@param [in] sll a pointer to the singly linked list.
@param [in] begin the start of the range in the list.
@param [in] end the exclusive end of the range in the list.
@return a pointer to the element following the range in the list or NULL if the
list is empty or any bad input is provided to the function.
@warning the provided range must be in the list.

Note that if allocation permission is given to the container it will free the
elements in the range. Otherwise, it is the user's responsibility to free the
types wrapping the range of elements. */
void *ccc_sll_erase_range(ccc_singly_linked_list *sll, ccc_sll_elem *begin,
                          ccc_sll_elem *end);

/** @brief Extracts an element from the list without freeing it. O(N).
@param [in] sll a pointer to the singly linked list.
@param [in] elem a handle to the intrusive element known to be in the list.
@return a pointer to the element following elem in the list.

Note that regardless of allocation permission this method will not free the
type wrapping elem. It only removes it from the list. */
void *ccc_sll_extract(ccc_singly_linked_list *sll, ccc_sll_elem *elem);

/** @brief Extracts a range of elements from the list without freeing them.
O(N).
@param [in] sll a pointer to the singly linked list.
@param [in] begin the start of the range in the list.
@param [in] end the exclusive end of the range in the list.
@return a pointer to the element following the range of elements in the list.

Note that the range remains in tact and can be iterated as one would iterate
a normal list. However, insertions and removals from a range are not possible
as they no longer belong to any list. */
void *ccc_sll_extract_range(ccc_singly_linked_list *sll, ccc_sll_elem *begin,
                            ccc_sll_elem *end);

/**@}*/

/** @name Deallocation Interface
Deallocate the container. */
/**@{*/

/** @brief Clears the list freeing memory if needed. O(N).
@param [in] sll a pointer to the singly linked list.
@param [in] fn a destructor function or NULL if not needed.
@return ok if the clear succeeded or an input error if sll or fn are NULL.

Note that if allocation is allowed, the container will free the user types
wrapping each intrusive element in the list after calling fn. Therefore, fn
should not free memory if the container has been given allocation permission.
It should only perform other necessary cleanup or state management.

If allocation is not allowed fn may free memory or not as the user sees fit.
The user is responsible for managing the memory that wraps each intrusive
handle as the elements are simply removed from the list. */
ccc_result ccc_sll_clear(ccc_singly_linked_list *sll, ccc_destructor_fn *fn);

/**@}*/

/** @name Iteration Interface
Iterate through the doubly linked list. */
/**@{*/

/** @brief Return the user type at the front of the list. O(1).
@param [in] sll a pointer to the singly linked list.
@return a pointer to the user type at the start of the list or the end sentinel.
NULL is returned if sll is NULL. */
[[nodiscard]] void *ccc_sll_begin(ccc_singly_linked_list const *sll);

/** @brief Return the sentinel at the end of the list. Do not access sentinel.
O(1).
@param [in] sll a pointer to the singly linked list.
@return a pointer to the sentinel at the end of the list. It is undefined to
access the sentinel. NULL is returned if sll is NULL.  */
[[nodiscard]] void *ccc_sll_end(ccc_singly_linked_list const *sll);

/** @brief Return the user type following elem in the list. O(1).
@param [in] sll a pointer to the singly linked list.
@param [in] elem a pointer to the intrusive handle known to be in the list.
@return the user type following elem or the end sentinel if none follow. NULL
is returned if sll or elem are NULL. */
[[nodiscard]] void *ccc_sll_next(ccc_singly_linked_list const *sll,
                                 ccc_sll_elem const *elem);

/**@}*/

/** @name State Interface
Obtain state from the doubly linked list. */
/**@{*/

/** @brief Return a pointer to the element at the front of the list. O(1).
@param [in] sll a pointer to the singly linked list.
@return a reference to the front element or NULL if empty or sll is NULL. */
[[nodiscard]] void *ccc_sll_front(ccc_singly_linked_list const *sll);

/** @brief Return a pointer to the first intrusive handle in the list. O(1).
@param [in] sll a pointer to the singly linked list.
@return a pointer to the first handle to a user type in the list or NULL if
empty. */
[[nodiscard]] ccc_sll_elem *
ccc_sll_begin_elem(ccc_singly_linked_list const *sll);

/** @brief Return a pointer to the first list element in the list. O(1).
@param [in] sll a pointer to the singly linked list.
@return a pointer to the first list element. This will never be NULL, even if
the list is empty. If bad input is provided (sll is NULL) NULL is returned. */
[[nodiscard]] ccc_sll_elem *
ccc_sll_begin_sentinel(ccc_singly_linked_list const *sll);

/** @brief Return the size of the list. O(1).
@param [in] sll a pointer to the singly linked list.
@return the size or 0 if the list is empty or sll is NULL. */
[[nodiscard]] size_t ccc_sll_size(ccc_singly_linked_list const *sll);

/** @brief Return true if the list is empty. O(1).
@param [in] sll a pointer to the singly linked list.
@return true if the size is 0 or sll is NULL otherwise false. */
[[nodiscard]] bool ccc_sll_is_empty(ccc_singly_linked_list const *sll);

/** @brief Returns true if the invariants of the list hold.
@param [in] sll a pointer to the singly linked list.
@return true if the list is valid, else false. */
[[nodiscard]] bool ccc_sll_validate(ccc_singly_linked_list const *sll);

/**@}*/

/** Define this preprocessor macro if shorter names are desired for the singly
linked list container. Check for namespace clashes before name shortening. */
#ifdef SINGLY_LINKED_LIST_USING_NAMESPACE_CCC
typedef ccc_sll_elem sll_elem;
typedef ccc_singly_linked_list singly_linked_list;
#    define sll_init(args...) ccc_sll_init(args)
#    define sll_emplace_front(args...) ccc_sll_emplace_front(args)
#    define sll_push_front(args...) ccc_sll_push_front(args)
#    define sll_front(args...) ccc_sll_front(args)
#    define sll_pop_front(args...) ccc_sll_pop_front(args)
#    define sll_splice(args...) ccc_sll_splice(args)
#    define sll_splice_range(args...) ccc_sll_splice_range(args)
#    define sll_extract(args...) ccc_sll_extract(args)
#    define sll_extract_range(args...) ccc_sll_extract_range(args)
#    define sll_erase(args...) ccc_sll_erase(args)
#    define sll_erase_range(args...) ccc_sll_erase_range(args)
#    define sll_begin(args...) ccc_sll_begin(args)
#    define sll_end(args...) ccc_sll_end(args)
#    define sll_next(args...) ccc_sll_next(args)
#    define sll_begin_elem(args...) ccc_sll_begin_elem(args)
#    define sll_begin_sentinel(args...) ccc_sll_begin_sentinel(args)
#    define sll_size(args...) ccc_sll_size(args)
#    define sll_is_empty(args...) ccc_sll_is_empty(args)
#    define sll_validate(args...) ccc_sll_validate(args)
#    define sll_clear(args...) ccc_sll_clear(args)
#endif /* SINGLY_LINKED_LIST_USING_NAMESPACE_CCC */

#endif /* CCC_FORWARD_LIST_H */
