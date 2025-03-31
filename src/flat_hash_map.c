#include <assert.h>
#include <emmintrin.h>
#include <limits.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "flat_hash_map.h"
#include "impl/impl_flat_hash_map.h"
#include "types.h"

#if defined(__GNUC__) || defined(__clang__)
#    define unlikely(expr) __builtin_expect(!!(expr), 0)
#    define likely(expr) __builtin_expect(!!(expr), 1)
#else
#    define unlikely(expr) expr
#    define likely(expr) expr
#endif

/* Can we vectorize? Single field unions are guaranteed by the standard to have
no padding even though some claim single field structs get the same benefit,
trivially. However, not guaranteed by the standard so use unions instead */
#if defined(__x86_64) && defined(__SSE2__) && !defined(CCC_FHM_PORTABLE)
#    include <immintrin.h>

typedef struct
{
    __m128i v;
} group;

typedef struct
{
    uint16_t v;
} index_mask;

enum : typeof((index_mask){}.v)
{
    INDEX_MASK_MSB = 0x8000,
};

#else

typedef struct
{
    uint64_t v;
} group;

typedef struct
{
    typeof((group){}.v) v;
} index_mask;

enum : typeof((index_mask){}.v)
{
    INDEX_MASK_MSB = 0x8000000000000000,
};

enum : typeof((group){}.v)
{
    GROUP_LSB = 0x101010101010101,
    GROUP_DELETED = 0x8080808080808080,
};

#endif /* defined(__x86_64) && defined(__SSE2__) &&                            \
          !defined(CCC_FHM_GENERIC)*/

enum : size_t
{
    GROUP_BYTES = sizeof(group),
    GROUP_BITS = GROUP_BYTES * CHAR_BIT,
    SIZE_T_MSB = 0x1000000000000000,
};

enum : uint8_t
{
    TAG_MSB = 0x80,
    TAG_LSB = 0x1,
    LOWER_7_BITS_MASK = 0x7F,
    TAG_BITS = sizeof(ccc_fhm_tag) * CHAR_BIT,
};

struct triangular_seq
{
    size_t i;
    size_t stride;
};

/*===========================   Prototypes   ================================*/

static struct ccc_fhash_entry_ container_entry(struct ccc_fhmap_ *h,
                                               void const *key);
static struct ccc_handl_ handle(struct ccc_fhmap_ *h, void const *key,
                                uint64_t hash);
static struct ccc_handl_ find_key_or_slot(struct ccc_fhmap_ const *h,
                                          void const *key, uint64_t hash);
static ccc_ucount find_key(struct ccc_fhmap_ const *h, void const *key,
                           uint64_t hash);
static size_t find_known_insert_slot(struct ccc_fhmap_ const *h, uint64_t hash);
static ccc_result maybe_rehash(struct ccc_fhmap_ *h, size_t to_add);
static void insert(struct ccc_fhmap_ *h, void const *key_val_type,
                   ccc_fhm_tag m, size_t i);
static void erase(struct ccc_fhmap_ *h, size_t i);
static void rehash_in_place(struct ccc_fhmap_ *h);
static ccc_result rehash_resize(struct ccc_fhmap_ *h, size_t to_add);
static void *key_at(struct ccc_fhmap_ const *h, size_t i);
static void *data_at(struct ccc_fhmap_ const *h, size_t i);
static void swap(char tmp[const], void *a, void *b, size_t ab_size);

