#ifndef CCC_IMPL_FLAT_HASH_MAP_H
#define CCC_IMPL_FLAT_HASH_MAP_H

#include "buffer.h"
#include "impl_types.h"
#include "types.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

enum : uint64_t
{
    CCC_FHM_EMPTY = 0,
};

struct ccc_fhm_elem_
{
    uint64_t hash_;
};

struct ccc_fhm_
{
    ccc_buffer buf_;
    ccc_hash_fn *hash_fn_;
    ccc_key_eq_fn *eq_fn_;
    void *aux_;
    size_t key_offset_;
    size_t hash_elem_offset_;
};

struct ccc_fhm_entry_
{
    struct ccc_fhm_ *h_;
    uint64_t hash_;
    struct ccc_entry_ entry_;
};

#define ccc_impl_fhm_init(flat_hash_map_ptr, memory_ptr, capacity,             \
                          struct_name, key_field, fhash_elem_field, alloc_fn,  \
                          hash_fn, key_eq_fn, aux)                             \
    ({                                                                         \
        (flat_hash_map_ptr)->buf_ = (ccc_buffer)ccc_buf_init(                  \
            (memory_ptr), struct_name, (capacity), (alloc_fn));                \
        ccc_result res_ = ccc_impl_fhm_init_buf(                               \
            (flat_hash_map_ptr), offsetof(struct_name, key_field),             \
            offsetof(struct_name, fhash_elem_field), (hash_fn), (key_eq_fn),   \
            (aux));                                                            \
        res_;                                                                  \
    })

ccc_result ccc_impl_fhm_init_buf(struct ccc_fhm_ *, size_t key_offset,
                                 size_t hash_elem_offset, ccc_hash_fn *,
                                 ccc_key_eq_fn *, void *aux);
struct ccc_entry_ ccc_impl_fhm_find(struct ccc_fhm_ const *, void const *key,
                                    uint64_t hash);
void ccc_impl_fhm_insert(struct ccc_fhm_ *h, void const *e, uint64_t hash,
                         size_t cur_i);

struct ccc_fhm_entry_ ccc_impl_fhm_entry(struct ccc_fhm_ *h, void const *key);
struct ccc_fhm_entry_ *ccc_impl_fhm_and_modify(struct ccc_fhm_entry_ *e,
                                               ccc_update_fn *fn);
struct ccc_fhm_elem_ *ccc_impl_fhm_in_slot(struct ccc_fhm_ const *h,
                                           void const *slot);
void *ccc_impl_fhm_key_in_slot(struct ccc_fhm_ const *h, void const *slot);
uint64_t *ccc_impl_fhm_hash_at(struct ccc_fhm_ const *h, size_t i);
size_t ccc_impl_fhm_distance(size_t capacity, size_t i, size_t j);
ccc_result ccc_impl_fhm_maybe_resize(struct ccc_fhm_ *);
uint64_t ccc_impl_fhm_filter(struct ccc_fhm_ const *, void const *key);
void *ccc_impl_fhm_base(struct ccc_fhm_ const *h);
size_t ccc_impl_fhm_increment(size_t capacity, size_t i);

/*==================   Helper Macros for Repeated Logic     =================*/

/* Internal helper assumes that swap_entry has already been evaluated once
   which it must have to make it to this point. */
#define ccc_impl_fhm_swaps(swap_entry, lazy_key_value...)                      \
    ({                                                                         \
        size_t fhm_i_ = ccc_buf_index_of(&((swap_entry)->h_->buf_),            \
                                         (swap_entry)->entry_.e_);             \
        if (*ccc_impl_fhm_hash_at((swap_entry)->h_, fhm_i_) == CCC_FHM_EMPTY)  \
        {                                                                      \
            *((typeof(lazy_key_value) *)ccc_buf_at(&((swap_entry)->h_->buf_),  \
                                                   fhm_i_))                    \
                = lazy_key_value;                                              \
            *ccc_impl_fhm_hash_at((swap_entry)->h_, fhm_i_)                    \
                = (swap_entry)->hash_;                                         \
            ccc_buf_size_plus(&(swap_entry)->h_->buf_);                        \
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
        struct ccc_fhm_entry_ fhm_mod_with_ent_                                \
            = (flat_hash_map_entry)->impl_;                                    \
        if (fhm_mod_with_ent_.entry_.stats_ == CCC_ENTRY_OCCUPIED)             \
        {                                                                      \
            __auto_type fhm_aux_ = aux;                                        \
            (mod_fn)(&(ccc_update){fhm_mod_with_ent_.entry_.e_, &fhm_aux_});   \
        }                                                                      \
        fhm_mod_with_ent_;                                                     \
    })

#define ccc_impl_fhm_or_insert_w(flat_hash_map_entry, lazy_key_value...)       \
    ({                                                                         \
        typeof(lazy_key_value) *res_;                                          \
        struct ccc_fhm_entry_ *fhm_or_ins_entry_                               \
            = &(flat_hash_map_entry)->impl_;                                   \
        assert(sizeof(*res_)                                                   \
               == ccc_buf_elem_size(&(fhm_or_ins_entry_->h_->buf_)));          \
        if (fhm_or_ins_entry_->entry_.stats_ & CCC_ENTRY_OCCUPIED)             \
        {                                                                      \
            res_ = fhm_or_ins_entry_->entry_.e_;                               \
        }                                                                      \
        else if (fhm_or_ins_entry_->entry_.stats_ & ~CCC_ENTRY_VACANT)         \
        {                                                                      \
            res_ = NULL;                                                       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            res_ = ccc_impl_fhm_swaps(fhm_or_ins_entry_, lazy_key_value);      \
        }                                                                      \
        res_;                                                                  \
    })

#define ccc_impl_fhm_insert_entry_w(flat_hash_map_entry_ptr,                   \
                                    lazy_key_value...)                         \
    ({                                                                         \
        typeof(lazy_key_value) *fhm_res_;                                      \
        struct ccc_fhm_entry_ *fhm_ins_ent_                                    \
            = &(flat_hash_map_entry_ptr)->impl_;                               \
        assert(sizeof(*fhm_res_)                                               \
               == ccc_buf_elem_size(&(fhm_ins_ent_->h_->buf_)));               \
        if (fhm_ins_ent_->entry_.stats_ & CCC_ENTRY_OCCUPIED)                  \
        {                                                                      \
            fhm_ins_ent_->entry_.stats_ = CCC_ENTRY_OCCUPIED;                  \
            *((typeof(fhm_res_))fhm_ins_ent_->entry_.e_) = lazy_key_value;     \
            ccc_impl_fhm_in_slot(fhm_ins_ent_->h_, fhm_ins_ent_->entry_.e_)    \
                ->hash_                                                        \
                = fhm_ins_ent_->hash_;                                         \
            fhm_res_ = fhm_ins_ent_->entry_.e_;                                \
        }                                                                      \
        else if (fhm_ins_ent_->entry_.stats_ & ~CCC_ENTRY_OCCUPIED)            \
        {                                                                      \
            fhm_res_ = NULL;                                                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            fhm_res_ = ccc_impl_fhm_swaps(fhm_ins_ent_, lazy_key_value);       \
        }                                                                      \
        fhm_res_;                                                              \
    })

