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
/** @internal
@file
@brief Private Flat Hash Map Interface

This flat hash map is a C Container Collection friendly interpretation of the
Rust Hashbrown hash table. This in turn is based on the Abseil flat hash table
from Google in C++. I simplified and modified the implementation for maximum
readability in one header and one file. Tracking how to manage different
platform implementations of groups and metadata fingerprint masks should be
much easier this way, rather than jumping across countless small implementation
files.

One key feature that is rigorously tested via static asserts is the ability
to create a static data segment or stack based map. This is a key feature of
the implementation but it requires significant set up ahead of time and lazy
initialization support. The lazy initialization presents the map with the
most complexity in the implementation. */
#ifndef CCC_PRIVATE_FLAT_HASH_MAP_H
#define CCC_PRIVATE_FLAT_HASH_MAP_H

/** @cond */
#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../types.h"
#include "private_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @internal If we only make these complex checks once, it is easier to read
and used the source code during all the platform based implementations. */
#if defined(__x86_64) && defined(__SSE2__)                                     \
    && !defined(CCC_FLAT_HASH_MAP_PORTABLE)
/** @internal Internal container collection detection for SIMD instructions on
the x86 architectures. This will be the most efficient version possible
offering the widest group matching. */
#    define CCC_HAS_X86_SIMD
#elif defined(__ARM_NEON__) && !defined(CCC_FLAT_HASH_MAP_PORTABLE)
/** @internal Internal container collection detection for SIMD instructions on
the NEON architecture. This implementation currently lacks some of the features
of the x86 SIMD version but should still be fast. */
#    define CCC_HAS_ARM_SIMD
#endif /* defined(__x86_64)&&defined(__SSE2__)&&!defined(CCC_FLAT_HASH_MAP_PORTABLE) \
        */
/** else we define nothing and the portable fallback will take effect. */

/** @internal An array of this byte will be in the tag array. Same idea as
Rust's Hashbrown table. The only value not represented by constants is
the following:

`DELETED  = 0b10000000`
`EMPTY    = 0x11111111`
`OCCUPIED = 0b0???????`

In this case `?` represents any 7 bits kept from the upper 7 bits of the
original hash code to signify an occupied slot. We know this slot is taken
because the Most Significant Bit is zero, something that is not true of any
other state. Wrap a byte in a struct to avoid strict-aliasing exceptions that
are granted to `uint8_t` (usually unsigned char) and `int8_t` (usually char)
when passed to functions as pointers. Maybe nets performance gain but depends on
aggressiveness of compiler. */
struct CCC_Flat_hash_map_tag
{
    /** Can be set to DELETED or EMPTY or an arbitrary hash 0b0???????. */
    uint8_t v;
};

/** @internal Vectorized group scanning allows more parallel scans but a
fallback of 8 is good for a portable implementation that will use the widest
word on a platform for group scanning. Right now, this lib targets 64-bit so
that means uint64_t is widest default integer widely supported. That width
is still valid on 32-bit but probably very slow due to emulation. */
enum : typeof((struct CCC_Flat_hash_map_tag){}.v)
{
#ifdef CCC_HAS_X86_SIMD
    /** A group of tags that can be loaded into a 128 bit vector. */
    CCC_FLAT_HASH_MAP_GROUP_SIZE = 16,
#elifdef CCC_HAS_ARM_SIMD
    /** A group of tags that can be loded into a 64 bit integer. */
    CCC_FLAT_HASH_MAP_GROUP_SIZE = 8,
#else  /* PORTABLE FALLBACK */
    /** A group of tags that can be loded into a 64 bit integer. */
    CCC_FLAT_HASH_MAP_GROUP_SIZE = 8,
#endif /* defined(CCC_HAS_X86_SIMD) */
};

