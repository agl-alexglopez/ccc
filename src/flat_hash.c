#include "flat_hash.h"
#include "buf.h"
#include "impl_flat_hash.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct query
{
    bool found;
    size_t stopped_at;
};

static double const load_factor = 0.8;

static void insert(struct ccc_impl_flat_hash *, void const *, uint64_t hash);
static void *erase(struct ccc_impl_flat_hash *, void const *, uint64_t hash);

static uint64_t hash_filter(struct ccc_impl_flat_hash const *,
                            void const *user_struct);
static bool is_prime(size_t);
static size_t next_prime(size_t);
static void swap(uint8_t tmp[], void *, void *, size_t);
static uint64_t *hash_at(struct ccc_impl_flat_hash const *, size_t i);
static void *struct_base(struct ccc_impl_flat_hash const *,
                         struct ccc_impl_fhash_elem const *);

ccc_result
ccc_fhash_init(ccc_flat_hash *const h, ccc_buf *const buf,
               size_t const hash_elem_offset, ccc_hash_fn *const hash_fn,
               ccc_eq_fn *const eq_fn, void *const aux)
{
    if (!h || !buf || !hash_fn || !eq_fn || !ccc_buf_capacity(buf))
    {
        return CCC_MEM_ERR;
    }
    h->impl.buf = buf;
    h->impl.hash_elem_offset = hash_elem_offset;
    h->impl.hash_fn = hash_fn;
    h->impl.eq_fn = eq_fn;
    h->impl.aux = aux;
    for (void *i = ccc_buf_begin(h->impl.buf);
         i != ccc_buf_capacity_end(h->impl.buf);
         i = ccc_buf_next(h->impl.buf, i))
    {
        ccc_impl_fhash_in_slot(&h->impl, i)->hash = EMPTY;
    }
    return CCC_OK;
}

bool
ccc_fhash_empty(ccc_flat_hash const *const h)
{
    return !ccc_buf_size(h->impl.buf);
}

bool
ccc_fhash_contains(ccc_flat_hash *const h, ccc_fhash_elem *const elem)
{
    return ccc_fhash_entry(h, elem).impl.entry.occupied;
}

size_t
ccc_fhash_size(ccc_flat_hash const *const h)
{
    return ccc_buf_size(h->impl.buf);
}

ccc_flat_hash_entry
ccc_fhash_entry(ccc_flat_hash *h, ccc_fhash_elem *const elem)
{
    void *const e = struct_base(&h->impl, &elem->impl);
    elem->impl.hash = hash_filter(&h->impl, e);
    return (ccc_flat_hash_entry){
        {
            .h = &h->impl,
            .query = &elem->impl,
            .entry = ccc_impl_fhash_find(&h->impl, e, elem->impl.hash),
        },
    };
}

void *
ccc_fhash_or_insert(ccc_flat_hash_entry h)
{
    if (h.impl.entry.occupied)
    {
        return h.impl.entry.entry;
    }
    ccc_result const res = ccc_impl_fhash_maybe_resize(h.impl.h);
    if (res != CCC_OK)
    {
        return NULL;
    }
    void *e = struct_base(h.impl.h, h.impl.query);
    h.impl.query->hash = hash_filter(h.impl.h, e);
    insert(h.impl.h, e, h.impl.query->hash);
    return h.impl.entry.entry;
}

void *
ccc_fhash_and_erase(ccc_flat_hash_entry h)
{
    if (!h.impl.entry.occupied)
    {
        return NULL;
    }
    return erase(h.impl.h, h.impl.entry.entry, h.impl.query->hash);
}

void *
ccc_fhash_get(ccc_flat_hash_entry e)
{
    if (!e.impl.entry.occupied)
    {
        return NULL;
    }
    return e.impl.entry.entry;
}

