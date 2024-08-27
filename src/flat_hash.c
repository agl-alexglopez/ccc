#include "flat_hash.h"
#include "buf.h"
#include "impl_flat_hash.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static double const load_factor = 0.8;

static void erase(struct ccc_impl_fhash *, void *, uint64_t hash);

static bool is_prime(size_t);
static void swap(uint8_t tmp[], void *, void *, size_t);
static void *struct_base(struct ccc_impl_fhash const *,
                         struct ccc_impl_fh_elem const *);

ccc_result
ccc_impl_fh_init(struct ccc_impl_fhash *const h, size_t key_offset,
                 size_t const hash_elem_offset, ccc_hash_fn *const hash_fn,
                 ccc_key_cmp_fn *const eq_fn, void *const aux)
{
    if (!h || !hash_fn || !eq_fn || !ccc_buf_capacity(&h->buf))
    {
        return CCC_MEM_ERR;
    }
    h->key_offset = key_offset;
    h->hash_elem_offset = hash_elem_offset;
    h->hash_fn = hash_fn;
    h->eq_fn = eq_fn;
    h->aux = aux;
    for (void *i = ccc_buf_begin(&h->buf); i != ccc_buf_capacity_end(&h->buf);
         i = ccc_buf_next(&h->buf, i))
    {
        ccc_impl_fh_in_slot(h, i)->hash = EMPTY;
    }
    return CCC_OK;
}

bool
ccc_fh_empty(ccc_fhash const *const h)
{
    return !ccc_buf_size(&h->impl.buf);
}

bool
ccc_fh_contains(ccc_fhash *const h, void const *const key)
{
    return ccc_fh_entry(h, key).impl.entry.status & CCC_ENTRY_OCCUPIED;
}

size_t
ccc_fh_size(ccc_fhash const *const h)
{
    return ccc_buf_size(&h->impl.buf);
}

ccc_fhash_entry
ccc_fh_entry(ccc_fhash *h, void const *const key)
{
    uint64_t const hash = ccc_impl_fh_filter(&h->impl, key);
    return (ccc_fhash_entry){
        {
            .h = &h->impl,
            .hash = hash,
            .entry = ccc_impl_fh_find(&h->impl, key, hash),
        },
    };
}

void *
ccc_fh_insert_entry(ccc_fhash_entry e, ccc_fhash_elem *const elem)
{
    if (ccc_impl_load_factor_exceeded(e.impl.h))
    {
        return NULL;
    }
    if (e.impl.entry.status & (CCC_ENTRY_NULL | CCC_ENTRY_SEARCH_ERROR))
    {
        return (void *)e.impl.entry.entry;
    }
    void *user_struct = struct_base(e.impl.h, &elem->impl);
    if (e.impl.entry.status & CCC_ENTRY_OCCUPIED)
    {
        memcpy((void *)e.impl.entry.entry, user_struct,
               ccc_buf_elem_size(&e.impl.h->buf));
        return (void *)e.impl.entry.entry;
    }
    ccc_impl_fh_insert(e.impl.h, user_struct, e.impl.hash,
                       ccc_buf_index_of(&e.impl.h->buf, e.impl.entry.entry));
    return ccc_impl_fh_maybe_resize(e.impl.h, (void *)e.impl.entry.entry);
}

bool
ccc_fh_remove_entry(ccc_fhash_entry e)
{
    if (e.impl.entry.status != CCC_ENTRY_OCCUPIED)
    {
        return false;
    }
    erase(e.impl.h, (void *)e.impl.entry.entry, e.impl.hash);
    return true;
}

ccc_fhash_entry
ccc_fh_and_modify(ccc_fhash_entry e, ccc_update_fn *const fn)
{
    if (e.impl.entry.status == CCC_ENTRY_OCCUPIED)
    {
        fn((ccc_update){(void *)e.impl.entry.entry, NULL});
    }
    return e;
}

