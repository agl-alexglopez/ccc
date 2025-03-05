#ifndef CCC_IMPL_HANDLE_ORDERED_MAP_H
#define CCC_IMPL_HANDLE_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../buffer.h"
#include "../types.h"
#include "impl_types.h"

/** @private */
struct ccc_homap_elem_
{
    size_t branch_[2];
    union
    {
        size_t parent_;
        size_t next_free_;
    };
};

/** @private */
struct ccc_homap_
{
    ccc_buffer buf_;
    size_t root_;
    size_t free_list_;
    size_t key_offset_;
    size_t node_elem_offset_;
    ccc_key_cmp_fn *cmp_;
};

/** @private */
struct ccc_htree_handle_
{
    struct ccc_homap_ *hom_;
    ccc_threeway_cmp last_cmp_;
    struct ccc_handl_ handle_;
};

/** @private */
union ccc_homap_handle_
{
    struct ccc_htree_handle_ impl_;
};

#define ccc_impl_hom_init(mem_ptr, node_elem_field, key_elem_field,            \
                          key_cmp_fn, alloc_fn, aux_data, capacity)            \
    {                                                                          \
        .buf_ = ccc_buf_init(mem_ptr, alloc_fn, aux_data, capacity),           \
        .root_ = 0,                                                            \
        .free_list_ = 0,                                                       \
        .key_offset_ = offsetof(typeof(*(mem_ptr)), key_elem_field),           \
        .node_elem_offset_ = offsetof(typeof(*(mem_ptr)), node_elem_field),    \
        .cmp_ = (key_cmp_fn),                                                  \
    }

#define ccc_impl_hom_as(handle_ordered_map_ptr, type_name, handle...)          \
    ((type_name *)ccc_buf_at(&(handle_ordered_map_ptr)->buf_, (handle)))

void ccc_impl_hom_insert(struct ccc_homap_ *hom, size_t elem_i);
struct ccc_htree_handle_ ccc_impl_hom_handle(struct ccc_homap_ *hom,
                                             void const *key);
void *ccc_impl_hom_key_from_node(struct ccc_homap_ const *hom,
                                 struct ccc_homap_elem_ const *elem);
void *ccc_impl_hom_key_at(struct ccc_homap_ const *hom, size_t slot);
struct ccc_homap_elem_ *ccc_impl_homap_elem_at(struct ccc_homap_ const *hom,
                                               size_t slot);
size_t ccc_impl_hom_alloc_slot(struct ccc_homap_ *hom);

/* NOLINTBEGIN(readability-identifier-naming) */

/*==================     Core Macro Implementations     =====================*/

