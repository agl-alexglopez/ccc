/** This implementation is a Robin Hood hash table with shift delete. It caches
the hash values for efficient distance calculations, resizing, and faster
comparison before being forced to call user comparison callback. It also
implements backshift deletions. At first I thought this was not a good starter
implementation but it actually has some nice features for its required use case

    - Robin Hood hash tables do not use tombstones. They suffer from primary
      clustering under linear probing, but deletions do allow us to heal the
      table in some cases. We also try to mitigate primary clustering by
      choosing prime number table sizes. This is very beneficial when the hash
      table is not initialized with allocation permission. Long term use of a
      fixed size table is a use case we must support.
    - Strict-aliasing is an issue for more complex table schemes. Here we
      require an intrusive element in the user type meaning they can pass us
      a block of their type for us to manage as a hash table. If we were to
      attempt to implement a small metadata array at the start and the user
      type after that--with no intrusive elements--we would need to likely
      expose some sort of new type to the user for initialization to help
      them avoid passing us just some generic char buffer they have on the
      data segment, stack, or heap; that would be a strict-aliasing violation.
    - Tailoring callback resistance. Right now, the table caches the full hash
      for the user because it is needed to accurately calculate distances to
      the home slot for the Robin Hood swapping purposes. This also helps
      prevent calling the user comparison callback which is nice; in this table
      if the user hash is good, they could hash strings and still operate at
      high efficiency with likely few collisions in the find op. This is a
      higher space overhead choice, however.

Those are some considerations I can think of now. When attempting to improve
this table we need to consider these points.

    - The hash table must support a non-allocating mode without the ability to
      resize to correct from any type of tombstone overload; an in-place
      compaction and rehash would be the only option (not sure if possible).
    - It must work with the user choosing where the memory comes from for the
      table. This is the other side of non-allocating mode. The user should
      be able to give us memory coming from the stack, heap, or data segment.
    - No strict-aliasing violations. I want this library to be useful if the
      user wants to write correct C code meaning it cannot violate this.

Overall, this means any new design needs slightly more consideration than I
first thought. However, improvements can still be made. */
#include "flat_hash_map.h"
#include "buffer.h"
#include "impl/impl_flat_hash_map.h"
#include "impl/impl_types.h"
#include "types.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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

static void erase(struct ccc_fhmap_ *, void *);
static void swap(char tmp[], void *, void *, size_t);
static void *struct_base(struct ccc_fhmap_ const *,
                         struct ccc_fhmap_elem_ const *);
static struct ccc_ent_ entry(struct ccc_fhmap_ *, void const *key,
                             uint64_t hash);
static struct ccc_fhash_entry_ container_entry(struct ccc_fhmap_ *h,
                                               void const *key);
static struct ccc_fhash_entry_ *and_modify(struct ccc_fhash_entry_ *e,
                                           ccc_update_fn *fn);
static bool valid_distance_from_home(struct ccc_fhmap_ const *,
                                     void const *slot);
static size_t to_i(size_t capacity, uint64_t hash);
static size_t increment(size_t capacity, size_t i);
static size_t decrement(size_t capacity, size_t i);
static size_t distance(size_t capacity, size_t i, size_t j);
static void *key_in_slot(struct ccc_fhmap_ const *h, void const *slot);
static struct ccc_fhmap_elem_ *elem_in_slot(struct ccc_fhmap_ const *h,
                                            void const *slot);
static ccc_result maybe_resize(struct ccc_fhmap_ *h);
static struct ccc_ent_ find(struct ccc_fhmap_ const *h, void const *key,
                            uint64_t hash);
static void insert(struct ccc_fhmap_ *h, void const *e, uint64_t hash,
                   size_t i);
static uint64_t *hash_at(struct ccc_fhmap_ const *h, size_t i);
static uint64_t filter(struct ccc_fhmap_ const *h, void const *key);
static size_t next_prime(size_t n);

/*=========================   Interface    ==================================*/

