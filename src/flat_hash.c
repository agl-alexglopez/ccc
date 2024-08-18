#include "flat_hash.h"

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

static size_t const empty = -1;
static double const load_factor = 0.8;

static struct query find(ccc_hash const *, void const *, size_t);
static size_t distance(ccc_hash const *, size_t, size_t);
static bool is_prime(size_t);
static size_t next_prime(size_t);
static ccc_hash_result maybe_resize(ccc_hash *);
static void insert(ccc_hash *, void const *, size_t);
static void swap(uint8_t tmp[], void *, void *, size_t);
static ccc_hash_elem *hash_in_slot(ccc_hash const *, void const *slot);

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
        ((ccc_hash_elem *)((uint8_t *)i + h->hash_elem_offset))->hash = empty;
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
    size_t const hash = h->hash_fn(e) % ccc_buf_capacity(h->buf);
    return find(h, e, hash).found;
}

void const *
ccc_hash_find(ccc_hash const *const h, void const *const e)
{
    if (ccc_buf_empty(h->buf))
    {
        return NULL;
    }
    size_t const hash = h->hash_fn(e) % ccc_buf_capacity(h->buf);
    struct query const q = find(h, e, hash);
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
    size_t const hash = h->hash_fn(e) % ccc_buf_capacity(h->buf);
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
    size_t const hash = h->hash_fn(e) % cap;
    struct query q = find(h, e, hash);
    if (!q.found)
    {
        return CCC_HASH_NOP;
    }
    size_t const elem_sz = ccc_buf_elem_size(h->buf);
    hash_in_slot(h, ccc_buf_at(h->buf, q.stopped_at))->hash = empty;
    size_t next = (hash + 1) % cap;
    uint8_t tmp[ccc_buf_elem_size(h->buf)];
    for (;; q.stopped_at = (q.stopped_at + 1) % cap, next = (next + 1) % cap)
    {
        void *next_slot = ccc_buf_at(h->buf, next);
        ccc_hash_elem *next_elem = hash_in_slot(h, next_slot);
        if (!distance(h, next_elem->hash, next))
        {
            break;
        }
        swap(tmp, next_slot, ccc_buf_at(h->buf, q.stopped_at), elem_sz);
    }
    --h->buf->sz;
    return CCC_HASH_OK;
}

/*=========================   Static Helpers    ============================*/

/* Assumes that element to be inserted does not already exist in the table.
   It is undefined to use this if the element has not been membership tested
   yet. */
static void
insert(ccc_hash *const h, void const *const e, size_t const hash)
{
    size_t const elem_sz = ccc_buf_elem_size(h->buf);
    size_t const cap = ccc_buf_capacity(h->buf);
    uint8_t slot_copy[elem_sz];
    (void)memcpy(slot_copy, e, elem_sz);

    /* This function cannot modify e and e may be copied over to new insertion
       from old table. So should this function invariantly assign starting
       hash to this slot copy for insertion? I think yes so far. */
    hash_in_slot(h, slot_copy)->hash = hash;
    size_t i = hash;
    size_t dist = 0;
    uint8_t tmp[elem_sz];
    for (;; i = (i + 1) % cap, ++dist)
    {
        void *const slot = ccc_buf_at(h->buf, i);
        ccc_hash_elem const *home
            = (ccc_hash_elem *)((uint8_t *)slot + h->hash_elem_offset);
        if (home->hash == empty)
        {
            memcpy(slot, e, elem_sz);
            ++h->buf->sz;
            return;
        }
        if (dist > distance(h, home->hash, i))
        {
            swap(tmp, slot_copy, slot, elem_sz);
        }
    }
}

static struct query
find(ccc_hash const *const h, void const *const elem, size_t const hash)
{
    size_t const cap = ccc_buf_capacity(h->buf);
    size_t cur_i = hash;
    for (size_t dist = 0; dist < cap; ++dist, cur_i = (cur_i + 1) % cap)
    {
        void const *const slot = ccc_buf_at(h->buf, cur_i);
        ccc_hash_elem const *const e
            = (ccc_hash_elem *)((uint8_t *)slot + h->hash_elem_offset);
        if (e->hash == empty)
        {
            return (struct query){.found = false, .stopped_at = cur_i};
        }
        if (dist > distance(h, cur_i, e->hash))
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
        ((ccc_hash_elem *)((uint8_t *)i + h->hash_elem_offset))->hash = empty;
    }
    for (void *slot = ccc_buf_begin(h->buf);
         slot != ccc_buf_capacity_end(h->buf);
         slot = ccc_buf_next(h->buf, slot))
    {
        ccc_hash_elem const *const e = hash_in_slot(h, slot);
        if (e->hash != empty)
        {
            size_t const hash = new_hash.hash_fn(slot);
            insert(&new_hash, slot, hash);
        }
    }
    return CCC_HASH_OK;
}

static void
swap(uint8_t tmp[], void *const a, void *const b, size_t ab_size)
{
    assert(a != b);
    (void)memcpy(tmp, a, ab_size);
    (void)memcpy(a, b, ab_size);
    (void)memcpy(b, tmp, ab_size);
}

static size_t
distance(ccc_hash const *const h, size_t const a, size_t const b)
{
    return a < b ? (ccc_buf_capacity(h->buf) - b) + a : b - a;
}

static inline ccc_hash_elem *
hash_in_slot(ccc_hash const *const h, void const *const slot)
{
    return (ccc_hash_elem *)((uint8_t *)slot + h->hash_elem_offset);
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
