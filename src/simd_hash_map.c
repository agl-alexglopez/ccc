#include <assert.h>
#include <emmintrin.h>
#include <limits.h>
#include <stdalign.h>
#include <stddef.h>
#include <string.h>

#include "impl/impl_simd_hash_map.h"
#include "simd_hash_map.h"
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
#if defined(__x86_64) && defined(__SSE2__)
#    include <immintrin.h>

typedef struct
{
    __m128i v;
} group;

typedef struct
{
    uint16_t v;
} index_mask;

enum
{
    GROUP_BYTES = sizeof(group),
    INDEX_MASK_MSB = 0x8000,
};

alignas(16) static const ccc_shm_meta empty_group[GROUP_BYTES] = {
    {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY},
    {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY},
    {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY},
    {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY},
};

#else
#    include <stdint.h>
typedef struct
{
    uint64_t v;
} group;

typedef struct
{
    typeof((group){}.v) v;
} index_mask;

enum
{
    GROUP_BYTES = sizeof(group),
    INDEX_MASK_MSBYTE = 0xFF00000000000000,
};

alignas(8) static const ccc_shm_meta empty_group[GROUP_BYTES] = {
    {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY},
    {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY}, {CCC_SHM_EMPTY},
};
#endif /* defined(__x86_64) && defined(__SSE2__) */

enum : uint8_t
{
    META_MSB = 0x80,
    META_LSB = 0x1,
    LOWER_7_BITS_MASK = 0x7F,
};

struct triangular_seq
{
    size_t i;
    size_t stride;
};

/*===========================   Prototypes   ================================*/

static struct ccc_shash_entry_ container_entry(struct ccc_shmap_ *h,
                                               void const *key);
static struct ccc_handl_ handle(struct ccc_shmap_ *h, void const *key,
                                uint64_t hash);
static struct ccc_handl_ find_key_or_slot(struct ccc_shmap_ const *h,
                                          void const *key, uint64_t hash);
static ccc_ucount find_key(struct ccc_shmap_ const *h, void const *key,
                           uint64_t hash);
static size_t find_known_insert_slot(struct ccc_shmap_ const *h, uint64_t hash);
static ccc_result maybe_rehash(struct ccc_shmap_ *h);
static void insert(struct ccc_shmap_ *h, void const *key_val_type,
                   ccc_shm_meta m, size_t i);
static void erase(struct ccc_shmap_ *h, size_t i);
static void rehash_in_place(struct ccc_shmap_ *h);
static ccc_result rehash_resize(struct ccc_shmap_ *h);
static void *key_at(struct ccc_shmap_ const *h, size_t i);
static void *data_at(struct ccc_shmap_ const *h, size_t i);
static void swap(char tmp[const], void *a, void *b, size_t ab_size);

static void set_meta(struct ccc_shmap_ *h, ccc_shm_meta m, size_t i);
static ccc_tribool is_index_on(index_mask m);
static size_t lowest_on_index(index_mask m);
static size_t trailing_zeros(index_mask m);
static size_t leading_zeros(index_mask m);
static size_t next_index(index_mask *m);
static ccc_tribool is_full(ccc_shm_meta m);
static ccc_tribool is_constant(ccc_shm_meta m);
static ccc_tribool is_empty_constant(ccc_shm_meta m);
static ccc_shm_meta to_meta(uint64_t hash);
static group load_group(ccc_shm_meta *src);
static void store_group(ccc_shm_meta *dst, group src);
static index_mask match_meta(group g, ccc_shm_meta m);
static index_mask match_empty(group g);
static index_mask match_empty_or_deleted(group g);
static index_mask match_full(group g);
static group make_deleted_empty_and_full_deleted(group g);
static unsigned countr_0(index_mask m);
static unsigned countl_0(index_mask m);

/*===========================    Interface   ================================*/