static void set_tag(struct ccc_fhmap_ *h, ccc_fhm_tag m, size_t i);
static ccc_tribool is_index_on(index_mask m);
static size_t lowest_on_index(index_mask m);
static size_t trailing_zeros(index_mask m);
static size_t leading_zeros(index_mask m);
static size_t next_index(index_mask *m);
static ccc_tribool is_full(ccc_fhm_tag m);
static ccc_tribool is_constant(ccc_fhm_tag m);
static ccc_tribool is_empty_constant(ccc_fhm_tag m);
static ccc_fhm_tag to_tag(uint64_t hash);
static group load_group(ccc_fhm_tag const *src);
static void store_group(ccc_fhm_tag *dst, group src);
static index_mask match_tag(group g, ccc_fhm_tag m);
static index_mask match_empty(group g);
static index_mask match_empty_or_deleted(group g);
static index_mask match_full(group g);
static group make_deleted_empty_and_full_deleted(group g);
static unsigned countr_0(index_mask m);
static unsigned countl_0(index_mask m);
static unsigned countl_0_size_t(size_t n);
static size_t next_power_of_two(size_t n);

/*===========================    Interface   ================================*/

ccc_tribool
ccc_fhm_contains(ccc_flat_hash_map *const h, void const *const key)
{
    if (unlikely(!h || !key))
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (unlikely(!h->init_ || !h->sz_))
    {
        return CCC_FALSE;
    }
    return CCC_RESULT_OK
           == find_key(
                  h, key,
                  h->hash_fn_((ccc_user_key){.user_key = key, .aux = h->aux_}))
                  .error;
}

ccc_fhmap_entry
ccc_fhm_entry(ccc_flat_hash_map *const h, void const *const key)
{
    if (unlikely(!h || !key))
    {
        return (ccc_fhmap_entry){{.handle_ = {.stats_ = CCC_ENTRY_ARG_ERROR}}};
    }
    return (ccc_fhmap_entry){container_entry(h, key)};
}

void *
ccc_fhm_insert_entry(ccc_fhmap_entry const *const e, void const *key_val_type)
{

    if (unlikely(!e || !key_val_type))
    {
        return NULL;
    }
    if (e->impl_.handle_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        void *const slot = data_at(e->impl_.h_, e->impl_.handle_.i_);
        (void)memcpy(slot, key_val_type, e->impl_.h_->elem_sz_);
        return slot;
    }
    if (e->impl_.handle_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return NULL;
    }
    insert(e->impl_.h_, key_val_type, e->impl_.tag_, e->impl_.handle_.i_);
    return data_at(e->impl_.h_, e->impl_.handle_.i_);
}

ccc_entry
ccc_fhm_remove_entry(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_ARG_ERROR}};
    }
    if (e->impl_.handle_.stats_ != CCC_ENTRY_OCCUPIED)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_VACANT}};
    }
    erase(e->impl_.h_, e->impl_.handle_.i_);
    return (ccc_entry){{.e_ = data_at(e->impl_.h_, e->impl_.handle_.i_),
                        .stats_ = CCC_ENTRY_OCCUPIED}};
}

/*=========================   Static Internals   ============================*/

static struct ccc_fhash_entry_
container_entry(struct ccc_fhmap_ *const h, void const *const key)
{
    uint64_t const hash = h->hash_fn_((ccc_user_key){key, h->aux_});
    return (struct ccc_fhash_entry_){
        .h_ = (struct ccc_fhmap_ *)h,
        .tag_ = to_tag(hash),
        .handle_ = handle(h, key, hash),
    };
}

static struct ccc_handl_
handle(struct ccc_fhmap_ *const h, void const *const key, uint64_t const hash)
{
    ccc_entry_status upcoming_insertion_error = 0;
    switch (maybe_rehash(h, 1))
    {
    case CCC_RESULT_OK:
        break;
    case CCC_RESULT_ARG_ERROR:
        return (struct ccc_handl_){.stats_ = CCC_ENTRY_ARG_ERROR};
        break;
    default:
        upcoming_insertion_error = CCC_ENTRY_INSERT_ERROR;
        break;
    };
    struct ccc_handl_ res = find_key_or_slot(h, key, hash);
    res.stats_ |= upcoming_insertion_error;
    return res;
}

static void
insert(struct ccc_fhmap_ *const h, void const *const key_val_type,
       ccc_fhm_tag const m, size_t const i)
{
    assert(i <= h->mask_);
    assert((m.v & TAG_MSB) == 0);
    h->avail_ -= is_empty_constant(h->tag_[i]);
    ++h->sz_;
    set_tag(h, m, i);
    (void)memcpy(data_at(h, i), key_val_type, h->elem_sz_);
}

