#ifndef CCC_IMPL_FLAT_ORDERED_MAP_H
#define CCC_IMPL_FLAT_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../buffer.h"
#include "../types.h"

/** @private */
struct ccc_fomap_elem_
{
    size_t branch_[2];
    size_t parent_;
};

/** @private */
struct ccc_fomap_
{
    ccc_buffer buf_;
    size_t root_;
    size_t key_offset_;
    size_t node_elem_offset_;
    ccc_key_cmp_fn *cmp_;
};

/** @private */
struct ccc_ftree_entry_
{
    struct ccc_fomap_ *fom_;
    ccc_threeway_cmp last_cmp_;
    /* The types.h entry is not quite suitable for this container so change. */
    size_t i_;
    /* Keep these types in sync in cases status reporting changes. */
    typeof((ccc_entry){}.impl_.stats_) stats_;
};

/** @private */
union ccc_fomap_entry_
{
    struct ccc_ftree_entry_ impl_;
};

#define ccc_impl_fom_init(mem_ptr, node_elem_field, key_elem_field,            \
                          key_cmp_fn, alloc_fn, aux_data, capacity)            \
    {                                                                          \
        .buf_ = ccc_buf_init(mem_ptr, alloc_fn, aux_data, capacity),           \
        .root_ = 0,                                                            \
        .key_offset_ = offsetof(typeof(*(mem_ptr)), key_elem_field),           \
        .node_elem_offset_ = offsetof(typeof(*(mem_ptr)), node_elem_field),    \
        .cmp_ = (key_cmp_fn),                                                  \
    }

void *ccc_impl_fom_insert(struct ccc_fomap_ *fom, size_t elem_i);
struct ccc_ftree_entry_ ccc_impl_fom_entry(struct ccc_fomap_ *fom,
                                           void const *key);
void *ccc_impl_fom_key_from_node(struct ccc_fomap_ const *fom,
                                 struct ccc_fomap_elem_ const *elem);
void *ccc_impl_fom_key_in_slot(struct ccc_fomap_ const *fom, void const *slot);
struct ccc_fomap_elem_ *
ccc_impl_fomap_elem_in_slot(struct ccc_fomap_ const *fom, void const *slot);
void *ccc_impl_fom_alloc_back(struct ccc_fomap_ *fom);

/* NOLINTBEGIN(readability-identifier-naming) */

/*==================     Core Macro Implementations     =====================*/

