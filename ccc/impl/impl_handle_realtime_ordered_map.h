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
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../types.h"
#include "impl_types.h" /* NOLINT */

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
struct ccc_romap_elem
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
struct ccc_hromap
{
    /** @private The contiguous array of user data. */
    void *data;
    /** @private The contiguous array of WAVL tree meta data. */
    struct ccc_hromap_elem *nodes;
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
    ccc_any_key_cmp_fn *cmp;
    /** @private The provided allocation function, if any. */
    ccc_any_alloc_fn *alloc;
    /** @private The provided auxiliary data, if any. */
    void *aux;
};

/** @private */
struct ccc_hrtree_handle
{
    /** @private Map associated with this handle. */
    struct ccc_hromap *hrm;
    /** @private Current index of the handle. */
    size_t i;
    /** @private Saves last comparison direction. */
    ccc_threeway_cmp last_cmp;
    /** @private The entry status flag. */
    ccc_entry_status stats;
};

/** @private Wrapper for return by pointer on the stack in C23. */
union ccc_hromap_handle
{
    /** @private Single field to enable return by compound literal reference. */
    struct ccc_hrtree_handle impl;
};

/*========================  Private Interface  ==============================*/

/** @private */
void *ccc_impl_hrm_data_at(struct ccc_hromap const *hrm, size_t slot);
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

/** @private Calculates the number of parity blocks needed to support the given
capacity. Provide the type used for the parity block array and the number of
blocks needed to round up to will be returned. */
#define ccc_impl_hrm_blocks(impl_cap)                                          \
    (((impl_cap) + ((sizeof(*(struct ccc_hromap){}.parity) * CHAR_BIT) - 1))   \
     / (sizeof(*(struct ccc_hromap){}.parity) * CHAR_BIT))

/** @private The user can declare a fixed size realtime ordered map with the
help of static asserts to ensure the layout is compatible with our internal
metadata. */
#define ccc_impl_hrm_declare_fixed_map(impl_fixed_map_type_name,               \
                                       impl_key_val_type_name, impl_capacity)  \
    static_assert((impl_capacity) > 1,                                         \
                  "fixed size map must have capacity greater than 1");         \
    typedef struct                                                             \
    {                                                                          \
        impl_key_val_type_name data[(impl_capacity)];                          \
        struct ccc_hromap_elem nodes[(impl_capacity)];                         \
        typeof(*(struct ccc_hromap){}                                          \
                    .parity) parity[ccc_impl_hrm_blocks((impl_capacity))];     \
    }(impl_fixed_map_type_name)

/** @private Taking the size of the array actually works here because the field
is of a known fixed size defined at compile time, not just a pointer. */
#define ccc_impl_hrm_fixed_capacity(fixed_map_type_name)                       \
    (sizeof((fixed_map_type_name){}.nodes) / sizeof(struct ccc_hromap_elem))

/** @private Initialization only tracks pointers to support a variety of memory
sources for both fixed and dynamic maps. The nodes and parity pointers will be
lazily initialized upon the first runtime opportunity. This allows the initial
memory provided to the data pointer to come from any source at compile or
runtime. */
#define ccc_impl_hrm_init(impl_memory_ptr, impl_type_name,                     \
                          impl_key_elem_field, impl_key_cmp_fn, impl_alloc_fn, \
                          impl_aux_data, impl_capacity)                        \
    {                                                                          \
        .data = (impl_memory_ptr),                                             \
        .nodes = NULL,                                                         \
        .parity = NULL,                                                        \
        .capacity = (impl_capacity),                                           \
        .count = 0,                                                            \
        .root = 0,                                                             \
        .free_list = 0,                                                        \
        .sizeof_type = sizeof(impl_type_name),                                 \
        .key_offset = offsetof(impl_type_name, impl_key_elem_field),           \
        .cmp = (impl_key_cmp_fn),                                              \
        .alloc = (impl_alloc_fn),                                              \
        .aux = (impl_aux_data),                                                \
    }

