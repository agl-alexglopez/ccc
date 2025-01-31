/** This is another take on the Robin Hood Hash table that seeks to combine
the simplicity and benefits of Robin Hood with handle stability. Handle
stability is like pointer stability except that it can only promise that the
user's data will remain in the same index slot in the table for its entire
lifetime and will only be copied in once.

This is a very important guarantee when it can be achieved because it allows
for better composition of multiple data structures in this collection. If the
user knows their struct can be placed in an array and be part of a hash table
without the hash table modifying their data once inserted, they can set up
long lived structs that are part of many different containers and systems in
their program safely. The lifetime of the struct as a member of the hash
table can be the longest lasting.

To achieve this the table runs the round robin hash table algorithm on only the
intrusive elements in the user struct. With only a single additional data field
compared to the normal hash table, we move the metadata around the table
according to the algorithm and leave the user data in the slot to which it is
first written. Then, as many interfaces as possible expose these handles rather
then pointers to encourage the user to think in terms of stable indices. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
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
static struct ccc_handl_ handle(struct ccc_hhmap_ *, void const *key,
                                uint64_t hash);
static struct ccc_hhash_handle_ container_handle(struct ccc_hhmap_ *h,
                                                 void const *key);
static struct ccc_hhash_handle_ *and_modify(struct ccc_hhash_handle_ *e,
                                            ccc_update_fn *fn);
static bool valid_distance_from_home(struct ccc_hhmap_ const *, size_t slot);
static size_t to_i(size_t capacity, uint64_t hash);
static size_t increment(size_t capacity, size_t i);
static size_t decrement(size_t capacity, size_t i);
static size_t distance(size_t capacity, size_t i, size_t j);
static void *key_in_slot(struct ccc_hhmap_ const *h, void const *slot);
static void *key_at(struct ccc_hhmap_ const *h, size_t i);
static struct ccc_hhmap_elem_ *elem_in_slot(struct ccc_hhmap_ const *h,
                                            void const *slot);
static struct ccc_hhmap_elem_ *elem_at(struct ccc_hhmap_ const *h, size_t i);
static ccc_result maybe_resize(struct ccc_hhmap_ *h);
static struct ccc_handl_ find(struct ccc_hhmap_ const *h, void const *key,
                              uint64_t hash);
static ccc_handle_i insert_meta(struct ccc_hhmap_ *h, uint64_t hash, size_t i);
static uint64_t *hash_at(struct ccc_hhmap_ const *h, size_t i);
static uint64_t filter(struct ccc_hhmap_ const *h, void const *key);
static size_t next_prime(size_t n);
static void copy_to_slot(struct ccc_hhmap_ *h, void *slot_dst,
                         void const *user_src);
static void bubble_down(struct ccc_hhmap_ *h, size_t i, size_t slots);
static void heapify_slots(struct ccc_hhmap_ *, size_t slots);
static void pop_slot(struct ccc_hhmap_ *, size_t slots);

/*=========================   Interface    ==================================*/

void *
ccc_hhm_at(ccc_handle_hash_map const *const h, ccc_handle_i i)
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
        return false;
    }
    return handle(h, key, filter(h, key)).stats_ == CCC_OCCUPIED;
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

ccc_hhmap_handle
ccc_hhm_handle(ccc_handle_hash_map *const h, void const *const key)
{
    if (unlikely(!h || !key))
    {
        return (ccc_hhmap_handle){{.handle_ = {.stats_ = CCC_INPUT_ERROR}}};
    }
    return (ccc_hhmap_handle){container_handle(h, key)};
}