/** @internal The layout of the map uses only pointers to account for the
possibility of memory provided from the data segment, stack, or heap. When the
map is allowed to allocate it will take care of aligning pointers appropriately.
In the fixed size case we rely on the user defining a fixed size type. In either
case the arrays are in one contiguous allocation but split as follows:

(N == capacity - 1) Where capacity is a power of 2. (G == group size - 1).

┌───┬───┬───┬───┬────┬───┬───┬───┬───┬───┬───┬───┬───┐
│D_0│D_1│...│D_N│Swap│T_0│T_1│...│T_N│R_0│R_1│...│R_G│
└───┴───┴───┴───┴─┬──┼───┴───┴───┴───┼───┴───┴───┴───┘
  ┌───────────────┘  │               │
┌─┴───────────┐ ┌────┴─────────┐ ┌───┴──────────────────────────────────────┐
│Swap slot for│ │Base address  │ │Start of replica of first group to support│
│in place     │ │of Tag array. │ │a group load that starts at T_N as well as│
│rehashing.   │ │Possible pad  │ │erase and inserts. This means R_G is never│
│Size = 1 data│ │bytes between.│ │needed but duplicated for branchless ops. │
└─────────────┘ └──────────────┘ └──────────────────────────────────────────┘

This is a different layout than Rust's Hashbrown table. Instead of a shared
base address of the data and tag arrays with padding at the start of the data
array, we use a normal two array layout with padding between. This is because
we must allow the user to allocate a hash table at compile time in the data
segment or on the stack. The fixed size map defined at compile time is a struct
and structs in the C standard are specified to have no padding at the start.
Therefore, so that the code within the map does not care whether it deals with
a fixed or dynamic map, we must assume data starts at the base address and that
there may be zero or more bytes of padding between the data and tag arrays for
alignment.

We may lose some of the assembly optimizations in indexing that Rust's table
gets by adding and subtracting from a shared base address. However, this table
still needs to use byte offset multiplication because the data is stored as
`void *` so we don't have the same optimizations available to us as Rust's
template generic system. Simple 0 based indexing makes the addition and
multiplication we perform as simple as possible. */
struct CCC_Flat_hash_map
{
    /** Reversed user type data array. */
    void *data;
    /** Tag array on byte following data(0). */
    struct CCC_Flat_hash_map_tag *tag;
    /** The number of user active slots. */
    size_t count;
    /** Track available slots given load factor constrains. When 0, rehash. */
    size_t remain;
    /** The mask for power of two table sizing. */
    size_t mask;
    /** Size of each user data element being stored. */
    size_t sizeof_type;
    /** The location of the key field in user type. */
    size_t key_offset;
    /** The user callback for equality comparison. */
    CCC_Key_comparator *compare;
    /** The hash function provided by user. */
    CCC_Key_hasher *hash;
    /** The allocation function, if any. */
    CCC_Allocator *allocate;
    /** Auxiliary data, if any. */
    void *context;
};

/** @internal A struct for containing all relevant information for a query
into one object so that passing to future functions is cleaner. */
struct CCC_Flat_hash_map_entry
{
    /** The map associated with this entry. */
    struct CCC_Flat_hash_map *map;
    /** The index in the data/tag array of this entry. */
    size_t index;
    /** The saved tag from the current query hash value. */
    struct CCC_Flat_hash_map_tag tag;
    /** The status of this entry. */
    enum CCC_Entry_status status;
};

/** @internal A simple wrapper for an entry that allows us to return a compound
literal reference. All interface functions accept pointers to entries and
a functional chain of calls is not possible with return by value. The interface
can then return `&(union
CCC_Flat_hash_map_entry_wrap){function_call(...).private}` which is a compound
literal reference in C23. */
union CCC_Flat_hash_map_entry_wrap
{
    /** @internal Wrapped type to make compound literal reference easy. */
    struct CCC_Flat_hash_map_entry private;
};

/** While the private interface functions are not strictly necessary containing
the logic of interacting with the map to the source implementation makes
reasoning and debugging the macros easier. It also cuts down on repeated logic.
*/
/*======================     Private Interface      =========================*/

struct CCC_Flat_hash_map_entry
CCC_private_flat_hash_map_entry(struct CCC_Flat_hash_map *, void const *);
void CCC_private_flat_hash_map_insert(struct CCC_Flat_hash_map *, void const *,
                                      struct CCC_Flat_hash_map_tag, size_t);
void CCC_private_flat_hash_map_erase(struct CCC_Flat_hash_map *, size_t);
void *CCC_private_flat_hash_map_data_at(struct CCC_Flat_hash_map const *,
                                        size_t);
void *CCC_private_flat_hash_map_key_at(struct CCC_Flat_hash_map const *,
                                       size_t);
