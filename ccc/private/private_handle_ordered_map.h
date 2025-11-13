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

#include "../types.h"
#include "private_types.h" /* NOLINT */

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private Runs the top down splay tree algorithm with the addition of a free
list for providing new nodes within the buffer. The parent field normally
tracks parent when in the tree for iteration purposes. When a node is removed
from the tree it is added to the free singly linked list. The free list is a
LIFO push to front stack. */
struct CCC_homap_elem
{
    /** @private Child nodes in array to unify Left and Right. */
    size_t branch[2];
    union
    {
        /** @private Parent of splay tree node when allocated. */
        size_t parent;
        /** @private Points to next free when not allocated. */
        size_t next_free;
    };
};

/** @private A handle ordered map is a modified struct of arrays layout
with the modification being that we may have the arrays as pointer offsets in
a contiguous allocation if the user desires a dynamic map.

The user data array comes first allowing the user to store any type they wish
in the container contiguously with no intrusive element padding, saving space.

The nodes array is next. These nodes track the indices of the child and parent
nodes in the WAVL tree.

Here is the layout in one contiguous array.

(D = Data Array, N = Nodes Array, _N = Capacity - 1)

┌───┬───┬───┬───┬───┬───┬───┬───┐
│D_0│D_1│...│D_N│N_0│N_1│...│N_N│
└───┴───┴───┴───┴───┴───┴───┴───┘

This layout costs us in consulting both the data and nodes array during the
top down splay operation. However, the benefit of space saving and no wasted
padding bytes between element fields or multiple elements in an array is the
goal of this implementation. Speed is a secondary goal to space for these
implementations as the space savings can be significant. */
struct CCC_homap
{
    /** @private The contiguous array of user data. */
    void *data;
    /** @private The contiguous array of WAVL tree meta data. */
    struct CCC_homap_elem *nodes;
    /** @private The current capacity. */
    size_t capacity;
    /** @private The current size. */
    size_t count;
    /** @private The root node of the Splay Tree. */
    size_t root;
    /** @private The start of the free singly linked list. */
    size_t free_list;
    /** @private The size of the type stored in the map. */
    size_t sizeof_type;
    /** @private Where user key can be found in type. */
    size_t key_offset;
    /** @private The provided key comparison function. */
    CCC_any_key_cmp_fn *cmp;
    /** @private The provided allocation function, if any. */
    CCC_Allocator *alloc;
    /** @private The provided auxiliary data, if any. */
    void *aux;
};

/** @private A handle is like an entry but if the handle is Occupied, we can
guarantee the user that their element will not move from the provided index. */
struct CCC_htree_handle
{
    /** @private Map associated with this handle. */
    struct CCC_homap *hom;
    /** @private Current index of the handle. */
    size_t i;
    /** @private Saves last comparison direction. */
    CCC_Order last_cmp;
    /** @private The entry status flag. */
    CCC_Entry_status stats;
};

/** @private Enable return by compound literal reference on the stack. Think
of this method as return by value but with the additional ability to pass by
pointer in a functional style. `fnB(&(union CCC_homap_handle){fnA().impl});` */
union CCC_homap_handle
{
    /** @private The field containing the handle struct. */
    struct CCC_htree_handle impl;
};

/*===========================   Private Interface ===========================*/

/** @private */
void CCC_private_hom_insert(struct CCC_homap *hom, size_t elem_i);
/** @private */
struct CCC_htree_handle CCC_private_hom_handle(struct CCC_homap *hom,
                                               void const *key);
/** @private */
void *CCC_private_hom_data_at(struct CCC_homap const *hom, size_t slot);
/** @private */
void *CCC_private_hom_key_at(struct CCC_homap const *hom, size_t slot);
/** @private */
size_t CCC_private_hom_alloc_slot(struct CCC_homap *hom);

/*========================     Initialization       =========================*/

/** @private The user can declare a fixed size ordered map with the help of
static asserts to ensure the layout is compatible with our internal metadata. */
#define CCC_private_hom_declare_fixed_map(                                     \
    private_fixed_map_type_name, private_key_val_type_name, private_capacity)  \
    static_assert((private_capacity) > 1,                                      \
                  "fixed size map must have capacity greater than 1");         \
    typedef struct                                                             \
    {                                                                          \
        private_key_val_type_name data[(private_capacity)];                    \
        struct CCC_homap_elem nodes[(private_capacity)];                       \
    }(private_fixed_map_type_name)

/** @private Taking the size of the array actually works here because the field
is of a known fixed size defined at compile time, not just a pointer. */
#define CCC_private_hom_fixed_capacity(fixed_map_type_name)                    \
    (sizeof((fixed_map_type_name){}.nodes) / sizeof(struct CCC_homap_elem))

/** @private */
#define CCC_private_hom_initialize(                                            \
    private_memory_ptr, private_type_name, private_key_elem_field,             \
    private_key_cmp_fn, private_alloc_fn, private_aux_data, private_capacity)  \
    {                                                                          \
        .data = (private_memory_ptr),                                          \
        .nodes = NULL,                                                         \
        .capacity = (private_capacity),                                        \
        .count = 0,                                                            \
        .root = 0,                                                             \
        .free_list = 0,                                                        \
        .sizeof_type = sizeof(private_type_name),                              \
        .key_offset = offsetof(private_type_name, private_key_elem_field),     \
        .cmp = (private_key_cmp_fn),                                           \
        .alloc = (private_alloc_fn),                                           \
        .aux = (private_aux_data),                                             \
    }

