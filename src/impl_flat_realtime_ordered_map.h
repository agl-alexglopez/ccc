#ifndef IMPL_FLAT_REALTIME_ORDERED_MAP_H
#define IMPL_FLAT_REALTIME_ORDERED_MAP_H

#include "buffer.h"
#include "types.h"

#include <stddef.h>
#include <stdint.h>

struct ccc_frm_elem_
{
    size_t branch_[2];
    size_t parent_;
    uint8_t parity_;
};

struct ccc_frm_
{
    ccc_buffer buf_;
    size_t root_;
    size_t key_offset_;
    size_t node_elem_offset_;
    ccc_key_cmp_fn *cmp_;
    void *aux_;
};

struct ccc_frm_entry_
{
    struct ccc_frm_ *frm_;
    ccc_threeway_cmp last_cmp_;
    /* The types.h entry is not quite suitable for this container so change. */
    size_t i_;
    /* Keep these types in sync in cases status reporting changes. */
    typeof((ccc_entry){}.impl_.stats_) stats_;
};

void *ccc_impl_frm_key_from_node(struct ccc_frm_ const *frm,
                                 struct ccc_frm_elem_ const *elem);
void *ccc_impl_frm_key_in_slot(struct ccc_frm_ const *frm, void const *slot);
struct ccc_frm_elem_ *ccc_impl_frm_elem_in_slot(struct ccc_frm_ const *frm,
                                                void const *slot);
struct ccc_frm_entry_ ccc_impl_frm_entry(struct ccc_frm_ const *frm,
                                         void const *key);
void *ccc_impl_frm_insert(struct ccc_frm_ *frm, size_t parent_i,
                          ccc_threeway_cmp last_cmp, size_t elem_i);
void *ccc_impl_frm_alloc_back(struct ccc_frm_ *frm);

/*=========================      Initialization     =========================*/

#define ccc_impl_frm_init(memory_ptr, capacity, struct_name, node_elem_field,  \
                          key_elem_field, alloc_fn, key_cmp_fn, aux_data)      \
    {                                                                          \
        .buf_ = ccc_buf_init(memory_ptr, struct_name, capacity, alloc_fn),     \
        .root_ = 0, .key_offset_ = offsetof(struct_name, key_elem_field),      \
        .node_elem_offset_ = offsetof(struct_name, node_elem_field),           \
        .cmp_ = (key_cmp_fn), .aux_ = (aux_data),                              \
    }

/*==================     Core Macro Implementations     =====================*/

#define ccc_impl_frm_and_modify_w(flat_realtime_ordered_map_entry, mod_fn,     \
                                  aux_data...)                                 \
    ({                                                                         \
        struct ccc_frm_entry_ frm_mod_ent_                                     \
            = (flat_realtime_ordered_map_entry);                               \
        if (frm_mod_ent_.stats_ & CCC_ENTRY_OCCUPIED)                          \
        {                                                                      \
            __auto_type frm_aux_data_ = aux_data;                              \
            (mod_fn)(&(ccc_update){.container = (void *const)e.entry,          \
                                   .aux = &frm_aux_data_});                    \
        }                                                                      \
        frm_mod_ent_;                                                          \
    })

#define ccc_impl_frm_or_insert_w(flat_realtime_ordered_map_entry,              \
                                 lazy_key_value...)                            \
    ({                                                                         \
        struct ccc_frm_entry_ *frm_or_ins_ent_                                 \
            = &(flat_realtime_ordered_map_entry)->impl_;                       \
        typeof(lazy_key_value) *frm_or_ins_ret_ = NULL;                        \
        if (frm_or_ins_ent_->stats_ == CCC_ENTRY_OCCUPIED)                     \
        {                                                                      \
            frm_or_ins_ret_ = ccc_buf_at(&frm_or_ins_ent_->frm_->buf_,         \
                                         frm_or_ins_ent_->i_);                 \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            frm_or_ins_ret_ = ccc_impl_frm_alloc_back(frm_or_ins_ent_->frm_);  \
            if (frm_or_ins_ret_)                                               \
            {                                                                  \
                *frm_or_ins_ret_ = lazy_key_value;                             \
                (void)ccc_impl_frm_insert(                                     \
                    frm_or_ins_ent_->frm_, frm_or_ins_ent_->i_,                \
                    frm_or_ins_ent_->last_cmp_,                                \
                    ccc_buf_size(&frm_or_ins_ent_->frm_->buf_) - 1);           \
            }                                                                  \
        }                                                                      \
        frm_or_ins_ret_;                                                       \
    })