ccc_fhash_entry
ccc_fh_and_modify_with(ccc_fhash_entry e, ccc_update_fn *const fn, void *aux)
{
    if (e.impl.entry.status == CCC_ENTRY_OCCUPIED)
    {
        fn((ccc_update){(void *)e.impl.entry.entry, aux});
    }
    return e;
}

ccc_fhash_entry
ccc_fh_insert(ccc_fhash *h, void *const key, ccc_fhash_elem *const out_handle)
{
    uint64_t const hash = ccc_impl_fh_filter(&h->impl, key);
    if (ccc_impl_load_factor_exceeded(&h->impl))
    {
        return (ccc_fhash_entry){
            {
                .h = &h->impl,
                .hash = hash,
                .entry = {.entry = NULL,
                          .status = CCC_ENTRY_VACANT | CCC_ENTRY_INSERT_ERROR},
            },
        };
    }
    void *user_return = struct_base(&h->impl, &out_handle->impl);
    size_t const user_struct_size = ccc_buf_elem_size(&h->impl.buf);
    ccc_entry const ent = ccc_impl_fh_find(&h->impl, key, hash);
    if (ent.status & CCC_ENTRY_OCCUPIED)
    {
        uint8_t tmp[user_struct_size];
        swap(tmp, (void *)ent.entry, user_return, user_struct_size);
        return (ccc_fhash_entry){
            {
                .h = &h->impl,
                .hash = hash,
                .entry = {.entry = ent.entry, .status = CCC_ENTRY_OCCUPIED},
            },
        };
    }
    ccc_impl_fh_insert(&h->impl, user_return, hash,
                       ccc_buf_index_of(&h->impl.buf, ent.entry));
    return (ccc_fhash_entry){
        {
            .h = &h->impl,
            .hash = hash,
            .entry = {.entry = ccc_impl_fh_maybe_resize(&h->impl, NULL),
                      .status = CCC_ENTRY_VACANT | CCC_ENTRY_NULL},
        },
    };
}

void *
ccc_fh_remove(ccc_fhash *const h, void *const key,
              ccc_fhash_elem *const out_handle)
{
    ccc_entry const ent
        = ccc_impl_fh_find(&h->impl, key, ccc_impl_fh_filter(&h->impl, key));
    if (ent.status == CCC_ENTRY_VACANT)
    {
        return NULL;
    }
    void *ret = struct_base(&h->impl, &out_handle->impl);
    memcpy(ret, ent.entry, ccc_buf_elem_size(&h->impl.buf));
    return ret;
}

void *
ccc_fh_or_insert(ccc_fhash_entry h, ccc_fhash_elem *const elem)
{
    if (ccc_impl_load_factor_exceeded(h.impl.h))
    {
        return NULL;
    }
    if (h.impl.entry.status != CCC_ENTRY_VACANT)
    {
        return (void *)h.impl.entry.entry;
    }
    void *e = struct_base(h.impl.h, &elem->impl);
    elem->impl.hash = h.impl.hash;
    ccc_impl_fh_insert(h.impl.h, e, elem->impl.hash,
                       ccc_buf_index_of(&h.impl.h->buf, h.impl.entry.entry));
    return ccc_impl_fh_maybe_resize(h.impl.h, (void *)h.impl.entry.entry);
}

inline void const *
ccc_fh_get(ccc_fhash_entry const e)
{
    if (!(e.impl.entry.status & CCC_ENTRY_OCCUPIED))
    {
        return NULL;
    }
    return e.impl.entry.entry;
}

inline void *
ccc_fh_get_mut(ccc_fhash_entry const e)
{
    if (!(e.impl.entry.status & CCC_ENTRY_OCCUPIED))
    {
        return NULL;
    }
    return (void *)e.impl.entry.entry;
}

inline bool
ccc_fh_occupied(ccc_fhash_entry const e)
{
    return e.impl.entry.status & CCC_ENTRY_OCCUPIED;
}

