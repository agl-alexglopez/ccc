#ifndef IMPL_HANDLE_REALTIME_ORDERED_MAP_H
#define IMPL_HANDLE_REALTIME_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../buffer.h"
#include "../types.h"

/** @private */
struct ccc_hromap_elem_
{
    size_t branch_[2];
    union
    {
        size_t parent_;
        size_t next_;
    };
    uint8_t parity_;
};

/** @private */
struct ccc_hromap_
{
    ccc_buffer buf_;
    size_t root_;
    size_t free_;
    size_t key_offset_;
    size_t node_elem_offset_;
    ccc_key_cmp_fn *cmp_;
};

/** @private */
struct ccc_hrtree_handle_
{
    struct ccc_hromap_ *hrm_;
    ccc_threeway_cmp last_cmp_;
    struct ccc_handl_ handle_; /** Index and a status. */
};

/** @private */
union ccc_hromap_handle_
{
    struct ccc_hrtree_handle_ impl_;
};

void *ccc_impl_hrm_key_from_node(struct ccc_hromap_ const *hrm,
                                 struct ccc_hromap_elem_ const *elem);
void *ccc_impl_hrm_key_at(struct ccc_hromap_ const *hrm, size_t slot);
struct ccc_hromap_elem_ *ccc_impl_hrm_elem_at(struct ccc_hromap_ const *hrm,
                                              size_t i);
struct ccc_hrtree_handle_ ccc_impl_hrm_handle(struct ccc_hromap_ const *hrm,
                                              void const *key);
void ccc_impl_hrm_insert(struct ccc_hromap_ *hrm, size_t parent_i,
                         ccc_threeway_cmp last_cmp, size_t elem_i);
size_t ccc_impl_hrm_alloc_slot(struct ccc_hromap_ *hrm);

/* NOLINTBEGIN(readability-identifier-naming) */

/*=========================      Initialization     =========================*/

#define ccc_impl_hrm_init(memory_ptr, node_elem_field, key_elem_field,         \
                          key_cmp_fn, alloc_fn, aux_data, capacity)            \
    {                                                                          \
        .buf_ = ccc_buf_init(memory_ptr, alloc_fn, aux_data, capacity),        \
        .root_ = 0,                                                            \
        .free_ = 0,                                                            \
        .key_offset_ = offsetof(typeof(*(memory_ptr)), key_elem_field),        \
        .node_elem_offset_ = offsetof(typeof(*(memory_ptr)), node_elem_field), \
        .cmp_ = (key_cmp_fn),                                                  \
    }

#define ccc_impl_hrm_as(handle_realtime_ordered_map_ptr, type_name, handle...) \
    ((type_name *)ccc_buf_at(&(handle_realtime_ordered_map_ptr)->buf_,         \
                             (handle)))

/*==================     Core Macro Implementations     =====================*/