#define ccc_impl_frm_insert_entry_w(flat_realtime_ordered_map_entry,           \
                                    lazy_key_value...)                         \
    ({                                                                         \
        struct ccc_frm_entry_ *frm_ins_ent_                                    \
            = &(flat_realtime_ordered_map_entry)->impl_;                       \
        typeof(lazy_key_value) *frm_ins_ent_ret_ = NULL;                       \
        if (!(frm_ins_ent_->stats_ & CCC_ENTRY_OCCUPIED))                      \
        {                                                                      \
            frm_ins_ent_ret_ = ccc_impl_frm_alloc_back(frm_ins_ent_->frm_);    \
            if (frm_ins_ent_ret_)                                              \
            {                                                                  \
                *frm_ins_ent_ret_ = lazy_key_value;                            \
                (void)ccc_impl_frm_insert(                                     \
                    frm_ins_ent_->frm_, frm_ins_ent_->i_,                      \
                    frm_ins_ent_->last_cmp_,                                   \
                    ccc_buf_size(&frm_ins_ent_->frm_->buf_) - 1);              \
            }                                                                  \
        }                                                                      \
        else if (frm_ins_ent_->stats_ == CCC_ENTRY_OCCUPIED)                   \
        {                                                                      \
            frm_ins_ent_ret_                                                   \
                = ccc_buf_at(&frm_ins_ent_->frm_->buf_, frm_ins_ent_->i_);     \
            struct ccc_frm_elem_ ins_ent_saved_ = *ccc_impl_frm_elem_in_slot(  \
                frm_ins_ent_->frm_, frm_ins_ent_ret_);                         \
            *frm_ins_ent_ret_ = lazy_key_value;                                \
            *ccc_impl_frm_elem_in_slot(frm_ins_ent_->frm_, frm_ins_ent_ret_)   \
                = ins_ent_saved_;                                              \
        }                                                                      \
        frm_ins_ent_ret_;                                                      \
    })

#define ccc_impl_frm_try_insert_w(flat_realtime_ordered_map_ptr, key,          \
                                  lazy_value...)                               \
    ({                                                                         \
        __auto_type frm_key_ = (key);                                          \
        struct ccc_frm_entry_ frm_try_ins_ent_                                 \
            = ccc_impl_frm_entry((flat_realtime_ordered_map_ptr), &frm_key_);  \
        struct ccc_entry_ frm_try_ins_ent_ret_ = {};                           \
        if (!(frm_try_ins_ent_.stats_ & CCC_ENTRY_OCCUPIED))                   \
        {                                                                      \
            frm_try_ins_ent_ret_ = (struct ccc_entry_){                        \
                .e_ = ccc_impl_frm_alloc_back(frm_try_ins_ent_.frm_),          \
                .stats_ = (CCC_ENTRY_INSERT_ERROR | CCC_ENTRY_CONTAINS_NULL    \
                           | CCC_ENTRY_VACANT)};                               \
            if (frm_try_ins_ent_ret_.e_)                                       \
            {                                                                  \
                *((typeof(lazy_value) *)frm_try_ins_ent_ret_.e_) = lazy_value; \
                *((typeof(frm_key_) *)ccc_impl_frm_key_in_slot(                \
                    frm_try_ins_ent_.frm_, frm_try_ins_ent_ret_.e_))           \
                    = frm_key_;                                                \
                (void)ccc_impl_frm_insert(                                     \
                    frm_try_ins_ent_.frm_, frm_try_ins_ent_.i_,                \
                    frm_try_ins_ent_.last_cmp_,                                \
                    ccc_buf_index_of(&frm_try_ins_ent_.frm_->buf_,             \
                                     frm_try_ins_ent_ret_.e_));                \
                frm_try_ins_ent_ret_.stats_ = CCC_ENTRY_VACANT;                \
            }                                                                  \
        }                                                                      \
        else if (frm_try_ins_ent_.stats_ == CCC_ENTRY_OCCUPIED)                \
        {                                                                      \
            frm_try_ins_ent_ret_ = (struct ccc_entry_){                        \
                ccc_buf_at(&frm_try_ins_ent_.frm_->buf_, frm_try_ins_ent_.i_), \
                .stats_ = frm_try_ins_ent_.stats_};                            \
        }                                                                      \
        frm_try_ins_ent_ret_;                                                  \
    })