inline bool
ccc_fh_insert_error(ccc_fhash_entry e)
{
    return e.impl.entry.status & CCC_ENTRY_INSERT_ERROR;
}

void const *
ccc_fh_begin(ccc_fhash const *const h)
{
    if (ccc_buf_empty(&h->impl.buf))
    {
        return NULL;
    }
    void const *iter = ccc_buf_begin(&h->impl.buf);
    for (; iter != ccc_buf_capacity_end(&h->impl.buf)
           && ccc_impl_fh_in_slot(&h->impl, iter)->hash == EMPTY;
         iter = ccc_buf_next(&h->impl.buf, iter))
    {}
    return iter;
}

void const *
ccc_fh_next(ccc_fhash const *const h, ccc_fhash_elem const *iter)
{
    void const *i = struct_base(&h->impl, &iter->impl);
    for (i = ccc_buf_next(&h->impl.buf, i);
         i != ccc_buf_capacity_end(&h->impl.buf)
         && ccc_impl_fh_in_slot(&h->impl, i)->hash == EMPTY;
         i = ccc_buf_next(&h->impl.buf, i))
    {}
    return i;
}

void const *
ccc_fh_end(ccc_fhash const *const h)
{
    return ccc_buf_capacity_end(&h->impl.buf);
}

ccc_entry
ccc_impl_fh_find(struct ccc_impl_fhash const *const h, void const *const key,
                 uint64_t const hash)
{
    size_t const cap = ccc_buf_capacity(&h->buf);
    size_t cur_i = hash % cap;
    void *slot = ccc_buf_at(&h->buf, cur_i);
    for (size_t dist = 0; dist < cap;
         ++dist, cur_i = (cur_i + 1) % cap, slot = ccc_buf_at(&h->buf, cur_i))
    {
        struct ccc_impl_fh_elem *const e = ccc_impl_fh_in_slot(h, slot);
        if (e->hash == EMPTY)
        {
            return (ccc_entry){.entry = slot, .status = CCC_ENTRY_VACANT};
        }
        if (dist > ccc_impl_fh_distance(cap, cur_i, e->hash))
        {
            return (ccc_entry){.entry = slot, .status = CCC_ENTRY_VACANT};
        }
        if (hash == e->hash
            && h->eq_fn(
                (ccc_key_cmp){.container = slot, .key = key, .aux = h->aux}))
        {
            return (ccc_entry){.entry = slot, .status = CCC_ENTRY_OCCUPIED};
        }
    }
    /* This should be impossible given we are managing load factor? */
    return (ccc_entry){.entry = slot, .status = CCC_ENTRY_SEARCH_ERROR};
}

inline struct ccc_impl_fh_elem *
ccc_impl_fh_in_slot(struct ccc_impl_fhash const *const h,
                    void const *const slot)
{
    return (struct ccc_impl_fh_elem *)((uint8_t *)slot + h->hash_elem_offset);
}

void *
ccc_impl_key_in_slot(struct ccc_impl_fhash const *h, void const *slot)
{
    return (struct ccc_impl_fh_elem *)((uint8_t *)slot + h->key_offset);
}

inline size_t
ccc_impl_fh_distance(size_t const capacity, size_t const index, uint64_t hash)
{
    hash %= capacity;
    return index < hash ? (capacity - hash) + index : hash - index;
}

/* TODO: There needs to be a return to signal a realloc failure here maybe
   setting the output parameter instead. Either way, this needs to be
   more communicative of errors. */
