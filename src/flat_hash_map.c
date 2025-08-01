/** Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file implements an interpretation of Rust's Hashbrown Hash Map which in
turn is based on Google's Abseil Flat Hash Map. This implementation is based
on Rust's version which is slightly simpler and a better fit for C code. The
required license for this adaptation is included at the bottom of the file.
This implementation has changed a variety of types and data structures to work
within the C language and its aliasing rules. Here are the two original
implementations for reference.

Abseil: https://github.com/abseil/abseil-cpp
Hashbrown: https://github.com/rust-lang/hashbrown

This implementation is focused on SIMD friendly code or Portable Word based
code when SIMD is not available. In any case the goal is to query multiple
candidate keys for a match in the map simultaneously. This is achieved in the
best case by having 16 one-byte hash fingerprints analyzed simultaneously for
a match against a candidate fingerprint. The details of how this is done and
trade-offs involved can be found in the comments around the implementations
and data structures. The ARM NEON implementation may be updated if they add
better capabilities for 128 bit group operations. */
#include <assert.h>
#include <limits.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "flat_hash_map.h"
#include "impl/impl_flat_hash_map.h"
#include "impl/impl_types.h"
#include "types.h"

/*=========================   Platform Selection  ===========================*/

/** Note that these includes must come after inclusion of the
`impl/impl_flat_hash_map.h` header. Two platforms offer some form of vector
instructions we can try. */
#if defined(CCC_HAS_X86_SIMD)
#    include <immintrin.h>
#elif defined(CCC_HAS_ARM_SIMD)
#    include <arm_neon.h>
#endif /* defined(CCC_HAS_X86_SIMD) */

/** Maybe the compiler can give us better performance in key paths. */
#if defined(__has_builtin) && __has_builtin(__builtin_expect)
#    define unlikely(expr) __builtin_expect(!!(expr), 0)
#    define likely(expr) __builtin_expect(!!(expr), 1)
#else /* !defined(__has_builtin) || !__has_builtin(__builtin_expect) */
#    define unlikely(expr) expr
#    define likely(expr) expr
#endif /* defined(__has_builtin) && __has_builtin(__builtin_expect) */

/* Can we vectorize instructions? Also it is possible to specify we want a
portable implementation. Consider exposing to user in header docs. */
#if defined(CCC_HAS_X86_SIMD)

/** @private The 128 bit vector type for efficient SIMD group scanning. 16 one
byte large tags fit in this type. */
typedef struct
{
    __m128i v;
} group;

/** @private Because we use 128 bit vectors over tags the results of various
operations can be compressed into a 16 bit integer. */
typedef struct
{
    uint16_t v;
} match_mask;

enum : typeof((match_mask){}.v)
{
    /** @private MSB tag bit used for static assert. */
    MATCH_MASK_MSB = 0x8000,
    /** @private All bits on in a mask except for the 0th tag bit. */
    MATCH_MASK_0TH_TAG_OFF = 0xFFFE,
};

#elif defined(CCC_HAS_ARM_SIMD)

/** @private The 64 bit vector is used on NEON due to a lack of ability to
compress a 128 bit vector to a smaller int efficiently. */
typedef struct
{
    /** @private NEON offers a specific type for 64 bit manipulations. */
    uint8x8_t v;
} group;

/** @private The mask will consist of 8 bytes with the most significant bit of
each byte on to indicate match statuses. */
typedef struct
{
    /** @private NEON returns this type from various uint8x8_t operations. */
    uint64_t v;
} match_mask;

enum : uint64_t
{
    /** @private MSB tag bit used for static assert. */
    MATCH_MASK_MSB = 0x8000000000000000,
    /** @private MSB tag bits used for byte and word level masking. */
    MATCH_MASK_TAGS_MSBS = 0x8080808080808080,
    /** @private LSB tag bits used for byte and word level masking. */
    MATCH_MASK_TAGS_LSBS = 0x101010101010101,
    /** @private Debug mode check for bits that must be off in match. */
    MATCH_MASK_TAGS_OFF_BITS = 0x7F7F7F7F7F7F7F7F,
    /** @private The MSB of each byte on except 0th is 0x00. */
    MATCH_MASK_0TH_TAG_OFF = 0x8080808080808000,
};

enum : typeof((ccc_fhm_tag){}.v)
{
    /** @private Bits in a tag used to help in creating a group of one tag. */
    TAG_BITS = sizeof(ccc_fhm_tag) * CHAR_BIT,
};

#else /* PORTABLE FALLBACK */

/** @private The 8 byte word for managing multiple simultaneous equality checks.
In contrast to SIMD this group size is the same as the match. */
typedef struct
{
    /** @private 64 bits allows 8 tags to be checked at once. */
    uint64_t v;
} group;

/** @private The match is the same size as the group because only the most
significant bit in a byte within the mask will be on to indicate the result of
various queries such as matching a tag, empty, or constant. */
typedef struct
{
    /** @private The match is the same as a group with MSB on. */
    typeof((group){}.v) v;
} match_mask;

enum : typeof((group){}.v)
{
    /** @private MSB tag bit used for static assert. */
    MATCH_MASK_MSB = 0x8000000000000000,
    /** @private MSB tag bits used for byte and word level masking. */
    MATCH_MASK_TAGS_MSBS = 0x8080808080808080,
    /** @private The EMPTY special constant tag in every byte of the mask. */
    MATCH_MASK_TAGS_EMPTY = 0x8080808080808080,
    /** @private LSB tag bits used for byte and word level masking. */
    MATCH_MASK_TAGS_LSBS = 0x101010101010101,
    /** @private Debug mode check for bits that must be off in match. */
    MATCH_MASK_TAGS_OFF_BITS = 0x7F7F7F7F7F7F7F7F,
    /** @private The MSB of each byte on except 0th is 0x00. */
    MATCH_MASK_0TH_TAG_OFF = 0x8080808080808000,
};

enum : typeof((ccc_fhm_tag){}.v)
{
    /** @private Bits in a tag used to help in creating a group of one tag. */
    TAG_BITS = sizeof(ccc_fhm_tag) * CHAR_BIT,
};

#endif /* defined(CCC_HAS_X86_SIMD) */

/*=======================   Data Alignment Test   ===========================*/

/** @private A macro version of the runtime alignment operations we perform
for calculating bytes. This way we can use in static asserts. */
#define comptime_roundup(bytes_to_round)                                       \
    (((bytes_to_round) + CCC_FHM_GROUP_SIZE - 1) & ~(CCC_FHM_GROUP_SIZE - 1))

/** @private The following test should ensure some safety in assumptions we make
when the user defines a fixed size map type. This is just a small type that
will remain internal to this translation unit. The tag array is not given a
replica group size at the end of its allocation because that wastes pointless
space and has no impact on the following layout and pointer arithmetic tests.
One behavior we want to ensure is that the tag array starts on the exact next
byte after the user data type because the tag array has no alignment
requirement: it is only a byte so any address following the data array will be
aligned. */
struct fixed_map_test_type
{
    struct
    {
        int i;
    } data[2 + 1];
    alignas(CCC_FHM_GROUP_SIZE) ccc_fhm_tag tag[2];
};
/** The type must actually get an allocation on the given platform to validate
some memory layout assumptions. This should be sufficient and the assumptions
will hold if the user happens to allocate a fixed size map on the stack. */
static struct fixed_map_test_type data_tag_layout_test;
/** The size of the user data and tags should be what we expect. No hidden
padding should violate our mental model of the bytes occupied by contiguous user
data and metadata tags. We don't care about padding after the tag array which
may very well exist. The continuity from the base of the user data to the start
of the tags array is the most important aspect for fixed size maps.

Over index the tag array to get the end address for pointer differences. The
0th element in an array at the start of a struct is guaranteed to start at the
first byte of the struct with no padding BEFORE this first element. */
static_assert(
    (char *)&data_tag_layout_test.tag[2] - (char *)&data_tag_layout_test.data[0]
        == (comptime_roundup((sizeof(data_tag_layout_test.data)))
            + sizeof(ccc_fhm_tag) * 2),
    "The size in bytes of the contiguous user data to tag array must be what "
    "we would expect with no padding that will interfere with pointer "
    "arithmetic.");
/** We must ensure that the tags array starts at the exact next byte boundary
after the user data type. This is required due to how we access tags and user
data via indexing. Data is accessed with pointer subtraction from the tags
array. The tags array 0th element is the shared base for both arrays.

Data has decreasing indices and tags have ascending indices. Here we index too
far for our type with padding to ensure the next assertion will hold when we
index from the shared tags base address and subtract to find user data. */
static_assert((char *)&data_tag_layout_test.data
                      + comptime_roundup((sizeof(data_tag_layout_test.data)))
                  == (char *)&data_tag_layout_test.tag,
              "The start of the tag array must align perfectly with the next "
              "byte past the final user data element.");
/** Same as the above test but this is how the location of the tag array would
be found in normal runtime code. */
static_assert(
    (char *)&data_tag_layout_test.tag
        == (char *)&data_tag_layout_test.data
               + comptime_roundup((sizeof(data_tag_layout_test.data))),
    "Manual pointer arithmetic from the base of data array to find "
    "tag array should result in correct location.");
/** We only want to align the tag array to the correct byte boundary otherwise
we could be aligning all the user data elements to a larger alignment than
needed, wasting space. This makes other code slightly more complicated but it
is better for space usage and aligned performance. We must be careful to account
for the padding between the end of the data array and the base for pointer
arithmetic and difference calculations. */
static_assert((offsetof(struct fixed_map_test_type, tag) % CCC_FHM_GROUP_SIZE)
                  == 0,
              "The tag array starts at an aligned group size byte boundary "
              "within the struct.");

/*=======================    Special Constants    ===========================*/

/** @private Range of constants specified as special for this hash table. Same
general design as Rust Hashbrown table. Importantly, we know these are special
constants because the most significant bit is on and then empty can be easily
distinguished from deleted by the least significant bit.

The full case is implicit in the table as it cannot be quantified by a simple
enum value.

TAG_FULL = 0b0???_????

The most significant bit is off and the lower 7 make up the hash bits. */
enum : typeof((ccc_fhm_tag){}.v)
{
    /** @private Deleted is applied when a removed value in a group must signal
    to a probe sequence to continue searching for a match or empty to stop. */
    TAG_DELETED = 0x80,
    /** @private Empty is the starting tag value and applied when other empties
    are in a group upon removal. */
    TAG_EMPTY = 0xFF,
    /** @private Used to verify if tag is constant or hash data. */
    TAG_MSB = TAG_DELETED,
    /** @private Used to create a one byte fingerprint of user hash. */
    TAG_LOWER_7_MASK = (typeof((ccc_fhm_tag){}.v))~TAG_DELETED,
};
static_assert(sizeof(ccc_fhm_tag) == sizeof(uint8_t),
              "tag must wrap a byte in a struct without padding for better "
              "optimizations and no strict-aliasing exceptions.");
static_assert(
    (TAG_DELETED | TAG_EMPTY) == (typeof((ccc_fhm_tag){}.v))~0,
    "all bits must be accounted for across deleted and empty status.");
static_assert(
    (TAG_DELETED ^ TAG_EMPTY) == 0x7F,
    "only empty should have lsb on and 7 bits are available for hash");

/*=======================    Type Declarations    ===========================*/

/** @private A triangular sequence of numbers is a probing sequence that will
visit every group in a power of 2 capacity hash table. Here is a popular proof:
https://fgiesen.wordpress.com/2015/02/22/triangular-numbers-mod-2n/

See also Donald Knuth's The Art of Computer Programming Volume 3, Chapter 6.4,
Answers to Exercises, problem 20, page 731 for another proof. */
struct probe_sequence
{
    /** @private The index this probe step has placed us on. */
    size_t i;
    /** @private Stride increases by group size on each iteration. */
    size_t stride;
};

/** @private Helper type for obtaining a search result on the map. */
struct query
{
    /** The slot in the table. */
    size_t i;
    /** Status indicating occupied, vacant, or possible error. */
    enum ccc_entry_status stats;
};

