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
@brief The C Container Collection Traits Interface

Many functionalities across containers are similar. These can be described as
traits that each container implements (see Rust Traits for a more pure example
of the topic). Only a selections of shared traits across containers are
represented here because some containers implement unique functionality that
cannot be shared with other containers. These can simplify code greatly at a
slightly higher compilation time cost. There is no runtime cost to using
traits.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define TRAITS_USING_NAMESPACE_CCC
```

All traits can then be written without the `CCC_` prefix. */
#ifndef CCC_TRAITS_H
#define CCC_TRAITS_H

#include "private/private_traits.h"

/** @name Entry Interface
Obtain and operate on container entries for efficient queries when non-trivial
control flow is needed. */
/**@{*/

/** @brief Insert an element and obtain the old value if Occupied.
@param[in] container_pointer a pointer to the container.
@param swap_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define CCC_swap_entry(container_pointer, swap_args...)                        \
    CCC_private_swap_entry(container_pointer, swap_args)

/** @brief Insert an element and obtain the old value if Occupied.
@param[in] container_pointer a pointer to the container.
@param swap_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define CCC_swap_entry_wrap(container_pointer, swap_args...)                   \
    CCC_private_swap_entry_wrap(container_pointer, swap_args)

/** @brief Insert an element and obtain the old value if Occupied.
@param[in] container_pointer a pointer to the container.
@param swap_args args depend on container.
@return a handle depending on container specific context.

See container documentation for specific behavior. */
#define CCC_swap_handle(container_pointer, swap_args...)                       \
    CCC_private_swap_handle(container_pointer, swap_args)

/** @brief Insert an element and obtain the old value if Occupied.
@param[in] container_pointer a pointer to the container.
@param swap_args args depend on container.
@return a handle depending on container specific context.

See container documentation for specific behavior. */
#define CCC_swap_array_wrap(container_pointer, swap_args...)                   \
    CCC_private_swap_array_wrap(container_pointer, swap_args)

/** @brief Insert an element if the entry is Vacant.
@param[in] container_pointer a pointer to the container.
@param try_insert_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define CCC_try_insert(container_pointer, try_insert_args...)                  \
    CCC_private_try_insert(container_pointer, try_insert_args)

/** @brief Insert an element if the entry is Vacant.
@param[in] container_pointer a pointer to the container.
@param try_insert_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define CCC_try_insert_wrap(container_pointer, try_insert_args...)             \
    CCC_private_try_insert_wrap(container_pointer, try_insert_args)

/** @brief Insert an element or overwrite the Occupied entry.
@param[in] container_pointer a pointer to the container.
@param insert_or_assign_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define CCC_insert_or_assign(container_pointer, insert_or_assign_args...)      \
    CCC_private_insert_or_assign(container_pointer, insert_or_assign_args)

/** @brief Insert an element or overwrite the Occupied entry.
@param[in] container_pointer a pointer to the container.
@param insert_or_assign_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define CCC_insert_or_assign_wrap(container_pointer, insert_or_assign_args...) \
    CCC_private_insert_or_assign_wrap(container_pointer, insert_or_assign_args)

/** @brief Remove an element and retain access to its value.
@param[in] container_pointer a pointer to the container.
@param remove_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define CCC_remove(container_pointer, remove_args...)                          \
    CCC_private_remove(container_pointer, remove_args)

/** @brief Remove an element and retain access to its value.
@param[in] container_pointer a pointer to the container.
@param remove_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define CCC_remove_wrap(container_pointer, remove_args...)                     \
    CCC_private_remove_wrap(container_pointer, remove_args)

/** @brief Obtain a container specific entry for the Entry Interface.
@param[in] container_pointer a pointer to the container.
@param[in] key_pointer a pointer to the search key.
@return a container specific entry depending on container specific context.

See container documentation for specific behavior. */
#define CCC_entry(container_pointer, key_pointer...)                           \
    CCC_private_entry(container_pointer, key_pointer)

/** @brief Obtain a container specific handle for the handle Interface.
@param[in] container_pointer a pointer to the container.
@param[in] key_pointer a pointer to the search key.
@return a container specific handle depending on container specific context.

See container documentation for specific behavior. */
#define CCC_handle(container_pointer, key_pointer...)                          \
    CCC_private_handle(container_pointer, key_pointer)

/** @brief Obtain a container specific entry for the Entry Interface.
@param[in] container_pointer a pointer to the container.
@param[in] key_pointer a pointer to the search key.
@return a container specific entry reference depending on container specific
context.

See container documentation for specific behavior. */
#define CCC_entry_wrap(container_pointer, key_pointer...)                      \
    CCC_private_entry_wrap(container_pointer, key_pointer)

/** @brief Obtain a container specific handle for the handle Interface.
@param[in] container_pointer a pointer to the container.
@param[in] key_pointer a pointer to the search key.
@return a container specific handle reference depending on container specific
context.

See container documentation for specific behavior. */
#define CCC_array_wrap(container_pointer, key_pointer...)                      \
    CCC_private_array_wrap(container_pointer, key_pointer)

/** @brief Modify an entry if Occupied.
@param[in] entry_pointer a pointer to the container.
@param[in] mod_fn a modification function that does not need context.
@return a reference to the modified entry if Occupied or original if Vacant.

See container documentation for specific behavior. */
#define CCC_and_modify(entry_pointer, mod_fn)                                  \
    CCC_private_and_modify(entry_pointer, mod_fn)

/** @brief Modify an entry if Occupied.
@param[in] entry_pointer a pointer to the container.
@param[in] modify a modification function.
@param[in] context_args context data for mod_fn.
@return a reference to the modified entry if Occupied or original if Vacant.

See container documentation for specific behavior. */
#define CCC_and_modify_context(entry_pointer, modify, context_args...)         \
    CCC_private_and_modify_context(entry_pointer, modify, context_args)

/** @brief Insert new element or overwrite old element.
@param[in] entry_pointer a pointer to the container.
@param insert_entry_args args depend on container.
@return an reference to the inserted element.

See container documentation for specific behavior. */
#define CCC_insert_entry(entry_pointer, insert_entry_args...)                  \
    CCC_private_insert_entry(entry_pointer, insert_entry_args)

/** @brief Insert new element or overwrite old element.
@param[in] array_pointer a pointer to the container.
@param insert_array_args args depend on container.
@return an reference to the inserted element.

See container documentation for specific behavior. */
#define CCC_insert_handle(array_pointer, insert_array_args...)                 \
    CCC_private_insert_handle(array_pointer, insert_array_args)

/** @brief Insert new element if the entry is Vacant.
@param[in] entry_pointer a pointer to the container.
@param or_insert_args args depend on container.
@return an reference to the old element or new element if entry was Vacant.

See container documentation for specific behavior. */
#define CCC_or_insert(entry_pointer, or_insert_args...)                        \
    CCC_private_or_insert(entry_pointer, or_insert_args)

/** @brief Remove the element if the entry is Occupied.
@param[in] entry_pointer a pointer to the container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define CCC_remove_entry(entry_pointer) CCC_private_remove_entry(entry_pointer)

/** @brief Remove the element if the entry is Occupied.
@param[in] entry_pointer a pointer to the container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define CCC_remove_entry_wrap(entry_pointer)                                   \
    CCC_private_remove_entry_wrap(entry_pointer)

/** @brief Remove the element if the handle is Occupied.
@param[in] array_pointer a pointer to the container.
@return an handle depending on container specific context.

See container documentation for specific behavior. */
#define CCC_remove_handle(array_pointer)                                       \
    CCC_private_remove_handle(array_pointer)

/** @brief Remove the element if the handle is Occupied.
@param[in] array_pointer a pointer to the container.
@return an handle depending on container specific context.

See container documentation for specific behavior. */
#define CCC_remove_array_wrap(array_pointer)                                   \
    CCC_private_remove_array_wrap(array_pointer)

/** @brief Unwrap user type in entry.
@param[in] entry_pointer a pointer to the container.
@return a valid reference if Occupied or NULL if vacant.

See container documentation for specific behavior. */
#define CCC_unwrap(entry_pointer) CCC_private_unwrap(entry_pointer)

/** @brief Check occupancy of entry.
@param[in] entry_pointer a pointer to the container.
@return true if Occupied, false if Vacant.

See container documentation for specific behavior. */
#define CCC_occupied(entry_pointer) CCC_private_occupied(entry_pointer)

/** @brief Check last insert status.
@param[in] entry_pointer a pointer to the container.
@return true if an insert error occurred false if not.

See container documentation for specific behavior. */
#define CCC_insert_error(entry_pointer) CCC_private_insert_error(entry_pointer)

/**@}*/

/**@name Membership Interface
Test membership or obtain references to stored user types directly. */
/**@{*/

/** @brief Obtain a reference to the user type stored at the key.
@param[in] container_pointer a pointer to the container.
@param[in] key_pointer a pointer to the search key.
@return a reference to the stored user type or NULL of absent.

See container documentation for specific behavior. */
#define CCC_get_key_value(container_pointer, key_pointer...)                   \
    CCC_private_get_key_value(container_pointer, key_pointer)

/** @brief Check for membership of the key.
@param[in] container_pointer a pointer to the container.
@param[in] key_pointer a pointer to the search key.
@return true if present false if absent.

See container documentation for specific behavior. */
#define CCC_contains(container_pointer, key_pointer...)                        \
    CCC_private_contains(container_pointer, key_pointer)

/**@}*/

/**@name Push Pop Front Back Interface
Push, pop, and view elements in sorted or unsorted containers. */
/**@{*/

/** @brief Push an element into a container.
@param[in] container_pointer a pointer to the container.
@param push_args depend on container.
@return a reference to the pushed element.

See container documentation for specific behavior. */
#define CCC_push(container_pointer, push_args...)                              \
    CCC_private_push(container_pointer, push_args)

/** @brief Push an element to the back of a container.
@param[in] container_pointer a pointer to the container.
@param push_args depend on container.
@return a reference to the pushed element.

See container documentation for specific behavior. */
#define CCC_push_back(container_pointer, push_args...)                         \
    CCC_private_push_back(container_pointer, push_args)

/** @brief Push an element to the front of a container.
@param[in] container_pointer a pointer to the container.
@param push_args depend on container.
@return a reference to the pushed element.

See container documentation for specific behavior. */
#define CCC_push_front(container_pointer, push_args...)                        \
    CCC_private_push_front(container_pointer, push_args)

/** @brief Pop an element from a container.
@param[in] container_pointer a pointer to the container.
@param[in] pop_args any supplementary args a container may have for the pop.
@return a result of the pop operation.

See container documentation for specific behavior. */
#define CCC_pop(container_pointer, pop_args...)                                \
    CCC_private_pop(container_pointer, pop_args)

/** @brief Pop an element from the front of a container.
@param[in] container_pointer a pointer to the container.
@return a result of the pop operation.

See container documentation for specific behavior. */
#define CCC_pop_front(container_pointer)                                       \
    CCC_private_pop_front(container_pointer)

/** @brief Pop an element from the back of a container.
@param[in] container_pointer a pointer to the container.
@return a result of the pop operation.

See container documentation for specific behavior. */
#define CCC_pop_back(container_pointer) CCC_private_pop_back(container_pointer)

/** @brief Obtain a reference the front element of a container.
@param[in] container_pointer a pointer to the container.
@return a reference to a user type.

See container documentation for specific behavior. */
#define CCC_front(container_pointer) CCC_private_front(container_pointer)

/** @brief Obtain a reference the back element of a container.
@param[in] container_pointer a pointer to the container.
@return a reference to a user type.

See container documentation for specific behavior. */
#define CCC_back(container_pointer) CCC_private_back(container_pointer)

/** @brief Splice an element from one position to another in the same or a
different container.
@param[in] container_pointer a pointer to the container.
@param splice_args are container specific.
@return the result of the splice.

See container documentation for specific behavior. */
#define CCC_splice(container_pointer, splice_args...)                          \
    CCC_private_splice(container_pointer, splice_args)

/** @brief Splice a range of elements from one position to another in the same
or a different container.
@param[in] container_pointer a pointer to the container.
@param splice_args are container specific.
@return the result of the splice.

See container documentation for specific behavior. */
#define CCC_splice_range(container_pointer, splice_args...)                    \
    CCC_private_splice_range(container_pointer, splice_args)

/**@}*/

/**@name Priority Queue Interface
Interface to support generic priority queue operations. */
/**@{*/

/** @brief Update the value of an element known to be in a container.
@param[in] container_pointer a pointer to the container.
@param update_args depend on the container.

See container documentation for specific behavior. */
#define CCC_update(container_pointer, update_args...)                          \
    CCC_private_update(container_pointer, update_args)

/** @brief Increase the value of an element known to be in a container.
@param[in] container_pointer a pointer to the container.
@param increase_args depend on the container.

See container documentation for specific behavior. */
#define CCC_increase(container_pointer, increase_args...)                      \
    CCC_private_increase(container_pointer, increase_args)

/** @brief Decrease the value of an element known to be in a container.
@param[in] container_pointer a pointer to the container.
@param decrease_args depend on the container.

See container documentation for specific behavior. */
#define CCC_decrease(container_pointer, decrease_args...)                      \
    CCC_private_decrease(container_pointer, decrease_args)

/** @brief Erase an element known to be in a container.
@param[in] container_pointer a pointer to the container.
@param erase_args depend on the container.

See container documentation for specific behavior. */
#define CCC_erase(container_pointer, erase_args...)                            \
    CCC_private_erase(container_pointer, erase_args)

/** @brief Extract an element known to be in a container (does not free).
@param[in] container_pointer a pointer to the container.
@param extract_args depend on the container.

See container documentation for specific behavior. */
#define CCC_extract(container_pointer, extract_args...)                        \
    CCC_private_extract(container_pointer, extract_args)

/** @brief Extract elements known to be in a container (does not free).
@param[in] container_pointer a pointer to the container.
@param extract_args depend on the container.

See container documentation for specific behavior. */
#define CCC_extract_range(container_pointer, extract_args...)                  \
    CCC_private_extract_range(container_pointer, extract_args)

/**@}*/

/** @name Iterator Interface
Obtain and manage iterators over the container. */
/**@{*/

/** @brief Obtain a reference to the start of a container.
@param[in] container_pointer a pointer to the container.
@return a reference to the user type stored at the start.

See container documentation for specific behavior. */
#define CCC_begin(container_pointer) CCC_private_begin(container_pointer)

/** @brief Obtain a reference to the reversed start of a container.
@param[in] container_pointer a pointer to the container.
@return a reference to the user type stored at the reversed start.

See container documentation for specific behavior. */
#define CCC_reverse_begin(container_pointer)                                   \
    CCC_private_reverse_begin(container_pointer)

/** @brief Obtain a reference to the next element in the container.
@param[in] container_pointer a pointer to the container.
@param[in] void_iterator_pointer the user type returned from the last
iteration.
@return a reference to the user type stored next.

See container documentation for specific behavior. */
#define CCC_next(container_pointer, void_iterator_pointer)                     \
    CCC_private_next(container_pointer, void_iterator_pointer)

/** @brief Obtain a reference to the reverse_next element in the container.
@param[in] container_pointer a pointer to the container.
@param[in] void_iterator_pointer the user type returned from the last
iteration.
@return a reference to the user type stored reverse_next.

See container documentation for specific behavior. */
#define CCC_reverse_next(container_pointer, void_iterator_pointer)             \
    CCC_private_reverse_next(container_pointer, void_iterator_pointer)

/** @brief Obtain a reference to the end sentinel of a container.
@param[in] container_pointer a pointer to the container.
@return a reference to the end sentinel.

See container documentation for specific behavior. */
#define CCC_end(container_pointer) CCC_private_end(container_pointer)

/** @brief Obtain a reference to the reverse_end sentinel of a container.
@param[in] container_pointer a pointer to the container.
@return a reference to the reverse_end sentinel.

See container documentation for specific behavior. */
#define CCC_reverse_end(container_pointer)                                     \
    CCC_private_reverse_end(container_pointer)

/** @brief Obtain a range of values from a container.
@param[in] container_pointer a pointer to the container.
@param range_args are container specific.
@return the range.

See container documentation for specific behavior. */
#define CCC_equal_range(container_pointer, range_args...)                      \
    CCC_private_equal_range(container_pointer, range_args)

/** @brief Obtain a range of values from a container.
@param[in] container_pointer a pointer to the container.
@param range_args are container specific.
@return a reference to the range.

See container documentation for specific behavior. */
#define CCC_equal_range_wrap(container_pointer, range_args...)                 \
    CCC_private_equal_range_wrap(container_pointer, range_args)

/** @brief Obtain a range_reverse of values from a container.
@param[in] container_pointer a pointer to the container.
@param range_reverse_args are container specific.
@return the range_reverse.

See container documentation for specific behavior. */
#define CCC_equal_range_reverse(container_pointer, range_reverse_args...)      \
    CCC_private_equal_range_reverse(container_pointer, range_reverse_args)

/** @brief Obtain a range_reverse of values from a container.
@param[in] container_pointer a pointer to the container.
@param range_reverse_args are container specific.
@return a reference to the range_reverse.

See container documentation for specific behavior. */
#define CCC_equal_range_reverse_wrap(container_pointer, range_reverse_args...) \
    CCC_private_equal_range_reverse_wrap(container_pointer, range_reverse_args)

/** @brief Obtain the beginning of the range iterator.
@param[in] range_pointer a pointer to the type of range.
@return the iterator representing the beginning. May be equal to end. */
#define CCC_range_begin(range_pointer) CCC_private_range_begin(range_pointer)

/** @brief Obtain the end of the range iterator.
@param[in] range_pointer a pointer to the type of range.
@return the iterator representing the end.
@warning Do not access the end. It is an exclusive end. */
#define CCC_range_end(range_pointer) CCC_private_range_end(range_pointer)

/** @brief Obtain the beginning of the reverse range iterator.
@param[in] range_reverse_pointer a pointer to the type of reverse range.
@return the iterator representing the reverse beginning. May be equal to reverse
end. */
#define CCC_range_reverse_begin(range_reverse_pointer)                         \
    CCC_private_range_reverse_begin(range_reverse_pointer)

/** @brief Obtain the end of the reverse range iterator.
@param[in] range_reverse_pointer a pointer to the type of reverse range.
@return the iterator representing the reverse end.
@warning Do not access the end. It is an exclusive reverse end. */
#define CCC_range_reverse_end(range_reverse_pointer)                           \
    CCC_private_range_reverse_end(range_reverse_pointer)

/**@}*/

/** @name Memory Management Interface
Manage underlying buffers for containers. */
/**@{*/

/** @brief Copy source containers memory to destination container.
@param[in] destination_container_pointer a pointer to the destination container.
@param[in] source_container_pointer a pointer to the source container.
@param[in] allocate_pointer the allocation function to use for resizing if
needed.
@return the result of the operation.

See container documentation for specific behavior. */
#define CCC_copy(destination_container_pointer, source_container_pointer,      \
                 allocate_pointer)                                             \
    CCC_private_copy(destination_container_pointer, source_container_pointer,  \
                     allocate_pointer)

/** @brief Reserve capacity for n_to_add new elements to be inserted.
@param[in] container_pointer a pointer to the container.
@param[in] n_to_add the number of elements to add to the container.
@param[in] allocate_pointer the allocation function to use for resizing if
needed.
@return the result of the operation.

See container documentation for specific behavior. */
#define CCC_reserve(container_pointer, n_to_add, allocate_pointer)             \
    CCC_private_reserve(container_pointer, n_to_add, allocate_pointer)

/** @brief Clears the container without freeing the underlying buffer.
@param[in] container_pointer a pointer to the container.
@param[in] destructor_args optional function to be called on each element.
@return the result of the operation.

See container documentation for specific behavior. */
#define CCC_clear(container_pointer, destructor_args...)                       \
    CCC_private_clear(container_pointer, destructor_args)

/** @brief Clears the container and frees the underlying buffer.
@param[in] container_pointer a pointer to the container.
@param[in] destructor_and_free_args optional function to be called on each
element.
@return the result of the operation.

See container documentation for specific behavior. */
#define CCC_clear_and_free(container_pointer, destructor_and_free_args...)     \
    CCC_private_clear_and_free(container_pointer, destructor_and_free_args)

/** @brief Clears the container previously reserved and frees its underlying
buffer. Covers the case of a one-time memory reserved container that does not
otherwise have permissions over its own memory to resize or free.
@param[in] container_pointer a pointer to the container.
@param[in] destructor_and_free_args optional destructor function to be called
on each element and the required allocation function to free memory.
@return the result of the operation.

See container documentation for specific behavior. */
#define CCC_clear_and_free_reserve(container_pointer,                          \
                                   destructor_and_free_args...)                \
    CCC_private_clear_and_free_reserve(container_pointer,                      \
                                       destructor_and_free_args)

/**@}*/

/** @name State Interface
Obtain the container state. */
/**@{*/

/** @brief Return the count of elements in the container.
@param[in] container_pointer a pointer to the container.
@return the size or an argument error is set if container_pointer is NULL.

See container documentation for specific behavior. */
#define CCC_count(container_pointer) CCC_private_count(container_pointer)

/** @brief Return the capacity of the container.
@param[in] container_pointer a pointer to the container.
@return the capacity or an argument error is set if container_pointer is NULL.

See container documentation for specific behavior. */
#define CCC_capacity(container_pointer) CCC_private_capacity(container_pointer)

/** @brief Return the size status of a container.
@param[in] container_pointer a pointer to the container.
@return true if empty or NULL false if not.

See container documentation for specific behavior. */
#define CCC_is_empty(container_pointer) CCC_private_is_empty(container_pointer)

/** @brief Return the invariant statuses of the container.
@param[in] container_pointer a pointer to the container.
@return true if all invariants hold, false if not.

See container documentation for specific behavior. */
#define CCC_validate(container_pointer) CCC_private_validate(container_pointer)

/**@}*/

/** Define this preprocessor directive to shorten trait names. */
#ifdef TRAITS_USING_NAMESPACE_CCC
#    define swap_entry(args...) CCC_swap_entry(args)
#    define swap_entry_wrap(args...) CCC_swap_entry_wrap(args)
#    define swap_handle(args...) CCC_swap_handle(args)
#    define swap_array_wrap(args...) CCC_swap_array_wrap(args)
#    define try_insert(args...) CCC_try_insert(args)
#    define insert_or_assign(args...) CCC_insert_or_assign(args)
#    define insert_or_assign_wrap(args...) CCC_insert_or_assign_wrap(args)
#    define try_insert_wrap(args...) CCC_try_insert_wrap(args)
#    define remove(args...) CCC_remove(args)
#    define remove_wrap(args...) CCC_remove_wrap(args)
#    define remove_entry(args...) CCC_remove_entry(args)
#    define remove_entry_wrap(args...) CCC_remove_entry_wrap(args)
#    define remove_handle(args...) CCC_remove_handle(args)
#    define remove_array_wrap(args...) CCC_remove_array_wrap(args)
#    define entry(args...) CCC_entry(args)
#    define entry_wrap(args...) CCC_entry_wrap(args)
#    define handle(args...) CCC_handle(args)
#    define array_wrap(args...) CCC_array_wrap(args)
#    define or_insert(args...) CCC_or_insert(args)
#    define insert_entry(args...) CCC_insert_entry(args)
#    define insert_handle(args...) CCC_insert_handle(args)
#    define and_modify(args...) CCC_and_modify(args)
#    define and_modify_context(args...) CCC_and_modify_context(args)
#    define occupied(args...) CCC_occupied(args)
#    define insert_error(args...) CCC_insert_error(args)
#    define unwrap(args...) CCC_unwrap(args)

#    define push(args...) CCC_push(args)
#    define push_back(args...) CCC_push_back(args)
#    define push_front(args...) CCC_push_front(args)
#    define pop(args...) CCC_pop(args)
#    define pop_front(args...) CCC_pop_front(args)
#    define pop_back(args...) CCC_pop_back(args)
#    define front(args...) CCC_front(args)
#    define back(args...) CCC_back(args)
#    define update(args...) CCC_update(args)
#    define increase(args...) CCC_increase(args)
#    define decrease(args...) CCC_decrease(args)
#    define erase(args...) CCC_erase(args)
#    define extract(args...) CCC_extract(args)
#    define extract_range(args...) CCC_extract_range(args)

#    define get_key_value(args...) CCC_get_key_value(args)
#    define get_mut(args...) CCC_get_key_value_mut(args)
#    define contains(args...) CCC_contains(args)

#    define begin(args...) CCC_begin(args)
#    define reverse_begin(args...) CCC_reverse_begin(args)
#    define next(args...) CCC_next(args)
#    define reverse_next(args...) CCC_reverse_next(args)
#    define end(args...) CCC_end(args)
#    define reverse_end(args...) CCC_reverse_end(args)

#    define equal_range(args...) CCC_equal_range(args)
#    define equal_range_reverse(args...) CCC_equal_range_reverse(args)
#    define equal_range_wrap(args...) CCC_equal_range_wrap(args)
#    define equal_range_reverse_wrap(args...) CCC_equal_range_reverse_wrap(args)
#    define range_begin(args...) CCC_range_begin(args)
#    define range_end(args...) CCC_range_end(args)
#    define range_reverse_begin(args...) CCC_range_reverse_begin(args)
#    define range_reverse_end(args...) CCC_range_reverse_end(args)
#    define splice(args...) CCC_splice(args)
#    define splice_range(args...) CCC_splice_range(args)

#    define copy(args...) CCC_copy(args)
#    define reserve(args...) CCC_reserve(args)
#    define clear(args...) CCC_clear(args)
#    define clear_and_free(args...) CCC_clear_and_free(args)
#    define clear_and_free_reserve(args...) CCC_clear_and_free_reserve(args)

#    define count(args...) CCC_count(args)
#    define capacity(args...) CCC_capacity(args)
#    define is_empty(args...) CCC_is_empty(args)
#    define validate(args...) CCC_validate(args)
#endif /* TRAITS_USING_NAMESPACE_CCC */

#endif /* CCC_TRAITS_H */