#define ccc_impl_frm_insert_or_assign_w(flat_realtime_ordered_map_ptr, key,    \
                                        lazy_value...)                         \
    ({                                                                         \
        __auto_type frm_key_ = (key);                                          \
        struct ccc_frm_entry_ frm_ins_or_assign_ent_                           \
            = ccc_impl_frm_entry((flat_realtime_ordered_map_ptr), &frm_key_);  \
        struct ccc_entry_ frm_ins_or_assign_ent_ret_ = {};                     \
        if (!(frm_ins_or_assign_ent_.stats_ & CCC_ENTRY_OCCUPIED))             \
        {                                                                      \
            frm_ins_or_assign_ent_ret_ = (struct ccc_entry_){                  \
                .e_ = ccc_impl_frm_alloc_back(frm_ins_or_assign_ent_.frm_),    \
                .stats_ = (CCC_ENTRY_INSERT_ERROR | CCC_ENTRY_CONTAINS_NULL    \
                           | CCC_ENTRY_VACANT)};                               \
            if (frm_ins_or_assign_ent_ret_.e_)                                 \
            {                                                                  \
                *((typeof(lazy_value) *)frm_ins_or_assign_ent_ret_.e_)         \
                    = lazy_value;                                              \
                *((typeof(frm_key_) *)ccc_impl_frm_key_in_slot(                \
                    frm_ins_or_assign_ent_.frm_,                               \
                    frm_ins_or_assign_ent_ret_.e_))                            \
                    = frm_key_;                                                \
                (void)ccc_impl_frm_insert(                                     \
                    frm_ins_or_assign_ent_.frm_, frm_ins_or_assign_ent_.i_,    \
                    frm_ins_or_assign_ent_.last_cmp_,                          \
                    ccc_buf_index_of(&frm_ins_or_assign_ent_.frm_->buf_,       \
                                     frm_ins_or_assign_ent_ret_.e_));          \
                frm_ins_or_assign_ent_ret_.stats_ = CCC_ENTRY_VACANT;          \
            }                                                                  \
        }                                                                      \
        else if (frm_ins_or_assign_ent_.stats_ == CCC_ENTRY_OCCUPIED)          \
        {                                                                      \
            void *frm_ins_or_assign_slot_                                      \
                = ccc_buf_at(&frm_ins_or_assign_ent_.frm_->buf_,               \
                             frm_ins_or_assign_ent_.i_);                       \
            struct ccc_frm_elem_ ins_ent_saved_ = *ccc_impl_frm_elem_in_slot(  \
                frm_ins_or_assign_ent_.frm_, frm_ins_or_assign_slot_);         \
            *((typeof(lazy_value) *)frm_ins_or_assign_slot_) = lazy_value;     \
            *ccc_impl_frm_elem_in_slot(frm_ins_or_assign_ent_.frm_,            \
                                       frm_ins_or_assign_slot_)                \
                = ins_ent_saved_;                                              \
            frm_ins_or_assign_ent_ret_ = (struct ccc_entry_){                  \
                .e_ = frm_ins_or_assign_slot_,                                 \
                .stats_ = frm_ins_or_assign_ent_.stats_};                      \
            *((typeof(frm_key_) *)ccc_impl_frm_key_in_slot(                    \
                frm_ins_or_assign_ent_.frm_, frm_ins_or_assign_slot_))         \
                = frm_key_;                                                    \
        }                                                                      \
        frm_ins_or_assign_ent_ret_;                                            \
    })

#endif /* IMPL_FLAT_REALTIME_ORDERED_MAP_H */