/** @private */
#define CCC_private_hom_as(handle_ordered_map_ptr, type_name, handle...)       \
    ((type_name *)CCC_private_hom_data_at((handle_ordered_map_ptr), (handle)))

/*==================     Core Macro Implementations     =====================*/

/** @private */
#define CCC_private_hom_and_modify_w(handle_ordered_map_handle_ptr, type_name, \
                                     closure_over_T...)                        \
    (__extension__({                                                           \
        __auto_type private_hom_mod_hndl_ptr                                   \
            = (handle_ordered_map_handle_ptr);                                 \
        struct CCC_htree_handle private_hom_mod_hndl                           \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (private_hom_mod_hndl_ptr)                                          \
        {                                                                      \
            private_hom_mod_hndl = private_hom_mod_hndl_ptr->impl;             \
            if (private_hom_mod_hndl.stats & CCC_ENTRY_OCCUPIED)               \
            {                                                                  \
                type_name *const T = CCC_private_hom_data_at(                  \
                    private_hom_mod_hndl.hom, private_hom_mod_hndl.i);         \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_hom_mod_hndl;                                                  \
    }))

/** @private */
#define CCC_private_hom_or_insert_w(handle_ordered_map_handle_ptr,             \
                                    lazy_key_value...)                         \
    (__extension__({                                                           \
        __auto_type private_hom_or_ins_hndl_ptr                                \
            = (handle_ordered_map_handle_ptr);                                 \
        CCC_Handle_index private_hom_or_ins_ret = 0;                           \
        if (private_hom_or_ins_hndl_ptr)                                       \
        {                                                                      \
            if (private_hom_or_ins_hndl_ptr->impl.stats == CCC_ENTRY_OCCUPIED) \
            {                                                                  \
                private_hom_or_ins_ret = private_hom_or_ins_hndl_ptr->impl.i;  \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_hom_or_ins_ret = CCC_private_hom_alloc_slot(           \
                    private_hom_or_ins_hndl_ptr->impl.hom);                    \
                if (private_hom_or_ins_ret)                                    \
                {                                                              \
                    *((typeof(lazy_key_value) *)CCC_private_hom_data_at(       \
                        private_hom_or_ins_hndl_ptr->impl.hom,                 \
                        private_hom_or_ins_ret))                               \
                        = lazy_key_value;                                      \
                    CCC_private_hom_insert(                                    \
                        private_hom_or_ins_hndl_ptr->impl.hom,                 \
                        private_hom_or_ins_ret);                               \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_hom_or_ins_ret;                                                \
    }))

/** @private */
#define CCC_private_hom_insert_handle_w(handle_ordered_map_handle_ptr,         \
                                        lazy_key_value...)                     \
    (__extension__({                                                           \
        __auto_type private_hom_ins_hndl_ptr                                   \
            = (handle_ordered_map_handle_ptr);                                 \
        CCC_Handle_index private_hom_ins_hndl_ret = 0;                         \
        if (private_hom_ins_hndl_ptr)                                          \
        {                                                                      \
            if (!(private_hom_ins_hndl_ptr->impl.stats & CCC_ENTRY_OCCUPIED))  \
            {                                                                  \
                private_hom_ins_hndl_ret = CCC_private_hom_alloc_slot(         \
                    private_hom_ins_hndl_ptr->impl.hom);                       \
                if (private_hom_ins_hndl_ret)                                  \
                {                                                              \
                    *((typeof(lazy_key_value) *)CCC_private_hom_data_at(       \
                        private_hom_ins_hndl_ptr->impl.hom,                    \
                        private_hom_ins_hndl_ret))                             \
                        = lazy_key_value;                                      \
                    CCC_private_hom_insert(private_hom_ins_hndl_ptr->impl.hom, \
                                           private_hom_ins_hndl_ret);          \
                }                                                              \
            }                                                                  \
            else if (private_hom_ins_hndl_ptr->impl.stats                      \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                *((typeof(lazy_key_value) *)CCC_private_hom_data_at(           \
                    private_hom_ins_hndl_ptr->impl.hom,                        \
                    private_hom_ins_hndl_ptr->impl.i))                         \
                    = lazy_key_value;                                          \
                private_hom_ins_hndl_ret = private_hom_ins_hndl_ptr->impl.i;   \
            }                                                                  \
        }                                                                      \
        private_hom_ins_hndl_ret;                                              \
    }))

/** @private */
#define CCC_private_hom_try_insert_w(handle_ordered_map_ptr, key,              \
                                     lazy_value...)                            \
    (__extension__({                                                           \
        __auto_type private_hom_try_ins_map_ptr = (handle_ordered_map_ptr);    \
        struct CCC_Handle private_hom_try_ins_hndl_ret                         \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (private_hom_try_ins_map_ptr)                                       \
        {                                                                      \
            __auto_type private_hom_key = (key);                               \
            struct CCC_htree_handle private_hom_try_ins_hndl                   \
                = CCC_private_hom_handle(private_hom_try_ins_map_ptr,          \
                                         (void *)&private_hom_key);            \
            if (!(private_hom_try_ins_hndl.stats & CCC_ENTRY_OCCUPIED))        \
            {                                                                  \
                private_hom_try_ins_hndl_ret = (struct CCC_Handle){            \
                    .i = CCC_private_hom_alloc_slot(                           \
                        private_hom_try_ins_hndl.hom),                         \
                    .stats = CCC_ENTRY_INSERT_ERROR,                           \
                };                                                             \
                if (private_hom_try_ins_hndl_ret.i)                            \
                {                                                              \
                    *((typeof(lazy_value) *)CCC_private_hom_data_at(           \
                        private_hom_try_ins_map_ptr,                           \
                        private_hom_try_ins_hndl_ret.i))                       \
                        = lazy_value;                                          \
                    *((typeof(private_hom_key) *)CCC_private_hom_key_at(       \
                        private_hom_try_ins_hndl.hom,                          \
                        private_hom_try_ins_hndl_ret.i))                       \
                        = private_hom_key;                                     \
                    CCC_private_hom_insert(private_hom_try_ins_hndl.hom,       \
                                           private_hom_try_ins_hndl_ret.i);    \
                    private_hom_try_ins_hndl_ret.stats = CCC_ENTRY_VACANT;     \
                }                                                              \
            }                                                                  \
            else if (private_hom_try_ins_hndl.stats == CCC_ENTRY_OCCUPIED)     \
            {                                                                  \
                private_hom_try_ins_hndl_ret = (struct CCC_Handle){            \
                    .i = private_hom_try_ins_hndl.i,                           \
                    .stats = private_hom_try_ins_hndl.stats};                  \
            }                                                                  \
        }                                                                      \
        private_hom_try_ins_hndl_ret;                                          \
    }))

/** @private */
#define CCC_private_hom_insert_or_assign_w(handle_ordered_map_ptr, key,        \
                                           lazy_value...)                      \
    (__extension__({                                                           \
        __auto_type private_hom_ins_or_assign_map_ptr                          \
            = (handle_ordered_map_ptr);                                        \
        struct CCC_Handle private_hom_ins_or_assign_hndl_ret                   \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (private_hom_ins_or_assign_map_ptr)                                 \
        {                                                                      \
            __auto_type private_hom_key = (key);                               \
            struct CCC_htree_handle private_hom_ins_or_assign_hndl             \
                = CCC_private_hom_handle(private_hom_ins_or_assign_map_ptr,    \
                                         (void *)&private_hom_key);            \
            if (!(private_hom_ins_or_assign_hndl.stats & CCC_ENTRY_OCCUPIED))  \
            {                                                                  \
                private_hom_ins_or_assign_hndl_ret = (struct CCC_Handle){      \
                    .i = CCC_private_hom_alloc_slot(                           \
                        private_hom_ins_or_assign_hndl.hom),                   \
                    .stats = CCC_ENTRY_INSERT_ERROR,                           \
                };                                                             \
                if (private_hom_ins_or_assign_hndl_ret.i)                      \
                {                                                              \
                    *((typeof(lazy_value) *)CCC_private_hom_data_at(           \
                        private_hom_ins_or_assign_map_ptr,                     \
                        private_hom_ins_or_assign_hndl_ret.i))                 \
                        = lazy_value;                                          \
                    *((typeof(private_hom_key) *)CCC_private_hom_key_at(       \
                        private_hom_ins_or_assign_hndl.hom,                    \
                        private_hom_ins_or_assign_hndl_ret.i))                 \
                        = private_hom_key;                                     \
                    CCC_private_hom_insert(                                    \
                        private_hom_ins_or_assign_hndl.hom,                    \
                        private_hom_ins_or_assign_hndl_ret.i);                 \
                    private_hom_ins_or_assign_hndl_ret.stats                   \
                        = CCC_ENTRY_VACANT;                                    \
                }                                                              \
            }                                                                  \
            else if (private_hom_ins_or_assign_hndl.stats                      \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                *((typeof(lazy_value) *)CCC_private_hom_data_at(               \
                    private_hom_ins_or_assign_hndl.hom,                        \
                    private_hom_ins_or_assign_hndl.i))                         \
                    = lazy_value;                                              \
                private_hom_ins_or_assign_hndl_ret = (struct CCC_Handle){      \
                    .i = private_hom_ins_or_assign_hndl.i,                     \
                    .stats = private_hom_ins_or_assign_hndl.stats,             \
                };                                                             \
                *((typeof(private_hom_key) *)CCC_private_hom_key_at(           \
                    private_hom_ins_or_assign_hndl.hom,                        \
                    private_hom_ins_or_assign_hndl.i))                         \
                    = private_hom_key;                                         \
            }                                                                  \
        }                                                                      \
        private_hom_ins_or_assign_hndl_ret;                                    \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_HANDLE_ORDERED_MAP_H */