#define ccc_impl_hom_and_modify_w(handle_ordered_map_handle_ptr, type_name,    \
                                  closure_over_T...)                           \
    (__extension__({                                                           \
        __auto_type hom_mod_hndl_ptr_ = (handle_ordered_map_handle_ptr);       \
        struct ccc_htree_handle_ hom_mod_hndl_                                 \
            = {.handle_ = {.stats_ = CCC_INPUT_ERROR}};                        \
        if (hom_mod_hndl_ptr_)                                                 \
        {                                                                      \
            hom_mod_hndl_ = hom_mod_hndl_ptr_->impl_;                          \
            if (hom_mod_hndl_.handle_.stats_ & CCC_OCCUPIED)                   \
            {                                                                  \
                type_name *const T = ccc_buf_at(&hom_mod_hndl_.hom_->buf_,     \
                                                hom_mod_hndl_.handle_.i_);     \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        hom_mod_hndl_;                                                         \
    }))

#define ccc_impl_hom_or_insert_w(handle_ordered_map_handle_ptr,                \
                                 lazy_key_value...)                            \
    (__extension__({                                                           \
        __auto_type hom_or_ins_hndl_ptr_ = (handle_ordered_map_handle_ptr);    \
        ccc_handle_i hom_or_ins_ret_ = 0;                                      \
        if (hom_or_ins_hndl_ptr_)                                              \
        {                                                                      \
            struct ccc_htree_handle_ *hom_or_ins_hndl_                         \
                = &hom_or_ins_hndl_ptr_->impl_;                                \
            if (hom_or_ins_hndl_->handle_.stats_ == CCC_OCCUPIED)              \
            {                                                                  \
                hom_or_ins_ret_ = hom_or_ins_hndl_->handle_.i_;                \
            }                                                                  \
            else                                                               \
            {                                                                  \
                hom_or_ins_ret_                                                \
                    = ccc_impl_hom_alloc_slot(hom_or_ins_hndl_->hom_);         \
                if (hom_or_ins_ret_)                                           \
                {                                                              \
                    *((typeof(lazy_key_value) *)ccc_buf_at(                    \
                        &hom_or_ins_hndl_->hom_->buf_, hom_or_ins_ret_))       \
                        = lazy_key_value;                                      \
                    ccc_impl_hom_insert(hom_or_ins_hndl_->hom_,                \
                                        hom_or_ins_ret_);                      \
                }                                                              \
            }                                                                  \
        }                                                                      \
        hom_or_ins_ret_;                                                       \
    }))

#define ccc_impl_hom_insert_handle_w(handle_ordered_map_handle_ptr,            \
                                     lazy_key_value...)                        \
    (__extension__({                                                           \
        __auto_type hom_ins_hndl_ptr_ = (handle_ordered_map_handle_ptr);       \
        ccc_handle_i hom_ins_hndl_ret_ = 0;                                    \
        if (hom_ins_hndl_ptr_)                                                 \
        {                                                                      \
            struct ccc_htree_handle_ *hom_ins_hndl_                            \
                = &hom_ins_hndl_ptr_->impl_;                                   \
            if (!(hom_ins_hndl_->handle_.stats_ & CCC_OCCUPIED))               \
            {                                                                  \
                hom_ins_hndl_ret_                                              \
                    = ccc_impl_hom_alloc_slot(hom_ins_hndl_->hom_);            \
                if (hom_ins_hndl_ret_)                                         \
                {                                                              \
                    *((typeof(lazy_key_value) *)ccc_buf_at(                    \
                        &hom_ins_hndl_->hom_->buf_, hom_ins_hndl_ret_))        \
                        = lazy_key_value;                                      \
                    ccc_impl_hom_insert(hom_ins_hndl_->hom_,                   \
                                        hom_ins_hndl_ret_);                    \
                }                                                              \
            }                                                                  \
            else if (hom_ins_hndl_->handle_.stats_ == CCC_OCCUPIED)            \
            {                                                                  \
                hom_ins_hndl_ret_ = hom_ins_hndl_->handle_.i_;                 \
                struct ccc_homap_elem_ ins_hndl_saved_                         \
                    = *ccc_impl_homap_elem_at(hom_ins_hndl_->hom_,             \
                                              hom_ins_hndl_ret_);              \
                *((typeof(lazy_key_value) *)ccc_buf_at(                        \
                    &hom_ins_hndl_->hom_->buf_, hom_ins_hndl_ret_))            \
                    = lazy_key_value;                                          \
                *ccc_impl_homap_elem_at(hom_ins_hndl_->hom_,                   \
                                        hom_ins_hndl_ret_)                     \
                    = ins_hndl_saved_;                                         \
            }                                                                  \
        }                                                                      \
        hom_ins_hndl_ret_;                                                     \
    }))

#define ccc_impl_hom_try_insert_w(handle_ordered_map_ptr, key, lazy_value...)  \
    (__extension__({                                                           \
        __auto_type hom_try_ins_map_ptr_ = (handle_ordered_map_ptr);           \
        struct ccc_handl_ hom_try_ins_hndl_ret_ = {.stats_ = CCC_INPUT_ERROR}; \
        if (hom_try_ins_map_ptr_)                                              \
        {                                                                      \
            __auto_type hom_key_ = (key);                                      \
            struct ccc_htree_handle_ hom_try_ins_hndl_ = ccc_impl_hom_handle(  \
                hom_try_ins_map_ptr_, (void *)&hom_key_);                      \
            if (!(hom_try_ins_hndl_.handle_.stats_ & CCC_OCCUPIED))            \
            {                                                                  \
                hom_try_ins_hndl_ret_ = (struct ccc_handl_){                   \
                    .i_ = ccc_impl_hom_alloc_slot(hom_try_ins_hndl_.hom_),     \
                    .stats_ = CCC_INSERT_ERROR};                               \
                if (hom_try_ins_hndl_ret_.i_)                                  \
                {                                                              \
                    *((typeof(lazy_value) *)ccc_buf_at(                        \
                        &hom_try_ins_map_ptr_->buf_,                           \
                        hom_try_ins_hndl_ret_.i_))                             \
                        = lazy_value;                                          \
                    *((typeof(hom_key_) *)ccc_impl_hom_key_at(                 \
                        hom_try_ins_hndl_.hom_, hom_try_ins_hndl_ret_.i_))     \
                        = hom_key_;                                            \
                    ccc_impl_hom_insert(hom_try_ins_hndl_.hom_,                \
                                        hom_try_ins_hndl_ret_.i_);             \
                    hom_try_ins_hndl_ret_.stats_ = CCC_VACANT;                 \
                }                                                              \
            }                                                                  \
            else if (hom_try_ins_hndl_.handle_.stats_ == CCC_OCCUPIED)         \
            {                                                                  \
                hom_try_ins_hndl_ret_ = (struct ccc_handl_){                   \
                    .i_ = hom_try_ins_hndl_.handle_.i_,                        \
                    .stats_ = hom_try_ins_hndl_.handle_.stats_};               \
            }                                                                  \
        }                                                                      \
        hom_try_ins_hndl_ret_;                                                 \
    }))