#define ccc_impl_fom_and_modify_w(flat_ordered_map_entry_ptr, type_name,       \
                                  closure_over_T...)                           \
    (__extension__({                                                           \
        __auto_type fom_mod_ent_ptr_ = (flat_ordered_map_entry_ptr);           \
        struct ccc_ftree_entry_ fom_mod_ent_ = {.stats_ = CCC_INPUT_ERROR};    \
        if (fom_mod_ent_ptr_)                                                  \
        {                                                                      \
            fom_mod_ent_ = fom_mod_ent_ptr_->impl_;                            \
            if (fom_mod_ent_.stats_ & CCC_OCCUPIED)                            \
            {                                                                  \
                type_name *const T                                             \
                    = ccc_buf_at(&fom_mod_ent_.fom_->buf_, fom_mod_ent_.i_);   \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        fom_mod_ent_;                                                          \
    }))

#define ccc_impl_fom_or_insert_w(flat_ordered_map_entry_ptr,                   \
                                 lazy_key_value...)                            \
    (__extension__({                                                           \
        __auto_type fom_or_ins_ent_ptr_ = (flat_ordered_map_entry_ptr);        \
        typeof(lazy_key_value) *fom_or_ins_ret_ = NULL;                        \
        if (fom_or_ins_ent_ptr_)                                               \
        {                                                                      \
            struct ccc_ftree_entry_ *fom_or_ins_ent_                           \
                = &fom_or_ins_ent_ptr_->impl_;                                 \
            if (fom_or_ins_ent_->stats_ == CCC_OCCUPIED)                       \
            {                                                                  \
                fom_or_ins_ret_ = ccc_buf_at(&fom_or_ins_ent_->fom_->buf_,     \
                                             fom_or_ins_ent_->i_);             \
            }                                                                  \
            else                                                               \
            {                                                                  \
                fom_or_ins_ret_                                                \
                    = ccc_impl_fom_alloc_back(fom_or_ins_ent_->fom_);          \
                if (fom_or_ins_ret_)                                           \
                {                                                              \
                    *fom_or_ins_ret_ = lazy_key_value;                         \
                    (void)ccc_impl_fom_insert(                                 \
                        fom_or_ins_ent_->fom_,                                 \
                        ccc_buf_size(&fom_or_ins_ent_->fom_->buf_) - 1);       \
                }                                                              \
            }                                                                  \
        }                                                                      \
        fom_or_ins_ret_;                                                       \
    }))

#define ccc_impl_fom_insert_entry_w(flat_ordered_map_entry_ptr,                \
                                    lazy_key_value...)                         \
    (__extension__({                                                           \
        __auto_type fom_ins_ent_ptr_ = (flat_ordered_map_entry_ptr);           \
        typeof(lazy_key_value) *fom_ins_ent_ret_ = NULL;                       \
        if (fom_ins_ent_ptr_)                                                  \
        {                                                                      \
            struct ccc_ftree_entry_ *fom_ins_ent_ = &fom_ins_ent_ptr_->impl_;  \
            if (!(fom_ins_ent_->stats_ & CCC_OCCUPIED))                        \
            {                                                                  \
                fom_ins_ent_ret_                                               \
                    = ccc_impl_fom_alloc_back(fom_ins_ent_->fom_);             \
                if (fom_ins_ent_ret_)                                          \
                {                                                              \
                    *fom_ins_ent_ret_ = lazy_key_value;                        \
                    (void)ccc_impl_fom_insert(                                 \
                        fom_ins_ent_->fom_,                                    \
                        ccc_buf_size(&fom_ins_ent_->fom_->buf_) - 1);          \
                }                                                              \
            }                                                                  \
            else if (fom_ins_ent_->stats_ == CCC_OCCUPIED)                     \
            {                                                                  \
                fom_ins_ent_ret_                                               \
                    = ccc_buf_at(&fom_ins_ent_->fom_->buf_, fom_ins_ent_->i_); \
                struct ccc_fomap_elem_ ins_ent_saved_                          \
                    = *ccc_impl_fomap_elem_in_slot(fom_ins_ent_->fom_,         \
                                                   fom_ins_ent_ret_);          \
                *fom_ins_ent_ret_ = lazy_key_value;                            \
                *ccc_impl_fomap_elem_in_slot(fom_ins_ent_->fom_,               \
                                             fom_ins_ent_ret_)                 \
                    = ins_ent_saved_;                                          \
            }                                                                  \
        }                                                                      \
        fom_ins_ent_ret_;                                                      \
    }))

#define ccc_impl_fom_try_insert_w(flat_ordered_map_ptr, key, lazy_value...)    \
    (__extension__({                                                           \
        __auto_type fom_try_ins_map_ptr_ = (flat_ordered_map_ptr);             \
        struct ccc_ent_ fom_try_ins_ent_ret_ = {.stats_ = CCC_INPUT_ERROR};    \
        if (fom_try_ins_map_ptr_)                                              \
        {                                                                      \
            __auto_type fom_key_ = (key);                                      \
            struct ccc_ftree_entry_ fom_try_ins_ent_                           \
                = ccc_impl_fom_entry(fom_try_ins_map_ptr_, (void *)&fom_key_); \
            if (!(fom_try_ins_ent_.stats_ & CCC_OCCUPIED))                     \
            {                                                                  \
                fom_try_ins_ent_ret_ = (struct ccc_ent_){                      \
                    .e_ = ccc_impl_fom_alloc_back(fom_try_ins_ent_.fom_),      \
                    .stats_ = CCC_INSERT_ERROR};                               \
                if (fom_try_ins_ent_ret_.e_)                                   \
                {                                                              \
                    *((typeof(lazy_value) *)fom_try_ins_ent_ret_.e_)           \
                        = lazy_value;                                          \
                    *((typeof(fom_key_) *)ccc_impl_fom_key_in_slot(            \
                        fom_try_ins_ent_.fom_, fom_try_ins_ent_ret_.e_))       \
                        = fom_key_;                                            \
                    (void)ccc_impl_fom_insert(                                 \
                        fom_try_ins_ent_.fom_,                                 \
                        ccc_buf_i(&fom_try_ins_ent_.fom_->buf_,                \
                                  fom_try_ins_ent_ret_.e_));                   \
                    fom_try_ins_ent_ret_.stats_ = CCC_VACANT;                  \
                }                                                              \
            }                                                                  \
            else if (fom_try_ins_ent_.stats_ == CCC_OCCUPIED)                  \
            {                                                                  \
                fom_try_ins_ent_ret_ = (struct ccc_ent_){                      \
                    ccc_buf_at(&fom_try_ins_ent_.fom_->buf_,                   \
                               fom_try_ins_ent_.i_),                           \
                    .stats_ = fom_try_ins_ent_.stats_};                        \
            }                                                                  \
        }                                                                      \
        fom_try_ins_ent_ret_;                                                  \
    }))

