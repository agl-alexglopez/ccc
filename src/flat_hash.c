#include "flat_hash.h"
#include "buf.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct query
{
    bool found;
    size_t stopped_at;
};

static int64_t const empty = -1;
static int64_t const force_positive_mask = ~INT64_MIN;
static double const load_factor = 0.8;

static struct query find(ccc_hash const *, void const *, int64_t);
static size_t distance(size_t capacity, size_t index, int64_t hash);
static bool is_prime(size_t);
static size_t next_prime(size_t);
static ccc_hash_result maybe_resize(ccc_hash *);
static void insert(ccc_hash *, void const *, int64_t hash);
static void swap(uint8_t tmp[], void *, void *, size_t);
static ccc_hash_elem *hash_in_slot(ccc_hash const *, void const *slot);
static int64_t *hash_at(ccc_hash const *, size_t i);
static int64_t hash_data(ccc_hash const *, void const *);

ccc_hash_result
ccc_hash_init(ccc_hash *const h, ccc_buf *const buf,
              size_t const hash_elem_offset, ccc_hash_fn *const hash_fn,
              ccc_hash_eq_fn *const eq_fn, void *const aux)
{
    if (!h || !buf || !hash_fn || !eq_fn || ccc_buf_empty(buf))
    {
        return CCC_HASH_ERR;
    }
    h->buf = buf;
    h->hash_elem_offset = hash_elem_offset;
    h->hash_fn = hash_fn;
    h->eq_fn = eq_fn;
    h->aux = aux;
    for (void *i = ccc_buf_begin(h->buf); i != ccc_buf_capacity_end(h->buf);
         i = ccc_buf_next(h->buf, i))
    {
        hash_in_slot(h, i)->hash = empty;
    }
    return CCC_HASH_OK;
}

bool
ccc_hash_contains(ccc_hash const *const h, void const *const e)
{
    if (ccc_buf_empty(h->buf))
    {
        return false;
    }
    return find(h, e, hash_data(h, e)).found;
}

void const *
ccc_hash_find(ccc_hash const *const h, void const *const e)
{
    if (ccc_buf_empty(h->buf))
    {
        return NULL;
    }
    struct query const q = find(h, e, hash_data(h, e));
    if (!q.found)
    {
        return NULL;
    }
    return ccc_buf_at(h->buf, q.stopped_at);
}

ccc_hash_result
ccc_hash_insert(ccc_hash *const h, void const *const e)
{
    ccc_hash_result const res = maybe_resize(h);
    if (res != CCC_HASH_OK)
    {
        return res;
    }
    int64_t const hash = hash_data(h, e);
    struct query const q = find(h, e, hash);
    if (q.found)
    {
        return CCC_HASH_NOP;
    }
    insert(h, e, hash);
    return CCC_HASH_OK;
}

ccc_hash_result
ccc_hash_erase(ccc_hash *const h, void const *const e)
{
    if (ccc_buf_empty(h->buf))
    {
        return CCC_HASH_NOP;
    }
    size_t const cap = ccc_buf_capacity(h->buf);
    int64_t const hash = hash_data(h, e);
    struct query q = find(h, e, hash);
    if (!q.found)
    {
        return CCC_HASH_NOP;
    }
    size_t const elem_sz = ccc_buf_elem_size(h->buf);
    *hash_at(h, q.stopped_at) = empty;
    size_t next = (hash + 1) % cap;
    uint8_t tmp[ccc_buf_elem_size(h->buf)];
    for (;; q.stopped_at = (q.stopped_at + 1) % cap, next = (next + 1) % cap)
    {
        void *next_slot = ccc_buf_at(h->buf, next);
        ccc_hash_elem *next_elem = hash_in_slot(h, next_slot);
        if (!distance(cap, next, next_elem->hash))
        {
            break;
        }
        swap(tmp, next_slot, ccc_buf_at(h->buf, q.stopped_at), elem_sz);
    }
    --h->buf->sz;
    return CCC_HASH_OK;
}

void const *
ccc_hash_begin(ccc_hash const *const h)
{
    if (ccc_buf_empty(h->buf))
    {
        return NULL;
    }
    void const *iter = ccc_buf_begin(h->buf);
    for (; iter != ccc_buf_capacity_end(h->buf)
           && hash_in_slot(h, iter)->hash == empty;
         iter = ccc_buf_next(h->buf, iter))
    {}
    return iter;
}

void const *
ccc_hash_next(ccc_hash const *const h, void const *iter)
{
    for (iter = ccc_buf_next(h->buf, iter);
         iter != ccc_buf_capacity_end(h->buf)
         && hash_in_slot(h, iter)->hash == empty;
         iter = ccc_buf_next(h->buf, iter))
    {}
    return iter;
}

void const *
ccc_hash_end(ccc_hash const *const h)
{
    return ccc_buf_capacity_end(h->buf);
}