static void
erase(struct ccc_fhmap_ *const h, size_t const i)
{
    assert(i <= h->mask_);
    size_t const i_before = (i - CCC_FHM_GROUP_SIZE) & h->mask_;
    index_mask const i_before_empty
        = match_empty(load_group(&h->tag_[i_before]));
    index_mask const i_empty = match_empty(load_group(&h->tag_[i]));
    ccc_fhm_tag const m = leading_zeros(i_before_empty) + leading_zeros(i_empty)
                                  >= CCC_FHM_GROUP_SIZE
                              ? (ccc_fhm_tag){CCC_FHM_DELETED}
                              : (ccc_fhm_tag){CCC_FHM_EMPTY};
    h->avail_ += (m.v == CCC_FHM_EMPTY);
    --h->sz_;
    set_tag(h, m, i);
}

/* Finds the specified hash or first available slot where the hash could be
inserted. If the element does not exist and a non-occupied slot is returned
that slot will have been the first empty or deleted slot encountered in the
probe sequence. This function assumes an empty slot exists in the table. */
static struct ccc_handl_
find_key_or_slot(struct ccc_fhmap_ const *const h, void const *const key,
                 uint64_t const hash)
{
    ccc_fhm_tag const tag = to_tag(hash);
    size_t const mask = h->mask_;
    struct triangular_seq seq = {.i = hash & mask, .stride = 0};
    ccc_ucount empty_or_deleted = {.error = CCC_RESULT_FAIL};
    do
    {
        group const g = load_group(&h->tag_[seq.i]);
        index_mask m = match_tag(g, tag);
        for (size_t i_match = lowest_on_index(m); i_match != CCC_FHM_GROUP_SIZE;
             i_match = next_index(&m))
        {
            i_match = (seq.i + i_match) & mask;
            if (h->eq_fn_((ccc_key_cmp){.key_lhs = key,
                                        .user_type_rhs = data_at(h, i_match),
                                        .aux = h->aux_}))
            {
                return (struct ccc_handl_){.i_ = i_match,
                                           .stats_ = CCC_ENTRY_OCCUPIED};
            }
        }
        if (empty_or_deleted.error)
        {
            size_t const i_take = lowest_on_index(match_empty_or_deleted(g));
            if (i_take != CCC_FHM_GROUP_SIZE)
            {
                empty_or_deleted.count = (seq.i + i_take) & mask;
                empty_or_deleted.error = CCC_RESULT_OK;
            }
        }
        if (is_index_on(match_empty(g)))
        {
            return (struct ccc_handl_){.i_ = empty_or_deleted.count,
                                       .stats_ = CCC_ENTRY_VACANT};
        }
        seq.stride += CCC_FHM_GROUP_SIZE;
        seq.i += seq.stride;
        seq.i &= mask;
    } while (1);
}

/* Finds key or quits when first empty slot is encountered after a group fails
to match. This function is better when a simple lookup is needed as a few
branches and loads of groups are omitted compared to the search with intention
to insert or remove. A successful search returns the index with an OK status
while a failed search indicates a failure error. */
static ccc_ucount
find_key(struct ccc_fhmap_ const *const h, void const *const key,
         uint64_t const hash)
{
    ccc_fhm_tag const tag = to_tag(hash);
    size_t const mask = h->mask_;
    struct triangular_seq seq = {.i = hash & mask, .stride = 0};
    do
    {
        group const g = load_group(&h->tag_[seq.i]);
        index_mask m = match_tag(g, tag);
        for (size_t i_match = lowest_on_index(m); i_match != CCC_FHM_GROUP_SIZE;
             i_match = next_index(&m))
        {
            i_match = (seq.i + i_match) & mask;
            if (h->eq_fn_((ccc_key_cmp){.key_lhs = key,
                                        .user_type_rhs = data_at(h, i_match),
                                        .aux = h->aux_}))
            {
                return (ccc_ucount){.count = i_match};
            }
        }
        if (is_index_on(match_empty(g)))
        {
            return (ccc_ucount){.error = CCC_RESULT_FAIL};
        }
        seq.stride += CCC_FHM_GROUP_SIZE;
        seq.i += seq.stride;
        seq.i &= mask;
    } while (1);
}