void
CCC_private_flat_hash_map_set_insert(struct CCC_Flat_hash_map_entry const *);

/*======================    Macro Implementations   =========================*/

/** @internal Helps the user declare a type for a fixed size map. They can then
use this type when they want a hash map as global, static global, or stack
local. They would need to define their fixed size type every time but that
should be fine as they are likely to only declare one or two. They would likely
only have a one fixed size map per translation unit if they are using these
capabilities. They control the name of the type so they can organize types as
they wish.

The declaration specifies that we have one extra data slot for swapping during
in place rehashing and some interface functions and an extra duplicate group
of tags at the end of the tag array for safer group loading.

Finally, we must align the tag array to start on an aligned group size byte
boundary to be able to perform aligned loads and stores. */
#define CCC_private_flat_hash_map_declare_fixed_map(                           \
    fixed_map_type_name, key_val_type_name, capacity)                          \
    static_assert((capacity) > 0,                                              \
                  "fixed size map must have capacity greater than 0");         \
    static_assert(                                                             \
        (capacity) >= CCC_FLAT_HASH_MAP_GROUP_SIZE,                            \
        "fixed size map must have capacity >= CCC_FLAT_HASH_MAP_GROUP_SIZE "   \
        "(8 or 16 depending on platform)");                                    \
    static_assert(((capacity) & ((capacity) - 1)) == 0,                        \
                  "fixed size map must be a power of 2 capacity (32, 64, "     \
                  "128, 256, etc.)");                                          \
    typedef struct                                                             \
    {                                                                          \
        key_val_type_name data[(capacity) + 1];                                \
        alignas(CCC_FLAT_HASH_MAP_GROUP_SIZE) struct CCC_Flat_hash_map_tag     \
            tag[(capacity) + CCC_FLAT_HASH_MAP_GROUP_SIZE];                    \
    }(fixed_map_type_name)

/** @internal If the user does not want to remember the capacity they chose
for their type or make mistakes this macro offers consistent calculation of
total capacity (aka buckets) of the map. This is not the capacity that is
limited by load factor.

The sizeof operator does not decay to a simple pointer here because the tag
array of a fixed type has a length known at compile time. Also the tag array is
simple byte sized chunks so no division needed. See earlier static asserts for
how we ensure no fixed size type is allowed to be defined in a way to make this
call unsafe. */
#define CCC_private_flat_hash_map_fixed_capacity(fixed_map_type_name)          \
    (sizeof((fixed_map_type_name){}.tag) - CCC_FLAT_HASH_MAP_GROUP_SIZE)

/** @internal Initialization is tricky but we simplify by only accepting a
pointer to the map this pointer could be any of the following.

    - The address of a user defined fixed size map stored in data segment.
    - The address of a user defined fixed size map stored on the stack.
    - The address of a user defined fixed size map allocated on the heap.
    - NULL if the user intends for a dynamic map.

All of the above cases are covered by accepting the pointer at .data and only
evaluating the argument once. This also allows the user to pass a compound
literal to the first argument and eliminate any dangling references, such as
`&(static user_defined_map_type){}`. However, to accept a map from all of these
sources at compile or runtime, we must implement lazy initialization. This is
because we can't initialize the tag array at compile time. By setting the tag
field to NULL we will be able to tell if our map is initialized whether it is
fixed size and has data or is dynamic and has not yet been given allocation. */
#define CCC_private_flat_hash_map_initialize(                                  \
    private_fixed_map_pointer, private_type_name, private_key_field,           \
    private_hash, private_key_compare, private_allocate, private_context_data, \
    private_capacity)                                                          \
    {                                                                          \
        .data = (private_fixed_map_pointer),                                   \
        .tag = NULL,                                                           \
        .count = 0,                                                            \
        .remain = (((private_capacity) / (size_t)8) * (size_t)7),              \
        .mask                                                                  \
        = (((private_capacity) > (size_t)0) ? ((private_capacity) - (size_t)1) \
                                            : (size_t)0),                      \
        .sizeof_type = sizeof(private_type_name),                              \
        .key_offset = offsetof(private_type_name, private_key_field),          \
        .compare = (private_key_compare),                                      \
        .hash = (private_hash),                                                \
        .allocate = (private_allocate),                                        \
        .context = (private_context_data),                                     \
    }

