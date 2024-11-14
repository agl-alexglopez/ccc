#ifndef CCC_IMPL_FLAT_HASH_MAP_H
#define CCC_IMPL_FLAT_HASH_MAP_H

#include "buffer.h"
#include "impl_types.h"
#include "types.h"

/** @cond */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
/** @endcond */

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

#define ccc_impl_fhm_init(flat_hash_map_ptr, memory_ptr, capacity, key_field,  \
                          fhash_elem_field, alloc_fn, hash_fn, key_eq_fn, aux) \
    ({                                                                         \
        __auto_type fhm_mem_ptr_ = (memory_ptr);                               \
        (flat_hash_map_ptr)->buf_                                              \
            = (ccc_buffer)ccc_buf_init(fhm_mem_ptr_, alloc_fn, aux, capacity); \
        ccc_result res_ = ccc_impl_fhm_init_buf(                               \
            (flat_hash_map_ptr), offsetof(typeof(*(fhm_mem_ptr_)), key_field), \
            offsetof(typeof(*(fhm_mem_ptr_)), fhash_elem_field), (hash_fn),    \
            (key_eq_fn), (aux));                                               \
        res_;                                                                  \
    })

ccc_result ccc_impl_fhm_init_buf(struct ccc_fhmap_ *, size_t key_offset,
                                 size_t hash_elem_offset, ccc_hash_fn *,
                                 ccc_key_eq_fn *, void *aux);
struct ccc_ent_ ccc_impl_fhm_find(struct ccc_fhmap_ const *, void const *key,
                                  uint64_t hash);
void ccc_impl_fhm_insert(struct ccc_fhmap_ *h, void const *e, uint64_t hash,
                         size_t cur_i);

struct ccc_fhash_entry_ ccc_impl_fhm_entry(struct ccc_fhmap_ *h,
                                           void const *key);
struct ccc_fhash_entry_ *ccc_impl_fhm_and_modify(struct ccc_fhash_entry_ *e,
                                                 ccc_update_fn *fn);
struct ccc_fhmap_elem_ *ccc_impl_fhm_in_slot(struct ccc_fhmap_ const *h,
                                             void const *slot);
void *ccc_impl_fhm_key_in_slot(struct ccc_fhmap_ const *h, void const *slot);
uint64_t *ccc_impl_fhm_hash_at(struct ccc_fhmap_ const *h, size_t i);
size_t ccc_impl_fhm_distance(size_t capacity, size_t i, size_t j);
ccc_result ccc_impl_fhm_maybe_resize(struct ccc_fhmap_ *);
uint64_t ccc_impl_fhm_filter(struct ccc_fhmap_ const *, void const *key);
void *ccc_impl_fhm_base(struct ccc_fhmap_ const *h);
size_t ccc_impl_fhm_increment(size_t capacity, size_t i);

/* NOLINTBEGIN(readability-identifier-naming) */

/*==================   Helper Macros for Repeated Logic     =================*/

/* Internal helper assumes that swap_entry has already been evaluated once
   which it must have to make it to this point. */
#define ccc_impl_fhm_swaps(swap_entry, lazy_key_value...)                      \
    ({                                                                         \
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
    })

/*=====================     Core Macro Implementations     ==================*/

#define ccc_impl_fhm_and_modify_w(flat_hash_map_entry, mod_fn, aux...)         \
    ({                                                                         \
        struct ccc_fhash_entry_ fhm_mod_with_ent_                              \
            = (flat_hash_map_entry)->impl_;                                    \
        ccc_update_fn *const fhm_mod_fn_ = (mod_fn);                           \
        if (fhm_mod_with_ent_.entry_.stats_ == CCC_ENTRY_OCCUPIED              \
            && fhm_mod_fn_)                                                    \
        {                                                                      \
            __auto_type fhm_aux_ = aux;                                        \
            (fhm_mod_fn_)(                                                     \
                (ccc_user_type){fhm_mod_with_ent_.entry_.e_, &fhm_aux_});      \
        }                                                                      \
        fhm_mod_with_ent_;                                                     \
    })

#define ccc_impl_fhm_or_insert_w(flat_hash_map_entry_ptr, lazy_key_value...)   \
    ({                                                                         \
        __auto_type fhm_or_ins_ent_ptr_ = (flat_hash_map_entry_ptr);           \
        typeof(lazy_key_value) *fhm_or_ins_res_ = NULL;                        \
        if (fhm_or_ins_ent_ptr_)                                               \
        {                                                                      \
            struct ccc_fhash_entry_ *fhm_or_ins_entry_                         \
                = &fhm_or_ins_ent_ptr_->impl_;                                 \
            assert(sizeof(*fhm_or_ins_res_)                                    \
                   == ccc_buf_elem_size(&(fhm_or_ins_entry_->h_->buf_)));      \
            if (fhm_or_ins_entry_->entry_.stats_ & CCC_ENTRY_OCCUPIED)         \
            {                                                                  \
                fhm_or_ins_res_ = fhm_or_ins_entry_->entry_.e_;                \
            }                                                                  \
            else if (fhm_or_ins_entry_->entry_.stats_                          \
                     & CCC_ENTRY_INSERT_ERROR)                                 \
            {                                                                  \
                fhm_or_ins_res_ = NULL;                                        \
            }                                                                  \
            else                                                               \
            {                                                                  \
                fhm_or_ins_res_                                                \
                    = ccc_impl_fhm_swaps(fhm_or_ins_entry_, lazy_key_value);   \
            }                                                                  \
        }                                                                      \
        fhm_or_ins_res_;                                                       \
    })