/* Finds an insert slot or loops forever. The caller of this function must know
that there is an available empty or deleted slot in the table. */
static size_t
find_known_insert_slot(struct ccc_fhmap_ const *const h, uint64_t const hash)
{
    size_t const mask = h->mask_;
    struct triangular_seq seq = {.i = hash & mask, .stride = 0};
    do
    {
        size_t const i = lowest_on_index(
            match_empty_or_deleted(load_group(&h->tag_[seq.i])));
        if (likely(i != CCC_FHM_GROUP_SIZE))
        {
            return i;
        }
        seq.stride += CCC_FHM_GROUP_SIZE;
        seq.i += seq.stride;
        seq.i &= mask;
    } while (1);
}

static ccc_result
maybe_rehash(struct ccc_fhmap_ *const h, size_t const to_add)
{
    if (unlikely(!h->mask_ && !h->alloc_fn_))
    {
        return CCC_RESULT_NO_ALLOC;
    }
    size_t required_cap = ((h->sz_ + to_add) * 8) / 7;
    if ((required_cap & (required_cap - 1)) != 0)
    {
        required_cap = next_power_of_two(required_cap);
    }
    if (unlikely(!h->init_))
    {
        if (h->mask_)
        {
            if (!h->tag_ || h->mask_ + 1 < required_cap)
            {
                return CCC_RESULT_MEM_ERROR;
            }
            if (h->mask_ + 1 < CCC_FHM_GROUP_SIZE
                || ((h->mask_ + 1) & h->mask_) != 0)
            {
                return CCC_RESULT_ARG_ERROR;
            }
            (void)memset(h->tag_, CCC_FHM_EMPTY,
                         (h->mask_ + 1) + CCC_FHM_GROUP_SIZE);
        }
        h->init_ = CCC_TRUE;
    }
    if (unlikely(!h->mask_))
    {
        size_t const total_bytes = ((required_cap + 1) * h->elem_sz_)
                                   + (required_cap + CCC_FHM_GROUP_SIZE);
        void *const buf = h->alloc_fn_(NULL, total_bytes, h->aux_);
        if (!buf)
        {
            return CCC_RESULT_MEM_ERROR;
        }
        h->mask_ = required_cap - 1;
        h->data_ = buf;
        h->avail_ = (required_cap / 8) * 7;
        h->tag_ = (ccc_fhm_tag *)(((char *)buf + total_bytes)
                                  - (required_cap + CCC_FHM_GROUP_SIZE));
        (void)memset(h->tag_, CCC_FHM_EMPTY, required_cap + CCC_FHM_GROUP_SIZE);
    }
    if (likely(h->avail_))
    {
        return CCC_RESULT_OK;
    }
    size_t const current_cap = ((h->mask_ + 1) / 8) * 7;
    if (h->alloc_fn_ && (h->sz_ + to_add) > current_cap / 2)
    {
        assert(h->alloc_fn_);
        return rehash_resize(h, to_add);
    }
    rehash_in_place(h);
    return CCC_RESULT_OK;
}

