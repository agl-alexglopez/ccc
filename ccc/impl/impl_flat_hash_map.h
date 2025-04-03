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

/** @private An array of this enum will be in the tagdata array. Same idea as
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
        CCC_FHM_DELETED = 0x80,
        CCC_FHM_EMPTY = 0xFF,
    } v;
} ccc_fhm_tag;
static_assert(sizeof(ccc_fhm_tag) == sizeof(uint8_t),
              "tag must wrap a byte in a struct without padding for better "
              "optimizations and no strict-aliasing exceptions.");
static_assert(
    (CCC_FHM_DELETED | CCC_FHM_EMPTY) == (uint8_t)~0,
    "all bits must be accounted for across deleted and empty status.");
static_assert(
    (CCC_FHM_DELETED ^ CCC_FHM_EMPTY) == 0x7F,
    "only empty should have lsb on and 7 bits are available for hash");

/* TODO: Come up with better system. For now, uncomment to define a preprocessor
directive to force generics. */
// #define CCC_FHM_PORTABLE

/** @private Vectorized group scanning allows more parallel scans but a
fallback of 8 is good for a portable implementation that will use the widest
word on a platform for group scanning. Right now, this lib targets 64-bit so
that means uint64_t is widest default integer widely supported. That width
is still valid on 32-bit but probably very slow due to emulation. */
enum
{
#if defined(__x86_64) && defined(__SSE2__) && !defined(CCC_FHM_PORTABLE)
    CCC_FHM_GROUP_SIZE = 16,
#else
    CCC_FHM_GROUP_SIZE = 8,
#endif /* defined(__x86_64) && defined(__SSE2__) */
};

/** @private The layout of the map uses only pointers to account for the
possibility of memory provided from the data segment, stack, or heap. When the
map is allowed to allocate it will take care of aligning pointers appropriately.
In the fixed size case we rely on the user defining a fixed size type. In either
case the arrays are in one contiguous allocation but split as follows:

(N == mask_ == capacity - 1) Where capacity is a required power of 2.
                         *
|Swap|D_N|...|D_2|D_1|D_0|T_0|T_1|T_2|...|T_N|R_0|...|R_N
 ^                       ^                   ^
 |                       |                   |
Swap slot for      Shared base      Start of Replica of first group to support
in place           address of       a group load that starts at T_N as well as
rehashing          Data and Tag.    erase and inserts. This means R_N is never
                   arrays.          needed but duplicated for branchless ops.
                   arrays.

The Data array has a reverse layout so that indices will be found as a
subtracted address offset from the shared base location. Individual elements are
still written into the slots in a normal fashion--such that functions like
memcpy work normally--we simply count backwards from the shared base.The
tagdata array ascends normally. The tagdata is located on the next byte
address from the 0th data element because it's size is only a byte meaning a
shared base address is possible with no alignment issues. */
struct ccc_fhmap_
{
    /** Reversed user type data array. */
    void *data_;
    /** Tagdata array on byte following data(0). */
    ccc_fhm_tag *tag_;
    /** The number of user active slots. */
    size_t sz_;
    /** Track to know when rehashing is needed. */
    size_t avail_;
    /** The mask for power of two table sizing. */
    size_t mask_;
    /** One-time flag to lazily initialize table. */
    ccc_tribool init_;
    /** Size of each user data element being stored. */
    size_t elem_sz_;
    /** The location of the key field in user type. */
    size_t key_offset_;
    /** The user callback for equality comparison. */
    ccc_key_eq_fn *eq_fn_;
    /** The hash function provided by user. */
    ccc_hash_fn *hash_fn_;
    /** The allocation function, if any. */
    ccc_alloc_fn *alloc_fn_;
    /** Auxiliary data, if any. */
    void *aux_;
};

/** @private */
struct ccc_fhash_entry_
{
    struct ccc_fhmap_ *h_;
    ccc_fhm_tag tag_;
    struct ccc_handl_ handle_;
};

/** @private */
union ccc_fhmap_entry_
{
    struct ccc_fhash_entry_ impl_;
};

/*======================     Private Interface      =========================*/

struct ccc_fhash_entry_ ccc_impl_fhm_entry(struct ccc_fhmap_ *h,
                                           void const *key);
void ccc_impl_fhm_insert(struct ccc_fhmap_ *h, void const *key_val_type,
                         ccc_fhm_tag m, size_t i);
void ccc_impl_fhm_erase(struct ccc_fhmap_ *h, size_t i);
void *ccc_impl_fhm_data_at(struct ccc_fhmap_ const *h, size_t i);
void ccc_impl_fhm_set_insert(struct ccc_fhash_entry_ const *e);

/*======================    Macro Implementations   =========================*/