/*===========================   Prototypes   ================================*/

static void swap(void *tmp, void *a, void *b, size_t ab_size);
static struct ccc_fhash_entry container_entry(struct ccc_fhmap *h,
                                              void const *key);
static struct query find(struct ccc_fhmap *h, void const *key, uint64_t hash);
static struct query find_key_or_slot(struct ccc_fhmap const *h, void const *key,
                                     uint64_t hash);
static ccc_ucount find_key_or_fail(struct ccc_fhmap const *h, void const *key,
                                   uint64_t hash);
static size_t find_slot_or_noreturn(struct ccc_fhmap const *h, uint64_t hash);
static void *find_first_full_slot(struct ccc_fhmap const *h, size_t start);
static match_mask find_first_full_group(struct ccc_fhmap const *h,
                                        size_t *start);
static ccc_result maybe_rehash(struct ccc_fhmap *h, size_t to_add,
                               ccc_any_alloc_fn);
static void insert_and_copy(struct ccc_fhmap *h, void const *key_val_type,
                            ccc_fhm_tag m, size_t i);
static void erase(struct ccc_fhmap *h, size_t i);
static ccc_result check_init(struct ccc_fhmap *, size_t required_total_cap,
                             ccc_any_alloc_fn *);
static void rehash_in_place(struct ccc_fhmap *h);
static ccc_tribool is_same_group(size_t i, size_t new_i, uint64_t hash,
                                 size_t mask);
static ccc_result rehash_resize(struct ccc_fhmap *h, size_t to_add,
                                ccc_any_alloc_fn);
static ccc_tribool eq_fn(struct ccc_fhmap const *h, void const *key, size_t i);
static uint64_t hash_fn(struct ccc_fhmap const *h, void const *any_key);
static void *key_at(struct ccc_fhmap const *h, size_t i);
static void *data_at(struct ccc_fhmap const *h, size_t i);
static ccc_fhm_tag *tag_pos(size_t sizeof_type, void const *data, size_t mask);
static void *key_in_slot(struct ccc_fhmap const *h, void const *slot);
static void *swap_slot(struct ccc_fhmap const *h);
static ccc_ucount data_i(struct ccc_fhmap const *h, void const *data_slot);
static size_t mask_to_total_bytes(size_t sizeof_type, size_t mask);
static size_t mask_to_tag_bytes(size_t mask);
static size_t mask_to_data_bytes(size_t sizeof_type, size_t mask);
static void set_insert_tag(struct ccc_fhmap *h, ccc_fhm_tag m, size_t i);
static size_t mask_to_load_factor_cap(size_t mask);
static size_t max(size_t a, size_t b);
static void tag_set(struct ccc_fhmap *h, ccc_fhm_tag m, size_t i);
static ccc_tribool match_has_one(match_mask m);
static size_t match_trailing_one(match_mask m);
static size_t match_leading_zeros(match_mask m);
static size_t match_trailing_zeros(match_mask m);
static size_t match_next_one(match_mask *m);
static ccc_tribool tag_full(ccc_fhm_tag m);
static ccc_tribool tag_constant(ccc_fhm_tag m);
static ccc_fhm_tag tag_from(uint64_t hash);
static group group_loadu(ccc_fhm_tag const *src);
static group group_loada(ccc_fhm_tag const *src);
static void group_storea(ccc_fhm_tag *dst, group src);
static match_mask match_tag(group g, ccc_fhm_tag m);
static match_mask match_empty(group g);
static match_mask match_deleted(group g);
static match_mask match_empty_deleted(group g);
static match_mask match_full(group g);
static match_mask match_leading_full(group g, size_t start_tag);
static group group_constant_to_empty_full_to_deleted(group g);
static unsigned ctz(match_mask m);
static unsigned clz(match_mask m);
static unsigned clz_size_t(size_t n);
static size_t next_power_of_two(size_t n);
static ccc_tribool is_power_of_two(size_t n);
static size_t to_power_of_two(size_t n);
static ccc_tribool is_uninitialized(struct ccc_fhmap const *);
static void destory_each(struct ccc_fhmap *h, ccc_any_type_destructor_fn *);
static size_t roundup(size_t bytes);

/*===========================    Interface   ================================*/

ccc_tribool
ccc_fhm_is_empty(ccc_flat_hash_map const *const h)
{
    if (unlikely(!h))
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !h->count;
}

ccc_ucount
ccc_fhm_count(ccc_flat_hash_map const *const h)
{
    if (!h)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = h->count};
}

ccc_ucount
ccc_fhm_capacity(ccc_flat_hash_map const *const h)
{
    if (!h)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = h->mask + 1};
}

ccc_tribool
ccc_fhm_contains(ccc_flat_hash_map const *const h, void const *const key)
{
    if (unlikely(!h || !key))
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (unlikely(is_uninitialized(h) || !h->count))
    {
        return CCC_FALSE;
    }
    return !find_key_or_fail(h, key, hash_fn(h, key)).error;
}

void *
ccc_fhm_get_key_val(ccc_flat_hash_map const *const h, void const *const key)
{
    if (unlikely(!h || !key || is_uninitialized(h) || !h->count))
    {
        return NULL;
    }
    ccc_ucount const i = find_key_or_fail(h, key, hash_fn(h, key));
    if (i.error)
    {
        return NULL;
    }
    return data_at(h, i.count);
}