static void
rehash_in_place(struct ccc_fhmap_ *const h)
{
    assert((h->mask_ + 1) % CCC_FHM_GROUP_SIZE == 0);
    size_t const mask = h->mask_;
    size_t const allowed_cap = ((mask + 1) / 8) * 7;
    for (size_t i = 0; i < mask + 1; i += CCC_FHM_GROUP_SIZE)
    {
        store_group(&h->tag_[i], make_deleted_empty_and_full_deleted(
                                     load_group(&h->tag_[i])));
    }
    (void)memcpy(h->tag_ + (mask + 1), h->tag_, CCC_FHM_GROUP_SIZE);
    for (size_t i = 0; i < mask + 1; ++i)
    {
        if (h->tag_[i].v == CCC_FHM_DELETED)
        {
            set_tag(h, (ccc_fhm_tag){CCC_FHM_EMPTY}, i);
            --h->sz_;
        }
    }
    h->avail_ = allowed_cap - h->sz_;
    for (size_t i = 0; i < mask + 1; ++i)
    {
        if (h->tag_[i].v != CCC_FHM_DELETED)
        {
            continue;
        }
        do
        {
            uint64_t const hash = h->hash_fn_(
                (ccc_user_key){.user_key = key_at(h, i), .aux = h->aux_});
            ccc_fhm_tag const hash_tag = to_tag(hash);
            size_t const new_slot = find_known_insert_slot(h, hash);
            size_t const hash_pos = (hash & mask);
            size_t const group_a = ((i - hash_pos) & mask) / CCC_FHM_GROUP_SIZE;
            size_t const group_b
                = ((new_slot - hash_pos) & mask) / CCC_FHM_GROUP_SIZE;
            if (group_a == group_b)
            {
                set_tag(h, hash_tag, i);
                break; /* continues outer loop */
            }
            ccc_fhm_tag const prev = h->tag_[new_slot];
            set_tag(h, hash_tag, new_slot);
            if (prev.v == CCC_FHM_EMPTY)
            {
                set_tag(h, (ccc_fhm_tag){CCC_FHM_EMPTY}, i);
                (void)memcpy(data_at(h, new_slot), data_at(h, i), h->elem_sz_);
                break; /* continues outer loop */
            }
            assert(prev.v == CCC_FHM_DELETED);
            swap(h->data_, data_at(h, i), data_at(h, new_slot), h->elem_sz_);
        } while (1);
    }
    h->avail_ = allowed_cap - h->sz_;
}

static ccc_result
rehash_resize(struct ccc_fhmap_ *const h, size_t const to_add)
{
    assert(((h->mask_ + 1) & h->mask_) == 0);
    size_t const new_pow2_cap = next_power_of_two(h->mask_ + 1 + to_add) << 1;
    if (new_pow2_cap < (h->mask_ + 1))
    {
        return CCC_RESULT_MEM_ERROR;
    }
    size_t const prev_bytes
        = ((h->mask_ + 2) * h->elem_sz_) + (h->mask_ + 1 + CCC_FHM_GROUP_SIZE);
    size_t const total_bytes = ((new_pow2_cap + 1) * h->elem_sz_)
                               + (new_pow2_cap + CCC_FHM_GROUP_SIZE);
    if (total_bytes < prev_bytes)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    void *const new_buf = h->alloc_fn_(NULL, total_bytes, h->aux_);
    if (!new_buf)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    struct ccc_fhmap_ new_h = *h;
    new_h.sz_ = 0;
    new_h.avail_ = (new_pow2_cap / 8) * 7;
    new_h.mask_ = new_pow2_cap - 1;
    new_h.tag_ = (ccc_fhm_tag *)(((char *)new_buf + total_bytes) - new_pow2_cap
                                 - CCC_FHM_GROUP_SIZE);
    new_h.data_ = new_buf;
    (void)memset(new_h.tag_, CCC_FHM_EMPTY, new_pow2_cap + CCC_FHM_GROUP_SIZE);
    for (size_t i = 0; i < (h->mask_ + 1); ++i)
    {
        if (is_full(h->tag_[i]))
        {
            uint64_t const hash = h->hash_fn_(
                (ccc_user_key){.user_key = key_at(h, i), .aux = h->aux_});
            size_t const new_i = find_known_insert_slot(&new_h, hash);
            set_tag(&new_h, to_tag(hash), new_i);
            (void)memcpy(data_at(&new_h, new_i), data_at(h, i), new_h.elem_sz_);
        }
    }
    new_h.avail_ -= h->sz_;
    new_h.sz_ = h->sz_;
    (void)h->alloc_fn_(h->data_, 0, h->aux_);
    *h = new_h;
    return CCC_RESULT_OK;
}

