/** @cond
Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
@endcond */
#ifndef IMPL_HANDLE_REALTIME_ORDERED_MAP_H
#define IMPL_HANDLE_REALTIME_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../buffer.h"
#include "../types.h"
#include "impl_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private Runs the standard WAVL tree algorithms with the addition of
a free list. The parent field tracks the parent for an allocated node in the
tree that the user has inserted into the array. When the user removes a node
it is added to the front of a free list. The map will track the first free
node. The list is push to front LIFO stack. */
struct ccc_hromap_elem_
{
    size_t branch_[2]; /** Child nodes in array to unify Left and Right. */
    union
    {
        size_t parent_;    /** Parent of WAVL node when allocated. */
        size_t next_free_; /** Points to next free when not allocated. */
    };
    uint8_t parity_; /** WAVL logic. Instead of rank integer, use 0 or 1. */
};

/** @private A realtime ordered map providing handle stability. Once elements
are inserted into the map they will not move slots even when the array resizes.
The free slots are tracked in a singly linked list that uses indices instead
of pointers so that it remains valid even when the table resizes. The 0th
index of the array is sacrificed for some coding simplicity and falsey 0. */
struct ccc_hromap_
{
    ccc_buffer buf_;          /** The buffer wrapping user memory. */
    size_t root_;             /** The root node of the WAVL tree. */
    size_t free_list_;        /** The start of the free singly linked list. */
    size_t key_offset_;       /** Where user key can be found in type. */
    size_t node_elem_offset_; /** Where intrusive elem is found in type. */
    ccc_any_key_cmp_fn *cmp_; /** The provided key comparison function. */
};

/** @private */
struct ccc_hrtree_handle_
{
    struct ccc_hromap_ *hrm_;   /** Map associated with this handle. */
    ccc_threeway_cmp last_cmp_; /** Saves last comparison direction. */
    struct ccc_handl_ handle_;  /** Index and a status. */
};

/** @private */
union ccc_hromap_handle_
{
    struct ccc_hrtree_handle_ impl_;
};

/*========================  Private Interface  ==============================*/

/** @private */
void *ccc_impl_hrm_key_at(struct ccc_hromap_ const *hrm, size_t slot);
/** @private */
struct ccc_hromap_elem_ *ccc_impl_hrm_elem_at(struct ccc_hromap_ const *hrm,
                                              size_t i);
/** @private */
struct ccc_hrtree_handle_ ccc_impl_hrm_handle(struct ccc_hromap_ const *hrm,
                                              void const *key);
/** @private */
void ccc_impl_hrm_insert(struct ccc_hromap_ *hrm, size_t parent_i,
                         ccc_threeway_cmp last_cmp, size_t elem_i);
/** @private */
size_t ccc_impl_hrm_alloc_slot(struct ccc_hromap_ *hrm);

/*=========================      Initialization     =========================*/

/** @private */
#define ccc_impl_hrm_init(memory_ptr, node_elem_field, key_elem_field,         \
                          key_cmp_fn, alloc_fn, aux_data, capacity)            \
    {                                                                          \
        .buf_ = ccc_buf_init(memory_ptr, alloc_fn, aux_data, capacity),        \
        .root_ = 0,                                                            \
        .free_list_ = 0,                                                       \
        .key_offset_ = offsetof(typeof(*(memory_ptr)), key_elem_field),        \
        .node_elem_offset_ = offsetof(typeof(*(memory_ptr)), node_elem_field), \
        .cmp_ = (key_cmp_fn),                                                  \
    }

/** @private */
#define ccc_impl_hrm_as(handle_realtime_ordered_map_ptr, type_name, handle...) \
    ((type_name *)ccc_buf_at(&(handle_realtime_ordered_map_ptr)->buf_,         \
                             (handle)))

/*==================     Core Macro Implementations     =====================*/

