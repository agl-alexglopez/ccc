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

/*==========================    Prototypes    ===============================*/

static ccc_result maybe_resize(struct ccc_fdeq_ *, size_t);
static size_t index_of(struct ccc_fdeq_ const *, void const *);
static void *at(struct ccc_fdeq_ const *, size_t i);
size_t increment(struct ccc_fdeq_ const *, size_t i);
size_t decrement(struct ccc_fdeq_ const *, size_t i);
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
    if (slot)
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
    if (slot)
    {
        (void)memcpy(slot, elem, ccc_buf_elem_size(&fdeq->buf_));
    }
    return slot;
}

ccc_result
ccc_fdeq_push_front_range(ccc_flat_double_ended_queue *const fdeq,
                          size_t const n, void const *const elems)
{
    if (!fdeq || !elems)
    {
        return CCC_INPUT_ERR;
    }
    return push_front_range(fdeq, n, elems);
}

ccc_result
ccc_fdeq_push_back_range(ccc_flat_double_ended_queue *const fdeq,
                         size_t const n, void const *elems)
{
    if (!fdeq || !elems)
    {
        return CCC_INPUT_ERR;
    }
    return push_back_range(fdeq, n, elems);
}

void *
ccc_fdeq_insert_range(ccc_flat_double_ended_queue *const fdeq, void *const pos,
                      size_t const n, void const *elems)
{
    if (!fdeq)
    {
        return NULL;
    }
    if (!n)
    {
        return pos;
    }
    if (pos == ccc_fdeq_begin(fdeq))
    {
        return push_front_range(fdeq, n, elems) != CCC_OK ? NULL
                                                          : at(fdeq, n - 1);
    }
    if (pos == ccc_fdeq_end(fdeq))
    {
        return push_back_range(fdeq, n, elems) != CCC_OK
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
        return CCC_INPUT_ERR;
    }
    if (ccc_buf_is_empty(&fdeq->buf_))
    {
        return CCC_INPUT_ERR;
    }
    fdeq->front_ = increment(fdeq, fdeq->front_);
    return ccc_buf_size_minus(&fdeq->buf_, 1);
}

ccc_result
ccc_fdeq_pop_back(ccc_flat_double_ended_queue *const fdeq)
{
    if (!fdeq)
    {
        return CCC_INPUT_ERR;
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

bool
ccc_fdeq_is_empty(ccc_flat_double_ended_queue const *const fdeq)
{
    return !fdeq || !ccc_buf_size(&fdeq->buf_);
}

size_t
ccc_fdeq_size(ccc_flat_double_ended_queue const *const fdeq)
{
    return fdeq ? ccc_buf_size(&fdeq->buf_) : 0;
}

void *
ccc_fdeq_at(ccc_flat_double_ended_queue const *const fdeq, size_t const i)
{
    if (!fdeq || i >= ccc_buf_capacity(&fdeq->buf_))
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
    size_t const next_i = increment(fdeq, index_of(fdeq, iter_ptr));
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
    size_t const cur_i = index_of(fdeq, iter_ptr);
    size_t const next_i = decrement(fdeq, cur_i);
    size_t const rbegin = last_elem_index(fdeq);
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

bool
ccc_fdeq_validate(ccc_flat_double_ended_queue const *const fdeq)
{
    if (ccc_fdeq_is_empty(fdeq))
    {
        return true;
    }
    void *iter = ccc_fdeq_begin(fdeq);
    if (ccc_buf_i(&fdeq->buf_, iter) != (ptrdiff_t)fdeq->front_)
    {
        return false;
    }
    size_t size = 0;
    for (; iter != ccc_fdeq_end(fdeq); iter = ccc_fdeq_next(fdeq, iter), ++size)
    {
        if (size >= ccc_fdeq_size(fdeq))
        {
            return false;
        }
    }
    if (size != ccc_fdeq_size(fdeq))
    {
        return false;
    }
    size = 0;
    iter = ccc_fdeq_rbegin(fdeq);
    if (ccc_buf_i(&fdeq->buf_, iter) != (ptrdiff_t)last_elem_index(fdeq))
    {
        return false;
    }
    for (; iter != ccc_fdeq_rend(fdeq);
         iter = ccc_fdeq_rnext(fdeq, iter), ++size)
    {
        if (size >= ccc_fdeq_size(fdeq))
        {
            return false;
        }
    }
    return size == ccc_fdeq_size(fdeq);
}

ccc_result
ccc_fdeq_clear(ccc_flat_double_ended_queue *const fdeq,
               ccc_destructor_fn *const destructor)
{
    if (!fdeq)
    {
        return CCC_INPUT_ERR;
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
                                   .aux = fdeq->aux_});
    }
    return CCC_OK;
}

ccc_result
ccc_fdeq_clear_and_free(ccc_flat_double_ended_queue *const fdeq,
                        ccc_destructor_fn *const destructor)
{
    if (!fdeq)
    {
        return CCC_INPUT_ERR;
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
                                   .aux = fdeq->aux_});
    }
    return ccc_buf_alloc(&fdeq->buf_, 0, fdeq->buf_.alloc_);
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

