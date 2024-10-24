#ifndef CCC_IMPL_FLAT_ORDERED_MAP_H
#define CCC_IMPL_FLAT_ORDERED_MAP_H

#include "buffer.h"
#include "types.h"

#include <stddef.h>

struct ccc_fom_elem_
{
    size_t branch_[2];
    size_t parent_;
};

struct ccc_fom_
{
    ccc_buffer buf_;
    size_t root_;
    size_t key_offset_;
    size_t node_elem_offset_;
    ccc_key_cmp_fn *cmp_;
    void *aux_;
};

struct ccc_fom_entry_
{
    struct ccc_fom_ *fom_;
    ccc_threeway_cmp last_cmp_;
    /* The types.h entry is not quite suitable for this container so change. */
    size_t i_;
    /* Keep these types in sync in cases status reporting changes. */
    typeof((ccc_entry){}.impl_.stats_) stats_;
};

#define ccc_impl_fom_init(mem_ptr, capacity, node_elem_field, key_elem_field,  \
                          alloc_fn, key_cmp_fn, aux_data)                      \
    {                                                                          \
        .buf_ = ccc_buf_init(mem_ptr, alloc_fn, capacity), .root_ = 0,         \
        .key_offset_ = offsetof(typeof(*(mem_ptr)), key_elem_field),           \
        .node_elem_offset_ = offsetof(typeof(*(mem_ptr)), node_elem_field),    \
        .cmp_ = (key_cmp_fn), .aux_ = (aux_data),                              \
    }

void *ccc_impl_fom_insert(struct ccc_fom_ *fom, size_t elem_i);
struct ccc_fom_entry_ ccc_impl_fom_entry(struct ccc_fom_ *fom, void const *key);
void *ccc_impl_fom_key_from_node(struct ccc_fom_ const *fom,
                                 struct ccc_fom_elem_ const *elem);
void *ccc_impl_fom_key_in_slot(struct ccc_fom_ const *fom, void const *slot);
struct ccc_fom_elem_ *ccc_impl_fom_elem_in_slot(struct ccc_fom_ const *fom,
                                                void const *slot);
void *ccc_impl_fom_alloc_back(struct ccc_fom_ *fom);

/* NOLINTBEGIN(readability-identifier-naming) */

/*==================     Core Macro Implementations     =====================*/

#define ccc_impl_fom_and_modify_w(flat_ordered_map_entry, mod_fn, aux_data...) \
    ({                                                                         \
        struct ccc_fom_entry_ fom_mod_ent_ = (flat_ordered_map_entry);         \
        if (fom_mod_ent_.stats_ & CCC_ENTRY_OCCUPIED)                          \
        {                                                                      \
            __auto_type fom_aux_data_ = aux_data;                              \
            (mod_fn)((ccc_user_type_mut){.user_type = e.entry,                 \
                                         .aux = &fom_aux_data_});              \
        }                                                                      \
        fom_mod_ent_;                                                          \
    })

#define ccc_impl_fom_or_insert_w(flat_ordered_map_entry, lazy_key_value...)    \
    ({                                                                         \
        struct ccc_fom_entry_ *fom_or_ins_ent_                                 \
            = &(flat_ordered_map_entry)->impl_;                                \
        typeof(lazy_key_value) *fom_or_ins_ret_ = NULL;                        \
        if (fom_or_ins_ent_->stats_ == CCC_ENTRY_OCCUPIED)                     \
        {                                                                      \
            fom_or_ins_ret_ = ccc_buf_at(&fom_or_ins_ent_->fom_->buf_,         \
                                         fom_or_ins_ent_->i_);                 \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            fom_or_ins_ret_ = ccc_impl_fom_alloc_back(fom_or_ins_ent_->fom_);  \
            if (fom_or_ins_ret_)                                               \
            {                                                                  \
                *fom_or_ins_ret_ = lazy_key_value;                             \
                (void)ccc_impl_fom_insert(                                     \
                    fom_or_ins_ent_->fom_,                                     \
                    ccc_buf_size(&fom_or_ins_ent_->fom_->buf_) - 1);           \
            }                                                                  \
        }                                                                      \
        fom_or_ins_ret_;                                                       \
    })

#define ccc_impl_fom_insert_entry_w(flat_ordered_map_entry, lazy_key_value...) \
    ({                                                                         \
        struct ccc_fom_entry_ *fom_ins_ent_                                    \
            = &(flat_ordered_map_entry)->impl_;                                \
        typeof(lazy_key_value) *fom_ins_ent_ret_ = NULL;                       \
        if (!(fom_ins_ent_->stats_ & CCC_ENTRY_OCCUPIED))                      \
        {                                                                      \
            fom_ins_ent_ret_ = ccc_impl_fom_alloc_back(fom_ins_ent_->fom_);    \
            if (fom_ins_ent_ret_)                                              \
            {                                                                  \
                *fom_ins_ent_ret_ = lazy_key_value;                            \
                (void)ccc_impl_fom_insert(                                     \
                    fom_ins_ent_->fom_,                                        \
                    ccc_buf_size(&fom_ins_ent_->fom_->buf_) - 1);              \
            }                                                                  \
        }                                                                      \
        else if (fom_ins_ent_->stats_ == CCC_ENTRY_OCCUPIED)                   \
        {                                                                      \
            fom_ins_ent_ret_                                                   \
                = ccc_buf_at(&fom_ins_ent_->fom_->buf_, fom_ins_ent_->i_);     \
            struct ccc_fom_elem_ ins_ent_saved_ = *ccc_impl_fom_elem_in_slot(  \
                fom_ins_ent_->fom_, fom_ins_ent_ret_);                         \
            *fom_ins_ent_ret_ = lazy_key_value;                                \
            *ccc_impl_fom_elem_in_slot(fom_ins_ent_->fom_, fom_ins_ent_ret_)   \
                = ins_ent_saved_;                                              \
        }                                                                      \
        fom_ins_ent_ret_;                                                      \
    })