ccc_handle_i
ccc_hhm_insert_handle(ccc_hhmap_handle const *const e,
                      ccc_hhmap_elem *const elem)
{
    if (unlikely(!e || !elem))
    {
        return 0;
    }
    void *const user_struct = struct_base(e->impl_.h_, elem);
    if (e->impl_.handle_.stats_ & CCC_OCCUPIED)
    {
        copy_to_slot(
            e->impl_.h_,
            ccc_buf_at(&e->impl_.h_->buf_,
                       elem_at(e->impl_.h_, e->impl_.handle_.i_)->slot_i_),
            struct_base(e->impl_.h_, elem));
        return elem_at(e->impl_.h_, e->impl_.handle_.i_)->slot_i_;
    }
    if (e->impl_.handle_.stats_ & CCC_INSERT_ERROR)
    {
        return 0;
    }
    ccc_handle_i const ins
        = insert_meta(e->impl_.h_, e->impl_.hash_, e->impl_.handle_.i_);
    copy_to_slot(
        e->impl_.h_,
        ccc_buf_at(&e->impl_.h_->buf_, elem_at(e->impl_.h_, ins)->slot_i_),
        user_struct);
    return elem_at(e->impl_.h_, ins)->slot_i_;
}

ccc_handle_i
ccc_hhm_get_key_val(ccc_handle_hash_map *const h, void const *const key)
{
    if (unlikely(!h || !key || !ccc_buf_capacity(&h->buf_)
                 || maybe_resize(h) != CCC_OK))
    {
        return 0;
    }
    struct ccc_handl_ e = find(h, key, filter(h, key));
    if (e.stats_ & CCC_OCCUPIED)
    {
        return elem_at(h, e.i_)->slot_i_;
    }
    return 0;
}

ccc_handle
ccc_hhm_remove_handle(ccc_hhmap_handle const *const e)
{
    if (unlikely(!e))
    {
        return (ccc_handle){{.stats_ = CCC_INPUT_ERROR}};
    }
    if (e->impl_.handle_.stats_ != CCC_OCCUPIED)
    {
        return (ccc_handle){{.stats_ = CCC_VACANT}};
    }
    erase_meta(e->impl_.h_, e->impl_.handle_.i_);
    return (ccc_handle){{.stats_ = CCC_OCCUPIED}};
}

ccc_hhmap_handle *
ccc_hhm_and_modify(ccc_hhmap_handle *const e, ccc_update_fn *const fn)
{
    return (ccc_hhmap_handle *)and_modify(&e->impl_, fn);
}

ccc_hhmap_handle *
ccc_hhm_and_modify_aux(ccc_hhmap_handle *const e, ccc_update_fn *const fn,
                       void *const aux)
{
    if (unlikely(!e))
    {
        return NULL;
    }
    if (e->impl_.handle_.stats_ == CCC_OCCUPIED && fn)
    {
        fn((ccc_user_type){
            ccc_buf_at(&e->impl_.h_->buf_,
                       elem_at(e->impl_.h_, e->impl_.handle_.i_)->slot_i_),
            aux});
    }
    return e;
}

ccc_handle
ccc_hhm_insert(ccc_handle_hash_map *const h, ccc_hhmap_elem *const out_handle)
{
    if (unlikely(!h || !out_handle))
    {
        return (ccc_handle){{.stats_ = CCC_INPUT_ERROR}};
    }
    void *const user_data = struct_base(h, out_handle);
    void *const key = key_in_slot(h, user_data);
    struct ccc_hhash_handle_ ent = container_handle(h, key);
    if (ent.handle_.stats_ & CCC_OCCUPIED)
    {
        swap_user_data(
            h, ccc_buf_at(&h->buf_, elem_at(h, ent.handle_.i_)->slot_i_),
            user_data);
        return (ccc_handle){{.i_ = elem_at(h, ent.handle_.i_)->slot_i_,
                             .stats_ = CCC_OCCUPIED}};
    }
    if (ent.handle_.stats_ & CCC_INSERT_ERROR)
    {
        return (ccc_handle){{.i_ = elem_at(h, ent.handle_.i_)->slot_i_,
                             .stats_ = CCC_INSERT_ERROR}};
    }
    ccc_handle_i const ins = insert_meta(h, ent.hash_, ent.handle_.i_);
    copy_to_slot(h, ccc_buf_at(&h->buf_, elem_at(h, ins)->slot_i_), user_data);
    return (ccc_handle){{.i_ = elem_at(h, ins)->slot_i_, .stats_ = CCC_VACANT}};
}

