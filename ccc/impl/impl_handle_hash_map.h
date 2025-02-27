#ifndef CCC_IMPL_HANDLE_HASH_MAP_H
#define CCC_IMPL_HANDLE_HASH_MAP_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../buffer.h"
#include "../types.h"
#include "impl_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private */
enum : uint64_t
{
    CCC_HHM_EMPTY = 0,
};

/** @private With only a single additional field compared to the standard flat
hash map we offer handle stability; handle stability is like pointer stability
but for an index in a table rather than a pointer in a heap. User data will not
move from its index regardless of other insertions, deletions, or resizing of
the table. Cache the full hash for efficient resizing, Robin Hood distance
calculations, and less reliance on user callbacks. Point back to the home slot
where the user data is. Because this is an intrusive element we will run Robin
Hood on just these intrusive metadata entries in the table and leave user data
where it is. These structs are swapped or backshifted while user data remains
untouched. */
struct ccc_hhmap_elem_
{
    /** Hash cached for distance, resizing, and callback avoidance. */
    uint64_t hash_;
    /** User data remains here until deleted. Callback compare needs this. */
    size_t slot_i_;
};

/** @private */
struct ccc_hhmap_
{
    ccc_buffer buf_;          /** Buffer of types with size, cap, and aux. */
    ccc_hash_fn *hash_fn_;    /** Hashing callback. */
    ccc_key_eq_fn *eq_fn_;    /** Equality callback. */
    size_t key_offset_;       /** Key in user defined type offset. */
    size_t hash_elem_offset_; /** Intrusive element offset. */
};

/** @private */
struct ccc_hhash_handle_
{
    struct ccc_hhmap_ *h_;     /** Map so no second fn arg needed. */
    uint64_t hash_;            /** Specific hash for this element. */
    struct ccc_handl_ handle_; /** Index and a status. */
};

/** @private */
union ccc_hhmap_handle_
{
    struct ccc_hhash_handle_ impl_;
};

/** @private */
union ccc_hhmap_ref_
{
    size_t impl_;
};

/** @private */
#define ccc_impl_hhm_init(memory_ptr, hhash_elem_field, key_field, hash_fn,    \
                          key_eq_fn, alloc_fn, aux, capacity)                  \
    {                                                                          \
        .buf_                                                                  \
        = (ccc_buffer)ccc_buf_init((memory_ptr), (alloc_fn), (aux), capacity), \
        .hash_fn_ = (hash_fn),                                                 \
        .eq_fn_ = (key_eq_fn),                                                 \
        .key_offset_ = offsetof(typeof(*(memory_ptr)), key_field),             \
        .hash_elem_offset_                                                     \
        = offsetof(typeof(*(memory_ptr)), hhash_elem_field),                   \
    }

/** @private */
#define ccc_impl_hhm_as(handle_hash_map_ptr, type_name, handle...)             \
    ((type_name *)ccc_buf_at(&(handle_hash_map_ptr)->buf_, (handle)))

struct ccc_handl_ ccc_impl_hhm_find(struct ccc_hhmap_ const *, void const *key,
                                    uint64_t hash);
ccc_handle_i ccc_impl_hhm_insert_meta(struct ccc_hhmap_ *h, uint64_t hash,
                                      size_t cur_i);

struct ccc_hhash_handle_ ccc_impl_hhm_handle(struct ccc_hhmap_ *h,
                                             void const *key);
struct ccc_hhash_handle_ *ccc_impl_hhm_and_modify(struct ccc_hhash_handle_ *e,
                                                  ccc_update_fn *fn);
struct ccc_hhmap_elem_ *ccc_impl_hhm_in_slot(struct ccc_hhmap_ const *h,
                                             void const *slot);
void *ccc_impl_hhm_key_at(struct ccc_hhmap_ const *h, size_t i);
uint64_t *ccc_impl_hhm_hash_at(struct ccc_hhmap_ const *h, size_t i);
size_t ccc_impl_hhm_distance(size_t capacity, size_t i, size_t j);
ccc_result ccc_impl_hhm_maybe_resize(struct ccc_hhmap_ *);
uint64_t ccc_impl_hhm_filter(struct ccc_hhmap_ const *, void const *key);
void *ccc_impl_hhm_base(struct ccc_hhmap_ const *h);
size_t ccc_impl_hhm_increment(size_t capacity, size_t i);
void ccc_impl_hhm_copy_to_slot(struct ccc_hhmap_ *h, void *slot_dst,
                               void const *slot_src);