#define ccc_impl_fom_try_insert_w(flat_ordered_map_ptr, key, lazy_value...)    \
    ({                                                                         \
        __auto_type fom_key_ = (key);                                          \
        struct ccc_fom_entry_ fom_try_ins_ent_                                 \
            = ccc_impl_fom_entry((flat_ordered_map_ptr), &fom_key_);           \
        struct ccc_entry_ fom_try_ins_ent_ret_ = {};                           \
        if (!(fom_try_ins_ent_.stats_ & CCC_ENTRY_OCCUPIED))                   \
        {                                                                      \
            fom_try_ins_ent_ret_ = (struct ccc_entry_){                        \
                .e_ = ccc_impl_fom_alloc_back(fom_try_ins_ent_.fom_),          \
                .stats_ = CCC_ENTRY_INSERT_ERROR};                             \
            if (fom_try_ins_ent_ret_.e_)                                       \
            {                                                                  \
                *((typeof(lazy_value) *)fom_try_ins_ent_ret_.e_) = lazy_value; \
                *((typeof(fom_key_) *)ccc_impl_fom_key_in_slot(                \
                    fom_try_ins_ent_.fom_, fom_try_ins_ent_ret_.e_))           \
                    = fom_key_;                                                \
                (void)ccc_impl_fom_insert(                                     \
                    fom_try_ins_ent_.fom_,                                     \
                    ccc_buf_i(&fom_try_ins_ent_.fom_->buf_,                    \
                              fom_try_ins_ent_ret_.e_));                       \
                fom_try_ins_ent_ret_.stats_ = CCC_ENTRY_VACANT;                \
            }                                                                  \
        }                                                                      \
        else if (fom_try_ins_ent_.stats_ == CCC_ENTRY_OCCUPIED)                \
        {                                                                      \
            fom_try_ins_ent_ret_ = (struct ccc_entry_){                        \
                ccc_buf_at(&fom_try_ins_ent_.fom_->buf_, fom_try_ins_ent_.i_), \
                .stats_ = fom_try_ins_ent_.stats_};                            \
        }                                                                      \
        fom_try_ins_ent_ret_;                                                  \
    })

#define ccc_impl_fom_insert_or_assign_w(flat_ordered_map_ptr, key,             \
                                        lazy_value...)                         \
    ({                                                                         \
        __auto_type fom_key_ = (key);                                          \
        struct ccc_fom_entry_ fom_ins_or_assign_ent_                           \
            = ccc_impl_fom_entry((flat_ordered_map_ptr), &fom_key_);           \
        struct ccc_entry_ fom_ins_or_assign_ent_ret_ = {};                     \
        if (!(fom_ins_or_assign_ent_.stats_ & CCC_ENTRY_OCCUPIED))             \
        {                                                                      \
            fom_ins_or_assign_ent_ret_ = (struct ccc_entry_){                  \
                .e_ = ccc_impl_fom_alloc_back(fom_ins_or_assign_ent_.fom_),    \
                .stats_ = CCC_ENTRY_INSERT_ERROR};                             \
            if (fom_ins_or_assign_ent_ret_.e_)                                 \
            {                                                                  \
                *((typeof(lazy_value) *)fom_ins_or_assign_ent_ret_.e_)         \
                    = lazy_value;                                              \
                *((typeof(fom_key_) *)ccc_impl_fom_key_in_slot(                \
                    fom_ins_or_assign_ent_.fom_,                               \
                    fom_ins_or_assign_ent_ret_.e_))                            \
                    = fom_key_;                                                \
                (void)ccc_impl_fom_insert(                                     \
                    fom_ins_or_assign_ent_.fom_,                               \
                    ccc_buf_i(&fom_ins_or_assign_ent_.fom_->buf_,              \
                              fom_ins_or_assign_ent_ret_.e_));                 \
                fom_ins_or_assign_ent_ret_.stats_ = CCC_ENTRY_VACANT;          \
            }                                                                  \
        }                                                                      \
        else if (fom_ins_or_assign_ent_.stats_ == CCC_ENTRY_OCCUPIED)          \
        {                                                                      \
            void *fom_ins_or_assign_slot_                                      \
                = ccc_buf_at(&fom_ins_or_assign_ent_.fom_->buf_,               \
                             fom_ins_or_assign_ent_.i_);                       \
            struct ccc_fom_elem_ ins_ent_saved_ = *ccc_impl_fom_elem_in_slot(  \
                fom_ins_or_assign_ent_.fom_, fom_ins_or_assign_slot_);         \
            *((typeof(lazy_value) *)fom_ins_or_assign_slot_) = lazy_value;     \
            *ccc_impl_fom_elem_in_slot(fom_ins_or_assign_ent_.fom_,            \
                                       fom_ins_or_assign_slot_)                \
                = ins_ent_saved_;                                              \
            fom_ins_or_assign_ent_ret_ = (struct ccc_entry_){                  \
                .e_ = fom_ins_or_assign_slot_,                                 \
                .stats_ = fom_ins_or_assign_ent_.stats_};                      \
            *((typeof(fom_key_) *)ccc_impl_fom_key_in_slot(                    \
                fom_ins_or_assign_ent_.fom_, fom_ins_or_assign_slot_))         \
                = fom_key_;                                                    \
        }                                                                      \
        fom_ins_or_assign_ent_ret_;                                            \
    })

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_ORDERED_MAP_H */
