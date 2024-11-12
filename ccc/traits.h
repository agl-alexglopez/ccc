/** @file
@brief The C Container Collection Traits Interface */
#ifndef CCC_TRAITS_H
#define CCC_TRAITS_H

#include "impl_traits.h"

/*======================     Entry API  =====================================*/

/** @brief Insert an element and obtain the old value if Occupied.
@param [in] container_ptr a pointer to the container.
@param insert_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define ccc_insert(container_ptr, insert_args...)                              \
    ccc_impl_insert(container_ptr, insert_args)

/** @brief Insert an element and obtain the old value if Occupied.
@param [in] container_ptr a pointer to the container.
@param insert_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define ccc_insert_r(container_ptr, insert_args...)                            \
    ccc_impl_insert_r(container_ptr, insert_args)

/** @brief Insert an element if the entry is Vacant.
@param [in] container_ptr a pointer to the container.
@param try_insert_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define ccc_try_insert(container_ptr, try_insert_args...)                      \
    ccc_impl_try_insert(container_ptr, try_insert_args)

/** @brief Insert an element if the entry is Vacant.
@param [in] container_ptr a pointer to the container.
@param try_insert_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define ccc_try_insert_r(container_ptr, try_insert_args...)                    \
    ccc_impl_try_insert_r(container_ptr, try_insert_args)

/** @brief Insert an element or overwrite the Occupied entry.
@param [in] container_ptr a pointer to the container.
@param insert_or_assign_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define ccc_insert_or_assign(container_ptr, insert_or_assign_args...)          \
    ccc_impl_insert_or_assign(container_ptr, insert_or_assign_args)

/** @brief Insert an element or overwrite the Occupied entry.
@param [in] container_ptr a pointer to the container.
@param insert_or_assign_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define ccc_insert_or_assign_r(container_ptr, insert_or_assign_args...)        \
    ccc_impl_insert_or_assign_r(container_ptr, insert_or_assign_args)

/** @brief Remove an element and retain access to its value.
@param [in] container_ptr a pointer to the container.
@param remove_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define ccc_remove(container_ptr, remove_args...)                              \
    ccc_impl_remove(container_ptr, remove_args)

/** @brief Remove an element and retain access to its value.
@param [in] container_ptr a pointer to the container.
@param remove_args args depend on container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define ccc_remove_r(container_ptr, remove_args...)                            \
    ccc_impl_remove_r(container_ptr, remove_args)

/** @brief Obtain a container specific entry for the Entry API.
@param [in] container_ptr a pointer to the container.
@param [in] key_ptr a pointer to the search key.
@return a container specific entry depending on container specific context.

See container documentation for specific behavior. */
#define ccc_entry(container_ptr, key_ptr...)                                   \
    ccc_impl_entry(container_ptr, key_ptr)

/** @brief Obtain a container specific entry for the Entry API.
@param [in] container_ptr a pointer to the container.
@param [in] key_ptr a pointer to the search key.
@return a container specific entry reference depending on container specific
context.

See container documentation for specific behavior. */
#define ccc_entry_r(container_ptr, key_ptr...)                                 \
    ccc_impl_entry_r(container_ptr, key_ptr)

/** @brief Modify an entry if Occupied.
@param [in] entry_ptr a pointer to the container.
@param [in] mod_fn a modification function that does not need aux.
@return a reference to the modified entry if Occupied or original if Vacant.

See container documentation for specific behavior. */
#define ccc_and_modify(entry_ptr, mod_fn) ccc_impl_and_modify(entry_ptr, mod_fn)

/** @brief Modify an entry if Occupied.
@param [in] entry_ptr a pointer to the container.
@param [in] mod_fn a modification function.
@param [in] aux_args auxiliary data for mod_fn.
@return a reference to the modified entry if Occupied or original if Vacant.

See container documentation for specific behavior. */
#define ccc_and_modify_aux(entry_ptr, mod_fn, aux_args...)                     \
    ccc_impl_and_modify_aux(entry_ptr, mod_fn, aux_args)

/** @brief Insert new element or overwrite old element.
@param [in] entry_ptr a pointer to the container.
@param insert_entry_args args depend on container.
@return an reference to the inserted element.

See container documentation for specific behavior. */
#define ccc_insert_entry(entry_ptr, insert_entry_args...)                      \
    ccc_impl_insert_entry(entry_ptr, insert_entry_args)

/** @brief Insert new element if the entry is Vacant.
@param [in] entry_ptr a pointer to the container.
@param or_insert_args args depend on container.
@return an reference to the old element or new element if entry was Vacant.

See container documentation for specific behavior. */
#define ccc_or_insert(entry_ptr, or_insert_args...)                            \
    ccc_impl_or_insert(entry_ptr, or_insert_args)

