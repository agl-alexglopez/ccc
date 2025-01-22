#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "handle_hash_map.h"
#include "impl/impl_handle_hash_map.h"
#include "impl/impl_types.h"
#include "types.h"

#if defined(__GNUC__) || defined(__clang__)
#    define unlikely(expr) __builtin_expect(!!(expr), 0)
#    define likely(expr) __builtin_expect(!!(expr), 1)
#else
#    define unlikely(expr) expr
#    define likely(expr) expr
#endif

/* We are fairly close to the maximum 64 bit address space size by the last
   prime so we will stop there as max size if we ever get there. Not likely.
   The last prime is closer to the second to last because doubling breaks down
   at the end. */
#define PRIMES_SIZE 58ULL
static size_t const primes[PRIMES_SIZE] = {
    11ULL,
    37ULL,
    79ULL,
    163ULL,
    331ULL,
    691ULL,
    1439ULL,
    2999ULL,
    6079ULL,
    12263ULL,
    25717ULL,
    53611ULL,
    111697ULL,
    232499ULL,
    483611ULL,
    1005761ULL,
    2091191ULL,
    4347799ULL,
    9039167ULL,
    18792019ULL,
    39067739ULL,
    81219493ULL,
    168849973ULL,
    351027263ULL,
    729760741ULL,
    1517120861ULL,
    3153985903ULL,
    6556911073ULL,
    13631348041ULL,
    28338593999ULL,
    58913902231ULL,
    122477772247ULL,
    254622492793ULL,
    529341875947ULL,
    1100463743389ULL,
    2287785087839ULL,
    4756140888377ULL,
    9887675318611ULL,
    20555766849589ULL,
    42733962953639ULL,
    88840839803449ULL,
    184693725350723ULL,
    383964990193007ULL,
    798235638020831ULL,
    1659474561692683ULL,
    3449928429320413ULL,
    7172153428667531ULL,
    14910391869919981ULL,
    30997633824443711ULL,
    64441854452711651ULL,
    133969987155270011ULL,
    278513981492471381ULL,
    579010564484755961ULL,
    1203721378684243091ULL,
    2502450294306576181ULL,
    5202414434410211531ULL,
    10815445968671840317ULL,
    17617221824571301183ULL,
};

/* Some claim that Robin Hood tables can support a much higher load factor. I
   would not be so sure. The primary clustering effect in these types of
   tables can quickly rise. Mitigating with a lower load factor and prime
   table sizes is a decent approach. Measure. */
static double const load_factor = 0.8;
static size_t const last_swap_slot = 1;
static size_t const num_swap_slots = 2;

/*=========================   Prototypes   ==================================*/

static void erase_meta(struct ccc_hhmap_ *, size_t e);
static void swap_user_data(struct ccc_hhmap_ *, void *, void *);
static void swap_meta_data(struct ccc_hhmap_ *h, struct ccc_hhmap_elem_ *a,
                           struct ccc_hhmap_elem_ *b);
static void *struct_base(struct ccc_hhmap_ const *,
                         struct ccc_hhmap_elem_ const *);
static struct ccc_handle_ entry(struct ccc_hhmap_ *, void const *key,
                                uint64_t hash);
static struct ccc_hhash_entry_ container_entry(struct ccc_hhmap_ *h,
                                               void const *key);
static struct ccc_hhash_entry_ *and_modify(struct ccc_hhash_entry_ *e,
                                           ccc_update_fn *fn);
static bool valid_distance_from_home(struct ccc_hhmap_ const *, size_t slot);
static size_t to_i(size_t capacity, uint64_t hash);
static size_t increment(size_t capacity, size_t i);
static size_t decrement(size_t capacity, size_t i);
static size_t distance(size_t capacity, size_t i, size_t j);
static void *key_in_slot(struct ccc_hhmap_ const *h, void const *slot);
static struct ccc_hhmap_elem_ *elem_in_slot(struct ccc_hhmap_ const *h,
                                            void const *slot);
static struct ccc_hhmap_elem_ *elem_at(struct ccc_hhmap_ const *h, size_t i);
static ccc_result maybe_resize(struct ccc_hhmap_ *h);
static struct ccc_handle_ find(struct ccc_hhmap_ const *h, void const *key,
                               uint64_t hash);
static ccc_handle insert_meta(struct ccc_hhmap_ *h, uint64_t hash, size_t i);
static uint64_t *hash_at(struct ccc_hhmap_ const *h, size_t i);
static uint64_t filter(struct ccc_hhmap_ const *h, void const *key);
static size_t next_prime(size_t n);
static void copy_to_slot(struct ccc_hhmap_ *h, void *slot_dst,
                         void const *user_src);

/*=========================   Interface    ==================================*/