#define ccc_impl_hrm_and_modify_w(handle_realtime_ordered_map_handle_ptr,      \
                                  type_name, closure_over_T...)                \
    (__extension__({                                                           \
        __auto_type hrm_hndl_ptr_ = (handle_realtime_ordered_map_handle_ptr);  \
        struct ccc_hrtree_handle_ hrm_mod_hndl_                                \
            = {.handle_ = {.stats_ = CCC_INPUT_ERROR}};                        \
        if (hrm_hndl_ptr_)                                                     \
        {                                                                      \
            hrm_mod_hndl_ = hrm_hndl_ptr_->impl_;                              \
            if (hrm_mod_hndl_.handle_.stats_ & CCC_OCCUPIED)                   \
            {                                                                  \
                type_name *const T = ccc_buf_at(&hrm_mod_hndl_.hrm_->buf_,     \
                                                hrm_mod_hndl_.handle_.i_);     \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        hrm_mod_hndl_;                                                         \
    }))

#define ccc_impl_hrm_or_insert_w(handle_realtime_ordered_map_handle_ptr,       \
                                 lazy_key_value...)                            \
    (__extension__({                                                           \
        __auto_type or_ins_handle_ptr_                                         \
            = (handle_realtime_ordered_map_handle_ptr);                        \
        ccc_handle_i hrm_or_ins_ret_ = 0;                                      \
        if (or_ins_handle_ptr_)                                                \
        {                                                                      \
            struct ccc_hrtree_handle_ *hrm_or_ins_hndl_                        \
                = &or_ins_handle_ptr_->impl_;                                  \
            if (hrm_or_ins_hndl_->handle_.stats_ == CCC_OCCUPIED)              \
            {                                                                  \
                hrm_or_ins_ret_ = hrm_or_ins_hndl_->handle_.i_;                \
            }                                                                  \
            else                                                               \
            {                                                                  \
                hrm_or_ins_ret_                                                \
                    = ccc_impl_hrm_alloc_slot(hrm_or_ins_hndl_->hrm_);         \
                if (hrm_or_ins_ret_)                                           \
                {                                                              \
                    *((typeof(lazy_key_value) *)ccc_buf_at(                    \
                        &hrm_or_ins_hndl_->hrm_->buf_, hrm_or_ins_ret_))       \
                        = lazy_key_value;                                      \
                    ccc_impl_hrm_insert(                                       \
                        hrm_or_ins_hndl_->hrm_, hrm_or_ins_hndl_->handle_.i_,  \
                        hrm_or_ins_hndl_->last_cmp_, hrm_or_ins_ret_);         \
                }                                                              \
            }                                                                  \
        }                                                                      \
        hrm_or_ins_ret_;                                                       \
    }))

#define ccc_impl_hrm_insert_handle_w(handle_realtime_ordered_map_handle_ptr,   \
                                     lazy_key_value...)                        \
    (__extension__({                                                           \
        __auto_type ins_handle_ptr_                                            \
            = (handle_realtime_ordered_map_handle_ptr);                        \
        ccc_handle_i hrm_ins_hndl_ret_ = 0;                                    \
        if (ins_handle_ptr_)                                                   \
        {                                                                      \
            struct ccc_hrtree_handle_ *hrm_ins_hndl_                           \
                = &ins_handle_ptr_->impl_;                                     \
            if (!(hrm_ins_hndl_->handle_.stats_ & CCC_OCCUPIED))               \
            {                                                                  \
                hrm_ins_hndl_ret_                                              \
                    = ccc_impl_hrm_alloc_slot(hrm_ins_hndl_->hrm_);            \
                if (hrm_ins_hndl_ret_)                                         \
                {                                                              \
                    *((typeof(lazy_key_value) *)ccc_buf_at(                    \
                        &hrm_ins_hndl_->hrm_->buf_, hrm_ins_hndl_ret_))        \
                        = lazy_key_value;                                      \
                    ccc_impl_hrm_insert(                                       \
                        hrm_ins_hndl_->hrm_, hrm_ins_hndl_->handle_.i_,        \
                        hrm_ins_hndl_->last_cmp_, hrm_ins_hndl_ret_);          \
                }                                                              \
            }                                                                  \
            else if (hrm_ins_hndl_->handle_.stats_ == CCC_OCCUPIED)            \
            {                                                                  \
                hrm_ins_hndl_ret_ = hrm_ins_hndl_->handle_.i_;                 \
                struct ccc_hromap_elem_ ins_hndl_saved_                        \
                    = *ccc_impl_hrm_elem_at(hrm_ins_hndl_->hrm_,               \
                                            hrm_ins_hndl_ret_);                \
                *((typeof(lazy_key_value) *)ccc_buf_at(                        \
                    &hrm_ins_hndl_->hrm_->buf_, hrm_ins_hndl_ret_))            \
                    = lazy_key_value;                                          \
                *ccc_impl_hrm_elem_at(hrm_ins_hndl_->hrm_, hrm_ins_hndl_ret_)  \
                    = ins_hndl_saved_;                                         \
            }                                                                  \
        }                                                                      \
        hrm_ins_hndl_ret_;                                                     \
    }))

#define ccc_impl_hrm_try_insert_w(handle_realtime_ordered_map_ptr, key,        \
                                  lazy_value...)                               \
    (__extension__({                                                           \
        __auto_type try_ins_map_ptr_ = (handle_realtime_ordered_map_ptr);      \
        struct ccc_handl_ hrm_try_ins_hndl_ret_ = {.stats_ = CCC_INPUT_ERROR}; \
        if (try_ins_map_ptr_)                                                  \
        {                                                                      \
            __auto_type hrm_key_ = (key);                                      \
            struct ccc_hrtree_handle_ hrm_try_ins_hndl_                        \
                = ccc_impl_hrm_handle(try_ins_map_ptr_, (void *)&hrm_key_);    \
            if (!(hrm_try_ins_hndl_.handle_.stats_ & CCC_OCCUPIED))            \
            {                                                                  \
                hrm_try_ins_hndl_ret_ = (struct ccc_handl_){                   \
                    .i_ = ccc_impl_hrm_alloc_slot(hrm_try_ins_hndl_.hrm_),     \
                    .stats_ = CCC_INSERT_ERROR};                               \
                if (hrm_try_ins_hndl_ret_.i_)                                  \
                {                                                              \
                    *((typeof(lazy_value) *)ccc_buf_at(                        \
                        &try_ins_map_ptr_->buf_, hrm_try_ins_hndl_ret_.i_))    \
                        = lazy_value;                                          \
                    *((typeof(hrm_key_) *)ccc_impl_hrm_key_at(                 \
                        try_ins_map_ptr_, hrm_try_ins_hndl_ret_.i_))           \
                        = hrm_key_;                                            \
                    ccc_impl_hrm_insert(hrm_try_ins_hndl_.hrm_,                \
                                        hrm_try_ins_hndl_.handle_.i_,          \
                                        hrm_try_ins_hndl_.last_cmp_,           \
                                        hrm_try_ins_hndl_ret_.i_);             \
                    hrm_try_ins_hndl_ret_.stats_ = CCC_VACANT;                 \
                }                                                              \
            }                                                                  \
            else if (hrm_try_ins_hndl_.handle_.stats_ == CCC_OCCUPIED)         \
            {                                                                  \
                hrm_try_ins_hndl_ret_ = (struct ccc_handl_){                   \
                    .i_ = hrm_try_ins_hndl_.handle_.i_,                        \
                    .stats_ = hrm_try_ins_hndl_.handle_.stats_};               \
            }                                                                  \
        }                                                                      \
        hrm_try_ins_hndl_ret_;                                                 \
    }))