/*=========================   Static Helpers    ============================*/

/* Assumes that element to be inserted does not already exist in the table.
   Assumes that the table has room for another insertion. It is undefined to
   use this if the element has not been membership tested yet or the table
   is full. */
static void
insert(ccc_hash *const h, void const *const e, int64_t const hash)
{
    size_t const elem_sz = ccc_buf_elem_size(h->buf);
    size_t const cap = ccc_buf_capacity(h->buf);
    uint8_t slot_copy[elem_sz];
    (void)memcpy(slot_copy, e, elem_sz);

    /* This function cannot modify e and e may be copied over to new insertion
       from old table. So should this function invariantly assign starting
       hash to this slot copy for insertion? I think yes so far. */
    hash_in_slot(h, slot_copy)->hash = hash;
    size_t i = hash % cap;
    size_t dist = 0;
    uint8_t tmp[elem_sz];
    for (;; i = (i + 1) % cap, ++dist)
    {
        void *const slot = ccc_buf_at(h->buf, i);
        ccc_hash_elem const *slot_hash = hash_in_slot(h, slot);
        if (slot_hash->hash == empty)
        {
            memcpy(slot, e, elem_sz);
            ++h->buf->sz;
            return;
        }
        if (dist > distance(cap, i, slot_hash->hash))
        {
            swap(tmp, slot_copy, slot, elem_sz);
        }
    }
}

static struct query
find(ccc_hash const *const h, void const *const elem, int64_t const hash)
{
    size_t const cap = ccc_buf_capacity(h->buf);
    size_t cur_i = hash % cap;
    for (size_t dist = 0; dist < cap; ++dist, cur_i = (cur_i + 1) % cap)
    {
        void const *const slot = ccc_buf_at(h->buf, cur_i);
        ccc_hash_elem const *const e = hash_in_slot(h, e);
        if (e->hash == empty)
        {
            return (struct query){.found = false, .stopped_at = cur_i};
        }
        if (dist > distance(cap, cur_i, e->hash))
        {
            return (struct query){.found = false, .stopped_at = cur_i};
        }
        if (hash == e->hash && h->eq_fn(slot, elem, h->aux))
        {
            return (struct query){.found = true, .stopped_at = cur_i};
        }
    }
    return (struct query){.found = false, .stopped_at = cur_i};
}

static ccc_hash_result
maybe_resize(ccc_hash *h)
{
    if ((double)ccc_buf_size(h->buf) / (double)ccc_buf_capacity(h->buf)
        <= load_factor)
    {
        return CCC_HASH_OK;
    }
    if (!h->buf->realloc_fn)
    {
        return CCC_HASH_FULL;
    }
    ccc_buf new_buf = *h->buf;
    new_buf.capacity = next_prime(ccc_buf_size(h->buf) * 2);
    new_buf.mem = h->buf->realloc_fn(NULL, new_buf.capacity);
    if (!new_buf.mem)
    {
        return CCC_HASH_ERR;
    }
    ccc_hash new_hash = *h;
    new_hash.buf = &new_buf;
    for (void *i = ccc_buf_begin(&new_buf); i != ccc_buf_capacity_end(&new_buf);
         i = ccc_buf_next(&new_buf, i))
    {
        hash_in_slot(h, i)->hash = empty;
    }
    for (void *slot = ccc_buf_begin(h->buf);
         slot != ccc_buf_capacity_end(h->buf);
         slot = ccc_buf_next(h->buf, slot))
    {
        ccc_hash_elem const *const e = hash_in_slot(h, slot);
        if (e->hash != empty)
        {
            insert(&new_hash, slot, e->hash);
        }
    }
    *h = new_hash;
    return CCC_HASH_OK;
}

static inline void
swap(uint8_t tmp[], void *const a, void *const b, size_t ab_size)
{
    assert(a != b);
    (void)memcpy(tmp, a, ab_size);
    (void)memcpy(a, b, ab_size);
    (void)memcpy(b, tmp, ab_size);
}

static inline size_t
distance(size_t const capacity, size_t const index, int64_t const hash)
{
    size_t hash_i = hash;
    hash_i %= capacity;
    return index < hash_i ? (capacity - hash_i) + index : hash_i - index;
}

static inline ccc_hash_elem *
hash_in_slot(ccc_hash const *const h, void const *const slot)
{
    return (ccc_hash_elem *)((uint8_t *)slot + h->hash_elem_offset);
}

static inline int64_t *
hash_at(ccc_hash const *const h, size_t const i)
{
    return &((ccc_hash_elem *)((uint8_t *)ccc_buf_at(h->buf, i)
                               + h->hash_elem_offset))
                ->hash;
}

static inline int64_t
hash_data(ccc_hash const *const h, void const *const data)
{
    return h->hash_fn(data) & force_positive_mask;
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
