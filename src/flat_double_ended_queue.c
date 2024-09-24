#include "flat_double_ended_queue.h"
#include "buffer.h"
#include "impl_flat_double_ended_queue.h"
#include "types.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static size_t const start_capacity = 8;

#define MIN(a, b)                                                              \
    ({                                                                         \
        size_t a_ = (a);                                                       \
        size_t b_ = (b);                                                       \
        a_ < b_ ? a_ : b_;                                                     \
    })

static ccc_result maybe_resize(struct ccc_fdeq_ *, size_t);
static size_t bytes(struct ccc_fdeq_ const *, size_t);
static size_t index_of(struct ccc_fdeq_ const *, void const *);
static void *at(struct ccc_fdeq_ const *, size_t i);
size_t increment(struct ccc_fdeq_ const *, size_t i);
size_t decrement(struct ccc_fdeq_ const *, size_t i);
static size_t distance(struct ccc_fdeq_ const *, size_t, size_t);
static size_t rdistance(struct ccc_fdeq_ const *, size_t, size_t);
static ccc_result push_front_range(struct ccc_fdeq_ *fq, size_t n,
                                   char const *elems);
static ccc_result push_back_range(struct ccc_fdeq_ *fq, size_t n,
                                  char const *elems);
static size_t back_free_slot(struct ccc_fdeq_ const *);
static size_t front_free_slot(size_t front, size_t cap);
static size_t last_elem_index(struct ccc_fdeq_ const *);

static inline ccc_result
push_back_range(struct ccc_fdeq_ *const fq, size_t const n, char const *elems)
{
    size_t const elem_sz = ccc_buf_elem_size(&fq->buf_);
    bool const full = maybe_resize(fq, n) != CCC_OK;
    size_t const cap = ccc_buf_capacity(&fq->buf_);
    if (fq->buf_.alloc_ && full)
    {
        return CCC_MEM_ERR;
    }
    /* If a range is too large we can make various simplifications to preserve
       the final capacity elements. */
    if (n >= cap)
    {
        elems += ((n - cap) * elem_sz);
        fq->front_ = 0;
        (void)memcpy(ccc_buf_at(&fq->buf_, 0), elems, elem_sz * cap);
        ccc_buf_size_set(&fq->buf_, cap);
        return CCC_OK;
    }
    size_t const back_slot = back_free_slot(fq);
    size_t const chunk = MIN(n, cap - back_slot);
    size_t const remainder_back_slot = (back_slot + chunk) % cap;
    size_t const remainder = (n - chunk);
    char const *const second_chunk = elems + (chunk * elem_sz);
    (void)memcpy(ccc_buf_at(&fq->buf_, back_slot), elems, chunk * elem_sz);
    if (remainder)
    {
        (void)memcpy(ccc_buf_at(&fq->buf_, remainder_back_slot), second_chunk,
                     remainder * elem_sz);
    }
    if (full && fq->front_ == back_slot)
    {
        fq->front_ = (fq->front_ + chunk) % cap;
    }
    if (full && remainder && fq->front_ == remainder_back_slot)
    {
        fq->front_ = (fq->front_ + remainder) % cap;
    }
    ccc_buf_size_set(&fq->buf_, MIN(cap, ccc_buf_size(&fq->buf_) + n));
    return CCC_OK;
}

static inline ccc_result
push_front_range(struct ccc_fdeq_ *const fq, size_t const n, char const *elems)
{
    size_t const elem_sz = ccc_buf_elem_size(&fq->buf_);
    bool const full = maybe_resize(fq, n) != CCC_OK;
    size_t const cap = ccc_buf_capacity(&fq->buf_);
    if (fq->buf_.alloc_ && full)
    {
        return CCC_MEM_ERR;
    }
    /* If a range is too large we can make various simplifications to preserve
       the final capacity elements. */
    if (n >= cap)
    {
        elems += ((n - cap) * elem_sz);
        fq->front_ = 0;
        (void)memcpy(ccc_buf_at(&fq->buf_, 0), elems, elem_sz * cap);
        ccc_buf_size_set(&fq->buf_, cap);
        return CCC_OK;
    }
    size_t const space_ahead = front_free_slot(fq->front_, cap) + 1;
    size_t const front_i = n > space_ahead ? 0 : space_ahead - n;
    size_t const chunk = MIN(n, space_ahead);
    size_t const remainder = (n - chunk);
    size_t const remainder_ahead = front_free_slot(front_i, cap) + 1;
    size_t const remainder_chunk = MIN(remainder, remainder_ahead);
    size_t const remainder_front_i
        = remainder > remainder_ahead ? 0 : remainder_ahead - remainder;
    char const *const first_chunk = elems + ((n - chunk) * elem_sz);
    (void)memcpy(ccc_buf_at(&fq->buf_, front_i), first_chunk, chunk * elem_sz);
    if (remainder)
    {
        (void)memcpy(ccc_buf_at(&fq->buf_, remainder_front_i), elems,
                     remainder_chunk * elem_sz);
    }
    ccc_buf_size_set(&fq->buf_, MIN(cap, ccc_buf_size(&fq->buf_) + n));
    fq->front_ = remainder ? remainder_front_i : front_i;
    return CCC_OK;
}