static inline void
set_tag(struct ccc_fhmap_ *const h, ccc_fhm_tag const m, size_t const i)
{
    size_t const replica_byte
        = ((i - CCC_FHM_GROUP_SIZE) & h->mask_) + CCC_FHM_GROUP_SIZE;
    h->tag_[i] = m;
    h->tag_[replica_byte] = m;
}

static inline ccc_tribool
is_full(ccc_fhm_tag const m)
{
    return (m.v & CCC_FHM_DELETED) == 0;
}

static inline ccc_tribool
is_constant(ccc_fhm_tag const m)
{
    return (m.v & TAG_MSB) != 0;
}

static inline ccc_tribool
is_empty_constant(ccc_fhm_tag const m)
{
    assert(is_constant(m));
    return (m.v & TAG_LSB) != 0;
}

static inline ccc_fhm_tag
to_tag(uint64_t const hash)
{
    return (ccc_fhm_tag){(hash >> ((sizeof(hash) * CHAR_BIT) - 7))
                         & LOWER_7_BITS_MASK};
}

static inline void *
key_at(struct ccc_fhmap_ const *const h, size_t const i)
{
    assert(i <= h->mask_);
    return (char *)(h->tag_ - ((i + 1) * h->elem_sz_)) + h->key_offset_;
}

static inline void *
data_at(struct ccc_fhmap_ const *const h, size_t const i)
{
    assert(i <= h->mask_);
    return h->tag_ - ((i + 1) * h->elem_sz_);
}

static inline void
swap(char tmp[const], void *const a, void *const b, size_t const ab_size)
{
    if (unlikely(!a || !b || a == b))
    {
        return;
    }
    (void)memcpy(tmp, a, ab_size);
    (void)memcpy(a, b, ab_size);
    (void)memcpy(b, tmp, ab_size);
}

static inline size_t
next_power_of_two(size_t const n)
{
    return n <= 1 ? n : (SIZE_MAX >> countl_0_size_t(n - 1)) + 1;
}

/*=====================   Intrinsics and Generics   =========================*/

/** Below are the implementations of the SIMD or bitwise operations needed to
run a search on multiple entries in the hash table simultaneously. For now,
the only container that will use these operations is this one so there is no
need to break out different headers and sources and clutter the src directory.
x86 is the only platform that gets the full benefit of SIMD. Apple and all
other platforms will get a portable implementation due to concerns over NEON
speed of vectorized instructions. However, loading up groups into a uint64_t is
still good and counts simultaneous operations just not the type that uses CPU
vector lanes for a single instruction. */

/* We can vectorize for x86 only. */
#if defined(__x86_64) && defined(__SSE2__) && !defined(CCC_FHM_PORTABLE)

/*========================  Index Mask Implementations   ====================*/

static inline ccc_tribool
is_index_on(index_mask const m)
{
    return m.v != 0;
}

static inline size_t
lowest_on_index(index_mask const m)
{
    return countr_0(m);
}

static inline size_t
trailing_zeros(index_mask const m)
{
    return countr_0(m);
}

static inline size_t
leading_zeros(index_mask const m)
{
    return countl_0(m);
}

static inline size_t
next_index(index_mask *const m)
{
    assert(m);
    size_t const index = lowest_on_index(*m);
    m->v &= (m->v - 1);
    return index;
}

/*=========================  Group Implementations   ========================*/

static inline group
load_group(ccc_fhm_tag const *const src)
{
    return (group){_mm_load_si128((__m128i *)&src->v)};
}

static inline void
store_group(ccc_fhm_tag *const dst, group const src)
{
    _mm_store_si128((__m128i *)&dst->v, src.v);
}

