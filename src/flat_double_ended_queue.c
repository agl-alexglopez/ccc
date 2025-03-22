#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "flat_double_ended_queue.h"
#include "impl/impl_flat_double_ended_queue.h"
#include "types.h"

static size_t const start_capacity = 8;

/*==========================    Prototypes    ===============================*/

static ccc_result maybe_resize(struct ccc_fdeq_ *, size_t);
static size_t index_of(struct ccc_fdeq_ const *, void const *);
static void *at(struct ccc_fdeq_ const *, size_t i);
static size_t increment(struct ccc_fdeq_ const *, size_t i);
static size_t decrement(struct ccc_fdeq_ const *, size_t i);
static size_t distance(struct ccc_fdeq_ const *, size_t, size_t);
static size_t rdistance(struct ccc_fdeq_ const *, size_t, size_t);
static ccc_result push_front_range(struct ccc_fdeq_ *fdeq, size_t n,
                                   char const *elems);
static ccc_result push_back_range(struct ccc_fdeq_ *fdeq, size_t n,
                                  char const *elems);
static size_t back_free_slot(struct ccc_fdeq_ const *);
static size_t front_free_slot(size_t front, size_t cap);
static size_t last_elem_index(struct ccc_fdeq_ const *);
static void *push_range(struct ccc_fdeq_ *fdeq, char const *pos, size_t n,
                        char const *elems);
static void *alloc_front(struct ccc_fdeq_ *fdeq);
static void *alloc_back(struct ccc_fdeq_ *fdeq);
static size_t min(size_t a, size_t b);

/*==========================     Interface    ===============================*/

void *
ccc_fdeq_push_back(ccc_flat_double_ended_queue *const fdeq,
                   void const *const elem)
{
    if (!fdeq || !elem)
    {
        return NULL;
    }
    void *const slot = alloc_back(fdeq);
    if (slot && slot != elem)
    {
        (void)memcpy(slot, elem, fdeq->buf_.elem_sz_);
    }
    return slot;
}

void *
ccc_fdeq_push_front(ccc_flat_double_ended_queue *const fdeq,
                    void const *const elem)
{
    if (!fdeq || !elem)
    {
        return NULL;
    }
    void *const slot = alloc_front(fdeq);
    if (slot && slot != elem)
    {
        (void)memcpy(slot, elem, fdeq->buf_.elem_sz_);
    }
    return slot;
}

ccc_result
ccc_fdeq_push_front_range(ccc_flat_double_ended_queue *const fdeq,
                          size_t const n, void const *const elems)
{
    if (!fdeq || !elems)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    return push_front_range(fdeq, n, elems);
}

ccc_result
ccc_fdeq_push_back_range(ccc_flat_double_ended_queue *const fdeq,
                         size_t const n, void const *elems)
{
    if (!fdeq || !elems)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    return push_back_range(fdeq, n, elems);
}

void *
ccc_fdeq_insert_range(ccc_flat_double_ended_queue *const fdeq, void *const pos,
                      size_t const n, void const *elems)
{
    if (!fdeq || !elems)
    {
        return NULL;
    }
    if (!n)
    {
        return pos;
    }
    if (pos == ccc_fdeq_begin(fdeq))
    {
        return push_front_range(fdeq, n, elems) != CCC_RESULT_OK
                   ? NULL
                   : at(fdeq, n - 1);
    }
    if (pos == ccc_fdeq_end(fdeq))
    {
        return push_back_range(fdeq, n, elems) != CCC_RESULT_OK
                   ? NULL
                   : at(fdeq, fdeq->buf_.sz_ - n);
    }
    return push_range(fdeq, pos, n, elems);
}

