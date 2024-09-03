#include "flat_queue.h"
#include "impl_flat_queue.h"

#include <string.h>

static size_t const start_capacity = 8;

#define MIN(a, b)                                                              \
    ({                                                                         \
        __auto_type _a_ = (a);                                                 \
        __auto_type _b_ = (b);                                                 \
        _a_ < _b_ ? _a_ : _b_;                                                 \
    })

static ccc_result maybe_resize(struct ccc_impl_flat_queue *);
static size_t bytes(struct ccc_impl_flat_queue const *, size_t);

void
ccc_fq_clear(ccc_flat_queue *const fq, ccc_destructor_fn *destructor)
{
    if (!fq)
    {
        return;
    }
    if (!destructor)
    {
        fq->impl.buf.impl.sz = fq->impl.front = 0;
        return;
    }
    size_t const cap = ccc_buf_capacity(&fq->impl.buf);
    size_t const back = (fq->impl.front + ccc_buf_size(&fq->impl.buf))
                        % ccc_buf_capacity(&fq->impl.buf);
    for (size_t i = fq->impl.front; i != back; i = (i + 1) % cap)
    {
        destructor(ccc_buf_at(&fq->impl.buf, i));
    }
}

void
ccc_fq_clear_and_free(ccc_flat_queue *const fq, ccc_destructor_fn *destructor)
{
    if (!fq)
    {
        return;
    }
    if (!destructor)
    {
        fq->impl.buf.impl.sz = fq->impl.front = 0;
        ccc_buf_realloc(&fq->impl.buf, 0, fq->impl.buf.impl.alloc);
        return;
    }
    size_t const cap = ccc_buf_capacity(&fq->impl.buf);
    size_t const back = (fq->impl.front + ccc_buf_size(&fq->impl.buf))
                        % ccc_buf_capacity(&fq->impl.buf);
    for (size_t i = fq->impl.front; i != back; i = (i + 1) % cap)
    {
        destructor(ccc_buf_at(&fq->impl.buf, i));
    }
    ccc_buf_realloc(&fq->impl.buf, 0, fq->impl.buf.impl.alloc);
}

void *
ccc_fq_push(ccc_flat_queue *const fq, void *const elem)
{
    if (!elem)
    {
        return NULL;
    }
    void *back = ccc_impl_fq_alloc(&fq->impl);
    memcpy(back, elem, ccc_buf_elem_size(&fq->impl.buf));
    return back;
}

void
ccc_fq_pop(ccc_flat_queue *const fq)
{
    if (ccc_buf_empty(&fq->impl.buf))
    {
        return;
    }
    fq->impl.front = (fq->impl.front + 1) % ccc_buf_capacity(&fq->impl.buf);
    --fq->impl.buf.impl.sz;
}

void *
ccc_fq_front(ccc_flat_queue const *const fq)
{
    if (ccc_buf_empty(&fq->impl.buf))
    {
        return NULL;
    }
    return ccc_buf_at(&fq->impl.buf, fq->impl.front);
}

bool
ccc_fq_empty(ccc_flat_queue const *const fq)
{
    return !fq || !fq->impl.buf.impl.sz;
}

size_t
ccc_fq_size(ccc_flat_queue const *const fq)
{
    return !fq ? 0 : fq->impl.buf.impl.sz;
}

void *
ccc_impl_fq_alloc(struct ccc_impl_flat_queue *fq)
{
    if (!fq || maybe_resize(fq) != CCC_OK)
    {
        return NULL;
    }
    void *new_slot = ccc_buf_at(&fq->buf, (fq->front + ccc_buf_size(&fq->buf))
                                              % ccc_buf_capacity(&fq->buf));
    ++fq->buf.impl.sz;
    return new_slot;
}

static ccc_result
maybe_resize(struct ccc_impl_flat_queue *const q)
{
    if (ccc_buf_size(&q->buf) < ccc_buf_capacity(&q->buf))
    {
        return CCC_OK;
    }
    if (!q->buf.impl.alloc)
    {
        return CCC_NO_REALLOC;
    }
    size_t const new_cap = ccc_buf_capacity(&q->buf)
                               ? (ccc_buf_capacity(&q->buf) * 2)
                               : start_capacity;
    void *new_mem
        = q->buf.impl.alloc(NULL, ccc_buf_elem_size(&q->buf) * new_cap);
    if (!new_mem)
    {
        return CCC_MEM_ERR;
    }
    size_t const first_chunk
        = MIN(ccc_buf_size(&q->buf), ccc_buf_capacity(&q->buf) - q->front);
    memcpy(new_mem, ccc_buf_at(&q->buf, q->front), bytes(q, first_chunk));
    if (first_chunk < ccc_buf_size(&q->buf))
    {
        memcpy((uint8_t *)new_mem + bytes(q, first_chunk),
               ccc_buf_base(&q->buf),
               bytes(q, ccc_buf_size(&q->buf) - first_chunk));
    }
    (void)ccc_buf_realloc(&q->buf, 0, q->buf.impl.alloc);
    q->buf.impl.mem = new_mem;
    q->front = 0;
    q->buf.impl.capacity = new_cap;
    return CCC_OK;
}

static inline size_t
bytes(struct ccc_impl_flat_queue const *q, size_t n)
{
    return ccc_buf_elem_size(&q->buf) * n;
}
