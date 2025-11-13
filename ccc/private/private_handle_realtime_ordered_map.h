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
#ifndef PRIVATE_HANDLE_REALTIME_ORDERED_MAP_H
#define PRIVATE_HANDLE_REALTIME_ORDERED_MAP_H

/** @cond */
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../types.h"
#include "private_types.h" /* NOLINT */

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private Runs the standard WAVL tree algorithms with the addition of
a free list. The parent field tracks the parent for an allocated node in the
tree that the user has inserted into the array. When the user removes a node
it is added to the front of a free list. The map will track the first free
node. The list is push to front LIFO stack. */
struct CCC_Handle_realtime_ordered_map_node
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
};

/** @private A handle realtime ordered map is a modified struct of arrays layout
with the modification being that we may have the arrays as pointer offsets in
a contiguous allocation if the user desires a dynamic map.

The user data array comes first allowing the user to store any type they wish
in the container contiguously with no intrusive element padding, saving space.

The nodes array is next. These nodes track the indices of the child and parent
nodes in the WAVL tree.

Finally, comes the parity bit array. This comes last because it allows the
optimal storage space to be used, a single bit. Usually, a data structure
theorist's "bit" of extra information per node comes out to a byte in practice
due to portability and implementation concerns. If this byte were to be included
in the current map elem, it would then be given 7 more bytes of padding by
the compiler to ensure proper alignment, wasting large amounts of space. Instead
all the bits are packed into their own bit array at the end of the allocation.
The bit at a given index represents the parity of data and its node at that same
index. This allows the implementation to follow the theorist's ideal.

Here is the layout in one contiguous array.

(D = Data Array, N = Nodes Array, P = Parity Bit Array, _N = Capacity - 1)

┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
│D_0│D_1│...│D_N│N_0│N_1│...│N_N│P_0│P_1│...│P_N│
└───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘

Consider how this layout saves space. Here is a more traditional approach.

```
struct CCC_Realtime_ordered_map_node
{
    size_t branch[2];
    union
    {
        size_t parent;
        size_t next_free;
    };
    uint8_t parity;
};
```

This struct wastes a byte for parity when only a bit is needed. It also has
an alignment of 8 bytes meaning the trailing 7 bytes are completely useless. If
this element were to intrude on a user element it would also force its 8 byte
alignment on user data. This is more wasted space if the user wanted to simply
use the map as a set for a smaller type such as an int.

If the user wanted a simple set of 64 ints, this intrusive design would cost
`64 * 40 = 2480` bytes, of which only `64 * (40 - 7 - 4) = 1856` bytes are in
use leaving `2480 - 1856 = 624` bytes wasted.

In contrast the current struct of arrays design lays out data such that we use
`(64 * 4) + 4 + (64 * 24) + 64 + B = 1860 + B` bytes where B is the number of
unused bits in the last block of the parity bit array (in this case 0). That
means there are only `4 + B` bytes wasted, 4 bytes of padding between the end of
the user type array and the start of the nodes array and the unused bits at the
end of the parity bit array. This also means it important to consider the
alignment differences that may occur between the user type and the node type.

This layout comes at the cost of consulting multiple arrays for many operations.
However, once user data has been inserted or removed the tree fix up operations
only need to consult the nodes array and the bit array which means more bits
and nodes can be loaded on a cache line. We no longer need to consider
arbitrarily sized or organized user data while we do operations on nodes and
bits. Performance metrics still must be measured to say whether this is faster
or slower than other approaches. However, the goal with this design is space
efficiency first, speed second. */
struct CCC_Handle_realtime_ordered_map
{
    /** @private The contiguous array of user data. */
    void *data;
    /** @private The contiguous array of WAVL tree meta data. */
    struct CCC_Handle_realtime_ordered_map_node *nodes;
    /** @private The parity bit array corresponding to each node. */
    unsigned *parity;
    /** @private The current capacity. */
    size_t capacity;
    /** @private The current size. */
    size_t count;
    /** @private The root node of the WAVL tree. */
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

/** @private */
struct CCC_Handle_realtime_ordered_map_handle
{
    /** @private Map associated with this handle. */
    struct CCC_Handle_realtime_ordered_map *handle_realtime_ordered_map;
    /** @private Current index of the handle. */
    size_t i;
    /** @private Saves last comparison direction. */
    CCC_Order last_order;
    /** @private The entry status flag. */
    CCC_Entry_status stats;
};

/** @private Wrapper for return by pointer on the stack in C23. */
union CCC_Handle_realtime_ordered_map_handle_wrap
{
    /** @private Single field to enable return by compound literal reference. */
    struct CCC_Handle_realtime_ordered_map_handle private;
};

/*========================  Private Interface  ==============================*/

/** @private */
void *CCC_private_handle_realtime_ordered_map_data_at(
    struct CCC_Handle_realtime_ordered_map const *handle_realtime_ordered_map,
    size_t slot);
/** @private */
void *CCC_private_handle_realtime_ordered_map_key_at(
    struct CCC_Handle_realtime_ordered_map const *handle_realtime_ordered_map,
    size_t slot);
/** @private */
struct CCC_Handle_realtime_ordered_map_node *
CCC_private_handle_realtime_ordered_map_node_at(
    struct CCC_Handle_realtime_ordered_map const *handle_realtime_ordered_map,
    size_t i);
/** @private */
struct CCC_Handle_realtime_ordered_map_handle
CCC_private_handle_realtime_ordered_map_handle(
    struct CCC_Handle_realtime_ordered_map const *handle_realtime_ordered_map,
    void const *key);
/** @private */
void CCC_private_handle_realtime_ordered_map_insert(
    struct CCC_Handle_realtime_ordered_map *handle_realtime_ordered_map,
    size_t parent_i, CCC_Order last_order, size_t elem_i);
/** @private */
size_t CCC_private_handle_realtime_ordered_map_allocate_slot(
    struct CCC_Handle_realtime_ordered_map *handle_realtime_ordered_map);

/*=========================      Initialization     =========================*/

/** @private Calculates the number of parity blocks needed to support the given
capacity. Provide the type used for the parity block array and the number of
blocks needed to round up to will be returned. */
#define CCC_private_handle_realtime_ordered_map_blocks(private_cap)            \
    (((private_cap)                                                            \
      + ((sizeof(*(struct CCC_Handle_realtime_ordered_map){}.parity)           \
          * CHAR_BIT)                                                          \
         - 1))                                                                 \
     / (sizeof(*(struct CCC_Handle_realtime_ordered_map){}.parity)             \
        * CHAR_BIT))

/** @private The user can declare a fixed size realtime ordered map with the
help of static asserts to ensure the layout is compatible with our internal
metadata. */
#define CCC_private_handle_realtime_ordered_map_declare_fixed_map(             \
    private_fixed_map_type_name, private_key_val_type_name, private_capacity)  \
    static_assert((private_capacity) > 1,                                      \
                  "fixed size map must have capacity greater than 1");         \
    typedef struct                                                             \
    {                                                                          \
        private_key_val_type_name data[(private_capacity)];                    \
        struct CCC_Handle_realtime_ordered_map_node nodes[(private_capacity)]; \
        typeof(*(struct CCC_Handle_realtime_ordered_map){}.parity)             \
            parity[CCC_private_handle_realtime_ordered_map_blocks(             \
                (private_capacity))];                                          \
    }(private_fixed_map_type_name)

