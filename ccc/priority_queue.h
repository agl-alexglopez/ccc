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
@brief The Priority Queue Interface

A priority queue offers simple, fast, pointer stable management of a priority
queue. Push is O(1). The cost to execute the increase key in a max heap and
decrease key in a min heap is O(1). However, due to the restructuring this
causes that increases the cost of later pops, the more accurate runtime is o(lg
N). The cost of a pop operation is O(lg N).

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define PRIORITY_QUEUE_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `CCC_` prefix. */
#ifndef CCC_PRIORITY_QUEUE_H
#define CCC_PRIORITY_QUEUE_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "private/private_priority_queue.h"
#include "types.h"

/** @name Container Types
Types available in the container interface. */
/**@{*/

/** @brief A container for pointer stability and an O(1) push and amortized o(lg
N) increase/decrease key.
@warning it is undefined behavior to access an uninitialized container.

A priority queue can be initialized on the stack, heap, or data segment at
runtime or compile time.*/
typedef struct CCC_Priority_queue CCC_Priority_queue;

/** @brief The embedded struct type for operation of the priority queue.

It can be used in an allocating or non allocating container. If allocation is
prohibited the container assumes the element is wrapped in pre-allocated
memory with the appropriate lifetime and scope for the user's needs; the
container does not allocate or free in this case. If allocation is allowed
the container will handle copying the data wrapping the element to allocations
and deallocating when necessary. */
typedef struct CCC_Priority_queue_node CCC_Priority_queue_node;

/**@}*/

/** @name Initialization Interface
Initialize the container with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize a priority queue at runtime or compile time.
@param[in] struct_name the name of the user type wrapping priority_queue elems.
@param[in] priority_queue_node_field the name of the field for the
priority_queue elem.
@param[in] priority_queue_order CCC_ORDER_LESSER for a min priority_queue or
CCC_ORDER_GREATER for a max priority_queue.
@param[in] order_fn the function used to compare two user types.
@param[in] allocate the allocation function or NULL if allocation is banned.
@param[in] context_data context data needed for comparison or destruction.
@return the initialized priority_queue on the right side of an equality operator
(e.g. CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(...);)
*/
#define CCC_priority_queue_initialize(struct_name, priority_queue_node_field,  \
                                      priority_queue_order, order_fn,          \
                                      allocate, context_data)                  \
    CCC_private_priority_queue_initialize(                                     \
        struct_name, priority_queue_node_field, priority_queue_order,          \
        order_fn, allocate, context_data)

/**@}*/

/** @name Insert and Remove Interface
Insert and remove elements from the priority queue. */
/**@{*/

/** @brief Adds an element to the priority queue in correct total order. O(1).
@param[in] priority_queue a pointer to the priority queue.
@param[in] elem a pointer to the intrusive element in the user type.
@return a reference to the newly inserted user type or NULL if NULL arguments
are provided or allocation fails when permitted.

Note that if allocation is permitted the user type is copied into a newly
allocated node.

If allocation is not permitted this function assumes the memory wrapping elem
has been allocated with the appropriate lifetime for the user's needs. */
[[nodiscard]] void *CCC_priority_queue_push(CCC_Priority_queue *priority_queue,
                                            CCC_Priority_queue_node *elem);

/** @brief Write user type directly to a newly allocated priority queue elem.
@param[in] Priority_queue_pointer a pointer to the priority queue.
@param[in] type_compound_literal the compound literal to write to the
allocation.
@return a reference to the successfully inserted element or NULL if allocation
fails or is not allowed.

Note that the priority queue must be initialized with allocation permission to
use this macro. */
#define CCC_priority_queue_emplace(Priority_queue_pointer,                     \
                                   type_compound_literal...)                   \
    CCC_private_priority_queue_emplace(Priority_queue_pointer,                 \
                                       type_compound_literal)

/** @brief Pops the front element from the priority queue. Amortized O(lgN).
@param[in] priority_queue a pointer to the priority queue.
@return ok if pop was successful or an input error if priority_queue is NULL or
empty. */
CCC_Result CCC_priority_queue_pop(CCC_Priority_queue *priority_queue);