struct ccc_hhmap_elem_ *ccc_impl_hhm_elem_at(struct ccc_hhmap_ const *h,
                                             size_t i);

/*==================   Helper Macros for Repeated Logic     =================*/

/** @private Internal helper assumes that handle has already been evaluated
once which it must have to make it to this point. */
#define ccc_impl_hhm_swaps(swap_handle, lazy_key_value...)                     \
    (__extension__({                                                           \
        size_t hhm_i_ = (swap_handle)->handle_.i_;                             \
        struct ccc_hhmap_elem_ *hhm_elem_                                      \
            = ccc_impl_hhm_elem_at((swap_handle)->h_, hhm_i_);                 \
        if (hhm_elem_->hash_ == CCC_HHM_EMPTY)                                 \
        {                                                                      \
            struct ccc_hhmap_elem_ const save_slot_ = *ccc_impl_hhm_elem_at(   \
                (swap_handle)->h_, hhm_elem_->slot_i_);                        \
            struct ccc_hhmap_elem_ const save_elem_ = *hhm_elem_;              \
            *((typeof(lazy_key_value) *)ccc_buf_at(&((swap_handle)->h_->buf_), \
                                                   save_elem_.slot_i_))        \
                = lazy_key_value;                                              \
            *ccc_impl_hhm_elem_at((swap_handle)->h_, save_elem_.slot_i_)       \
                = save_slot_;                                                  \
            *ccc_impl_hhm_hash_at((swap_handle)->h_, hhm_i_)                   \
                = (swap_handle)->hash_;                                        \
            (void)ccc_buf_size_plus(&(swap_handle)->h_->buf_, 1);              \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            hhm_i_ = ccc_impl_hhm_insert_meta((swap_handle)->h_,               \
                                              (swap_handle)->hash_, hhm_i_);   \
            struct ccc_hhmap_elem_ const save_slot_ = *ccc_impl_hhm_elem_at(   \
                (swap_handle)->h_,                                             \
                ccc_impl_hhm_elem_at((swap_handle)->h_, hhm_i_)->slot_i_);     \
            struct ccc_hhmap_elem_ const save_elem_                            \
                = *ccc_impl_hhm_elem_at((swap_handle)->h_, hhm_i_);            \
            *((typeof(lazy_key_value) *)ccc_buf_at(&((swap_handle)->h_->buf_), \
                                                   save_elem_.slot_i_))        \
                = lazy_key_value;                                              \
            *ccc_impl_hhm_elem_at((swap_handle)->h_, save_elem_.slot_i_)       \
                = save_slot_;                                                  \
        }                                                                      \
        hhm_i_;                                                                \
    }))

/*=====================     Core Macro Implementations     ==================*/