/** @private Taking the size of the array actually works here because the field
is of a known fixed size defined at compile time, not just a pointer. */
#define CCC_private_handle_realtime_ordered_map_fixed_capacity(                \
    fixed_map_type_name)                                                       \
    (sizeof((fixed_map_type_name){}.nodes)                                     \
     / sizeof(struct CCC_Handle_realtime_ordered_map_node))

/** @private Initialization only tracks pointers to support a variety of memory
sources for both fixed and dynamic maps. The nodes and parity pointers will be
lazily initialized upon the first runtime opportunity. This allows the initial
memory provided to the data pointer to come from any source at compile or
runtime. */
#define CCC_private_handle_realtime_ordered_map_initialize(                    \
    private_memory_ptr, private_type_name, private_key_node_field,             \
    private_key_order_fn, private_allocate, private_context_data,              \
    private_capacity)                                                          \
    {                                                                          \
        .data = (private_memory_ptr),                                          \
        .nodes = NULL,                                                         \
        .parity = NULL,                                                        \
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
#define CCC_private_handle_realtime_ordered_map_as(                            \
    Handle_realtime_ordered_map_ptr, type_name, handle...)                     \
    ((type_name *)CCC_private_handle_realtime_ordered_map_data_at(             \
        (Handle_realtime_ordered_map_ptr), (handle)))

