#include <assert.h>
#include <emmintrin.h>
#include <limits.h>
#include <stdalign.h>
#include <string.h>

#include "impl/impl_simd_hash_map.h"
#include "simd_hash_map.h"
#include "types.h"

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

/** Below are the implementations of the SIMD or bitwise operations needed to
run a search on multiple entries in the hash table simultaneously. For now,
the only container that will use these operations is this one so there is no
need to break out different headers and sources and clutter the src directory.
x86 is the only platform that gets the full benefit of SIMD. Apple and all
other platforms will get a portable implementation due to concerns over NEON
speed of vectorized instructions. */

/* We can vectorize for x86 only. */
#if defined(__x86_64) && defined(__SSE2__)

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
    return (ccc_shm_meta){(hash >> ((sizeof(uint64_t) * CHAR_BIT) - 7))
                          & LOWER_7_BITS_MASK};
}

/*=========================  Group Implementations   ========================*/

static inline group
load_group(ccc_shm_meta *const src)
{
    return (group){_mm_load_si128((__m128i *)&src->v)};
}

static inline void
store_group(ccc_shm_meta *const dst, group const *const src)
{
    _mm_store_si128((__m128i *)&dst->v, src->v);
}

static inline index_mask
match_meta(group const *const g, ccc_shm_meta const m)
{
    return (index_mask){
        _mm_movemask_epi8(_mm_cmpeq_epi8(g->v, _mm_set1_epi8((int8_t)m.v)))};
}

static inline index_mask
match_empty(group const *const g)
{
    return match_meta(g, (ccc_shm_meta){CCC_SHM_EMPTY});
}

static inline index_mask
match_empty_or_deleted(group const *const g)
{
    static_assert(sizeof(int) >= sizeof(uint16_t));
    return (index_mask){_mm_movemask_epi8(g->v)};
}

static inline index_mask
match_full(group const *const g)
{
    index_mask m = match_empty_or_deleted(g);
    m.v = ~m.v;
    return m;
}

static inline group
make_deleted_empty_and_full_deleted(group const *const g)
{
    __m128i const zero = _mm_setzero_si128();
    __m128i const match_constants = _mm_cmpgt_epi8(zero, g->v);
    return (group){
        _mm_or_si128(match_constants, _mm_set1_epi8((int8_t)CCC_SHM_DELETED))};
}

#else
#endif /* defined(__x86_64) && defined(__SSE2__) */