/** @private Helps the user declare a type for a fixed size map. They can then
use this type when they want a hash map as global, static global, or stack
local. They would need to define their fixed size type every time but that
should be fine as they are likely to only declare one or two. The tagdata
array must be rounded up */
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

#define ccc_impl_fhm_fixed_capacity(fixed_map_type_name)                       \
    (sizeof((fixed_map_type_name){}.tag) - CCC_FHM_GROUP_SIZE)

#define ccc_impl_fhm_init(data_ptr, tag_ptr, key_field, hash_fn, key_eq_fn,    \
                          alloc_fn, aux_data, capacity)                        \
    {                                                                          \
        .data_ = (data_ptr),                                                   \
        .tag_ = (tag_ptr),                                                     \
        .sz_ = 0,                                                              \
        .avail_ = (((capacity) / (size_t)8) * (size_t)7),                      \
        .mask_                                                                 \
        = (((capacity) > (size_t)0) ? ((capacity) - (size_t)1) : (size_t)0),   \
        .init_ = CCC_FALSE,                                                    \
        .elem_sz_ = sizeof(*(data_ptr)),                                       \
        .key_offset_ = offsetof(typeof(*(data_ptr)), key_field),               \
        .eq_fn_ = (key_eq_fn),                                                 \
        .hash_fn_ = (hash_fn),                                                 \
        .alloc_fn_ = (alloc_fn),                                               \
        .aux_ = (aux_data),                                                    \
    }

/*========================    Construct In Place    =========================*/

