#include "flat_hash_map.h"
#include "buffer.h"
#include "impl_flat_hash_map.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static double const load_factor = 0.8;
static size_t const default_prime = 11;
static size_t const last_swap_slot = 1;
static size_t const num_swap_slots = 2;

static void erase(struct ccc_fhm_ *, void *);

static bool is_prime(size_t);
static void swap(uint8_t tmp[], void *, void *, size_t);
static void *struct_base(struct ccc_fhm_ const *, struct ccc_fhm_elem_ const *);
static struct ccc_entry_ entry(struct ccc_fhm_ *, void const *key,
                               uint64_t hash);
static bool valid_distance_from_home(struct ccc_fhm_ const *, void const *slot);
static size_t to_index(size_t capacity, uint64_t hash);
static size_t decrement(size_t capacity, size_t i);

ccc_result
ccc_impl_fhm_init(struct ccc_fhm_ *const h, size_t key_offset,
                  size_t const hash_elem_offset, ccc_hash_fn *const hash_fn,
                  ccc_key_eq_fn *const eq_fn, void *const aux)
{
    if (!h || !hash_fn || !eq_fn)
    {
        return CCC_MEM_ERR;
    }
    h->key_offset_ = key_offset;
    h->hash_elem_offset_ = hash_elem_offset;
    h->hash_fn_ = hash_fn;
    h->eq_fn_ = eq_fn;
    h->aux_ = aux;
    memset(ccc_buf_begin(&h->buf_), CCC_FHM_EMPTY,
           ccc_buf_capacity(&h->buf_) * ccc_buf_elem_size(&h->buf_));
    return CCC_OK;
}

bool
ccc_fhm_empty(ccc_flat_hash_map const *const h)
{
    return !ccc_buf_size(&h->impl_.buf_);
}

bool
ccc_fhm_contains(ccc_flat_hash_map *const h, void const *const key)
{
    return entry(&h->impl_, key, ccc_impl_fhm_filter(&h->impl_, key)).stats_
           & CCC_ENTRY_OCCUPIED;
}

size_t
ccc_fhm_size(ccc_flat_hash_map const *const h)
{
    return ccc_buf_size(&h->impl_.buf_);
}

ccc_fh_map_entry
ccc_fhm_entry(ccc_flat_hash_map *h, void const *const key)
{
    return (ccc_fh_map_entry){ccc_impl_fhm_entry(&h->impl_, key)};
}

struct ccc_fhm_entry_
ccc_impl_fhm_entry(struct ccc_fhm_ *h, void const *key)
{
    uint64_t const hash = ccc_impl_fhm_filter(h, key);
    return (struct ccc_fhm_entry_){
        .h_ = h,
        .hash_ = hash,
        .entry_ = entry(h, key, hash),
    };
}

void *
ccc_fhm_insert_entry(ccc_fh_map_entry const *const e,
                     ccc_fh_map_elem *const elem)
{
    void *user_struct = struct_base(e->impl_.h_, &elem->impl_);
    if (e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        /* It is ok if an insert error was indicated because we do not
           need more space if we are overwriting a previous value. */
        elem->impl_.hash_ = e->impl_.hash_;
        memcpy(e->impl_.entry_.e_, user_struct,
               ccc_buf_elem_size(&e->impl_.h_->buf_));
        return e->impl_.entry_.e_;
    }
    if (e->impl_.entry_.stats_ & ~CCC_ENTRY_OCCUPIED)
    {
        return NULL;
    }
    ccc_impl_fhm_insert(
        e->impl_.h_, user_struct, e->impl_.hash_,
        ccc_buf_index_of(&e->impl_.h_->buf_, e->impl_.entry_.e_));
    return e->impl_.entry_.e_;
}

void *
ccc_fhm_get_key_val(ccc_flat_hash_map *const h, void const *const key)
{
    if (!ccc_buf_capacity(&h->impl_.buf_))
    {
        return NULL;
    }
    struct ccc_entry_ e = ccc_impl_fhm_find(
        &h->impl_, key, ccc_impl_fhm_filter(&h->impl_, key));
    if (e.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return e.e_;
    }
    return NULL;
}

ccc_entry
ccc_fhm_remove_entry(ccc_fh_map_entry const *const e)
{
    if (e->impl_.entry_.stats_ != CCC_ENTRY_OCCUPIED)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
    }
    erase(e->impl_.h_, e->impl_.entry_.e_);
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_OCCUPIED}};
}