void *
ccc_hhm_at(ccc_handle_hash_map const *const h, ccc_handle i)
{
    if (!h || !i)
    {
        return NULL;
    }
    return ccc_buf_at(&h->buf_, i);
}

bool
ccc_hhm_is_empty(ccc_handle_hash_map const *const h)
{
    if (unlikely(!h))
    {
        return true;
    }
    return !ccc_hhm_size(h);
}

bool
ccc_hhm_contains(ccc_handle_hash_map *const h, void const *const key)
{
    if (unlikely(!h || !key))
    {
        return 0;
    }
    return entry(h, key, filter(h, key)).stats_ == CCC_ENTRY_OCCUPIED;
}

size_t
ccc_hhm_size(ccc_handle_hash_map const *const h)
{
    if (unlikely(!h))
    {
        return 0;
    }
    size_t const size = ccc_buf_size(&h->buf_);
    return size ? size - num_swap_slots : 0;
}

ccc_hhmap_entry
ccc_hhm_entry(ccc_handle_hash_map *const h, void const *const key)
{
    if (unlikely(!h || !key))
    {
        return (ccc_hhmap_entry){{.entry_ = {.stats_ = CCC_ENTRY_INPUT_ERROR}}};
    }
    return (ccc_hhmap_entry){container_entry(h, key)};
}

ccc_handle
ccc_hhm_insert_entry(ccc_hhmap_entry const *const e, ccc_hhmap_elem *const elem)
{
    if (unlikely(!e || !elem))
    {
        return 0;
    }
    void *const user_struct = struct_base(e->impl_.h_, elem);
    if (e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        copy_to_slot(e->impl_.h_,
                     ccc_buf_at(&e->impl_.h_->buf_, e->impl_.entry_.i_),
                     struct_base(e->impl_.h_, elem));
        return e->impl_.entry_.i_;
    }
    if (e->impl_.entry_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return 0;
    }
    ccc_handle const ins
        = insert_meta(e->impl_.h_, e->impl_.hash_, e->impl_.entry_.i_);
    copy_to_slot(e->impl_.h_, ccc_buf_at(&e->impl_.h_->buf_, ins), user_struct);
    return ins;
}

ccc_handle
ccc_hhm_get_key_val(ccc_handle_hash_map *const h, void const *const key)
{
    if (unlikely(!h || !key || !ccc_buf_capacity(&h->buf_)))
    {
        return 0;
    }
    struct ccc_handle_ e = find(h, key, filter(h, key));
    if (e.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return e.i_;
    }
    return 0;
}

ccc_entry
ccc_hhm_remove_entry(ccc_hhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    if (e->impl_.entry_.stats_ != CCC_ENTRY_OCCUPIED)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_VACANT}};
    }
    erase_meta(e->impl_.h_, elem_at(e->impl_.h_, e->impl_.entry_.i_)->meta_i_);
    return (ccc_entry){{.stats_ = CCC_ENTRY_OCCUPIED}};
}

ccc_hhmap_entry *
ccc_hhm_and_modify(ccc_hhmap_entry *const e, ccc_update_fn *const fn)
{
    return (ccc_hhmap_entry *)and_modify(&e->impl_, fn);
}

ccc_hhmap_entry *
ccc_hhm_and_modify_aux(ccc_hhmap_entry *const e, ccc_update_fn *const fn,
                       void *const aux)
{
    if (unlikely(!e))
    {
        return NULL;
    }
    if (e->impl_.entry_.stats_ == CCC_ENTRY_OCCUPIED && fn)
    {
        fn((ccc_user_type){ccc_buf_at(&e->impl_.h_->buf_, e->impl_.entry_.i_),
                           aux});
    }
    return e;
}