#define ccc_impl_fom_insert_or_assign_w(flat_ordered_map_ptr, key,             \
                                        lazy_value...)                         \
    (__extension__({                                                           \
        __auto_type fom_ins_or_assign_map_ptr_ = (flat_ordered_map_ptr);       \
        struct ccc_ent_ fom_ins_or_assign_ent_ret_                             \
            = {.stats_ = CCC_INPUT_ERROR};                                     \
        if (fom_ins_or_assign_map_ptr_)                                        \
        {                                                                      \
            __auto_type fom_key_ = (key);                                      \
            struct ccc_ftree_entry_ fom_ins_or_assign_ent_                     \
                = ccc_impl_fom_entry((flat_ordered_map_ptr),                   \
                                     (void *)&fom_key_);                       \
            if (!(fom_ins_or_assign_ent_.stats_ & CCC_OCCUPIED))               \
            {                                                                  \
                fom_ins_or_assign_ent_ret_                                     \
                    = (struct ccc_ent_){.e_ = ccc_impl_fom_alloc_back(         \
                                            fom_ins_or_assign_ent_.fom_),      \
                                        .stats_ = CCC_INSERT_ERROR};           \
                if (fom_ins_or_assign_ent_ret_.e_)                             \
                {                                                              \
                    *((typeof(lazy_value) *)fom_ins_or_assign_ent_ret_.e_)     \
                        = lazy_value;                                          \
                    *((typeof(fom_key_) *)ccc_impl_fom_key_in_slot(            \
                        fom_ins_or_assign_ent_.fom_,                           \
                        fom_ins_or_assign_ent_ret_.e_))                        \
                        = fom_key_;                                            \
                    (void)ccc_impl_fom_insert(                                 \
                        fom_ins_or_assign_ent_.fom_,                           \
                        ccc_buf_i(&fom_ins_or_assign_ent_.fom_->buf_,          \
                                  fom_ins_or_assign_ent_ret_.e_));             \
                    fom_ins_or_assign_ent_ret_.stats_ = CCC_VACANT;            \
                }                                                              \
            }                                                                  \
            else if (fom_ins_or_assign_ent_.stats_ == CCC_OCCUPIED)            \
            {                                                                  \
                void *fom_ins_or_assign_slot_                                  \
                    = ccc_buf_at(&fom_ins_or_assign_ent_.fom_->buf_,           \
                                 fom_ins_or_assign_ent_.i_);                   \
                struct ccc_fomap_elem_ ins_ent_saved_                          \
                    = *ccc_impl_fomap_elem_in_slot(                            \
                        fom_ins_or_assign_ent_.fom_, fom_ins_or_assign_slot_); \
                *((typeof(lazy_value) *)fom_ins_or_assign_slot_) = lazy_value; \
                *ccc_impl_fomap_elem_in_slot(fom_ins_or_assign_ent_.fom_,      \
                                             fom_ins_or_assign_slot_)          \
                    = ins_ent_saved_;                                          \
                fom_ins_or_assign_ent_ret_ = (struct ccc_ent_){                \
                    .e_ = fom_ins_or_assign_slot_,                             \
                    .stats_ = fom_ins_or_assign_ent_.stats_};                  \
                *((typeof(fom_key_) *)ccc_impl_fom_key_in_slot(                \
                    fom_ins_or_assign_ent_.fom_, fom_ins_or_assign_slot_))     \
                    = fom_key_;                                                \
            }                                                                  \
        }                                                                      \
        fom_ins_or_assign_ent_ret_;                                            \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_ORDERED_MAP_H */
