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
struct ccc_homap_elem
{
    size_t branch[2]; /** Child nodes in array to unify Left and Right. */
    union
    {
        size_t parent;    /** Parent of splay tree node when allocated. */
        size_t next_free; /** Points to next free when not allocated. */
    };
};

/** @private A ordered map providing handle stability. Once elements are
inserted into the map they will not move slots even when the array resizes. The
free slots are tracked in a singly linked list that uses indices instead of
pointers so that it remains valid even when the table resizes. The 0th index of
the array is sacrificed for some coding simplicity and falsey 0. */
struct ccc_homap
{
    ccc_buffer buf;          /** Buffer wrapping user provided memory. */
    size_t root;             /** The root node of the Splay Tree. */
    size_t free_list;        /** The start of the free singly linked list. */
    size_t key_offset;       /** Where user key can be found in type. */
    size_t node_elem_offset; /** Where intrusive elem is found in type. */
    ccc_any_key_cmp_fn *cmp; /** The provided key comparison function. */
};

/** @private */
struct ccc_htree_handle
{
    struct ccc_homap *hom;     /** Map associated with this handle. */
    ccc_threeway_cmp last_cmp; /** Saves last comparison direction. */
    struct ccc_handl handle;   /** Index and a status. */
};

/** @private */
union ccc_homap_handle
{
    struct ccc_htree_handle impl;
};

/*===========================   Private Interface ===========================*/

/** @private */
void ccc_impl_hom_insert(struct ccc_homap *hom, size_t elem_i);
/** @private */
struct ccc_htree_handle ccc_impl_hom_handle(struct ccc_homap *hom,
                                            void const *key);
/** @private */
void *ccc_impl_hom_key_at(struct ccc_homap const *hom, size_t slot);
/** @private */
struct ccc_homap_elem *ccc_impl_homap_elem_at(struct ccc_homap const *hom,
                                              size_t slot);
/** @private */
size_t ccc_impl_hom_alloc_slot(struct ccc_homap *hom);

/*========================     Initialization       =========================*/

/** @private */
#define ccc_impl_hom_init(impl_mem_ptr, impl_node_elem_field,                  \
                          impl_key_elem_field, impl_key_cmp_fn, impl_alloc_fn, \
                          impl_aux_data, impl_capacity)                        \
    {                                                                          \
        .buf = ccc_buf_init(impl_mem_ptr, impl_alloc_fn, impl_aux_data,        \
                            impl_capacity),                                    \
        .root = 0,                                                             \
        .free_list = 0,                                                        \
        .key_offset = offsetof(typeof(*(impl_mem_ptr)), impl_key_elem_field),  \
        .node_elem_offset                                                      \
        = offsetof(typeof(*(impl_mem_ptr)), impl_node_elem_field),             \
        .cmp = (impl_key_cmp_fn),                                              \
    }

/** @private */
#define ccc_impl_hom_as(handle_ordered_map_ptr, type_name, handle...)          \
    ((type_name *)ccc_buf_at(&(handle_ordered_map_ptr)->buf, (handle)))

/*==================     Core Macro Implementations     =====================*/