ccc_fhmap_entry
ccc_fhm_entry(ccc_flat_hash_map *const h, void const *const key)
{
    if (unlikely(!h || !key))
    {
        return (ccc_fhmap_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    return (ccc_fhmap_entry){container_entry(h, key)};
}

void *
ccc_fhm_or_insert(ccc_fhmap_entry const *const e, void const *key_val_type)
{
    if (unlikely(!e || !key_val_type))
    {
        return NULL;
    }
    if (e->impl.stats & CCC_ENTRY_OCCUPIED)
    {
        return data_at(e->impl.h, e->impl.i);
    }
    if (e->impl.stats & CCC_ENTRY_INSERT_ERROR)
    {
        return NULL;
    }
    insert_and_copy(e->impl.h, key_val_type, e->impl.tag, e->impl.i);
    return data_at(e->impl.h, e->impl.i);
}

void *
ccc_fhm_insert_entry(ccc_fhmap_entry const *const e, void const *key_val_type)
{
    if (unlikely(!e || !key_val_type))
    {
        return NULL;
    }
    if (e->impl.stats & CCC_ENTRY_OCCUPIED)
    {
        void *const slot = data_at(e->impl.h, e->impl.i);
        (void)memcpy(slot, key_val_type, e->impl.h->sizeof_type);
        return slot;
    }
    if (e->impl.stats & CCC_ENTRY_INSERT_ERROR)
    {
        return NULL;
    }
    insert_and_copy(e->impl.h, key_val_type, e->impl.tag, e->impl.i);
    return data_at(e->impl.h, e->impl.i);
}

ccc_entry
ccc_fhm_remove_entry(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    if (!(e->impl.stats & CCC_ENTRY_OCCUPIED))
    {
        return (ccc_entry){{.stats = CCC_ENTRY_VACANT}};
    }
    erase(e->impl.h, e->impl.i);
    return (ccc_entry){{.stats = CCC_ENTRY_OCCUPIED}};
}

ccc_fhmap_entry *
ccc_fhm_and_modify(ccc_fhmap_entry *const e, ccc_any_type_update_fn *const fn)
{
    if (e && fn && (e->impl.stats & CCC_ENTRY_OCCUPIED) != 0)
    {
        fn((ccc_any_type){
            .any_type = data_at(e->impl.h, e->impl.i),
            .aux = NULL,
        });
    }
    return e;
}

ccc_fhmap_entry *
ccc_fhm_and_modify_aux(ccc_fhmap_entry *const e,
                       ccc_any_type_update_fn *const fn, void *const aux)
{
    if (e && fn && (e->impl.stats & CCC_ENTRY_OCCUPIED) != 0)
    {
        fn((ccc_any_type){
            .any_type = data_at(e->impl.h, e->impl.i),
            .aux = aux,
        });
    }
    return e;
}

ccc_entry
ccc_fhm_swap_entry(ccc_flat_hash_map *const h, void *const key_val_type_output)
{
    if (unlikely(!h || !key_val_type_output))
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    void *const key = key_in_slot(h, key_val_type_output);
    struct ccc_fhash_entry ent = container_entry(h, key);
    if (ent.stats & CCC_ENTRY_OCCUPIED)
    {
        swap(swap_slot(h), data_at(h, ent.i), key_val_type_output,
             h->sizeof_type);
        return (ccc_entry){{
            .e = key_val_type_output,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    if (ent.stats & CCC_ENTRY_INSERT_ERROR)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_INSERT_ERROR}};
    }
    insert_and_copy(ent.h, key_val_type_output, ent.tag, ent.i);
    return (ccc_entry){{
        .e = data_at(h, ent.i),
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_entry
ccc_fhm_try_insert(ccc_flat_hash_map *const h, void const *const key_val_type)
{
    if (unlikely(!h || !key_val_type))
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    void *const key = key_in_slot(h, key_val_type);
    struct ccc_fhash_entry ent = container_entry(h, key);
    if (ent.stats & CCC_ENTRY_OCCUPIED)
    {
        return (ccc_entry){{
            .e = data_at(h, ent.i),
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    if (ent.stats & CCC_ENTRY_INSERT_ERROR)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_INSERT_ERROR}};
    }
    insert_and_copy(ent.h, key_val_type, ent.tag, ent.i);
    return (ccc_entry){{
        .e = data_at(h, ent.i),
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_entry
ccc_fhm_insert_or_assign(ccc_flat_hash_map *const h,
                         void const *const key_val_type)
{
    if (unlikely(!h || !key_val_type))
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    void *const key = key_in_slot(h, key_val_type);
    struct ccc_fhash_entry ent = container_entry(h, key);
    if (ent.stats & CCC_ENTRY_OCCUPIED)
    {
        (void)memcpy(data_at(h, ent.i), key_val_type, h->sizeof_type);
        return (ccc_entry){{
            .e = data_at(h, ent.i),
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    if (ent.stats & CCC_ENTRY_INSERT_ERROR)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_INSERT_ERROR}};
    }
    insert_and_copy(ent.h, key_val_type, ent.tag, ent.i);
    return (ccc_entry){{
        .e = data_at(h, ent.i),
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_entry
ccc_fhm_remove(ccc_flat_hash_map *const h, void *const key_val_type_output)
{
    if (unlikely(!h || !key_val_type_output))
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    if (unlikely(is_uninitialized(h) || !h->count))
    {
        return (ccc_entry){{.stats = CCC_ENTRY_VACANT}};
    }
    void *const key = key_in_slot(h, key_val_type_output);
    ccc_ucount const index = find_key_or_fail(h, key, hash_fn(h, key));
    if (index.error)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_VACANT}};
    }
    (void)memcpy(key_val_type_output, data_at(h, index.count), h->sizeof_type);
    erase(h, index.count);
    return (ccc_entry){{
        .e = key_val_type_output,
        .stats = CCC_ENTRY_OCCUPIED,
    }};
}

void *
ccc_fhm_begin(ccc_flat_hash_map const *const h)
{
    if (unlikely(!h || !h->mask || is_uninitialized(h) || !h->count))
    {
        return NULL;
    }
    return find_first_full_slot(h, 0);
}

void *
ccc_fhm_next(ccc_flat_hash_map const *const h,
             void const *const key_val_type_iter)
{
    if (unlikely(!h || !key_val_type_iter || !h->mask || is_uninitialized(h)
                 || !h->count))
    {
        return NULL;
    }
    ccc_ucount i = data_i(h, key_val_type_iter);
    if (i.error)
    {
        return NULL;
    }
    size_t const aligned_group_start
        = i.count & ~((typeof(i.count))(CCC_FHM_GROUP_SIZE - 1));
    match_mask m = match_leading_full(group_loada(&h->tag[aligned_group_start]),
                                      i.count & (CCC_FHM_GROUP_SIZE - 1));
    size_t const bit = match_next_one(&m);
    if (bit != CCC_FHM_GROUP_SIZE)
    {
        return data_at(h, aligned_group_start + bit);
    }
    return find_first_full_slot(h, aligned_group_start + CCC_FHM_GROUP_SIZE);
}

void *
ccc_fhm_end(ccc_flat_hash_map const *const)
{
    return NULL;
}

void *
ccc_fhm_unwrap(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e) || !(e->impl.stats & CCC_ENTRY_OCCUPIED))
    {
        return NULL;
    }
    return data_at(e->impl.h, e->impl.i);
}

ccc_result
ccc_fhm_clear(ccc_flat_hash_map *const h, ccc_any_type_destructor_fn *const fn)
{
    if (unlikely(!h))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (unlikely(is_uninitialized(h) || !h->mask || !h->tag))
    {
        return CCC_RESULT_OK;
    }
    if (!fn)
    {
        (void)memset(h->tag, TAG_EMPTY, mask_to_tag_bytes(h->mask));
        h->remain = mask_to_load_factor_cap(h->mask);
        h->count = 0;
        return CCC_RESULT_OK;
    }
    destory_each(h, fn);
    (void)memset(h->tag, TAG_EMPTY, mask_to_tag_bytes(h->mask));
    h->remain = mask_to_load_factor_cap(h->mask);
    h->count = 0;
    return CCC_RESULT_OK;
}

ccc_result
ccc_fhm_clear_and_free(ccc_flat_hash_map *const h,
                       ccc_any_type_destructor_fn *const fn)
{
    if (unlikely(!h || !h->data || !h->mask || is_uninitialized(h)))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!h->alloc_fn)
    {
        (void)ccc_fhm_clear(h, fn);
        return CCC_RESULT_NO_ALLOC;
    }
    if (fn)
    {
        destory_each(h, fn);
    }
    h->remain = 0;
    h->mask = 0;
    h->count = 0;
    h->tag = NULL;
    (void)h->alloc_fn(h->data, 0, h->aux);
    h->data = NULL;
    return CCC_RESULT_OK;
}

ccc_result
ccc_fhm_clear_and_free_reserve(ccc_flat_hash_map *const h,
                               ccc_any_type_destructor_fn *const destructor,
                               ccc_any_alloc_fn *const alloc)
{
    if (unlikely(!h || !h->data || is_uninitialized(h) || !h->mask
                 || (h->alloc_fn && h->alloc_fn != alloc)))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!alloc)
    {
        (void)ccc_fhm_clear(h, destructor);
        return CCC_RESULT_NO_ALLOC;
    }
    if (destructor)
    {
        destory_each(h, destructor);
    }
    h->remain = 0;
    h->mask = 0;
    h->count = 0;
    h->tag = NULL;
    (void)alloc(h->data, 0, h->aux);
    h->data = NULL;
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_fhm_occupied(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_OCCUPIED) != 0;
}

ccc_tribool
ccc_fhm_insert_error(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

ccc_entry_status
ccc_fhm_entry_status(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return CCC_ENTRY_ARG_ERROR;
    }
    return e->impl.stats;
}

void *
ccc_fhm_data(ccc_flat_hash_map const *h)
{
    return h ? h->data : NULL;
}

ccc_result
ccc_fhm_copy(ccc_flat_hash_map *const dst, ccc_flat_hash_map const *const src,
             ccc_any_alloc_fn *const fn)
{
    if (!dst || !src || src == dst
        || (src->mask && !is_power_of_two(src->mask + 1)))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (dst->mask < src->mask && !fn)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    ccc_result check = check_init(dst, 0, fn);
    if (check != CCC_RESULT_OK)
    {
        return check;
    }
    /* The destination could be messed up in a variety of ways that make it
       incompatible with src. Overwrite everything and save what we need from
       dst for a smooth copy over. */
    void *const dst_data = dst->data;
    void *const dst_tag = dst->tag;
    size_t const dst_mask = dst->mask;
    size_t const dst_remain = dst->remain;
    ccc_any_alloc_fn *const dst_alloc = dst->alloc_fn;
    *dst = *src;
    dst->data = dst_data;
    dst->tag = dst_tag;
    dst->mask = dst_mask;
    dst->remain = dst_remain;
    dst->alloc_fn = dst_alloc;
    if (!src->mask || is_uninitialized(src))
    {
        return CCC_RESULT_OK;
    }
    size_t const src_bytes = mask_to_total_bytes(src->sizeof_type, src->mask);
    if (dst->mask < src->mask)
    {
        void *const new_mem = dst->alloc_fn(dst->data, src_bytes, dst->aux);
        if (!new_mem)
        {
            return CCC_RESULT_MEM_ERROR;
        }
        dst->data = new_mem;
        dst->tag = tag_pos(src->sizeof_type, new_mem, src->mask);
        dst->mask = src->mask;
    }
    if (!dst->data || !src->data)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    (void)memset(dst->tag, TAG_EMPTY, mask_to_tag_bytes(dst->mask));
    dst->remain = mask_to_load_factor_cap(dst->mask);
    dst->count = 0;
    size_t group_start = 0;
    match_mask full = {};
    while ((full = find_first_full_group(src, &group_start)).v)
    {
        size_t tag_i = 0;
        while ((tag_i = match_next_one(&full)) != CCC_FHM_GROUP_SIZE)
        {
            tag_i += group_start;
            uint64_t const hash = hash_fn(src, key_at(src, tag_i));
            size_t const new_i = find_slot_or_noreturn(dst, hash);
            tag_set(dst, tag_from(hash), new_i);
            (void)memcpy(data_at(dst, new_i), data_at(src, tag_i),
                         dst->sizeof_type);
        }
        group_start += CCC_FHM_GROUP_SIZE;
    }
    dst->remain -= src->count;
    dst->count = src->count;
    return CCC_RESULT_OK;
}

ccc_result
ccc_fhm_reserve(ccc_flat_hash_map *const h, size_t const to_add,
                ccc_any_alloc_fn *const fn)
{
    if (unlikely(!h || !to_add || !fn))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    return maybe_rehash(h, to_add, fn);
}

ccc_tribool
ccc_fhm_validate(ccc_flat_hash_map const *const h)
{
    if (!h)
    {
        return CCC_TRIBOOL_ERROR;
    }
    /* We initialized the metadata array of 0 capacity table? Not possible. */
    if (!is_uninitialized(h) && !h->mask)
    {
        return CCC_FALSE;
    }
    /* No point checking invariants when lazy init hasn't happened yet. */
    if (is_uninitialized(h) || !h->mask)
    {
        return CCC_TRUE;
    }
    /* We are initialized, these need to point to the array positions. */
    if (!h->data || !h->tag)
    {
        return CCC_FALSE;
    }
    /* The replica group should be in sync. */
    for (size_t original = 0, clone = (h->mask + 1);
         original < CCC_FHM_GROUP_SIZE; ++original, ++clone)
    {
        if (h->tag[original].v != h->tag[clone].v)
        {
            return CCC_FALSE;
        }
    }
    size_t occupied = 0;
    size_t remain = 0;
    size_t deleted = 0;
    for (size_t i = 0; i < (h->mask + 1); ++i)
    {
        ccc_fhm_tag const t = h->tag[i];
        /* If we are a special constant there are only two possible values. */
        if (tag_constant(t) && t.v != TAG_DELETED && t.v != TAG_EMPTY)
        {
            return CCC_FALSE;
        }
        if (t.v == TAG_EMPTY)
        {
            ++remain;
        }
        else if (t.v == TAG_DELETED)
        {
            ++deleted;
        }
        else if (tag_full(t))
        {
            if (tag_from(hash_fn(h, data_at(h, i))).v != t.v)
            {
                return CCC_FALSE;
            }
            ++occupied;
        }
    }
    /* Do our tags agree with our manually tracked and set state? */
    if (occupied != h->count || occupied + remain + deleted != h->mask + 1
        || mask_to_load_factor_cap(occupied + remain + deleted) - occupied
                   - deleted
               != h->remain)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

/*======================     Private Interface      =========================*/

struct ccc_fhash_entry
ccc_impl_fhm_entry(struct ccc_fhmap *const h, void const *const key)
{
    return container_entry(h, key);
}

void
ccc_impl_fhm_insert(struct ccc_fhmap *h, void const *key_val_type,
                    ccc_fhm_tag m, size_t i)
{
    insert_and_copy(h, key_val_type, m, i);
}

void
ccc_impl_fhm_erase(struct ccc_fhmap *h, size_t i)
{
    erase(h, i);
}

void *
ccc_impl_fhm_data_at(struct ccc_fhmap const *const h, size_t const i)
{
    return data_at(h, i);
}

void *
ccc_impl_fhm_key_at(struct ccc_fhmap const *const h, size_t const i)
{
    return key_at(h, i);
}

/* This is needed to help the macros only set a new insert conditionally. */
void
ccc_impl_fhm_set_insert(struct ccc_fhash_entry const *const e)
{
    return set_insert_tag(e->h, e->tag, e->i);
}

/*=========================   Static Internals   ============================*/

/** Returns the container entry prepared for further insertion, removal, or
searched queries. This entry gives a reference to the associated map and any
metadata and location info necessary for future actions. If this entry was
obtained in hopes of insertions but insertion will cause an error. A status
flag in the handle field will indicate the error. */
static struct ccc_fhash_entry
container_entry(struct ccc_fhmap *const h, void const *const key)
{
    uint64_t const hash = hash_fn(h, key);
    struct query const e = find(h, key, hash);
    return (struct ccc_fhash_entry){
        .h = (struct ccc_fhmap *)h,
        .tag = tag_from(hash),
        .i = e.i,
        .stats = e.stats,
    };
}

/** Obtaining a handle may fail if a resize or rehash fails but certain queries
must continue with that information. The status of the handle will indicate if
an entry is occupied, vacant, or some error has occurred. */
static struct query
find(struct ccc_fhmap *const h, void const *const key, uint64_t const hash)
{
    ccc_entry_status upcoming_insertion_error = 0;
    switch (maybe_rehash(h, 1, h->alloc_fn))
    {
        case CCC_RESULT_OK:
            break;
        case CCC_RESULT_ARG_ERROR:
            return (struct query){.stats = CCC_ENTRY_ARG_ERROR};
            break;
        default:
            upcoming_insertion_error = CCC_ENTRY_INSERT_ERROR;
            break;
    };
    struct query res = find_key_or_slot(h, key, hash);
    res.stats |= upcoming_insertion_error;
    return res;
}

/** Sets the insert tag meta data and copies the user type into the associated
data slot. It is user's responsibility to ensure that the insert is valid. */
static inline void
insert_and_copy(struct ccc_fhmap *const h, void const *const key_val_type,
                ccc_fhm_tag const m, size_t const i)
{
    set_insert_tag(h, m, i);
    (void)memcpy(data_at(h, i), key_val_type, h->sizeof_type);
}

/** Sets the insert tag meta data. It is user's responsibility to ensure that
the insert is valid. */
static inline void
set_insert_tag(struct ccc_fhmap *const h, ccc_fhm_tag const m, size_t const i)
{
    assert(i <= h->mask);
    assert((m.v & TAG_MSB) == 0);
    h->remain -= (h->tag[i].v == TAG_EMPTY);
    ++h->count;
    tag_set(h, m, i);
}

/** Erases an element at the provided index from the tag array, forfeiting its
data in the data array for re-use later. The erase procedure decides how to mark
a removal from the table: deleted or empty. Which option to choose is
determined by what is required to ensure the probing sequence works correctly in
all future cases. */
static inline void
erase(struct ccc_fhmap *const h, size_t const i)
{
    assert(i <= h->mask);
    size_t const prev_i = (i - CCC_FHM_GROUP_SIZE) & h->mask;
    match_mask const prev_empties = match_empty(group_loadu(&h->tag[prev_i]));
    match_mask const empties = match_empty(group_loadu(&h->tag[i]));
    /* Leading means start at most significant bit aka last group member.
       Trailing means start at the least significant bit aka first group member.

       Marking the slot as empty is ideal. This will allow future probe
       sequences to stop as early as possible for best performance.

       However, we have asked how many DELETED or FULL slots are before and
       after our current position. If the answer is greater than or equal to the
       size of a group we must mark ourselves as deleted so that probing does
       not stop too early. All the other entries in this group are either full
       or deleted and empty would incorrectly signal to search functions that
       the requested value does not exist in the table. Instead, the request
       needs to see that hash collisions or removals have created displacements
       that must be probed past to be sure the element in question is absent.

       Because probing operates on groups this check ensures that any group
       load at any position that includes this item will continue as long as
       needed to ensure the searched key is absent. An important edge case this
       covers is one in which the previous group is completely full of FULL or
       DELETED entries and this tag will be the first in the next group. This
       is an important case where we must mark our tag as deleted. */
    ccc_fhm_tag const m
        = (match_leading_zeros(prev_empties) + match_trailing_zeros(empties)
           >= CCC_FHM_GROUP_SIZE)
            ? (ccc_fhm_tag){TAG_DELETED}
            : (ccc_fhm_tag){TAG_EMPTY};
    h->remain += (TAG_EMPTY == m.v);
    --h->count;
    tag_set(h, m, i);
}

/** Finds the specified hash or first available slot where the hash could be
inserted. If the element does not exist and a non-occupied slot is returned
that slot will have been the first empty or deleted slot encountered in the
probe sequence. This function assumes an empty slot exists in the table. */
static struct query
find_key_or_slot(struct ccc_fhmap const *const h, void const *const key,
                 uint64_t const hash)
{
    ccc_fhm_tag const tag = tag_from(hash);
    size_t const mask = h->mask;
    struct probe_sequence p = {
        .i = hash & mask,
        .stride = 0,
    };
    ccc_ucount empty_deleted = {.error = CCC_RESULT_FAIL};
    do
    {
        group const g = group_loadu(&h->tag[p.i]);
        match_mask m = match_tag(g, tag);
        size_t tag_i = 0;
        while ((tag_i = match_next_one(&m)) != CCC_FHM_GROUP_SIZE)
        {
            tag_i = (p.i + tag_i) & mask;
            if (likely(eq_fn(h, key, tag_i)))
            {
                return (struct query){
                    .i = tag_i,
                    .stats = CCC_ENTRY_OCCUPIED,
                };
            }
        }
        /* Taking the first available slot once probing is done is important
           to preserve probing operation and efficiency. */
        if (likely(empty_deleted.error))
        {
            size_t const i_take = match_trailing_one(match_empty_deleted(g));
            if (likely(i_take != CCC_FHM_GROUP_SIZE))
            {
                empty_deleted.count = (p.i + i_take) & mask;
                empty_deleted.error = CCC_RESULT_OK;
            }
        }
        if (likely(match_has_one(match_empty(g))))
        {
            return (struct query){
                .i = empty_deleted.count,
                .stats = CCC_ENTRY_VACANT,
            };
        }
        p.stride += CCC_FHM_GROUP_SIZE;
        p.i += p.stride;
        p.i &= mask;
    }
    while (1);
}

/** Finds key or fails when first empty slot is encountered after a group fails
to match. If the search is successful the ucount holds the index of the desired
key, otherwise the ucount holds the failure status flag and the index is
undefined. This index would not be helpful if an insert slot is desired because
we may have passed preferred deleted slots for insertion to find this empty one.

This function is better when a simple lookup is needed as a few branches and
loads are omitted compared to the search with intention to insert or remove. */
static ccc_ucount
find_key_or_fail(struct ccc_fhmap const *const h, void const *const key,
                 uint64_t const hash)
{
    ccc_fhm_tag const tag = tag_from(hash);
    size_t const mask = h->mask;
    struct probe_sequence p = {
        .i = hash & mask,
        .stride = 0,
    };
    do
    {
        group const g = group_loadu(&h->tag[p.i]);
        match_mask m = match_tag(g, tag);
        size_t tag_i = 0;
        while ((tag_i = match_next_one(&m)) != CCC_FHM_GROUP_SIZE)
        {
            tag_i = (p.i + tag_i) & mask;
            if (likely(eq_fn(h, key, tag_i)))
            {
                return (ccc_ucount){.count = tag_i};
            }
        }
        if (likely(match_has_one(match_empty(g))))
        {
            return (ccc_ucount){.error = CCC_RESULT_FAIL};
        }
        p.stride += CCC_FHM_GROUP_SIZE;
        p.i += p.stride;
        p.i &= mask;
    }
    while (1);
}

/** Finds the first available empty or deleted insert slot or loops forever. The
caller of this function must know that there is an available empty or deleted
slot in the table. */
static size_t
find_slot_or_noreturn(struct ccc_fhmap const *const h, uint64_t const hash)
{
    size_t const mask = h->mask;
    struct probe_sequence p = {
        .i = hash & mask,
        .stride = 0,
    };
    do
    {
        size_t const i = match_trailing_one(
            match_empty_deleted(group_loadu(&h->tag[p.i])));
        if (likely(i != CCC_FHM_GROUP_SIZE))
        {
            return (p.i + i) & mask;
        }
        p.stride += CCC_FHM_GROUP_SIZE;
        p.i += p.stride;
        p.i &= mask;
    }
    while (1);
}

/** Finds the first occupied slot in the table. The full slot is one where the
user has hash bits occupying the lower 7 bits of the tag. Assumes that the start
index is the base index of a group of tags such that as we scan groups the
loads are aligned for performance. */
static inline void *
find_first_full_slot(struct ccc_fhmap const *const h, size_t start)
{
    assert((start & ~((size_t)(CCC_FHM_GROUP_SIZE - 1))) == start);
    while (start < (h->mask + 1))
    {
        size_t const full
            = match_trailing_one(match_full(group_loada(&h->tag[start])));
        if (full != CCC_FHM_GROUP_SIZE)
        {
            return data_at(h, start + full);
        }
        start += CCC_FHM_GROUP_SIZE;
    }
    return NULL;
}

/** Returns the first full group mask if found and progresses the start index
as needed to find the index corresponding to the first element of this group.
If no group with a full slot is found a 0 mask is returned and the index will
have been progressed past mask + 1 aka capacity.

Assumes that start is aligned to the 0th tag of a group and only progresses
start by the size of a group such that it is always aligned. */
static inline match_mask
find_first_full_group(struct ccc_fhmap const *const h, size_t *const start)
{
    assert((*start & ~((size_t)(CCC_FHM_GROUP_SIZE - 1))) == *start);
    while (*start < (h->mask + 1))
    {
        match_mask const full = match_full(group_loada(&h->tag[*start]));
        if (full.v)
        {
            return full;
        }
        *start += CCC_FHM_GROUP_SIZE;
    }
    return (match_mask){};
}

/** Returns the first deleted group mask if found and progresses the start index
as needed to find the index corresponding to the first deleted element of this
group. If no group with a deleted slot is found a 0 mask is returned and the
index will have been progressed past mask + 1 aka capacity.

Assumes that start is aligned to the 0th tag of a group and only progresses
start by the size of a group such that it is always aligned. */
static inline match_mask
find_first_deleted_group(struct ccc_fhmap const *const h, size_t *const start)
{
    assert((*start & ~((size_t)(CCC_FHM_GROUP_SIZE - 1))) == *start);
    while (*start < (h->mask + 1))
    {
        match_mask const deleted = match_deleted(group_loada(&h->tag[*start]));
        if (deleted.v)
        {
            return deleted;
        }
        *start += CCC_FHM_GROUP_SIZE;
    }
    return (match_mask){};
}

/** Accepts the map, elements to add, and an allocation function if resizing
may be needed. While containers normally remember their own allocation
permissions, this function may be called in a variety of scenarios; one of which
is when the user wants to reserve the necessary space dynamically at runtime
but only once and for a container that is not given permission to resize
arbitrarily. */
static ccc_result
maybe_rehash(struct ccc_fhmap *const h, size_t const to_add,
             ccc_any_alloc_fn *const fn)
{
    if (unlikely(!h->mask && !fn))
    {
        return CCC_RESULT_NO_ALLOC;
    }
    size_t required_total_cap = to_power_of_two(((h->count + to_add) * 8) / 7);
    if (!required_total_cap)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    ccc_result const init = check_init(h, required_total_cap, fn);
    if (init != CCC_RESULT_OK)
    {
        return init;
    }
    if (likely(h->remain))
    {
        return CCC_RESULT_OK;
    }
    size_t const current_total_cap = h->mask + 1;
    if (fn && (h->count + to_add) > current_total_cap / 2)
    {
        return rehash_resize(h, to_add, fn);
    }
    if (h->count == mask_to_load_factor_cap(h->mask))
    {
        return CCC_RESULT_NO_ALLOC;
    }
    rehash_in_place(h);
    return CCC_RESULT_OK;
}

/** Rehashes the map in place. Elements may or may not move, depending on
results. Assumes the table has been allocated and had no more remaining slots
for insertion. Rehashing in place repeatedly can be expensive so the user
should ensure to select an appropriate capacity for fixed size tables. */
static void
rehash_in_place(struct ccc_fhmap *const h)
{
    assert((h->mask + 1) % CCC_FHM_GROUP_SIZE == 0);
    assert(h->tag && h->data);
    size_t const mask = h->mask;
    for (size_t i = 0; i < mask + 1; i += CCC_FHM_GROUP_SIZE)
    {
        group_storea(&h->tag[i], group_constant_to_empty_full_to_deleted(
                                     group_loada(&h->tag[i])));
    }
    (void)memcpy(h->tag + (mask + 1), h->tag, CCC_FHM_GROUP_SIZE);
    size_t group_start = 0;
    match_mask deleted = {};
    /* Because the load factor is roughly 87% we could have large spans of
       unoccupied slots in large tables due to full slots we have converted to
       deleted tags. There could also be many tombstones that were just
       converted to empty slots in the prep loop earlier. We can speed things up
       by performing aligned group scans checking for any groups with elements
       that need to be rehashed. */
    while ((deleted = find_first_deleted_group(h, &group_start)).v)
    {
        size_t tag_i = 0;
        while ((tag_i = match_next_one(&deleted)) != CCC_FHM_GROUP_SIZE)
        {
            tag_i += group_start;
            /* The inner loop swap case may have made a previously deleted entry
               in this group filled with the swapped element's hash. The mask
               cannot be updated to notice this and the swapped element was
               taken care of by retrying to find a slot in the innermost loop.
               Therefore skip this slot. It no longer needs processing. */
            if (h->tag[tag_i].v != TAG_DELETED)
            {
                continue;
            }
            do
            {
                uint64_t const hash = hash_fn(h, key_at(h, tag_i));
                size_t const new_i = find_slot_or_noreturn(h, hash);
                ccc_fhm_tag const hash_tag = tag_from(hash);
                /* We analyze groups not slots. Do not move the element to
                   another slot in the same unaligned group load. The tag is in
                   the proper group for an unaligned load based on where the
                   hashed value will start its loads and the match and does not
                   need relocation. */
                if (likely(is_same_group(tag_i, new_i, hash, mask)))
                {
                    tag_set(h, hash_tag, tag_i);
                    break; /* continues outer loop */
                }
                ccc_fhm_tag const occupant = h->tag[new_i];
                tag_set(h, hash_tag, new_i);
                if (occupant.v == TAG_EMPTY)
                {
                    tag_set(h, (ccc_fhm_tag){TAG_EMPTY}, tag_i);
                    (void)memcpy(data_at(h, new_i), data_at(h, tag_i),
                                 h->sizeof_type);
                    break; /* continues outer loop */
                }
                /* The other slots data has been swapped and we rehash every
                   element for this algorithm so there is no need to write its
                   tag to this slot. It's data is in the correct location and
                   we now will loop to try to find it a rehashed slot. */
                assert(occupant.v == TAG_DELETED);
                swap(swap_slot(h), data_at(h, tag_i), data_at(h, new_i),
                     h->sizeof_type);
            }
            while (1);
        }
        group_start += CCC_FHM_GROUP_SIZE;
    }
    h->remain = mask_to_load_factor_cap(mask) - h->count;
}

/** Returns true if the position being rehashed would be moved to a new slot
in the same group it is already in. This means when this data is hashed to its
ideal index in the table, both i and new_slot are already in that group that
would be loaded for simultaneous scanning. */
static inline ccc_tribool
is_same_group(size_t const i, size_t const new_i, uint64_t const hash,
              size_t const mask)
{
    return (((i - (hash & mask)) & mask) / CCC_FHM_GROUP_SIZE)
        == (((new_i - (hash & mask)) & mask) / CCC_FHM_GROUP_SIZE);
}

static ccc_result
rehash_resize(struct ccc_fhmap *const h, size_t const to_add,
              ccc_any_alloc_fn *const fn)
{
    assert(((h->mask + 1) & h->mask) == 0);
    size_t const new_pow2_cap = next_power_of_two((h->mask + 1 + to_add) << 1);
    if (new_pow2_cap < (h->mask + 1))
    {
        return CCC_RESULT_MEM_ERROR;
    }
    size_t const prev_bytes = mask_to_total_bytes(h->sizeof_type, h->mask);
    size_t const total_bytes
        = mask_to_total_bytes(h->sizeof_type, new_pow2_cap - 1);
    if (total_bytes < prev_bytes)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    void *const new_buf = fn(NULL, total_bytes, h->aux);
    if (!new_buf)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    struct ccc_fhmap new_h = *h;
    new_h.count = 0;
    new_h.mask = new_pow2_cap - 1;
    new_h.remain = mask_to_load_factor_cap(new_h.mask);
    new_h.data = new_buf;
    /* Our static assertions at start of file guarantee this is correct. */
    new_h.tag = tag_pos(new_h.sizeof_type, new_buf, new_h.mask);
    (void)memset(new_h.tag, TAG_EMPTY, mask_to_tag_bytes(new_h.mask));
    size_t group_start = 0;
    match_mask full = {};
    while ((full = find_first_full_group(h, &group_start)).v)
    {
        size_t tag_i = 0;
        while ((tag_i = match_next_one(&full)) != CCC_FHM_GROUP_SIZE)
        {
            tag_i += group_start;
            uint64_t const hash = hash_fn(h, key_at(h, tag_i));
            size_t const new_i = find_slot_or_noreturn(&new_h, hash);
            tag_set(&new_h, tag_from(hash), new_i);
            (void)memcpy(data_at(&new_h, new_i), data_at(h, tag_i),
                         new_h.sizeof_type);
        }
        group_start += CCC_FHM_GROUP_SIZE;
    }
    new_h.remain -= h->count;
    new_h.count = h->count;
    (void)fn(h->data, 0, h->aux);
    *h = new_h;
    return CCC_RESULT_OK;
}

/** Ensures the map is initialized due to our allowance of lazy initialization
to support various sources of memory at compile and runtime. */
static inline ccc_result
check_init(struct ccc_fhmap *const h, size_t required_total_cap,
           ccc_any_alloc_fn *const fn)
{
    if (likely(!is_uninitialized(h)))
    {
        return CCC_RESULT_OK;
    }
    if (h->mask)
    {
        /* A fixed size map that is not initialized. */
        if (!h->data || h->mask + 1 < required_total_cap)
        {
            return CCC_RESULT_MEM_ERROR;
        }
        if (h->mask + 1 < CCC_FHM_GROUP_SIZE || !is_power_of_two(h->mask + 1))
        {
            return CCC_RESULT_ARG_ERROR;
        }
        h->tag = tag_pos(h->sizeof_type, h->data, h->mask);
        (void)memset(h->tag, TAG_EMPTY, mask_to_tag_bytes(h->mask));
    }
    else
    {
        /* A dynamic map we can re-size as needed. */
        required_total_cap = max(required_total_cap, CCC_FHM_GROUP_SIZE);
        size_t const total_bytes
            = mask_to_total_bytes(h->sizeof_type, required_total_cap - 1);
        h->data = fn(NULL, total_bytes, h->aux);
        if (!h->data)
        {
            return CCC_RESULT_MEM_ERROR;
        }
        h->mask = required_total_cap - 1;
        h->remain = mask_to_load_factor_cap(h->mask);
        h->tag = tag_pos(h->sizeof_type, h->data, h->mask);
        (void)memset(h->tag, TAG_EMPTY, mask_to_tag_bytes(h->mask));
    }
    return CCC_RESULT_OK;
}

static inline void
destory_each(struct ccc_fhmap *const h, ccc_any_type_destructor_fn *const fn)
{
    for (void *i = ccc_fhm_begin(h); i != ccc_fhm_end(h);
         i = ccc_fhm_next(h, i))
    {
        fn((ccc_any_type){
            .any_type = i,
            .aux = h->aux,
        });
    }
}

static inline uint64_t
hash_fn(struct ccc_fhmap const *const h, void const *const any_key)
{
    return h->hash_fn((ccc_any_key){
        .any_key = any_key,
        .aux = h->aux,
    });
}

static inline ccc_tribool
eq_fn(struct ccc_fhmap const *const h, void const *const key, size_t const i)
{
    return h->eq_fn((ccc_any_key_cmp){
        .any_key_lhs = key,
        .any_type_rhs = data_at(h, i),
        .aux = h->aux,
    });
}

static inline void *
key_at(struct ccc_fhmap const *const h, size_t const i)
{
    return (char *)data_at(h, i) + h->key_offset;
}

static inline void *
data_at(struct ccc_fhmap const *const h, size_t const i)
{
    assert(i <= h->mask);
    return (char *)h->data + (i * h->sizeof_type);
}

static inline ccc_ucount
data_i(struct ccc_fhmap const *const h, void const *const data_slot)
{
    if (unlikely((char *)data_slot
                     >= (char *)h->data + (h->sizeof_type * (h->mask + 1))
                 || (char *)data_slot < (char *)h->data))
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){
        .count = ((char *)data_slot - (char *)h->data) / h->sizeof_type,
    };
}

static inline void *
swap_slot(struct ccc_fhmap const *h)
{
    return (char *)h->data + (h->sizeof_type * (h->mask + 1));
}

static inline void
swap(void *const tmp, void *const a, void *const b, size_t const ab_size)
{
    if (unlikely(!a || !b || a == b))
    {
        return;
    }
    (void)memcpy(tmp, a, ab_size);
    (void)memcpy(a, b, ab_size);
    (void)memcpy(b, tmp, ab_size);
}

static inline void *
key_in_slot(struct ccc_fhmap const *const h, void const *const slot)
{
    return (char *)slot + h->key_offset;
}

/** Return n if a power of 2, otherwise returns next greater power of 2. 0 is
returned if overflow will occur. */
static inline size_t
to_power_of_two(size_t const n)
{
    if (is_power_of_two(n))
    {
        return n;
    }
    return next_power_of_two(n);
}

/** Returns next power of 2 greater than n or 0 if no greater can be found. */
static inline size_t
next_power_of_two(size_t const n)
{
    if (n <= 1)
    {
        return n + 1;
    }
    unsigned const shifts = clz_size_t(n - 1);
    return shifts >= sizeof(size_t) * CHAR_BIT ? 0 : (SIZE_MAX >> shifts) + 1;
}

/** Returns true if n is a power of two. 0 is not considered a power of 2. */
static inline ccc_tribool
is_power_of_two(size_t const n)
{
    return n && ((n & (n - 1)) == 0);
}

/** Returns the total bytes used by the map in the contiguous allocation. This
includes the bytes for the user data array (swap slot included) and the tag
array. The tag array also has an duplicate group at the end that must be
counted.

This calculation includes any unusable padding bytes added to the end of the
user data array. Padding may be required if the alignment of the user type is
less than that of a group size. This will allow aligned group loads.

This number of bytes should be consistently correct whether the map we are
dealing with is fixed size or dynamic. A fixed size map could technically have
more bytes as padding after the tag array but we never need or access those
bytes so we are only interested in contiguous bytes from start of user data to
last byte of tag array. */
static inline size_t
mask_to_total_bytes(size_t const sizeof_type, size_t const mask)
{
    if (unlikely(!mask))
    {
        return 0;
    }
    return mask_to_data_bytes(sizeof_type, mask) + mask_to_tag_bytes(mask);
}

/** Returns the bytes needed for the tag metadata array. This includes the
bytes for the duplicate group that is at the end of the tag array.

Assumes the mask is non-zero. */
static inline size_t
mask_to_tag_bytes(size_t const mask)
{
    static_assert(sizeof(ccc_fhm_tag) == sizeof(uint8_t));
    return mask + 1 + CCC_FHM_GROUP_SIZE;
}

/** Returns the capacity count that is available with a current load factor of
87.5% percent. The returned count is the maximum allowable capacity that can
store user tags and data before the load factor is reached. The total capacity
of the table is (mask + 1) which is not the capacity that this function
calculates. For example, if (mask + 1 = 64), then this function returns 56.

Assumes the mask is non-zero. */
static inline size_t
mask_to_load_factor_cap(size_t const mask)
{
    return ((mask + 1) / 8) * 7;
}

/** Returns the number of bytes taken by the user data array. This includes the
extra swap slot provided at the start of the array. This swap slot is never
accounted for in load factor or capacity calculations but must be remembered in
cases like this for resizing and allocation purposes.

Any unusable extra alignment padding bytes added to the end of the user data
array are also accounted for here so that the tag array position starts after
the correct number of aligned user data bytes. This allows aligned group loads.

Assumes the mask is non-zero. */
static inline size_t
mask_to_data_bytes(size_t const sizeof_type, size_t const mask)
{
    /* Add two because there is always a bonus user data type at the 0th index
       of the data array for swapping purposes. */
    return roundup(sizeof_type * (mask + 2));
}

/** Returns the correct position of the start of the tag array given the base
of the data array. This position is determined by the size of the type in the
data array and the current mask being used for the hash map to which the data
belongs. */
static inline ccc_fhm_tag *
tag_pos(size_t const sizeof_type, void const *const data, size_t const mask)
{
    /* Static assertions at top of file ensure this is correct. */
    return (ccc_fhm_tag *)((char *)data
                           + mask_to_data_bytes(sizeof_type, mask));
}

static inline size_t
max(size_t const a, size_t const b)
{
    return a > b ? a : b;
}

static inline ccc_tribool
is_uninitialized(struct ccc_fhmap const *const h)
{
    return !h->data || !h->tag;
}

/** Rounds up the provided bytes to a valid alignment for group size. */
static inline size_t
roundup(size_t const bytes)
{
    return (bytes + CCC_FHM_GROUP_SIZE - 1) & ~(CCC_FHM_GROUP_SIZE - 1);
}

/*=====================   Intrinsics and Generics   =========================*/

/** Below are the implementations of the SIMD or bitwise operations needed to
run a search on multiple entries in the hash table simultaneously. For now,
the only container that will use these operations is this one so there is no
need to break out different headers and sources and clutter the src directory.
x86 is the only platform that gets the full benefit of SIMD. Apple and all
other platforms will get a portable implementation due to concerns over NEON
speed of vectorized instructions. However, loading up groups into a uint64_t is
still good and counts as simultaneous operations just not the type that uses CPU
vector lanes for a single instruction. */

/*========================   Tag Implementations    =========================*/

/** Sets the specified tag at the index provided. Ensures that the replica
group at the end of the tag array remains in sync with current tag if needed. */
static inline void
tag_set(struct ccc_fhmap *const h, ccc_fhm_tag const m, size_t const i)
{
    size_t const replica_byte
        = ((i - CCC_FHM_GROUP_SIZE) & h->mask) + CCC_FHM_GROUP_SIZE;
    h->tag[i] = m;
    h->tag[replica_byte] = m;
}

/** Returns CCC_TRUE if the tag holds user hash bits, meaning it is occupied. */
static inline ccc_tribool
tag_full(ccc_fhm_tag const m)
{
    return (m.v & TAG_MSB) == 0;
}

/** Returns CCC_TRUE if the tag is one of the two special constants EMPTY or
DELETED. */
static inline ccc_tribool
tag_constant(ccc_fhm_tag const m)
{
    return (m.v & TAG_MSB) != 0;
}

/** Converts a full hash code to a tag fingerprint. The tag consists of the top
7 bits of the hash code. Therefore, hash functions with good entropy in the
upper bits are desirable. */
static inline ccc_fhm_tag
tag_from(uint64_t const hash)
{
    return (ccc_fhm_tag){
        (hash >> ((sizeof(hash) * CHAR_BIT) - 7)) & TAG_LOWER_7_MASK,
    };
}

/*========================  Index Mask Implementations   ====================*/

/** Returns true if any index is on in the mask otherwise false. */
static inline ccc_tribool
match_has_one(match_mask const m)
{
    return m.v != 0;
}

/** Return the index of the first trailing one in the given match in the
range `[0, CCC_FHM_GROUP_SIZE]` to indicate a positive result of a group query
operation. This index represents the group member with a tag that has matched.
Because 0 is a valid index the user must check the index against
`CCC_FHM_GROUP_SIZE`, which means no trailing one is found. */
static inline size_t
match_trailing_one(match_mask const m)
{
    return ctz(m);
}

/** A function to aid in iterating over on bits/indices in a match. The
function returns the 0-based index of the current on index and then adjusts the
mask appropriately for future iteration by removing the lowest on index bit. If
no bits are found the width of the mask is returned. */
static inline size_t
match_next_one(match_mask *const m)
{
    assert(m);
    size_t const index = match_trailing_one(*m);
    m->v &= (m->v - 1);
    return index;
}

/** Counts the leading zeros in a match. Leading zeros are those starting
at the most significant bit. */
static inline size_t
match_leading_zeros(match_mask const m)
{
    return clz(m);
}

/** Counts the trailing zeros in a match. Trailing zeros are those
starting at the least significant bit. */
static inline size_t
match_trailing_zeros(match_mask const m)
{
    return ctz(m);
}

/** We have abstracted at much as we can before this point. Now implementations
will need to vary based on availability of vectorized instructions. */
#if defined(CCC_HAS_X86_SIMD)

/*=========================   Match SIMD Matching    ========================*/

/** Returns a match with a bit on if the tag at that index in group g
matches the provided tag m. If no indices matched this will be a 0 match.

Here is the process to help understand the dense intrinsics.

1. Load the tag into a 128 bit vector (_mm_set1_epi8). For example m = 0x73:

0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73

2. g holds 16 tags from tag array. Find matches (_mm_cmpeq_epi8).

0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73|0x73
0x79|0x33|0x21|0x73|0x45|0x55|0x12|0x54|0x11|0x44|0x73|0xFF|0xFF|0xFF|0xFF|0xFF
                │                                  │
0x00|0x00|0x00|0xFF|0x00|0x00|0x00|0x00|0x00|0x00|0xFF|0x00|0x00|0x00|0x00|0x00

3. Compress most significant bit of each byte to a uint16_t (_mm_movemask_epi8)

0x00|0x00|0x00|0xFF|0x00|0x00|0x00|0x00|0x00|0x00|0xFF|0x00|0x00|0x00|0x00|0x00
     ┌──────────┘                                  │
     │      ┌──────────────────────────────────────┘
0x0001000000100000

4. Return the result as a match.

(match_mask){0x0001000000100000}

With a good hash function it is very likely that the first match will be the
hashed data and the full comparison will evaluate to true. Note that this
method inevitably forces a call to the comparison callback function on every
match so an efficient comparison is beneficial. */
static inline match_mask
match_tag(group const g, ccc_fhm_tag const m)
{
    return (match_mask){
        _mm_movemask_epi8(_mm_cmpeq_epi8(g.v, _mm_set1_epi8((int8_t)m.v))),
    };
}

/** Returns 0 based match with every bit on representing those tags in
group g that are the empty special constant. The user must interpret this 0
based index in the context of the probe sequence. */
static inline match_mask
match_empty(group const g)
{
    return match_tag(g, (ccc_fhm_tag){TAG_EMPTY});
}

/** Returns 0 based match with every bit on representing those tags in
group g that are the deleted special constant. The user must interpret this 0
based index in the context of the probe sequence. */
static inline match_mask
match_deleted(group const g)
{
    return match_tag(g, (ccc_fhm_tag){TAG_DELETED});
}

/** Returns a 0 based match with every bit on representing those tags
in the group that are the special constant empty or deleted. These are easy
to find because they are the one tags in a group with the most significant
bit on. */
static inline match_mask
match_empty_deleted(group const g)
{
    static_assert(sizeof(int) >= sizeof(uint16_t));
    return (match_mask){_mm_movemask_epi8(g.v)};
}

/** Returns a 0 based match with every bit on representing those tags in the
group that are occupied by a user hash value. These are those tags that have
the most significant bit off and the lower 7 bits occupied by user hash. */
static inline match_mask
match_full(group const g)
{
    return (match_mask){~match_empty_deleted(g).v};
}

/** Matches all full tag slots into a mask excluding the starting position and
only considering the leading full slots from this position. Assumes start bit
is 0 indexed such that only the exclusive range of leading bits is considered
(start_tag, CCC_FHM_GROUP_SIZE). All trailing bits in the inclusive range from
[0, start_tag] are zeroed out in the mask.

Assumes start tag is less than group size. */
static inline match_mask
match_leading_full(group const g, size_t const start_tag)
{
    assert(start_tag < CCC_FHM_GROUP_SIZE);
    return (match_mask){(~match_empty_deleted(g).v)
                        & (MATCH_MASK_0TH_TAG_OFF << start_tag)};
}

/*=========================  Group Implementations   ========================*/

/** Loads a group starting at src into a 128 bit vector. This is a aligned
load and the user must ensure the load will not go off then end of the tag
array. */
static inline group
group_loada(ccc_fhm_tag const *const src)
{
    return (group){_mm_load_si128((__m128i *)src)};
}

/** Stores the src group to dst. The store is aligned and the user must ensure
the store will not go off the end of the tag array. */
static inline void
group_storea(ccc_fhm_tag *const dst, group const src)
{
    _mm_store_si128((__m128i *)dst, src.v);
}

/** Loads a group starting at src into a 128 bit vector. This is an unaligned
load and the user must ensure the load will not go off then end of the tag
array. */
static inline group
group_loadu(ccc_fhm_tag const *const src)
{
    return (group){_mm_loadu_si128((__m128i *)src)};
}

/** Converts the empty and deleted constants all TAG_EMPTY and the full tags
representing hashed user data TAG_DELETED. This will result in the hashed
fingerprint lower 7 bits of the user data being lost, so a rehash will be
required for the data corresponding to this slot.

For example, both of the special constant tags will be converted as follows.

TAG_EMPTY   = 0b1111_1111 -> 0b1111_1111
TAG_DELETED = 0b1000_0000 -> 0b1111_1111

The full tags with hashed user data will be converted as follows.

TAG_FULL = 0b0101_1101 -> 0b1000_000

The hashed bits are lost because the full slot has the high bit off and
therefore is not a match for the constants mask. */
static inline group
group_constant_to_empty_full_to_deleted(group const g)
{
    __m128i const zero = _mm_setzero_si128();
    __m128i const match_mask_constants = _mm_cmpgt_epi8(zero, g.v);
    return (group){
        _mm_or_si128(match_mask_constants, _mm_set1_epi8((int8_t)TAG_DELETED)),
    };
}

#elif defined(CCC_HAS_ARM_SIMD)

/** Below is the experimental NEON implementation for ARM architectures. This
implementation assumes a little endian architecture as that is the norm in
99.9% of ARM devices. However, monitor trends just in case. This implementation
is very similar to the portable one. This is largely due to the lack of an
equivalent operation to the x86_64 _mm_movemask_epi8, the operation responsible
for compressing a 128 bit vector into a uint16_t. NEON therefore opts for a
family of 64 bit operations targeted at u8 bytes. If NEON develops an efficient
instruction for compressing a 128 bit result into an int--or in our case a
uint16_t--we should revisit this section for 128 bit targeted intrinsics. */

/*=========================   Match SIMD Matching    ========================*/

/** Returns a match with the most significant bit set for each byte to
indicate if the byte in the group matched the mask to be searched. The only
bit on shall be this most significant bit to ensure iterating through index
masks is easier and counting bits make sense in the find loops. */
static inline match_mask
match_tag(group const g, ccc_fhm_tag const m)
{
    match_mask const res = {
        vget_lane_u64(vreinterpret_u64_u8(vceq_u8(g.v, vdup_n_u8(m.v))), 0)
            & MATCH_MASK_TAGS_MSBS,
    };
    assert(
        (res.v & MATCH_MASK_TAGS_OFF_BITS) == 0
        && "For bit counting and iteration purposes the most significant bit "
           "in every byte will indicate a match for a tag has occurred.");
    return res;
}

/** Returns 0 based match_mask with every bit on representing those tags in
group g that are the empty special constant. The user must interpret this 0
based index in the context of the probe sequence. */
static inline match_mask
match_empty(group const g)
{
    return match_tag(g, (ccc_fhm_tag){TAG_EMPTY});
}

/** Returns 0 based match_mask with every bit on representing those tags in
group g that are the empty special constant. The user must interpret this 0
based index in the context of the probe sequence. */
static inline match_mask
match_deleted(group const g)
{
    return match_tag(g, (ccc_fhm_tag){TAG_DELETED});
}

/** Returns a 0 based match with every bit on representing those tags
in the group that are the special constant empty or deleted. These are easy
to find because they are the one tags in a group with the most significant
bit on. */
static inline match_mask
match_empty_deleted(group const g)
{
    uint8x8_t const cmp = vcltz_s8(vreinterpret_s8_u8(g.v));
    match_mask const res = {
        vget_lane_u64(vreinterpret_u64_u8(cmp), 0) & MATCH_MASK_TAGS_MSBS,
    };
    assert(
        (res.v & MATCH_MASK_TAGS_OFF_BITS) == 0
        && "For bit counting and iteration purposes the most significant bit "
           "in every byte will indicate a match for a tag has occurred.");
    return res;
}

/** Returns a 0 based match with every bit on representing those tags in the
group that are occupied by a user hash value. These are those tags that have
the most significant bit off and the lower 7 bits occupied by user hash. */
static inline match_mask
match_full(group const g)
{
    uint8x8_t const cmp = vcgez_s8(vreinterpret_s8_u8(g.v));
    match_mask const res = {
        vget_lane_u64(vreinterpret_u64_u8(cmp), 0) & MATCH_MASK_TAGS_MSBS,
    };
    assert(
        (res.v & MATCH_MASK_TAGS_OFF_BITS) == 0
        && "For bit counting and iteration purposes the most significant bit "
           "in every byte will indicate a match for a tag has occurred.");
    return res;
}

/** Returns a 0 based match with every bit on representing those tags in the
group that are occupied by a user hash value leading from the provided start
bit. These are those tags that have the most significant bit off and the lower 7
bits occupied by user hash. All bits in the tags from [0, start_tag] are zeroed
out such that only the tags in the range (start_tag, CCC_FHM_GROUP_SIZE) are
considered.

Assumes start tag is less than group size. */
static inline match_mask
match_leading_full(group const g, size_t const start_tag)
{
    assert(start_tag < CCC_FHM_GROUP_SIZE);
    uint8x8_t const cmp = vcgez_s8(vreinterpret_s8_u8(g.v));
    match_mask const res = {
        vget_lane_u64(vreinterpret_u64_u8(cmp), 0)
            & (MATCH_MASK_0TH_TAG_OFF << (start_tag * TAG_BITS)),
    };
    assert(
        (res.v & MATCH_MASK_TAGS_OFF_BITS) == 0
        && "For bit counting and iteration purposes the most significant bit "
           "in every byte will indicate a match for a tag has occurred.");
    return res;
}

/*=========================  Group Implementations   ========================*/

/** Loads a group starting at src into a 8x8 (64) bit vector. This is an
aligned load and the user must ensure the load will not go off then end of the
tag array. */
static inline group
group_loada(ccc_fhm_tag const *const src)
{
    return (group){vld1_u8(&src->v)};
}

/** Stores the src group to dst. The store is aligned and the user must ensure
the store will not go off the end of the tag array. */
static inline void
group_storea(ccc_fhm_tag *const dst, group const src)
{
    vst1_u8(&dst->v, src.v);
}

/** Loads a group starting at src into a 8x8 (64) bit vector. This is an
unaligned load and the user must ensure the load will not go off then end of the
tag array. */
static inline group
group_loadu(ccc_fhm_tag const *const src)
{
    return (group){vld1_u8(&src->v)};
}

/** Converts the empty and deleted constants all TAG_EMPTY and the full tags
representing hashed user data TAG_DELETED. This will result in the hashed
fingerprint lower 7 bits of the user data being lost, so a rehash will be
required for the data corresponding to this slot.

For example, both of the special constant tags will be converted as follows.

TAG_EMPTY   = 0b1111_1111 -> 0b1111_1111
TAG_DELETED = 0b1000_0000 -> 0b1111_1111

The full tags with hashed user data will be converted as follows.

TAG_FULL = 0b0101_1101 -> 0b1000_000

The hashed bits are lost because the full slot has the high bit off and
therefore is not a match for the constants mask. */
static inline group
group_constant_to_empty_full_to_deleted(group const g)
{
    uint8x8_t const constant = vcltz_s8(vreinterpret_s8_u8(g.v));
    return (group){vorr_u8(constant, vdup_n_u8(TAG_MSB))};
}

#else /* FALLBACK PORTABLE IMPLEMENTATION */

/* What follows is the generic portable implementation when high width SIMD
can't be achieved. This ideally works for most platforms. */

/*=========================  Endian Helpers    ==============================*/

/* Returns 1=true if platform is little endian, else false for big endian. */
static inline int
is_little_endian(void)
{
    unsigned int x = 1;
    char *c = (char *)&x;
    return (int)*c;
}

/* Returns a mask converted to little endian byte layout. On a little endian
platform the value is returned, otherwise byte swapping occurs. */
static inline match_mask
to_little_endian(match_mask m)
{
    if (is_little_endian())
    {
        return m;
    }
#    if defined(__has_builtin) && __has_builtin(__builtin_bswap64)
    m.v = __builtin_bswap64(m.v);
#    else
    m.v = (m.v & 0x00000000FFFFFFFF) << 32 | (m.v & 0xFFFFFFFF00000000) >> 32;
    m.v = (m.v & 0x0000FFFF0000FFFF) << 16 | (m.v & 0xFFFF0000FFFF0000) >> 16;
    m.v = (m.v & 0x00FF00FF00FF00FF) << 8 | (m.v & 0xFF00FF00FF00FF00) >> 8;
#    endif
    return m;
}

/*=========================   Match SRMD Matching    ========================*/

/** Returns a match_mask indicating all tags in the group which may have the
given value. The match_mask will only have the most significant bit on within
the byte representing the tag for the match_mask. This function may return a
false positive in certain cases where the tag in the group differs from the
searched value only in its lowest bit. This is fine because:
- This never happens for `EMPTY` and `DELETED`, only full entries.
- The check for key equality will catch these.
- This only happens if there is at least 1 true match.
- The chance of this happening is very low (< 1% chance per byte).
This algorithm is derived from:
https://graphics.stanford.edu/~seander/bithacks.html##ValueInWord */
static inline match_mask
match_tag(group g, ccc_fhm_tag const m)
{
    group const cmp = {
        g.v
            ^ ((((typeof(g.v))m.v) << (TAG_BITS * 7UL))
               | (((typeof(g.v))m.v) << (TAG_BITS * 6UL))
               | (((typeof(g.v))m.v) << (TAG_BITS * 5UL))
               | (((typeof(g.v))m.v) << (TAG_BITS * 4UL))
               | (((typeof(g.v))m.v) << (TAG_BITS * 3UL))
               | (((typeof(g.v))m.v) << (TAG_BITS * 2UL))
               | (((typeof(g.v))m.v) << TAG_BITS) | (m.v)),
    };
    match_mask const res = to_little_endian((match_mask){
        (cmp.v - MATCH_MASK_TAGS_LSBS) & ~cmp.v & MATCH_MASK_TAGS_MSBS,
    });
    assert(
        (res.v & MATCH_MASK_TAGS_OFF_BITS) == 0
        && "For bit counting and iteration purposes the most significant bit "
           "in every byte will indicate a match for a tag has occurred.");
    return res;
}

/** Returns a match_mask with the most significant bit in every byte on if
that tag in g is empty. */
static inline match_mask
match_empty(group const g)
{
    /* EMPTY has all bits on and DELETED has the most significant bit on so
       EMPTY must have the top 2 bits on. Because the empty mask has only
       the most significant bit on this also ensure the mask has only the
       MSB on to indicate a match. */
    match_mask const res = to_little_endian((match_mask){
        g.v & (g.v << 1) & MATCH_MASK_TAGS_EMPTY,
    });
    assert(
        (res.v & MATCH_MASK_TAGS_OFF_BITS) == 0
        && "For bit counting and iteration purposes the most significant bit "
           "in every byte will indicate a match for a tag has occurred.");
    return res;
}

/** Returns a match_mask with the most significant bit in every byte on if
that tag in g is empty. */
static inline match_mask
match_deleted(group const g)
{
    /* This is the same process as matching a tag but easier because we can
       make the empty mask a constant at compile time instead of runtime. */
    group const empty_cmp = {g.v ^ MATCH_MASK_TAGS_EMPTY};
    match_mask const res = to_little_endian((match_mask){
        (empty_cmp.v - MATCH_MASK_TAGS_LSBS) & ~empty_cmp.v
            & MATCH_MASK_TAGS_MSBS,
    });
    assert(
        (res.v & MATCH_MASK_TAGS_OFF_BITS) == 0
        && "For bit counting and iteration purposes the most significant bit "
           "in every byte will indicate a match for a tag has occurred.");
    return res;
}

/** Returns a match with the most significant bit in every byte on if
that tag in g is empty or deleted. This is found by the most significant bit. */
static inline match_mask
match_empty_deleted(group const g)
{
    match_mask const res
        = to_little_endian((match_mask){g.v & MATCH_MASK_TAGS_MSBS});
    assert(
        (res.v & MATCH_MASK_TAGS_OFF_BITS) == 0
        && "For bit counting and iteration purposes the most significant bit "
           "in every byte will indicate a match for a tag has occurred.");
    return res;
}

/** Returns a 0 based match with every bit on representing those tags in the
group that are occupied by a user hash value. These are those tags that have
the most significant bit off and the lower 7 bits occupied by user hash. */
static inline match_mask
match_full(group const g)
{
    match_mask const res
        = to_little_endian((match_mask){(~g.v) & MATCH_MASK_TAGS_MSBS});
    assert(
        (res.v & MATCH_MASK_TAGS_OFF_BITS) == 0
        && "For bit counting and iteration purposes the most significant bit "
           "in every byte will indicate a match for a tag has occurred.");
    return res;
}

/** Returns a 0 based match with every bit on representing those tags in the
group that are occupied by a user hash value leading from the provided start
bit. These are those tags that have the most significant bit off and the lower 7
bits occupied by user hash. All bits in the tags from [0, start_tag] are zeroed
out such that only the tags in the range (start_tag, CCC_FHM_GROUP_SIZE) are
considered.

Assumes start_tag is less than group size. */
static inline match_mask
match_leading_full(group const g, size_t const start_tag)
{
    assert(start_tag < CCC_FHM_GROUP_SIZE);
    /* The 0th tag off mask we use also happens to ensure only the MSB in each
       byte of a match is on as the assert confirms after. */
    match_mask const res = to_little_endian((match_mask){
        (~g.v) & (MATCH_MASK_0TH_TAG_OFF << (start_tag * TAG_BITS)),
    });
    assert(
        (res.v & MATCH_MASK_TAGS_OFF_BITS) == 0
        && "For bit counting and iteration purposes the most significant bit "
           "in every byte will indicate a match for a tag has occurred.");
    return res;
}

/*=========================  Group Implementations   ========================*/

/** Loads tags into a group without violating strict aliasing. */
static inline group
group_loada(ccc_fhm_tag const *const src)
{
    group g;
    (void)memcpy(&g, src, sizeof(g));
    return g;
}

/** Stores a group back into the tag array without violating strict aliasing. */
static inline void
group_storea(ccc_fhm_tag *const dst, group const src)
{
    (void)memcpy(dst, &src, sizeof(src));
}

/** Loads tags into a group without violating strict aliasing. */
static inline group
group_loadu(ccc_fhm_tag const *const src)
{
    group g;
    (void)memcpy(&g, src, sizeof(g));
    return g;
}

/** Converts the empty and deleted constants all TAG_EMPTY and the full tags
representing hashed user data TAG_DELETED. This will result in the hashed
fingerprint lower 7 bits of the user data being lost, so a rehash will be
required for the data corresponding to this slot.

For example, both of the special constant tags will be converted as follows.

TAG_EMPTY   = 0b1111_1111 -> 0b1111_1111
TAG_DELETED = 0b1000_0000 -> 0b1111_1111

The full tags with hashed user data will be converted as follows.

TAG_FULL = 0b0101_1101 -> 0b1000_000

The hashed bits are lost because the full slot has the high bit off and
therefore is not a match for the constants mask. */
static inline group
group_constant_to_empty_full_to_deleted(group g)
{
    g.v = ~g.v & MATCH_MASK_TAGS_MSBS;
    g.v = ~g.v + (g.v >> (TAG_BITS - 1));
    return g;
}

#endif /* defined(CCC_HAS_X86_SIMD) */

/*====================  Bit Counting for Index Mask   =======================*/

/** How we count bits can vary depending on the implementation, group size,
and match_mask width. Keep the bit counting logic separate here so the above
implementations can simply rely on counting zeros that yields correct results
for their implementation. Each implementation attempts to use the built-ins
first and then falls back to manual bit counting. */

#if defined(CCC_HAS_X86_SIMD)

#    if defined(__has_builtin) && __has_builtin(__builtin_ctz)                 \
        && __has_builtin(__builtin_clz) && __has_builtin(__builtin_clzl)

static_assert(sizeof((match_mask){}.v) <= sizeof(unsigned),
              "a match_mask is expected to be smaller than an unsigned due to "
              "available builtins on the given platform.");

static inline unsigned
ctz(match_mask const m)
{
    static_assert(__builtin_ctz(0x8000) == CCC_FHM_GROUP_SIZE - 1,
                  "Counting trailing zeros will always result in a valid mask "
                  "based on match_mask width if the mask is not 0, even though "
                  "m is implicitly widened to an int.");
    return m.v ? __builtin_ctz(m.v) : CCC_FHM_GROUP_SIZE;
}

static inline unsigned
clz(match_mask const m)
{
    static_assert(sizeof((match_mask){}.v) * 2UL == sizeof(unsigned),
                  "a match_mask will be implicitly widened to exactly twice "
                  "its width if non-zero due to builtin functions available.");
    return m.v ? __builtin_clz(((unsigned)m.v) << CCC_FHM_GROUP_SIZE)
               : CCC_FHM_GROUP_SIZE;
}

static inline unsigned
clz_size_t(size_t const n)
{
    static_assert(sizeof(size_t) == sizeof(unsigned long),
                  "Ensure the available builtin works for the platform defined "
                  "size of a size_t.");
    return n ? __builtin_clzl(n) : sizeof(size_t) * CHAR_BIT;
}

#    else /* !defined(__has_builtin) || !__has_builtin(__builtin_ctz)          \
        || !__has_builtin(__builtin_clz) || !__has_builtin(__builtin_clzl) */

enum : size_t
{
    /** @private Most significant bit of size_t for bit counting. */
    SIZE_T_MSB = 0x8000000000000000,
};

static inline unsigned
ctz(match_mask m)
{
    if (!m.v)
    {
        return CCC_FHM_GROUP_SIZE;
    }
    unsigned cnt = 0;
    for (; m.v; cnt += ((m.v & 1U) == 0), m.v >>= 1U)
    {}
    return cnt;
}

static inline unsigned
clz(match_mask m)
{
    if (!m.v)
    {
        return CCC_FHM_GROUP_SIZE;
    }
    unsigned mv = (unsigned)m.v << CCC_FHM_GROUP_SIZE;
    unsigned cnt = 0;
    for (; (mv & (MATCH_MASK_MSB << CCC_FHM_GROUP_SIZE)) == 0; ++cnt, mv <<= 1U)
    {}
    return cnt;
}

static inline unsigned
clz_size_t(size_t n)
{
    if (!n)
    {
        return sizeof(size_t) * CHAR_BIT;
    }
    unsigned cnt = 0;
    for (; !(n & SIZE_T_MSB); ++cnt, n <<= 1U)
    {}
    return cnt;
}

#    endif /* defined(__has_builtin) && __has_builtin(__builtin_ctz)           \
        && __has_builtin(__builtin_clz) && __has_builtin(__builtin_clzl) */

#else /* NEON and PORTABLE implementation count bits the same way. */

#    if defined(__has_builtin) && __has_builtin(__builtin_ctzl)                \
        && __has_builtin(__builtin_clzl)

static_assert(
    sizeof((match_mask){}.v) == sizeof(long),
    "builtin assumes an integer width that must be compatible with match_mask");

static inline unsigned
ctz(match_mask const m)
{
    static_assert(__builtin_ctzl(MATCH_MASK_MSB) / CCC_FHM_GROUP_SIZE
                      == CCC_FHM_GROUP_SIZE - 1,
                  "builtin trailing zeros must produce number of bits we "
                  "expect for mask");
    return m.v ? __builtin_ctzl(m.v) / CCC_FHM_GROUP_SIZE : CCC_FHM_GROUP_SIZE;
}

static inline unsigned
clz(match_mask const m)
{
    static_assert(__builtin_clzl((typeof((match_mask){}.v))0x1)
                          / CCC_FHM_GROUP_SIZE
                      == CCC_FHM_GROUP_SIZE - 1,
                  "builtin trailing zeros must produce number of bits we "
                  "expect for mask");
    return m.v ? __builtin_clzl(m.v) / CCC_FHM_GROUP_SIZE : CCC_FHM_GROUP_SIZE;
}

static inline unsigned
clz_size_t(size_t const n)
{
    static_assert(sizeof(size_t) == sizeof(unsigned long));
    return n ? __builtin_clzl(n) : sizeof(size_t) * CHAR_BIT;
}

#    else /* defined(__has_builtin) && __has_builtin(__builtin_ctzl) &&        \
             __has_builtin(__builtin_clzl) */

enum : size_t
{
    /** @private Most significant bit of size_t for bit counting. */
    SIZE_T_MSB = 0x8000000000000000,
};

static inline unsigned
ctz(match_mask m)
{
    if (!m.v)
    {
        return CCC_FHM_GROUP_SIZE;
    }
    unsigned cnt = 0;
    for (; m.v; cnt += ((m.v & 1U) == 0), m.v >>= 1U)
    {}
    return cnt / CCC_FHM_GROUP_SIZE;
}

static inline unsigned
clz(match_mask m)
{
    if (!m.v)
    {
        return CCC_FHM_GROUP_SIZE;
    }
    unsigned cnt = 0;
    for (; (m.v & MATCH_MASK_MSB) == 0; ++cnt, m.v <<= 1U)
    {}
    return cnt / CCC_FHM_GROUP_SIZE;
}

static inline unsigned
clz_size_t(size_t n)
{
    if (!n)
    {
        return sizeof(size_t) * CHAR_BIT;
    }
    unsigned cnt = 0;
    for (; (n & SIZE_T_MSB) == 0; ++cnt, n <<= 1U)
    {}
    return cnt;
}

#    endif /* !defined(__has_builtin) || !__has_builtin(__builtin_ctzl) ||     \
              !__has_builtin(__builtin_clzl) */

#endif /* defined(CCC_HAS_X86_SIMD) */

/** The following Apache license follows as required by the Rust Hashbrown
table which in turn is based on the Abseil Flat Hash Map developed at Google:

Abseil: https://github.com/abseil/abseil-cpp
Hashbrown: https://github.com/rust-lang/hashbrown

Because both Abseil and Hashbrown require inclusion of the following license,
it is included below. The implementation in this file is based strictly on the
Hashbrown version and has been modified to work with C and the C Container
Collection.

                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/

   TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

   1. Definitions.

      "License" shall mean the terms and conditions for use, reproduction,
      and distribution as defined by Sections 1 through 9 of this document.

      "Licensor" shall mean the copyright owner or entity authorized by
      the copyright owner that is granting the License.

      "Legal Entity" shall mean the union of the acting entity and all
      other entities that control, are controlled by, or are under common
      control with that entity. For the purposes of this definition,
      "control" means (i) the power, direct or indirect, to cause the
      direction or management of such entity, whether by contract or
      otherwise, or (ii) ownership of fifty percent (50%) or more of the
      outstanding shares, or (iii) beneficial ownership of such entity.

      "You" (or "Your") shall mean an individual or Legal Entity
      exercising permissions granted by this License.

      "Source" form shall mean the preferred form for making modifications,
      including but not limited to software source code, documentation
      source, and configuration files.

      "Object" form shall mean any form resulting from mechanical
      transformation or translation of a Source form, including but
      not limited to compiled object code, generated documentation,
      and conversions to other media types.

      "Work" shall mean the work of authorship, whether in Source or
      Object form, made available under the License, as indicated by a
      copyright notice that is included in or attached to the work
      (an example is provided in the Appendix below).

      "Derivative Works" shall mean any work, whether in Source or Object
      form, that is based on (or derived from) the Work and for which the
      editorial revisions, annotations, elaborations, or other modifications
      represent, as a whole, an original work of authorship. For the purposes
      of this License, Derivative Works shall not include works that remain
      separable from, or merely link (or bind by name) to the interfaces of,
      the Work and Derivative Works thereof.

      "Contribution" shall mean any work of authorship, including
      the original version of the Work and any modifications or additions
      to that Work or Derivative Works thereof, that is intentionally
      submitted to Licensor for inclusion in the Work by the copyright owner
      or by an individual or Legal Entity authorized to submit on behalf of
      the copyright owner. For the purposes of this definition, "submitted"
      means any form of electronic, verbal, or written communication sent
      to the Licensor or its representatives, including but not limited to
      communication on electronic mailing lists, source code control systems,
      and issue tracking systems that are managed by, or on behalf of, the
      Licensor for the purpose of discussing and improving the Work, but
      excluding communication that is conspicuously marked or otherwise
      designated in writing by the copyright owner as "Not a Contribution."

      "Contributor" shall mean Licensor and any individual or Legal Entity
      on behalf of whom a Contribution has been received by Licensor and
      subsequently incorporated within the Work.

   2. Grant of Copyright License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      copyright license to reproduce, prepare Derivative Works of,
      publicly display, publicly perform, sublicense, and distribute the
      Work and such Derivative Works in Source or Object form.

   3. Grant of Patent License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      (except as stated in this section) patent license to make, have made,
      use, offer to sell, sell, import, and otherwise transfer the Work,
      where such license applies only to those patent claims licensable
      by such Contributor that are necessarily infringed by their
      Contribution(s) alone or by combination of their Contribution(s)
      with the Work to which such Contribution(s) was submitted. If You
      institute patent litigation against any entity (including a
      cross-claim or counterclaim in a lawsuit) alleging that the Work
      or a Contribution incorporated within the Work constitutes direct
      or contributory patent infringement, then any patent licenses
      granted to You under this License for that Work shall terminate
      as of the date such litigation is filed.

   4. Redistribution. You may reproduce and distribute copies of the
      Work or Derivative Works thereof in any medium, with or without
      modifications, and in Source or Object form, provided that You
      meet the following conditions:

      (a) You must give any other recipients of the Work or
          Derivative Works a copy of this License; and

      (b) You must cause any modified files to carry prominent notices
          stating that You changed the files; and

      (c) You must retain, in the Source form of any Derivative Works
          that You distribute, all copyright, patent, trademark, and
          attribution notices from the Source form of the Work,
          excluding those notices that do not pertain to any part of
          the Derivative Works; and

      (d) If the Work includes a "NOTICE" text file as part of its
          distribution, then any Derivative Works that You distribute must
          include a readable copy of the attribution notices contained
          within such NOTICE file, excluding those notices that do not
          pertain to any part of the Derivative Works, in at least one
          of the following places: within a NOTICE text file distributed
          as part of the Derivative Works; within the Source form or
          documentation, if provided along with the Derivative Works; or,
          within a display generated by the Derivative Works, if and
          wherever such third-party notices normally appear. The contents
          of the NOTICE file are for informational purposes only and
          do not modify the License. You may add Your own attribution
          notices within Derivative Works that You distribute, alongside
          or as an addendum to the NOTICE text from the Work, provided
          that such additional attribution notices cannot be construed
          as modifying the License.

      You may add Your own copyright statement to Your modifications and
      may provide additional or different license terms and conditions
      for use, reproduction, or distribution of Your modifications, or
      for any such Derivative Works as a whole, provided Your use,
      reproduction, and distribution of the Work otherwise complies with
      the conditions stated in this License.

   5. Submission of Contributions. Unless You explicitly state otherwise,
      any Contribution intentionally submitted for inclusion in the Work
      by You to the Licensor shall be under the terms and conditions of
      this License, without any additional terms or conditions.
      Notwithstanding the above, nothing herein shall supersede or modify
      the terms of any separate license agreement you may have executed
      with Licensor regarding such Contributions.

   6. Trademarks. This License does not grant permission to use the trade
      names, trademarks, service marks, or product names of the Licensor,
      except as required for reasonable and customary use in describing the
      origin of the Work and reproducing the content of the NOTICE file.

   7. Disclaimer of Warranty. Unless required by applicable law or
      agreed to in writing, Licensor provides the Work (and each
      Contributor provides its Contributions) on an "AS IS" BASIS,
      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
      implied, including, without limitation, any warranties or conditions
      of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
      PARTICULAR PURPOSE. You are solely responsible for determining the
      appropriateness of using or redistributing the Work and assume any
      risks associated with Your exercise of permissions under this License.

   8. Limitation of Liability. In no event and under no legal theory,
      whether in tort (including negligence), contract, or otherwise,
      unless required by applicable law (such as deliberate and grossly
      negligent acts) or agreed to in writing, shall any Contributor be
      liable to You for damages, including any direct, indirect, special,
      incidental, or consequential damages of any character arising as a
      result of this License or out of the use or inability to use the
      Work (including but not limited to damages for loss of goodwill,
      work stoppage, computer failure or malfunction, or any and all
      other commercial damages or losses), even if such Contributor
      has been advised of the possibility of such damages.

   9. Accepting Warranty or Additional Liability. While redistributing
      the Work or Derivative Works thereof, You may choose to offer,
      and charge a fee for, acceptance of support, warranty, indemnity,
      or other liability obligations and/or rights consistent with this
      License. However, in accepting such obligations, You may act only
      on Your own behalf and on Your sole responsibility, not on behalf
      of any other Contributor, and only if You agree to indemnify,
      defend, and hold each Contributor harmless for any liability
      incurred by, or claims asserted against, such Contributor by reason
      of your accepting any such warranty or additional liability.

   END OF TERMS AND CONDITIONS

   APPENDIX: How to apply the Apache License to your work.

      To apply the Apache License to your work, attach the following
      boilerplate notice, with the fields enclosed by brackets "{}"
      replaced with your own identifying information. (Don't include
      the brackets!)  The text should be enclosed in the appropriate
      comment syntax for the file format. We also recommend that a
      file or class name and description of purpose be included on the
      same "printed page" as the copyright notice for easier
      identification within third-party archives.

   Copyright {yyyy} {name of copyright owner}

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */
