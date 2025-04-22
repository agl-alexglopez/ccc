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
struct ccc_hromap_elem
{
    /** @private Child nodes in array to unify Left and Right. */
    size_t branch[2];
    union
    {
        /** @private Parent of WAVL node when allocated. */
        size_t parent;
        /** @private Points to next free when not allocated. */
        size_t next_free;
    };
    /** @private WAVL logic. Instead of rank integer, use 0 or 1. */
    uint8_t parity;
};

/** @private A realtime ordered map providing handle stability. Once elements
are inserted into the map they will not move slots even when the array resizes.
The free slots are tracked in a singly linked list that uses indices instead
of pointers so that it remains valid even when the table resizes. The 0th
index of the array is sacrificed for some coding simplicity and falsey 0. */
struct ccc_hromap
{
    /** @private The buffer wrapping user memory. */
    ccc_buffer buf;
    /** @private The root node of the WAVL tree. */
    size_t root;
    /** @private The start of the free singly linked list. */
    size_t free_list;
    /** @private Where user key can be found in type. */
    size_t key_offset;
    /** @private Where intrusive elem is found in type. */
    size_t node_elem_offset;
    /** @private The provided key comparison function. */
    ccc_any_key_cmp_fn *cmp;
};

/** @private */
struct ccc_hrtree_handle
{
    /** @private Map associated with this handle. */
    struct ccc_hromap *hrm;
    /** @private Saves last comparison direction. */
    ccc_threeway_cmp last_cmp;
    /** @private Index and a status. */
    struct ccc_handl handle;
};

/** @private Wrapper for return by pointer on the stack in C23. */
union ccc_hromap_handle
{
    /** @private Single field to enable return by compound literal reference. */
    struct ccc_hrtree_handle impl;
};

/*========================  Private Interface  ==============================*/

/** @private */
void *ccc_impl_hrm_key_at(struct ccc_hromap const *hrm, size_t slot);
/** @private */
struct ccc_hromap_elem *ccc_impl_hrm_elem_at(struct ccc_hromap const *hrm,
                                             size_t i);
/** @private */
struct ccc_hrtree_handle ccc_impl_hrm_handle(struct ccc_hromap const *hrm,
                                             void const *key);
/** @private */
void ccc_impl_hrm_insert(struct ccc_hromap *hrm, size_t parent_i,
                         ccc_threeway_cmp last_cmp, size_t elem_i);
/** @private */
size_t ccc_impl_hrm_alloc_slot(struct ccc_hromap *hrm);

/*=========================      Initialization     =========================*/

/** @private */
#define ccc_impl_hrm_init(impl_memory_ptr, impl_node_elem_field,               \
                          impl_key_elem_field, impl_key_cmp_fn, impl_alloc_fn, \
                          impl_aux_data, impl_capacity)                        \
    {                                                                          \
        .buf = ccc_buf_init(impl_memory_ptr, impl_alloc_fn, impl_aux_data,     \
                            impl_capacity),                                    \
        .root = 0,                                                             \
        .free_list = 0,                                                        \
        .key_offset                                                            \
        = offsetof(typeof(*(impl_memory_ptr)), impl_key_elem_field),           \
        .node_elem_offset                                                      \
        = offsetof(typeof(*(impl_memory_ptr)), impl_node_elem_field),          \
        .cmp = (impl_key_cmp_fn),                                              \
    }

/** @private */
#define ccc_impl_hrm_as(handle_realtime_ordered_map_ptr, type_name, handle...) \
    ((type_name *)ccc_buf_at(&(handle_realtime_ordered_map_ptr)->buf, (handle)))

/*==================     Core Macro Implementations     =====================*/