ccc_fh_map_entry *
ccc_fhm_and_modify(ccc_fh_map_entry *const e, ccc_update_fn *const fn)
{
    return (ccc_fh_map_entry *)ccc_impl_fhm_and_modify(&e->impl_, fn);
}

struct ccc_fhm_entry_ *
ccc_impl_fhm_and_modify(struct ccc_fhm_entry_ *const e, ccc_update_fn *const fn)
{
    if (e->entry_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        fn(&(ccc_update){e->entry_.e_, NULL});
    }
    return e;
}

ccc_fh_map_entry *
ccc_fhm_and_modify_with(ccc_fh_map_entry *const e, ccc_update_fn *const fn,
                        void *aux)
{
    if (e->impl_.entry_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        fn(&(ccc_update){e->impl_.entry_.e_, aux});
    }
    return e;
}

ccc_entry
ccc_fhm_insert(ccc_flat_hash_map *h, ccc_fh_map_elem *const out_handle,
               void *const tmp)
{
    void *user_return = struct_base(&h->impl_, &out_handle->impl_);
    void *key = ccc_impl_fhm_key_in_slot(&h->impl_, user_return);
    size_t const user_struct_size = ccc_buf_elem_size(&h->impl_.buf_);
    struct ccc_fhm_entry_ ent = ccc_impl_fhm_entry(&h->impl_, key);
    if (ent.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        out_handle->impl_.hash_ = ent.hash_;
        swap(tmp, ent.entry_.e_, user_return, user_struct_size);
        return (ccc_entry){{.e_ = user_return, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (ent.entry_.stats_ & (CCC_ENTRY_CONTAINS_NULL | CCC_ENTRY_INSERT_ERROR))
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    ccc_impl_fhm_insert(&h->impl_, user_return, ent.hash_,
                        ccc_buf_index_of(&h->impl_.buf_, ent.entry_.e_));
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_fhm_try_insert(ccc_flat_hash_map *const h,
                   ccc_fh_map_elem *const key_val_handle)
{
    void *user_base = struct_base(&h->impl_, &key_val_handle->impl_);
    struct ccc_fhm_entry_ ent = ccc_impl_fhm_entry(
        &h->impl_, ccc_impl_fhm_key_in_slot(&h->impl_, user_base));
    if (ent.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return (ccc_entry){{.e_ = ent.entry_.e_, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (ent.entry_.stats_ & (CCC_ENTRY_CONTAINS_NULL | CCC_ENTRY_INSERT_ERROR))
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    ccc_impl_fhm_insert(&h->impl_, user_base, ent.hash_,
                        ccc_buf_index_of(&h->impl_.buf_, ent.entry_.e_));
    return (ccc_entry){{.e_ = ent.entry_.e_, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_fhm_insert_or_assign(ccc_flat_hash_map *h, ccc_fh_map_elem *key_val_handle)
{
    void *user_base = struct_base(&h->impl_, &key_val_handle->impl_);
    struct ccc_fhm_entry_ ent = ccc_impl_fhm_entry(
        &h->impl_, ccc_impl_fhm_key_in_slot(&h->impl_, user_base));
    if (ent.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        memcpy(ent.entry_.e_, user_base, ccc_buf_elem_size(&h->impl_.buf_));
        return (ccc_entry){{.e_ = ent.entry_.e_, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (ent.entry_.stats_ & (CCC_ENTRY_CONTAINS_NULL | CCC_ENTRY_INSERT_ERROR))
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    ccc_impl_fhm_insert(&h->impl_, user_base, ent.hash_,
                        ccc_buf_index_of(&h->impl_.buf_, ent.entry_.e_));
    return (ccc_entry){{.e_ = ent.entry_.e_, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_fhm_remove(ccc_flat_hash_map *const h, ccc_fh_map_elem *const out_handle)
{
    void *ret = struct_base(&h->impl_, &out_handle->impl_);
    void *key = ccc_impl_fhm_key_in_slot(&h->impl_, ret);
    struct ccc_entry_ const ent = ccc_impl_fhm_find(
        &h->impl_, key, ccc_impl_fhm_filter(&h->impl_, key));
    if (ent.stats_ == CCC_ENTRY_VACANT || !ret || !ent.e_)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
    }
    memcpy(ret, ent.e_, ccc_buf_elem_size(&h->impl_.buf_));
    erase(&h->impl_, ent.e_);
    return (ccc_entry){{.e_ = ret, .stats_ = CCC_ENTRY_OCCUPIED}};
}

void *
ccc_fhm_or_insert(ccc_fh_map_entry const *const e, ccc_fh_map_elem *const elem)
{
    if (e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return e->impl_.entry_.e_;
    }
    if (e->impl_.entry_.stats_ & ~CCC_ENTRY_VACANT)
    {
        return NULL;
    }
    void *user_struct = struct_base(e->impl_.h_, &elem->impl_);
    elem->impl_.hash_ = e->impl_.hash_;
    ccc_impl_fhm_insert(
        e->impl_.h_, user_struct, elem->impl_.hash_,
        ccc_buf_index_of(&e->impl_.h_->buf_, e->impl_.entry_.e_));
    return e->impl_.entry_.e_;
}

void const *
ccc_fhm_unwrap(ccc_fh_map_entry const *const e)
{
    if (!(e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED))
    {
        return NULL;
    }
    return e->impl_.entry_.e_;
}

bool
ccc_fhm_occupied(ccc_fh_map_entry const *const e)
{
    return e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED;
}

bool
ccc_fhm_insert_error(ccc_fh_map_entry const *const e)
{
    return e->impl_.entry_.stats_ & CCC_ENTRY_INSERT_ERROR;
}

void *
ccc_fhm_begin(ccc_flat_hash_map const *const h)
{
    if (ccc_buf_empty(&h->impl_.buf_))
    {
        return NULL;
    }
    void *iter = ccc_buf_begin(&h->impl_.buf_);
    for (; iter != ccc_buf_capacity_end(&h->impl_.buf_)
           && ccc_impl_fhm_in_slot(&h->impl_, iter)->hash_ == CCC_FHM_EMPTY;
         iter = ccc_buf_next(&h->impl_.buf_, iter))
    {}
    return iter == ccc_buf_capacity_end(&h->impl_.buf_) ? NULL : iter;
}

void *
ccc_fhm_next(ccc_flat_hash_map const *const h, ccc_fh_map_elem const *iter)
{
    void *i = struct_base(&h->impl_, &iter->impl_);
    for (i = ccc_buf_next(&h->impl_.buf_, i);
         i != ccc_buf_capacity_end(&h->impl_.buf_)
         && ccc_impl_fhm_in_slot(&h->impl_, i)->hash_ == CCC_FHM_EMPTY;
         i = ccc_buf_next(&h->impl_.buf_, i))
    {}
    return i == ccc_buf_capacity_end(&h->impl_.buf_) ? NULL : i;
}

void *
ccc_fhm_end([[maybe_unused]] ccc_flat_hash_map const *const h)
{
    return NULL;
}

static struct ccc_entry_
entry(struct ccc_fhm_ *const h, void const *key, uint64_t const hash)
{
    uint8_t upcoming_insertion_error = 0;
    if (ccc_impl_fhm_maybe_resize(h) != CCC_OK)
    {
        upcoming_insertion_error = CCC_ENTRY_INSERT_ERROR;
    }
    struct ccc_entry_ res = ccc_impl_fhm_find(h, key, hash);
    res.stats_ |= upcoming_insertion_error;
    return res;
}

struct ccc_entry_
ccc_impl_fhm_find(struct ccc_fhm_ const *const h, void const *const key,
                  uint64_t const hash)
{
    size_t const cap = ccc_buf_capacity(&h->buf_);
    assert(cap);
    size_t cur_i = to_index(cap, hash);
    for (size_t dist = 0; dist < cap;
         ++dist, cur_i = ccc_impl_fhm_increment(cap, cur_i))
    {
        void *slot = ccc_buf_at(&h->buf_, cur_i);
        struct ccc_fhm_elem_ *const e = ccc_impl_fhm_in_slot(h, slot);
        if (e->hash_ == CCC_FHM_EMPTY)
        {
            return (struct ccc_entry_){.e_ = slot, .stats_ = CCC_ENTRY_VACANT};
        }
        if (dist > ccc_impl_fhm_distance(cap, cur_i, to_index(cap, e->hash_)))
        {
            return (struct ccc_entry_){.e_ = slot, .stats_ = CCC_ENTRY_VACANT};
        }
        if (hash == e->hash_
            && h->eq_fn_(
                &(ccc_key_cmp){.container = slot, .key = key, .aux = h->aux_}))
        {
            return (struct ccc_entry_){.e_ = slot,
                                       .stats_ = CCC_ENTRY_OCCUPIED};
        }
    }
    return (struct ccc_entry_){
        .e_ = NULL, .stats_ = CCC_ENTRY_VACANT | CCC_ENTRY_CONTAINS_NULL};
}

/* Assumes that element to be inserted does not already exist in the table.
   Assumes that the table has room for another insertion. It is undefined to
   use this if the element has not been membership tested yet or the table
   is full. */
void
ccc_impl_fhm_insert(struct ccc_fhm_ *const h, void const *const e,
                    uint64_t const hash, size_t cur_i)
{
    size_t const elem_sz = ccc_buf_elem_size(&h->buf_);
    size_t const cap = ccc_buf_capacity(&h->buf_);
    void *floater = ccc_buf_at(&h->buf_, 0);
    (void)memcpy(floater, e, elem_sz);

    /* This function cannot modify e and e may be copied over to new
       insertion from old table. So should this function invariantly assign
       starting hash to this slot copy for insertion? I think yes so far. */
    ccc_impl_fhm_in_slot(h, floater)->hash_ = hash;
    size_t dist = ccc_impl_fhm_distance(cap, cur_i, to_index(cap, hash));
    for (;; cur_i = ccc_impl_fhm_increment(cap, cur_i), ++dist)
    {
        void *const slot = ccc_buf_at(&h->buf_, cur_i);
        struct ccc_fhm_elem_ const *slot_hash = ccc_impl_fhm_in_slot(h, slot);
        if (slot_hash->hash_ == CCC_FHM_EMPTY)
        {
            memcpy(slot, floater, elem_sz);
            ccc_buf_size_plus(&h->buf_);
            *ccc_impl_fhm_hash_at(h, 0) = CCC_FHM_EMPTY;
            *ccc_impl_fhm_hash_at(h, 1) = CCC_FHM_EMPTY;
            return;
        }
        size_t const slot_dist = ccc_impl_fhm_distance(
            cap, cur_i, to_index(cap, slot_hash->hash_));
        if (dist > slot_dist)
        {
            void *tmp = ccc_buf_at(&h->buf_, 1);
            swap(tmp, floater, slot, elem_sz);
            dist = slot_dist;
        }
    }
}

static void
erase(struct ccc_fhm_ *const h, void *const e)
{
    size_t const cap = ccc_buf_capacity(&h->buf_);
    size_t const elem_sz = ccc_buf_elem_size(&h->buf_);
    size_t stopped_at = ccc_buf_index_of(&h->buf_, e);
    *ccc_impl_fhm_hash_at(h, stopped_at) = CCC_FHM_EMPTY;
    size_t next = ccc_impl_fhm_increment(cap, stopped_at);
    void *tmp = ccc_buf_at(&h->buf_, 0);
    for (;; stopped_at = ccc_impl_fhm_increment(cap, stopped_at),
            next = ccc_impl_fhm_increment(cap, next))
    {
        void *next_slot = ccc_buf_at(&h->buf_, next);
        struct ccc_fhm_elem_ *next_elem = ccc_impl_fhm_in_slot(h, next_slot);
        if (next_elem->hash_ == CCC_FHM_EMPTY
            || !ccc_impl_fhm_distance(cap, next,
                                      to_index(cap, next_elem->hash_)))
        {
            break;
        }
        swap(tmp, next_slot, ccc_buf_at(&h->buf_, stopped_at), elem_sz);
    }
    *ccc_impl_fhm_hash_at(h, 0) = CCC_FHM_EMPTY;
    ccc_buf_size_minus(&h->buf_);
}

ccc_result
ccc_impl_fhm_maybe_resize(struct ccc_fhm_ *h)
{
    if ((double)(ccc_buf_size(&h->buf_) + num_swap_slots)
            / (double)ccc_buf_capacity(&h->buf_)
        <= load_factor)
    {
        return CCC_OK;
    }
    if (!h->buf_.impl_.alloc_)
    {
        return CCC_NO_REALLOC;
    }
    struct ccc_fhm_ new_hash = *h;
    new_hash.buf_.impl_.sz_ = 0;
    new_hash.buf_.impl_.capacity_
        = new_hash.buf_.impl_.capacity_
              ? ccc_fhm_next_prime(ccc_buf_size(&h->buf_) * 2)
              : default_prime;
    new_hash.buf_.impl_.mem_ = new_hash.buf_.impl_.alloc_(
        NULL, ccc_buf_elem_size(&h->buf_) * new_hash.buf_.impl_.capacity_);
    if (!new_hash.buf_.impl_.mem_)
    {
        return CCC_MEM_ERR;
    }
    /* Empty is intentionally chosen as zero so every byte is just set to
       0 in this new array. */
    memset(ccc_buf_base(&new_hash.buf_), CCC_FHM_EMPTY,
           ccc_buf_capacity(&new_hash.buf_)
               * ccc_buf_elem_size(&new_hash.buf_));
    for (void *slot = ccc_buf_begin(&h->buf_);
         slot != ccc_buf_capacity_end(&h->buf_);
         slot = ccc_buf_next(&h->buf_, slot))
    {
        struct ccc_fhm_elem_ const *const e = ccc_impl_fhm_in_slot(h, slot);
        if (e->hash_ != CCC_FHM_EMPTY)
        {
            struct ccc_entry_ new_ent = ccc_impl_fhm_find(
                &new_hash, ccc_impl_fhm_key_in_slot(h, slot), e->hash_);
            ccc_impl_fhm_insert(&new_hash, slot, e->hash_,
                                ccc_buf_index_of(&new_hash.buf_, new_ent.e_));
        }
    }
    (void)ccc_buf_realloc(&h->buf_, 0, h->buf_.impl_.alloc_);
    *h = new_hash;
    return CCC_OK;
}

void
ccc_fhm_print(ccc_flat_hash_map const *h, ccc_print_fn *fn)
{
    for (void const *i = ccc_buf_begin(&h->impl_.buf_);
         i != ccc_buf_capacity_end(&h->impl_.buf_);
         i = ccc_buf_next(&h->impl_.buf_, i))
    {
        if (ccc_impl_fhm_in_slot(&h->impl_, i)->hash_ != CCC_FHM_EMPTY)
        {
            fn(i);
        }
    }
}

size_t
ccc_fhm_next_prime(size_t n)
{
    if (n <= 1)
    {
        return 2;
    }
    while (!is_prime(++n))
    {}
    return n;
}

void
ccc_fhm_clear(ccc_flat_hash_map *const h, ccc_destructor_fn *const fn)
{
    if (!fn)
    {
        h->impl_.buf_.impl_.sz_ = 0;
        return;
    }
    for (void *slot = ccc_buf_begin(&h->impl_.buf_);
         slot != ccc_buf_capacity_end(&h->impl_.buf_);
         slot = ccc_buf_next(&h->impl_.buf_, slot))
    {
        if (ccc_impl_fhm_in_slot(&h->impl_, slot)->hash_ != CCC_FHM_EMPTY)
        {
            fn(slot);
        }
    }
}

ccc_result
ccc_fhm_clear_and_free(ccc_flat_hash_map *const h, ccc_destructor_fn *const fn)
{
    if (!fn)
    {
        h->impl_.buf_.impl_.sz_ = 0;
        return ccc_buf_free(&h->impl_.buf_, h->impl_.buf_.impl_.alloc_);
    }
    for (void *slot = ccc_buf_begin(&h->impl_.buf_);
         slot != ccc_buf_capacity_end(&h->impl_.buf_);
         slot = ccc_buf_next(&h->impl_.buf_, slot))
    {
        if (ccc_impl_fhm_in_slot(&h->impl_, slot)->hash_ != CCC_FHM_EMPTY)
        {
            fn(slot);
        }
    }
    return ccc_buf_free(&h->impl_.buf_, h->impl_.buf_.impl_.alloc_);
}

size_t
ccc_fhm_capacity(ccc_flat_hash_map const *const h)
{
    return ccc_buf_capacity(&h->impl_.buf_);
}

inline struct ccc_fhm_elem_ *
ccc_impl_fhm_in_slot(struct ccc_fhm_ const *const h, void const *const slot)
{
    return (struct ccc_fhm_elem_ *)((uint8_t *)slot + h->hash_elem_offset_);
}

void *
ccc_impl_fhm_key_in_slot(struct ccc_fhm_ const *h, void const *slot)
{
    return (uint8_t *)slot + h->key_offset_;
}

inline size_t
ccc_impl_fhm_distance(size_t const capacity, size_t const i, size_t const j)
{
    return i < j ? (capacity - j) + i - num_swap_slots : i - j;
}

bool
ccc_fhm_validate(ccc_flat_hash_map const *const h)
{
    size_t empties = 0;
    size_t occupied = 0;
    for (void const *i = ccc_buf_begin(&h->impl_.buf_);
         i != ccc_buf_capacity_end(&h->impl_.buf_);
         i = ccc_buf_next(&h->impl_.buf_, i))
    {
        if (ccc_impl_fhm_in_slot(&h->impl_, i)->hash_ == CCC_FHM_EMPTY)
        {
            ++empties;
        }
        else
        {
            ++occupied;
        }
        if (ccc_impl_fhm_in_slot(&h->impl_, i)->hash_ != CCC_FHM_EMPTY
            && !valid_distance_from_home(&h->impl_, i))
        {
            return false;
        }
    }
    return occupied == ccc_fhm_size(h)
           && empties == (ccc_buf_capacity(&h->impl_.buf_) - occupied);
}

void *
ccc_impl_fhm_base(struct ccc_fhm_ const *const h)
{
    return ccc_buf_base(&h->buf_);
}

/*=========================   Static Helpers ============================*/

size_t
ccc_impl_fhm_increment(size_t const capacity, size_t i)
{
    i = (i + 1) % capacity;
    return i <= last_swap_slot ? last_swap_slot + 1 : i;
}

static size_t
decrement(size_t const capacity, size_t i)
{
    i = i ? i - 1 : capacity - 1;
    return i <= last_swap_slot ? capacity - 1 : i;
}

static size_t
to_index(size_t const capacity, uint64_t hash)
{
    hash = hash % capacity;
    return hash <= last_swap_slot ? last_swap_slot + 1 : hash;
}

static void *
struct_base(struct ccc_fhm_ const *const h, struct ccc_fhm_elem_ const *const e)
{
    return ((uint8_t *)&e->hash_) - h->hash_elem_offset_;
}

static inline void
swap(uint8_t tmp[], void *const a, void *const b, size_t ab_size)
{
    if (a == b)
    {
        return;
    }
    (void)memcpy(tmp, a, ab_size);
    (void)memcpy(a, b, ab_size);
    (void)memcpy(b, tmp, ab_size);
}

inline uint64_t *
ccc_impl_fhm_hash_at(struct ccc_fhm_ const *const h, size_t const i)
{
    return &((struct ccc_fhm_elem_ *)((uint8_t *)ccc_buf_at(&h->buf_, i)
                                      + h->hash_elem_offset_))
                ->hash_;
}

inline uint64_t
ccc_impl_fhm_filter(struct ccc_fhm_ const *const h, void const *const key)
{
    uint64_t const hash = h->hash_fn_(key);
    return hash == CCC_FHM_EMPTY ? hash + 1 : hash;
}

static inline bool
is_prime(size_t const n)
{
    if (n <= 1)
    {
        return false;
    }
    if (n <= 3)
    {
        return true;
    }
    if (n % 2 == 0 || n % 3 == 0)
    {
        return false;
    }
    for (size_t i = 5; i * i <= n; i += 6)
    {
        if (n % i == 0 || n % (i + 2) == 0)
        {
            return false;
        }
    }
    return true;
}

static bool
valid_distance_from_home(struct ccc_fhm_ const *h, void const *slot)
{
    size_t const cap = ccc_buf_capacity(&h->buf_);
    uint64_t const hash = ccc_impl_fhm_in_slot(h, slot)->hash_;
    size_t const home = to_index(cap, hash);
    size_t const end = decrement(cap, home);
    for (size_t i = ccc_buf_index_of(&h->buf_, slot),
                distance_to_home
                = ccc_impl_fhm_distance(cap, i, to_index(cap, hash));
         i != end; --distance_to_home, i = decrement(cap, i))
    {
        uint64_t const cur_hash = *ccc_impl_fhm_hash_at(h, i);
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
        if (distance_to_home
            > ccc_impl_fhm_distance(cap, i, to_index(cap, cur_hash)))
        {
            return false;
        }
    }
    return true;
}