/** @brief Remove the element if the entry is Occupied.
@param [in] entry_ptr a pointer to the container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define ccc_remove_entry(entry_ptr) ccc_impl_remove_entry(entry_ptr)

/** @brief Remove the element if the entry is Occupied.
@param [in] entry_ptr a pointer to the container.
@return an entry depending on container specific context.

See container documentation for specific behavior. */
#define ccc_remove_entry_r(entry_ptr) ccc_impl_remove_entry_r(entry_ptr)

/** @brief Unwrap user type in entry.
@param [in] entry_ptr a pointer to the container.
@return a valid reference if Occupied or NULL if vacant.

See container documentation for specific behavior. */
#define ccc_unwrap(entry_ptr) ccc_impl_unwrap(entry_ptr)

/** @brief Check occupancy of entry.
@param [in] entry_ptr a pointer to the container.
@return true if Occupied, false if Vacant.

See container documentation for specific behavior. */
#define ccc_occupied(entry_ptr) ccc_impl_occupied(entry_ptr)

/** @brief Check last insert status.
@param [in] entry_ptr a pointer to the container.
@return true if an insert error occurred false if not.

See container documentation for specific behavior. */
#define ccc_insert_error(entry_ptr) ccc_impl_insert_error(entry_ptr)

/*======================    Misc Search API  ================================*/

/** @brief Obtain a reference to the user type stored at the key.
@param [in] container_ptr a pointer to the container.
@param [in] key_ptr a pointer to the search key.
@return a reference to the stored user type or NULL of absent.

See container documentation for specific behavior. */
#define ccc_get_key_val(container_ptr, key_ptr...)                             \
    ccc_impl_get_key_val(container_ptr, key_ptr)

/** @brief Check for membership of the key.
@param [in] container_ptr a pointer to the container.
@param [in] key_ptr a pointer to the search key.
@return true if present false if absent.

See container documentation for specific behavior. */
#define ccc_contains(container_ptr, key_ptr...)                                \
    ccc_impl_contains(container_ptr, key_ptr)

/*================       Sequential Containers API      =====================*/

/** @brief Push an element into a container.
@param [in] container_ptr a pointer to the container.
@param push_args depend on container.
@return a reference to the pushed element.

See container documentation for specific behavior. */
#define ccc_push(container_ptr, push_args...)                                  \
    ccc_impl_push(container_ptr, push_args)

/** @brief Push an element to the back of a container.
@param [in] container_ptr a pointer to the container.
@param push_args depend on container.
@return a reference to the pushed element.

See container documentation for specific behavior. */
#define ccc_push_back(container_ptr, push_args...)                             \
    ccc_impl_push_back(container_ptr, push_args)

/** @brief Push an element to the front of a container.
@param [in] container_ptr a pointer to the container.
@param push_args depend on container.
@return a reference to the pushed element.

See container documentation for specific behavior. */
#define ccc_push_front(container_ptr, push_args...)                            \
    ccc_impl_push_front(container_ptr, push_args)

/** @brief Pop an element from a container.
@param [in] container_ptr a pointer to the container.
@return a result of the pop operation.

See container documentation for specific behavior. */
#define ccc_pop(container_ptr) ccc_impl_pop(container_ptr)

/** @brief Pop an element from the front of a container.
@param [in] container_ptr a pointer to the container.
@return a result of the pop operation.

See container documentation for specific behavior. */
#define ccc_pop_front(container_ptr) ccc_impl_pop_front(container_ptr)

/** @brief Pop an element from the back of a container.
@param [in] container_ptr a pointer to the container.
@return a result of the pop operation.

See container documentation for specific behavior. */
#define ccc_pop_back(container_ptr) ccc_impl_pop_back(container_ptr)

/** @brief Obtain a reference the front element of a container.
@param [in] container_ptr a pointer to the container.
@return a reference to a user type.

See container documentation for specific behavior. */
#define ccc_front(container_ptr) ccc_impl_front(container_ptr)

/** @brief Obtain a reference the back element of a container.
@param [in] container_ptr a pointer to the container.
@return a reference to a user type.

See container documentation for specific behavior. */
#define ccc_back(container_ptr) ccc_impl_back(container_ptr)

/*================       Priority Queue Update API      =====================*/

/** @brief Update the value of an element known to be in a container.
@param [in] container_ptr a pointer to the container.
@param update_args depend on the container.

See container documentation for specific behavior. */
#define ccc_update(container_ptr, update_args...)                              \
    ccc_impl_update(container_ptr, update_args)