bool
ccc_fhm_is_empty(ccc_flat_hash_map const *const h)
{
    if (unlikely(!h))
    {
        return true;
    }
    return !ccc_buf_size(&h->buf_);
}

bool
ccc_fhm_contains(ccc_flat_hash_map *const h, void const *const key)
{
    if (unlikely(!h || !key))
    {
        return false;
    }
    return entry(h, key, filter(h, key)).stats_ & CCC_ENTRY_OCCUPIED;
}

size_t
ccc_fhm_size(ccc_flat_hash_map const *const h)
{
    if (unlikely(!h))
    {
        return 0;
    }
    return ccc_buf_size(&h->buf_);
}

ccc_fhmap_entry
ccc_fhm_entry(ccc_flat_hash_map *h, void const *const key)
{
    if (unlikely(!h || !key))
    {
        return (ccc_fhmap_entry){{.entry_ = {.stats_ = CCC_ENTRY_INPUT_ERROR}}};
    }
    return (ccc_fhmap_entry){container_entry(h, key)};
}

void *
ccc_fhm_insert_entry(ccc_fhmap_entry const *const e, ccc_fhmap_elem *const elem)
{
    if (unlikely(!e || !elem))
    {
        return NULL;
    }
    void *const user_struct = struct_base(e->impl_.h_, elem);
    if (e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        elem->hash_ = e->impl_.hash_;
        (void)memcpy(e->impl_.entry_.e_, user_struct,
                     ccc_buf_elem_size(&e->impl_.h_->buf_));
        return e->impl_.entry_.e_;
    }
    if (e->impl_.entry_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return NULL;
    }
    insert(e->impl_.h_, user_struct, e->impl_.hash_,
           ccc_buf_i(&e->impl_.h_->buf_, e->impl_.entry_.e_));
    return e->impl_.entry_.e_;
}

void *
ccc_fhm_get_key_val(ccc_flat_hash_map *const h, void const *const key)
{
    if (unlikely(!h || !key || !ccc_buf_capacity(&h->buf_)))
    {
        return NULL;
    }
    struct ccc_ent_ e = find(h, key, filter(h, key));
    if (e.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return e.e_;
    }
    return NULL;
}

ccc_entry
ccc_fhm_remove_entry(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    if (e->impl_.entry_.stats_ != CCC_ENTRY_OCCUPIED)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_VACANT}};
    }
    erase(e->impl_.h_, e->impl_.entry_.e_);
    return (ccc_entry){{.stats_ = CCC_ENTRY_OCCUPIED}};
}

ccc_fhmap_entry *
ccc_fhm_and_modify(ccc_fhmap_entry *const e, ccc_update_fn *const fn)
{
    return (ccc_fhmap_entry *)and_modify(&e->impl_, fn);
}

ccc_fhmap_entry *
ccc_fhm_and_modify_aux(ccc_fhmap_entry *const e, ccc_update_fn *const fn,
                       void *aux)
{
    if (unlikely(!e))
    {
        return NULL;
    }
    if (e->impl_.entry_.stats_ == CCC_ENTRY_OCCUPIED && fn)
    {
        fn((ccc_user_type){e->impl_.entry_.e_, aux});
    }
    return e;
}