/** @private */
#define ccc_impl_hhm_and_modify_w(handle_hash_map_handle_ptr, type_name,       \
                                  closure_over_T...)                           \
    (__extension__({                                                           \
        __auto_type hhm_mod_handl_ptr_ = (handle_hash_map_handle_ptr);         \
        struct ccc_hhash_handle_ hhm_mod_with_handl_                           \
            = {.handle_ = {.stats_ = CCC_INPUT_ERROR}};                        \
        if (hhm_mod_handl_ptr_)                                                \
        {                                                                      \
            hhm_mod_with_handl_ = hhm_mod_handl_ptr_->impl_;                   \
            if (hhm_mod_with_handl_.handle_.stats_ == CCC_OCCUPIED)            \
            {                                                                  \
                type_name *const T = ccc_buf_at(                               \
                    &hhm_mod_with_handl_.h_->buf_,                             \
                    ccc_impl_hhm_elem_at(hhm_mod_with_handl_.h_,               \
                                         hhm_mod_with_handl_.handle_.i_)       \
                        ->slot_i_);                                            \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        hhm_mod_with_handl_;                                                   \
    }))

/** @private */
#define ccc_impl_hhm_or_insert_w(handle_hash_map_handle_ptr,                   \
                                 lazy_key_value...)                            \
    (__extension__({                                                           \
        __auto_type hhm_or_ins_handl_ptr_ = (handle_hash_map_handle_ptr);      \
        ccc_handle_i hhm_or_ins_res_ = 0;                                      \
        if (hhm_or_ins_handl_ptr_)                                             \
        {                                                                      \
            struct ccc_hhash_handle_ *hhm_or_ins_handle_                       \
                = &hhm_or_ins_handl_ptr_->impl_;                               \
            if (!(hhm_or_ins_handle_->handle_.stats_ & CCC_INSERT_ERROR))      \
            {                                                                  \
                if (hhm_or_ins_handle_->handle_.stats_ & CCC_OCCUPIED)         \
                {                                                              \
                    hhm_or_ins_res_ = hhm_or_ins_handle_->handle_.i_;          \
                }                                                              \
                else                                                           \
                {                                                              \
                    hhm_or_ins_res_ = ccc_impl_hhm_swaps(hhm_or_ins_handle_,   \
                                                         lazy_key_value);      \
                }                                                              \
                hhm_or_ins_res_                                                \
                    = ccc_impl_hhm_elem_at(hhm_or_ins_handl_ptr_->impl_.h_,    \
                                           hhm_or_ins_res_)                    \
                          ->slot_i_;                                           \
            }                                                                  \
        }                                                                      \
        hhm_or_ins_res_;                                                       \
    }))

/** @private */
#define ccc_impl_hhm_insert_handle_w(handle_hash_map_handle_ptr,               \
                                     lazy_key_value...)                        \
    (__extension__({                                                           \
        __auto_type hhm_ins_handl_ptr_ = (handle_hash_map_handle_ptr);         \
        ccc_handle_i hhm_res_ = 0;                                             \
        if (hhm_ins_handl_ptr_)                                                \
        {                                                                      \
            struct ccc_hhash_handle_ *hhm_ins_handl_                           \
                = &hhm_ins_handl_ptr_->impl_;                                  \
            if (!(hhm_ins_handl_->handle_.stats_ & CCC_INSERT_ERROR))          \
            {                                                                  \
                if (hhm_ins_handl_->handle_.stats_ & CCC_OCCUPIED)             \
                {                                                              \
                    hhm_ins_handl_->handle_.stats_ = CCC_OCCUPIED;             \
                    struct ccc_hhmap_elem_ save_slot_ = *ccc_impl_hhm_elem_at( \
                        hhm_ins_handl_->h_,                                    \
                        ccc_impl_hhm_elem_at(hhm_ins_handl_->h_,               \
                                             hhm_ins_handl_->handle_.i_)       \
                            ->slot_i_);                                        \
                    struct ccc_hhmap_elem_ save_elem_ = *ccc_impl_hhm_elem_at( \
                        hhm_ins_handl_->h_, hhm_ins_handl_->handle_.i_);       \
                    *((typeof(lazy_key_value) *)ccc_buf_at(                    \
                        &hhm_ins_handl_->h_->buf_, save_elem_.slot_i_))        \
                        = lazy_key_value;                                      \
                    *ccc_impl_hhm_elem_at(hhm_ins_handl_->h_,                  \
                                          save_elem_.slot_i_)                  \
                        = save_slot_;                                          \
                    hhm_res_ = hhm_ins_handl_->handle_.i_;                     \
                }                                                              \
                else                                                           \
                {                                                              \
                    hhm_res_                                                   \
                        = ccc_impl_hhm_swaps(hhm_ins_handl_, lazy_key_value);  \
                }                                                              \
                hhm_res_ = ccc_impl_hhm_elem_at(hhm_ins_handl_ptr_->impl_.h_,  \
                                                hhm_res_)                      \
                               ->slot_i_;                                      \
            }                                                                  \
        }                                                                      \
        hhm_res_;                                                              \
    }))

/** @private */
#define ccc_impl_hhm_try_insert_w(handle_hash_map_ptr, key, lazy_value...)     \
    (__extension__({                                                           \
        struct ccc_hhmap_ *handle_hash_map_ptr_ = (handle_hash_map_ptr);       \
        struct ccc_handl_ hhm_try_insert_res_ = {.stats_ = CCC_INPUT_ERROR};   \
        if (handle_hash_map_ptr_)                                              \
        {                                                                      \
            __auto_type hhm_key_ = key;                                        \
            struct ccc_hhash_handle_ hhm_try_ins_handl_ = ccc_impl_hhm_handle( \
                handle_hash_map_ptr_, (void *)&hhm_key_);                      \
            if ((hhm_try_ins_handl_.handle_.stats_ & CCC_OCCUPIED)             \
                || (hhm_try_ins_handl_.handle_.stats_ & CCC_INSERT_ERROR))     \
            {                                                                  \
                hhm_try_insert_res_ = hhm_try_ins_handl_.handle_;              \
            }                                                                  \
            else                                                               \
            {                                                                  \
                hhm_try_insert_res_ = (struct ccc_handl_){                     \
                    .i_                                                        \
                    = ccc_impl_hhm_swaps((&hhm_try_ins_handl_), lazy_value),   \
                    .stats_ = CCC_VACANT,                                      \
                };                                                             \
                *((typeof(hhm_key_) *)ccc_impl_hhm_key_at(                     \
                    handle_hash_map_ptr_,                                      \
                    ccc_impl_hhm_elem_at(handle_hash_map_ptr_,                 \
                                         hhm_try_insert_res_.i_)               \
                        ->slot_i_))                                            \
                    = hhm_key_;                                                \
            }                                                                  \
            hhm_try_insert_res_.i_                                             \
                = ccc_impl_hhm_elem_at(handle_hash_map_ptr_,                   \
                                       hhm_try_insert_res_.i_)                 \
                      ->slot_i_;                                               \
        }                                                                      \
        hhm_try_insert_res_;                                                   \
    }))