static inline index_mask
match_tag(group const g, ccc_fhm_tag const m)
{
    return (index_mask){
        _mm_movemask_epi8(_mm_cmpeq_epi8(g.v, _mm_set1_epi8((int8_t)m.v)))};
}

static inline index_mask
match_empty(group const g)
{
    return match_tag(g, (ccc_fhm_tag){CCC_FHM_EMPTY});
}

static inline index_mask
match_empty_or_deleted(group const g)
{
    static_assert(sizeof(int) >= sizeof(uint16_t));
    return (index_mask){_mm_movemask_epi8(g.v)};
}

static inline index_mask
match_full(group const g)
{
    index_mask m = match_empty_or_deleted(g);
    m.v = ~m.v;
    return m;
}

static inline group
make_deleted_empty_and_full_deleted(group const g)
{
    __m128i const zero = _mm_setzero_si128();
    __m128i const match_constants = _mm_cmpgt_epi8(zero, g.v);
    return (group){
        _mm_or_si128(match_constants, _mm_set1_epi8((int8_t)CCC_FHM_DELETED))};
}

/*====================  Bit Counting for Index Mask   =======================*/

#    if defined(__has_builtin) && __has_builtin(__builtin_ctz)                 \
        && __has_builtin(__builtin_clz) && __has_builtin(__builtin_clzl)

static_assert(sizeof((index_mask){}.v) <= sizeof(unsigned));

static inline unsigned
countr_0(index_mask const m)
{
    /* Even though the mask is implicitly widened to int width there is
       guaranteed to be a 1 bit between bit index 0 and CCC_FHM_GROUP_SIZE - 1
       if the builtin is called. */
    return m.v ? __builtin_ctz(m.v) : CCC_FHM_GROUP_SIZE;
}

static inline unsigned
countl_0(index_mask const m)
{
    /* In the simd version the index mask is only a uint16_t so the number of
       leading zeros would be inaccurate due to widening. There is only a
       builtin for leading and trailing zeros for int width as smallest. */
    static_assert(sizeof(m.v) * 2 == sizeof(unsigned));
    static_assert(CCC_FHM_GROUP_SIZE * 2UL == sizeof(unsigned) * CHAR_BIT);
    return m.v ? __builtin_clz(((unsigned)m.v) << CCC_FHM_GROUP_SIZE)
               : CCC_FHM_GROUP_SIZE;
}

static inline unsigned
countl_0_size_t(size_t const n)
{
    static_assert(sizeof(size_t) == sizeof(unsigned long));
    return n ? __builtin_clzl(n) : sizeof(size_t) * CHAR_BIT;
}

#    else /* !defined(__has_builtin) || !__has_builtin(__builtin_ctz)          \
        || !__has_builtin(__builtin_clz) || !__has_builtin(__builtin_clzl) */

static inline unsigned
countr_0(index_mask m)
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
countl_0(index_mask m)
{
    if (!m.v)
    {
        return CCC_FHM_GROUP_SIZE;
    }
    unsigned mv = (unsigned)m.v << CCC_FHM_GROUP_SIZE;
    unsigned cnt = 0;
    for (; (mv & (INDEX_MASK_MSB << CCC_FHM_GROUP_SIZE)) == 0; ++cnt, mv <<= 1U)
    {}
    return cnt;
}

static inline unsigned
countl_0_size_t(size_t n)
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

#else /* !defined(__x86_64) || !defined(__SSE2__) ||                           \
         !defined(CCC_FHM_PORTABLE) */

/* What follows is the generic portable implementation when high width SIMD
can't be achieved. For now this means NEON on Apple defaults to generic but
this will likely change in the future as NEON improves. */

/*==========================    Bit Helpers    ==============================*/

#    if defined(__has_builtin) && __has_builtin(__builtin_ctzl)                \
        && __has_builtin(__builtin_clzl)

static_assert(sizeof((index_mask){}.v) == sizeof(long));