void *
ccc_impl_fh_maybe_resize(struct ccc_impl_fhash *h,
                         void *const track_after_resize)
{
    if ((double)ccc_buf_size(&h->buf) / (double)ccc_buf_capacity(&h->buf)
        <= load_factor)
    {
        return track_after_resize;
    }
    if (!h->buf.impl.realloc_fn)
    {
        return track_after_resize;
    }
    struct ccc_impl_fhash new_hash = *h;
    new_hash.buf.impl.sz = 0;
    new_hash.buf.impl.capacity = ccc_fh_next_prime(ccc_buf_size(&h->buf) * 2);
    new_hash.buf.impl.mem = new_hash.buf.impl.realloc_fn(
        NULL, ccc_buf_elem_size(&h->buf) * new_hash.buf.impl.capacity);
    if (!new_hash.buf.impl.mem)
    {
        return NULL;
    }
    for (void *i = ccc_buf_begin(&new_hash.buf);
         i != ccc_buf_capacity_end(&new_hash.buf);
         i = ccc_buf_next(&new_hash.buf, i))
    {
        ccc_impl_fh_in_slot(&new_hash, i)->hash = EMPTY;
    }
    for (void *slot = ccc_buf_begin(&h->buf);
         slot != ccc_buf_capacity_end(&h->buf);
         slot = ccc_buf_next(&h->buf, slot))
    {
        struct ccc_impl_fh_elem const *const e = ccc_impl_fh_in_slot(h, slot);
        if (e->hash != EMPTY)
        {
            void *old_key = ccc_impl_key_in_slot(h, slot);
            ccc_entry new_ent = ccc_impl_fh_find(&new_hash, old_key, e->hash);
            ccc_impl_fh_insert(&new_hash, slot, e->hash,
                               ccc_buf_index_of(&new_hash.buf, new_ent.entry));
        }
    }
    /* All the insertions in the new table could have moved our old element
       around quite a bit in the new table. Do another search. */
    void *ret = (void *)ccc_impl_fh_find(
                    &new_hash, ccc_impl_key_in_slot(h, track_after_resize),
                    ccc_impl_fh_in_slot(h, track_after_resize)->hash)
                    .entry;
    (void)ccc_buf_realloc(&h->buf, 0, h->buf.impl.realloc_fn);
    *h = new_hash;
    return ret;
}

void
ccc_fh_print(ccc_fhash const *h, ccc_print_fn *fn)
{
    for (void const *i = ccc_buf_begin(&h->impl.buf);
         i != ccc_buf_capacity_end(&h->impl.buf);
         i = ccc_buf_next(&h->impl.buf, i))
    {
        if (ccc_impl_fh_in_slot(&h->impl, i)->hash != EMPTY)
        {
            fn(i);
        }
    }
}

inline bool
ccc_impl_load_factor_exceeded(struct ccc_impl_fhash const *const h)
{
    return ((double)ccc_buf_size(&h->buf) / (double)ccc_buf_capacity(&h->buf))
           > load_factor;
}

inline size_t
ccc_fh_next_prime(size_t n)
{
    if (n <= 1)
    {
        return 2;
    }
    while (!is_prime(++n))
    {}
    return n;
}

void *
ccc_fh_buf_base(ccc_fhash const *const h)
{
    return ccc_buf_base(&h->impl.buf);
}

void
ccc_fh_clear(ccc_fhash *const h, ccc_destructor_fn *const fn)
{
    if (!fn)
    {
        h->impl.buf.impl.sz = 0;
        return;
    }
    for (void *slot = ccc_buf_begin(&h->impl.buf);
         slot != ccc_buf_capacity_end(&h->impl.buf);
         slot = ccc_buf_next(&h->impl.buf, slot))
    {
        if (ccc_impl_fh_in_slot(&h->impl, slot)->hash != EMPTY)
        {
            fn(slot);
        }
    }
}