/** @private */
#define ccc_impl_hrm_and_modify_w(handle_realtime_ordered_map_handle_ptr,      \
                                  type_name, closure_over_T...)                \
    (__extension__({                                                           \
        __auto_type hrm_hndl_ptr_ = (handle_realtime_ordered_map_handle_ptr);  \
        struct ccc_hrtree_handle_ hrm_mod_hndl_                                \
            = {.handle_ = {.stats_ = CCC_ENTRY_ARG_ERROR}};                    \
        if (hrm_hndl_ptr_)                                                     \
        {                                                                      \
            hrm_mod_hndl_ = hrm_hndl_ptr_->impl_;                              \
            if (hrm_mod_hndl_.handle_.stats_ & CCC_ENTRY_OCCUPIED)             \
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

/** @private */
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
            if (hrm_or_ins_hndl_->handle_.stats_ == CCC_ENTRY_OCCUPIED)        \
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

/** @private */
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
            if (!(hrm_ins_hndl_->handle_.stats_ & CCC_ENTRY_OCCUPIED))         \
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
            else if (hrm_ins_hndl_->handle_.stats_ == CCC_ENTRY_OCCUPIED)      \
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

/** @private */
#define ccc_impl_hrm_try_insert_w(handle_realtime_ordered_map_ptr, key,        \
                                  lazy_value...)                               \
    (__extension__({                                                           \
        __auto_type try_ins_map_ptr_ = (handle_realtime_ordered_map_ptr);      \
        struct ccc_handl_ hrm_try_ins_hndl_ret_                                \
            = {.stats_ = CCC_ENTRY_ARG_ERROR};                                 \
        if (try_ins_map_ptr_)                                                  \
        {                                                                      \
            __auto_type hrm_key_ = (key);                                      \
            struct ccc_hrtree_handle_ hrm_try_ins_hndl_                        \
                = ccc_impl_hrm_handle(try_ins_map_ptr_, (void *)&hrm_key_);    \
            if (!(hrm_try_ins_hndl_.handle_.stats_ & CCC_ENTRY_OCCUPIED))      \
            {                                                                  \
                hrm_try_ins_hndl_ret_ = (struct ccc_handl_){                   \
                    .i_ = ccc_impl_hrm_alloc_slot(hrm_try_ins_hndl_.hrm_),     \
                    .stats_ = CCC_ENTRY_INSERT_ERROR};                         \
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
                    hrm_try_ins_hndl_ret_.stats_ = CCC_ENTRY_VACANT;           \
                }                                                              \
            }                                                                  \
            else if (hrm_try_ins_hndl_.handle_.stats_ == CCC_ENTRY_OCCUPIED)   \
            {                                                                  \
                hrm_try_ins_hndl_ret_ = (struct ccc_handl_){                   \
                    .i_ = hrm_try_ins_hndl_.handle_.i_,                        \
                    .stats_ = hrm_try_ins_hndl_.handle_.stats_};               \
            }                                                                  \
        }                                                                      \
        hrm_try_ins_hndl_ret_;                                                 \
    }))

/** @private */
#define ccc_impl_hrm_insert_or_assign_w(handle_realtime_ordered_map_ptr, key,  \
                                        lazy_value...)                         \
    (__extension__({                                                           \
        __auto_type ins_or_assign_map_ptr_                                     \
            = (handle_realtime_ordered_map_ptr);                               \
        struct ccc_handl_ hrm_ins_or_assign_hndl_ret_                          \
            = {.stats_ = CCC_ENTRY_ARG_ERROR};                                 \
        if (ins_or_assign_map_ptr_)                                            \
        {                                                                      \
            __auto_type hrm_key_ = (key);                                      \
            struct ccc_hrtree_handle_ hrm_ins_or_assign_hndl_                  \
                = ccc_impl_hrm_handle(ins_or_assign_map_ptr_,                  \
                                      (void *)&hrm_key_);                      \
            if (!(hrm_ins_or_assign_hndl_.handle_.stats_                       \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                hrm_ins_or_assign_hndl_ret_                                    \
                    = (struct ccc_handl_){.i_ = ccc_impl_hrm_alloc_slot(       \
                                              hrm_ins_or_assign_hndl_.hrm_),   \
                                          .stats_ = CCC_ENTRY_INSERT_ERROR};   \
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
                    hrm_ins_or_assign_hndl_ret_.stats_ = CCC_ENTRY_VACANT;     \
                }                                                              \
            }                                                                  \
            else if (hrm_ins_or_assign_hndl_.handle_.stats_                    \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct ccc_hromap_elem_ ins_hndl_saved_                        \
                    = *ccc_impl_hrm_elem_at(                                   \
                        hrm_ins_or_assign_hndl_.hrm_,                          \
                        hrm_ins_or_assign_hndl_.handle_.i_);                   \
                *((typeof(lazy_value) *)ccc_buf_at(                            \
                    &hrm_ins_or_assign_hndl_.hrm_->buf_,                       \
                    hrm_ins_or_assign_hndl_.handle_.i_))                       \
                    = lazy_value;                                              \
                *ccc_impl_hrm_elem_at(hrm_ins_or_assign_hndl_.hrm_,            \
                                      hrm_ins_or_assign_hndl_.handle_.i_)      \
                    = ins_hndl_saved_;                                         \
                hrm_ins_or_assign_hndl_ret_ = (struct ccc_handl_){             \
                    .i_ = hrm_ins_or_assign_hndl_.handle_.i_,                  \
                    .stats_ = hrm_ins_or_assign_hndl_.handle_.stats_};         \
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