/** Extract the element known to be in the priority_queue without freeing
memory. Amortized O(lgN).
@param[in] priority_queue a pointer to the priority queue.
@param[in] elem a pointer to the intrusive element in the user type.
@return a pointer to the extracted user type.

Note that the user must ensure that elem is in the priority queue. */
[[nodiscard]] void *
CCC_priority_queue_extract(CCC_Priority_queue *priority_queue,
                           CCC_Priority_queue_node *elem);

/** @brief Erase elem from the priority_queue. Amortized O(lgN).
@param[in] priority_queue a pointer to the priority queue.
@param[in] elem a pointer to the intrusive element in the user type.
@return ok if erase was successful or an input error if priority_queue or elem
is NULL or priority_queue is empty.

Note that the user must ensure that elem is in the priority queue. */
CCC_Result CCC_priority_queue_erase(CCC_Priority_queue *priority_queue,
                                    CCC_Priority_queue_node *elem);

/** @brief Update the priority in the user type wrapping elem.
@param[in] priority_queue a pointer to the priority queue.
@param[in] elem a pointer to the intrusive element in the user type.
@param[in] fn the update function to act on the type wrapping elem.
@param[in] context any context data needed for the update function.
@return a reference to the updated user type or NULL if update failed due to
bad arguments provided.
@warning the user must ensure elem is in the priority_queue.

Note that this operation may incur unnecessary overhead if the user can't
deduce if an increase or decrease is occurring. See the increase and decrease
operations. O(1) best case, O(lgN) worst case. */
void *CCC_priority_queue_update(CCC_Priority_queue *priority_queue,
                                CCC_Priority_queue_node *elem,
                                CCC_Type_modifier *fn, void *context);

/** @brief Update the priority in the user type stored in the container.
@param[in] priority_queue_pointer a pointer to the priority queue.
@param[in] type_pointer a pointer to the user struct type in the
priority_queue.
@param[in] update_closure_over_T a pointer to the user struct type T is made
available. Use a semicolon separated statements to execute on the user type
which wraps priority_queue_node_pointer (optionally wrapping {code here} in
braces may help with formatting). This closure may safely modify the key used to
track the user element's priority in the priority queue.
@return a reference to the updated user type or NULL if update failed due to
bad arguments provided.
@warning the user must ensure the type_pointer is a reference to an instance
of the type actively stored in the priority queue.

```
#define PRIORITY_QUEUE_USING_NAMESPACE_CCC
struct Val
{
    priority_queue_node e;
    int key;
};
Priority_queue priority_queue = build_rand_priority_queue();
priority_queue_update_w(&priority_queue, get_rand_val(&priority_queue), { T->key
= rand_key(); });
```

Note that this operation may incur unnecessary overhead if the user can't
deduce if an increase or decrease is occurring. See the increase and decrease
operations. O(1) best case, O(lgN) worst case. */
#define CCC_priority_queue_update_w(priority_queue_pointer, type_pointer,      \
                                    update_closure_over_T...)                  \
    CCC_private_priority_queue_update_w(priority_queue_pointer, type_pointer,  \
                                        update_closure_over_T)

/** @brief Increases the priority of the type wrapping elem. O(1) or O(lgN)
@param[in] priority_queue a pointer to the priority queue.
@param[in] elem a pointer to the intrusive element in the user type.
@param[in] fn the update function to act on the type wrapping elem.
@param[in] context any context data needed for the update function.
@return a reference to the updated user type or NULL if update failed due to
bad arguments provided.
@warning the data structure will be in an invalid state if the user decreases
the priority by mistake in this function.

Note that this is optimal update technique if the priority queue has been
initialized as a max queue and the new value is known to be greater than the old
value. If this is a max heap O(1), otherwise O(lgN).

While the best case operation is O(1) the impact of restructuring on future pops
from the priority_queue creates an amortized o(lgN) runtime for this function.
*/
void *CCC_priority_queue_increase(CCC_Priority_queue *priority_queue,
                                  CCC_Priority_queue_node *elem,
                                  CCC_Type_modifier *fn, void *context);

