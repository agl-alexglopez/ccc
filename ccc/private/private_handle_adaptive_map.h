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
#ifndef CCC_PRIVATE_HANDLE_ADAPTIVE_MAP_H
#define CCC_PRIVATE_HANDLE_ADAPTIVE_MAP_H

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
struct CCC_Handle_adaptive_map_node
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
struct CCC_Handle_adaptive_map
{
    /** @private The contiguous array of user data. */
    void *data;
    /** @private The contiguous array of WAVL tree meta data. */
    struct CCC_Handle_adaptive_map_node *nodes;
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
    CCC_Key_comparator *order;
    /** @private The provided allocation function, if any. */
    CCC_Allocator *allocate;
    /** @private The provided context data, if any. */
    void *context;
};

/** @private A handle is like an entry but if the handle is Occupied, we can
guarantee the user that their element will not move from the provided index. */
struct CCC_Handle_adaptive_map_handle
{
    /** @private Map associated with this handle. */
    struct CCC_Handle_adaptive_map *map;
    /** @private Current index of the handle. */
    size_t index;
    /** @private Saves last comparison direction. */
    CCC_Order last_order;
    /** @private The entry status flag. */
    CCC_Entry_status status;
};

/** @private Enable return by compound literal reference on the stack. Think
of this method as return by value but with the additional ability to pass by
pointer in a functional style. `fnB(&(union
CCC_Handle_adaptive_map_handle_wrap){fnA().private});` */
union CCC_Handle_adaptive_map_handle_wrap
{
    /** @private The field containing the handle struct. */
    struct CCC_Handle_adaptive_map_handle private;
};

/*===========================   Private Interface ===========================*/

/** @private */
void CCC_private_handle_adaptive_map_insert(struct CCC_Handle_adaptive_map *,
                                            size_t);
/** @private */
struct CCC_Handle_adaptive_map_handle
CCC_private_handle_adaptive_map_handle(struct CCC_Handle_adaptive_map *,
                                       void const *key);
/** @private */
void *
CCC_private_handle_adaptive_map_data_at(struct CCC_Handle_adaptive_map const *,
                                        size_t slot);
/** @private */
void *
CCC_private_handle_adaptive_map_key_at(struct CCC_Handle_adaptive_map const *,
                                       size_t slot);
/** @private */
size_t
CCC_private_handle_adaptive_map_allocate_slot(struct CCC_Handle_adaptive_map *);

/*========================     Initialization       =========================*/

/** @private The user can declare a fixed size ordered map with the help of
static asserts to ensure the layout is compatible with our internal metadata. */
#define CCC_private_handle_adaptive_map_declare_fixed_map(                     \
    private_fixed_map_type_name, private_key_val_type_name, private_capacity)  \
    static_assert((private_capacity) > 1,                                      \
                  "fixed size map must have capacity greater than 1");         \
    typedef struct                                                             \
    {                                                                          \
        private_key_val_type_name data[(private_capacity)];                    \
        struct CCC_Handle_adaptive_map_node nodes[(private_capacity)];         \
    }(private_fixed_map_type_name)

/** @private Taking the size of the array actually works here because the field
is of a known fixed size defined at compile time, not just a pointer. */
#define CCC_private_handle_adaptive_map_fixed_capacity(fixed_map_type_name)    \
    (sizeof((fixed_map_type_name){}.nodes)                                     \
     / sizeof(struct CCC_Handle_adaptive_map_node))

/** @private */
#define CCC_private_handle_adaptive_map_initialize(                            \
    private_memory_pointer, private_type_name, private_key_node_field,         \
    private_key_order_fn, private_allocate, private_context_data,              \
    private_capacity)                                                          \
    {                                                                          \
        .data = (private_memory_pointer),                                      \
        .nodes = NULL,                                                         \
        .capacity = (private_capacity),                                        \
        .count = 0,                                                            \
        .root = 0,                                                             \
        .free_list = 0,                                                        \
        .sizeof_type = sizeof(private_type_name),                              \
        .key_offset = offsetof(private_type_name, private_key_node_field),     \
        .order = (private_key_order_fn),                                       \
        .allocate = (private_allocate),                                        \
        .context = (private_context_data),                                     \
    }

/** @private */
#define CCC_private_handle_adaptive_map_as(Handle_adaptive_map_pointer,        \
                                           type_name, handle...)               \
    ((type_name *)CCC_private_handle_adaptive_map_data_at(                     \
        (Handle_adaptive_map_pointer), (handle)))

