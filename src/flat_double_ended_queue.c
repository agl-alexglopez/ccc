#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "flat_double_ended_queue.h"
#include "impl/impl_flat_double_ended_queue.h"
#include "types.h"

static ptrdiff_t const start_capacity = 8;

/*==========================    Prototypes    ===============================*/

static ccc_result maybe_resize(struct ccc_fdeq_ *, ptrdiff_t);
static ptrdiff_t index_of(struct ccc_fdeq_ const *, void const *);
static void *at(struct ccc_fdeq_ const *, ptrdiff_t i);
static ptrdiff_t increment(struct ccc_fdeq_ const *, ptrdiff_t i);
static ptrdiff_t decrement(struct ccc_fdeq_ const *, ptrdiff_t i);
static ptrdiff_t distance(struct ccc_fdeq_ const *, ptrdiff_t, ptrdiff_t);
static ptrdiff_t rdistance(struct ccc_fdeq_ const *, ptrdiff_t, ptrdiff_t);
static ccc_result push_front_range(struct ccc_fdeq_ *fdeq, ptrdiff_t n,
                                   char const *elems);
static ccc_result push_back_range(struct ccc_fdeq_ *fdeq, ptrdiff_t n,
                                  char const *elems);
static ptrdiff_t back_free_slot(struct ccc_fdeq_ const *);
static ptrdiff_t front_free_slot(ptrdiff_t front, ptrdiff_t cap);
static ptrdiff_t last_elem_index(struct ccc_fdeq_ const *);
static void *push_range(struct ccc_fdeq_ *fdeq, char const *pos, ptrdiff_t n,
                        char const *elems);
static void *alloc_front(struct ccc_fdeq_ *fdeq);
static void *alloc_back(struct ccc_fdeq_ *fdeq);
static ptrdiff_t min(ptrdiff_t a, ptrdiff_t b);
static ccc_tribool add_overflow(ptrdiff_t a, ptrdiff_t b);

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
        (void)memcpy(slot, elem, ccc_buf_elem_size(&fdeq->buf_));
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
        (void)memcpy(slot, elem, ccc_buf_elem_size(&fdeq->buf_));
    }
    return slot;
}

ccc_result
ccc_fdeq_push_front_range(ccc_flat_double_ended_queue *const fdeq,
                          ptrdiff_t const n, void const *const elems)
{
    if (!fdeq || !elems || n < 0)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    return push_front_range(fdeq, n, elems);
}

ccc_result
ccc_fdeq_push_back_range(ccc_flat_double_ended_queue *const fdeq,
                         ptrdiff_t const n, void const *elems)
{
    if (!fdeq || !elems || n < 0)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    return push_back_range(fdeq, n, elems);
}

void *
ccc_fdeq_insert_range(ccc_flat_double_ended_queue *const fdeq, void *const pos,
                      ptrdiff_t const n, void const *elems)
{
    if (!fdeq || n < 0 || !elems)
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
                   : at(fdeq, ccc_buf_size(&fdeq->buf_) - n);
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
        return CCC_BOOL_ERR;
    }
    return !ccc_buf_size(&fdeq->buf_);
}

ptrdiff_t
ccc_fdeq_size(ccc_flat_double_ended_queue const *const fdeq)
{
    return fdeq ? ccc_buf_size(&fdeq->buf_) : 0;
}

ptrdiff_t
ccc_fdeq_capacity(ccc_flat_double_ended_queue const *const fdeq)
{
    return fdeq ? ccc_buf_capacity(&fdeq->buf_) : 0;
}

void *
ccc_fdeq_at(ccc_flat_double_ended_queue const *const fdeq, ptrdiff_t const i)
{
    if (!fdeq || i < 0 || i >= ccc_buf_capacity(&fdeq->buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf_,
                      (fdeq->front_ + i) % ccc_buf_capacity(&fdeq->buf_));
}

void *
ccc_fdeq_begin(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq || !ccc_buf_size(&fdeq->buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf_, fdeq->front_);
}

void *
ccc_fdeq_rbegin(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq || !ccc_buf_size(&fdeq->buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf_, last_elem_index(fdeq));
}

void *
ccc_fdeq_next(ccc_flat_double_ended_queue const *const fdeq,
              void const *const iter_ptr)
{
    ptrdiff_t const next_i = increment(fdeq, index_of(fdeq, iter_ptr));
    if (next_i == fdeq->front_
        || distance(fdeq, next_i, fdeq->front_) >= ccc_buf_size(&fdeq->buf_))
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf_, next_i);
}