ccc_handle
ccc_hhm_try_insert(ccc_handle_hash_map *const h,
                   ccc_hhmap_elem *const key_val_handle)
{
    if (unlikely(!h || !key_val_handle))
    {
        return (ccc_handle){{.stats_ = CCC_INPUT_ERROR}};
    }
    void *const user_data = struct_base(h, key_val_handle);
    struct ccc_hhash_handle_ ent
        = container_handle(h, key_in_slot(h, user_data));
    if (ent.handle_.stats_ & CCC_OCCUPIED)
    {
        return (ccc_handle){{.i_ = elem_at(h, ent.handle_.i_)->slot_i_,
                             .stats_ = CCC_OCCUPIED}};
    }
    if (ent.handle_.stats_ & CCC_INSERT_ERROR)
    {
        return (ccc_handle){{.stats_ = CCC_INSERT_ERROR}};
    }
    ccc_handle_i const ins = insert_meta(h, ent.hash_, ent.handle_.i_);
    copy_to_slot(h, ccc_buf_at(&h->buf_, elem_at(h, ins)->slot_i_), user_data);
    return (ccc_handle){{.i_ = elem_at(h, ins)->slot_i_, .stats_ = CCC_VACANT}};
}

ccc_handle
ccc_hhm_insert_or_assign(ccc_handle_hash_map *const h,
                         ccc_hhmap_elem *const key_val_handle)
{
    if (unlikely(!h || !key_val_handle))
    {
        return (ccc_handle){{.stats_ = CCC_INPUT_ERROR}};
    }
    void *const user_base = struct_base(h, key_val_handle);
    struct ccc_hhash_handle_ ent
        = container_handle(h, key_in_slot(h, user_base));
    if (ent.handle_.stats_ & CCC_OCCUPIED)
    {
        copy_to_slot(h,
                     ccc_buf_at(&h->buf_, elem_at(h, ent.handle_.i_)->slot_i_),
                     user_base);
        return (ccc_handle){{.i_ = elem_at(h, ent.handle_.i_)->slot_i_,
                             .stats_ = CCC_OCCUPIED}};
    }
    if (ent.handle_.stats_ & CCC_INSERT_ERROR)
    {
        return (ccc_handle){{.stats_ = CCC_INSERT_ERROR}};
    }
    ccc_handle_i const ins = insert_meta(h, ent.hash_, ent.handle_.i_);
    copy_to_slot(h, ccc_buf_at(&h->buf_, elem_at(h, ins)->slot_i_), user_base);
    return (ccc_handle){{.i_ = elem_at(h, ins)->slot_i_, .stats_ = CCC_VACANT}};
}

ccc_handle
ccc_hhm_remove(ccc_handle_hash_map *const h, ccc_hhmap_elem *const out_handle)
{
    if (unlikely(!h || !out_handle))
    {
        return (ccc_handle){{.stats_ = CCC_INPUT_ERROR}};
    }
    void *const data = struct_base(h, out_handle);
    void *const key = key_in_slot(h, data);
    struct ccc_handl_ const ent = find(h, key, filter(h, key));
    if (ent.stats_ & CCC_OCCUPIED)
    {
        (void)memcpy(data, ccc_buf_at(&h->buf_, elem_at(h, ent.i_)->slot_i_),
                     ccc_buf_elem_size(&h->buf_));
        erase_meta(h, ent.i_);
        return (ccc_handle){{.stats_ = CCC_OCCUPIED}};
    }
    return (ccc_handle){{.stats_ = CCC_VACANT}};
}

