#ifndef CCC_IMPL_FLAT_HASH_MAP_H
#define CCC_IMPL_FLAT_HASH_MAP_H

/** @cond */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/** @endcond */

#include "../buffer.h"
#include "../types.h"
#include "impl_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private */
enum : uint64_t
{
    CCC_FHM_EMPTY = 0,
};

/** @private */
struct ccc_fhmap_elem_
{
    uint64_t hash_;
};

/** @private */
struct ccc_fhmap_
{
    ccc_buffer buf_;
    ccc_hash_fn *hash_fn_;
    ccc_key_eq_fn *eq_fn_;
    size_t key_offset_;
    size_t hash_elem_offset_;
};

/** @private */
struct ccc_fhash_entry_
{
    struct ccc_fhmap_ *h_;
    uint64_t hash_;
    struct ccc_ent_ entry_;
};

/** @private */
union ccc_fhmap_entry_
{
    struct ccc_fhash_entry_ impl_;
};

#define ccc_impl_fhm_init(memory_ptr, fhash_elem_field, key_field, hash_fn,    \
                          key_eq_fn, alloc_fn, aux, capacity)                  \
    {                                                                          \
        .buf_                                                                  \
        = (ccc_buffer)ccc_buf_init((memory_ptr), (alloc_fn), (aux), capacity), \
        .hash_fn_ = (hash_fn),                                                 \
        .eq_fn_ = (key_eq_fn),                                                 \
        .key_offset_ = offsetof(typeof(*(memory_ptr)), key_field),             \
        .hash_elem_offset_                                                     \
        = offsetof(typeof(*(memory_ptr)), fhash_elem_field),                   \
    }

/*===============   Wrappers for Macros to Access Internals     =============*/

/** @private */
void ccc_impl_fhm_insert(struct ccc_fhmap_ *h, void const *e, uint64_t hash,
                         size_t cur_i);
/** @private */
struct ccc_fhash_entry_ ccc_impl_fhm_entry(struct ccc_fhmap_ *h,
                                           void const *key);
/** @private */
struct ccc_fhmap_elem_ *ccc_impl_fhm_in_slot(struct ccc_fhmap_ const *h,
                                             void const *slot);
/** @private */
void *ccc_impl_fhm_key_in_slot(struct ccc_fhmap_ const *h, void const *slot);
/** @private */
uint64_t *ccc_impl_fhm_hash_at(struct ccc_fhmap_ const *h, size_t i);
/** @private */
size_t ccc_impl_fhm_increment(size_t capacity, size_t i);

/*==================   Helper Macros for Repeated Logic     =================*/

/* Internal helper assumes that swap_entry has already been evaluated once
   which it must have to make it to this point. */
#define ccc_impl_fhm_swaps(swap_entry, lazy_key_value...)                      \
    (__extension__({                                                           \
        size_t fhm_i_                                                          \
            = ccc_buf_i(&((swap_entry)->h_->buf_), (swap_entry)->entry_.e_);   \
        if (*ccc_impl_fhm_hash_at((swap_entry)->h_, fhm_i_) == CCC_FHM_EMPTY)  \
        {                                                                      \
            *((typeof(lazy_key_value) *)ccc_buf_at(&((swap_entry)->h_->buf_),  \
                                                   fhm_i_))                    \
                = lazy_key_value;                                              \
            *ccc_impl_fhm_hash_at((swap_entry)->h_, fhm_i_)                    \
                = (swap_entry)->hash_;                                         \
            (void)ccc_buf_size_plus(&(swap_entry)->h_->buf_, 1);               \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            typeof(lazy_key_value) fhm_cur_slot_                               \
                = *((typeof(fhm_cur_slot_) *)ccc_buf_at(                       \
                    &((swap_entry)->h_->buf_), fhm_i_));                       \
            *((typeof(fhm_cur_slot_) *)ccc_buf_at(&((swap_entry)->h_->buf_),   \
                                                  fhm_i_))                     \
                = lazy_key_value;                                              \
            *ccc_impl_fhm_hash_at((swap_entry)->h_, fhm_i_)                    \
                = (swap_entry)->hash_;                                         \
            fhm_i_ = ccc_impl_fhm_increment(                                   \
                ccc_buf_capacity(&(swap_entry)->h_->buf_), fhm_i_);            \
            ccc_impl_fhm_insert(                                               \
                (swap_entry)->h_, &fhm_cur_slot_,                              \
                ccc_impl_fhm_in_slot((swap_entry)->h_, &fhm_cur_slot_)->hash_, \
                fhm_i_);                                                       \
        }                                                                      \
        (swap_entry)->entry_.e_;                                               \
    }))