/** @private */
#define ccc_impl_hrm_as(handle_realtime_ordered_map_ptr, type_name, handle...) \
    ((type_name *)ccc_impl_hrm_data_at((handle_realtime_ordered_map_ptr),      \
                                       (handle)))

/*==================     Core Macro Implementations     =====================*/

/** @private */
#define ccc_impl_hrm_and_modify_w(handle_realtime_ordered_map_handle_ptr,      \
                                  type_name, closure_over_T...)                \
    (__extension__({                                                           \
        __auto_type impl_hrm_hndl_ptr                                          \
            = (handle_realtime_ordered_map_handle_ptr);                        \
        struct ccc_hrtree_handle impl_hrm_mod_hndl                             \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_hrm_hndl_ptr)                                                 \
        {                                                                      \
            impl_hrm_mod_hndl = impl_hrm_hndl_ptr->impl;                       \
            if (impl_hrm_mod_hndl.stats & CCC_ENTRY_OCCUPIED)                  \
            {                                                                  \
                type_name *const T = ccc_impl_hrm_data_at(                     \
                    impl_hrm_mod_hndl.hrm, impl_hrm_mod_hndl.i);               \
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
            if (impl_hrm_or_ins_hndl->stats == CCC_ENTRY_OCCUPIED)             \
            {                                                                  \
                impl_hrm_or_ins_ret = impl_hrm_or_ins_hndl->i;                 \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_hrm_or_ins_ret                                            \
                    = ccc_impl_hrm_alloc_slot(impl_hrm_or_ins_hndl->hrm);      \
                if (impl_hrm_or_ins_ret)                                       \
                {                                                              \
                    *((typeof(lazy_key_value) *)ccc_impl_hrm_data_at(          \
                        impl_hrm_or_ins_hndl->hrm, impl_hrm_or_ins_ret))       \
                        = lazy_key_value;                                      \
                    ccc_impl_hrm_insert(                                       \
                        impl_hrm_or_ins_hndl->hrm, impl_hrm_or_ins_hndl->i,    \
                        impl_hrm_or_ins_hndl->last_cmp, impl_hrm_or_ins_ret);  \
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
            if (!(impl_hrm_ins_hndl->stats & CCC_ENTRY_OCCUPIED))              \
            {                                                                  \
                impl_hrm_ins_hndl_ret                                          \
                    = ccc_impl_hrm_alloc_slot(impl_hrm_ins_hndl->hrm);         \
                if (impl_hrm_ins_hndl_ret)                                     \
                {                                                              \
                    *((typeof(lazy_key_value) *)ccc_impl_hrm_data_at(          \
                        impl_hrm_ins_hndl->hrm, impl_hrm_ins_hndl_ret))        \
                        = lazy_key_value;                                      \
                    ccc_impl_hrm_insert(                                       \
                        impl_hrm_ins_hndl->hrm, impl_hrm_ins_hndl->i,          \
                        impl_hrm_ins_hndl->last_cmp, impl_hrm_ins_hndl_ret);   \
                }                                                              \
            }                                                                  \
            else if (impl_hrm_ins_hndl->stats == CCC_ENTRY_OCCUPIED)           \
            {                                                                  \
                impl_hrm_ins_hndl_ret = impl_hrm_ins_hndl->i;                  \
                *((typeof(lazy_key_value) *)ccc_impl_hrm_data_at(              \
                    impl_hrm_ins_hndl->hrm, impl_hrm_ins_hndl_ret))            \
                    = lazy_key_value;                                          \
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
            if (!(impl_hrm_try_ins_hndl.stats & CCC_ENTRY_OCCUPIED))           \
            {                                                                  \
                impl_hrm_try_ins_hndl_ret = (struct ccc_handl){                \
                    .i = ccc_impl_hrm_alloc_slot(impl_hrm_try_ins_hndl.hrm),   \
                    .stats = CCC_ENTRY_INSERT_ERROR,                           \
                };                                                             \
                if (impl_hrm_try_ins_hndl_ret.i)                               \
                {                                                              \
                    *((typeof(lazy_value) *)ccc_impl_hrm_data_at(              \
                        impl_try_ins_map_ptr, impl_hrm_try_ins_hndl_ret.i))    \
                        = lazy_value;                                          \
                    *((typeof(impl_hrm_key) *)ccc_impl_hrm_key_at(             \
                        impl_try_ins_map_ptr, impl_hrm_try_ins_hndl_ret.i))    \
                        = impl_hrm_key;                                        \
                    ccc_impl_hrm_insert(impl_hrm_try_ins_hndl.hrm,             \
                                        impl_hrm_try_ins_hndl.i,               \
                                        impl_hrm_try_ins_hndl.last_cmp,        \
                                        impl_hrm_try_ins_hndl_ret.i);          \
                    impl_hrm_try_ins_hndl_ret.stats = CCC_ENTRY_VACANT;        \
                }                                                              \
            }                                                                  \
            else if (impl_hrm_try_ins_hndl.stats == CCC_ENTRY_OCCUPIED)        \
            {                                                                  \
                impl_hrm_try_ins_hndl_ret = (struct ccc_handl){                \
                    .i = impl_hrm_try_ins_hndl.i,                              \
                    .stats = impl_hrm_try_ins_hndl.stats,                      \
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
            if (!(impl_hrm_ins_or_assign_hndl.stats & CCC_ENTRY_OCCUPIED))     \
            {                                                                  \
                impl_hrm_ins_or_assign_hndl_ret = (struct ccc_handl){          \
                    .i = ccc_impl_hrm_alloc_slot(                              \
                        impl_hrm_ins_or_assign_hndl.hrm),                      \
                    .stats = CCC_ENTRY_INSERT_ERROR,                           \
                };                                                             \
                if (impl_hrm_ins_or_assign_hndl_ret.i)                         \
                {                                                              \
                    *((typeof(lazy_value) *)ccc_impl_hrm_data_at(              \
                        impl_hrm_ins_or_assign_hndl.hrm,                       \
                        impl_hrm_ins_or_assign_hndl_ret.i))                    \
                        = lazy_value;                                          \
                    *((typeof(impl_hrm_key) *)ccc_impl_hrm_key_at(             \
                        impl_hrm_ins_or_assign_hndl.hrm,                       \
                        impl_hrm_ins_or_assign_hndl_ret.i))                    \
                        = impl_hrm_key;                                        \
                    ccc_impl_hrm_insert(impl_hrm_ins_or_assign_hndl.hrm,       \
                                        impl_hrm_ins_or_assign_hndl.i,         \
                                        impl_hrm_ins_or_assign_hndl.last_cmp,  \
                                        impl_hrm_ins_or_assign_hndl_ret.i);    \
                    impl_hrm_ins_or_assign_hndl_ret.stats = CCC_ENTRY_VACANT;  \
                }                                                              \
            }                                                                  \
            else if (impl_hrm_ins_or_assign_hndl.stats == CCC_ENTRY_OCCUPIED)  \
            {                                                                  \
                *((typeof(lazy_value) *)ccc_impl_hrm_data_at(                  \
                    impl_hrm_ins_or_assign_hndl.hrm,                           \
                    impl_hrm_ins_or_assign_hndl.i))                            \
                    = lazy_value;                                              \
                impl_hrm_ins_or_assign_hndl_ret = (struct ccc_handl){          \
                    .i = impl_hrm_ins_or_assign_hndl.i,                        \
                    .stats = impl_hrm_ins_or_assign_hndl.stats,                \
                };                                                             \
                *((typeof(impl_hrm_key) *)ccc_impl_hrm_key_at(                 \
                    impl_hrm_ins_or_assign_hndl.hrm,                           \
                    impl_hrm_ins_or_assign_hndl.i))                            \
                    = impl_hrm_key;                                            \
            }                                                                  \
        }                                                                      \
        impl_hrm_ins_or_assign_hndl_ret;                                       \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* IMPL_HANDLE_REALTIME_ORDERED_MAP_H */