/** @private */
#define ccc_impl_hom_and_modify_w(handle_ordered_map_handle_ptr, type_name,    \
                                  closure_over_T...)                           \
    (__extension__({                                                           \
        __auto_type impl_hom_mod_hndl_ptr = (handle_ordered_map_handle_ptr);   \
        struct ccc_htree_handle impl_hom_mod_hndl                              \
            = {.handle = {.stats = CCC_ENTRY_ARG_ERROR}};                      \
        if (impl_hom_mod_hndl_ptr)                                             \
        {                                                                      \
            impl_hom_mod_hndl = impl_hom_mod_hndl_ptr->impl;                   \
            if (impl_hom_mod_hndl.handle.stats & CCC_ENTRY_OCCUPIED)           \
            {                                                                  \
                type_name *const T = ccc_buf_at(&impl_hom_mod_hndl.hom->buf,   \
                                                impl_hom_mod_hndl.handle.i);   \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_hom_mod_hndl;                                                     \
    }))

/** @private */
#define ccc_impl_hom_or_insert_w(handle_ordered_map_handle_ptr,                \
                                 lazy_key_value...)                            \
    (__extension__({                                                           \
        __auto_type impl_hom_or_ins_hndl_ptr                                   \
            = (handle_ordered_map_handle_ptr);                                 \
        ccc_handle_i impl_hom_or_ins_ret = 0;                                  \
        if (impl_hom_or_ins_hndl_ptr)                                          \
        {                                                                      \
            struct ccc_htree_handle *impl_hom_or_ins_hndl                      \
                = &impl_hom_or_ins_hndl_ptr->impl;                             \
            if (impl_hom_or_ins_hndl->handle.stats == CCC_ENTRY_OCCUPIED)      \
            {                                                                  \
                impl_hom_or_ins_ret = impl_hom_or_ins_hndl->handle.i;          \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_hom_or_ins_ret                                            \
                    = ccc_impl_hom_alloc_slot(impl_hom_or_ins_hndl->hom);      \
                if (impl_hom_or_ins_ret)                                       \
                {                                                              \
                    *((typeof(lazy_key_value) *)ccc_buf_at(                    \
                        &impl_hom_or_ins_hndl->hom->buf, impl_hom_or_ins_ret)) \
                        = lazy_key_value;                                      \
                    ccc_impl_hom_insert(impl_hom_or_ins_hndl->hom,             \
                                        impl_hom_or_ins_ret);                  \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_hom_or_ins_ret;                                                   \
    }))

/** @private */
#define ccc_impl_hom_insert_handle_w(handle_ordered_map_handle_ptr,            \
                                     lazy_key_value...)                        \
    (__extension__({                                                           \
        __auto_type impl_hom_ins_hndl_ptr = (handle_ordered_map_handle_ptr);   \
        ccc_handle_i impl_hom_ins_hndl_ret = 0;                                \
        if (impl_hom_ins_hndl_ptr)                                             \
        {                                                                      \
            struct ccc_htree_handle *impl_hom_ins_hndl                         \
                = &impl_hom_ins_hndl_ptr->impl;                                \
            if (!(impl_hom_ins_hndl->handle.stats & CCC_ENTRY_OCCUPIED))       \
            {                                                                  \
                impl_hom_ins_hndl_ret                                          \
                    = ccc_impl_hom_alloc_slot(impl_hom_ins_hndl->hom);         \
                if (impl_hom_ins_hndl_ret)                                     \
                {                                                              \
                    *((typeof(lazy_key_value) *)ccc_buf_at(                    \
                        &impl_hom_ins_hndl->hom->buf, impl_hom_ins_hndl_ret))  \
                        = lazy_key_value;                                      \
                    ccc_impl_hom_insert(impl_hom_ins_hndl->hom,                \
                                        impl_hom_ins_hndl_ret);                \
                }                                                              \
            }                                                                  \
            else if (impl_hom_ins_hndl->handle.stats == CCC_ENTRY_OCCUPIED)    \
            {                                                                  \
                impl_hom_ins_hndl_ret = impl_hom_ins_hndl->handle.i;           \
                struct ccc_homap_elem impl_ins_hndl_saved                      \
                    = *ccc_impl_homap_elem_at(impl_hom_ins_hndl->hom,          \
                                              impl_hom_ins_hndl_ret);          \
                *((typeof(lazy_key_value) *)ccc_buf_at(                        \
                    &impl_hom_ins_hndl->hom->buf, impl_hom_ins_hndl_ret))      \
                    = lazy_key_value;                                          \
                *ccc_impl_homap_elem_at(impl_hom_ins_hndl->hom,                \
                                        impl_hom_ins_hndl_ret)                 \
                    = impl_ins_hndl_saved;                                     \
            }                                                                  \
        }                                                                      \
        impl_hom_ins_hndl_ret;                                                 \
    }))

/** @private */
#define ccc_impl_hom_try_insert_w(handle_ordered_map_ptr, key, lazy_value...)  \
    (__extension__({                                                           \
        __auto_type impl_hom_try_ins_map_ptr = (handle_ordered_map_ptr);       \
        struct ccc_handl impl_hom_try_ins_hndl_ret                             \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_hom_try_ins_map_ptr)                                          \
        {                                                                      \
            __auto_type impl_hom_key = (key);                                  \
            struct ccc_htree_handle impl_hom_try_ins_hndl                      \
                = ccc_impl_hom_handle(impl_hom_try_ins_map_ptr,                \
                                      (void *)&impl_hom_key);                  \
            if (!(impl_hom_try_ins_hndl.handle.stats & CCC_ENTRY_OCCUPIED))    \
            {                                                                  \
                impl_hom_try_ins_hndl_ret = (struct ccc_handl){                \
                    .i = ccc_impl_hom_alloc_slot(impl_hom_try_ins_hndl.hom),   \
                    .stats = CCC_ENTRY_INSERT_ERROR};                          \
                if (impl_hom_try_ins_hndl_ret.i)                               \
                {                                                              \
                    *((typeof(lazy_value) *)ccc_buf_at(                        \
                        &impl_hom_try_ins_map_ptr->buf,                        \
                        impl_hom_try_ins_hndl_ret.i))                          \
                        = lazy_value;                                          \
                    *((typeof(impl_hom_key) *)ccc_impl_hom_key_at(             \
                        impl_hom_try_ins_hndl.hom,                             \
                        impl_hom_try_ins_hndl_ret.i))                          \
                        = impl_hom_key;                                        \
                    ccc_impl_hom_insert(impl_hom_try_ins_hndl.hom,             \
                                        impl_hom_try_ins_hndl_ret.i);          \
                    impl_hom_try_ins_hndl_ret.stats = CCC_ENTRY_VACANT;        \
                }                                                              \
            }                                                                  \
            else if (impl_hom_try_ins_hndl.handle.stats == CCC_ENTRY_OCCUPIED) \
            {                                                                  \
                impl_hom_try_ins_hndl_ret = (struct ccc_handl){                \
                    .i = impl_hom_try_ins_hndl.handle.i,                       \
                    .stats = impl_hom_try_ins_hndl.handle.stats};              \
            }                                                                  \
        }                                                                      \
        impl_hom_try_ins_hndl_ret;                                             \
    }))

/** @private */
#define ccc_impl_hom_insert_or_assign_w(handle_ordered_map_ptr, key,           \
                                        lazy_value...)                         \
    (__extension__({                                                           \
        __auto_type impl_hom_ins_or_assign_map_ptr = (handle_ordered_map_ptr); \
        struct ccc_handl impl_hom_ins_or_assign_hndl_ret                       \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_hom_ins_or_assign_map_ptr)                                    \
        {                                                                      \
            __auto_type impl_hom_key = (key);                                  \
            struct ccc_htree_handle impl_hom_ins_or_assign_hndl                \
                = ccc_impl_hom_handle(impl_hom_ins_or_assign_map_ptr,          \
                                      (void *)&impl_hom_key);                  \
            if (!(impl_hom_ins_or_assign_hndl.handle.stats                     \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                impl_hom_ins_or_assign_hndl_ret                                \
                    = (struct ccc_handl){.i = ccc_impl_hom_alloc_slot(         \
                                             impl_hom_ins_or_assign_hndl.hom), \
                                         .stats = CCC_ENTRY_INSERT_ERROR};     \
                if (impl_hom_ins_or_assign_hndl_ret.i)                         \
                {                                                              \
                    *((typeof(lazy_value) *)ccc_buf_at(                        \
                        &impl_hom_ins_or_assign_map_ptr->buf,                  \
                        impl_hom_ins_or_assign_hndl_ret.i))                    \
                        = lazy_value;                                          \
                    *((typeof(impl_hom_key) *)ccc_impl_hom_key_at(             \
                        impl_hom_ins_or_assign_hndl.hom,                       \
                        impl_hom_ins_or_assign_hndl_ret.i))                    \
                        = impl_hom_key;                                        \
                    ccc_impl_hom_insert(impl_hom_ins_or_assign_hndl.hom,       \
                                        impl_hom_ins_or_assign_hndl_ret.i);    \
                    impl_hom_ins_or_assign_hndl_ret.stats = CCC_ENTRY_VACANT;  \
                }                                                              \
            }                                                                  \
            else if (impl_hom_ins_or_assign_hndl.handle.stats                  \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct ccc_homap_elem impl_ins_hndl_saved                      \
                    = *ccc_impl_homap_elem_at(                                 \
                        impl_hom_ins_or_assign_hndl.hom,                       \
                        impl_hom_ins_or_assign_hndl.handle.i);                 \
                *((typeof(lazy_value) *)ccc_buf_at(                            \
                    &impl_hom_ins_or_assign_hndl.hom->buf,                     \
                    impl_hom_ins_or_assign_hndl.handle.i))                     \
                    = lazy_value;                                              \
                *ccc_impl_homap_elem_at(impl_hom_ins_or_assign_hndl.hom,       \
                                        impl_hom_ins_or_assign_hndl.handle.i)  \
                    = impl_ins_hndl_saved;                                     \
                impl_hom_ins_or_assign_hndl_ret = (struct ccc_handl){          \
                    .i = impl_hom_ins_or_assign_hndl.handle.i,                 \
                    .stats = impl_hom_ins_or_assign_hndl.handle.stats};        \
                *((typeof(impl_hom_key) *)ccc_impl_hom_key_at(                 \
                    impl_hom_ins_or_assign_hndl.hom,                           \
                    impl_hom_ins_or_assign_hndl.handle.i))                     \
                    = impl_hom_key;                                            \
            }                                                                  \
        }                                                                      \
        impl_hom_ins_or_assign_hndl_ret;                                       \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_HANDLE_ORDERED_MAP_H */