void const *
ccc_fhash_begin(ccc_flat_hash const *const h)
{
    if (ccc_buf_empty(h->impl.buf))
    {
        return NULL;
    }
    void const *iter = ccc_buf_begin(h->impl.buf);
    for (; iter != ccc_buf_capacity_end(h->impl.buf)
           && ccc_impl_fhash_in_slot(&h->impl, iter)->hash == EMPTY;
         iter = ccc_buf_next(h->impl.buf, iter))
    {}
    return iter;
}

void const *
ccc_fhash_next(ccc_flat_hash const *const h, ccc_fhash_elem const *iter)
{
    void const *i = struct_base(&h->impl, &iter->impl);
    for (i = ccc_buf_next(h->impl.buf, i);
         i != ccc_buf_capacity_end(h->impl.buf)
         && ccc_impl_fhash_in_slot(&h->impl, i)->hash == EMPTY;
         i = ccc_buf_next(h->impl.buf, i))
    {}
    return i;
}

void const *
ccc_fhash_end(ccc_flat_hash const *const h)
{
    return ccc_buf_capacity_end(h->impl.buf);
}

ccc_entry
ccc_impl_fhash_find(struct ccc_impl_flat_hash const *const h,
                    void const *const elem, uint64_t const hash)
{
    size_t const cap = ccc_buf_capacity(h->buf);
    size_t cur_i = hash % cap;
    void *slot = ccc_buf_at(h->buf, cur_i);
    for (size_t dist = 0; dist < cap;
         ++dist, cur_i = (cur_i + 1) % cap, slot = ccc_buf_at(h->buf, cur_i))
    {
        struct ccc_impl_fhash_elem *const e = ccc_impl_fhash_in_slot(h, slot);
        if (e->hash == EMPTY)
        {
            return (ccc_entry){.occupied = false, .entry = slot};
        }
        if (dist > ccc_impl_fhash_distance(cap, cur_i, e->hash))
        {
            return (ccc_entry){.occupied = false, .entry = slot};
        }
        if (hash == e->hash && h->eq_fn(slot, elem, h->aux))
        {
            return (ccc_entry){.occupied = true, .entry = slot};
        }
    }
    return (ccc_entry){.occupied = false, .entry = slot};
}

inline struct ccc_impl_fhash_elem *
ccc_impl_fhash_in_slot(struct ccc_impl_flat_hash const *const h,
                       void const *const slot)
{
    return (struct ccc_impl_fhash_elem *)((uint8_t *)slot
                                          + h->hash_elem_offset);
}

inline size_t
ccc_impl_fhash_distance(size_t const capacity, size_t const index,
                        uint64_t hash)
{
    hash %= capacity;
    return index < hash ? (capacity - hash) + index : hash - index;
}

ccc_result
ccc_impl_fhash_maybe_resize(struct ccc_impl_flat_hash *h)
{
    if ((double)ccc_buf_size(h->buf) / (double)ccc_buf_capacity(h->buf)
        <= load_factor)
    {
        return CCC_OK;
    }
    if (!h->buf->impl.realloc_fn)
    {
        return CCC_NO_REALLOC;
    }
    ccc_buf new_buf = *h->buf;
    new_buf.impl.capacity = next_prime(ccc_buf_size(h->buf) * 2);
    new_buf.impl.mem = h->buf->impl.realloc_fn(NULL, new_buf.impl.capacity);
    if (!new_buf.impl.mem)
    {
        return CCC_MEM_ERR;
    }
    struct ccc_impl_flat_hash new_hash = *h;
    new_hash.buf = &new_buf;
    for (void *i = ccc_buf_begin(&new_buf); i != ccc_buf_capacity_end(&new_buf);
         i = ccc_buf_next(&new_buf, i))
    {
        ccc_impl_fhash_in_slot(h, i)->hash = EMPTY;
    }
    for (void *slot = ccc_buf_begin(h->buf);
         slot != ccc_buf_capacity_end(h->buf);
         slot = ccc_buf_next(h->buf, slot))
    {
        struct ccc_impl_fhash_elem const *const e
            = ccc_impl_fhash_in_slot(h, slot);
        if (e->hash != EMPTY)
        {
            insert(&new_hash, slot, e->hash);
        }
    }
    (void)ccc_buf_realloc(h->buf, 0, h->buf->impl.realloc_fn);
    *h = new_hash;
    return CCC_OK;
}