ccc_handle_i
ccc_hhm_or_insert(ccc_hhmap_handle const *const e, ccc_hhmap_elem *const elem)
{
    if (unlikely(!e || !elem))
    {
        return 0;
    }
    if (e->impl_.handle_.stats_ & CCC_OCCUPIED)
    {
        return elem_at(e->impl_.h_, e->impl_.handle_.i_)->slot_i_;
    }
    if (e->impl_.handle_.stats_ & CCC_INSERT_ERROR)
    {
        return 0;
    }
    void *user_struct = struct_base(e->impl_.h_, elem);
    ccc_handle_i const ins
        = insert_meta(e->impl_.h_, e->impl_.hash_, e->impl_.handle_.i_);
    copy_to_slot(
        e->impl_.h_,
        ccc_buf_at(&e->impl_.h_->buf_, elem_at(e->impl_.h_, ins)->slot_i_),
        user_struct);
    return elem_at(e->impl_.h_, ins)->slot_i_;
}

ccc_handle_i
ccc_hhm_unwrap(ccc_hhmap_handle const *const e)
{
    if (unlikely(!e) || !(e->impl_.handle_.stats_ & CCC_OCCUPIED))
    {
        return 0;
    }
    return elem_at(e->impl_.h_, e->impl_.handle_.i_)->slot_i_;
}

bool
ccc_hhm_occupied(ccc_hhmap_handle const *const e)
{
    if (unlikely(!e))
    {
        return false;
    }
    return e->impl_.handle_.stats_ & CCC_OCCUPIED;
}

bool
ccc_hhm_insert_error(ccc_hhmap_handle const *const e)
{
    if (unlikely(!e))
    {
        return false;
    }
    return e->impl_.handle_.stats_ & CCC_INSERT_ERROR;
}

ccc_handle_status
ccc_hhm_handle_status(ccc_hhmap_handle const *const e)
{
    if (unlikely(!e))
    {
        return CCC_INPUT_ERROR;
    }
    return e->impl_.handle_.stats_;
}

ccc_hhmap_handle
ccc_hhm_begin(ccc_handle_hash_map const *const h)
{
    if (unlikely(!h || ccc_buf_is_empty(&h->buf_)))
    {
        return (ccc_hhmap_handle){{.hash_ = CCC_HHM_EMPTY}};
    }
    size_t iter = 0;
    for (; iter < ccc_buf_capacity(&h->buf_)
           && elem_at(h, iter)->hash_ == CCC_HHM_EMPTY;
         ++iter)
    {}
    if (iter >= ccc_buf_capacity(&h->buf_))
    {
        return (ccc_hhmap_handle){{.hash_ = CCC_HHM_EMPTY}};
    }
    return (ccc_hhmap_handle){
        {.h_ = (ccc_handle_hash_map *)h,
         .hash_ = elem_at(h, iter)->hash_,
         .handle_ = {.i_ = iter, .stats_ = CCC_OCCUPIED}}};
}

ccc_result
ccc_hhm_next(ccc_hhmap_handle *const iter)
{
    if (unlikely(!iter))
    {
        return CCC_INPUT_ERR;
    }
    ++iter->impl_.handle_.i_;
    for (; iter->impl_.handle_.i_ < ccc_buf_capacity(&iter->impl_.h_->buf_)
           && elem_at(iter->impl_.h_, iter->impl_.handle_.i_)->hash_
                  == CCC_HHM_EMPTY;
         ++iter->impl_.handle_.i_)
    {}
    if (iter->impl_.handle_.i_ >= ccc_buf_capacity(&iter->impl_.h_->buf_))
    {
        *iter = (ccc_hhmap_handle){{.hash_ = CCC_HHM_EMPTY}};
    }
    iter->impl_.hash_ = elem_at(iter->impl_.h_, iter->impl_.handle_.i_)->hash_;
    iter->impl_.handle_.stats_ = CCC_OCCUPIED;
    return CCC_OK;
}