/** @private */
#define ccc_impl_hrm_and_modify_w(handle_realtime_ordered_map_handle_ptr,      \
                                  type_name, closure_over_T...)                \
    (__extension__({                                                           \
        __auto_type impl_hrm_hndl_ptr                                          \
            = (handle_realtime_ordered_map_handle_ptr);                        \
        struct ccc_hrtree_handle impl_hrm_mod_hndl                             \
            = {.handle = {.stats = CCC_ENTRY_ARG_ERROR}};                      \
        if (impl_hrm_hndl_ptr)                                                 \
        {                                                                      \
            impl_hrm_mod_hndl = impl_hrm_hndl_ptr->impl;                       \
            if (impl_hrm_mod_hndl.handle.stats & CCC_ENTRY_OCCUPIED)           \
            {                                                                  \
                type_name *const T = ccc_buf_at(&impl_hrm_mod_hndl.hrm->buf,   \
                                                impl_hrm_mod_hndl.handle.i);   \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_hrm_mod_hndl;                                                     \
    }))

/** @private */
#define ccc_impl_hrm_or_insert_w(handle_realtime_ordered_map_handle_ptr,       \
                                 lazy_key_value...)                            \
    (__extension__({                                                           \
        __auto_type impl_or_ins_handle_ptr                                     \
            = (handle_realtime_ordered_map_handle_ptr);                        \
        ccc_handle_i impl_hrm_or_ins_ret = 0;                                  \
        if (impl_or_ins_handle_ptr)                                            \
        {                                                                      \
            struct ccc_hrtree_handle *impl_hrm_or_ins_hndl                     \
                = &impl_or_ins_handle_ptr->impl;                               \
            if (impl_hrm_or_ins_hndl->handle.stats == CCC_ENTRY_OCCUPIED)      \
            {                                                                  \
                impl_hrm_or_ins_ret = impl_hrm_or_ins_hndl->handle.i;          \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_hrm_or_ins_ret                                            \
                    = ccc_impl_hrm_alloc_slot(impl_hrm_or_ins_hndl->hrm);      \
                if (impl_hrm_or_ins_ret)                                       \
                {                                                              \
                    *((typeof(lazy_key_value) *)ccc_buf_at(                    \
                        &impl_hrm_or_ins_hndl->hrm->buf, impl_hrm_or_ins_ret)) \
                        = lazy_key_value;                                      \
                    ccc_impl_hrm_insert(impl_hrm_or_ins_hndl->hrm,             \
                                        impl_hrm_or_ins_hndl->handle.i,        \
                                        impl_hrm_or_ins_hndl->last_cmp,        \
                                        impl_hrm_or_ins_ret);                  \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_hrm_or_ins_ret;                                                   \
    }))

/** @private */
#define ccc_impl_hrm_insert_handle_w(handle_realtime_ordered_map_handle_ptr,   \
                                     lazy_key_value...)                        \
    (__extension__({                                                           \
        __auto_type impl_ins_handle_ptr                                        \
            = (handle_realtime_ordered_map_handle_ptr);                        \
        ccc_handle_i impl_hrm_ins_hndl_ret = 0;                                \
        if (impl_ins_handle_ptr)                                               \
        {                                                                      \
            struct ccc_hrtree_handle *impl_hrm_ins_hndl                        \
                = &impl_ins_handle_ptr->impl;                                  \
            if (!(impl_hrm_ins_hndl->handle.stats & CCC_ENTRY_OCCUPIED))       \
            {                                                                  \
                impl_hrm_ins_hndl_ret                                          \
                    = ccc_impl_hrm_alloc_slot(impl_hrm_ins_hndl->hrm);         \
                if (impl_hrm_ins_hndl_ret)                                     \
                {                                                              \
                    *((typeof(lazy_key_value) *)ccc_buf_at(                    \
                        &impl_hrm_ins_hndl->hrm->buf, impl_hrm_ins_hndl_ret))  \
                        = lazy_key_value;                                      \
                    ccc_impl_hrm_insert(                                       \
                        impl_hrm_ins_hndl->hrm, impl_hrm_ins_hndl->handle.i,   \
                        impl_hrm_ins_hndl->last_cmp, impl_hrm_ins_hndl_ret);   \
                }                                                              \
            }                                                                  \
            else if (impl_hrm_ins_hndl->handle.stats == CCC_ENTRY_OCCUPIED)    \
            {                                                                  \
                impl_hrm_ins_hndl_ret = impl_hrm_ins_hndl->handle.i;           \
                struct ccc_hromap_elem impl_ins_hndl_saved                     \
                    = *ccc_impl_hrm_elem_at(impl_hrm_ins_hndl->hrm,            \
                                            impl_hrm_ins_hndl_ret);            \
                *((typeof(lazy_key_value) *)ccc_buf_at(                        \
                    &impl_hrm_ins_hndl->hrm->buf, impl_hrm_ins_hndl_ret))      \
                    = lazy_key_value;                                          \
                *ccc_impl_hrm_elem_at(impl_hrm_ins_hndl->hrm,                  \
                                      impl_hrm_ins_hndl_ret)                   \
                    = impl_ins_hndl_saved;                                     \
            }                                                                  \
        }                                                                      \
        impl_hrm_ins_hndl_ret;                                                 \
    }))

/** @private */
#define ccc_impl_hrm_try_insert_w(handle_realtime_ordered_map_ptr, key,        \
                                  lazy_value...)                               \
    (__extension__({                                                           \
        __auto_type impl_try_ins_map_ptr = (handle_realtime_ordered_map_ptr);  \
        struct ccc_handl impl_hrm_try_ins_hndl_ret                             \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_try_ins_map_ptr)                                              \
        {                                                                      \
            __auto_type impl_hrm_key = (key);                                  \
            struct ccc_hrtree_handle impl_hrm_try_ins_hndl                     \
                = ccc_impl_hrm_handle(impl_try_ins_map_ptr,                    \
                                      (void *)&impl_hrm_key);                  \
            if (!(impl_hrm_try_ins_hndl.handle.stats & CCC_ENTRY_OCCUPIED))    \
            {                                                                  \
                impl_hrm_try_ins_hndl_ret = (struct ccc_handl){                \
                    .i = ccc_impl_hrm_alloc_slot(impl_hrm_try_ins_hndl.hrm),   \
                    .stats = CCC_ENTRY_INSERT_ERROR,                           \
                };                                                             \
                if (impl_hrm_try_ins_hndl_ret.i)                               \
                {                                                              \
                    *((typeof(lazy_value) *)ccc_buf_at(                        \
                        &impl_try_ins_map_ptr->buf,                            \
                        impl_hrm_try_ins_hndl_ret.i))                          \
                        = lazy_value;                                          \
                    *((typeof(impl_hrm_key) *)ccc_impl_hrm_key_at(             \
                        impl_try_ins_map_ptr, impl_hrm_try_ins_hndl_ret.i))    \
                        = impl_hrm_key;                                        \
                    ccc_impl_hrm_insert(impl_hrm_try_ins_hndl.hrm,             \
                                        impl_hrm_try_ins_hndl.handle.i,        \
                                        impl_hrm_try_ins_hndl.last_cmp,        \
                                        impl_hrm_try_ins_hndl_ret.i);          \
                    impl_hrm_try_ins_hndl_ret.stats = CCC_ENTRY_VACANT;        \
                }                                                              \
            }                                                                  \
            else if (impl_hrm_try_ins_hndl.handle.stats == CCC_ENTRY_OCCUPIED) \
            {                                                                  \
                impl_hrm_try_ins_hndl_ret = (struct ccc_handl){                \
                    .i = impl_hrm_try_ins_hndl.handle.i,                       \
                    .stats = impl_hrm_try_ins_hndl.handle.stats,               \
                };                                                             \
            }                                                                  \
        }                                                                      \
        impl_hrm_try_ins_hndl_ret;                                             \
    }))

/** @private */
#define ccc_impl_hrm_insert_or_assign_w(handle_realtime_ordered_map_ptr, key,  \
                                        lazy_value...)                         \
    (__extension__({                                                           \
        __auto_type impl_ins_or_assign_map_ptr                                 \
            = (handle_realtime_ordered_map_ptr);                               \
        struct ccc_handl impl_hrm_ins_or_assign_hndl_ret                       \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_ins_or_assign_map_ptr)                                        \
        {                                                                      \
            __auto_type impl_hrm_key = (key);                                  \
            struct ccc_hrtree_handle impl_hrm_ins_or_assign_hndl               \
                = ccc_impl_hrm_handle(impl_ins_or_assign_map_ptr,              \
                                      (void *)&impl_hrm_key);                  \
            if (!(impl_hrm_ins_or_assign_hndl.handle.stats                     \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                impl_hrm_ins_or_assign_hndl_ret                                \
                    = (struct ccc_handl){.i = ccc_impl_hrm_alloc_slot(         \
                                             impl_hrm_ins_or_assign_hndl.hrm), \
                                         .stats = CCC_ENTRY_INSERT_ERROR};     \
                if (impl_hrm_ins_or_assign_hndl_ret.i)                         \
                {                                                              \
                    *((typeof(lazy_value) *)ccc_buf_at(                        \
                        &impl_hrm_ins_or_assign_hndl.hrm->buf,                 \
                        impl_hrm_ins_or_assign_hndl_ret.i))                    \
                        = lazy_value;                                          \
                    *((typeof(impl_hrm_key) *)ccc_impl_hrm_key_at(             \
                        impl_hrm_ins_or_assign_hndl.hrm,                       \
                        impl_hrm_ins_or_assign_hndl_ret.i))                    \
                        = impl_hrm_key;                                        \
                    ccc_impl_hrm_insert(impl_hrm_ins_or_assign_hndl.hrm,       \
                                        impl_hrm_ins_or_assign_hndl.handle.i,  \
                                        impl_hrm_ins_or_assign_hndl.last_cmp,  \
                                        impl_hrm_ins_or_assign_hndl_ret.i);    \
                    impl_hrm_ins_or_assign_hndl_ret.stats = CCC_ENTRY_VACANT;  \
                }                                                              \
            }                                                                  \
            else if (impl_hrm_ins_or_assign_hndl.handle.stats                  \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct ccc_hromap_elem impl_ins_hndl_saved                     \
                    = *ccc_impl_hrm_elem_at(                                   \
                        impl_hrm_ins_or_assign_hndl.hrm,                       \
                        impl_hrm_ins_or_assign_hndl.handle.i);                 \
                *((typeof(lazy_value) *)ccc_buf_at(                            \
                    &impl_hrm_ins_or_assign_hndl.hrm->buf,                     \
                    impl_hrm_ins_or_assign_hndl.handle.i))                     \
                    = lazy_value;                                              \
                *ccc_impl_hrm_elem_at(impl_hrm_ins_or_assign_hndl.hrm,         \
                                      impl_hrm_ins_or_assign_hndl.handle.i)    \
                    = impl_ins_hndl_saved;                                     \
                impl_hrm_ins_or_assign_hndl_ret = (struct ccc_handl){          \
                    .i = impl_hrm_ins_or_assign_hndl.handle.i,                 \
                    .stats = impl_hrm_ins_or_assign_hndl.handle.stats};        \
                *((typeof(impl_hrm_key) *)ccc_impl_hrm_key_at(                 \
                    impl_hrm_ins_or_assign_hndl.hrm,                           \
                    impl_hrm_ins_or_assign_hndl.handle.i))                     \
                    = impl_hrm_key;                                            \
            }                                                                  \
        }                                                                      \
        impl_hrm_ins_or_assign_hndl_ret;                                       \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* IMPL_HANDLE_REALTIME_ORDERED_MAP_H */