void *
ccc_fdeq_rnext(ccc_flat_double_ended_queue const *const fdeq,
               void const *const iter_ptr)
{
    ptrdiff_t const cur_i = index_of(fdeq, iter_ptr);
    ptrdiff_t const next_i = decrement(fdeq, cur_i);
    ptrdiff_t const rbegin = last_elem_index(fdeq);
    if (next_i == rbegin
        || rdistance(fdeq, next_i, rbegin) >= ccc_buf_size(&fdeq->buf_))
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
    ptrdiff_t const dst_cap = dst->buf_.capacity_;
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
        ptrdiff_t const first_chunk
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
    ptrdiff_t const back = back_free_slot(fdeq);
    for (ptrdiff_t i = fdeq->front_; i != back; i = increment(fdeq, i))
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
    ptrdiff_t const back = back_free_slot(fdeq);
    for (ptrdiff_t i = fdeq->front_; i != back; i = increment(fdeq, i))
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
        return CCC_BOOL_ERR;
    }
    if (ccc_fdeq_is_empty(fdeq))
    {
        return CCC_TRUE;
    }
    void *iter = ccc_fdeq_begin(fdeq);
    if (ccc_buf_i(&fdeq->buf_, iter) != fdeq->front_)
    {
        return CCC_FALSE;
    }
    ptrdiff_t size = 0;
    for (; iter != ccc_fdeq_end(fdeq); iter = ccc_fdeq_next(fdeq, iter), ++size)
    {
        if (size >= ccc_fdeq_size(fdeq))
        {
            return CCC_FALSE;
        }
    }
    if (size != ccc_fdeq_size(fdeq))
    {
        return CCC_FALSE;
    }
    size = 0;
    iter = ccc_fdeq_rbegin(fdeq);
    if (ccc_buf_i(&fdeq->buf_, iter) != last_elem_index(fdeq))
    {
        return CCC_FALSE;
    }
    for (; iter != ccc_fdeq_rend(fdeq);
         iter = ccc_fdeq_rnext(fdeq, iter), ++size)
    {
        if (size >= ccc_fdeq_size(fdeq))
        {
            return CCC_FALSE;
        }
    }
    return size == ccc_fdeq_size(fdeq);
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
    fdeq->front_ = front_free_slot(fdeq->front_, ccc_buf_capacity(&fdeq->buf_));
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
push_back_range(struct ccc_fdeq_ *const fdeq, ptrdiff_t const n,
                char const *elems)
{
    size_t const elem_sz = ccc_buf_elem_size(&fdeq->buf_);
    ccc_tribool const full = maybe_resize(fdeq, n) != CCC_RESULT_OK;
    ptrdiff_t const cap = ccc_buf_capacity(&fdeq->buf_);
    if (fdeq->buf_.alloc_ && full)
    {
        return CCC_RESULT_MEM_ERR;
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
    ptrdiff_t const new_size = ccc_buf_size(&fdeq->buf_) + n;
    ptrdiff_t const back_slot = back_free_slot(fdeq);
    ptrdiff_t const chunk = min(n, cap - back_slot);
    ptrdiff_t const remainder_back_slot = (back_slot + chunk) % cap;
    ptrdiff_t const remainder = (n - chunk);
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
push_front_range(struct ccc_fdeq_ *const fdeq, ptrdiff_t const n,
                 char const *elems)
{
    size_t const elem_sz = ccc_buf_elem_size(&fdeq->buf_);
    ccc_tribool const full = maybe_resize(fdeq, n) != CCC_RESULT_OK;
    ptrdiff_t const cap = ccc_buf_capacity(&fdeq->buf_);
    if (fdeq->buf_.alloc_ && full)
    {
        return CCC_RESULT_MEM_ERR;
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
    ptrdiff_t const space_ahead = front_free_slot(fdeq->front_, cap) + 1;
    ptrdiff_t const i = n > space_ahead ? 0 : space_ahead - n;
    ptrdiff_t const chunk = min(n, space_ahead);
    ptrdiff_t const remainder = (n - chunk);
    char const *const first_chunk = elems + ((n - chunk) * elem_sz);
    (void)memcpy(ccc_buf_at(&fdeq->buf_, i), first_chunk, chunk * elem_sz);
    if (remainder)
    {
        (void)memcpy(ccc_buf_at(&fdeq->buf_, cap - remainder), elems,
                     remainder * elem_sz);
    }
    (void)ccc_buf_size_set(&fdeq->buf_,
                           min(cap, ccc_buf_size(&fdeq->buf_) + n));
    fdeq->front_ = remainder ? cap - remainder : i;
    return CCC_RESULT_OK;
}

static void *
push_range(struct ccc_fdeq_ *const fdeq, char const *const pos, ptrdiff_t n,
           char const *elems)
{
    size_t const elem_sz = ccc_buf_elem_size(&fdeq->buf_);
    ccc_tribool const full = maybe_resize(fdeq, n) != CCC_RESULT_OK;
    if (fdeq->buf_.alloc_ && full)
    {
        return NULL;
    }
    ptrdiff_t const cap = ccc_buf_capacity(&fdeq->buf_);
    ptrdiff_t const new_size = ccc_buf_size(&fdeq->buf_) + n;
    if (n >= cap)
    {
        elems += ((n - cap) * elem_sz);
        fdeq->front_ = 0;
        void *const ret = ccc_buf_at(&fdeq->buf_, 0);
        (void)memcpy(ret, elems, elem_sz * cap);
        (void)ccc_buf_size_set(&fdeq->buf_, cap);
        return ret;
    }
    ptrdiff_t const pos_i = index_of(fdeq, pos);
    ptrdiff_t const back = back_free_slot(fdeq);
    ptrdiff_t const to_move = back > pos_i ? back - pos_i : cap - pos_i + back;
    ptrdiff_t const move_i = (pos_i + n) % cap;
    ptrdiff_t move_chunk = move_i + to_move > cap ? cap - move_i : to_move;
    move_chunk = back < pos_i ? min(cap - pos_i, move_chunk)
                              : min(back - pos_i, move_chunk);
    ptrdiff_t const move_remain = to_move - move_chunk;
    (void)memmove(ccc_buf_at(&fdeq->buf_, move_i),
                  ccc_buf_at(&fdeq->buf_, pos_i), move_chunk * elem_sz);
    if (move_remain)
    {
        ptrdiff_t const move_remain_i = (move_i + move_chunk) % cap;
        ptrdiff_t const remaining_start_i = (pos_i + move_chunk) % cap;
        (void)memmove(ccc_buf_at(&fdeq->buf_, move_remain_i),
                      ccc_buf_at(&fdeq->buf_, remaining_start_i),
                      move_remain * elem_sz);
    }
    ptrdiff_t const elems_chunk = min(n, cap - pos_i);
    ptrdiff_t const elems_remain = n - elems_chunk;
    (void)memcpy(ccc_buf_at(&fdeq->buf_, pos_i), elems, elems_chunk * elem_sz);
    if (elems_remain)
    {
        char const *const second_chunk = elems + (elems_chunk * elem_sz);
        ptrdiff_t const second_chunk_i = (pos_i + elems_chunk) % cap;
        (void)memcpy(ccc_buf_at(&fdeq->buf_, second_chunk_i), second_chunk,
                     elems_remain * elem_sz);
    }
    if (new_size > cap)
    {
        ptrdiff_t const excess = (new_size - cap);
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
maybe_resize(struct ccc_fdeq_ *const q, ptrdiff_t const additional_elems_to_add)
{
    if (ccc_buf_size(&q->buf_) < 0 || additional_elems_to_add < 0
        || add_overflow(ccc_buf_size(&q->buf_), additional_elems_to_add))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (ccc_buf_size(&q->buf_) + additional_elems_to_add
        < ccc_buf_capacity(&q->buf_))
    {
        return CCC_RESULT_OK;
    }
    if (!q->buf_.alloc_)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    size_t const elem_sz = ccc_buf_elem_size(&q->buf_);
    ptrdiff_t const new_cap
        = ccc_buf_capacity(&q->buf_)
              ? ((ccc_buf_capacity(&q->buf_) + additional_elems_to_add) * 2)
              : start_capacity;
    void *const new_mem = q->buf_.alloc_(
        NULL, ccc_buf_elem_size(&q->buf_) * new_cap, q->buf_.aux_);
    if (!new_mem)
    {
        return CCC_RESULT_MEM_ERR;
    }
    if (ccc_buf_size(&q->buf_))
    {
        ptrdiff_t const first_chunk = min(
            ccc_buf_size(&q->buf_), ccc_buf_capacity(&q->buf_) - q->front_);
        (void)memcpy(new_mem, ccc_buf_at(&q->buf_, q->front_),
                     elem_sz * first_chunk);
        if (first_chunk < ccc_buf_size(&q->buf_))
        {
            (void)memcpy((char *)new_mem + (elem_sz * first_chunk),
                         ccc_buf_begin(&q->buf_),
                         elem_sz * (ccc_buf_size(&q->buf_) - first_chunk));
        }
    }
    (void)ccc_buf_alloc(&q->buf_, 0, q->buf_.alloc_);
    q->buf_.mem_ = new_mem;
    q->front_ = 0;
    q->buf_.capacity_ = new_cap;
    return CCC_RESULT_OK;
}

static inline ptrdiff_t
distance(struct ccc_fdeq_ const *const fdeq, ptrdiff_t const iter,
         ptrdiff_t const origin)
{
    return iter > origin ? iter - origin
                         : (ccc_buf_capacity(&fdeq->buf_) - origin) + iter;
}

static inline ptrdiff_t
rdistance(struct ccc_fdeq_ const *const fdeq, ptrdiff_t const iter,
          ptrdiff_t const origin)
{
    return iter > origin ? (ccc_buf_capacity(&fdeq->buf_) - iter) + origin
                         : origin - iter;
}

static inline ptrdiff_t
index_of(struct ccc_fdeq_ const *const fdeq, void const *const pos)
{
    assert(pos >= ccc_buf_begin(&fdeq->buf_));
    assert(pos < ccc_buf_capacity_end(&fdeq->buf_));
    return (ptrdiff_t)(((char *)pos - (char *)ccc_buf_begin(&fdeq->buf_))
                       / ccc_buf_elem_size(&fdeq->buf_));
}

static inline void *
at(struct ccc_fdeq_ const *const fdeq, ptrdiff_t const i)
{
    return ccc_buf_at(&fdeq->buf_,
                      (fdeq->front_ + i) % ccc_buf_capacity(&fdeq->buf_));
}

static inline ptrdiff_t
increment(struct ccc_fdeq_ const *const fdeq, ptrdiff_t const i)
{
    return i == (ccc_buf_capacity(&fdeq->buf_) - 1) ? 0 : i + 1;
}

static inline ptrdiff_t
decrement(struct ccc_fdeq_ const *const fdeq, ptrdiff_t const i)
{
    return i ? i - 1 : ccc_buf_capacity(&fdeq->buf_) - 1;
}

static inline ptrdiff_t
back_free_slot(struct ccc_fdeq_ const *const fdeq)
{
    return (fdeq->front_ + ccc_buf_size(&fdeq->buf_))
           % ccc_buf_capacity(&fdeq->buf_);
}

static inline ptrdiff_t
front_free_slot(ptrdiff_t const front, ptrdiff_t const cap)
{
    return front ? front - 1 : cap - 1;
}

/* Assumes the queue is not empty. */
static inline ptrdiff_t
last_elem_index(struct ccc_fdeq_ const *const fdeq)
{
    return (fdeq->front_ + (ccc_buf_size(&fdeq->buf_) - 1))
           % ccc_buf_capacity(&fdeq->buf_);
}

static inline ptrdiff_t
min(ptrdiff_t const a, ptrdiff_t const b)
{
    return a < b ? a : b;
}

static inline ccc_tribool
add_overflow(ptrdiff_t const a, ptrdiff_t const b)
{
    return PTRDIFF_MAX - b < a;
}