ccc_result
ccc_fdeq_pop_front(ccc_flat_double_ended_queue *const fdeq)
{
    if (!fdeq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (ccc_buf_is_empty(&fdeq->buf_))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    fdeq->front_ = increment(fdeq, fdeq->front_);
    return ccc_buf_size_minus(&fdeq->buf_, 1);
}

ccc_result
ccc_fdeq_pop_back(ccc_flat_double_ended_queue *const fdeq)
{
    if (!fdeq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    return ccc_buf_size_minus(&fdeq->buf_, 1);
}

void *
ccc_fdeq_front(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq || ccc_buf_is_empty(&fdeq->buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf_, fdeq->front_);
}

void *
ccc_fdeq_back(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq || ccc_buf_is_empty(&fdeq->buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf_, last_elem_index(fdeq));
}

ccc_tribool
ccc_fdeq_is_empty(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !fdeq->buf_.sz_;
}

ccc_ucount
ccc_fdeq_size(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = fdeq->buf_.sz_};
}

ccc_ucount
ccc_fdeq_capacity(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = fdeq->buf_.capacity_};
}

void *
ccc_fdeq_at(ccc_flat_double_ended_queue const *const fdeq, size_t const i)
{
    if (!fdeq || i >= fdeq->buf_.capacity_)
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf_, (fdeq->front_ + i) % fdeq->buf_.capacity_);
}

void *
ccc_fdeq_begin(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq || !fdeq->buf_.sz_)
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf_, fdeq->front_);
}

void *
ccc_fdeq_rbegin(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq || !fdeq->buf_.sz_)
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf_, last_elem_index(fdeq));
}

void *
ccc_fdeq_next(ccc_flat_double_ended_queue const *const fdeq,
              void const *const iter_ptr)
{
    size_t const next_i = increment(fdeq, index_of(fdeq, iter_ptr));
    if (next_i == fdeq->front_
        || distance(fdeq, next_i, fdeq->front_) >= fdeq->buf_.sz_)
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf_, next_i);
}

void *
ccc_fdeq_rnext(ccc_flat_double_ended_queue const *const fdeq,
               void const *const iter_ptr)
{
    size_t const cur_i = index_of(fdeq, iter_ptr);
    size_t const next_i = decrement(fdeq, cur_i);
    size_t const rbegin = last_elem_index(fdeq);
    if (next_i == rbegin || rdistance(fdeq, next_i, rbegin) >= fdeq->buf_.sz_)
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf_, next_i);
}

void *
ccc_fdeq_end(ccc_flat_double_ended_queue const *const)
{
    return NULL;
}

void *
ccc_fdeq_rend(ccc_flat_double_ended_queue const *const)
{
    return NULL;
}

void *
ccc_fdeq_data(ccc_flat_double_ended_queue const *const fdeq)
{
    return fdeq ? ccc_buf_begin(&fdeq->buf_) : NULL;
}