ccc_shmap_entry
ccc_shm_entry(ccc_simd_hash_map *const h, void const *const key)
{
    if (unlikely(!h || !key))
    {
        return (ccc_shmap_entry){{.handle_ = {.stats_ = CCC_ENTRY_ARG_ERROR}}};
    }
    return (ccc_shmap_entry){container_entry(h, key)};
}

void *
ccc_shm_insert_entry(ccc_shmap_entry *const e, void const *key_val_type)
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
    insert(e->impl_.h_, key_val_type, e->impl_.meta_, e->impl_.handle_.i_);
    return data_at(e->impl_.h_, e->impl_.handle_.i_);
}

/*=========================   Static Internals   ============================*/

static struct ccc_shash_entry_
container_entry(struct ccc_shmap_ *const h, void const *const key)
{
    uint64_t const hash = h->hash_fn_((ccc_user_key){key, h->aux_});
    return (struct ccc_shash_entry_){
        .h_ = (struct ccc_shmap_ *)h,
        .meta_ = to_meta(hash),
        .handle_ = handle(h, key, hash),
    };
}

static struct ccc_handl_
handle(struct ccc_shmap_ *const h, void const *const key, uint64_t const hash)
{
    ccc_entry_status upcoming_insertion_error = 0;
    switch (maybe_rehash(h))
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
insert(struct ccc_shmap_ *const h, void const *const key_val_type,
       ccc_shm_meta const m, size_t const i)
{
    assert(i <= h->mask_);
    assert((m.v & META_MSB) == 0);
    h->avail_ -= is_empty_constant(h->meta_[i]);
    ++h->sz_;
    set_meta(h, m, i);
    (void)memcpy(data_at(h, i), key_val_type, h->elem_sz_);
}

static void
erase(struct ccc_shmap_ *const h, size_t const i)
{
    assert(i <= h->mask_);
    size_t const i_before = (i - CCC_SHM_GROUP_SIZE) & h->mask_;
    index_mask const i_before_empty
        = match_empty(load_group(&h->meta_[i_before]));
    index_mask const i_empty = match_empty(load_group(&h->meta_[i]));
    ccc_shm_meta const m
        = leading_zeros(i_before_empty) + leading_zeros(i_empty)
                  >= CCC_SHM_GROUP_SIZE
              ? (ccc_shm_meta){CCC_SHM_DELETED}
              : (ccc_shm_meta){CCC_SHM_EMPTY};
    h->avail_ += (m.v == CCC_SHM_EMPTY);
    --h->sz_;
    set_meta(h, m, i);
}

/* Finds the specified hash or first available slot where the hash could be
inserted. If the element does not exist and a non-occupied slot is returned
that slot will have been the first empty or deleted slot encountered in the
probe sequence. This function assumes an empty slot exists in the table. */
static struct ccc_handl_
find_key_or_slot(struct ccc_shmap_ const *const h, void const *const key,
                 uint64_t const hash)
{
    ccc_shm_meta const meta = to_meta(hash);
    size_t const mask = h->mask_;
    struct triangular_seq seq = {.i = hash & mask, .stride = 0};
    ccc_ucount empty_or_deleted = {.error = CCC_RESULT_FAIL};
    do
    {
        group const g = load_group(&h->meta_[seq.i]);
        index_mask m = match_meta(g, meta);
        for (size_t i_match = lowest_on_index(m); i_match != CCC_SHM_GROUP_SIZE;
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
            if (i_take != CCC_SHM_GROUP_SIZE)
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
        seq.stride += CCC_SHM_GROUP_SIZE;
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
find_key(struct ccc_shmap_ const *const h, void const *const key,
         uint64_t const hash)
{
    ccc_shm_meta const meta = to_meta(hash);
    size_t const mask = h->mask_;
    struct triangular_seq seq = {.i = hash & mask, .stride = 0};
    do
    {
        group const g = load_group(&h->meta_[seq.i]);
        index_mask m = match_meta(g, meta);
        for (size_t i_match = lowest_on_index(m); i_match != CCC_SHM_GROUP_SIZE;
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
        seq.stride += CCC_SHM_GROUP_SIZE;
        seq.i += seq.stride;
        seq.i &= mask;
    } while (1);
}

static size_t
find_known_insert_slot(struct ccc_shmap_ const *const h, uint64_t const hash)
{
    size_t const mask = h->mask_;
    struct triangular_seq seq = {.i = hash & mask, .stride = 0};
    do
    {
        size_t const i = lowest_on_index(
            match_empty_or_deleted(load_group(&h->meta_[seq.i])));
        if (likely(i != CCC_SHM_GROUP_SIZE))
        {
            return i;
        }
        seq.stride += CCC_SHM_GROUP_SIZE;
        seq.i += seq.stride;
        seq.i &= mask;
    } while (1);
}

static ccc_result
maybe_rehash(struct ccc_shmap_ *const h)
{
    if (unlikely(!h->mask_ && !h->alloc_fn_))
    {
        return CCC_RESULT_NO_ALLOC;
    }
    if (unlikely(!h->init_))
    {
        if (h->mask_)
        {
            if (!h->meta_)
            {
                return CCC_RESULT_MEM_ERROR;
            }
            if (h->mask_ + 1 < CCC_SHM_GROUP_SIZE
                || ((h->mask_ + 1) & h->mask_) != 0)
            {
                return CCC_RESULT_ARG_ERROR;
            }
            (void)memset(h->meta_, CCC_SHM_EMPTY,
                         (h->mask_ + 1) + CCC_SHM_GROUP_SIZE);
        }
        h->init_ = CCC_TRUE;
    }
    if (unlikely(!h->mask_))
    {
        size_t const total_bytes = ((CCC_SHM_GROUP_SIZE + 1) * h->elem_sz_)
                                   + (CCC_SHM_GROUP_SIZE * 2UL);
        void *const buf = h->alloc_fn_(NULL, total_bytes, h->aux_);
        if (!buf)
        {
            return CCC_RESULT_MEM_ERROR;
        }
        h->mask_ = CCC_SHM_GROUP_SIZE - 1;
        h->data_ = buf;
        h->avail_ = CCC_SHM_GROUP_SIZE;
        h->meta_ = (ccc_shm_meta *)(((char *)buf + total_bytes)
                                    - (CCC_SHM_GROUP_SIZE * 2UL));
        (void)memset(h->meta_, CCC_SHM_EMPTY, (CCC_SHM_GROUP_SIZE * 2UL));
    }
    if (likely(h->avail_))
    {
        return CCC_RESULT_OK;
    }
    size_t const allowed_cap = ((h->mask_ + 1) / 8) * 7;
    if (h->alloc_fn_ && (h->sz_ + 1) > allowed_cap / 2)
    {
        assert(h->alloc_fn_);
        return rehash_resize(h);
    }
    rehash_in_place(h);
    return CCC_RESULT_OK;
}

static void
rehash_in_place(struct ccc_shmap_ *const h)
{
    assert((h->mask_ + 1) % CCC_SHM_GROUP_SIZE == 0);
    size_t const mask = h->mask_;
    size_t const allowed_cap = ((mask + 1) / 8) * 7;
    for (size_t i = 0; i < mask + 1; i += CCC_SHM_GROUP_SIZE)
    {
        store_group(&h->meta_[i], make_deleted_empty_and_full_deleted(
                                      load_group(&h->meta_[i])));
    }
    (void)memcpy(h->meta_ + (mask + 1), h->meta_, CCC_SHM_GROUP_SIZE);
    for (size_t i = 0; i < mask + 1; ++i)
    {
        if (h->meta_[i].v == CCC_SHM_DELETED)
        {
            set_meta(h, (ccc_shm_meta){CCC_SHM_EMPTY}, i);
            --h->sz_;
        }
    }
    h->avail_ = allowed_cap - h->sz_;
    for (size_t i = 0; i < mask + 1; ++i)
    {
        if (h->meta_[i].v != CCC_SHM_DELETED)
        {
            continue;
        }
        do
        {
            uint64_t const hash = h->hash_fn_(
                (ccc_user_key){.user_key = key_at(h, i), .aux = h->aux_});
            ccc_shm_meta const hash_meta = to_meta(hash);
            size_t const new_slot = find_known_insert_slot(h, hash);
            size_t const hash_pos = (hash & mask);
            size_t const group_a = ((i - hash_pos) & mask) / CCC_SHM_GROUP_SIZE;
            size_t const group_b
                = ((new_slot - hash_pos) & mask) / CCC_SHM_GROUP_SIZE;
            if (group_a == group_b)
            {
                set_meta(h, hash_meta, i);
                break; /* continues outer loop. */
            }
            ccc_shm_meta const prev = h->meta_[new_slot];
            set_meta(h, hash_meta, new_slot);
            if (prev.v == CCC_SHM_EMPTY)
            {
                set_meta(h, (ccc_shm_meta){CCC_SHM_EMPTY}, i);
                (void)memcpy(data_at(h, new_slot), data_at(h, i), h->elem_sz_);
                break; /* continues outer loop. */
            }
            assert(prev.v == CCC_SHM_DELETED);
            swap(h->data_, data_at(h, i), data_at(h, new_slot), h->elem_sz_);
        } while (1);
    }
    h->avail_ = allowed_cap - h->sz_;
}

static ccc_result
rehash_resize(struct ccc_shmap_ *const h)
{
    assert(((h->mask_ + 1) & h->mask_) == 0);
    size_t const new_pow2_cap = (h->mask_ + 1) << 1;
    assert((new_pow2_cap & (new_pow2_cap - 1)) == 0);
    if (new_pow2_cap < (h->mask_ + 1))
    {
        return CCC_RESULT_MEM_ERROR;
    }
    size_t const prev_bytes
        = ((h->mask_ + 2) * h->elem_sz_) + (h->mask_ + 1 + CCC_SHM_GROUP_SIZE);
    size_t const total_bytes
        = ((new_pow2_cap + 1) * h->elem_sz_) + (new_pow2_cap * 2);
    if (total_bytes < prev_bytes)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    void *const new_buf = h->alloc_fn_(NULL, total_bytes, h->aux_);
    if (!new_buf)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    struct ccc_shmap_ new_h = *h;
    new_h.sz_ = 0;
    new_h.avail_ = (new_pow2_cap / 8) * 7;
    new_h.mask_ = new_pow2_cap - 1;
    new_h.meta_ = (ccc_shm_meta *)(((char *)new_buf + total_bytes)
                                   - new_pow2_cap - CCC_SHM_GROUP_SIZE);
    new_h.data_ = new_buf;
    (void)memset(new_h.meta_, CCC_SHM_EMPTY, new_pow2_cap + CCC_SHM_GROUP_SIZE);
    for (size_t i = 0; i < (h->mask_ + 1); ++i)
    {
        if (is_full(h->meta_[i]))
        {
            uint64_t const hash = h->hash_fn_(
                (ccc_user_key){.user_key = key_at(h, i), .aux = h->aux_});
            size_t const new_i = find_known_insert_slot(&new_h, hash);
            set_meta(&new_h, to_meta(hash), new_i);
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
set_meta(struct ccc_shmap_ *const h, ccc_shm_meta const m, size_t const i)
{
    size_t const replica_byte
        = ((i - CCC_SHM_GROUP_SIZE) & h->mask_) + CCC_SHM_GROUP_SIZE;
    h->meta_[i] = m;
    h->meta_[replica_byte] = m;
}

static inline void *
key_at(struct ccc_shmap_ const *const h, size_t const i)
{
    assert(i <= h->mask_);
    return (char *)(h->meta_ - ((i + 1) * h->elem_sz_)) + h->key_offset_;
}

static inline void *
data_at(struct ccc_shmap_ const *const h, size_t const i)
{
    assert(i <= h->mask_);
    return h->meta_ - ((i + 1) * h->elem_sz_);
}

static inline void
swap(char tmp[const], void *a, void *b, size_t ab_size)
{
    if (unlikely(!a || !b || a == b))
    {
        return;
    }
    (void)memcpy(tmp, a, ab_size);
    (void)memcpy(a, b, ab_size);
    (void)memcpy(b, tmp, ab_size);
}

/*=====================   Intrinsics and Generics   =========================*/

/** Below are the implementations of the SIMD or bitwise operations needed to
run a search on multiple entries in the hash table simultaneously. For now,
the only container that will use these operations is this one so there is no
need to break out different headers and sources and clutter the src directory.
x86 is the only platform that gets the full benefit of SIMD. Apple and all
other platforms will get a portable implementation due to concerns over NEON
speed of vectorized instructions. However, loading up groups into a uint64_t is
still good and for simultaneous operations just not the type that uses CPU
vector lanes for a single instruction. */

/* We can vectorize for x86 only. */
#if defined(__x86_64) && defined(__SSE2__)

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

/*========================  Metadata Implementations   ======================*/

static inline ccc_tribool
is_full(ccc_shm_meta const m)
{
    return (m.v & CCC_SHM_DELETED) == 0;
}

static inline ccc_tribool
is_constant(ccc_shm_meta const m)
{
    return (m.v & META_MSB) != 0;
}

static inline ccc_tribool
is_empty_constant(ccc_shm_meta const m)
{
    assert(is_constant(m));
    return (m.v & META_LSB) != 0;
}

static inline ccc_shm_meta
to_meta(uint64_t const hash)
{
    return (ccc_shm_meta){(hash >> ((sizeof(hash) * CHAR_BIT) - 7))
                          & LOWER_7_BITS_MASK};
}

/*=========================  Group Implementations   ========================*/

static inline group
load_group(ccc_shm_meta *const src)
{
    return (group){_mm_load_si128((__m128i *)&src->v)};
}

static inline void
store_group(ccc_shm_meta *const dst, group const src)
{
    _mm_store_si128((__m128i *)&dst->v, src.v);
}

static inline index_mask
match_meta(group const g, ccc_shm_meta const m)
{
    return (index_mask){
        _mm_movemask_epi8(_mm_cmpeq_epi8(g.v, _mm_set1_epi8((int8_t)m.v)))};
}

static inline index_mask
match_empty(group const g)
{
    return match_meta(g, (ccc_shm_meta){CCC_SHM_EMPTY});
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
        _mm_or_si128(match_constants, _mm_set1_epi8((int8_t)CCC_SHM_DELETED))};
}

/*====================  Bit Counting for Index Mask   =======================*/

#    if defined(__GNUC__) || defined(__clang__)

static inline unsigned
countr_0(index_mask const m)
{
    return m.v ? __builtin_ctz(m.v) : CCC_SHM_GROUP_SIZE;
}

static inline unsigned
countl_0(index_mask const m)
{
    return m.v ? __builtin_clz(m.v) : CCC_SHM_GROUP_SIZE;
}

#    else /* !defined(__GNUC__) && !defined(__clang__) */

static inline unsigned
countr_0(index_mask m)
{
    if (!m.v)
    {
        return CCC_SHM_GROUP_SIZE;
    }
    unsigned cnt = 0;
    for (; m.v; cnt += !!(m.v & 1U), m.v >>= 1U)
    {}
    return cnt;
}

static inline unsigned
countl_0(index_mask m)
{
    if (!m.v)
    {
        return CCC_SHM_GROUP_SIZE;
    }
    unsigned cnt = 0;
    for (; !(m.v & INDEX_MASK_MSB); ++cnt, m.v <<= 1U)
    {}
    return cnt;
}

#    endif /* defined(__GNUC__) || defined(__clang__) */

#else
#endif /* defined(__x86_64) && defined(__SSE2__) */