/** @brief Increases the priority of the user type stored in the container.
@param[in] priority_queue_pointer a pointer to the priority queue.
@param[in] type_pointer a pointer to the user struct type in the
priority_queue.
@param[in] increase_closure_over_T a pointer to the user struct type T is made
available. Use a semicolon separated statements to execute on the user type
which wraps priority_queue_node_pointer (optionally wrapping {code here} in
braces may help with formatting). This closure may safely modify the key used to
track the user element's priority in the priority queue.
@return a reference to the updated user type or NULL if update failed due to
bad arguments provided.
@warning the user must ensure the type_pointer is a reference to an instance
of the type actively stored in the priority queue. The data structure will be in
an invalid state if the user decreases the priority by mistake in this function.

```
#define PRIORITY_QUEUE_USING_NAMESPACE_CCC
struct Val
{
    priority_queue_node e;
    int key;
};
Priority_queue priority_queue = build_rand_priority_queue();
priority_queue_increase_w(&priority_queue, get_rand_val(&priority_queue), {
T->key++; });
```

Note that this is optimal update technique if the priority queue has been
initialized as a max queue and the new value is known to be greater than the old
value. If this is a max heap O(1), otherwise O(lgN).

While the best case operation is O(1) the impact of restructuring on future pops
from the priority_queue creates an amortized o(lgN) runtime for this function.
*/
#define CCC_priority_queue_increase_w(priority_queue_pointer, type_pointer,    \
                                      increase_closure_over_T...)              \
    CCC_private_priority_queue_increase_w(                                     \
        priority_queue_pointer, type_pointer, increase_closure_over_T)

/** @brief Decreases the value of the type wrapping elem. O(1) or O(lgN)
@param[in] priority_queue a pointer to the priority queue.
@param[in] elem a pointer to the intrusive element in the user type.
@param[in] fn the update function to act on the type wrapping elem.
@param[in] context any context data needed for the update function.
@return a reference to the updated user type or NULL if update failed due to
bad arguments provided.

Note that this is optimal update technique if the priority queue has been
initialized as a min queue and the new value is known to be less than the old
value. If this is a min heap O(1), otherwise O(lgN).

While the best case operation is O(1) the impact of restructuring on future pops
from the priority_queue creates an amortized o(lgN) runtime for this function.
*/
void *CCC_priority_queue_decrease(CCC_Priority_queue *priority_queue,
                                  CCC_Priority_queue_node *elem,
                                  CCC_Type_modifier *fn, void *context);

/** @brief Decreases the priority of the user type stored in the container.
@param[in] priority_queue_pointer a pointer to the priority queue.
@param[in] type_pointer a pointer to the user struct type in the
priority_queue.
@param[in] decrease_closure_over_T a pointer to the user struct type T is made
available. Use a semicolon separated statements to execute on the user type
which wraps priority_queue_node_pointer (optionally wrapping {code here} in
braces may help with formatting). This closure may safely modify the key used to
track the user element's priority in the priority queue.
@return a reference to the updated user type or NULL if update failed due to
bad arguments provided.
@warning the user must ensure the type_pointer is a reference to an instance
of the type actively stored in the priority queue. The data structure will be in
an invalid state if the user decreases the priority by mistake in this function.

```
#define PRIORITY_QUEUE_USING_NAMESPACE_CCC
struct Val
{
    priority_queue_node e;
    int key;
};
Priority_queue priority_queue = build_rand_priority_queue();
priority_queue_decrease_w(&priority_queue,
get_rand_priority_queue_node(&priority_queue), { T->key--; });
```

Note that this is optimal update technique if the priority queue has been
initialized as a min queue and the new value is known to be less than the old
value. If this is a min heap O(1), otherwise O(lgN).

While the best case operation is O(1) the impact of restructuring on future pops
from the priority_queue creates an amortized o(lgN) runtime for this function.
*/
#define CCC_priority_queue_decrease_w(priority_queue_pointer, type_pointer,    \
                                      decrease_closure_over_T...)              \
    CCC_private_priority_queue_decrease_w(                                     \
        priority_queue_pointer, type_pointer, decrease_closure_over_T)

/**@}*/

/** @name Deallocation Interface
Deallocate the container. */
/**@{*/