/** @private */
#define ccc_impl_hhm_insert_or_assign_w(handle_hash_map_ptr, key,              \
                                        lazy_value...)                         \
    (__extension__({                                                           \
        struct ccc_hhmap_ *handle_hash_map_ptr_ = (handle_hash_map_ptr);       \
        struct ccc_handl_ hhm_ins_or_assign_res_                               \
            = {.stats_ = CCC_INPUT_ERROR};                                     \
        if (handle_hash_map_ptr_)                                              \
        {                                                                      \
            __auto_type hhm_key_ = key;                                        \
            struct ccc_hhash_handle_ hhm_ins_or_assign_handl_                  \
                = ccc_impl_hhm_handle(handle_hash_map_ptr_,                    \
                                      (void *)&hhm_key_);                      \
            if (hhm_ins_or_assign_handl_.handle_.stats_ & CCC_OCCUPIED)        \
            {                                                                  \
                hhm_ins_or_assign_res_ = hhm_ins_or_assign_handl_.handle_;     \
                struct ccc_hhmap_elem_ const save_elem_                        \
                    = *ccc_impl_hhm_elem_at(                                   \
                        handle_hash_map_ptr_,                                  \
                        hhm_ins_or_assign_handl_.handle_.i_);                  \
                struct ccc_hhmap_elem_ const save_slot_                        \
                    = *ccc_impl_hhm_elem_at(handle_hash_map_ptr_,              \
                                            save_elem_.slot_i_);               \
                *((typeof(lazy_value) *)ccc_buf_at(                            \
                    &handle_hash_map_ptr_->buf_, save_elem_.slot_i_))          \
                    = lazy_value;                                              \
                *ccc_impl_hhm_elem_at(handle_hash_map_ptr_,                    \
                                      save_elem_.slot_i_)                      \
                    = save_slot_;                                              \
                *((typeof(hhm_key_) *)ccc_impl_hhm_key_at(                     \
                    handle_hash_map_ptr_, save_elem_.slot_i_))                 \
                    = hhm_key_;                                                \
            }                                                                  \
            else if (hhm_ins_or_assign_handl_.handle_.stats_                   \
                     & CCC_INSERT_ERROR)                                       \
            {                                                                  \
                hhm_ins_or_assign_res_.i_ = 0;                                 \
                hhm_ins_or_assign_res_.stats_ = CCC_INSERT_ERROR;              \
            }                                                                  \
            else                                                               \
            {                                                                  \
                hhm_ins_or_assign_res_ = (struct ccc_handl_){                  \
                    .i_ = ccc_impl_hhm_swaps((&hhm_ins_or_assign_handl_),      \
                                             lazy_value),                      \
                    .stats_ = CCC_VACANT,                                      \
                };                                                             \
                *((typeof(hhm_key_) *)ccc_impl_hhm_key_at(                     \
                    handle_hash_map_ptr_,                                      \
                    ccc_impl_hhm_elem_at(handle_hash_map_ptr_,                 \
                                         hhm_ins_or_assign_handl_.handle_.i_)  \
                        ->slot_i_))                                            \
                    = hhm_key_;                                                \
            }                                                                  \
            hhm_ins_or_assign_res_.i_                                          \
                = ccc_impl_hhm_elem_at(handle_hash_map_ptr_,                   \
                                       hhm_ins_or_assign_res_.i_)              \
                      ->slot_i_;                                               \
        }                                                                      \
        hhm_ins_or_assign_res_;                                                \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_HANDLE_HASH_MAP_H */
