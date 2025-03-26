#ifndef CCC_IMPL_SIMD_HASH_MAP_H
#define CCC_IMPL_SIMD_HASH_MAP_H

#include <stddef.h>
#include <stdint.h>

/** @private An array of this enum will be at the start of the contiguous
block of memory. Same idea as Rust's Hashbrown table. The only value not
represented by these fields is the following:

OCCUPIED: 0b0???????

In this case (?) represents any 7 bits kept from the upper 7 bits of the
original hash code to signify an occupied slot. We know this slot is taken
because the Most Significant Bit is zero, something that is not true of any
other state. Wrapped in a struct to avoid strict-aliasing exceptions that are
granted to uint8_t (usually unsigned char) and int8_t (usually char). Maybe
nets performance gain but depends on aggressiveness of compiler. */
typedef struct
{
    enum : uint8_t
    {
        CCC_SHM_DELETED = 0x80,
        CCC_SHM_EMPTY = 0xFF,
    } v;
} ccc_shm_meta;

/** @private Vectorized group scanning allows more parallel scans but a
fallback of 8 is good for a portable implementation that will use the widest
word on a platform for group scanning. Right now, this lib targets x86 so
that means uint64_t is widest default integer widely supported. That width is
still valid on 32-bit but probably very slow due to emulation. */
enum
{
#if defined(__x86_64) && defined(__SSE2__)
    CCC_SHM_GROUP_SIZE = 16,
#else
    CCC_SHM_GROUP_SIZE = 8,
#endif /* defined(__x86_64) && defined(__SSE2__) */
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
    }(fixed_map_type_name);

struct simd_hash_map_
{
    ccc_shm_meta *meta_;
    void *data_;
    size_t sz_;
    size_t avail_;
    size_t cap_;
    size_t elem_sz_;
    size_t key_offset_;
};

#endif /* CCC_IMPL_SIMD_HASH_MAP_H */
