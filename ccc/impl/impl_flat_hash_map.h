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
#ifndef CCC_IMPL_FLAT_HASH_MAP_H
#define CCC_IMPL_FLAT_HASH_MAP_H

/** @cond */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../types.h"
#include "impl_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/* TODO: Come up with better system. For now, uncomment to define a preprocessor
directive to force generics. This has been used for testing for now. */
// #define CCC_FHM_PORTABLE

/** @private An array of this byte will be in the tag array. Same idea as
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
typedef struct
{
    /** Can be set to DELETED or EMPTY or an arbitrary hash 0b0???????. */
    uint8_t v;
} ccc_fhm_tag;

#if defined(__x86_64) && defined(__SSE2__) && !defined(CCC_FHM_PORTABLE)
/** @private Internal container collection detection for SIMD instructions on
the x86 architectures. This will be the most efficient version possible
offering the widest group matching. */
#    define CCC_HAS_X86_SIMD
#elif defined(__ARM_NEON__) && !defined(CCC_FHM_PORTABLE)
/** @private Internal container collection detection for SIMD instructions on
the NEON architecture. This implementation currently lacks some of the features
of the x86 SIMD version but should still be fast. */
#    define CCC_HAS_ARM_SIMD
#endif
/** Else we define nothing and the portable fallback will take effect. */

/** @private Vectorized group scanning allows more parallel scans but a
fallback of 8 is good for a portable implementation that will use the widest
word on a platform for group scanning. Right now, this lib targets 64-bit so
that means uint64_t is widest default integer widely supported. That width
is still valid on 32-bit but probably very slow due to emulation. */
enum : uint8_t
{
#if defined(CCC_HAS_X86_SIMD)
    /** A group of tags that can be loaded into a 128 bit vector. */
    CCC_FHM_GROUP_SIZE = 16,
#elif defined(CCC_HAS_ARM_SIMD)
    /** A group of tags that can be loded into a 64 bit integer. */
    CCC_FHM_GROUP_SIZE = 8,
#else  /* PORTABLE FALLBACK */
    /** A group of tags that can be loded into a 64 bit integer. */
    CCC_FHM_GROUP_SIZE = 8,
#endif /* defined(CCC_HAS_X86_SIMD) */
};

/** @private The layout of the map uses only pointers to account for the
possibility of memory provided from the data segment, stack, or heap. When the
map is allowed to allocate it will take care of aligning pointers appropriately.
In the fixed size case we rely on the user defining a fixed size type. In either
case the arrays are in one contiguous allocation but split as follows:

(N == capacity - 1) Where capacity is a power of 2. (G == group size - 1).

┌────┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
│Swap│D_N│...│D_1│D_0│T_0│T_1│...│T_N│R_0│R_1│...│R_G│
└─┬──┴───┴───┴───┴───┼───┴───┴───┴───┼───┴───┴───┴───┘
┌─┴───────────┐ ┌────┴────────┐ ┌────┴─────────────────────────────────────┐
│Swap slot for│ │Shared base  │ │Start of replica of first group to support│
│in place     │ │address of   │ │a group load that starts at T_N as well as│
│rehashing.   │ │Data and Tag.│ │erase and inserts. This means R_G is never│
│Size = 1 data│ │arrays.      │ │needed but duplicated for branchless ops. │
└─────────────┘ └─────────────┘ └──────────────────────────────────────────┘

The Data array has a reverse layout so that indices will be found as a
subtracted address offset from the shared base location. Individual elements are
still written into the slots in a normal fashion--such that functions like
memcpy work normally--we simply count backwards from the shared base.The
tag array ascends normally. The tag array is located on the next byte address
from the 0th data element because it's size is only a byte meaning a shared base
address is possible with no alignment issues. */
struct ccc_fhmap
{
    /** Reversed user type data array. */
    void *data;
    /** Tag array on byte following data(0). */
    ccc_fhm_tag *tag;
    /** The number of user active slots. */
    size_t count;
    /** Track available slots given load factor constrains. When 0, rehash. */
    size_t remain;
    /** The mask for power of two table sizing. */
    size_t mask;
    /** One-time flag to lazily initialize table. */
    ccc_tribool init;
    /** Size of each user data element being stored. */
    size_t sizeof_type;
    /** The location of the key field in user type. */
    size_t key_offset;
    /** The user callback for equality comparison. */
    ccc_any_key_eq_fn *eq_fn;
    /** The hash function provided by user. */
    ccc_any_key_hash_fn *hash_fn;
    /** The allocation function, if any. */
    ccc_any_alloc_fn *alloc_fn;
    /** Auxiliary data, if any. */
    void *aux;
};

