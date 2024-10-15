#ifndef CCC_ORDERED_MULTIMAP_H
#define CCC_ORDERED_MULTIMAP_H

#include "impl_ordered_multimap.h"
#include "impl_tree.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

typedef union
{
    struct ccc_node_ impl_;
} ccc_omm_elem;

typedef union
{
    struct ccc_tree_ impl_;
} ccc_ordered_multimap;

typedef union
{
    struct ccc_tree_entry_ impl_;
} ccc_o_mm_entry;

#define ccc_omm_init(struct_name, omm_elem_field, key_field, omm_name,         \
                     alloc_fn, key_cmp_fn, aux)                                \
    ccc_impl_omm_init(struct_name, omm_elem_field, key_field, omm_name,        \
                      alloc_fn, key_cmp_fn, aux)

/*=======================    Lazy Construction   ============================*/

#define ccc_omm_and_modify_w(ordered_multimap_entry_ptr, mod_fn, aux_data...)  \
    &(ccc_o_mm_entry)                                                          \
    {                                                                          \
        ccc_impl_omm_and_modify_w(ordered_multimap_entry_ptr, mod_fn,          \
                                  aux_data)                                    \
    }

#define ccc_omm_or_insert_w(ordered_multimap_entry_ptr, lazy_key_value...)     \
    ccc_impl_omm_or_insert_w(ordered_multimap_entry_ptr, lazy_key_value)

#define ccc_omm_insert_entry_w(ordered_multimap_entry_ptr, lazy_key_value...)  \
    ccc_impl_omm_insert_entry_w(ordered_multimap_entry_ptr, lazy_key_value)

#define ccc_omm_try_insert_w(ordered_multimap_ptr, key, lazy_value...)         \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_omm_try_insert_w(ordered_multimap_ptr, key, lazy_value)       \
    }

#define ccc_omm_insert_or_assign_w(ordered_multimap_ptr, key, lazy_value...)   \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_omm_insert_or_assign_w(ordered_multimap_ptr, key, lazy_value) \
    }

/*=========================    Membership   =================================*/

bool ccc_omm_contains(ccc_ordered_multimap *mm, void const *key);
void *ccc_omm_get_key_val(ccc_ordered_multimap *mm, void const *key);

/*=========================    Entry API    =================================*/

/** @brief Returns an entry pointing to the newly inserted element and a status
indicating if the map has already be Occupied at the given key.
@param [in] mm a pointer to the multimap.
@param [in] e a handle to the new key value to be inserted.
@return an entry that can be unwrapped to view the inserted element. The status
will be Occupied if this element is a duplicate added to a duplicate list or
Vacant if this key is the first of its value inserted into the multimap. If the
element cannot be added due to an allocator error, an insert error is set. */
ccc_entry ccc_omm_insert(ccc_ordered_multimap *mm, ccc_omm_elem *e);

ccc_entry ccc_omm_try_insert(ccc_ordered_multimap *,
                             ccc_omm_elem *key_val_handle);

ccc_entry ccc_omm_insert_or_assign(ccc_ordered_multimap *,
                                   ccc_omm_elem *key_val_handle);

ccc_entry ccc_omm_remove(ccc_ordered_multimap *, ccc_omm_elem *out_handle);

/* Standard Entry API. */

#define ccc_omm_entry_vr(ordered_multimap_ptr, key_ptr)                        \
    &(ccc_o_mm_entry)                                                          \
    {                                                                          \
        ccc_omm_entry((ordered_multimap_ptr), (key_ptr)).impl_                 \
    }

ccc_o_mm_entry ccc_omm_entry(ccc_ordered_multimap *mm, void const *key);

ccc_o_mm_entry *ccc_omm_and_modify(ccc_o_mm_entry *e, ccc_update_fn *fn);

ccc_o_mm_entry *ccc_omm_and_modify_aux(ccc_o_mm_entry *e, ccc_update_fn *fn,
                                       void *aux);

void *ccc_omm_or_insert(ccc_o_mm_entry const *e, ccc_omm_elem *elem);

void *ccc_omm_insert_entry(ccc_o_mm_entry const *e, ccc_omm_elem *elem);

ccc_entry ccc_omm_remove_entry(ccc_o_mm_entry *e);

void *ccc_omm_unwrap(ccc_o_mm_entry const *e);
bool ccc_omm_insert_error(ccc_o_mm_entry const *e);
bool ccc_omm_occupied(ccc_o_mm_entry const *e);

/*===================    Priority Queue Helpers    ==========================*/

ccc_result ccc_omm_pop_max(ccc_ordered_multimap *mm);

ccc_result ccc_omm_pop_min(ccc_ordered_multimap *mm);

void *ccc_omm_max(ccc_ordered_multimap *mm);

void *ccc_omm_min(ccc_ordered_multimap *mm);

bool ccc_omm_is_max(ccc_ordered_multimap *mm, ccc_omm_elem const *);

bool ccc_omm_is_min(ccc_ordered_multimap *mm, ccc_omm_elem const *);

void *ccc_omm_erase(ccc_ordered_multimap *mm, ccc_omm_elem *);

bool ccc_omm_update(ccc_ordered_multimap *mm, ccc_omm_elem *, ccc_update_fn *,
                    void *);

bool ccc_omm_increase(ccc_ordered_multimap *mm, ccc_omm_elem *, ccc_update_fn *,
                      void *);

bool ccc_omm_decrease(ccc_ordered_multimap *mm, ccc_omm_elem *, ccc_update_fn *,
                      void *);