static inline unsigned
countr_0(index_mask const m)
{
    return m.v ? __builtin_ctzl(m.v) / CCC_FHM_GROUP_SIZE : CCC_FHM_GROUP_SIZE;
}

static inline unsigned
countl_0(index_mask const m)
{
    return m.v ? __builtin_clzl(m.v) / CCC_FHM_GROUP_SIZE : CCC_FHM_GROUP_SIZE;
}

static inline unsigned
countl_0_size_t(size_t const n)
{
    static_assert(sizeof(size_t) == sizeof(unsigned long));
    return n ? __builtin_clzl(n) : sizeof(size_t) * CHAR_BIT;
}

#    else /* defined(__has_builtin) && __has_builtin(__builtin_ctzl) &&        \
             __has_builtin(__builtin_clzl) */

static inline unsigned
countr_0(index_mask m)
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
countl_0(index_mask m)
{
    if (!m.v)
    {
        return CCC_FHM_GROUP_SIZE;
    }
    unsigned cnt = 0;
    for (; (m.v & INDEX_MASK_MSB) == 0; ++cnt, m.v <<= 1U)
    {}
    return cnt / CCC_FHM_GROUP_SIZE;
}

static inline unsigned
countl_0_size_t(size_t n)
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

/* Returns 1 aka true if platform is little endian otherwise false for big
endian. */
static inline int
is_little_endian(void)
{
    unsigned int x = 1;
    char *c = (char *)&x;
    return (int)*c;
}

static inline index_mask
to_little_endian(index_mask m)
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

/*========================  Index Mask Implementations   ====================*/

static inline ccc_tribool
is_index_on(index_mask const m)
{
    return m.v != 0;
}

static inline size_t
lowest_on_index(index_mask const m)
{
    return countr_0(m);
}

static inline size_t
trailing_zeros(index_mask const m)
{
    return countr_0(m);
}

static inline size_t
leading_zeros(index_mask const m)
{
    return countl_0(m);
}

static inline size_t
next_index(index_mask *const m)
{
    assert(m);
    size_t const index = lowest_on_index(*m);
    m->v &= (m->v - 1);
    return index;
}

/*=========================  Group Implementations   ========================*/

static inline group
load_group(ccc_fhm_tag const *const src)
{
    group g;
    (void)memcpy(&g, src, sizeof(g));
    return g;
}

static inline void
store_group(ccc_fhm_tag *const dst, group const src)
{
    (void)memcpy(dst, &src, sizeof(src));
}

static inline index_mask
match_tag(group g, ccc_fhm_tag const m)
{
    group const cmp = {g.v
                       ^ ((((typeof(g.v))m.v) << (TAG_BITS * 7UL))
                          | (((typeof(g.v))m.v) << (TAG_BITS * 6UL))
                          | (((typeof(g.v))m.v) << (TAG_BITS * 5UL))
                          | (((typeof(g.v))m.v) << (TAG_BITS * 4UL))
                          | (((typeof(g.v))m.v) << (TAG_BITS * 3UL))
                          | (((typeof(g.v))m.v) << (TAG_BITS * 2UL))
                          | (((typeof(g.v))m.v) << TAG_BITS) | m.v)};
    return to_little_endian(
        (index_mask){(cmp.v - GROUP_LSB) & ~cmp.v & GROUP_DELETED});
}

static inline index_mask
match_empty(group const g)
{
    return to_little_endian((index_mask){g.v & (g.v << 1) & GROUP_DELETED});
}

static inline index_mask
match_empty_or_deleted(group const g)
{
    return to_little_endian((index_mask){g.v & GROUP_DELETED});
}

static inline index_mask
match_full(group const g)
{
    index_mask res = match_empty_or_deleted(g);
    res.v = ~res.v;
    return res;
}

static inline group
make_deleted_empty_and_full_deleted(group g)
{
    g.v = ~g.v & GROUP_DELETED;
    g.v = ~g.v + (g.v >> (TAG_BITS - 1));
    return g;
}

#endif /* defined(__x86_64) && defined(__SSE2__) */