/** @brief Increase the value of an element known to be in a container.
@param [in] container_ptr a pointer to the container.
@param increase_args depend on the container.

See container documentation for specific behavior. */
#define ccc_increase(container_ptr, increase_args...)                          \
    ccc_impl_increase(container_ptr, increase_args)

/** @brief Decrease the value of an element known to be in a container.
@param [in] container_ptr a pointer to the container.
@param decrease_args depend on the container.

See container documentation for specific behavior. */
#define ccc_decrease(container_ptr, decrease_args...)                          \
    ccc_impl_decrease(container_ptr, decrease_args)

/** @brief Erase an element known to be in a container.
@param [in] container_ptr a pointer to the container.
@param erase_args depend on the container.

See container documentation for specific behavior. */
#define ccc_erase(container_ptr, erase_args...)                                \
    ccc_impl_erase(container_ptr, erase_args)

/** @brief Extract an element known to be in a container (does not free).
@param [in] container_ptr a pointer to the container.
@param extract_args depend on the container.

See container documentation for specific behavior. */
#define ccc_extract(container_ptr, extract_args...)                            \
    ccc_impl_extract(container_ptr, extract_args)

/** @brief Extract elements known to be in a container (does not free).
@param [in] container_ptr a pointer to the container.
@param extract_args depend on the container.

See container documentation for specific behavior. */
#define ccc_extract_range(container_ptr, extract_args...)                      \
    ccc_impl_extract_range(container_ptr, extract_args)

/*===================       Iterators API      ==============================*/

/** @brief Obtain a reference to the start of a container.
@param [in] container_ptr a pointer to the container.
@return a reference to the user type stored at the start.

See container documentation for specific behavior. */
#define ccc_begin(container_ptr) ccc_impl_begin(container_ptr)

/** @brief Obtain a reference to the reversed start of a container.
@param [in] container_ptr a pointer to the container.
@return a reference to the user type stored at the reversed start.

See container documentation for specific behavior. */
#define ccc_rbegin(container_ptr) ccc_impl_rbegin(container_ptr)

/** @brief Obtain a reference to the next element in the container.
@param [in] container_ptr a pointer to the container.
@param [in] void_iterator_ptr the user type returned from the last iteration.
@return a reference to the user type stored next.

See container documentation for specific behavior. */
#define ccc_next(container_ptr, void_iterator_ptr)                             \
    ccc_impl_next(container_ptr, void_iterator_ptr)

/** @brief Obtain a reference to the rnext element in the container.
@param [in] container_ptr a pointer to the container.
@param [in] void_iterator_ptr the user type returned from the last iteration.
@return a reference to the user type stored rnext.

See container documentation for specific behavior. */
#define ccc_rnext(container_ptr, void_iterator_ptr)                            \
    ccc_impl_rnext(container_ptr, void_iterator_ptr)

/** @brief Obtain a reference to the end sentinel of a container.
@param [in] container_ptr a pointer to the container.
@return a reference to the end sentinel.

See container documentation for specific behavior. */
#define ccc_end(container_ptr) ccc_impl_end(container_ptr)

/** @brief Obtain a reference to the rend sentinel of a container.
@param [in] container_ptr a pointer to the container.
@return a reference to the rend sentinel.

See container documentation for specific behavior. */
#define ccc_rend(container_ptr) ccc_impl_rend(container_ptr)

/** @brief Obtain a range of values from a container.
@param [in] container_ptr a pointer to the container.
@param range_args are container specific.
@return the range.

See container documentation for specific behavior. */
#define ccc_equal_range(container_ptr, range_args...)                          \
    ccc_impl_equal_range(container_ptr, range_args)

/** @brief Obtain a range of values from a container.
@param [in] container_ptr a pointer to the container.
@param range_args are container specific.
@return a reference to the range.

See container documentation for specific behavior. */
#define ccc_equal_range_r(container_ptr, range_args...)                        \
    ccc_impl_equal_range_r(container_ptr, range_args)

/** @brief Obtain a rrange of values from a container.
@param [in] container_ptr a pointer to the container.
@param rrange_args are container specific.
@return the rrange.

See container documentation for specific behavior. */
#define ccc_equal_rrange(container_ptr, rrange_args...)                        \
    ccc_impl_equal_rrange(container_ptr, rrange_args)

/** @brief Obtain a rrange of values from a container.
@param [in] container_ptr a pointer to the container.
@param rrange_args are container specific.
@return a reference to the rrange.

See container documentation for specific behavior. */
#define ccc_equal_rrange_r(container_ptr, rrange_args...)                      \
    ccc_impl_equal_rrange_r(container_ptr, rrange_args)

