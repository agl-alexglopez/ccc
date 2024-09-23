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
        __auto_type a_ = (a);                                                  \
        __auto_type b_ = (b);                                                  \
        a_ < b_ ? a_ : b_;                                                     \
    })

static ccc_result maybe_resize(struct ccc_fdeq_ *);
static size_t bytes(struct ccc_fdeq_ const *, size_t);
static size_t index_of(struct ccc_fdeq_ const *, void const *);
static void *at(struct ccc_fdeq_ const *, size_t i);
static size_t distance(struct ccc_fdeq_ const *, size_t, size_t);
static size_t rdistance(struct ccc_fdeq_ const *, size_t, size_t);
static void *push(struct ccc_fdeq_ *, size_t elem_sz, char const *elem,
                  enum ccc_impl_fdeq_alloc_slot);
static ccc_result push_range(struct ccc_fdeq_ *fq, size_t n, char const *elems,
                             enum ccc_impl_fdeq_alloc_slot dir);
static size_t back_free_slot(struct ccc_fdeq_ const *);
static size_t front_free_slot(struct ccc_fdeq_ const *);
static size_t last_elem_index(struct ccc_fdeq_ const *);

static inline void *
push(struct ccc_fdeq_ *const fq, size_t const elem_sz, char const *const elem,
     enum ccc_impl_fdeq_alloc_slot const dir)
{
    void *slot = ccc_impl_fdeq_alloc(fq, dir);
    if (slot)
    {
        (void)memcpy(slot, elem, elem_sz);
    }
    return slot;
}

static ccc_result
push_range(struct ccc_fdeq_ *const fq, size_t const n, char const *elems,
           enum ccc_impl_fdeq_alloc_slot const dir)
{
    size_t const elem_sz = ccc_buf_elem_size(&fq->buf_);
    /* TODO: Ew. This should precalculate space needed or chunking for wrapping
       and perform at most two memcpy calls. No loops. */
    switch (dir)
    {
    case CCC_IMPL_FDEQ_BACK:
    {
        for (size_t i = 0; i < n; ++i, elems += elem_sz)
        {
            if (!push(fq, elem_sz, elems, dir))
            {
                return CCC_MEM_ERR;
            }
        }
    }
    break;
    case CCC_IMPL_FDEQ_FRONT:
    {
        elems = elems + (elem_sz * (n - 1));
        for (size_t i = n; i--; elems -= elem_sz)
        {
            if (!push(fq, elem_sz, elems, dir))
            {
                return CCC_MEM_ERR;
            }
        }
    }
    break;
    }
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
    return push(fq, ccc_buf_elem_size(&fq->buf_), elem, CCC_IMPL_FDEQ_BACK);
}

void *
ccc_fdeq_push_front(ccc_flat_double_ended_queue *const fq,
                    void const *const elem)
{
    if (!fq || !elem)
    {
        return NULL;
    }
    return push(fq, ccc_buf_elem_size(&fq->buf_), elem, CCC_IMPL_FDEQ_FRONT);
}

ccc_result
ccc_fdeq_push_front_range(ccc_flat_double_ended_queue *const fq, size_t const n,
                          void const *elems)
{
    if (!fq || !elems)
    {
        return CCC_INPUT_ERR;
    }
    return push_range(fq, n, elems, CCC_IMPL_FDEQ_FRONT);
}

ccc_result
ccc_fdeq_push_back_range(ccc_flat_double_ended_queue *const fq, size_t const n,
                         void const *elems)
{
    if (!fq || !elems)
    {
        return CCC_INPUT_ERR;
    }
    return push_range(fq, n, elems, CCC_IMPL_FDEQ_BACK);
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
        return push_range(fq, n, elems, CCC_IMPL_FDEQ_FRONT) != CCC_OK
                   ? NULL
                   : at(fq, n - 1);
    }
    if (pos == ccc_fdeq_end(fq))
    {
        return push_range(fq, n, elems, CCC_IMPL_FDEQ_BACK) != CCC_OK
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
    fq->front_ = (fq->front_ + 1) % ccc_buf_capacity(&fq->buf_);
    ccc_buf_size_minus(&fq->buf_);
}

void
ccc_fdeq_pop_back(ccc_flat_double_ended_queue *const fq)
{
    if (ccc_buf_empty(&fq->buf_))
    {
        return;
    }
    ccc_buf_size_minus(&fq->buf_);
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
ccc_impl_fdeq_alloc(struct ccc_fdeq_ *fq,
                    enum ccc_impl_fdeq_alloc_slot const dir)
{
    void *new_slot = NULL;
    bool const full = maybe_resize(fq) != CCC_OK;
    /* Should have been able to resize. Bad error. */
    if (fq->buf_.alloc_ && full)
    {
        return NULL;
    }
    switch (dir)
    {
    case CCC_IMPL_FDEQ_BACK:
    {
        new_slot = ccc_buf_at(&fq->buf_, back_free_slot(fq));
        /* If no reallocation policy is given we are a ring buffer. */
        if (full)
        {
            fq->front_ = (fq->front_ + 1) % ccc_buf_capacity(&fq->buf_);
        }
    }
    break;
    case CCC_IMPL_FDEQ_FRONT:
    {
        fq->front_ = front_free_slot(fq);
        new_slot = ccc_buf_at(&fq->buf_, fq->front_);
    }
    }
    if (!full)
    {
        ccc_buf_size_plus(&fq->buf_);
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
    size_t const next_i
        = (index_of(fq, iter_ptr) + 1) % ccc_buf_capacity(&fq->buf_);
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
    size_t const next_i = cur_i ? cur_i - 1 : ccc_buf_capacity(&fq->buf_) - 1;
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
    size_t const cap = ccc_buf_capacity(&fq->buf_);
    size_t const back = back_free_slot(fq);
    for (size_t i = fq->front_; i != back; i = (i + 1) % cap)
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
    size_t const cap = ccc_buf_capacity(&fq->buf_);
    size_t const back = back_free_slot(fq);
    for (size_t i = fq->front_; i != back; i = (i + 1) % cap)
    {
        destructor(ccc_buf_at(&fq->buf_, i));
    }
    (void)ccc_buf_realloc(&fq->buf_, 0, fq->buf_.alloc_);
}

static inline ccc_result
maybe_resize(struct ccc_fdeq_ *const q)
{
    if (ccc_buf_size(&q->buf_) < ccc_buf_capacity(&q->buf_))
    {
        return CCC_OK;
    }
    if (!q->buf_.alloc_)
    {
        return CCC_NO_REALLOC;
    }
    size_t const new_cap = ccc_buf_capacity(&q->buf_)
                               ? (ccc_buf_capacity(&q->buf_) * 2)
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

static inline size_t
back_free_slot(struct ccc_fdeq_ const *const fq)
{
    return (fq->front_ + ccc_buf_size(&fq->buf_)) % ccc_buf_capacity(&fq->buf_);
}

static inline size_t
front_free_slot(struct ccc_fdeq_ const *const fq)
{
    return fq->front_ ? fq->front_ - 1 : ccc_buf_capacity(&fq->buf_) - 1;
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