#define ccc_impl_hom_insert_or_assign_w(handle_ordered_map_ptr, key,           \
                                        lazy_value...)                         \
    (__extension__({                                                           \
        __auto_type hom_ins_or_assign_map_ptr_ = (handle_ordered_map_ptr);     \
        struct ccc_handl_ hom_ins_or_assign_hndl_ret_                          \
            = {.stats_ = CCC_INPUT_ERROR};                                     \
        if (hom_ins_or_assign_map_ptr_)                                        \
        {                                                                      \
            __auto_type hom_key_ = (key);                                      \
            struct ccc_htree_handle_ hom_ins_or_assign_hndl_                   \
                = ccc_impl_hom_handle((handle_ordered_map_ptr),                \
                                      (void *)&hom_key_);                      \
            if (!(hom_ins_or_assign_hndl_.handle_.stats_ & CCC_OCCUPIED))      \
            {                                                                  \
                hom_ins_or_assign_hndl_ret_                                    \
                    = (struct ccc_handl_){.i_ = ccc_impl_hom_alloc_slot(       \
                                              hom_ins_or_assign_hndl_.hom_),   \
                                          .stats_ = CCC_INSERT_ERROR};         \
                if (hom_ins_or_assign_hndl_ret_.i_)                            \
                {                                                              \
                    *((typeof(lazy_value) *)ccc_buf_at(                        \
                        &hom_ins_or_assign_map_ptr_->buf_,                     \
                        hom_ins_or_assign_hndl_ret_.i_))                       \
                        = lazy_value;                                          \
                    *((typeof(hom_key_) *)ccc_impl_hom_key_at(                 \
                        hom_ins_or_assign_hndl_.hom_,                          \
                        hom_ins_or_assign_hndl_ret_.i_))                       \
                        = hom_key_;                                            \
                    ccc_impl_hom_insert(hom_ins_or_assign_hndl_.hom_,          \
                                        hom_ins_or_assign_hndl_ret_.i_);       \
                    hom_ins_or_assign_hndl_ret_.stats_ = CCC_VACANT;           \
                }                                                              \
            }                                                                  \
            else if (hom_ins_or_assign_hndl_.handle_.stats_ == CCC_OCCUPIED)   \
            {                                                                  \
                struct ccc_homap_elem_ ins_hndl_saved_                         \
                    = *ccc_impl_homap_elem_at(                                 \
                        hom_ins_or_assign_hndl_.hom_,                          \
                        hom_ins_or_assign_hndl_.handle_.i_);                   \
                *((typeof(lazy_value) *)ccc_buf_at(                            \
                    &hom_ins_or_assign_hndl_.hom_->buf_,                       \
                    hom_ins_or_assign_hndl_.handle_.i_))                       \
                    = lazy_value;                                              \
                *ccc_impl_homap_elem_at(hom_ins_or_assign_hndl_.hom_,          \
                                        hom_ins_or_assign_hndl_.handle_.i_)    \
                    = ins_hndl_saved_;                                         \
                hom_ins_or_assign_hndl_ret_ = (struct ccc_handl_){             \
                    .i_ = hom_ins_or_assign_hndl_.handle_.i_,                  \
                    .stats_ = hom_ins_or_assign_hndl_.handle_.stats_};         \
                *((typeof(hom_key_) *)ccc_impl_hom_key_at(                     \
                    hom_ins_or_assign_hndl_.hom_,                              \
                    hom_ins_or_assign_hndl_.handle_.i_))                       \
                    = hom_key_;                                                \
            }                                                                  \
        }                                                                      \
        hom_ins_or_assign_hndl_ret_;                                           \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_HANDLE_ORDERED_MAP_H */