static inline void *
alloc_front(struct ccc_fdeq_ *const fdeq)
{
    void *new_slot = NULL;
    bool const full = maybe_resize(fdeq, 0) != CCC_OK;
    /* Should have been able to resize. Bad error. */
    if (fdeq->buf_.alloc_ && full)
    {
        return NULL;
    }
    fdeq->front_ = front_free_slot(fdeq->front_, ccc_buf_capacity(&fdeq->buf_));
    new_slot = ccc_buf_at(&fdeq->buf_, fdeq->front_);
    if (!full)
    {
        (void)ccc_buf_size_plus(&fdeq->buf_, 1);
    }
    return new_slot;
}

static inline void *
alloc_back(struct ccc_fdeq_ *const fdeq)
{
    void *new_slot = NULL;
    bool const full = maybe_resize(fdeq, 0) != CCC_OK;
    /* Should have been able to resize. Bad error. */
    if (fdeq->buf_.alloc_ && full)
    {
        return NULL;
    }
    new_slot = ccc_buf_at(&fdeq->buf_, back_free_slot(fdeq));
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

static inline ccc_result
push_back_range(struct ccc_fdeq_ *const fdeq, size_t const n, char const *elems)
{
    size_t const elem_sz = ccc_buf_elem_size(&fdeq->buf_);
    bool const full = maybe_resize(fdeq, n) != CCC_OK;
    size_t const cap = ccc_buf_capacity(&fdeq->buf_);
    if (fdeq->buf_.alloc_ && full)
    {
        return CCC_MEM_ERR;
    }
    /* If a range is too large we can make various simplifications to preserve
       the final capacity elements. */
    if (n >= cap)
    {
        elems += ((n - cap) * elem_sz);
        fdeq->front_ = 0;
        (void)memcpy(ccc_buf_at(&fdeq->buf_, 0), elems, elem_sz * cap);
        (void)ccc_buf_size_set(&fdeq->buf_, cap);
        return CCC_OK;
    }
    size_t const new_size = ccc_buf_size(&fdeq->buf_) + n;
    size_t const back_slot = back_free_slot(fdeq);
    size_t const chunk = MIN(n, cap - back_slot);
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
    (void)ccc_buf_size_set(&fdeq->buf_, MIN(cap, new_size));
    return CCC_OK;
}

static inline ccc_result
push_front_range(struct ccc_fdeq_ *const fdeq, size_t const n,
                 char const *elems)
{
    size_t const elem_sz = ccc_buf_elem_size(&fdeq->buf_);
    bool const full = maybe_resize(fdeq, n) != CCC_OK;
    size_t const cap = ccc_buf_capacity(&fdeq->buf_);
    if (fdeq->buf_.alloc_ && full)
    {
        return CCC_MEM_ERR;
    }
    /* If a range is too large we can make various simplifications to preserve
       the final capacity elements. */
    if (n >= cap)
    {
        elems += ((n - cap) * elem_sz);
        fdeq->front_ = 0;
        (void)memcpy(ccc_buf_at(&fdeq->buf_, 0), elems, elem_sz * cap);
        (void)ccc_buf_size_set(&fdeq->buf_, cap);
        return CCC_OK;
    }
    size_t const space_ahead = front_free_slot(fdeq->front_, cap) + 1;
    size_t const i = n > space_ahead ? 0 : space_ahead - n;
    size_t const chunk = MIN(n, space_ahead);
    size_t const remainder = (n - chunk);
    char const *const first_chunk = elems + ((n - chunk) * elem_sz);
    (void)memcpy(ccc_buf_at(&fdeq->buf_, i), first_chunk, chunk * elem_sz);
    if (remainder)
    {
        (void)memcpy(ccc_buf_at(&fdeq->buf_, cap - remainder), elems,
                     remainder * elem_sz);
    }
    (void)ccc_buf_size_set(&fdeq->buf_,
                           MIN(cap, ccc_buf_size(&fdeq->buf_) + n));
    fdeq->front_ = remainder ? cap - remainder : i;
    return CCC_OK;
}

static inline void *
push_range(struct ccc_fdeq_ *const fdeq, char const *const pos, size_t n,
           char const *elems)
{
    size_t const elem_sz = ccc_buf_elem_size(&fdeq->buf_);
    bool const full = maybe_resize(fdeq, n) != CCC_OK;
    if (fdeq->buf_.alloc_ && full)
    {
        return NULL;
    }
    size_t const cap = ccc_buf_capacity(&fdeq->buf_);
    size_t const new_size = ccc_buf_size(&fdeq->buf_) + n;
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
    move_chunk = back < pos_i ? MIN(cap - pos_i, move_chunk)
                              : MIN(back - pos_i, move_chunk);
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
    size_t const elems_chunk = MIN(n, cap - pos_i);
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
    (void)ccc_buf_size_set(&fdeq->buf_, MIN(cap, new_size));
    return ccc_buf_at(&fdeq->buf_, pos_i);
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
        return CCC_NO_ALLOC;
    }
    size_t const elem_sz = ccc_buf_elem_size(&q->buf_);
    size_t const new_cap
        = ccc_buf_capacity(&q->buf_)
              ? ((ccc_buf_capacity(&q->buf_) + additional_elems_to_add) * 2)
              : start_capacity;
    void *const new_mem
        = q->buf_.alloc_(NULL, ccc_buf_elem_size(&q->buf_) * new_cap);
    if (!new_mem)
    {
        return CCC_MEM_ERR;
    }
    if (ccc_buf_size(&q->buf_))
    {
        size_t const first_chunk = MIN(ccc_buf_size(&q->buf_),
                                       ccc_buf_capacity(&q->buf_) - q->front_);
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
    return CCC_OK;
}

static inline size_t
distance(struct ccc_fdeq_ const *const fdeq, size_t const iter,
         size_t const origin)
{
    return iter > origin ? iter - origin
                         : (ccc_buf_capacity(&fdeq->buf_) - origin) + iter;
}

static inline size_t
rdistance(struct ccc_fdeq_ const *const fdeq, size_t const iter,
          size_t const origin)
{
    return iter > origin ? (ccc_buf_capacity(&fdeq->buf_) - iter) + origin
                         : origin - iter;
}

static inline size_t
index_of(struct ccc_fdeq_ const *const fdeq, void const *const pos)
{
    assert(pos >= ccc_buf_begin(&fdeq->buf_));
    assert(pos < ccc_buf_capacity_end(&fdeq->buf_));
    return ((char *)pos - (char *)ccc_buf_begin(&fdeq->buf_))
           / ccc_buf_elem_size(&fdeq->buf_);
}

static void *
at(struct ccc_fdeq_ const *const fdeq, size_t const i)
{
    return ccc_buf_at(&fdeq->buf_,
                      (fdeq->front_ + i) % ccc_buf_capacity(&fdeq->buf_));
}

size_t
increment(struct ccc_fdeq_ const *const fdeq, size_t i)
{
    return i == (ccc_buf_capacity(&fdeq->buf_) - 1) ? 0 : ++i;
}

size_t
decrement(struct ccc_fdeq_ const *const fdeq, size_t i)
{
    return i ? --i : ccc_buf_capacity(&fdeq->buf_) - 1;
}

static inline size_t
back_free_slot(struct ccc_fdeq_ const *const fdeq)
{
    return (fdeq->front_ + ccc_buf_size(&fdeq->buf_))
           % ccc_buf_capacity(&fdeq->buf_);
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
    return (fdeq->front_ + (ccc_buf_size(&fdeq->buf_) - 1))
           % ccc_buf_capacity(&fdeq->buf_);
}
