#ifndef CCC_IMPL_SIMD_HASH_MAP_H
#define CCC_IMPL_SIMD_HASH_MAP_H

/** @cond */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../types.h"
#include "impl_types.h"

/** @private An array of this enum will be in the metadata array. Same idea as
Rust's Hashbrown table. The only value not represented by these fields is the
following:

OCCUPIED: 0b0???????

In this case (?) represents any 7 bits kept from the upper 7 bits of the
original hash code to signify an occupied slot. We know this slot is taken
because the Most Significant Bit is zero, something that is not true of any
other state. Wrapped in a struct to avoid strict-aliasing exceptions that are
granted to uint8_t (usually unsigned char) and int8_t (usually char) when passed
to functions as pointers. Maybe nets performance gain but depends on
aggressiveness of compiler. */
typedef struct
{
    enum : uint8_t
    {
        CCC_SHM_DELETED = 0x80,
        CCC_SHM_EMPTY = 0xFF,
    } v;
} ccc_shm_meta;
static_assert(sizeof(ccc_shm_meta) == sizeof(uint8_t),
              "meta must wrap a byte in a struct without padding for better "
              "optimizations and no strict-aliasing exceptions.");
static_assert(
    (CCC_SHM_DELETED | CCC_SHM_EMPTY) == (uint8_t)~0,
    "all bits must be accounted for across deleted and empty status.");
static_assert(
    (CCC_SHM_DELETED ^ CCC_SHM_EMPTY) == 0x7F,
    "only empty should have lsb on and 7 bits are available for hash");

/** @private Vectorized group scanning allows more parallel scans but a
fallback of 8 is good for a portable implementation that will use the widest
word on a platform for group scanning. Right now, this lib targets 64-bit so
that means uint64_t is widest default integer widely supported. That width
is still valid on 32-bit but probably very slow due to emulation. */
enum
{
#if defined(__x86_64) && defined(__SSE2__)
    CCC_SHM_GROUP_SIZE = 16,
#else
    CCC_SHM_GROUP_SIZE = 8,
#endif /* defined(__x86_64) && defined(__SSE2__) */
};

/** @private The layout of the map uses only pointers to account for the
possibility of memory provided from the data segment, stack, or heap. When the
map is allowed to allocate it will take care of aligning pointers appropriately.
In the fixed size case we rely on the user defining a fixed size type. In either
case the arrays are in one contiguous allocation but split as follows:

(N == mask_ == capacity - 1) Where capacity is a required power of 2.
                        *
|Pad|D_N|...|D_2|D_1|D_0|M_0|M_1|M_2|...|M_N|R_0|...|R_N
                        ^                   ^
                        |                   |
                   Shared base      Start of Replica of first group to support
                   address of       a group load that starts at M_N as well as
                   Data and Meta.   erase and inserts. This means R_N is never
                   arrays.          needed but duplicated for branchless ops.
                   arrays.

The Data array has a reverse layout so that indices will be found as a
subtracted address offset from the shared base location. Individual elements are
still written into the slots in a normal fashion--such that functions like
memcpy work normally--we simply count backwards from the shared base.The
metadata array ascends normally. The metadata is located on the next byte
address from the 0th data element because it's size is only a byte meaning a
shared base address is possible with no alignment issues. */
struct ccc_shmap_
{
    void *data_;             /** Reversed user type data array. */
    ccc_shm_meta *meta_;     /** Metadata array on byte following data_[0]. */
    size_t sz_;              /** The number of user active slots. */
    size_t avail_;           /** Track to know when rehashing is needed. */
    size_t mask_;            /** The mask for power of two table sizing. */
    ccc_tribool init_;       /** One-time flag to lazily initialize table. */
    size_t elem_sz_;         /** Size of each user data element being stored. */
    size_t key_offset_;      /** The location of the key field in user type. */
    ccc_key_eq_fn *eq_fn_;   /** The user callback for equality comparison. */
    ccc_hash_fn *hash_fn_;   /** The hash function provided by user. */
    ccc_alloc_fn *alloc_fn_; /** The allocation function, if any. */
    void *aux_;              /** Auxiliary data, if any. */
};

/** @private */
struct ccc_shash_entry_
{
    struct ccc_shmap_ *h_;
    ccc_shm_meta meta_;
    struct ccc_handl_ handle_;
};

/** @private */
union ccc_shmap_entry_
{
    struct ccc_shash_entry_ impl_;
};

/** @private Helps the user declare a type for a fixed size map. They can then
use this type when they want a hash map as global, static global, or stack
local. They would need to define their fixed size type every time but that
should be fine as they are likely to only declare one or two. The metadata
array must be rounded up */
#define ccc_impl_shm_declare_fixed_map(fixed_map_type_name, key_val_type_name, \
                                       capacity)                               \
    typedef struct                                                             \
    {                                                                          \
        key_val_type_name data[capacity];                                      \
        ccc_shm_meta                                                           \
            meta[(((capacity) + ((capacity) - 1)) / CCC_SHM_GROUP_SIZE)        \
                 + CCC_SHM_GROUP_SIZE];                                        \
    }(fixed_map_type_name);                                                    \
    static_assert(sizeof((fixed_map_type_name){}.data) != 0,                   \
                  "fixed size map must have capacity greater than "            \
                  "0.");                                                       \
    static_assert((sizeof((fixed_map_type_name){}.data)                        \
                   / sizeof((fixed_map_type_name){}.data[0]))                  \
                      >= CCC_SHM_GROUP_SIZE,                                   \
                  "fixed size map must have capacity >= "                      \
                  "CCC_SHM_GROUP_SIZE (8 or 16 depending on platform).");      \
    static_assert(((sizeof((fixed_map_type_name){}.data)                       \
                    / sizeof((fixed_map_type_name){}.data[0]))                 \
                   & ((sizeof((fixed_map_type_name){}.data)                    \
                       / sizeof((fixed_map_type_name){}.data[0]))              \
                      - 1))                                                    \
                      == 0,                                                    \
                  "fixed size map must be a power of 2 capacity (32, 64, "     \
                  "128, 256, etc.).");

#define ccc_impl_shm_init(data_ptr, meta_ptr, key_field, hash_fn, key_eq_fn,   \
                          alloc_fn, aux_data, capacity)                        \
    {                                                                          \
        .data_ = (data_ptr),                                                   \
        .meta_ = (meta_ptr),                                                   \
        .sz_ = 0,                                                              \
        .avail_ = (((capacity) / 8) * 7),                                      \
        .mask_ = (((capacity) > 0) ? ((capacity) - 1) : 0),                    \
        .init_ = CCC_FALSE,                                                    \
        .elem_sz_ = sizeof(*(data_ptr)),                                       \
        .key_offset_ = offsetof(*(data_ptr), key_field),                       \
        .hash_fn_ = (hash_fn),                                                 \
        .alloc_fn_ = (alloc_fn),                                               \
        .aux_ = (aux_data),                                                    \
    }

#endif /* CCC_IMPL_SIMD_HASH_MAP_H */