#define ccc_impl_fhm_insert_entry_w(flat_hash_map_entry_ptr,                   \
                                    lazy_key_value...)                         \
    ({                                                                         \
        __auto_type fhm_ins_ent_ptr_ = (flat_hash_map_entry_ptr);              \
        typeof(lazy_key_value) *fhm_res_ = NULL;                               \
        if (fhm_ins_ent_ptr_)                                                  \
        {                                                                      \
            struct ccc_fhash_entry_ *fhm_ins_ent_ = &fhm_ins_ent_ptr_->impl_;  \
            assert(sizeof(*fhm_res_)                                           \
                   == ccc_buf_elem_size(&(fhm_ins_ent_->h_->buf_)));           \
            if (fhm_ins_ent_->entry_.stats_ & CCC_ENTRY_OCCUPIED)              \
            {                                                                  \
                fhm_ins_ent_->entry_.stats_ = CCC_ENTRY_OCCUPIED;              \
                *((typeof(fhm_res_))fhm_ins_ent_->entry_.e_) = lazy_key_value; \
                ccc_impl_fhm_in_slot(fhm_ins_ent_->h_,                         \
                                     fhm_ins_ent_->entry_.e_)                  \
                    ->hash_                                                    \
                    = fhm_ins_ent_->hash_;                                     \
                fhm_res_ = fhm_ins_ent_->entry_.e_;                            \
            }                                                                  \
            else if (fhm_ins_ent_->entry_.stats_ & CCC_ENTRY_INSERT_ERROR)     \
            {                                                                  \
                fhm_res_ = NULL;                                               \
            }                                                                  \
            else                                                               \
            {                                                                  \
                fhm_res_ = ccc_impl_fhm_swaps(fhm_ins_ent_, lazy_key_value);   \
            }                                                                  \
        }                                                                      \
        fhm_res_;                                                              \
    })

#define ccc_impl_fhm_try_insert_w(flat_hash_map_ptr, key, lazy_value...)       \
    ({                                                                         \
        struct ccc_fhmap_ *flat_hash_map_ptr_ = (flat_hash_map_ptr);           \
        struct ccc_ent_ fhm_try_insert_res_                                    \
            = {.stats_ = CCC_ENTRY_INPUT_ERROR};                               \
        if (flat_hash_map_ptr_)                                                \
        {                                                                      \
            __auto_type fhm_key_ = key;                                        \
            struct ccc_fhash_entry_ fhm_try_ins_ent_                           \
                = ccc_impl_fhm_entry(flat_hash_map_ptr_, (void *)&fhm_key_);   \
            if ((fhm_try_ins_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)          \
                || (fhm_try_ins_ent_.entry_.stats_ & CCC_ENTRY_INSERT_ERROR))  \
            {                                                                  \
                fhm_try_insert_res_ = fhm_try_ins_ent_.entry_;                 \
            }                                                                  \
            else                                                               \
            {                                                                  \
                fhm_try_insert_res_ = (struct ccc_ent_){                       \
                    ccc_impl_fhm_swaps((&fhm_try_ins_ent_), lazy_value),       \
                    CCC_ENTRY_VACANT,                                          \
                };                                                             \
                *((typeof(fhm_key_) *)ccc_impl_fhm_key_in_slot(                \
                    flat_hash_map_ptr_, fhm_try_insert_res_.e_))               \
                    = fhm_key_;                                                \
            }                                                                  \
        }                                                                      \
        fhm_try_insert_res_;                                                   \
    })

#define ccc_impl_fhm_insert_or_assign_w(flat_hash_map_ptr, key, lazy_value...) \
    ({                                                                         \
        struct ccc_fhmap_ *flat_hash_map_ptr_ = (flat_hash_map_ptr);           \
        struct ccc_ent_ fhm_ins_or_assign_res_                                 \
            = {.stats_ = CCC_ENTRY_INPUT_ERROR};                               \
        if (flat_hash_map_ptr_)                                                \
        {                                                                      \
            __auto_type fhm_key_ = key;                                        \
            struct ccc_fhash_entry_ fhm_ins_or_assign_ent_                     \
                = ccc_impl_fhm_entry(flat_hash_map_ptr_, (void *)&fhm_key_);   \
            if (fhm_ins_or_assign_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)     \
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
            else if (fhm_ins_or_assign_ent_.entry_.stats_                      \
                     & CCC_ENTRY_INSERT_ERROR)                                 \
            {                                                                  \
                fhm_ins_or_assign_res_.e_ = NULL;                              \
                fhm_ins_or_assign_res_.stats_ = CCC_ENTRY_INSERT_ERROR;        \
            }                                                                  \
            else                                                               \
            {                                                                  \
                fhm_ins_or_assign_res_ = (struct ccc_ent_){                    \
                    ccc_impl_fhm_swaps((&fhm_ins_or_assign_ent_), lazy_value), \
                    CCC_ENTRY_VACANT,                                          \
                };                                                             \
                *((typeof(fhm_key_) *)ccc_impl_fhm_key_in_slot(                \
                    flat_hash_map_ptr_, fhm_ins_or_assign_res_.e_))            \
                    = fhm_key_;                                                \
            }                                                                  \
        }                                                                      \
        fhm_ins_or_assign_res_;                                                \
    })

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_HASH_MAP_H */