/** @private A struct for containing all relevant information for a query
into one object so that passing to future functions is cleaner. */
struct ccc_fhash_entry
{
    /** The map associated with this entry. */
    struct ccc_fhmap *h;
    /** The saved tag from the current query hash value. */
    ccc_fhm_tag tag;
    /** The location and status of the query index, occupied, vacant, etc. */
    struct ccc_handl handle;
};

/** @private A simple wrapper for an entry that allows us to return a compound
literal reference. All interface functions accept pointers to entries and
a functional chain of calls is not possible with return by value. The interface
can then return `&(union ccc_fhmap_entry){function_call(...).impl}` which is
a compound literal reference in C23. */
union ccc_fhmap_entry
{
    struct ccc_fhash_entry impl;
};

/** While the private interface functions are not strictly necessary containing
the logic of interacting with the map to the src implementation makes reasoning
and debugging the macros easier. It also cuts down on repeated logic. */
/*======================     Private Interface      =========================*/

struct ccc_fhash_entry ccc_impl_fhm_entry(struct ccc_fhmap *h, void const *key);
void ccc_impl_fhm_insert(struct ccc_fhmap *h, void const *key_val_type,
                         ccc_fhm_tag m, size_t i);
void ccc_impl_fhm_erase(struct ccc_fhmap *h, size_t i);
void *ccc_impl_fhm_data_at(struct ccc_fhmap const *h, size_t i);
void *ccc_impl_fhm_key_at(struct ccc_fhmap const *h, size_t i);
void ccc_impl_fhm_set_insert(struct ccc_fhash_entry const *e);

/*======================    Macro Implementations   =========================*/

/** @private Helps the user declare a type for a fixed size map. They can then
use this type when they want a hash map as global, static global, or stack
local. They would need to define their fixed size type every time but that
should be fine as they are likely to only declare one or two. They would likely
only have a one fixed size map per translation unit if they are using these
capabilities. They control the name of the type so they can organize types as
they wish. */
#define ccc_impl_fhm_declare_fixed_map(fixed_map_type_name, key_val_type_name, \
                                       capacity)                               \
    static_assert((capacity) != 0,                                             \
                  "fixed size map must have capacity greater than 0.");        \
    static_assert((capacity) >= CCC_FHM_GROUP_SIZE,                            \
                  "fixed size map must have capacity >= CCC_FHM_GROUP_SIZE "   \
                  "(8 or 16 depending on platform).");                         \
    static_assert(((capacity) & ((capacity) - 1)) == 0,                        \
                  "fixed size map must be a power of 2 capacity (32, 64, "     \
                  "128, 256, etc.).");                                         \
    typedef struct                                                             \
    {                                                                          \
        key_val_type_name data[(capacity) + 1];                                \
        ccc_fhm_tag tag[(capacity) + CCC_FHM_GROUP_SIZE];                      \
    }(fixed_map_type_name);

/** @private If the user does not want to remember the capacity they chose
for their type or make mistakes this macro offers consistent calculation of
total capacity (aka buckets) of the map. This is not the capacity that is
limited by load factor. */
#define ccc_impl_fhm_fixed_capacity(fixed_map_type_name)                       \
    (sizeof((fixed_map_type_name){}.tag) - CCC_FHM_GROUP_SIZE)

/** @private Initializing is tricky due to variety of sources of memory we must
support. To make it easier we allow the user to pass data and tag arrays as
two separate pointers even though they are in the same contiguous allocation.
This could lead to some errors but we will have to make the user facing header
documentation abundantly clear about what we expect and why.

We will not support being passed a dynamically allocated array at runtime.
Instead we will expose a reserve() interface to allow the user to specify a
fixed size map when they don't know exactly the size needed until runtime. */
#define ccc_impl_fhm_init(impl_data_ptr, impl_tag_ptr, impl_key_field,         \
                          impl_hash_fn, impl_key_eq_fn, impl_alloc_fn,         \
                          impl_aux_data, impl_capacity)                        \
    {                                                                          \
        .data = (impl_data_ptr),                                               \
        .tag = (impl_tag_ptr),                                                 \
        .count = 0,                                                            \
        .remain = (((impl_capacity) / (size_t)8) * (size_t)7),                 \
        .mask = (((impl_capacity) > (size_t)0) ? ((impl_capacity) - (size_t)1) \
                                               : (size_t)0),                   \
        .init = CCC_FALSE,                                                     \
        .sizeof_type = sizeof(*(impl_data_ptr)),                               \
        .key_offset = offsetof(typeof(*(impl_data_ptr)), impl_key_field),      \
        .eq_fn = (impl_key_eq_fn),                                             \
        .hash_fn = (impl_hash_fn),                                             \
        .alloc_fn = (impl_alloc_fn),                                           \
        .aux = (impl_aux_data),                                                \
    }

