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

static uint64_t const empty = -1;
static double const load_factor = 0.8;

static struct query find(struct ccc_impl_flat_hash const *, void const *,
                         uint64_t);
static size_t distance(size_t capacity, size_t index, uint64_t hash);
static bool is_prime(size_t);
static size_t next_prime(size_t);
static ccc_result maybe_resize(struct ccc_impl_flat_hash *);
static void insert(struct ccc_impl_flat_hash *, void const *, uint64_t hash);
static void swap(uint8_t tmp[], void *, void *, size_t);
static ccc_fhash_elem *hash_in_slot(struct ccc_impl_flat_hash const *,
                                    void const *slot);
static uint64_t *hash_at(struct ccc_impl_flat_hash const *, size_t i);

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
        hash_in_slot(&h->impl, i)->impl.hash = empty;
    }
    return CCC_OK;
}

bool
ccc_fhash_empty(ccc_flat_hash const *const h)
{
    return !ccc_buf_size(h->impl.buf);
}

bool
ccc_fhash_contains(ccc_flat_hash const *const h, void const *const e)
{
    if (ccc_buf_empty(h->impl.buf))
    {
        return false;
    }
    return find(&h->impl, e, h->impl.hash_fn(e)).found;
}

size_t
ccc_fhash_size(ccc_flat_hash const *const h)
{
    return ccc_buf_size(h->impl.buf);
}

void const *
ccc_fhash_find(ccc_flat_hash const *const h, void const *const e)
{
    if (ccc_buf_empty(h->impl.buf))
    {
        return NULL;
    }
    struct query const q = find(&h->impl, e, h->impl.hash_fn(e));
    if (!q.found)
    {
        return NULL;
    }
    return ccc_buf_at(h->impl.buf, q.stopped_at);
}

ccc_result
ccc_fhash_insert(ccc_flat_hash *const h, void const *const e)
{
    ccc_result const res = maybe_resize(&h->impl);
    if (res != CCC_OK)
    {
        return res;
    }
    uint64_t const hash = h->impl.hash_fn(e);
    struct query const q = find(&h->impl, e, hash);
    if (q.found)
    {
        return CCC_NOP;
    }
    insert(&h->impl, e, hash);
    return CCC_OK;
}

ccc_result
ccc_fhash_erase(ccc_flat_hash *const h, void const *const e)
{
    if (ccc_buf_empty(h->impl.buf))
    {
        return CCC_NOP;
    }
    size_t const cap = ccc_buf_capacity(h->impl.buf);
    uint64_t const hash = h->impl.hash_fn(e);
    struct query q = find(&h->impl, e, hash);
    if (!q.found)
    {
        return CCC_NOP;
    }
    size_t const elem_sz = ccc_buf_elem_size(h->impl.buf);
    *hash_at(&h->impl, q.stopped_at) = empty;
    size_t next = (hash + 1) % cap;
    uint8_t tmp[ccc_buf_elem_size(h->impl.buf)];
    for (;; q.stopped_at = (q.stopped_at + 1) % cap, next = (next + 1) % cap)
    {
        void *next_slot = ccc_buf_at(h->impl.buf, next);
        ccc_fhash_elem *next_elem = hash_in_slot(&h->impl, next_slot);
        if (!distance(cap, next, next_elem->impl.hash))
        {
            break;
        }
        swap(tmp, next_slot, ccc_buf_at(h->impl.buf, q.stopped_at), elem_sz);
    }
    --h->impl.buf->impl.sz;
    return CCC_OK;
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
           && hash_in_slot(&h->impl, iter)->impl.hash == empty;
         iter = ccc_buf_next(h->impl.buf, iter))
    {}
    return iter;
}

void const *
ccc_fhash_next(ccc_flat_hash const *const h, void const *iter)
{
    for (iter = ccc_buf_next(h->impl.buf, iter);
         iter != ccc_buf_capacity_end(h->impl.buf)
         && hash_in_slot(&h->impl, iter)->impl.hash == empty;
         iter = ccc_buf_next(h->impl.buf, iter))
    {}
    return iter;
}

void const *
ccc_fhash_end(ccc_flat_hash const *const h)
{
    return ccc_buf_capacity_end(h->impl.buf);
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
    hash_in_slot(h, slot_copy)->impl.hash = hash;
    size_t i = hash % cap;
    size_t dist = 0;
    uint8_t tmp[elem_sz];
    for (;; i = (i + 1) % cap, ++dist)
    {
        void *const slot = ccc_buf_at(h->buf, i);
        ccc_fhash_elem const *slot_hash = hash_in_slot(h, slot);
        if (slot_hash->impl.hash == empty)
        {
            memcpy(slot, slot_copy, elem_sz);
            ++h->buf->impl.sz;
            return;
        }
        if (dist > distance(cap, i, slot_hash->impl.hash))
        {
            swap(tmp, slot_copy, slot, elem_sz);
        }
    }
}

static struct query
find(struct ccc_impl_flat_hash const *const h, void const *const elem,
     uint64_t const hash)
{
    size_t const cap = ccc_buf_capacity(h->buf);
    size_t cur_i = hash % cap;
    for (size_t dist = 0; dist < cap; ++dist, cur_i = (cur_i + 1) % cap)
    {
        void const *const slot = ccc_buf_at(h->buf, cur_i);
        ccc_fhash_elem const *const e = hash_in_slot(h, slot);
        if (e->impl.hash == empty)
        {
            return (struct query){.found = false, .stopped_at = cur_i};
        }
        if (dist > distance(cap, cur_i, e->impl.hash))
        {
            return (struct query){.found = false, .stopped_at = cur_i};
        }
        if (hash == e->impl.hash && h->eq_fn(slot, elem, h->aux))
        {
            return (struct query){.found = true, .stopped_at = cur_i};
        }
    }
    return (struct query){.found = false, .stopped_at = cur_i};
}

static ccc_result
maybe_resize(struct ccc_impl_flat_hash *h)
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
        hash_in_slot(h, i)->impl.hash = empty;
    }
    for (void *slot = ccc_buf_begin(h->buf);
         slot != ccc_buf_capacity_end(h->buf);
         slot = ccc_buf_next(h->buf, slot))
    {
        ccc_fhash_elem const *const e = hash_in_slot(h, slot);
        if (e->impl.hash != empty)
        {
            insert(&new_hash, slot, e->impl.hash);
        }
    }
    *h = new_hash;
    return CCC_OK;
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

static inline size_t
distance(size_t const capacity, size_t const index, uint64_t hash)
{
    hash %= capacity;
    return index < hash ? (capacity - hash) + index : hash - index;
}

static inline ccc_fhash_elem *
hash_in_slot(struct ccc_impl_flat_hash const *const h, void const *const slot)
{
    return (ccc_fhash_elem *)((uint8_t *)slot + h->hash_elem_offset);
}

static inline uint64_t *
hash_at(struct ccc_impl_flat_hash const *const h, size_t const i)
{
    return &((struct ccc_impl_fhash_elem *)((uint8_t *)ccc_buf_at(h->buf, i)
                                            + h->hash_elem_offset))
                ->hash;
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