ccc_result
ccc_fh_clear_and_free(ccc_fhash *const h, ccc_destructor_fn *const fn)
{
    if (!fn)
    {
        h->impl.buf.impl.sz = 0;
        return ccc_buf_free(&h->impl.buf, h->impl.buf.impl.realloc_fn);
    }
    for (void *slot = ccc_buf_begin(&h->impl.buf);
         slot != ccc_buf_capacity_end(&h->impl.buf);
         slot = ccc_buf_next(&h->impl.buf, slot))
    {
        if (ccc_impl_fh_in_slot(&h->impl, slot)->hash != EMPTY)
        {
            fn(slot);
        }
    }
    return ccc_buf_free(&h->impl.buf, h->impl.buf.impl.realloc_fn);
}

size_t
ccc_fh_capacity(ccc_fhash const *const h)
{
    return ccc_buf_capacity(&h->impl.buf);
}

/*=========================   Static Helpers    ============================*/

/* Assumes that element to be inserted does not already exist in the table.
   Assumes that the table has room for another insertion. It is undefined to
   use this if the element has not been membership tested yet or the table
   is full. */
void
ccc_impl_fh_insert(struct ccc_impl_fhash *const h, void const *const e,
                   uint64_t const hash, size_t cur_i)
{
    size_t const elem_sz = ccc_buf_elem_size(&h->buf);
    size_t const cap = ccc_buf_capacity(&h->buf);
    uint8_t floater[elem_sz];
    (void)memcpy(floater, e, elem_sz);

    /* This function cannot modify e and e may be copied over to new insertion
       from old table. So should this function invariantly assign starting
       hash to this slot copy for insertion? I think yes so far. */
    ccc_impl_fh_in_slot(h, floater)->hash = hash;
    size_t dist = ccc_impl_fh_distance(cap, cur_i, hash);
    for (;; cur_i = (cur_i + 1) % cap, ++dist)
    {
        void *const slot = ccc_buf_at(&h->buf, cur_i);
        struct ccc_impl_fh_elem const *slot_hash = ccc_impl_fh_in_slot(h, slot);
        if (slot_hash->hash == EMPTY)
        {
            memcpy(slot, floater, elem_sz);
            ++h->buf.impl.sz;
            return;
        }
        size_t const slot_dist
            = ccc_impl_fh_distance(cap, cur_i, slot_hash->hash);
        if (dist > slot_dist)
        {
            uint8_t tmp[elem_sz];
            swap(tmp, floater, slot, elem_sz);
            dist = slot_dist;
        }
    }
}

static void
erase(struct ccc_impl_fhash *const h, void *const e, uint64_t const hash)
{
    size_t const cap = ccc_buf_capacity(&h->buf);
    size_t stopped_at = ccc_buf_index_of(&h->buf, e);
    size_t const elem_sz = ccc_buf_elem_size(&h->buf);
    *ccc_impl_hash_at(h, stopped_at) = EMPTY;
    size_t next = (hash % cap) + 1;
    uint8_t tmp[ccc_buf_elem_size(&h->buf)];
    for (;; stopped_at = (stopped_at + 1) % cap, next = (next + 1) % cap)
    {
        void *next_slot = ccc_buf_at(&h->buf, next);
        struct ccc_impl_fh_elem *next_elem = ccc_impl_fh_in_slot(h, next_slot);
        if (!ccc_impl_fh_distance(cap, next, next_elem->hash))
        {
            break;
        }
        swap(tmp, next_slot, ccc_buf_at(&h->buf, stopped_at), elem_sz);
    }
    --h->buf.impl.sz;
}

static void *
struct_base(struct ccc_impl_fhash const *const h,
            struct ccc_impl_fh_elem const *const e)
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

inline uint64_t *
ccc_impl_hash_at(struct ccc_impl_fhash const *const h, size_t const i)
{
    return &((struct ccc_impl_fh_elem *)((uint8_t *)ccc_buf_at(&h->buf, i)
                                         + h->hash_elem_offset))
                ->hash;
}

inline uint64_t
ccc_impl_fh_filter(struct ccc_impl_fhash const *const h, void const *const key)
{
    uint64_t const hash = h->hash_fn(key);
    return hash == EMPTY ? hash - 1 : hash;
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
