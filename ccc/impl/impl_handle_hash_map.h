#ifndef CCC_IMPL_HANDLE_HASH_MAP_H
#define CCC_IMPL_HANDLE_HASH_MAP_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

#include "../buffer.h"
#include "../types.h"
#include "impl_types.h"

/** @private */
enum : uint64_t
{
    CCC_HHM_EMPTY = 0,
};

/** @private To offer handle "stability," similar to pointer stability except
with indices rather than pointers, we will run the robin hood hash table
algorithm with backshift deletions on only the metadata field of the intrusive
element. The metadata will be swapped across user entries in the table while
the user meta index tracking stays in the same slot with arbitrary sized user
data. Both the meta and user fields will need to point to each other so that
each can be updated on swaps, back shifts, deletions, and insertions. The home
slot in the table never changes for a metadata entry. However, the metadata
tracking must be updated every time a back shift or swap occurs. */
struct ccc_hhmap_elem_
{
    /* The struct that swaps during the robin hood algo. Caching the full hash
       here to avoid using pointer to user data for full comparison callback. */
    struct
    {
        /* The full hash of the user data. Reduces callbacks and rehashing. */
        uint64_t hash_;
        /* Index of the permanent home of the data associated with this hash.
           Does not change once initialized even when an element is removed. */
        size_t slot_i_;
    } meta_;
    /* This field stays at original slot. Updated whenever meta moves. */
    size_t meta_i_;
};

/** @private */
struct ccc_hhmap_
{
    ccc_buffer buf_;          /* Buffer of types with size, cap, and aux. */
    ccc_hash_fn *hash_fn_;    /* Hashing callback. */
    ccc_key_eq_fn *eq_fn_;    /* Equality callback. */
    size_t key_offset_;       /* Key in user defined type offset. */
    size_t hash_elem_offset_; /* Intrusive element offset. */
};

/** @private */
struct ccc_handle_
{
    /* The types.h entry is not quite suitable for this container so change. */
    size_t i_;
    /* Keep these types in sync in cases status reporting changes. */
    typeof((ccc_entry){}.impl_.stats_) stats_;
};

/** @private */
struct ccc_hhash_entry_
{
    struct ccc_hhmap_ *h_;
    uint64_t hash_;
    struct ccc_handle_ entry_;
};

/** @private */
union ccc_hhmap_entry_
{
    struct ccc_hhash_entry_ impl_;
};

#define ccc_impl_hhm_init(memory_ptr, capacity, hhash_elem_field, key_field,   \
                          alloc_fn, hash_fn, key_eq_fn, aux)                   \
    {                                                                          \
        .buf_                                                                  \
        = (ccc_buffer)ccc_buf_init((memory_ptr), (alloc_fn), (aux), capacity), \
        .hash_fn_ = (hash_fn),                                                 \
        .eq_fn_ = (key_eq_fn),                                                 \
        .key_offset_ = offsetof(typeof(*(memory_ptr)), key_field),             \
        .hash_elem_offset_                                                     \
        = offsetof(typeof(*(memory_ptr)), hhash_elem_field),                   \
    }

#endif /* CCC_IMPL_HANDLE_HASH_MAP_H */