void *
ccc_fdeq_push_back(ccc_flat_double_ended_queue *const fq,
                   void const *const elem)
{
    if (!fq || !elem)
    {
        return NULL;
    }
    void *slot = ccc_impl_fdeq_alloc_back(fq);
    if (slot)
    {
        (void)memcpy(slot, elem, ccc_buf_elem_size(&fq->buf_));
    }
    return slot;
}

void *
ccc_fdeq_push_front(ccc_flat_double_ended_queue *const fq,
                    void const *const elem)
{
    if (!fq || !elem)
    {
        return NULL;
    }
    void *slot = ccc_impl_fdeq_alloc_front(fq);
    if (slot)
    {
        (void)memcpy(slot, elem, ccc_buf_elem_size(&fq->buf_));
    }
    return slot;
}

ccc_result
ccc_fdeq_push_front_range(ccc_flat_double_ended_queue *const fq, size_t const n,
                          void const *elems)
{
    if (!fq || !elems)
    {
        return CCC_INPUT_ERR;
    }
    return push_front_range(fq, n, elems);
}

ccc_result
ccc_fdeq_push_back_range(ccc_flat_double_ended_queue *const fq, size_t const n,
                         void const *elems)
{
    if (!fq || !elems)
    {
        return CCC_INPUT_ERR;
    }
    return push_back_range(fq, n, elems);
}

void *
ccc_fdeq_insert_range(ccc_flat_double_ended_queue *fq, void const *pos,
                      size_t n, void const *elems)
{
    if (!fq)
    {
        return NULL;
    }
    if (!n)
    {
        return (void *)pos;
    }
    if (pos == ccc_fdeq_begin(fq))
    {
        return push_front_range(fq, n, elems) != CCC_OK ? NULL : at(fq, n - 1);
    }
    if (pos == ccc_fdeq_end(fq))
    {
        return push_back_range(fq, n, elems) != CCC_OK
                   ? NULL
                   : at(fq, ccc_buf_size(&fq->buf_) - n);
    }
    /* TODO: Implementing insert range in a flat queue is non trivial and will
       vary depending on resizing or not. */
    return NULL;
}

void
ccc_fdeq_pop_front(ccc_flat_double_ended_queue *const fq)
{
    if (ccc_buf_empty(&fq->buf_))
    {
        return;
    }
    fq->front_ = increment(fq, fq->front_);
    ccc_buf_size_minus(&fq->buf_, 1);
}

void
ccc_fdeq_pop_back(ccc_flat_double_ended_queue *const fq)
{
    if (ccc_buf_empty(&fq->buf_))
    {
        return;
    }
    ccc_buf_size_minus(&fq->buf_, 1);
}

