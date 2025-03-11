#ifndef CCC_IMPL_HANDLE_ORDERED_MAP_H
#define CCC_IMPL_HANDLE_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../buffer.h"
#include "../types.h"
#include "impl_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private Runs the top down splay tree algorithm with the addition of a free
list for providing new nodes within the buffer. The parent field normally
tracks parent when in the tree for iteration purposes. When a node is removed
from the tree it is added to the free singly linked list. The free list is a
LIFO push to front stack. */
struct ccc_homap_elem_
{
    size_t branch_[2]; /** Child nodes in array to unify Left and Right. */
    union
    {
        size_t parent_;    /** Parent of splay tree node when allocated. */
        size_t next_free_; /** Points to next free when not allocated. */
    };
};

/** @private A ordered map providing handle stability. Once elements are
inserted into the map they will not move slots even when the array resizes. The
free slots are tracked in a singly linked list that uses indices instead of
pointers so that it remains valid even when the table resizes. The 0th index of
the array is sacrificed for some coding simplicity and falsey 0. */
struct ccc_homap_
{
    ccc_buffer buf_;          /** Buffer wrapping user provided memory. */
    size_t root_;             /** The root node of the Splay Tree. */
    size_t free_list_;        /** The start of the free singly linked list. */
    size_t key_offset_;       /** Where user key can be found in type. */
    size_t node_elem_offset_; /** Where intrusive elem is found in type. */
    ccc_key_cmp_fn *cmp_;     /** The provided key comparison function. */
};

/** @private */
struct ccc_htree_handle_
{
    struct ccc_homap_ *hom_;    /** Map associated with this handle. */
    ccc_threeway_cmp last_cmp_; /** Saves last comparison direction. */
    struct ccc_handl_ handle_;  /** Index and a status. */
};

/** @private */
union ccc_homap_handle_
{
    struct ccc_htree_handle_ impl_;
};

/*===========================   Private Interface ===========================*/

/** @private */
void ccc_impl_hom_insert(struct ccc_homap_ *hom, size_t elem_i);
/** @private */
struct ccc_htree_handle_ ccc_impl_hom_handle(struct ccc_homap_ *hom,
                                             void const *key);
/** @private */
void *ccc_impl_hom_key_at(struct ccc_homap_ const *hom, size_t slot);
/** @private */
struct ccc_homap_elem_ *ccc_impl_homap_elem_at(struct ccc_homap_ const *hom,
                                               size_t slot);
/** @private */
size_t ccc_impl_hom_alloc_slot(struct ccc_homap_ *hom);

/*========================     Initialization       =========================*/

/** @private */
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

/** @private */
#define ccc_impl_hom_as(handle_ordered_map_ptr, type_name, handle...)          \
    ((type_name *)ccc_buf_at(&(handle_ordered_map_ptr)->buf_, (handle)))

/*==================     Core Macro Implementations     =====================*/

/** @private */
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

/** @private */
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

/** @private */
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

/** @private */
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

/** @private */
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