#define ccc_impl_fhm_try_insert_w(flat_hash_map_ptr, key, lazy_value...)       \
    ({                                                                         \
        __auto_type fhm_key_ = key;                                            \
        struct ccc_fhm_ *flat_hash_map_ptr_ = (flat_hash_map_ptr);             \
        struct ccc_fhm_entry_ fhm_try_ins_ent_                                 \
            = ccc_impl_fhm_entry(flat_hash_map_ptr_, &fhm_key_);               \
        struct ccc_entry_ fhm_try_insert_res_ = {};                            \
        if ((fhm_try_ins_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)              \
            || (fhm_try_ins_ent_.entry_.stats_ & ~CCC_ENTRY_VACANT))           \
        {                                                                      \
            fhm_try_insert_res_ = fhm_try_ins_ent_.entry_;                     \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            fhm_try_insert_res_ = (struct ccc_entry_){                         \
                ccc_impl_fhm_swaps((&fhm_try_ins_ent_), lazy_value),           \
                CCC_ENTRY_VACANT,                                              \
            };                                                                 \
            *((typeof(fhm_key_) *)ccc_impl_fhm_key_in_slot(                    \
                flat_hash_map_ptr_, fhm_try_insert_res_.e_))                   \
                = fhm_key_;                                                    \
        }                                                                      \
        fhm_try_insert_res_;                                                   \
    })

#define ccc_impl_fhm_insert_or_assign_w(flat_hash_map_ptr, key, lazy_value...) \
    ({                                                                         \
        __auto_type fhm_key_ = key;                                            \
        struct ccc_fhm_ *flat_hash_map_ptr_ = (flat_hash_map_ptr);             \
        struct ccc_fhm_entry_ fhm_ins_or_assign_ent_                           \
            = ccc_impl_fhm_entry(flat_hash_map_ptr_, &fhm_key_);               \
        struct ccc_entry_ fhm_ins_or_assign_res_ = {};                         \
        if (fhm_ins_or_assign_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)         \
        {                                                                      \
            fhm_ins_or_assign_res_ = fhm_ins_or_assign_ent_.entry_;            \
            *((typeof(lazy_value) *)fhm_ins_or_assign_res_.e_) = lazy_value;   \
            *((typeof(fhm_key_) *)ccc_impl_fhm_key_in_slot(                    \
                flat_hash_map_ptr_, fhm_ins_or_assign_res_.e_))                \
                = fhm_key_;                                                    \
            ccc_impl_fhm_in_slot(fhm_ins_or_assign_ent_.h_,                    \
                                 fhm_ins_or_assign_ent_.entry_.e_)             \
                ->hash_                                                        \
                = fhm_ins_or_assign_ent_.hash_;                                \
        }                                                                      \
        else if (fhm_ins_or_assign_ent_.entry_.stats_ & ~CCC_ENTRY_VACANT)     \
        {                                                                      \
            fhm_ins_or_assign_res_ = fhm_ins_or_assign_ent_.entry_;            \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            fhm_ins_or_assign_res_ = (struct ccc_entry_){                      \
                ccc_impl_fhm_swaps((&fhm_ins_or_assign_ent_), lazy_value),     \
                CCC_ENTRY_VACANT,                                              \
            };                                                                 \
            *((typeof(fhm_key_) *)ccc_impl_fhm_key_in_slot(                    \
                flat_hash_map_ptr_, fhm_ins_or_assign_res_.e_))                \
                = fhm_key_;                                                    \
        }                                                                      \
        fhm_ins_or_assign_res_;                                                \
    })

#endif /* CCC_IMPL_FLAT_HASH_MAP_H */