/*========================    Construct In Place    =========================*/

/** @private A fairly good approximation of closures given C23 capabilities.
The user facing docs clarify that T is a correctly typed reference to the
desired data if occupied. */
#define ccc_impl_fhm_and_modify_w(flat_hash_map_entry_ptr, type_name,          \
                                  closure_over_T...)                           \
    (__extension__({                                                           \
        __auto_type impl_fhm_mod_ent_ptr = (flat_hash_map_entry_ptr);          \
        struct ccc_fhash_entry impl_fhm_mod_with_ent                           \
            = {.handle = {.stats = CCC_ENTRY_ARG_ERROR}};                      \
        if (impl_fhm_mod_ent_ptr)                                              \
        {                                                                      \
            impl_fhm_mod_with_ent = impl_fhm_mod_ent_ptr->impl;                \
            if (impl_fhm_mod_with_ent.handle.stats & CCC_ENTRY_OCCUPIED)       \
            {                                                                  \
                type_name *const T = ccc_impl_fhm_data_at(                     \
                    impl_fhm_mod_with_ent.h, impl_fhm_mod_with_ent.handle.i);  \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_fhm_mod_with_ent;                                                 \
    }))

/** @private The or insert method is unique in that it directly returns a
reference to the inserted data rather than a entry with a status. This is
because it should not fail. If NULL is returned the user knows there is a
problem. */
#define ccc_impl_fhm_or_insert_w(flat_hash_map_entry_ptr, lazy_key_value...)   \
    (__extension__({                                                           \
        __auto_type impl_fhm_or_ins_ent_ptr = (flat_hash_map_entry_ptr);       \
        typeof(lazy_key_value) *impl_fhm_or_ins_res = NULL;                    \
        if (impl_fhm_or_ins_ent_ptr)                                           \
        {                                                                      \
            struct ccc_fhash_entry *impl_fhm_or_ins_entry                      \
                = &impl_fhm_or_ins_ent_ptr->impl;                              \
            if (!(impl_fhm_or_ins_entry->handle.stats                          \
                  & CCC_ENTRY_INSERT_ERROR))                                   \
            {                                                                  \
                impl_fhm_or_ins_res                                            \
                    = ccc_impl_fhm_data_at(impl_fhm_or_ins_entry->h,           \
                                           impl_fhm_or_ins_entry->handle.i);   \
                if (impl_fhm_or_ins_entry->handle.stats == CCC_ENTRY_VACANT)   \
                {                                                              \
                    *impl_fhm_or_ins_res = lazy_key_value;                     \
                    ccc_impl_fhm_set_insert(impl_fhm_or_ins_entry);            \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_fhm_or_ins_res;                                                   \
    }))

/** @private Insert entry also should not fail and therefore returns a reference
directly. This is similar to insert or assign where overwriting may occur. */
#define ccc_impl_fhm_insert_entry_w(flat_hash_map_entry_ptr,                   \
                                    lazy_key_value...)                         \
    (__extension__({                                                           \
        __auto_type impl_fhm_ins_ent_ptr = (flat_hash_map_entry_ptr);          \
        typeof(lazy_key_value) *impl_fhm_ins_ent_res = NULL;                   \
        if (impl_fhm_ins_ent_ptr)                                              \
        {                                                                      \
            struct ccc_fhash_entry *impl_fhm_ins_entry                         \
                = &impl_fhm_ins_ent_ptr->impl;                                 \
            if (!(impl_fhm_ins_entry->handle.stats & CCC_ENTRY_INSERT_ERROR))  \
            {                                                                  \
                impl_fhm_ins_ent_res = ccc_impl_fhm_data_at(                   \
                    impl_fhm_ins_entry->h, impl_fhm_ins_entry->handle.i);      \
                *impl_fhm_ins_ent_res = lazy_key_value;                        \
                if (impl_fhm_ins_entry->handle.stats == CCC_ENTRY_VACANT)      \
                {                                                              \
                    ccc_impl_fhm_set_insert(impl_fhm_ins_entry);               \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_fhm_ins_ent_res;                                                  \
    }))

/** @private Because this function does not start with an entry it has the
option to give user more information and therefore returns an entry.
Importantly, this function makes sure the key is in sync with key in table. */
#define ccc_impl_fhm_try_insert_w(flat_hash_map_ptr, key, lazy_value...)       \
    (__extension__({                                                           \
        struct ccc_fhmap *impl_flat_hash_map_ptr = (flat_hash_map_ptr);        \
        struct ccc_ent impl_fhm_try_insert_res                                 \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_flat_hash_map_ptr)                                            \
        {                                                                      \
            __auto_type impl_fhm_key = key;                                    \
            struct ccc_fhash_entry impl_fhm_try_ins_ent = ccc_impl_fhm_entry(  \
                impl_flat_hash_map_ptr, (void *)&impl_fhm_key);                \
            if ((impl_fhm_try_ins_ent.handle.stats & CCC_ENTRY_OCCUPIED)       \
                || (impl_fhm_try_ins_ent.handle.stats                          \
                    & CCC_ENTRY_INSERT_ERROR))                                 \
            {                                                                  \
                impl_fhm_try_insert_res = (struct ccc_ent){                    \
                    .e = ccc_impl_fhm_data_at(impl_fhm_try_ins_ent.h,          \
                                              impl_fhm_try_ins_ent.handle.i),  \
                    .stats = impl_fhm_try_ins_ent.handle.stats,                \
                };                                                             \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_fhm_try_insert_res = (struct ccc_ent){                    \
                    .e = ccc_impl_fhm_data_at(impl_fhm_try_ins_ent.h,          \
                                              impl_fhm_try_ins_ent.handle.i),  \
                    .stats = CCC_ENTRY_VACANT,                                 \
                };                                                             \
                *((typeof(lazy_value) *)impl_fhm_try_insert_res.e)             \
                    = lazy_value;                                              \
                *((typeof(impl_fhm_key) *)ccc_impl_fhm_key_at(                 \
                    impl_fhm_try_ins_ent.h, impl_fhm_try_ins_ent.handle.i))    \
                    = impl_fhm_key;                                            \
                ccc_impl_fhm_set_insert(&impl_fhm_try_ins_ent);                \
            }                                                                  \
        }                                                                      \
        impl_fhm_try_insert_res;                                               \
    }))

/** @private Because this function does not start with an entry it has the
option to give user more information and therefore returns an entry.
Importantly, this function makes sure the key is in sync with key in table.
Similar to insert entry this will overwrite. */
#define ccc_impl_fhm_insert_or_assign_w(flat_hash_map_ptr, key, lazy_value...) \
    (__extension__({                                                           \
        struct ccc_fhmap *impl_flat_hash_map_ptr = (flat_hash_map_ptr);        \
        struct ccc_ent impl_fhm_insert_or_assign_res                           \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_flat_hash_map_ptr)                                            \
        {                                                                      \
            impl_fhm_insert_or_assign_res.stats = CCC_ENTRY_INSERT_ERROR;      \
            __auto_type impl_fhm_key = key;                                    \
            struct ccc_fhash_entry impl_fhm_ins_or_assign_ent                  \
                = ccc_impl_fhm_entry(impl_flat_hash_map_ptr,                   \
                                     (void *)&impl_fhm_key);                   \
            if (!(impl_fhm_ins_or_assign_ent.handle.stats                      \
                  & CCC_ENTRY_INSERT_ERROR))                                   \
            {                                                                  \
                impl_fhm_insert_or_assign_res = (struct ccc_ent){              \
                    .e = ccc_impl_fhm_data_at(                                 \
                        impl_fhm_ins_or_assign_ent.h,                          \
                        impl_fhm_ins_or_assign_ent.handle.i),                  \
                    .stats = impl_fhm_ins_or_assign_ent.handle.stats,          \
                };                                                             \
                *((typeof(lazy_value) *)impl_fhm_insert_or_assign_res.e)       \
                    = lazy_value;                                              \
                *((typeof(impl_fhm_key) *)ccc_impl_fhm_key_at(                 \
                    impl_fhm_ins_or_assign_ent.h,                              \
                    impl_fhm_ins_or_assign_ent.handle.i))                      \
                    = impl_fhm_key;                                            \
                if (impl_fhm_ins_or_assign_ent.handle.stats                    \
                    == CCC_ENTRY_VACANT)                                       \
                {                                                              \
                    ccc_impl_fhm_set_insert(&impl_fhm_ins_or_assign_ent);      \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_fhm_insert_or_assign_res;                                         \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_HASH_MAP_H */