void *
ccc_fdeq_front(ccc_flat_double_ended_queue const *const fq)
{
    if (ccc_buf_empty(&fq->buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->buf_, fq->front_);
}

void *
ccc_fdeq_back(ccc_flat_double_ended_queue const *const fq)
{
    if (ccc_buf_empty(&fq->buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->buf_, last_elem_index(fq));
}

bool
ccc_fdeq_empty(ccc_flat_double_ended_queue const *const fq)
{
    return !fq || !ccc_buf_size(&fq->buf_);
}

size_t
ccc_fdeq_size(ccc_flat_double_ended_queue const *const fq)
{
    return !fq ? 0 : ccc_buf_size(&fq->buf_);
}

void *
ccc_fdeq_at(ccc_flat_double_ended_queue const *const fq, size_t const i)
{
    if (i >= ccc_fdeq_size(fq))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->buf_,
                      (fq->front_ + i) % ccc_buf_capacity(&fq->buf_));
}

void *
ccc_impl_fdeq_alloc_back(struct ccc_fdeq_ *fq)
{
    void *new_slot = NULL;
    bool const full = maybe_resize(fq, 0) != CCC_OK;
    /* Should have been able to resize. Bad error. */
    if (fq->buf_.alloc_ && full)
    {
        return NULL;
    }
    new_slot = ccc_buf_at(&fq->buf_, back_free_slot(fq));
    /* If no reallocation policy is given we are a ring buffer. */
    if (full)
    {
        fq->front_ = increment(fq, fq->front_);
    }
    else
    {
        ccc_buf_size_plus(&fq->buf_, 1);
    }
    return new_slot;
}

void *
ccc_impl_fdeq_alloc_front(struct ccc_fdeq_ *fq)
{
    void *new_slot = NULL;
    bool const full = maybe_resize(fq, 0) != CCC_OK;
    /* Should have been able to resize. Bad error. */
    if (fq->buf_.alloc_ && full)
    {
        return NULL;
    }
    fq->front_ = front_free_slot(fq->front_, ccc_buf_capacity(&fq->buf_));
    new_slot = ccc_buf_at(&fq->buf_, fq->front_);
    if (!full)
    {
        ccc_buf_size_plus(&fq->buf_, 1);
    }
    return new_slot;
}

void *
ccc_fdeq_begin(ccc_flat_double_ended_queue const *fq)
{
    return ccc_buf_at(&fq->buf_, fq->front_);
}

void *
ccc_fdeq_rbegin(ccc_flat_double_ended_queue const *fq)
{
    if (!ccc_buf_size(&fq->buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->buf_, last_elem_index(fq));
}

void *
ccc_fdeq_next(ccc_flat_double_ended_queue const *fq, void const *iter_ptr)
{
    size_t const next_i = increment(fq, index_of(fq, iter_ptr));
    if (next_i == fq->front_
        || distance(fq, next_i, fq->front_) >= ccc_buf_size(&fq->buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->buf_, next_i);
}

void *
ccc_fdeq_rnext(ccc_flat_double_ended_queue const *fq, void const *iter_ptr)
{
    size_t const cur_i = index_of(fq, iter_ptr);
    size_t const next_i = decrement(fq, cur_i);
    size_t const rbegin = last_elem_index(fq);
    if (next_i == rbegin
        || rdistance(fq, next_i, rbegin) >= ccc_buf_size(&fq->buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->buf_, next_i);
}

void *
ccc_fdeq_end([[maybe_unused]] ccc_flat_double_ended_queue const *const fq)
{
    return NULL;
}

void *
ccc_fdeq_rend([[maybe_unused]] ccc_flat_double_ended_queue const *const fq)
{
    return NULL;
}

bool
ccc_fdeq_validate(ccc_flat_double_ended_queue const *const fq)
{
    void *iter = ccc_fdeq_begin(fq);
    if (ccc_buf_index_of(&fq->buf_, iter) != fq->front_)
    {
        return false;
    }
    size_t size = 0;
    for (; iter != ccc_fdeq_end(fq); iter = ccc_fdeq_next(fq, iter), ++size)
    {
        if (size >= ccc_fdeq_size(fq))
        {
            return false;
        }
    }
    if (size != ccc_fdeq_size(fq))
    {
        return false;
    }
    size = 0;
    iter = ccc_fdeq_rbegin(fq);
    if (ccc_buf_index_of(&fq->buf_, iter) != last_elem_index(fq))
    {
        return false;
    }
    for (; iter != ccc_fdeq_rend(fq); iter = ccc_fdeq_rnext(fq, iter), ++size)
    {
        if (size >= ccc_fdeq_size(fq))
        {
            return false;
        }
    }
    return size == ccc_fdeq_size(fq);
}

void
ccc_fdeq_clear(ccc_flat_double_ended_queue *const fq,
               ccc_destructor_fn *destructor)
{
    if (!fq)
    {
        return;
    }
    if (!destructor)
    {
        fq->front_ = 0;
        ccc_buf_size_set(&fq->buf_, 0);
        return;
    }
    size_t const back = back_free_slot(fq);
    for (size_t i = fq->front_; i != back; i = increment(fq, i))
    {
        destructor(ccc_buf_at(&fq->buf_, i));
    }
}

void
ccc_fdeq_clear_and_free(ccc_flat_double_ended_queue *const fq,
                        ccc_destructor_fn *destructor)
{
    if (!fq)
    {
        return;
    }
    if (!destructor)
    {
        fq->buf_.sz_ = fq->front_ = 0;
        ccc_buf_realloc(&fq->buf_, 0, fq->buf_.alloc_);
        return;
    }
    size_t const back = back_free_slot(fq);
    for (size_t i = fq->front_; i != back; i = increment(fq, i))
    {
        destructor(ccc_buf_at(&fq->buf_, i));
    }
    (void)ccc_buf_realloc(&fq->buf_, 0, fq->buf_.alloc_);
}

static inline ccc_result
maybe_resize(struct ccc_fdeq_ *const q, size_t const additional_elems_to_add)
{
    if (ccc_buf_size(&q->buf_) + additional_elems_to_add
        < ccc_buf_capacity(&q->buf_))
    {
        return CCC_OK;
    }
    if (!q->buf_.alloc_)
    {
        return CCC_NO_REALLOC;
    }
    size_t const new_cap
        = ccc_buf_capacity(&q->buf_)
              ? ((ccc_buf_capacity(&q->buf_) + additional_elems_to_add) * 2)
              : start_capacity;
    void *new_mem = q->buf_.alloc_(NULL, ccc_buf_elem_size(&q->buf_) * new_cap);
    if (!new_mem)
    {
        return CCC_MEM_ERR;
    }
    size_t const first_chunk
        = MIN(ccc_buf_size(&q->buf_), ccc_buf_capacity(&q->buf_) - q->front_);
    (void)memcpy(new_mem, ccc_buf_at(&q->buf_, q->front_),
                 bytes(q, first_chunk));
    if (first_chunk < ccc_buf_size(&q->buf_))
    {
        (void)memcpy((char *)new_mem + bytes(q, first_chunk),
                     ccc_buf_base(&q->buf_),
                     bytes(q, ccc_buf_size(&q->buf_) - first_chunk));
    }
    (void)ccc_buf_realloc(&q->buf_, 0, q->buf_.alloc_);
    q->buf_.mem_ = new_mem;
    q->front_ = 0;
    q->buf_.capacity_ = new_cap;
    return CCC_OK;
}

static inline size_t
distance(struct ccc_fdeq_ const *const fq, size_t const iter,
         size_t const origin)
{
    return iter > origin ? iter - origin
                         : (ccc_buf_capacity(&fq->buf_) - origin) + iter;
}

static inline size_t
rdistance(struct ccc_fdeq_ const *const fq, size_t const iter,
          size_t const origin)
{
    return iter > origin ? (ccc_buf_capacity(&fq->buf_) - iter) + origin
                         : origin - iter;
}

static inline size_t
index_of(struct ccc_fdeq_ const *const fq, void const *const pos)
{
    assert(pos >= ccc_buf_base(&fq->buf_));
    assert(pos < ccc_buf_capacity_end(&fq->buf_));
    return ((char *)pos - (char *)ccc_buf_base(&fq->buf_))
           / ccc_buf_elem_size(&fq->buf_);
}

static void *
at(struct ccc_fdeq_ const *const fq, size_t const i)
{
    if (i >= ccc_buf_capacity(&fq->buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->buf_,
                      (fq->front_ + i) % ccc_buf_capacity(&fq->buf_));
}

size_t
increment(struct ccc_fdeq_ const *const fq, size_t i)
{
    return i == (ccc_buf_capacity(&fq->buf_) - 1) ? 0 : ++i;
}

size_t
decrement(struct ccc_fdeq_ const *const fq, size_t i)
{
    return i ? --i : ccc_buf_capacity(&fq->buf_) - 1;
}

static inline size_t
back_free_slot(struct ccc_fdeq_ const *const fq)
{
    return (fq->front_ + ccc_buf_size(&fq->buf_)) % ccc_buf_capacity(&fq->buf_);
}

static inline size_t
front_free_slot(size_t const front, size_t const cap)
{
    return front ? front - 1 : cap - 1;
}

/* Assumes the queue is not empty. */
static inline size_t
last_elem_index(struct ccc_fdeq_ const *const fq)
{
    return (fq->front_ + (ccc_buf_size(&fq->buf_) - 1))
           % ccc_buf_capacity(&fq->buf_);
}

static inline size_t
bytes(struct ccc_fdeq_ const *q, size_t n)
{
    return ccc_buf_elem_size(&q->buf_) * n;
}