/** @brief Splice an element from one position to another in the same or a
different container.
@param [in] container_ptr a pointer to the container.
@param splice_args are container specific.
@return the result of the splice.

See container documentation for specific behavior. */
#define ccc_splice(container_ptr, splice_args...)                              \
    ccc_impl_splice(container_ptr, splice_args)

/** @brief Splice a range of elements from one position to another in the same
or a different container.
@param [in] container_ptr a pointer to the container.
@param splice_args are container specific.
@return the result of the splice.

See container documentation for specific behavior. */
#define ccc_splice_range(container_ptr, splice_args...)                        \
    ccc_impl_splice_range(container_ptr, splice_args)

/*===================    Standard Getters API  ==============================*/

/** @brief Return the size of the container.
@param [in] container_ptr a pointer to the container.
@return the size or zero if container_ptr is NULL.

See container documentation for specific behavior. */
#define ccc_size(container_ptr) ccc_impl_size(container_ptr)

/** @brief Return the size status of a container.
@param [in] container_ptr a pointer to the container.
@return true if empty or NULL false if not.

See container documentation for specific behavior. */
#define ccc_is_empty(container_ptr) ccc_impl_is_empty(container_ptr)

/** @brief Return the invariant statuses of the container.
@param [in] container_ptr a pointer to the container.
@return true if all invariants hold, false if not.

See container documentation for specific behavior. */
#define ccc_validate(container_ptr) ccc_impl_validate(container_ptr)

/** Define this preprocessor directive to shorten trait names. */
#ifdef TRAITS_USING_NAMESPACE_CCC

#    define insert(args...) ccc_insert(args)
#    define try_insert(args...) ccc_try_insert(args)
#    define insert_or_assign(args...) ccc_insert_or_assign(args)
#    define try_insert_r(args...) ccc_try_insert_r(args)
#    define insert_r(args...) ccc_insert_r(args)
#    define remove(args...) ccc_remove(args)
#    define remove_r(args...) ccc_remove_r(args)
#    define remove_entry(args...) ccc_remove_entry(args)
#    define remove_entry_r(args...) ccc_remove_entry_r(args)
#    define entry(args...) ccc_entry(args)
#    define entry_r(args...) ccc_entry_r(args)
#    define or_insert(args...) ccc_or_insert(args)
#    define insert_entry(args...) ccc_insert_entry(args)
#    define and_modify(args...) ccc_and_modify(args)
#    define and_modify_aux(args...) ccc_and_modify_aux(args)
#    define occupied(args...) ccc_occupied(args)
#    define insert_error(args...) ccc_insert_error(args)
#    define unwrap(args...) ccc_unwrap(args)

#    define push(args...) ccc_push(args)
#    define push_back(args...) ccc_push_back(args)
#    define push_front(args...) ccc_push_front(args)
#    define pop(args...) ccc_pop(args)
#    define pop_front(args...) ccc_pop_front(args)
#    define pop_back(args...) ccc_pop_back(args)
#    define front(args...) ccc_front(args)
#    define back(args...) ccc_back(args)
#    define update(args...) ccc_update(args)
#    define increase(args...) ccc_increase(args)
#    define decrease(args...) ccc_decrease(args)
#    define erase(args...) ccc_erase(args)
#    define extract(args...) ccc_extract(args)
#    define extract_range(args...) ccc_extract_range(args)

#    define get_key_val(args...) ccc_get_key_val(args)
#    define get_mut(args...) ccc_get_key_val_mut(args)
#    define contains(args...) ccc_contains(args)

#    define begin(args...) ccc_begin(args)
#    define rbegin(args...) ccc_rbegin(args)
#    define next(args...) ccc_next(args)
#    define rnext(args...) ccc_rnext(args)
#    define end(args...) ccc_end(args)
#    define rend(args...) ccc_rend(args)

#    define equal_range(args...) ccc_equal_range(args)
#    define equal_rrange(args...) ccc_equal_rrange(args)
#    define equal_range_r(args...) ccc_equal_range_r(args)
#    define equal_rrange_r(args...) ccc_equal_rrange_r(args)
#    define splice(args...) ccc_splice(args)
#    define splice_range(args...) ccc_splice_range(args)

#    define size(args...) ccc_size(args)
#    define is_empty(args...) ccc_is_empty(args)
#    define validate(args...) ccc_validate(args)

#endif /* CCC_USING_NAMESPACE_CCC */

#endif /* CCC_TRAITS_H */