/*=====================     Core Macro Implementations     ==================*/

#define ccc_impl_fhm_and_modify_w(flat_hash_map_entry_ptr, type_name,          \
                                  closure_over_T...)                           \
    (__extension__({                                                           \
        __auto_type fhm_mod_ent_ptr_ = (flat_hash_map_entry_ptr);              \
        struct ccc_fhash_entry_ fhm_mod_with_ent_                              \
            = {.entry_ = {.stats_ = CCC_INPUT_ERROR}};                         \
        if (fhm_mod_ent_ptr_)                                                  \
        {                                                                      \
            fhm_mod_with_ent_ = fhm_mod_ent_ptr_->impl_;                       \
            if (fhm_mod_with_ent_.entry_.stats_ == CCC_OCCUPIED)               \
            {                                                                  \
                type_name *const T = fhm_mod_with_ent_.entry_.e_;              \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        fhm_mod_with_ent_;                                                     \
    }))

#define ccc_impl_fhm_or_insert_w(flat_hash_map_entry_ptr, lazy_key_value...)   \
    (__extension__({                                                           \
        __auto_type fhm_or_ins_ent_ptr_ = (flat_hash_map_entry_ptr);           \
        typeof(lazy_key_value) *fhm_or_ins_res_ = NULL;                        \
        if (fhm_or_ins_ent_ptr_)                                               \
        {                                                                      \
            struct ccc_fhash_entry_ *fhm_or_ins_entry_                         \
                = &fhm_or_ins_ent_ptr_->impl_;                                 \
            if (!(fhm_or_ins_entry_->entry_.stats_ & CCC_INSERT_ERROR))        \
            {                                                                  \
                if (fhm_or_ins_entry_->entry_.stats_ & CCC_OCCUPIED)           \
                {                                                              \
                    fhm_or_ins_res_ = fhm_or_ins_entry_->entry_.e_;            \
                }                                                              \
                else                                                           \
                {                                                              \
                    fhm_or_ins_res_ = ccc_impl_fhm_swaps(fhm_or_ins_entry_,    \
                                                         lazy_key_value);      \
                }                                                              \
            }                                                                  \
        }                                                                      \
        fhm_or_ins_res_;                                                       \
    }))

#define ccc_impl_fhm_insert_entry_w(flat_hash_map_entry_ptr,                   \
                                    lazy_key_value...)                         \
    (__extension__({                                                           \
        __auto_type fhm_ins_ent_ptr_ = (flat_hash_map_entry_ptr);              \
        typeof(lazy_key_value) *fhm_res_ = NULL;                               \
        if (fhm_ins_ent_ptr_)                                                  \
        {                                                                      \
            struct ccc_fhash_entry_ *fhm_ins_ent_ = &fhm_ins_ent_ptr_->impl_;  \
            if (!(fhm_ins_ent_->entry_.stats_ & CCC_INSERT_ERROR))             \
            {                                                                  \
                if (fhm_ins_ent_->entry_.stats_ & CCC_OCCUPIED)                \
                {                                                              \
                    fhm_ins_ent_->entry_.stats_ = CCC_OCCUPIED;                \
                    *((typeof(fhm_res_))fhm_ins_ent_->entry_.e_)               \
                        = lazy_key_value;                                      \
                    ccc_impl_fhm_in_slot(fhm_ins_ent_->h_,                     \
                                         fhm_ins_ent_->entry_.e_)              \
                        ->hash_                                                \
                        = fhm_ins_ent_->hash_;                                 \
                    fhm_res_ = fhm_ins_ent_->entry_.e_;                        \
                }                                                              \
                else                                                           \
                {                                                              \
                    fhm_res_                                                   \
                        = ccc_impl_fhm_swaps(fhm_ins_ent_, lazy_key_value);    \
                }                                                              \
            }                                                                  \
        }                                                                      \
        fhm_res_;                                                              \
    }))

#define ccc_impl_fhm_try_insert_w(flat_hash_map_ptr, key, lazy_value...)       \
    (__extension__({                                                           \
        struct ccc_fhmap_ *flat_hash_map_ptr_ = (flat_hash_map_ptr);           \
        struct ccc_ent_ fhm_try_insert_res_ = {.stats_ = CCC_INPUT_ERROR};     \
        if (flat_hash_map_ptr_)                                                \
        {                                                                      \
            __auto_type fhm_key_ = key;                                        \
            struct ccc_fhash_entry_ fhm_try_ins_ent_                           \
                = ccc_impl_fhm_entry(flat_hash_map_ptr_, (void *)&fhm_key_);   \
            if ((fhm_try_ins_ent_.entry_.stats_ & CCC_OCCUPIED)                \
                || (fhm_try_ins_ent_.entry_.stats_ & CCC_INSERT_ERROR))        \
            {                                                                  \
                fhm_try_insert_res_ = fhm_try_ins_ent_.entry_;                 \
            }                                                                  \
            else                                                               \
            {                                                                  \
                fhm_try_insert_res_ = (struct ccc_ent_){                       \
                    ccc_impl_fhm_swaps((&fhm_try_ins_ent_), lazy_value),       \
                    CCC_VACANT,                                                \
                };                                                             \
                *((typeof(fhm_key_) *)ccc_impl_fhm_key_in_slot(                \
                    flat_hash_map_ptr_, fhm_try_insert_res_.e_))               \
                    = fhm_key_;                                                \
            }                                                                  \
        }                                                                      \
        fhm_try_insert_res_;                                                   \
    }))

#define ccc_impl_fhm_insert_or_assign_w(flat_hash_map_ptr, key, lazy_value...) \
    (__extension__({                                                           \
        struct ccc_fhmap_ *flat_hash_map_ptr_ = (flat_hash_map_ptr);           \
        struct ccc_ent_ fhm_ins_or_assign_res_ = {.stats_ = CCC_INPUT_ERROR};  \
        if (flat_hash_map_ptr_)                                                \
        {                                                                      \
            __auto_type fhm_key_ = key;                                        \
            struct ccc_fhash_entry_ fhm_ins_or_assign_ent_                     \
                = ccc_impl_fhm_entry(flat_hash_map_ptr_, (void *)&fhm_key_);   \
            if (fhm_ins_or_assign_ent_.entry_.stats_ & CCC_OCCUPIED)           \
            {                                                                  \
                fhm_ins_or_assign_res_ = fhm_ins_or_assign_ent_.entry_;        \
                *((typeof(lazy_value) *)fhm_ins_or_assign_res_.e_)             \
                    = lazy_value;                                              \
                *((typeof(fhm_key_) *)ccc_impl_fhm_key_in_slot(                \
                    flat_hash_map_ptr_, fhm_ins_or_assign_res_.e_))            \
                    = fhm_key_;                                                \
                ccc_impl_fhm_in_slot(fhm_ins_or_assign_ent_.h_,                \
                                     fhm_ins_or_assign_ent_.entry_.e_)         \
                    ->hash_                                                    \
                    = fhm_ins_or_assign_ent_.hash_;                            \
            }                                                                  \
            else if (fhm_ins_or_assign_ent_.entry_.stats_ & CCC_INSERT_ERROR)  \
            {                                                                  \
                fhm_ins_or_assign_res_.e_ = NULL;                              \
                fhm_ins_or_assign_res_.stats_ = CCC_INSERT_ERROR;              \
            }                                                                  \
            else                                                               \
            {                                                                  \
                fhm_ins_or_assign_res_ = (struct ccc_ent_){                    \
                    ccc_impl_fhm_swaps((&fhm_ins_or_assign_ent_), lazy_value), \
                    CCC_VACANT,                                                \
                };                                                             \
                *((typeof(fhm_key_) *)ccc_impl_fhm_key_in_slot(                \
                    flat_hash_map_ptr_, fhm_ins_or_assign_res_.e_))            \
                    = fhm_key_;                                                \
            }                                                                  \
        }                                                                      \
        fhm_ins_or_assign_res_;                                                \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_HASH_MAP_H */