#define ccc_impl_fhm_and_modify_w(flat_hash_map_entry_ptr, type_name,          \
                                  closure_over_T...)                           \
    (__extension__({                                                           \
        __auto_type fhm_mod_ent_ptr_ = (flat_hash_map_entry_ptr);              \
        struct ccc_fhash_entry_ fhm_mod_with_ent_                              \
            = {.handle_ = {.stats_ = CCC_ENTRY_ARG_ERROR}};                    \
        if (fhm_mod_ent_ptr_)                                                  \
        {                                                                      \
            fhm_mod_with_ent_ = fhm_mod_ent_ptr_->impl_;                       \
            if (fhm_mod_with_ent_.handle_.stats_ & CCC_ENTRY_OCCUPIED)         \
            {                                                                  \
                type_name *const T = ccc_impl_fhm_data_at(                     \
                    fhm_mod_with_ent_.h_, fhm_mod_with_ent_.handle_.i_);       \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        fhm_mod_with_ent_;                                                     \
    }))

/** @private */
#define ccc_impl_fhm_or_insert_w(flat_hash_map_entry_ptr, lazy_key_value...)   \
    (__extension__({                                                           \
        __auto_type fhm_or_ins_ent_ptr_ = (flat_hash_map_entry_ptr);           \
        typeof(lazy_key_value) *fhm_or_ins_res_ = NULL;                        \
        if (fhm_or_ins_ent_ptr_)                                               \
        {                                                                      \
            struct ccc_fhash_entry_ *fhm_or_ins_entry_                         \
                = &fhm_or_ins_ent_ptr_->impl_;                                 \
            if (!(fhm_or_ins_entry_->handle_.stats_ & CCC_ENTRY_INSERT_ERROR)) \
            {                                                                  \
                fhm_or_ins_res_ = ccc_impl_fhm_data_at(                        \
                    fhm_or_ins_entry_->h_, fhm_or_ins_entry_->handle_.i_);     \
                if (fhm_or_ins_entry_->handle_.stats_ == CCC_ENTRY_VACANT)     \
                {                                                              \
                    *fhm_or_ins_res_ = lazy_key_value;                         \
                    ccc_impl_fhm_set_insert(fhm_or_ins_entry_);                \
                }                                                              \
            }                                                                  \
        }                                                                      \
        fhm_or_ins_res_;                                                       \
    }))

#define ccc_impl_fhm_insert_entry_w(flat_hash_map_entry_ptr,                   \
                                    lazy_key_value...)                         \
    (__extension__({                                                           \
        __auto_type fhm_ins_ent_ptr_ = (flat_hash_map_entry_ptr);              \
        typeof(lazy_key_value) *fhm_ins_ent_res_ = NULL;                       \
        if (fhm_ins_ent_ptr_)                                                  \
        {                                                                      \
            struct ccc_fhash_entry_ *fhm_ins_entry_                            \
                = &fhm_ins_ent_ptr_->impl_;                                    \
            if (!(fhm_ins_entry_->handle_.stats_ & CCC_ENTRY_INSERT_ERROR))    \
            {                                                                  \
                fhm_ins_ent_res_ = ccc_impl_fhm_data_at(                       \
                    fhm_ins_entry_->h_, fhm_ins_entry_->handle_.i_);           \
                *fhm_ins_ent_res_ = lazy_key_value;                            \
                ccc_impl_fhm_set_insert(fhm_ins_entry_);                       \
            }                                                                  \
        }                                                                      \
        fhm_ins_ent_res_;                                                      \
    }))

/** @private */
#define ccc_impl_fhm_try_insert_w(flat_hash_map_ptr, key, lazy_value...)       \
    (__extension__({                                                           \
        struct ccc_fhmap_ *flat_hash_map_ptr_ = (flat_hash_map_ptr);           \
        struct ccc_ent_ fhm_try_insert_res_ = {.stats_ = CCC_ENTRY_ARG_ERROR}; \
        if (flat_hash_map_ptr_)                                                \
        {                                                                      \
            __auto_type fhm_key_ = key;                                        \
            struct ccc_fhash_entry_ fhm_try_ins_ent_                           \
                = ccc_impl_fhm_entry(flat_hash_map_ptr_, (void *)&fhm_key_);   \
            if ((fhm_try_ins_ent_.handle_.stats_ & CCC_ENTRY_OCCUPIED)         \
                || (fhm_try_ins_ent_.handle_.stats_ & CCC_ENTRY_INSERT_ERROR)) \
            {                                                                  \
                fhm_try_insert_res_ = (struct ccc_ent_){                       \
                    .e_ = ccc_impl_fhm_data_at(fhm_try_ins_ent_.h_,            \
                                               fhm_try_ins_ent_.handle_.i_),   \
                    .stats_ = fhm_try_ins_ent_.handle_.stats_,                 \
                };                                                             \
            }                                                                  \
            else                                                               \
            {                                                                  \
                fhm_try_insert_res_ = (struct ccc_ent_){                       \
                    .e_ = ccc_impl_fhm_data_at(fhm_try_ins_ent_.h_,            \
                                               fhm_try_ins_ent_.handle_.i_),   \
                    .stats_ = CCC_ENTRY_VACANT,                                \
                };                                                             \
                *((typeof(lazy_value) *)fhm_try_insert_res_.e_) = lazy_value;  \
                ccc_impl_fhm_set_insert(&fhm_try_ins_ent_);                    \
            }                                                                  \
        }                                                                      \
        fhm_try_insert_res_;                                                   \
    }))

/** @private */
#define ccc_impl_fhm_insert_or_assign_w(flat_hash_map_ptr, key, lazy_value...) \
    (__extension__({                                                           \
        struct ccc_fhmap_ *flat_hash_map_ptr_ = (flat_hash_map_ptr);           \
        struct ccc_ent_ fhm_insert_or_assign_res_                              \
            = {.stats_ = CCC_ENTRY_ARG_ERROR};                                 \
        if (flat_hash_map_ptr_)                                                \
        {                                                                      \
            __auto_type fhm_key_ = key;                                        \
            struct ccc_fhash_entry_ fhm_ins_or_assign_ent_                     \
                = ccc_impl_fhm_entry(flat_hash_map_ptr_, (void *)&fhm_key_);   \
            if (fhm_ins_or_assign_ent_.handle_.stats_ & CCC_ENTRY_OCCUPIED)    \
            {                                                                  \
                fhm_insert_or_assign_res_ = (struct ccc_ent_){                 \
                    .e_                                                        \
                    = ccc_impl_fhm_data_at(fhm_ins_or_assign_ent_.h_,          \
                                           fhm_ins_or_assign_ent_.handle_.i_), \
                    .stats_ = fhm_ins_or_assign_ent_.handle_.stats_,           \
                };                                                             \
            }                                                                  \
            else if (fhm_ins_or_assign_ent_.handle_.stats_                     \
                     & CCC_ENTRY_INSERT_ERROR)                                 \
            {                                                                  \
                fhm_insert_or_assign_res_.e_ = NULL;                           \
                fhm_insert_or_assign_res_.stats_ = CCC_ENTRY_INSERT_ERROR;     \
            }                                                                  \
            else                                                               \
            {                                                                  \
                fhm_insert_or_assign_res_ = (struct ccc_ent_){                 \
                    .e_                                                        \
                    = ccc_impl_fhm_data_at(fhm_ins_or_assign_ent_.h_,          \
                                           fhm_ins_or_assign_ent_.handle_.i_), \
                    .stats_ = CCC_ENTRY_VACANT,                                \
                };                                                             \
                *((typeof(lazy_value) *)fhm_insert_or_assign_res_.e_)          \
                    = lazy_value;                                              \
                ccc_impl_fhm_set_insert(&fhm_ins_or_assign_ent_);              \
            }                                                                  \
        }                                                                      \
        fhm_insert_or_assign_res_;                                             \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_HASH_MAP_H */