/*==================     Core Macro Implementations     =====================*/

/** @private */
#define CCC_private_handle_adaptive_map_and_modify_w(                          \
    Handle_adaptive_map_handle_pointer, type_name, closure_over_T...)          \
    (__extension__({                                                           \
        __auto_type private_handle_adaptive_map_mod_hndl_pointer               \
            = (Handle_adaptive_map_handle_pointer);                            \
        struct CCC_Handle_adaptive_map_handle                                  \
            private_handle_adaptive_map_mod_hndl                               \
            = {.status = CCC_ENTRY_ARGUMENT_ERROR};                            \
        if (private_handle_adaptive_map_mod_hndl_pointer)                      \
        {                                                                      \
            private_handle_adaptive_map_mod_hndl                               \
                = private_handle_adaptive_map_mod_hndl_pointer->private;       \
            if (private_handle_adaptive_map_mod_hndl.status                    \
                & CCC_ENTRY_OCCUPIED)                                          \
            {                                                                  \
                type_name *const T = CCC_private_handle_adaptive_map_data_at(  \
                    private_handle_adaptive_map_mod_hndl.map,                  \
                    private_handle_adaptive_map_mod_hndl.index);               \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_handle_adaptive_map_mod_hndl;                                  \
    }))

/** @private */
#define CCC_private_handle_adaptive_map_or_insert_w(                           \
    Handle_adaptive_map_handle_pointer, lazy_key_value...)                     \
    (__extension__({                                                           \
        __auto_type private_handle_adaptive_map_or_ins_hndl_pointer            \
            = (Handle_adaptive_map_handle_pointer);                            \
        CCC_Handle_index private_handle_adaptive_map_or_ins_ret = 0;           \
        if (private_handle_adaptive_map_or_ins_hndl_pointer)                   \
        {                                                                      \
            if (private_handle_adaptive_map_or_ins_hndl_pointer->private       \
                    .status                                                    \
                == CCC_ENTRY_OCCUPIED)                                         \
            {                                                                  \
                private_handle_adaptive_map_or_ins_ret                         \
                    = private_handle_adaptive_map_or_ins_hndl_pointer->private \
                          .index;                                              \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_handle_adaptive_map_or_ins_ret                         \
                    = CCC_private_handle_adaptive_map_allocate_slot(           \
                        private_handle_adaptive_map_or_ins_hndl_pointer        \
                            ->private.map);                                    \
                if (private_handle_adaptive_map_or_ins_ret)                    \
                {                                                              \
                    *((typeof(lazy_key_value) *)                               \
                          CCC_private_handle_adaptive_map_data_at(             \
                              private_handle_adaptive_map_or_ins_hndl_pointer  \
                                  ->private.map,                               \
                              private_handle_adaptive_map_or_ins_ret))         \
                        = lazy_key_value;                                      \
                    CCC_private_handle_adaptive_map_insert(                    \
                        private_handle_adaptive_map_or_ins_hndl_pointer        \
                            ->private.map,                                     \
                        private_handle_adaptive_map_or_ins_ret);               \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_handle_adaptive_map_or_ins_ret;                                \
    }))

/** @private */
#define CCC_private_handle_adaptive_map_insert_handle_w(                       \
    Handle_adaptive_map_handle_pointer, lazy_key_value...)                     \
    (__extension__({                                                           \
        __auto_type private_handle_adaptive_map_ins_hndl_pointer               \
            = (Handle_adaptive_map_handle_pointer);                            \
        CCC_Handle_index private_handle_adaptive_map_ins_hndl_ret = 0;         \
        if (private_handle_adaptive_map_ins_hndl_pointer)                      \
        {                                                                      \
            if (!(private_handle_adaptive_map_ins_hndl_pointer->private.status \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                private_handle_adaptive_map_ins_hndl_ret                       \
                    = CCC_private_handle_adaptive_map_allocate_slot(           \
                        private_handle_adaptive_map_ins_hndl_pointer->private  \
                            .map);                                             \
                if (private_handle_adaptive_map_ins_hndl_ret)                  \
                {                                                              \
                    *((typeof(lazy_key_value) *)                               \
                          CCC_private_handle_adaptive_map_data_at(             \
                              private_handle_adaptive_map_ins_hndl_pointer     \
                                  ->private.map,                               \
                              private_handle_adaptive_map_ins_hndl_ret))       \
                        = lazy_key_value;                                      \
                    CCC_private_handle_adaptive_map_insert(                    \
                        private_handle_adaptive_map_ins_hndl_pointer->private  \
                            .map,                                              \
                        private_handle_adaptive_map_ins_hndl_ret);             \
                }                                                              \
            }                                                                  \
            else if (private_handle_adaptive_map_ins_hndl_pointer->private     \
                         .status                                               \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                *((typeof(lazy_key_value) *)                                   \
                      CCC_private_handle_adaptive_map_data_at(                 \
                          private_handle_adaptive_map_ins_hndl_pointer         \
                              ->private.map,                                   \
                          private_handle_adaptive_map_ins_hndl_pointer         \
                              ->private.index))                                \
                    = lazy_key_value;                                          \
                private_handle_adaptive_map_ins_hndl_ret                       \
                    = private_handle_adaptive_map_ins_hndl_pointer->private    \
                          .index;                                              \
            }                                                                  \
        }                                                                      \
        private_handle_adaptive_map_ins_hndl_ret;                              \
    }))

/** @private */
#define CCC_private_handle_adaptive_map_try_insert_w(                          \
    Handle_adaptive_map_pointer, key, type_compound_literal...)                \
    (__extension__({                                                           \
        __auto_type private_handle_adaptive_map_try_ins_map_pointer            \
            = (Handle_adaptive_map_pointer);                                   \
        struct CCC_Handle private_handle_adaptive_map_try_ins_hndl_ret         \
            = {.status = CCC_ENTRY_ARGUMENT_ERROR};                            \
        if (private_handle_adaptive_map_try_ins_map_pointer)                   \
        {                                                                      \
            __auto_type private_handle_adaptive_map_key = (key);               \
            struct CCC_Handle_adaptive_map_handle                              \
                private_handle_adaptive_map_try_ins_hndl                       \
                = CCC_private_handle_adaptive_map_handle(                      \
                    private_handle_adaptive_map_try_ins_map_pointer,           \
                    (void *)&private_handle_adaptive_map_key);                 \
            if (!(private_handle_adaptive_map_try_ins_hndl.status              \
                  & CCC_ENTRY_OCCUPIED))                                       \
            {                                                                  \
                private_handle_adaptive_map_try_ins_hndl_ret                   \
                    = (struct CCC_Handle){                                     \
                        .index                                                 \
                        = CCC_private_handle_adaptive_map_allocate_slot(       \
                            private_handle_adaptive_map_try_ins_hndl.map),     \
                        .status = CCC_ENTRY_INSERT_ERROR,                      \
                    };                                                         \
                if (private_handle_adaptive_map_try_ins_hndl_ret.index)        \
                {                                                              \
                    *((typeof(type_compound_literal) *)                        \
                          CCC_private_handle_adaptive_map_data_at(             \
                              private_handle_adaptive_map_try_ins_map_pointer, \
                              private_handle_adaptive_map_try_ins_hndl_ret     \
                                  .index))                                     \
                        = type_compound_literal;                               \
                    *((typeof(private_handle_adaptive_map_key) *)              \
                          CCC_private_handle_adaptive_map_key_at(              \
                              private_handle_adaptive_map_try_ins_hndl.map,    \
                              private_handle_adaptive_map_try_ins_hndl_ret     \
                                  .index))                                     \
                        = private_handle_adaptive_map_key;                     \
                    CCC_private_handle_adaptive_map_insert(                    \
                        private_handle_adaptive_map_try_ins_hndl.map,          \
                        private_handle_adaptive_map_try_ins_hndl_ret.index);   \
                    private_handle_adaptive_map_try_ins_hndl_ret.status        \
                        = CCC_ENTRY_VACANT;                                    \
                }                                                              \
            }                                                                  \
            else if (private_handle_adaptive_map_try_ins_hndl.status           \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                private_handle_adaptive_map_try_ins_hndl_ret                   \
                    = (struct CCC_Handle){                                     \
                        .index                                                 \
                        = private_handle_adaptive_map_try_ins_hndl.index,      \
                        .status                                                \
                        = private_handle_adaptive_map_try_ins_hndl.status};    \
            }                                                                  \
        }                                                                      \
        private_handle_adaptive_map_try_ins_hndl_ret;                          \
    }))

/** @private */
#define CCC_private_handle_adaptive_map_insert_or_assign_w(                          \
    Handle_adaptive_map_pointer, key, type_compound_literal...)                      \
    (__extension__({                                                                 \
        __auto_type private_handle_adaptive_map_ins_or_assign_map_pointer            \
            = (Handle_adaptive_map_pointer);                                         \
        struct CCC_Handle private_handle_adaptive_map_ins_or_assign_hndl_ret         \
            = {.status = CCC_ENTRY_ARGUMENT_ERROR};                                  \
        if (private_handle_adaptive_map_ins_or_assign_map_pointer)                   \
        {                                                                            \
            __auto_type private_handle_adaptive_map_key = (key);                     \
            struct CCC_Handle_adaptive_map_handle                                    \
                private_handle_adaptive_map_ins_or_assign_hndl                       \
                = CCC_private_handle_adaptive_map_handle(                            \
                    private_handle_adaptive_map_ins_or_assign_map_pointer,           \
                    (void *)&private_handle_adaptive_map_key);                       \
            if (!(private_handle_adaptive_map_ins_or_assign_hndl.status              \
                  & CCC_ENTRY_OCCUPIED))                                             \
            {                                                                        \
                private_handle_adaptive_map_ins_or_assign_hndl_ret                   \
                    = (struct CCC_Handle){                                           \
                        .index                                                       \
                        = CCC_private_handle_adaptive_map_allocate_slot(             \
                            private_handle_adaptive_map_ins_or_assign_hndl           \
                                .map),                                               \
                        .status = CCC_ENTRY_INSERT_ERROR,                            \
                    };                                                               \
                if (private_handle_adaptive_map_ins_or_assign_hndl_ret.index)        \
                {                                                                    \
                    *((typeof(type_compound_literal) *)                              \
                          CCC_private_handle_adaptive_map_data_at(                   \
                              private_handle_adaptive_map_ins_or_assign_map_pointer, \
                              private_handle_adaptive_map_ins_or_assign_hndl_ret     \
                                  .index))                                           \
                        = type_compound_literal;                                     \
                    *((typeof(private_handle_adaptive_map_key) *)                    \
                          CCC_private_handle_adaptive_map_key_at(                    \
                              private_handle_adaptive_map_ins_or_assign_hndl         \
                                  .map,                                              \
                              private_handle_adaptive_map_ins_or_assign_hndl_ret     \
                                  .index))                                           \
                        = private_handle_adaptive_map_key;                           \
                    CCC_private_handle_adaptive_map_insert(                          \
                        private_handle_adaptive_map_ins_or_assign_hndl.map,          \
                        private_handle_adaptive_map_ins_or_assign_hndl_ret           \
                            .index);                                                 \
                    private_handle_adaptive_map_ins_or_assign_hndl_ret.status        \
                        = CCC_ENTRY_VACANT;                                          \
                }                                                                    \
            }                                                                        \
            else if (private_handle_adaptive_map_ins_or_assign_hndl.status           \
                     == CCC_ENTRY_OCCUPIED)                                          \
            {                                                                        \
                *((typeof(type_compound_literal) *)                                  \
                      CCC_private_handle_adaptive_map_data_at(                       \
                          private_handle_adaptive_map_ins_or_assign_hndl.map,        \
                          private_handle_adaptive_map_ins_or_assign_hndl             \
                              .index))                                               \
                    = type_compound_literal;                                         \
                private_handle_adaptive_map_ins_or_assign_hndl_ret                   \
                    = (struct CCC_Handle){                                           \
                        .index                                                       \
                        = private_handle_adaptive_map_ins_or_assign_hndl             \
                              .index,                                                \
                        .status                                                      \
                        = private_handle_adaptive_map_ins_or_assign_hndl             \
                              .status,                                               \
                    };                                                               \
                *((typeof(private_handle_adaptive_map_key) *)                        \
                      CCC_private_handle_adaptive_map_key_at(                        \
                          private_handle_adaptive_map_ins_or_assign_hndl.map,        \
                          private_handle_adaptive_map_ins_or_assign_hndl             \
                              .index))                                               \
                    = private_handle_adaptive_map_key;                               \
            }                                                                        \
        }                                                                            \
        private_handle_adaptive_map_ins_or_assign_hndl_ret;                          \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_PRIVATE_HANDLE_ADAPTIVE_MAP_H */
