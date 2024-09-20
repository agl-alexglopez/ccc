#include "flat_double_ended_queue.h"
#include "buffer.h"
#include "impl_flat_double_ended_queue.h"
#include "types.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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
static size_t distance(struct ccc_fdeq_ const *, size_t, size_t);
static size_t rdistance(struct ccc_fdeq_ const *, size_t, size_t);
static void *push(struct ccc_fdeq_ *, void const *,
                  enum ccc_impl_fdeq_alloc_slot);

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
        fq->impl_.front_ = 0;
        ccc_buf_size_set(&fq->impl_.buf_, 0);
        return;
    }
    size_t const cap = ccc_buf_capacity(&fq->impl_.buf_);
    size_t const back = (fq->impl_.front_ + ccc_buf_size(&fq->impl_.buf_))
                        % ccc_buf_capacity(&fq->impl_.buf_);
    for (size_t i = fq->impl_.front_; i != back; i = (i + 1) % cap)
    {
        destructor(ccc_buf_at(&fq->impl_.buf_, i));
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
        fq->impl_.buf_.sz_ = fq->impl_.front_ = 0;
        ccc_buf_realloc(&fq->impl_.buf_, 0, fq->impl_.buf_.alloc_);
        return;
    }
    size_t const cap = ccc_buf_capacity(&fq->impl_.buf_);
    size_t const back = (fq->impl_.front_ + ccc_buf_size(&fq->impl_.buf_))
                        % ccc_buf_capacity(&fq->impl_.buf_);
    for (size_t i = fq->impl_.front_; i != back; i = (i + 1) % cap)
    {
        destructor(ccc_buf_at(&fq->impl_.buf_, i));
    }
    ccc_buf_realloc(&fq->impl_.buf_, 0, fq->impl_.buf_.alloc_);
}

static void *
push(struct ccc_fdeq_ *const fq, void const *const elem,
     enum ccc_impl_fdeq_alloc_slot const dir)
{
    if (!elem)
    {
        return NULL;
    }
    void *slot = ccc_impl_fdeq_alloc(fq, dir);
    if (slot)
    {
        memcpy(slot, elem, ccc_buf_elem_size(&fq->buf_));
    }
    return slot;
}

void *
ccc_fdeq_push_back(ccc_flat_double_ended_queue *const fq,
                   void const *const elem)
{
    return push(&fq->impl_, elem, CCC_IMPL_FDEQ_BACK);
}

void *
ccc_fdeq_push_front(ccc_flat_double_ended_queue *const fq,
                    void const *const elem)
{
    return push(&fq->impl_, elem, CCC_IMPL_FDEQ_FRONT);
}

void
ccc_fdeq_pop_front(ccc_flat_double_ended_queue *const fq)
{
    if (ccc_buf_empty(&fq->impl_.buf_))
    {
        return;
    }
    fq->impl_.front_
        = (fq->impl_.front_ + 1) % ccc_buf_capacity(&fq->impl_.buf_);
    ccc_buf_size_minus(&fq->impl_.buf_);
}

void
ccc_fdeq_pop_back(ccc_flat_double_ended_queue *const fq)
{
    if (ccc_buf_empty(&fq->impl_.buf_))
    {
        return;
    }
    ccc_buf_size_minus(&fq->impl_.buf_);
}

void *
ccc_fdeq_front(ccc_flat_double_ended_queue const *const fq)
{
    if (ccc_buf_empty(&fq->impl_.buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->impl_.buf_, fq->impl_.front_);
}

void *
ccc_fdeq_back(ccc_flat_double_ended_queue const *const fq)
{
    if (ccc_buf_empty(&fq->impl_.buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->impl_.buf_,
                      (fq->impl_.front_ + (ccc_buf_size(&fq->impl_.buf_) - 1))
                          % ccc_buf_capacity(&fq->impl_.buf_));
}

bool
ccc_fdeq_empty(ccc_flat_double_ended_queue const *const fq)
{
    return !fq || !ccc_buf_size(&fq->impl_.buf_);
}

size_t
ccc_fdeq_size(ccc_flat_double_ended_queue const *const fq)
{
    return !fq ? 0 : ccc_buf_size(&fq->impl_.buf_);
}

void *
ccc_impl_fdeq_alloc(struct ccc_fdeq_ *fq,
                    enum ccc_impl_fdeq_alloc_slot const dir)
{
    void *new_slot = NULL;
    if (!fq || maybe_resize(fq) != CCC_OK)
    {
        return new_slot;
    }
    switch (dir)
    {
    case CCC_IMPL_FDEQ_BACK:
    {
        new_slot = ccc_buf_at(&fq->buf_, (fq->front_ + ccc_buf_size(&fq->buf_))
                                             % ccc_buf_capacity(&fq->buf_));
    }
    break;
    case CCC_IMPL_FDEQ_FRONT:
    {
        fq->front_
            = fq->front_ ? fq->front_ - 1 : ccc_buf_capacity(&fq->buf_) - 1;
        new_slot = ccc_buf_at(&fq->buf_, fq->front_);
    }
    }
    ccc_buf_size_plus(&fq->buf_);
    return new_slot;
}

void *
ccc_fdeq_begin(ccc_flat_double_ended_queue const *fq)
{
    return ccc_buf_at(&fq->impl_.buf_, fq->impl_.front_);
}

void *
ccc_fdeq_rbegin(ccc_flat_double_ended_queue const *fq)
{
    if (!ccc_buf_size(&fq->impl_.buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->impl_.buf_,
                      ((fq->impl_.front_ + (ccc_buf_size(&fq->impl_.buf_) - 1))
                       % ccc_buf_capacity(&fq->impl_.buf_)));
}

void *
ccc_fdeq_next(ccc_flat_double_ended_queue const *fq, void const *iter_ptr)
{
    size_t const next_i = (index_of(&fq->impl_, iter_ptr) + 1)
                          % ccc_buf_capacity(&fq->impl_.buf_);
    if (distance(&fq->impl_, next_i, fq->impl_.front_)
        >= ccc_buf_size(&fq->impl_.buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->impl_.buf_, next_i);
}

void *
ccc_fdeq_rnext(ccc_flat_double_ended_queue const *fq, void const *iter_ptr)
{
    size_t const cur_i = index_of(&fq->impl_, iter_ptr);
    size_t const next_i
        = cur_i ? cur_i - 1 : ccc_buf_capacity(&fq->impl_.buf_) - 1;
    if (rdistance(&fq->impl_, next_i, fq->impl_.front_)
        >= ccc_buf_size(&fq->impl_.buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->impl_.buf_, next_i);
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

static ccc_result
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
    memcpy(new_mem, ccc_buf_at(&q->buf_, q->front_), bytes(q, first_chunk));
    if (first_chunk < ccc_buf_size(&q->buf_))
    {
        memcpy((uint8_t *)new_mem + bytes(q, first_chunk),
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

static size_t
index_of(struct ccc_fdeq_ const *const fq, void const *const pos)
{
    assert(pos > ccc_buf_base(&fq->buf_));
    assert(pos < ccc_buf_capacity_end(&fq->buf_));
    return ((uint8_t *)pos - (uint8_t *)ccc_buf_base(&fq->buf_))
           / ccc_buf_elem_size(&fq->buf_);
}

static inline size_t
bytes(struct ccc_fdeq_ const *q, size_t n)
{
    return ccc_buf_elem_size(&q->buf_) * n;
}