ccc_result
ccc_fdeq_copy(ccc_flat_double_ended_queue *const dst,
              ccc_flat_double_ended_queue const *const src,
              ccc_alloc_fn *const fn)
{
    if (!dst || !src || src == dst
        || (dst->buf_.capacity_ < src->buf_.capacity_ && !fn))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Copy everything so we don't worry about staying in sync with future
       changes to buf container. But we have to give back original destination
       memory in case it has already been allocated. Alloc will remain the
       same as in dst initialization because that controls whether the fdeq
       is a ring buffer or dynamic fdeq. */
    void *const dst_mem = dst->buf_.mem_;
    size_t const dst_cap = dst->buf_.capacity_;
    ccc_alloc_fn *const dst_alloc = dst->buf_.alloc_;
    *dst = *src;
    dst->buf_.mem_ = dst_mem;
    dst->buf_.capacity_ = dst_cap;
    dst->buf_.alloc_ = dst_alloc;
    /* Copying from an empty source is odd but supported. */
    if (!src->buf_.capacity_)
    {
        return CCC_RESULT_OK;
    }
    if (dst->buf_.capacity_ < src->buf_.capacity_)
    {
        ccc_result resize_res
            = ccc_buf_alloc(&dst->buf_, src->buf_.capacity_, fn);
        if (resize_res != CCC_RESULT_OK)
        {
            return resize_res;
        }
        dst->buf_.capacity_ = src->buf_.capacity_;
    }
    if (!dst->buf_.mem_ || !src->buf_.mem_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (dst->buf_.capacity_ > src->buf_.capacity_)
    {
        size_t const first_chunk
            = min(src->buf_.sz_, src->buf_.capacity_ - src->front_);
        (void)memcpy(dst->buf_.mem_, ccc_buf_at(&src->buf_, src->front_),
                     src->buf_.elem_sz_ * first_chunk);
        if (first_chunk < src->buf_.sz_)
        {
            (void)memcpy((char *)dst->buf_.mem_
                             + (src->buf_.elem_sz_ * first_chunk),
                         src->buf_.mem_,
                         src->buf_.elem_sz_ * (src->buf_.sz_ - first_chunk));
        }
        dst->front_ = 0;
        return CCC_RESULT_OK;
    }
    (void)memcpy(dst->buf_.mem_, src->buf_.mem_,
                 src->buf_.capacity_ * src->buf_.elem_sz_);
    return CCC_RESULT_OK;
}

ccc_result
ccc_fdeq_clear(ccc_flat_double_ended_queue *const fdeq,
               ccc_destructor_fn *const destructor)
{
    if (!fdeq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!destructor)
    {
        fdeq->front_ = 0;
        return ccc_buf_size_set(&fdeq->buf_, 0);
    }
    size_t const back = back_free_slot(fdeq);
    for (size_t i = fdeq->front_; i != back; i = increment(fdeq, i))
    {
        destructor((ccc_user_type){.user_type = ccc_buf_at(&fdeq->buf_, i),
                                   .aux = fdeq->buf_.aux_});
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_fdeq_clear_and_free(ccc_flat_double_ended_queue *const fdeq,
                        ccc_destructor_fn *const destructor)
{
    if (!fdeq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!destructor)
    {
        fdeq->buf_.sz_ = fdeq->front_ = 0;
        return ccc_buf_alloc(&fdeq->buf_, 0, fdeq->buf_.alloc_);
    }
    size_t const back = back_free_slot(fdeq);
    for (size_t i = fdeq->front_; i != back; i = increment(fdeq, i))
    {
        destructor((ccc_user_type){.user_type = ccc_buf_at(&fdeq->buf_, i),
                                   .aux = fdeq->buf_.aux_});
    }
    return ccc_buf_alloc(&fdeq->buf_, 0, fdeq->buf_.alloc_);
}

ccc_tribool
ccc_fdeq_validate(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (ccc_fdeq_is_empty(fdeq))
    {
        return CCC_TRUE;
    }
    void *iter = ccc_fdeq_begin(fdeq);
    if (ccc_buf_i(&fdeq->buf_, iter).count != fdeq->front_)
    {
        return CCC_FALSE;
    }
    size_t size = 0;
    for (; iter != ccc_fdeq_end(fdeq); iter = ccc_fdeq_next(fdeq, iter), ++size)
    {
        if (size >= ccc_fdeq_size(fdeq).count)
        {
            return CCC_FALSE;
        }
    }
    if (size != ccc_fdeq_size(fdeq).count)
    {
        return CCC_FALSE;
    }
    size = 0;
    iter = ccc_fdeq_rbegin(fdeq);
    if (ccc_buf_i(&fdeq->buf_, iter).count != last_elem_index(fdeq))
    {
        return CCC_FALSE;
    }
    for (; iter != ccc_fdeq_rend(fdeq);
         iter = ccc_fdeq_rnext(fdeq, iter), ++size)
    {
        if (size >= ccc_fdeq_size(fdeq).count)
        {
            return CCC_FALSE;
        }
    }
    return size == ccc_fdeq_size(fdeq).count;
}

/*======================   Private Interface   ==============================*/

void *
ccc_impl_fdeq_alloc_back(struct ccc_fdeq_ *const fdeq)
{
    return alloc_back(fdeq);
}

void *
ccc_impl_fdeq_alloc_front(struct ccc_fdeq_ *const fdeq)
{
    return alloc_front(fdeq);
}

/*======================     Static Helpers    ==============================*/

static void *
alloc_front(struct ccc_fdeq_ *const fdeq)
{
    ccc_tribool const full = maybe_resize(fdeq, 0) != CCC_RESULT_OK;
    /* Should have been able to resize. Bad error. */
    if (fdeq->buf_.alloc_ && full)
    {
        return NULL;
    }
    fdeq->front_ = front_free_slot(fdeq->front_, fdeq->buf_.capacity_);
    void *const new_slot = ccc_buf_at(&fdeq->buf_, fdeq->front_);
    if (!full)
    {
        (void)ccc_buf_size_plus(&fdeq->buf_, 1);
    }
    return new_slot;
}

static void *
alloc_back(struct ccc_fdeq_ *const fdeq)
{
    ccc_tribool const full = maybe_resize(fdeq, 0) != CCC_RESULT_OK;
    /* Should have been able to resize. Bad error. */
    if (fdeq->buf_.alloc_ && full)
    {
        return NULL;
    }
    void *const new_slot = ccc_buf_at(&fdeq->buf_, back_free_slot(fdeq));
    /* If no reallocation policy is given we are a ring buffer. */
    if (full)
    {
        fdeq->front_ = increment(fdeq, fdeq->front_);
    }
    else
    {
        (void)ccc_buf_size_plus(&fdeq->buf_, 1);
    }
    return new_slot;
}

static ccc_result
push_back_range(struct ccc_fdeq_ *const fdeq, size_t const n, char const *elems)
{
    size_t const elem_sz = fdeq->buf_.elem_sz_;
    ccc_tribool const full = maybe_resize(fdeq, n) != CCC_RESULT_OK;
    size_t const cap = fdeq->buf_.capacity_;
    if (fdeq->buf_.alloc_ && full)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    /* If a range is too large we can make various simplifications to preserve
       the final capacity elements. */
    if (n >= cap)
    {
        elems += ((n - cap) * elem_sz);
        fdeq->front_ = 0;
        (void)memcpy(ccc_buf_at(&fdeq->buf_, 0), elems, elem_sz * cap);
        (void)ccc_buf_size_set(&fdeq->buf_, cap);
        return CCC_RESULT_OK;
    }
    size_t const new_size = fdeq->buf_.sz_ + n;
    size_t const back_slot = back_free_slot(fdeq);
    size_t const chunk = min(n, cap - back_slot);
    size_t const remainder_back_slot = (back_slot + chunk) % cap;
    size_t const remainder = (n - chunk);
    char const *const second_chunk = elems + (chunk * elem_sz);
    (void)memcpy(ccc_buf_at(&fdeq->buf_, back_slot), elems, chunk * elem_sz);
    if (remainder)
    {
        (void)memcpy(ccc_buf_at(&fdeq->buf_, remainder_back_slot), second_chunk,
                     remainder * elem_sz);
    }
    if (new_size > cap)
    {
        fdeq->front_ = (fdeq->front_ + (new_size - cap)) % cap;
    }
    (void)ccc_buf_size_set(&fdeq->buf_, min(cap, new_size));
    return CCC_RESULT_OK;
}

static ccc_result
push_front_range(struct ccc_fdeq_ *const fdeq, size_t const n,
                 char const *elems)
{
    size_t const elem_sz = fdeq->buf_.elem_sz_;
    ccc_tribool const full = maybe_resize(fdeq, n) != CCC_RESULT_OK;
    size_t const cap = fdeq->buf_.capacity_;
    if (fdeq->buf_.alloc_ && full)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    /* If a range is too large we can make various simplifications to preserve
       the final capacity elements. */
    if (n >= cap)
    {
        elems += ((n - cap) * elem_sz);
        fdeq->front_ = 0;
        (void)memcpy(ccc_buf_at(&fdeq->buf_, 0), elems, elem_sz * cap);
        (void)ccc_buf_size_set(&fdeq->buf_, cap);
        return CCC_RESULT_OK;
    }
    size_t const space_ahead = front_free_slot(fdeq->front_, cap) + 1;
    size_t const i = n > space_ahead ? 0 : space_ahead - n;
    size_t const chunk = min(n, space_ahead);
    size_t const remainder = (n - chunk);
    char const *const first_chunk = elems + ((n - chunk) * elem_sz);
    (void)memcpy(ccc_buf_at(&fdeq->buf_, i), first_chunk, chunk * elem_sz);
    if (remainder)
    {
        (void)memcpy(ccc_buf_at(&fdeq->buf_, cap - remainder), elems,
                     remainder * elem_sz);
    }
    (void)ccc_buf_size_set(&fdeq->buf_, min(cap, fdeq->buf_.sz_ + n));
    fdeq->front_ = remainder ? cap - remainder : i;
    return CCC_RESULT_OK;
}

static void *
push_range(struct ccc_fdeq_ *const fdeq, char const *const pos, size_t n,
           char const *elems)
{
    size_t const elem_sz = fdeq->buf_.elem_sz_;
    ccc_tribool const full = maybe_resize(fdeq, n) != CCC_RESULT_OK;
    if (fdeq->buf_.alloc_ && full)
    {
        return NULL;
    }
    size_t const cap = fdeq->buf_.capacity_;
    size_t const new_size = fdeq->buf_.sz_ + n;
    if (n >= cap)
    {
        elems += ((n - cap) * elem_sz);
        fdeq->front_ = 0;
        void *const ret = ccc_buf_at(&fdeq->buf_, 0);
        (void)memcpy(ret, elems, elem_sz * cap);
        (void)ccc_buf_size_set(&fdeq->buf_, cap);
        return ret;
    }
    size_t const pos_i = index_of(fdeq, pos);
    size_t const back = back_free_slot(fdeq);
    size_t const to_move = back > pos_i ? back - pos_i : cap - pos_i + back;
    size_t const move_i = (pos_i + n) % cap;
    size_t move_chunk = move_i + to_move > cap ? cap - move_i : to_move;
    move_chunk = back < pos_i ? min(cap - pos_i, move_chunk)
                              : min(back - pos_i, move_chunk);
    size_t const move_remain = to_move - move_chunk;
    (void)memmove(ccc_buf_at(&fdeq->buf_, move_i),
                  ccc_buf_at(&fdeq->buf_, pos_i), move_chunk * elem_sz);
    if (move_remain)
    {
        size_t const move_remain_i = (move_i + move_chunk) % cap;
        size_t const remaining_start_i = (pos_i + move_chunk) % cap;
        (void)memmove(ccc_buf_at(&fdeq->buf_, move_remain_i),
                      ccc_buf_at(&fdeq->buf_, remaining_start_i),
                      move_remain * elem_sz);
    }
    size_t const elems_chunk = min(n, cap - pos_i);
    size_t const elems_remain = n - elems_chunk;
    (void)memcpy(ccc_buf_at(&fdeq->buf_, pos_i), elems, elems_chunk * elem_sz);
    if (elems_remain)
    {
        char const *const second_chunk = elems + (elems_chunk * elem_sz);
        size_t const second_chunk_i = (pos_i + elems_chunk) % cap;
        (void)memcpy(ccc_buf_at(&fdeq->buf_, second_chunk_i), second_chunk,
                     elems_remain * elem_sz);
    }
    if (new_size > cap)
    {
        size_t const excess = (new_size - cap);
        /* Wrapping behavior stops if it would overwrite the start of the
           range being inserted. This is to preserve as much info about
           the range as possible. If wrapping occurs the range is the new
           front. */
        if ((fdeq->front_ <= pos_i && fdeq->front_ + excess >= pos_i)
            || (fdeq->front_ + excess > cap
                && ((fdeq->front_ + excess) - cap) >= pos_i))
        {
            fdeq->front_ = pos_i;
        }
        else
        {
            fdeq->front_ = (fdeq->front_ + excess) % cap;
        }
    }
    (void)ccc_buf_size_set(&fdeq->buf_, min(cap, new_size));
    return ccc_buf_at(&fdeq->buf_, pos_i);
}

static ccc_result
maybe_resize(struct ccc_fdeq_ *const q, size_t const additional_elems_to_add)
{
    size_t const old_size = ccc_buf_size(&q->buf_).count;
    if (old_size + additional_elems_to_add < old_size)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (old_size + additional_elems_to_add < ccc_buf_capacity(&q->buf_).count)
    {
        return CCC_RESULT_OK;
    }
    if (!q->buf_.alloc_)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    size_t const elem_sz = q->buf_.elem_sz_;
    size_t const new_cap
        = ccc_buf_capacity(&q->buf_).count
              ? ((ccc_buf_capacity(&q->buf_).count + additional_elems_to_add)
                 * 2)
              : start_capacity;
    void *const new_mem
        = q->buf_.alloc_(NULL, q->buf_.elem_sz_ * new_cap, q->buf_.aux_);
    if (!new_mem)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    if (ccc_buf_size(&q->buf_).count)
    {
        size_t const first_chunk
            = min(ccc_buf_size(&q->buf_).count,
                  ccc_buf_capacity(&q->buf_).count - q->front_);
        (void)memcpy(new_mem, ccc_buf_at(&q->buf_, q->front_),
                     elem_sz * first_chunk);
        if (first_chunk < ccc_buf_size(&q->buf_).count)
        {
            (void)memcpy((char *)new_mem + (elem_sz * first_chunk),
                         ccc_buf_begin(&q->buf_),
                         elem_sz
                             * (ccc_buf_size(&q->buf_).count - first_chunk));
        }
    }
    (void)ccc_buf_alloc(&q->buf_, 0, q->buf_.alloc_);
    q->buf_.mem_ = new_mem;
    q->front_ = 0;
    q->buf_.capacity_ = new_cap;
    return CCC_RESULT_OK;
}

static inline size_t
distance(struct ccc_fdeq_ const *const fdeq, size_t const iter,
         size_t const origin)
{
    return iter > origin ? iter - origin
                         : (fdeq->buf_.capacity_ - origin) + iter;
}

static inline size_t
rdistance(struct ccc_fdeq_ const *const fdeq, size_t const iter,
          size_t const origin)
{
    return iter > origin ? (fdeq->buf_.capacity_ - iter) + origin
                         : origin - iter;
}

static inline size_t
index_of(struct ccc_fdeq_ const *const fdeq, void const *const pos)
{
    assert(pos >= ccc_buf_begin(&fdeq->buf_));
    assert(pos < ccc_buf_capacity_end(&fdeq->buf_));
    return (size_t)(((char *)pos - (char *)ccc_buf_begin(&fdeq->buf_))
                    / fdeq->buf_.elem_sz_);
}

static inline void *
at(struct ccc_fdeq_ const *const fdeq, size_t const i)
{
    return ccc_buf_at(&fdeq->buf_, (fdeq->front_ + i) % fdeq->buf_.capacity_);
}

static inline size_t
increment(struct ccc_fdeq_ const *const fdeq, size_t const i)
{
    return i == (fdeq->buf_.capacity_ - 1) ? 0 : i + 1;
}

static inline size_t
decrement(struct ccc_fdeq_ const *const fdeq, size_t const i)
{
    return i ? i - 1 : fdeq->buf_.capacity_ - 1;
}

static inline size_t
back_free_slot(struct ccc_fdeq_ const *const fdeq)
{
    return (fdeq->front_ + fdeq->buf_.sz_) % fdeq->buf_.capacity_;
}

static inline size_t
front_free_slot(size_t const front, size_t const cap)
{
    return front ? front - 1 : cap - 1;
}

/* Assumes the queue is not empty. */
static inline size_t
last_elem_index(struct ccc_fdeq_ const *const fdeq)
{
    return (fdeq->front_ + (fdeq->buf_.sz_ - 1)) % fdeq->buf_.capacity_;
}

static inline size_t
min(size_t const a, size_t const b)
{
    return a < b ? a : b;
}