/*=========================   Static Helpers    ============================*/

/* Assumes that element to be inserted does not already exist in the table.
   Assumes that the table has room for another insertion. It is undefined to
   use this if the element has not been membership tested yet or the table
   is full. */
static void
insert(struct ccc_impl_flat_hash *const h, void const *const e,
       uint64_t const hash)
{
    size_t const elem_sz = ccc_buf_elem_size(h->buf);
    size_t const cap = ccc_buf_capacity(h->buf);
    uint8_t slot_copy[elem_sz];
    (void)memcpy(slot_copy, e, elem_sz);

    /* This function cannot modify e and e may be copied over to new insertion
       from old table. So should this function invariantly assign starting
       hash to this slot copy for insertion? I think yes so far. */
    ccc_impl_fhash_in_slot(h, slot_copy)->hash = hash;
    size_t i = hash % cap;
    size_t dist = 0;
    uint8_t tmp[elem_sz];
    for (;; i = (i + 1) % cap, ++dist)
    {
        void *const slot = ccc_buf_at(h->buf, i);
        struct ccc_impl_fhash_elem const *slot_hash
            = ccc_impl_fhash_in_slot(h, slot);
        if (slot_hash->hash == EMPTY)
        {
            memcpy(slot, slot_copy, elem_sz);
            ++h->buf->impl.sz;
            return;
        }
        if (dist > ccc_impl_fhash_distance(cap, i, slot_hash->hash))
        {
            swap(tmp, slot_copy, slot, elem_sz);
        }
    }
}

static void *
erase(struct ccc_impl_flat_hash *const h, void const *const e,
      uint64_t const hash)
{
    size_t const cap = ccc_buf_capacity(h->buf);
    size_t stopped_at = ccc_buf_index_of(h->buf, e);
    size_t const elem_sz = ccc_buf_elem_size(h->buf);
    *hash_at(h, stopped_at) = EMPTY;
    size_t next = (hash + 1) % cap;
    uint8_t tmp[ccc_buf_elem_size(h->buf)];
    for (;; stopped_at = (stopped_at + 1) % cap, next = (next + 1) % cap)
    {
        void *next_slot = ccc_buf_at(h->buf, next);
        struct ccc_impl_fhash_elem *next_elem
            = ccc_impl_fhash_in_slot(h, next_slot);
        if (!ccc_impl_fhash_distance(cap, next, next_elem->hash))
        {
            break;
        }
        swap(tmp, next_slot, ccc_buf_at(h->buf, stopped_at), elem_sz);
    }
    --h->buf->impl.sz;
    return ccc_buf_at(h->buf, stopped_at);
}

static void *
struct_base(struct ccc_impl_flat_hash const *const h,
            struct ccc_impl_fhash_elem const *const e)
{
    return ((uint8_t *)&e->hash) - h->hash_elem_offset;
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

static inline uint64_t *
hash_at(struct ccc_impl_flat_hash const *const h, size_t const i)
{
    return &((struct ccc_impl_fhash_elem *)((uint8_t *)ccc_buf_at(h->buf, i)
                                            + h->hash_elem_offset))
                ->hash;
}

static inline uint64_t
hash_filter(struct ccc_impl_flat_hash const *const h,
            void const *const user_struct)
{
    uint64_t const hash = h->hash_fn(user_struct);
    return hash == EMPTY ? hash - 1 : hash;
}

static inline size_t
next_prime(size_t n)
{
    if (n <= 1)
    {
        return 2;
    }
    while (!is_prime(++n))
    {}
    return n;
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