/** @brief Removes all elements from the priority_queue, freeing if needed.
@param[in] priority_queue a pointer to the priority queue.
@param[in] fn the destructor function or NULL if not needed.
@return ok if the clear was successful or an input error for NULL args.

Note that if allocation is allowed the container will free the user type
wrapping each element in the priority_queue. Therefore, the user should not free
in the destructor function. Only perform context cleanup operations if needed.

If allocation is not allowed, the user may free their stored types in the
destructor function if they wish to do so. The container simply removes all
the elements from the priority_queue, calling fn on each user type if provided,
and sets the size to zero. */
CCC_Result CCC_priority_queue_clear(CCC_Priority_queue *priority_queue,
                                    CCC_Type_destructor *fn);

/**@}*/

/** @name State Interface
Obtain state from the container. */
/**@{*/

/** @brief Obtain a reference to the front of the priority queue. O(1).
@param[in] priority_queue a pointer to the priority queue.
@return a reference to the front element in the priority_queue. */
[[nodiscard]] void *
CCC_priority_queue_front(CCC_Priority_queue const *priority_queue);

/** @brief Returns true if the priority queue is empty false if not. O(1).
@param[in] priority_queue a pointer to the priority queue.
@return true if the size is 0, false if not empty. Error if priority_queue is
NULL. */
[[nodiscard]] CCC_Tribool
CCC_priority_queue_is_empty(CCC_Priority_queue const *priority_queue);

/** @brief Returns the count of priority queue occupied nodes.
@param[in] priority_queue a pointer to the priority queue.
@return the size of the priority_queue or an argument error is set if
priority_queue is NULL. */
[[nodiscard]] CCC_Count
CCC_priority_queue_count(CCC_Priority_queue const *priority_queue);

/** @brief Verifies the internal invariants of the priority_queue hold.
@param[in] priority_queue a pointer to the priority queue.
@return true if the priority_queue is valid false if priority_queue is invalid.
Error if priority_queue is NULL. */
[[nodiscard]] CCC_Tribool
CCC_priority_queue_validate(CCC_Priority_queue const *priority_queue);

/** @brief Return the order used to initialize the priority_queue.
@param[in] priority_queue a pointer to the priority queue.
@return LES or GRT ordering. Any other ordering is invalid. */
[[nodiscard]] CCC_Order
CCC_priority_queue_order(CCC_Priority_queue const *priority_queue);

/**@}*/

/** Define this preprocessor directive if shortened names are desired for the
priority queue container. Check for collisions before name shortening. */
#ifdef PRIORITY_QUEUE_USING_NAMESPACE_CCC
typedef CCC_Priority_queue_node priority_queue_node;
typedef CCC_Priority_queue Priority_queue;
#    define priority_queue_initialize(args...)                                 \
        CCC_priority_queue_initialize(args)
#    define priority_queue_front(args...) CCC_priority_queue_front(args)
#    define priority_queue_push(args...) CCC_priority_queue_push(args)
#    define priority_queue_emplace(args...) CCC_priority_queue_emplace(args)
#    define priority_queue_pop(args...) CCC_priority_queue_pop(args)
#    define priority_queue_extract(args...) CCC_priority_queue_extract(args)
#    define priority_queue_is_empty(args...) CCC_priority_queue_is_empty(args)
#    define priority_queue_count(args...) CCC_priority_queue_count(args)
#    define priority_queue_update(args...) CCC_priority_queue_update(args)
#    define priority_queue_increase(args...) CCC_priority_queue_increase(args)
#    define priority_queue_decrease(args...) CCC_priority_queue_decrease(args)
#    define priority_queue_update_w(args...) CCC_priority_queue_update_w(args)
#    define priority_queue_increase_w(args...)                                 \
        CCC_priority_queue_increase_w(args)
#    define priority_queue_decrease_w(args...)                                 \
        CCC_priority_queue_decrease_w(args)
#    define priority_queue_order(args...) CCC_priority_queue_order(args)
#    define priority_queue_clear(args...) CCC_priority_queue_clear(args)
#    define priority_queue_validate(args...) CCC_priority_queue_validate(args)
#endif /* PRIORITY_QUEUE_USING_NAMESPACE_CCC */

#endif /* CCC_PRIORITY_QUEUE_H */