/*==================     Core Macro Implementations     =====================*/

/** @private */
#define CCC_private_handle_realtime_ordered_map_and_modify_w(                  \
    Handle_realtime_ordered_map_handle_ptr, type_name, closure_over_T...)      \
    (__extension__({                                                           \
        __auto_type private_handle_realtime_ordered_map_hndl_ptr               \
            = (Handle_realtime_ordered_map_handle_ptr);                        \
        struct CCC_Handle_realtime_ordered_map_handle                          \
            private_handle_realtime_ordered_map_mod_hndl                       \
            = {.stats = CCC_ENTRY_ARGUMENT_ERROR};                             \
        if (private_handle_realtime_ordered_map_hndl_ptr)                      \
        {                                                                      \
            private_handle_realtime_ordered_map_mod_hndl                       \
                = private_handle_realtime_ordered_map_hndl_ptr->private;       \
            if (private_handle_realtime_ordered_map_mod_hndl.stats             \
                & CCC_ENTRY_OCCUPIED)                                          \
            {                                                                  \
                type_name *const T                                             \
                    = CCC_private_handle_realtime_ordered_map_data_at(         \
                        private_handle_realtime_ordered_map_mod_hndl           \
                            .handle_realtime_ordered_map,                      \
                        private_handle_realtime_ordered_map_mod_hndl.i);       \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_handle_realtime_ordered_map_mod_hndl;                          \
    }))

/** @private */
#define CCC_private_handle_realtime_ordered_map_or_insert_w(                   \
    Handle_realtime_ordered_map_handle_ptr, lazy_key_value...)                 \
    (__extension__({                                                           \
        __auto_type private_or_ins_handle_ptr                                  \
            = (Handle_realtime_ordered_map_handle_ptr);                        \
        CCC_Handle_index private_handle_realtime_ordered_map_or_ins_ret = 0;   \
        if (private_or_ins_handle_ptr)                                         \
        {                                                                      \
            if (private_or_ins_handle_ptr->private.stats                       \
                == CCC_ENTRY_OCCUPIED)                                         \
            {                                                                  \
                private_handle_realtime_ordered_map_or_ins_ret                 \
                    = private_or_ins_handle_ptr->private.i;                    \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_handle_realtime_ordered_map_or_ins_ret                 \
                    = CCC_private_handle_realtime_ordered_map_allocate_slot(   \
                        private_or_ins_handle_ptr->private                     \
                            .handle_realtime_ordered_map);                     \
                if (private_handle_realtime_ordered_map_or_ins_ret)            \
                {                                                              \
                    *((typeof(lazy_key_value) *)                               \
                          CCC_private_handle_realtime_ordered_map_data_at(     \
                              private_or_ins_handle_ptr->private               \
                                  .handle_realtime_ordered_map,                \
                              private_handle_realtime_ordered_map_or_ins_ret)) \
                        = lazy_key_value;                                      \
                    CCC_private_handle_realtime_ordered_map_insert(            \
                        private_or_ins_handle_ptr->private                     \
                            .handle_realtime_ordered_map,                      \
                        private_or_ins_handle_ptr->private.i,                  \
                        private_or_ins_handle_ptr->private.last_order,         \
                        private_handle_realtime_ordered_map_or_ins_ret);       \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_handle_realtime_ordered_map_or_ins_ret;                        \
    }))

/** @private */
#define CCC_private_handle_realtime_ordered_map_insert_handle_w(                 \
    Handle_realtime_ordered_map_handle_ptr, lazy_key_value...)                   \
    (__extension__({                                                             \
        __auto_type private_ins_handle_ptr                                       \
            = (Handle_realtime_ordered_map_handle_ptr);                          \
        CCC_Handle_index private_handle_realtime_ordered_map_ins_hndl_ret = 0;   \
        if (private_ins_handle_ptr)                                              \
        {                                                                        \
            if (!(private_ins_handle_ptr->private.stats & CCC_ENTRY_OCCUPIED))   \
            {                                                                    \
                private_handle_realtime_ordered_map_ins_hndl_ret                 \
                    = CCC_private_handle_realtime_ordered_map_allocate_slot(     \
                        private_ins_handle_ptr->private                          \
                            .handle_realtime_ordered_map);                       \
                if (private_handle_realtime_ordered_map_ins_hndl_ret)            \
                {                                                                \
                    *((typeof(lazy_key_value) *)                                 \
                          CCC_private_handle_realtime_ordered_map_data_at(       \
                              private_ins_handle_ptr->private                    \
                                  .handle_realtime_ordered_map,                  \
                              private_handle_realtime_ordered_map_ins_hndl_ret)) \
                        = lazy_key_value;                                        \
                    CCC_private_handle_realtime_ordered_map_insert(              \
                        private_ins_handle_ptr->private                          \
                            .handle_realtime_ordered_map,                        \
                        private_ins_handle_ptr->private.i,                       \
                        private_ins_handle_ptr->private.last_order,              \
                        private_handle_realtime_ordered_map_ins_hndl_ret);       \
                }                                                                \
            }                                                                    \
            else if (private_ins_handle_ptr->private.stats                       \
                     == CCC_ENTRY_OCCUPIED)                                      \
            {                                                                    \
                private_handle_realtime_ordered_map_ins_hndl_ret                 \
                    = private_ins_handle_ptr->private.i;                         \
                *((typeof(lazy_key_value) *)                                     \
                      CCC_private_handle_realtime_ordered_map_data_at(           \
                          private_ins_handle_ptr->private                        \
                              .handle_realtime_ordered_map,                      \
                          private_handle_realtime_ordered_map_ins_hndl_ret))     \
                    = lazy_key_value;                                            \
            }                                                                    \
        }                                                                        \
        private_handle_realtime_ordered_map_ins_hndl_ret;                        \
    }))

/** @private */
#define CCC_private_handle_realtime_ordered_map_try_insert_w(                      \
    Handle_realtime_ordered_map_ptr, key, lazy_value...)                           \
    (__extension__({                                                               \
        __auto_type private_try_ins_map_ptr                                        \
            = (Handle_realtime_ordered_map_ptr);                                   \
        struct CCC_Handle private_handle_realtime_ordered_map_try_ins_hndl_ret     \
            = {.stats = CCC_ENTRY_ARGUMENT_ERROR};                                 \
        if (private_try_ins_map_ptr)                                               \
        {                                                                          \
            __auto_type private_handle_realtime_ordered_map_key = (key);           \
            struct CCC_Handle_realtime_ordered_map_handle                          \
                private_handle_realtime_ordered_map_try_ins_hndl                   \
                = CCC_private_handle_realtime_ordered_map_handle(                  \
                    private_try_ins_map_ptr,                                       \
                    (void *)&private_handle_realtime_ordered_map_key);             \
            if (!(private_handle_realtime_ordered_map_try_ins_hndl.stats           \
                  & CCC_ENTRY_OCCUPIED))                                           \
            {                                                                      \
                private_handle_realtime_ordered_map_try_ins_hndl_ret               \
                    = (struct CCC_Handle){                                         \
                        .i                                                         \
                        = CCC_private_handle_realtime_ordered_map_allocate_slot(   \
                            private_handle_realtime_ordered_map_try_ins_hndl       \
                                .handle_realtime_ordered_map),                     \
                        .stats = CCC_ENTRY_INSERT_ERROR,                           \
                    };                                                             \
                if (private_handle_realtime_ordered_map_try_ins_hndl_ret.i)        \
                {                                                                  \
                    *((typeof(lazy_value) *)                                       \
                          CCC_private_handle_realtime_ordered_map_data_at(         \
                              private_try_ins_map_ptr,                             \
                              private_handle_realtime_ordered_map_try_ins_hndl_ret \
                                  .i))                                             \
                        = lazy_value;                                              \
                    *((typeof(private_handle_realtime_ordered_map_key) *)          \
                          CCC_private_handle_realtime_ordered_map_key_at(          \
                              private_try_ins_map_ptr,                             \
                              private_handle_realtime_ordered_map_try_ins_hndl_ret \
                                  .i))                                             \
                        = private_handle_realtime_ordered_map_key;                 \
                    CCC_private_handle_realtime_ordered_map_insert(                \
                        private_handle_realtime_ordered_map_try_ins_hndl           \
                            .handle_realtime_ordered_map,                          \
                        private_handle_realtime_ordered_map_try_ins_hndl.i,        \
                        private_handle_realtime_ordered_map_try_ins_hndl           \
                            .last_order,                                           \
                        private_handle_realtime_ordered_map_try_ins_hndl_ret       \
                            .i);                                                   \
                    private_handle_realtime_ordered_map_try_ins_hndl_ret.stats     \
                        = CCC_ENTRY_VACANT;                                        \
                }                                                                  \
            }                                                                      \
            else if (private_handle_realtime_ordered_map_try_ins_hndl.stats        \
                     == CCC_ENTRY_OCCUPIED)                                        \
            {                                                                      \
                private_handle_realtime_ordered_map_try_ins_hndl_ret               \
                    = (struct CCC_Handle){                                         \
                        .i                                                         \
                        = private_handle_realtime_ordered_map_try_ins_hndl.i,      \
                        .stats                                                     \
                        = private_handle_realtime_ordered_map_try_ins_hndl         \
                              .stats,                                              \
                    };                                                             \
            }                                                                      \
        }                                                                          \
        private_handle_realtime_ordered_map_try_ins_hndl_ret;                      \
    }))

/** @private */
#define CCC_private_handle_realtime_ordered_map_insert_or_assign_w(                      \
    Handle_realtime_ordered_map_ptr, key, lazy_value...)                                 \
    (__extension__({                                                                     \
        __auto_type private_ins_or_assign_map_ptr                                        \
            = (Handle_realtime_ordered_map_ptr);                                         \
        struct CCC_Handle                                                                \
            private_handle_realtime_ordered_map_ins_or_assign_hndl_ret                   \
            = {.stats = CCC_ENTRY_ARGUMENT_ERROR};                                       \
        if (private_ins_or_assign_map_ptr)                                               \
        {                                                                                \
            __auto_type private_handle_realtime_ordered_map_key = (key);                 \
            struct CCC_Handle_realtime_ordered_map_handle                                \
                private_handle_realtime_ordered_map_ins_or_assign_hndl                   \
                = CCC_private_handle_realtime_ordered_map_handle(                        \
                    private_ins_or_assign_map_ptr,                                       \
                    (void *)&private_handle_realtime_ordered_map_key);                   \
            if (!(private_handle_realtime_ordered_map_ins_or_assign_hndl.stats           \
                  & CCC_ENTRY_OCCUPIED))                                                 \
            {                                                                            \
                private_handle_realtime_ordered_map_ins_or_assign_hndl_ret               \
                    = (struct CCC_Handle){                                               \
                        .i                                                               \
                        = CCC_private_handle_realtime_ordered_map_allocate_slot(         \
                            private_handle_realtime_ordered_map_ins_or_assign_hndl       \
                                .handle_realtime_ordered_map),                           \
                        .stats = CCC_ENTRY_INSERT_ERROR,                                 \
                    };                                                                   \
                if (private_handle_realtime_ordered_map_ins_or_assign_hndl_ret           \
                        .i)                                                              \
                {                                                                        \
                    *((typeof(lazy_value) *)                                             \
                          CCC_private_handle_realtime_ordered_map_data_at(               \
                              private_handle_realtime_ordered_map_ins_or_assign_hndl     \
                                  .handle_realtime_ordered_map,                          \
                              private_handle_realtime_ordered_map_ins_or_assign_hndl_ret \
                                  .i))                                                   \
                        = lazy_value;                                                    \
                    *((typeof(private_handle_realtime_ordered_map_key) *)                \
                          CCC_private_handle_realtime_ordered_map_key_at(                \
                              private_handle_realtime_ordered_map_ins_or_assign_hndl     \
                                  .handle_realtime_ordered_map,                          \
                              private_handle_realtime_ordered_map_ins_or_assign_hndl_ret \
                                  .i))                                                   \
                        = private_handle_realtime_ordered_map_key;                       \
                    CCC_private_handle_realtime_ordered_map_insert(                      \
                        private_handle_realtime_ordered_map_ins_or_assign_hndl           \
                            .handle_realtime_ordered_map,                                \
                        private_handle_realtime_ordered_map_ins_or_assign_hndl           \
                            .i,                                                          \
                        private_handle_realtime_ordered_map_ins_or_assign_hndl           \
                            .last_order,                                                 \
                        private_handle_realtime_ordered_map_ins_or_assign_hndl_ret       \
                            .i);                                                         \
                    private_handle_realtime_ordered_map_ins_or_assign_hndl_ret           \
                        .stats                                                           \
                        = CCC_ENTRY_VACANT;                                              \
                }                                                                        \
            }                                                                            \
            else if (private_handle_realtime_ordered_map_ins_or_assign_hndl              \
                         .stats                                                          \
                     == CCC_ENTRY_OCCUPIED)                                              \
            {                                                                            \
                *((typeof(lazy_value) *)                                                 \
                      CCC_private_handle_realtime_ordered_map_data_at(                   \
                          private_handle_realtime_ordered_map_ins_or_assign_hndl         \
                              .handle_realtime_ordered_map,                              \
                          private_handle_realtime_ordered_map_ins_or_assign_hndl         \
                              .i))                                                       \
                    = lazy_value;                                                        \
                private_handle_realtime_ordered_map_ins_or_assign_hndl_ret               \
                    = (struct CCC_Handle){                                               \
                        .i                                                               \
                        = private_handle_realtime_ordered_map_ins_or_assign_hndl         \
                              .i,                                                        \
                        .stats                                                           \
                        = private_handle_realtime_ordered_map_ins_or_assign_hndl         \
                              .stats,                                                    \
                    };                                                                   \
                *((typeof(private_handle_realtime_ordered_map_key) *)                    \
                      CCC_private_handle_realtime_ordered_map_key_at(                    \
                          private_handle_realtime_ordered_map_ins_or_assign_hndl         \
                              .handle_realtime_ordered_map,                              \
                          private_handle_realtime_ordered_map_ins_or_assign_hndl         \
                              .i))                                                       \
                    = private_handle_realtime_ordered_map_key;                           \
            }                                                                            \
        }                                                                                \
        private_handle_realtime_ordered_map_ins_or_assign_hndl_ret;                      \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* PRIVATE_HANDLE_REALTIME_ORDERED_MAP_H */