ccc_entry
ccc_hhm_insert(ccc_handle_hash_map *const h, ccc_hhmap_elem *const out_handle)
{
    if (unlikely(!h || !out_handle))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    void *const user_return = struct_base(h, out_handle);
    void *const key = key_in_slot(h, user_return);
    struct ccc_hhash_entry_ ent = container_entry(h, key);
    if (ent.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        swap_user_data(h, ccc_buf_at(&h->buf_, ent.entry_.i_), user_return);
        return (ccc_entry){{.e_ = user_return, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (ent.entry_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    ccc_handle const ins = insert_meta(h, ent.hash_, ent.entry_.i_);
    copy_to_slot(h, ccc_buf_at(&h->buf_, ins), user_return);
    return (ccc_entry){
        {.e_ = ccc_buf_at(&h->buf_, ins), .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_hhm_try_insert(ccc_handle_hash_map *const h,
                   ccc_hhmap_elem *const key_val_handle)
{
    if (unlikely(!h || !key_val_handle))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    void *const user_base = struct_base(h, key_val_handle);
    struct ccc_hhash_entry_ ent = container_entry(h, key_in_slot(h, user_base));
    if (ent.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return (ccc_entry){{.e_ = ccc_buf_at(&h->buf_, ent.entry_.i_),
                            .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (ent.entry_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    ccc_handle const ins = insert_meta(h, ent.hash_, ent.entry_.i_);
    copy_to_slot(h, ccc_buf_at(&h->buf_, ins), user_base);
    return (ccc_entry){
        {.e_ = ccc_buf_at(&h->buf_, ins), .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_hhm_insert_or_assign(ccc_handle_hash_map *const h,
                         ccc_hhmap_elem *const key_val_handle)
{
    if (unlikely(!h || !key_val_handle))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    void *const user_base = struct_base(h, key_val_handle);
    struct ccc_hhash_entry_ ent = container_entry(h, key_in_slot(h, user_base));
    void *const ret = ccc_buf_at(&h->buf_, ent.entry_.i_);
    if (ent.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        copy_to_slot(h, ret, user_base);
        return (ccc_entry){{.e_ = ret, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (ent.entry_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    ccc_handle const ins = insert_meta(h, ent.hash_, ent.entry_.i_);
    copy_to_slot(h, ccc_buf_at(&h->buf_, ins), user_base);
    return (ccc_entry){
        {.e_ = ccc_buf_at(&h->buf_, ins), .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_hhm_remove(ccc_handle_hash_map *const h, ccc_hhmap_elem *const out_handle)
{
    if (unlikely(!h || !out_handle))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    void *const ret = struct_base(h, out_handle);
    void *const key = key_in_slot(h, ret);
    struct ccc_handle_ const ent = find(h, key, filter(h, key));
    if (ent.stats_ & CCC_ENTRY_OCCUPIED)
    {
        (void)memcpy(ret, ccc_buf_at(&h->buf_, ent.i_),
                     ccc_buf_elem_size(&h->buf_));
        erase_meta(h, elem_at(h, ent.i_)->meta_i_);
        return (ccc_entry){{.e_ = ret, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_handle
ccc_hhm_or_insert(ccc_hhmap_entry const *const e, ccc_hhmap_elem *const elem)
{
    if (unlikely(!e || !elem))
    {
        return 0;
    }
    if (e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return e->impl_.entry_.i_;
    }
    if (e->impl_.entry_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return 0;
    }
    void *user_struct = struct_base(e->impl_.h_, elem);
    ccc_handle const ins
        = insert_meta(e->impl_.h_, e->impl_.hash_, e->impl_.entry_.i_);
    copy_to_slot(e->impl_.h_, ccc_buf_at(&e->impl_.h_->buf_, ins), user_struct);
    return ins;
}

ccc_handle
ccc_hhm_unwrap(ccc_hhmap_entry const *const e)
{
    if (unlikely(!e) || !(e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED))
    {
        return 0;
    }
    return e->impl_.entry_.i_;
}

bool
ccc_hhm_occupied(ccc_hhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return false;
    }
    return e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED;
}

bool
ccc_hhm_insert_error(ccc_hhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return false;
    }
    return e->impl_.entry_.stats_ & CCC_ENTRY_INSERT_ERROR;
}

ccc_entry_status
ccc_hhm_entry_status(ccc_hhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return CCC_ENTRY_INPUT_ERROR;
    }
    return e->impl_.entry_.stats_;
}

ccc_handle
ccc_hhm_begin(ccc_handle_hash_map const *const h)
{
    if (unlikely(!h || ccc_buf_is_empty(&h->buf_)))
    {
        return 0;
    }
    size_t iter = 0;
    for (; iter < ccc_buf_capacity(&h->buf_)
           && elem_at(h, iter)->meta_.hash_ == CCC_HHM_EMPTY;
         ++iter)
    {}
    if (iter >= ccc_buf_capacity(&h->buf_))
    {
        return 0;
    }
    return elem_at(h, iter)->meta_.slot_i_;
}

ccc_handle
ccc_hhm_next(ccc_handle_hash_map const *const h, ccc_handle iter)
{
    if (unlikely(!h))
    {
        return 0;
    }
    if (!iter)
    {
        return 0;
    }
    iter = elem_at(h, iter)->meta_i_ + 1;
    for (; iter < ccc_buf_capacity(&h->buf_)
           && elem_at(h, iter)->meta_.hash_ == CCC_HHM_EMPTY;
         ++iter)
    {}
    if (iter >= ccc_buf_capacity(&h->buf_))
    {
        return 0;
    }
    return elem_at(h, iter)->meta_.slot_i_;
}

ccc_handle
ccc_hhm_end(ccc_handle_hash_map const *const)
{
    return 0;
}

size_t
ccc_hhm_next_prime(size_t const n)
{
    return next_prime(n);
}

ccc_result
ccc_hhm_copy(ccc_handle_hash_map *const dst,
             ccc_handle_hash_map const *const src, ccc_alloc_fn *const fn)
{
    if (!dst || !src || (dst->buf_.capacity_ < src->buf_.capacity_ && !fn))
    {
        return CCC_INPUT_ERR;
    }
    /* Copy everything so we don't worry about staying in sync with future
       changes to buf container. But we have to give back original destination
       memory in case it has already been allocated. Alloc will remain the
       same as in dst initialization because that controls permission. */
    void *const dst_mem = dst->buf_.mem_;
    size_t const dst_cap = dst->buf_.capacity_;
    ccc_alloc_fn *const dst_alloc = dst->buf_.alloc_;
    *dst = *src;
    dst->buf_.mem_ = dst_mem;
    dst->buf_.capacity_ = dst_cap;
    dst->buf_.alloc_ = dst_alloc;
    if (dst->buf_.capacity_ < src->buf_.capacity_)
    {
        ccc_result resize_res
            = ccc_buf_alloc(&dst->buf_, src->buf_.capacity_, fn);
        if (resize_res != CCC_OK)
        {
            return resize_res;
        }
        dst->buf_.capacity_ = src->buf_.capacity_;
    }
    (void)memcpy(dst->buf_.mem_, src->buf_.mem_,
                 src->buf_.capacity_ * src->buf_.elem_sz_);
    return CCC_OK;
}

ccc_result
ccc_hhm_clear(ccc_handle_hash_map *const h, ccc_destructor_fn *const fn)
{
    if (unlikely(!h))
    {
        return CCC_INPUT_ERR;
    }
    if (!fn)
    {
        h->buf_.sz_ = 0;
        return CCC_OK;
    }
    for (void *slot = ccc_buf_begin(&h->buf_);
         slot != ccc_buf_capacity_end(&h->buf_);
         slot = ccc_buf_next(&h->buf_, slot))
    {
        struct ccc_hhmap_elem_ *const e = elem_in_slot(h, slot);
        if (e->meta_.hash_ != CCC_HHM_EMPTY)
        {
            fn((ccc_user_type){.user_type = ccc_buf_at(&h->buf_, e->meta_i_),
                               .aux = h->buf_.aux_});
        }
    }
    return CCC_OK;
}

ccc_result
ccc_hhm_clear_and_free(ccc_handle_hash_map *const h,
                       ccc_destructor_fn *const fn)
{
    if (unlikely(!h))
    {
        return CCC_INPUT_ERR;
    }
    if (!fn)
    {
        h->buf_.sz_ = 0;
        return ccc_buf_alloc(&h->buf_, 0, h->buf_.alloc_);
    }
    for (void *slot = ccc_buf_begin(&h->buf_);
         slot != ccc_buf_capacity_end(&h->buf_);
         slot = ccc_buf_next(&h->buf_, slot))
    {
        struct ccc_hhmap_elem_ *const e = elem_in_slot(h, slot);
        if (e->meta_.hash_ != CCC_HHM_EMPTY)
        {
            fn((ccc_user_type){.user_type = ccc_buf_at(&h->buf_, e->meta_i_),
                               .aux = h->buf_.aux_});
        }
    }
    return ccc_buf_alloc(&h->buf_, 0, h->buf_.alloc_);
}

size_t
ccc_hhm_capacity(ccc_handle_hash_map const *const h)
{
    if (unlikely(!h))
    {
        return 0;
    }
    return ccc_buf_capacity(&h->buf_);
}

void *
ccc_hhm_data(ccc_handle_hash_map const *const h)
{
    return h ? ccc_buf_begin(&h->buf_) : NULL;
}

bool
ccc_hhm_validate(ccc_handle_hash_map const *const h)
{
    if (!h || !h->eq_fn_ || !h->hash_fn_)
    {
        return false;
    }
    /* Not yet initialized, pass for now. */
    if (!ccc_buf_size(&h->buf_))
    {
        return true;
    }
    size_t empties = 0;
    size_t occupied = 0;
    for (size_t i = 0; i < ccc_buf_capacity(&h->buf_); ++i)
    {
        struct ccc_hhmap_elem_ const *const e = elem_at(h, i);
        if (i >= num_swap_slots && elem_at(h, e->meta_.slot_i_)->meta_i_ != i)
        {
            return false;
        }
        if (e->meta_.hash_ == CCC_HHM_EMPTY)
        {
            ++empties;
        }
        else
        {
            ++occupied;
        }
        if (e->meta_.hash_ != CCC_HHM_EMPTY && !valid_distance_from_home(h, i))
        {
            return false;
        }
    }
    return occupied == ccc_hhm_size(h)
           && empties == (ccc_buf_capacity(&h->buf_) - occupied);
}

static bool
valid_distance_from_home(struct ccc_hhmap_ const *const h, size_t const slot)
{
    size_t const cap = ccc_buf_capacity(&h->buf_);
    uint64_t const hash = elem_at(h, slot)->meta_.hash_;
    size_t const home = to_i(cap, hash);
    size_t const end = decrement(cap, home);
    for (size_t i = slot, distance_to_home = distance(cap, i, to_i(cap, hash));
         i != end; --distance_to_home, i = decrement(cap, i))
    {
        uint64_t const cur_hash = *hash_at(h, i);
        /* The only reason an element is not home is because it has been
           moved away to help another element be closer to its home. This
           would break the purpose of doing that. Upon erase everyone needs
           to shuffle closer to home. */
        if (cur_hash == CCC_HHM_EMPTY)
        {
            return false;
        }
        /* This shouldn't happen either. The whole point of Robin Hood is
           taking from the close and giving to the far. If this happens
           we have made our algorithm greedy not altruistic. */
        if (distance_to_home > distance(cap, i, to_i(cap, cur_hash)))
        {
            return false;
        }
    }
    return true;
}

/*=======================   Private Interface   =============================*/

struct ccc_hhash_entry_
ccc_impl_hhm_entry(struct ccc_hhmap_ *const h, void const *const key)
{
    return container_entry(h, key);
}

struct ccc_hhash_entry_ *
ccc_impl_hhm_and_modify(struct ccc_hhash_entry_ *const e,
                        ccc_update_fn *const fn)
{
    return and_modify(e, fn);
}

struct ccc_handle_
ccc_impl_hhm_find(struct ccc_hhmap_ const *const h, void const *const key,
                  uint64_t const hash)
{
    return find(h, key, hash);
}

ccc_handle
ccc_impl_hhm_insert_meta(struct ccc_hhmap_ *const h, uint64_t const hash,
                         size_t cur_i)
{
    return insert_meta(h, hash, cur_i);
}

ccc_result
ccc_impl_hhm_maybe_resize(struct ccc_hhmap_ *const h)
{
    return maybe_resize(h);
}

struct ccc_hhmap_elem_ *
ccc_impl_hhm_in_slot(struct ccc_hhmap_ const *const h, void const *const slot)
{
    return elem_in_slot(h, slot);
}

void *
ccc_impl_hhm_key_at(struct ccc_hhmap_ const *h, size_t const i)
{
    return (char *)ccc_buf_at(&h->buf_, i) + h->key_offset_;
}

size_t
ccc_impl_hhm_distance(size_t const capacity, size_t const i, size_t const j)
{
    return distance(capacity, i, j);
}

size_t
ccc_impl_hhm_increment(size_t const capacity, size_t const i)
{
    return increment(capacity, i);
}

void *
ccc_impl_hhm_base(struct ccc_hhmap_ const *const h)
{
    return ccc_buf_begin(&h->buf_);
}

uint64_t *
ccc_impl_hhm_hash_at(struct ccc_hhmap_ const *const h, size_t const i)
{
    return hash_at(h, i);
}

uint64_t
ccc_impl_hhm_filter(struct ccc_hhmap_ const *const h, void const *const key)
{
    return filter(h, key);
}

void
ccc_impl_hhm_copy_to_slot(struct ccc_hhmap_ *const h, void *const slot_dst,
                          void const *const slot_src)
{
    copy_to_slot(h, slot_dst, slot_src);
}

struct ccc_hhmap_elem_ *
ccc_impl_hhm_elem_at(struct ccc_hhmap_ const *const h, size_t const i)
{
    return elem_at(h, i);
}

/*=======================     Static Helpers    =============================*/

static inline struct ccc_handle_
entry(struct ccc_hhmap_ *const h, void const *const key, uint64_t const hash)
{
    uint8_t future_insert_error = 0;
    if (maybe_resize(h) != CCC_OK)
    {
        future_insert_error = CCC_ENTRY_INSERT_ERROR;
    }
    struct ccc_handle_ res = find(h, key, hash);
    res.stats_ |= future_insert_error;
    return res;
}

static inline struct ccc_handle_
find(struct ccc_hhmap_ const *const h, void const *const key,
     uint64_t const hash)
{
    size_t const cap = ccc_buf_capacity(&h->buf_);
    /* A few sanity checks. The load factor should be managed a full table is
       never allowed even under no allocation permission because that could
       lead to an infinite loop and illustrates a degenerate table anyway. */
    if (unlikely(!cap))
    {
        return (struct ccc_handle_){.i_ = 0, .stats_ = CCC_ENTRY_VACANT};
    }
    if (unlikely(ccc_buf_size(&h->buf_) >= ccc_buf_capacity(&h->buf_)))
    {
        return (struct ccc_handle_){.i_ = 0, .stats_ = CCC_ENTRY_INPUT_ERROR};
    }
    size_t i = to_i(cap, hash);
    size_t dist = 0;
    do
    {
        struct ccc_hhmap_elem_ const *const e = elem_at(h, i);
        if (e->meta_.hash_ == CCC_HHM_EMPTY
            || dist > distance(cap, i, to_i(cap, e->meta_.hash_)))
        {
            return (struct ccc_handle_){.i_ = e->meta_.slot_i_,
                                        .stats_ = CCC_ENTRY_VACANT
                                                  | CCC_ENTRY_NO_UNWRAP};
        }
        if (hash == e->meta_.hash_
            && h->eq_fn_((ccc_key_cmp){.key_lhs = key,
                                       .user_type_rhs
                                       = ccc_buf_at(&h->buf_, e->meta_.slot_i_),
                                       .aux = h->buf_.aux_}))
        {
            return (struct ccc_handle_){.i_ = e->meta_.slot_i_,
                                        .stats_ = CCC_ENTRY_OCCUPIED};
        }
        ++dist;
        i = increment(cap, i);
    } while (true);
}

/* Assumes that element to be inserted does not already exist in the table.
   Assumes that the table has room for another insertion. Unexpected results
   may occur if these assumptions are not accommodated. After managing metadata
   entries in the table for Round Robin, returns the slot the data may be copied
   to for this insert. Be sure not to overwrite the hhmap element field of the
   returned slot as it holds important metadata for another slot elsewhere. */
static inline ccc_handle
insert_meta(struct ccc_hhmap_ *const h, uint64_t const hash, size_t i)
{
    size_t const cap = ccc_buf_capacity(&h->buf_);
    struct ccc_hhmap_elem_ *const floater = elem_at(h, 1);
    floater->meta_.hash_ = hash;
    floater->meta_.slot_i_ = i;
    size_t const e_meta = elem_at(h, i)->meta_i_;
    floater->meta_i_ = e_meta;
    size_t dist = distance(cap, e_meta, to_i(cap, hash));
    /* The slot we are given is where the actual data will eventually go. We
       now need to run the robin hood algo on just the metadata entries. */
    i = e_meta;
    do
    {
        struct ccc_hhmap_elem_ *const elem = elem_at(h, i);
        ccc_handle const user_data_slot = elem->meta_.slot_i_;
        if (elem->meta_.hash_ == CCC_HHM_EMPTY)
        {
            elem_at(h, user_data_slot)->meta_i_ = e_meta;
            elem_at(h, e_meta)->meta_.slot_i_ = user_data_slot;
            elem->meta_ = floater->meta_;
            elem_at(h, floater->meta_.slot_i_)->meta_i_ = i;
            (void)ccc_buf_size_plus(&h->buf_, 1);
            *hash_at(h, 0) = CCC_HHM_EMPTY;
            *hash_at(h, 1) = CCC_HHM_EMPTY;
            return user_data_slot;
        }
        size_t const slot_dist = distance(cap, i, to_i(cap, elem->meta_.hash_));
        if (dist > slot_dist)
        {
            elem_at(h, floater->meta_.slot_i_)->meta_i_ = i;
            swap_meta_data(h, floater, elem);
            dist = slot_dist;
        }
        i = increment(cap, i);
        ++dist;
    } while (true);
}

/* Backshift deletion is important in for this table because it may not be able
   to allocate. This prevents the need for tombstones which would hurt table
   quality quickly if we can't resize. */
static void
erase_meta(struct ccc_hhmap_ *const h, size_t e)
{
    *hash_at(h, e) = CCC_HHM_EMPTY;
    size_t const cap = ccc_buf_capacity(&h->buf_);
    size_t i = e;
    size_t next = increment(cap, i);
    do
    {
        struct ccc_hhmap_elem_ *const next_elem = elem_at(h, next);
        if (next_elem->meta_.hash_ == CCC_HHM_EMPTY
            || !distance(cap, next, to_i(cap, next_elem->meta_.hash_)))
        {
            *hash_at(h, 0) = CCC_HHM_EMPTY;
            (void)ccc_buf_size_minus(&h->buf_, 1);
            return;
        }
        struct ccc_hhmap_elem_ *const i_elem = elem_at(h, i);
        swap_meta_data(h, i_elem, next_elem);
        elem_at(h, i_elem->meta_.slot_i_)->meta_i_ = i;
        i = next;
        next = increment(cap, next);
    } while (true);
}

static inline struct ccc_hhash_entry_
container_entry(struct ccc_hhmap_ *const h, void const *const key)
{
    uint64_t const hash = filter(h, key);
    return (struct ccc_hhash_entry_){
        .h_ = h,
        .hash_ = hash,
        .entry_ = entry(h, key, hash),
    };
}

static inline struct ccc_hhash_entry_ *
and_modify(struct ccc_hhash_entry_ *const e, ccc_update_fn *const fn)
{
    if (e->entry_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        fn((ccc_user_type){ccc_buf_at(&e->h_->buf_, e->entry_.i_), NULL});
    }
    return e;
}

static inline ccc_result
maybe_resize(struct ccc_hhmap_ *const h)
{
    if (ccc_buf_capacity(&h->buf_) && ccc_buf_size(&h->buf_) < num_swap_slots)
    {

        for (size_t i = 0; i < ccc_buf_capacity(&h->buf_); ++i)
        {
            (void)memset(ccc_buf_at(&h->buf_, i), 0,
                         ccc_buf_elem_size(&h->buf_));
            struct ccc_hhmap_elem_ *const e = elem_at(h, i);
            e->meta_.slot_i_ = i;
            e->meta_i_ = i;
        }
        (void)ccc_buf_size_set(&h->buf_, num_swap_slots);
    }
    if (ccc_buf_capacity(&h->buf_)
        && (double)(ccc_buf_size(&h->buf_)) / (double)ccc_buf_capacity(&h->buf_)
               <= load_factor)
    {
        return CCC_OK;
    }
    if (!h->buf_.alloc_)
    {
        return CCC_NO_ALLOC;
    }
    struct ccc_hhmap_ new_hash = *h;
    new_hash.buf_.sz_ = 0;
    new_hash.buf_.capacity_ = new_hash.buf_.capacity_
                                  ? next_prime(ccc_buf_size(&h->buf_) * 2)
                                  : primes[0];
    if (new_hash.buf_.capacity_ <= h->buf_.capacity_)
    {
        return CCC_MEM_ERR;
    }
    new_hash.buf_.mem_ = new_hash.buf_.alloc_(
        NULL, ccc_buf_elem_size(&h->buf_) * new_hash.buf_.capacity_,
        h->buf_.aux_);
    if (!new_hash.buf_.mem_)
    {
        return CCC_MEM_ERR;
    }
    for (size_t i = 0; i < ccc_buf_capacity(&new_hash.buf_); ++i)
    {
        (void)memset(ccc_buf_at(&new_hash.buf_, i), 0,
                     ccc_buf_elem_size(&new_hash.buf_));
        struct ccc_hhmap_elem_ *const e = elem_at(&new_hash, i);
        e->meta_.slot_i_ = i;
        e->meta_i_ = i;
    }
    (void)ccc_buf_size_set(&new_hash.buf_, num_swap_slots);
    for (void *slot = ccc_buf_begin(&h->buf_);
         slot != ccc_buf_capacity_end(&h->buf_);
         slot = ccc_buf_next(&h->buf_, slot))
    {
        struct ccc_hhmap_elem_ const *const e = elem_in_slot(h, slot);
        if (e->meta_.hash_ != CCC_HHM_EMPTY)
        {
            struct ccc_handle_ const new_ent
                = find(&new_hash, key_in_slot(h, slot), e->meta_.hash_);
            ccc_handle const ins
                = insert_meta(&new_hash, e->meta_.hash_, new_ent.i_);
            copy_to_slot(&new_hash, ccc_buf_at(&new_hash.buf_, ins), slot);
        }
    }
    if (ccc_buf_alloc(&h->buf_, 0, h->buf_.alloc_) != CCC_OK)
    {
        *h = new_hash;
        return CCC_MEM_ERR;
    }
    *h = new_hash;
    return CCC_OK;
}

static inline void
copy_to_slot(struct ccc_hhmap_ *const h, void *const slot_dst,
             void const *const user_src)
{
    struct ccc_hhmap_elem_ *const elem = elem_in_slot(h, slot_dst);
    size_t surrounding_bytes = (char *)elem - (char *)slot_dst;
    if (surrounding_bytes)
    {
        (void)memcpy(slot_dst, user_src, surrounding_bytes);
    }
    char *const remain_start
        = (char *)elem_in_slot(h, user_src) + sizeof(struct ccc_hhmap_elem_);
    surrounding_bytes = (char *)((char *)user_src + ccc_buf_elem_size(&h->buf_))
                        - remain_start;
    if (surrounding_bytes)
    {
        (void)memcpy((char *)elem + sizeof(struct ccc_hhmap_elem_),
                     remain_start, surrounding_bytes);
    }
}

static inline size_t
next_prime(size_t const n)
{
    for (size_t i = 0; i < PRIMES_SIZE; ++i)
    {
        if (primes[i] > n)
        {
            return primes[i];
        }
    }
    return 0;
}

static inline uint64_t *
hash_at(struct ccc_hhmap_ const *const h, size_t const i)
{
    return &((struct ccc_hhmap_elem_ *)((char *)ccc_buf_at(&h->buf_, i)
                                        + h->hash_elem_offset_))
                ->meta_.hash_;
}

static inline uint64_t
filter(struct ccc_hhmap_ const *const h, void const *const key)
{
    uint64_t const hash
        = h->hash_fn_((ccc_user_key){.user_key = key, .aux = h->buf_.aux_});
    return hash == CCC_HHM_EMPTY ? hash + 1 : hash;
}

static inline struct ccc_hhmap_elem_ *
elem_in_slot(struct ccc_hhmap_ const *const h, void const *const slot)
{
    return (struct ccc_hhmap_elem_ *)((char *)slot + h->hash_elem_offset_);
}

static inline struct ccc_hhmap_elem_ *
elem_at(struct ccc_hhmap_ const *const h, size_t const i)
{
    return (struct ccc_hhmap_elem_ *)((char *)ccc_buf_at(&h->buf_, i)
                                      + h->hash_elem_offset_);
}

static inline void *
key_in_slot(struct ccc_hhmap_ const *const h, void const *const slot)
{
    return (char *)slot + h->key_offset_;
}

static inline size_t
distance(size_t const capacity, size_t const i, size_t const j)
{
    return i < j ? (capacity - j) + i - num_swap_slots : i - j;
}

static inline size_t
increment(size_t const capacity, size_t const i)
{
    return (i + 1) >= capacity ? last_swap_slot + 1 : i + 1;
}

static inline size_t
decrement(size_t const capacity, size_t const i)
{
    return i <= num_swap_slots ? capacity - 1 : i - 1;
}

static inline void *
struct_base(struct ccc_hhmap_ const *const h,
            struct ccc_hhmap_elem_ const *const e)
{
    return ((char *)&e->meta_.hash_) - h->hash_elem_offset_;
}

static inline void
swap_user_data(struct ccc_hhmap_ *const h, void *const a, void *const b)
{
    if (a == b)
    {
        return;
    }
    void *const tmp = ccc_buf_at(&h->buf_, 0);
    (void)memcpy(tmp, a, ccc_buf_elem_size(&h->buf_));
    copy_to_slot(h, a, b);
    copy_to_slot(h, b, tmp);
}

static inline void
swap_meta_data(struct ccc_hhmap_ *const h, struct ccc_hhmap_elem_ *const a,
               struct ccc_hhmap_elem_ *const b)
{
    if (a == b)
    {
        return;
    }
    struct ccc_hhmap_elem_ *const tmp
        = elem_in_slot(h, ccc_buf_at(&h->buf_, 0));
    tmp->meta_ = a->meta_;
    a->meta_ = b->meta_;
    b->meta_ = tmp->meta_;
}

/** This function converts a hash to its home index in the table. Because this
operation is in the hot path of all table functions and Robin Hood relies on
home slot distance calculations, this function tries to use Daniel Lemire's
fastrange library modulo calculation alternative:
    https://github.com/lemire/fastrange
Such a technique is helpful because the prime table capacities used to mitigate
primary clustering of Robin Hood hash tables make efficient modulo calculations
more difficult than power of two table capacities. The appropriate license for
this technique is included immediately following this function and only applies
to this function in this source file. */
static inline size_t
to_i(size_t const capacity, uint64_t hash)
{
#ifdef __SIZEOF_INT128__
    hash = (uint64_t)(((__uint128_t)hash * (__uint128_t)capacity) >> 64);
#else
    hash %= capacity;
#endif
    return hash <= last_swap_slot ? last_swap_slot + 1 : hash;
}

/** The following Apache License applies only to the above function. This
function uses one line from Daniel Lemire's fastrange library to compute the
modulo of a number range with multiplication and shifts rather than  modulo
division. Specifically, the 128 bit widening is from the fastrange library which
can be found here: https://github.com/lemire/fastrange

Below is the required license for the code in the above function. Lemire's
copyright applies to the integer widening line of code as he copyrights the
source file from which it came with his name and the year. The notice in
https://github.com/lemire/fastrange/fastrange.h appears as follows:

```
Fair maps to intervals without division.
Reference :
http://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/

(c) Daniel Lemire
Apache License 2.0
```

The license for the cited repository is included below for completeness.

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