/*===========================   Iterators   =================================*/

#define ccc_omm_equal_range_vr(ordered_multimap_ptr,                           \
                               begin_and_end_key_ptrs...)                      \
    &(ccc_range)                                                               \
    {                                                                          \
        ccc_omm_equal_range(ordered_multimap_ptr, begin_and_end_key_ptrs)      \
            .impl_                                                             \
    }

#define ccc_omm_equal_rrange_vr(ordered_multimap_ptr,                          \
                                rbegin_and_rend_key_ptrs...)                   \
    &(ccc_rrange)                                                              \
    {                                                                          \
        ccc_omm_equal_rrange(ordered_multimap_ptr, rbegin_and_rend_key_ptrs)   \
            .impl_                                                             \
    }

ccc_range ccc_omm_equal_range(ccc_ordered_multimap *mm, void const *begin_key,
                              void const *end_key);

ccc_rrange ccc_omm_equal_rrange(ccc_ordered_multimap *mm,
                                void const *rbegin_key, void const *end_key);

void *ccc_omm_begin(ccc_ordered_multimap const *mm);

void *ccc_omm_rbegin(ccc_ordered_multimap const *mm);

void *ccc_omm_next(ccc_ordered_multimap const *mm, ccc_omm_elem const *);

void *ccc_omm_rnext(ccc_ordered_multimap const *mm, ccc_omm_elem const *);

void *ccc_omm_end(ccc_ordered_multimap const *mm);

void *ccc_omm_rend(ccc_ordered_multimap const *mm);

void ccc_omm_clear(ccc_ordered_multimap *mm, ccc_destructor_fn *destructor);

/*===========================     Getters   =================================*/

bool ccc_omm_is_empty(ccc_ordered_multimap const *mm);

size_t ccc_omm_size(ccc_ordered_multimap const *mm);

ccc_result ccc_omm_print(ccc_ordered_multimap const *mm, ccc_print_fn *);

bool ccc_omm_validate(ccc_ordered_multimap const *mm);

#ifdef ORDERED_MULTIMAP_USING_NAMESPACE_CCC
typedef ccc_omm_elem omm_elem;
typedef ccc_ordered_multimap ordered_multimap;
typedef ccc_o_mm_entry o_mm_entry;
#    define omm_and_modify_w(args...) ccc_omm_and_modify_w(args)
#    define omm_or_insert_w(args...) ccc_omm_or_insert_w(args)
#    define omm_insert_entry_w(args...) ccc_omm_insert_entry_w(args)
#    define omm_try_insert_w(args...) ccc_omm_try_insert_w(args)
#    define omm_insert_or_assign_w(args...) ccc_omm_insert_or_assign_w(args)
#    define omm_init(args...) ccc_omm_init(args)
#    define omm_insert(args...) ccc_omm_insert(args)
#    define omm_try_insert(args...) ccc_omm_try_insert(args)
#    define omm_insert_or_assign(args...) ccc_omm_insert_or_assign(args)
#    define omm_remove(args...) ccc_omm_remove(args)
#    define omm_remove_entry(args...) ccc_omm_remove_entry(args)
#    define omm_entry_vr(args...) ccc_omm_entry_vr(args)
#    define omm_entry(args...) ccc_omm_entry(args)
#    define omm_and_modify(args...) ccc_omm_and_modify(args)
#    define omm_and_modify_aux(args...) ccc_omm_and_modify_aux(args)
#    define omm_or_insert(args...) ccc_omm_or_insert(args)
#    define omm_insert_entry(args...) ccc_omm_insert_entry(args)
#    define omm_unwrap(args...) ccc_omm_unwrap(args)
#    define omm_insert_error(args...) ccc_omm_insert_error(args)
#    define omm_occupied(args...) ccc_omm_occupied(args)
#    define omm_clear(args...) ccc_omm_clear(args)
#    define omm_is_empty(args...) ccc_omm_is_empty(args)
#    define omm_size(args...) ccc_omm_size(args)
#    define omm_pop_max(args...) ccc_omm_pop_max(args)
#    define omm_pop_min(args...) ccc_omm_pop_min(args)
#    define omm_max(args...) ccc_omm_max(args)
#    define omm_min(args...) ccc_omm_min(args)
#    define omm_is_max(args...) ccc_omm_is_max(args)
#    define omm_is_min(args...) ccc_omm_is_min(args)
#    define omm_erase(args...) ccc_omm_erase(args)
#    define omm_update(args...) ccc_omm_update(args)
#    define omm_increase(args...) ccc_omm_increase(args)
#    define omm_decrease(args...) ccc_omm_decrease(args)
#    define omm_contains(args...) ccc_omm_contains(args)
#    define omm_begin(args...) ccc_omm_begin(args)
#    define omm_rbegin(args...) ccc_omm_rbegin(args)
#    define omm_next(args...) ccc_omm_next(args)
#    define omm_rnext(args...) ccc_omm_rnext(args)
#    define omm_end(args...) ccc_omm_end(args)
#    define omm_rend(args...) ccc_omm_rend(args)
#    define omm_equal_range(args...) ccc_omm_equal_range(args)
#    define omm_equal_rrange(args...) ccc_omm_equal_rrange(args)
#    define omm_print(args...) ccc_omm_print(args)
#    define omm_validate(args...) ccc_omm_validate(args)
#endif /* ORDERED_MULTIMAP_USING_NAMESPACE_CCC */

#endif /* CCC_ORDERED_MULTIMAP_H */