/** @internal Initialize  dynamic container with a compound literal array. */
#define CCC_private_flat_hash_map_from(                                        \
    private_key_field, private_hash, private_key_compare, private_allocate,    \
    private_context_data, private_optional_cap,                                \
    private_array_compound_literal...)                                         \
    (__extension__({                                                           \
        typeof(*private_array_compound_literal)                                \
            *private_flat_hash_map_initializer_list                            \
            = private_array_compound_literal;                                  \
        struct CCC_Flat_hash_map private_map                                   \
            = CCC_private_flat_hash_map_initialize(                            \
                NULL, typeof(*private_flat_hash_map_initializer_list),         \
                private_key_field, private_hash, private_key_compare,          \
                private_allocate, private_context_data, 0);                    \
        size_t const private_n                                                 \
            = sizeof(private_array_compound_literal)                           \
            / sizeof(*private_flat_hash_map_initializer_list);                 \
        size_t const private_cap = private_optional_cap;                       \
        if (CCC_flat_hash_map_reserve(                                         \
                &private_map,                                                  \
                (private_n > private_cap ? private_n : private_cap),           \
                private_allocate)                                              \
            == CCC_RESULT_OK)                                                  \
        {                                                                      \
            for (size_t i = 0; i < private_n; ++i)                             \
            {                                                                  \
                struct CCC_Flat_hash_map_entry private_ent                     \
                    = CCC_private_flat_hash_map_entry(                         \
                        &private_map,                                          \
                        (void const                                            \
                             *)&private_flat_hash_map_initializer_list[i]      \
                            .private_key_field);                               \
                if (!(private_ent.status & CCC_ENTRY_INSERT_ERROR))            \
                {                                                              \
                    *((typeof(*private_flat_hash_map_initializer_list) *)      \
                          CCC_private_flat_hash_map_data_at(                   \
                              private_ent.map, private_ent.index))             \
                        = private_flat_hash_map_initializer_list[i];           \
                    if (private_ent.status == CCC_ENTRY_VACANT)                \
                    {                                                          \
                        CCC_private_flat_hash_map_set_insert(&private_ent);    \
                    }                                                          \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_map;                                                           \
    }))

/** @internal Initializes the flat hash map with the specified capacity. */
#define CCC_private_flat_hash_map_with_capacity(                               \
    private_type_name, private_key_field, private_hash, private_key_compare,   \
    private_allocate, private_context_data, private_cap)                       \
    (__extension__({                                                           \
        struct CCC_Flat_hash_map private_map                                   \
            = CCC_private_flat_hash_map_initialize(                            \
                NULL, private_type_name, private_key_field, private_hash,      \
                private_key_compare, private_allocate, private_context_data,   \
                0);                                                            \
        (void)CCC_flat_hash_map_reserve(&private_map, private_cap,             \
                                        private_allocate);                     \
        private_map;                                                           \
    }))

/*========================    Construct In Place    =========================*/

/** @internal A fairly good approximation of closures given C23 capabilities.
The user facing docs clarify that T is a correctly typed reference to the
desired data if occupied. */
#define CCC_private_flat_hash_map_and_modify_with(                             \
    Flat_hash_map_entry_pointer, type_name, closure_over_T...)                 \
    (__extension__({                                                           \
        __auto_type private_flat_hash_map_mod_ent_pointer                      \
            = (Flat_hash_map_entry_pointer);                                   \
        struct CCC_Flat_hash_map_entry private_flat_hash_map_mod_with_ent      \
            = {.status = CCC_ENTRY_ARGUMENT_ERROR};                            \
        if (private_flat_hash_map_mod_ent_pointer)                             \
        {                                                                      \
            private_flat_hash_map_mod_with_ent                                 \
                = private_flat_hash_map_mod_ent_pointer->private;              \
            if (private_flat_hash_map_mod_with_ent.status                      \
                & CCC_ENTRY_OCCUPIED)                                          \
            {                                                                  \
                type_name *const T = CCC_private_flat_hash_map_data_at(        \
                    private_flat_hash_map_mod_with_ent.map,                    \
                    private_flat_hash_map_mod_with_ent.index);                 \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_flat_hash_map_mod_with_ent;                                    \
    }))

/** @internal The or insert method is unique in that it directly returns a
reference to the inserted data rather than a entry with a status. This is
because it should not fail. If NULL is returned the user knows there is a
problem. */
#define CCC_private_flat_hash_map_or_insert_with(Flat_hash_map_entry_pointer,  \
                                                 type_compound_literal...)     \
    (__extension__({                                                           \
        __auto_type private_flat_hash_map_or_ins_ent_pointer                   \
            = (Flat_hash_map_entry_pointer);                                   \
        typeof(type_compound_literal) *private_flat_hash_map_or_ins_res        \
            = NULL;                                                            \
        if (private_flat_hash_map_or_ins_ent_pointer)                          \
        {                                                                      \
            if (!(private_flat_hash_map_or_ins_ent_pointer->private.status     \
                  & CCC_ENTRY_INSERT_ERROR))                                   \
            {                                                                  \
                private_flat_hash_map_or_ins_res                               \
                    = CCC_private_flat_hash_map_data_at(                       \
                        private_flat_hash_map_or_ins_ent_pointer->private.map, \
                        private_flat_hash_map_or_ins_ent_pointer->private      \
                            .index);                                           \
                if (private_flat_hash_map_or_ins_ent_pointer->private.status   \
                    == CCC_ENTRY_VACANT)                                       \
                {                                                              \
                    *private_flat_hash_map_or_ins_res = type_compound_literal; \
                    CCC_private_flat_hash_map_set_insert(                      \
                        &private_flat_hash_map_or_ins_ent_pointer->private);   \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_flat_hash_map_or_ins_res;                                      \
    }))

/** @internal Insert entry also should not fail and therefore returns a
reference directly. This is similar to insert or assign where overwriting may
occur. */
#define CCC_private_flat_hash_map_insert_entry_with(                           \
    Flat_hash_map_entry_pointer, type_compound_literal...)                     \
    (__extension__({                                                           \
        __auto_type private_flat_hash_map_ins_ent_pointer                      \
            = (Flat_hash_map_entry_pointer);                                   \
        typeof(type_compound_literal) *private_flat_hash_map_ins_ent_res       \
            = NULL;                                                            \
        if (private_flat_hash_map_ins_ent_pointer)                             \
        {                                                                      \
            if (!(private_flat_hash_map_ins_ent_pointer->private.status        \
                  & CCC_ENTRY_INSERT_ERROR))                                   \
            {                                                                  \
                private_flat_hash_map_ins_ent_res                              \
                    = CCC_private_flat_hash_map_data_at(                       \
                        private_flat_hash_map_ins_ent_pointer->private.map,    \
                        private_flat_hash_map_ins_ent_pointer->private.index); \
                *private_flat_hash_map_ins_ent_res = type_compound_literal;    \
                if (private_flat_hash_map_ins_ent_pointer->private.status      \
                    == CCC_ENTRY_VACANT)                                       \
                {                                                              \
                    CCC_private_flat_hash_map_set_insert(                      \
                        &private_flat_hash_map_ins_ent_pointer->private);      \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_flat_hash_map_ins_ent_res;                                     \
    }))

/** @internal Because this function does not start with an entry it has the
option to give user more information and therefore returns an entry.
Importantly, this function makes sure the key is in sync with key in table. */
#define CCC_private_flat_hash_map_try_insert_with(flat_hash_map_pointer, key,  \
                                                  type_compound_literal...)    \
    (__extension__({                                                           \
        struct CCC_Flat_hash_map *private_flat_hash_map_pointer                \
            = (flat_hash_map_pointer);                                         \
        struct CCC_Entry private_flat_hash_map_try_insert_res                  \
            = {.status = CCC_ENTRY_ARGUMENT_ERROR};                            \
        if (private_flat_hash_map_pointer)                                     \
        {                                                                      \
            __auto_type private_flat_hash_map_key = key;                       \
            struct CCC_Flat_hash_map_entry private_flat_hash_map_try_ins_ent   \
                = CCC_private_flat_hash_map_entry(                             \
                    private_flat_hash_map_pointer,                             \
                    (void *)&private_flat_hash_map_key);                       \
            if ((private_flat_hash_map_try_ins_ent.status                      \
                 & CCC_ENTRY_OCCUPIED)                                         \
                || (private_flat_hash_map_try_ins_ent.status                   \
                    & CCC_ENTRY_INSERT_ERROR))                                 \
            {                                                                  \
                private_flat_hash_map_try_insert_res = (struct CCC_Entry){     \
                    .type = CCC_private_flat_hash_map_data_at(                 \
                        private_flat_hash_map_try_ins_ent.map,                 \
                        private_flat_hash_map_try_ins_ent.index),              \
                    .status = private_flat_hash_map_try_ins_ent.status,        \
                };                                                             \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_flat_hash_map_try_insert_res = (struct CCC_Entry){     \
                    .type = CCC_private_flat_hash_map_data_at(                 \
                        private_flat_hash_map_try_ins_ent.map,                 \
                        private_flat_hash_map_try_ins_ent.index),              \
                    .status = CCC_ENTRY_VACANT,                                \
                };                                                             \
                *((typeof(type_compound_literal) *)                            \
                      private_flat_hash_map_try_insert_res.type)               \
                    = type_compound_literal;                                   \
                *((typeof(private_flat_hash_map_key) *)                        \
                      CCC_private_flat_hash_map_key_at(                        \
                          private_flat_hash_map_try_ins_ent.map,               \
                          private_flat_hash_map_try_ins_ent.index))            \
                    = private_flat_hash_map_key;                               \
                CCC_private_flat_hash_map_set_insert(                          \
                    &private_flat_hash_map_try_ins_ent);                       \
            }                                                                  \
        }                                                                      \
        private_flat_hash_map_try_insert_res;                                  \
    }))

/** @internal Because this function does not start with an entry it has the
option to give user more information and therefore returns an entry.
Importantly, this function makes sure the key is in sync with key in table.
Similar to insert entry this will overwrite. */
#define CCC_private_flat_hash_map_insert_or_assign_with(                       \
    flat_hash_map_pointer, key, type_compound_literal...)                      \
    (__extension__({                                                           \
        struct CCC_Flat_hash_map *private_flat_hash_map_pointer                \
            = (flat_hash_map_pointer);                                         \
        struct CCC_Entry private_flat_hash_map_insert_or_assign_res            \
            = {.status = CCC_ENTRY_ARGUMENT_ERROR};                            \
        if (private_flat_hash_map_pointer)                                     \
        {                                                                      \
            private_flat_hash_map_insert_or_assign_res.status                  \
                = CCC_ENTRY_INSERT_ERROR;                                      \
            __auto_type private_flat_hash_map_key = key;                       \
            struct CCC_Flat_hash_map_entry                                     \
                private_flat_hash_map_ins_or_assign_ent                        \
                = CCC_private_flat_hash_map_entry(                             \
                    private_flat_hash_map_pointer,                             \
                    (void *)&private_flat_hash_map_key);                       \
            if (!(private_flat_hash_map_ins_or_assign_ent.status               \
                  & CCC_ENTRY_INSERT_ERROR))                                   \
            {                                                                  \
                private_flat_hash_map_insert_or_assign_res                     \
                    = (struct CCC_Entry){                                      \
                        .type = CCC_private_flat_hash_map_data_at(             \
                            private_flat_hash_map_ins_or_assign_ent.map,       \
                            private_flat_hash_map_ins_or_assign_ent.index),    \
                        .status                                                \
                        = private_flat_hash_map_ins_or_assign_ent.status,      \
                    };                                                         \
                *((typeof(type_compound_literal) *)                            \
                      private_flat_hash_map_insert_or_assign_res.type)         \
                    = type_compound_literal;                                   \
                *((typeof(private_flat_hash_map_key) *)                        \
                      CCC_private_flat_hash_map_key_at(                        \
                          private_flat_hash_map_ins_or_assign_ent.map,         \
                          private_flat_hash_map_ins_or_assign_ent.index))      \
                    = private_flat_hash_map_key;                               \
                if (private_flat_hash_map_ins_or_assign_ent.status             \
                    == CCC_ENTRY_VACANT)                                       \
                {                                                              \
                    CCC_private_flat_hash_map_set_insert(                      \
                        &private_flat_hash_map_ins_or_assign_ent);             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_flat_hash_map_insert_or_assign_res;                            \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_PRIVATE_FLAT_HASH_MAP_H */