ccc_entry
ccc_fhm_insert(ccc_flat_hash_map *h, ccc_fhmap_elem *const out_handle)
{
    if (unlikely(!h || !out_handle))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    void *const user_return = struct_base(h, out_handle);
    void *const key = key_in_slot(h, user_return);
    size_t const user_struct_size = ccc_buf_elem_size(&h->buf_);
    struct ccc_fhash_entry_ ent = container_entry(h, key);
    if (ent.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        out_handle->hash_ = ent.hash_;
        void *const tmp = ccc_buf_at(&h->buf_, 0);
        swap(tmp, ent.entry_.e_, user_return, user_struct_size);
        *hash_at(h, 0) = CCC_FHM_EMPTY;
        return (ccc_entry){{.e_ = user_return, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (ent.entry_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    insert(h, user_return, ent.hash_, ccc_buf_i(&h->buf_, ent.entry_.e_));
    return (ccc_entry){{.stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_fhm_try_insert(ccc_flat_hash_map *const h,
                   ccc_fhmap_elem *const key_val_handle)
{
    if (unlikely(!h || !key_val_handle))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    void *const user_base = struct_base(h, key_val_handle);
    struct ccc_fhash_entry_ ent = container_entry(h, key_in_slot(h, user_base));
    if (ent.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return (ccc_entry){{.e_ = ent.entry_.e_, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (ent.entry_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    insert(h, user_base, ent.hash_, ccc_buf_i(&h->buf_, ent.entry_.e_));
    return (ccc_entry){{.e_ = ent.entry_.e_, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_fhm_insert_or_assign(ccc_flat_hash_map *h, ccc_fhmap_elem *key_val_handle)
{
    if (unlikely(!h || !key_val_handle))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    void *const user_base = struct_base(h, key_val_handle);
    struct ccc_fhash_entry_ ent = container_entry(h, key_in_slot(h, user_base));
    if (ent.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        key_val_handle->hash_ = ent.hash_;
        (void)memcpy(ent.entry_.e_, user_base, ccc_buf_elem_size(&h->buf_));
        return (ccc_entry){{.e_ = ent.entry_.e_, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (ent.entry_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    insert(h, user_base, ent.hash_, ccc_buf_i(&h->buf_, ent.entry_.e_));
    return (ccc_entry){{.e_ = ent.entry_.e_, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_fhm_remove(ccc_flat_hash_map *const h, ccc_fhmap_elem *const out_handle)
{
    if (unlikely(!h || !out_handle))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    void *const ret = struct_base(h, out_handle);
    void *const key = key_in_slot(h, ret);
    struct ccc_ent_ const ent = find(h, key, filter(h, key));
    if (ent.stats_ & CCC_ENTRY_OCCUPIED)
    {
        (void)memcpy(ret, ent.e_, ccc_buf_elem_size(&h->buf_));
        erase(h, ent.e_);
        return (ccc_entry){{.e_ = ret, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

void *
ccc_fhm_or_insert(ccc_fhmap_entry const *const e, ccc_fhmap_elem *const elem)
{
    if (unlikely(!e || !elem))
    {
        return NULL;
    }
    if (e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return e->impl_.entry_.e_;
    }
    if (e->impl_.entry_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return NULL;
    }
    void *user_struct = struct_base(e->impl_.h_, elem);
    elem->hash_ = e->impl_.hash_;
    insert(e->impl_.h_, user_struct, elem->hash_,
           ccc_buf_i(&e->impl_.h_->buf_, e->impl_.entry_.e_));
    return e->impl_.entry_.e_;
}

void *
ccc_fhm_unwrap(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e) || !(e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED))
    {
        return NULL;
    }
    return e->impl_.entry_.e_;
}

bool
ccc_fhm_occupied(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return false;
    }
    return e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED;
}

bool
ccc_fhm_insert_error(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return false;
    }
    return e->impl_.entry_.stats_ & CCC_ENTRY_INSERT_ERROR;
}

ccc_entry_status
ccc_fhm_entry_status(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return CCC_ENTRY_INPUT_ERROR;
    }
    return e->impl_.entry_.stats_;
}

void *
ccc_fhm_begin(ccc_flat_hash_map const *const h)
{
    if (unlikely(!h || ccc_buf_is_empty(&h->buf_)))
    {
        return NULL;
    }
    void *iter = ccc_buf_begin(&h->buf_);
    for (; iter != ccc_buf_capacity_end(&h->buf_)
           && elem_in_slot(h, iter)->hash_ == CCC_FHM_EMPTY;
         iter = ccc_buf_next(&h->buf_, iter))
    {}
    return iter == ccc_buf_capacity_end(&h->buf_) ? NULL : iter;
}

void *
ccc_fhm_next(ccc_flat_hash_map const *const h, ccc_fhmap_elem const *iter)
{
    if (unlikely(!h))
    {
        return NULL;
    }
    void *i = struct_base(h, iter);
    for (i = ccc_buf_next(&h->buf_, i);
         i != ccc_buf_capacity_end(&h->buf_)
         && elem_in_slot(h, i)->hash_ == CCC_FHM_EMPTY;
         i = ccc_buf_next(&h->buf_, i))
    {}
    return i == ccc_buf_capacity_end(&h->buf_) ? NULL : i;
}

void *
ccc_fhm_end(ccc_flat_hash_map const *const)
{
    return NULL;
}

size_t
ccc_fhm_next_prime(size_t n)
{
    return next_prime(n);
}

ccc_result
ccc_fhm_clear(ccc_flat_hash_map *const h, ccc_destructor_fn *const fn)
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
        if (elem_in_slot(h, slot)->hash_ != CCC_FHM_EMPTY)
        {
            fn((ccc_user_type){.user_type = slot, .aux = h->buf_.aux_});
        }
    }
    return CCC_OK;
}

ccc_result
ccc_fhm_clear_and_free(ccc_flat_hash_map *const h, ccc_destructor_fn *const fn)
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
        if (elem_in_slot(h, slot)->hash_ != CCC_FHM_EMPTY)
        {
            fn((ccc_user_type){.user_type = slot, .aux = h->buf_.aux_});
        }
    }
    return ccc_buf_alloc(&h->buf_, 0, h->buf_.alloc_);
}

size_t
ccc_fhm_capacity(ccc_flat_hash_map const *const h)
{
    if (unlikely(!h))
    {
        return 0;
    }
    return ccc_buf_capacity(&h->buf_);
}

bool
ccc_fhm_validate(ccc_flat_hash_map const *const h)
{
    if (!h)
    {
        return false;
    }
    size_t empties = 0;
    size_t occupied = 0;
    for (void const *i = ccc_buf_begin(&h->buf_);
         i != ccc_buf_capacity_end(&h->buf_); i = ccc_buf_next(&h->buf_, i))
    {
        if (elem_in_slot(h, i)->hash_ == CCC_FHM_EMPTY)
        {
            ++empties;
        }
        else
        {
            ++occupied;
        }
        if (elem_in_slot(h, i)->hash_ != CCC_FHM_EMPTY
            && !valid_distance_from_home(h, i))
        {
            return false;
        }
    }
    return occupied == ccc_fhm_size(h)
           && empties == (ccc_buf_capacity(&h->buf_) - occupied);
}

/*=======================   Private Interface   =============================*/

ccc_result
ccc_impl_fhm_init_buf(struct ccc_fhmap_ *const h, size_t key_offset,
                      size_t const hash_elem_offset, ccc_hash_fn *const hash_fn,
                      ccc_key_eq_fn *const eq_fn, void *const aux)
{
    if (!h || !hash_fn || !eq_fn)
    {
        return CCC_INPUT_ERR;
    }
    h->key_offset_ = key_offset;
    h->hash_elem_offset_ = hash_elem_offset;
    h->hash_fn_ = hash_fn;
    h->eq_fn_ = eq_fn;
    h->buf_.aux_ = aux;
    if (ccc_buf_begin(&h->buf_))
    {
        (void)memset(ccc_buf_begin(&h->buf_), CCC_FHM_EMPTY,
                     ccc_buf_capacity(&h->buf_) * ccc_buf_elem_size(&h->buf_));
    }
    return CCC_OK;
}

struct ccc_fhash_entry_
ccc_impl_fhm_entry(struct ccc_fhmap_ *h, void const *key)
{
    return container_entry(h, key);
}

struct ccc_fhash_entry_ *
ccc_impl_fhm_and_modify(struct ccc_fhash_entry_ *const e,
                        ccc_update_fn *const fn)
{
    return and_modify(e, fn);
}

struct ccc_ent_
ccc_impl_fhm_find(struct ccc_fhmap_ const *const h, void const *const key,
                  uint64_t const hash)
{
    return find(h, key, hash);
}

void
ccc_impl_fhm_insert(struct ccc_fhmap_ *const h, void const *const e,
                    uint64_t const hash, size_t cur_i)
{
    insert(h, e, hash, cur_i);
}

ccc_result
ccc_impl_fhm_maybe_resize(struct ccc_fhmap_ *h)
{
    return maybe_resize(h);
}

struct ccc_fhmap_elem_ *
ccc_impl_fhm_in_slot(struct ccc_fhmap_ const *const h, void const *const slot)
{
    return elem_in_slot(h, slot);
}

void *
ccc_impl_fhm_key_in_slot(struct ccc_fhmap_ const *h, void const *slot)
{
    return key_in_slot(h, slot);
}

size_t
ccc_impl_fhm_distance(size_t const capacity, size_t const i, size_t const j)
{
    return distance(capacity, i, j);
}

size_t
ccc_impl_fhm_increment(size_t const capacity, size_t i)
{
    return increment(capacity, i);
}

void *
ccc_impl_fhm_base(struct ccc_fhmap_ const *const h)
{
    return ccc_buf_begin(&h->buf_);
}

inline uint64_t *
ccc_impl_fhm_hash_at(struct ccc_fhmap_ const *const h, size_t const i)
{
    return hash_at(h, i);
}

uint64_t
ccc_impl_fhm_filter(struct ccc_fhmap_ const *const h, void const *const key)
{
    return filter(h, key);
}

/*=======================     Static Helpers    =============================*/

static inline struct ccc_ent_
entry(struct ccc_fhmap_ *const h, void const *key, uint64_t const hash)
{
    char upcoming_insertion_error = 0;
    if (maybe_resize(h) != CCC_OK)
    {
        upcoming_insertion_error = CCC_ENTRY_INSERT_ERROR;
    }
    struct ccc_ent_ res = find(h, key, hash);
    res.stats_ |= upcoming_insertion_error;
    return res;
}

static inline struct ccc_ent_
find(struct ccc_fhmap_ const *const h, void const *const key,
     uint64_t const hash)
{
    size_t const cap = ccc_buf_capacity(&h->buf_);
    /* A few sanity checks. The load factor should be managed a full table is
       never allowed even under no allocation permission because that could
       lead to an infinite loop and illustrates a degenerate table anyway. */
    if (unlikely(!cap))
    {
        return (struct ccc_ent_){.e_ = NULL, .stats_ = CCC_ENTRY_VACANT};
    }
    if (unlikely(ccc_buf_size(&h->buf_) >= ccc_buf_capacity(&h->buf_)))
    {
        return (struct ccc_ent_){.e_ = NULL, .stats_ = CCC_ENTRY_INPUT_ERROR};
    }
    size_t i = to_i(cap, hash);
    size_t dist = 0;
    do
    {
        void *const slot = ccc_buf_at(&h->buf_, i);
        uint64_t const slot_hash = elem_in_slot(h, slot)->hash_;
        if (slot_hash == CCC_FHM_EMPTY
            || dist > distance(cap, i, to_i(cap, slot_hash)))
        {
            return (struct ccc_ent_){
                .e_ = slot, .stats_ = CCC_ENTRY_VACANT | CCC_ENTRY_NO_UNWRAP};
        }
        if (hash == slot_hash
            && h->eq_fn_((ccc_key_cmp){
                .key_lhs = key, .user_type_rhs = slot, .aux = h->buf_.aux_}))
        {
            return (struct ccc_ent_){.e_ = slot, .stats_ = CCC_ENTRY_OCCUPIED};
        }
        ++dist;
        i = increment(cap, i);
    } while (true);
}

/* Assumes that element to be inserted does not already exist in the table.
   Assumes that the table has room for another insertion. Unexpected results
   may occur if these assumptions are not accommodated. */
static inline void
insert(struct ccc_fhmap_ *const h, void const *const e, uint64_t const hash,
       size_t i)
{
    size_t const elem_sz = ccc_buf_elem_size(&h->buf_);
    size_t const cap = ccc_buf_capacity(&h->buf_);
    void *const tmp = ccc_buf_at(&h->buf_, 1);
    void *const floater = ccc_buf_at(&h->buf_, 0);
    (void)memcpy(floater, e, elem_sz);

    /* This function cannot modify e and e may be copied over to new
       insertion from old table. So should this function invariantly assign
       starting hash to this slot copy for insertion? I think yes so far. */
    elem_in_slot(h, floater)->hash_ = hash;
    size_t dist = distance(cap, i, to_i(cap, hash));
    do
    {
        void *const slot = ccc_buf_at(&h->buf_, i);
        uint64_t const slot_hash = elem_in_slot(h, slot)->hash_;
        if (slot_hash == CCC_FHM_EMPTY)
        {
            (void)memcpy(slot, floater, elem_sz);
            (void)ccc_buf_size_plus(&h->buf_, 1);
            elem_in_slot(h, floater)->hash_ = CCC_FHM_EMPTY;
            elem_in_slot(h, tmp)->hash_ = CCC_FHM_EMPTY;
            return;
        }
        size_t const slot_dist = distance(cap, i, to_i(cap, slot_hash));
        if (dist > slot_dist)
        {
            swap(tmp, floater, slot, elem_sz);
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
erase(struct ccc_fhmap_ *const h, void *const e)
{
    *hash_at(h, ccc_buf_i(&h->buf_, e)) = CCC_FHM_EMPTY;
    size_t const cap = ccc_buf_capacity(&h->buf_);
    size_t const elem_sz = ccc_buf_elem_size(&h->buf_);
    void *const tmp = ccc_buf_at(&h->buf_, 0);
    size_t i = ccc_buf_i(&h->buf_, e);
    size_t next = increment(cap, i);
    do
    {
        void *const next_slot = ccc_buf_at(&h->buf_, next);
        uint64_t const next_hash = elem_in_slot(h, next_slot)->hash_;
        if (next_hash == CCC_FHM_EMPTY
            || !distance(cap, next, to_i(cap, next_hash)))
        {
            elem_in_slot(h, tmp)->hash_ = CCC_FHM_EMPTY;
            (void)ccc_buf_size_minus(&h->buf_, 1);
            return;
        }
        swap(tmp, next_slot, ccc_buf_at(&h->buf_, i), elem_sz);
        i = next;
        next = increment(cap, next);
    } while (true);
}

static inline struct ccc_fhash_entry_
container_entry(struct ccc_fhmap_ *const h, void const *const key)
{
    uint64_t const hash = filter(h, key);
    return (struct ccc_fhash_entry_){
        .h_ = h,
        .hash_ = hash,
        .entry_ = entry(h, key, hash),
    };
}

static inline struct ccc_fhash_entry_ *
and_modify(struct ccc_fhash_entry_ *const e, ccc_update_fn *const fn)
{
    if (e->entry_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        fn((ccc_user_type){e->entry_.e_, NULL});
    }
    return e;
}

static ccc_result
maybe_resize(struct ccc_fhmap_ *const h)
{
    if (ccc_buf_capacity(&h->buf_)
        && (double)(ccc_buf_size(&h->buf_) + num_swap_slots)
                   / (double)ccc_buf_capacity(&h->buf_)
               <= load_factor)
    {
        return CCC_OK;
    }
    if (!h->buf_.alloc_)
    {
        return CCC_NO_ALLOC;
    }
    struct ccc_fhmap_ new_hash = *h;
    new_hash.buf_.sz_ = 0;
    new_hash.buf_.capacity_ = new_hash.buf_.capacity_
                                  ? next_prime(ccc_buf_size(&h->buf_) * 2)
                                  : primes[0];
    if (!new_hash.buf_.capacity_)
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
    /* Empty is intentionally chosen as zero so every byte is just set to
       0 in this new array. */
    (void)memset(ccc_buf_begin(&new_hash.buf_), CCC_FHM_EMPTY,
                 ccc_buf_capacity(&new_hash.buf_)
                     * ccc_buf_elem_size(&new_hash.buf_));
    for (void *slot = ccc_buf_begin(&h->buf_);
         slot != ccc_buf_capacity_end(&h->buf_);
         slot = ccc_buf_next(&h->buf_, slot))
    {
        struct ccc_fhmap_elem_ const *const e = elem_in_slot(h, slot);
        if (e->hash_ != CCC_FHM_EMPTY)
        {
            struct ccc_ent_ const new_ent
                = find(&new_hash, key_in_slot(h, slot), e->hash_);
            insert(&new_hash, slot, e->hash_,
                   ccc_buf_i(&new_hash.buf_, new_ent.e_));
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

static inline size_t
next_prime(size_t n)
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
hash_at(struct ccc_fhmap_ const *const h, size_t const i)
{
    return &((struct ccc_fhmap_elem_ *)((char *)ccc_buf_at(&h->buf_, i)
                                        + h->hash_elem_offset_))
                ->hash_;
}

static inline uint64_t
filter(struct ccc_fhmap_ const *const h, void const *const key)
{
    uint64_t const hash
        = h->hash_fn_((ccc_user_key){.user_key = key, .aux = h->buf_.aux_});
    return hash == CCC_FHM_EMPTY ? hash + 1 : hash;
}

static inline struct ccc_fhmap_elem_ *
elem_in_slot(struct ccc_fhmap_ const *const h, void const *const slot)
{
    return (struct ccc_fhmap_elem_ *)((char *)slot + h->hash_elem_offset_);
}

static inline void *
key_in_slot(struct ccc_fhmap_ const *const h, void const *const slot)
{
    return (char *)slot + h->key_offset_;
}

static inline size_t
distance(size_t const capacity, size_t const i, size_t const j)
{
    return i < j ? (capacity - j) + i - num_swap_slots : i - j;
}

static inline size_t
increment(size_t const capacity, size_t i)
{
    i = (i + 1) % capacity;
    return i <= last_swap_slot ? last_swap_slot + 1 : i;
}

static inline size_t
decrement(size_t const capacity, size_t i)
{
    i = i ? i - 1 : capacity - 1;
    return i <= last_swap_slot ? capacity - 1 : i;
}

static size_t
to_i(size_t const capacity, uint64_t hash)
{
    hash = hash % capacity;
    return hash <= last_swap_slot ? last_swap_slot + 1 : hash;
}

static inline void *
struct_base(struct ccc_fhmap_ const *const h,
            struct ccc_fhmap_elem_ const *const e)
{
    return ((char *)&e->hash_) - h->hash_elem_offset_;
}

static inline void
swap(char tmp[], void *const a, void *const b, size_t ab_size)
{
    if (a == b)
    {
        return;
    }
    (void)memcpy(tmp, a, ab_size);
    (void)memcpy(a, b, ab_size);
    (void)memcpy(b, tmp, ab_size);
}

static bool
valid_distance_from_home(struct ccc_fhmap_ const *h, void const *slot)
{
    size_t const cap = ccc_buf_capacity(&h->buf_);
    uint64_t const hash = elem_in_slot(h, slot)->hash_;
    size_t const home = to_i(cap, hash);
    size_t const end = decrement(cap, home);
    for (size_t i = ccc_buf_i(&h->buf_, slot),
                distance_to_home = distance(cap, i, to_i(cap, hash));
         i != end; --distance_to_home, i = decrement(cap, i))
    {
        uint64_t const cur_hash = *hash_at(h, i);
        /* The only reason an element is not home is because it has been
           moved away to help another element be closer to its home. This
           would break the purpose of doing that. Upon erase everyone needs
           to shuffle closer to home. */
        if (cur_hash == CCC_FHM_EMPTY)
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