bool
ccc_hhm_end(ccc_hhmap_handle const *const iter)
{
    return !iter || iter->impl_.hash_ == CCC_HHM_EMPTY;
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
    if (dst->buf_.capacity_ == src->buf_.capacity_)
    {
        (void)memcpy(dst->buf_.mem_, src->buf_.mem_,
                     src->buf_.capacity_ * src->buf_.elem_sz_);
        return CCC_OK;
    }
    dst->buf_.sz_ = num_swap_slots;
    for (size_t i = 0; i < ccc_buf_capacity(&dst->buf_); ++i)
    {
        struct ccc_hhmap_elem_ *const e = elem_at(dst, i);
        e->hash_ = CCC_HHM_EMPTY;
        e->slot_i_ = i;
    }
    for (size_t i = 0; i < ccc_buf_capacity(&src->buf_); ++i)
    {
        struct ccc_hhmap_elem_ *const e = elem_at(src, i);
        if (e->hash_ == CCC_HHM_EMPTY)
        {
            continue;
        }
        struct ccc_handl_ const new_ent
            = find(dst, key_at(src, e->slot_i_), e->hash_);
        ccc_handle_i const ins = insert_meta(dst, e->hash_, new_ent.i_);
        copy_to_slot(dst, ccc_buf_at(&dst->buf_, elem_at(dst, ins)->slot_i_),
                     ccc_buf_at(&src->buf_, e->slot_i_));
    }
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
    for (size_t i = 0; i < ccc_buf_capacity(&h->buf_); ++i)
    {
        struct ccc_hhmap_elem_ *const e = elem_at(h, i);
        if (e->hash_ != CCC_HHM_EMPTY)
        {
            fn((ccc_user_type){.user_type
                               = ccc_buf_at(&h->buf_, elem_at(h, i)->slot_i_),
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
    for (size_t i = 0; i < ccc_buf_capacity(&h->buf_); ++i)
    {
        struct ccc_hhmap_elem_ *const e = elem_at(h, i);
        if (e->hash_ != CCC_HHM_EMPTY)
        {
            fn((ccc_user_type){.user_type
                               = ccc_buf_at(&h->buf_, elem_at(h, i)->slot_i_),
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
        if (e->slot_i_ >= ccc_buf_capacity(&h->buf_))
        {
            return false;
        }
        if (e->hash_ == CCC_HHM_EMPTY)
        {
            ++empties;
        }
        else
        {
            ++occupied;
            uint64_t const hash = filter(h, key_at(h, e->slot_i_));
            if (hash != e->hash_)
            {
                return false;
            }
            if (!valid_distance_from_home(h, i))
            {
                return false;
            }
        }
    }
    return occupied == ccc_hhm_size(h)
           && empties == (ccc_buf_capacity(&h->buf_) - occupied);
}

static bool
valid_distance_from_home(struct ccc_hhmap_ const *const h, size_t const slot)
{
    size_t const cap = ccc_buf_capacity(&h->buf_);
    uint64_t const hash = elem_at(h, slot)->hash_;
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

struct ccc_hhash_handle_
ccc_impl_hhm_handle(struct ccc_hhmap_ *const h, void const *const key)
{
    return container_handle(h, key);
}

struct ccc_hhash_handle_ *
ccc_impl_hhm_and_modify(struct ccc_hhash_handle_ *const e,
                        ccc_update_fn *const fn)
{
    return and_modify(e, fn);
}

struct ccc_handl_
ccc_impl_hhm_find(struct ccc_hhmap_ const *const h, void const *const key,
                  uint64_t const hash)
{
    return find(h, key, hash);
}

ccc_handle_i
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

static inline struct ccc_handl_
handle(struct ccc_hhmap_ *const h, void const *const key, uint64_t const hash)
{
    uint8_t future_insert_error = 0;
    if (maybe_resize(h) != CCC_OK)
    {
        future_insert_error = CCC_INSERT_ERROR;
    }
    struct ccc_handl_ res = find(h, key, hash);
    res.stats_ |= future_insert_error;
    return res;
}

/* Finds and returns a handle to the metadata (NOT the home slot with user data)
   for the specified query. If the data cannot be found the slot at which the
   data SHOULD have its metadata inserted is returned. This could be an empty
   slot or a slot with an element that is already using it. In the second case
   this is where Robin Hood would start swapping should we choose to follow
   through with an insert. Metadata slots point to the backing storage slot. */
static inline struct ccc_handl_
find(struct ccc_hhmap_ const *const h, void const *const key,
     uint64_t const hash)
{
    size_t const cap = ccc_buf_capacity(&h->buf_);
    /* A few sanity checks. The load factor should be managed a full table is
       never allowed even under no allocation permission because that could
       lead to an infinite loop and illustrates a degenerate table anyway. */
    if (unlikely(!cap))
    {
        return (struct ccc_handl_){.i_ = 0, .stats_ = CCC_VACANT};
    }
    if (unlikely(ccc_buf_size(&h->buf_) >= ccc_buf_capacity(&h->buf_)))
    {
        return (struct ccc_handl_){.i_ = 0, .stats_ = CCC_INPUT_ERROR};
    }
    size_t i = to_i(cap, hash);
    size_t dist = 0;
    do
    {
        struct ccc_hhmap_elem_ const *const e = elem_at(h, i);
        if (e->hash_ == CCC_HHM_EMPTY
            || dist > distance(cap, i, to_i(cap, e->hash_)))
        {
            return (struct ccc_handl_){.i_ = i,
                                       .stats_ = CCC_VACANT | CCC_NO_UNWRAP};
        }
        if (hash == e->hash_
            && h->eq_fn_(
                (ccc_key_cmp){.key_lhs = key,
                              .user_type_rhs = ccc_buf_at(&h->buf_, e->slot_i_),
                              .aux = h->buf_.aux_}))
        {
            return (struct ccc_handl_){.i_ = i, .stats_ = CCC_OCCUPIED};
        }
        ++dist;
        i = increment(cap, i);
    } while (true);
}

/* Assumes the table is prepared for the hash to be inserted at given index i
   (obtained from a previous search). Manages metadata only for this insertion
   and the effects it may have on Robin Hood logic. Returns the handle of the
   metadata for this new hash that has been inserted. */
static inline ccc_handle_i
insert_meta(struct ccc_hhmap_ *const h, uint64_t const hash, size_t i)
{
    size_t const cap = ccc_buf_capacity(&h->buf_);
    struct ccc_hhmap_elem_ *const floater = elem_at(h, 1);
    size_t const e_meta = i;
    *floater = *elem_at(h, e_meta);
    floater->hash_ = hash;
    size_t dist = distance(cap, e_meta, to_i(cap, hash));
    /* The slot we are given is where the actual data will eventually go. We
       now need to run the robin hood algo on just the metadata entries. */
    i = e_meta;
    do
    {
        struct ccc_hhmap_elem_ *const elem = elem_at(h, i);
        ccc_handle_i const user_data_slot = elem->slot_i_;
        if (elem->hash_ == CCC_HHM_EMPTY)
        {
            elem_at(h, e_meta)->slot_i_ = user_data_slot;
            *elem = *floater;
            (void)ccc_buf_size_plus(&h->buf_, 1);
            *elem_at(h, 0) = (struct ccc_hhmap_elem_){};
            *elem_at(h, 1) = (struct ccc_hhmap_elem_){};
            return e_meta;
        }
        size_t const slot_dist = distance(cap, i, to_i(cap, elem->hash_));
        if (dist > slot_dist)
        {
            swap_meta_data(h, floater, elem);
            dist = slot_dist;
        }
        i = increment(cap, i);
        ++dist;
    } while (true);
}

/* Backshift deletion is important in for this table because it may not be able
   to allocate. This prevents the need for tombstones which would hurt table
   quality quickly if we can't resize. Only metadata is moved. User data remains
   in the same slot for handle stability. */
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
        if (next_elem->hash_ == CCC_HHM_EMPTY
            || !distance(cap, next, to_i(cap, next_elem->hash_)))
        {
            *hash_at(h, 0) = CCC_HHM_EMPTY;
            (void)ccc_buf_size_minus(&h->buf_, 1);
            return;
        }
        swap_meta_data(h, elem_at(h, i), next_elem);
        i = next;
        next = increment(cap, next);
    } while (true);
}

static inline struct ccc_hhash_handle_
container_handle(struct ccc_hhmap_ *const h, void const *const key)
{
    uint64_t const hash = filter(h, key);
    return (struct ccc_hhash_handle_){
        .h_ = h,
        .hash_ = hash,
        .handle_ = handle(h, key, hash),
    };
}

static inline struct ccc_hhash_handle_ *
and_modify(struct ccc_hhash_handle_ *const e, ccc_update_fn *const fn)
{
    if (e->handle_.stats_ == CCC_OCCUPIED)
    {
        fn((ccc_user_type){
            ccc_buf_at(&e->h_->buf_, elem_at(e->h_, e->handle_.i_)->slot_i_),
            NULL});
    }
    return e;
}

/* A simple memcpy will not work for this table because important metadata for
   a user slot elsewhere could be occupying this slot. Because only metadata
   is swapped and moved during Robin Hood, metadata and the user data in a slot
   may be completely unrelated. The intrusive element we are using could also
   be anywhere in the provided user struct so we need careful copy. */
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

/* This is by far the most complex and questionable operation across any
   container in the collection so far. A handle hash map should support
   complete handle stability for elements in the hash table, the same as the
   user would expect when an std::vector resizes in C++; the index at which user
   data lives should not change when resized. However, to support Robin Hood
   hashing metadata to the appropriate slot while keeping user data at the
   same position, the algorithm is hard.

     - First, copy the existing table as is to the larger table. This keeps
       user data at the same slot.
     - Then take whatever nonsense slot the new hash table has chosen for the
       inserted metadata and change it to the appropriate slot from the old
       table. This is the same index but in the new larger capacity table.
     - We have created a huge problem by doing this. Empty slots may have had
       their associated slots stolen by linking metadata to the original user
       data slots.
     - Gather all available free slots remaining in the new capacity table and
       link them to every empty metadata slot in the table ensuring every
       metadata entry has a unique slot as its backing storage space.
     - By the end of these operations every metadata slot should point to its
       own unique backing storage slot. All metadata for old data from the old
       table should point to the same slots as backing stores even if the
       metadata is now in a different slot itself.

   Best algorithm I could come up with is O(NlgN) time with no auxiliary space.
   We repurpose the old hash table to help sort and distribute the free slots in
   the new table. Resizing should occur relatively infrequently, and I deem this
   an acceptable trade off for the complete handle stability this offers for
   now, but we can try to improve this in the future. I'm sure there is a
   better way.

   We could be faster if we ran a free list with extra fields in intrusive
   elems, but it would cost one or two fields just to optimize ideally
   infrequent operations of resizing. That space cost would be too high. */
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
            e->slot_i_ = i;
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
    /* The handles shall be stable. Keep user data in the same slot. */
    if (h->buf_.mem_)
    {
        (void)memcpy(new_hash.buf_.mem_, h->buf_.mem_,
                     ccc_buf_capacity(&h->buf_) * ccc_buf_elem_size(&h->buf_));
    }
    /* Hash needs to start empty. Not sure if slot matters. */
    for (size_t i = 0; i < ccc_buf_capacity(&new_hash.buf_); ++i)
    {
        struct ccc_hhmap_elem_ *const e = elem_at(&new_hash, i);
        e->hash_ = 0;
        e->slot_i_ = i;
    }
    (void)ccc_buf_size_set(&new_hash.buf_, num_swap_slots);
    /* Run Robin Hood on the hash but instead of copying data to the chosen
       slot, link to the existing data at the old handle. */
    size_t allocated_slots = 0;
    for (size_t slot = 0; slot < ccc_buf_capacity(&h->buf_); ++slot)
    {
        struct ccc_hhmap_elem_ const *const e = elem_at(h, slot);
        if (e->hash_ == CCC_HHM_EMPTY)
        {
            continue;
        }
        struct ccc_handl_ const new_ent
            = find(&new_hash, key_at(h, e->slot_i_), e->hash_);
        ccc_handle_i const ins = insert_meta(&new_hash, e->hash_, new_ent.i_);
        /* Old handle linking. Same slot in larger table. */
        elem_at(&new_hash, ins)->slot_i_ = e->slot_i_;
        /* Repurpose the old table for allocation sorting as we go. */
        elem_at(h, allocated_slots)->slot_i_ = e->slot_i_;
        ++allocated_slots;
    }
    /* We will use an in place O(N) heapify to tell us where the free slot runs
       are between allocated slots. Consider what happens if we have N sorted
       integers each representing an occupied slot. Every integer starting at 0
       between these sorted integers represents a free slot that can be given to
       an empty slot in need in the new table. */
    heapify_slots(h, allocated_slots);
    for (size_t slot = 0, free_slot = 0;
         slot < ccc_buf_capacity(&new_hash.buf_); ++slot)
    {
        struct ccc_hhmap_elem_ *const e = elem_at(&new_hash, slot);
        if (e->hash_ != CCC_HHM_EMPTY)
        {
            continue;
        }
        /* Continually pop from the heap until the free slot finds a gap
           between occupied slots or there are no longer taken slots.
           The taken slots will naturally run out because the new capacity
           is greater than the old. This is an O(lgN) operation in the resizing
           scheme. Nested loop but not O(N^2) because we are progressing through
           free and allocated slot numbers linearly and will not revisit a
           number after it is passed as we visit each slot in the larger table.
           Each visited table slot, allocated heap slot, and free slot
           distribution progress together linearly, so O(NlgN). */
        while (allocated_slots && free_slot == elem_at(h, 0)->slot_i_)
        {
            pop_slot(h, allocated_slots);
            ++free_slot;
            --allocated_slots;
        }
        e->slot_i_ = free_slot;
        ++free_slot;
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
heapify_slots(struct ccc_hhmap_ *const h, size_t const slots)
{
    if (!slots)
    {
        return;
    }
    for (size_t i = (slots / 2) + 1; i--;)
    {
        bubble_down(h, i, slots);
    }
}

static inline void
pop_slot(struct ccc_hhmap_ *const h, size_t const slots)
{
    elem_at(h, 0)->slot_i_ = elem_at(h, slots - 1)->slot_i_;
    bubble_down(h, 0, slots - 1);
}

static inline void
bubble_down(struct ccc_hhmap_ *const h, size_t i, size_t const slots)
{
    struct ccc_hhmap_elem_ *const tmp = elem_at(h, slots);
    for (size_t next = i, left = (i * 2) + 1, right = left + 1; left < slots;
         i = next, left = (i * 2) + 1, right = left + 1)
    {
        next = (right < slots
                && elem_at(h, right)->slot_i_ < elem_at(h, left)->slot_i_)
                   ? right
                   : left;
        if (elem_at(h, next)->slot_i_ >= elem_at(h, i)->slot_i_)
        {
            return;
        }
        tmp->slot_i_ = elem_at(h, next)->slot_i_;
        elem_at(h, next)->slot_i_ = elem_at(h, i)->slot_i_;
        elem_at(h, i)->slot_i_ = tmp->slot_i_;
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
                ->hash_;
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

static inline void *
key_at(struct ccc_hhmap_ const *const h, size_t const i)
{
    return (char *)ccc_buf_at(&h->buf_, i) + h->key_offset_;
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
    return ((char *)&e->hash_) - h->hash_elem_offset_;
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
    *elem_at(h, 0) = (struct ccc_hhmap_elem_){};
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
    *tmp = *a;
    *a = *b;
    *b = *tmp;
    *elem_at(h, 0) = (struct ccc_hhmap_elem_){};
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