#define ccc_impl_hrm_insert_or_assign_w(handle_realtime_ordered_map_ptr, key,  \
                                        lazy_value...)                         \
    (__extension__({                                                           \
        __auto_type ins_or_assign_map_ptr_                                     \
            = (handle_realtime_ordered_map_ptr);                               \
        struct ccc_handl_ hrm_ins_or_assign_hndl_ret_                          \
            = {.stats_ = CCC_INPUT_ERROR};                                     \
        if (ins_or_assign_map_ptr_)                                            \
        {                                                                      \
            __auto_type hrm_key_ = (key);                                      \
            struct ccc_hrtree_handle_ hrm_ins_or_assign_hndl_                  \
                = ccc_impl_hrm_handle(ins_or_assign_map_ptr_,                  \
                                      (void *)&hrm_key_);                      \
            if (!(hrm_ins_or_assign_hndl_.handle_.stats_ & CCC_OCCUPIED))      \
            {                                                                  \
                hrm_ins_or_assign_hndl_ret_                                    \
                    = (struct ccc_handl_){.i_ = ccc_impl_hrm_alloc_slot(       \
                                              hrm_ins_or_assign_hndl_.hrm_),   \
                                          .stats_ = CCC_INSERT_ERROR};         \
                if (hrm_ins_or_assign_hndl_ret_.i_)                            \
                {                                                              \
                    *((typeof(lazy_value) *)ccc_buf_at(                        \
                        &hrm_ins_or_assign_hndl_.hrm_->buf_,                   \
                        hrm_ins_or_assign_hndl_ret_.i_))                       \
                        = lazy_value;                                          \
                    *((typeof(hrm_key_) *)ccc_impl_hrm_key_at(                 \
                        hrm_ins_or_assign_hndl_.hrm_,                          \
                        hrm_ins_or_assign_hndl_ret_.i_))                       \
                        = hrm_key_;                                            \
                    ccc_impl_hrm_insert(hrm_ins_or_assign_hndl_.hrm_,          \
                                        hrm_ins_or_assign_hndl_.handle_.i_,    \
                                        hrm_ins_or_assign_hndl_.last_cmp_,     \
                                        hrm_ins_or_assign_hndl_ret_.i_);       \
                    hrm_ins_or_assign_hndl_ret_.stats_ = CCC_VACANT;           \
                }                                                              \
            }                                                                  \
            else if (hrm_ins_or_assign_hndl_.handle_.stats_ == CCC_OCCUPIED)   \
            {                                                                  \
                struct ccc_hromap_elem_ ins_hndl_saved_                        \
                    = *ccc_impl_hrm_elem_at(                                   \
                        hrm_ins_or_assign_hndl_.hrm_,                          \
                        hrm_ins_or_assign_hndl_.handle_.i_);                   \
                *((typeof(lazy_value) *)ccc_buf_at(                            \
                    hrm_ins_or_assign_hndl_.hrm_,                              \
                    hrm_ins_or_assign_hndl_.handle_.i_))                       \
                    = lazy_value;                                              \
                *ccc_impl_hrm_elem_at(hrm_ins_or_assign_hndl_.hrm_,            \
                                      hrm_ins_or_assign_hndl_.handle_.i_)      \
                    = ins_hndl_saved_;                                         \
                hrm_ins_or_assign_hndl_ret_ = (struct ccc_handl_){             \
                    .i_ = hrm_ins_or_assign_hndl_.handle_.i_,                  \
                    .stats_ = hrm_ins_or_assign_hndl_.stats_};                 \
                *((typeof(hrm_key_) *)ccc_impl_hrm_key_at(                     \
                    hrm_ins_or_assign_hndl_.hrm_,                              \
                    hrm_ins_or_assign_hndl_.handle_.i_))                       \
                    = hrm_key_;                                                \
            }                                                                  \
        }                                                                      \
        hrm_ins_or_assign_hndl_ret_;                                           \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* IMPL_HANDLE_REALTIME_ORDERED_MAP_H */
