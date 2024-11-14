#include "flat_priority_queue.h"
#include "buffer.h"
#include "impl/impl_flat_priority_queue.h"
#include "types.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

static void *at(struct ccc_fpq_ const *, size_t);
static size_t index_of(struct ccc_fpq_ const *, void const *);
static bool wins(struct ccc_fpq_ const *, void const *winner,
                 void const *loser);
static void swap(struct ccc_fpq_ *, char tmp[], size_t, size_t);
static size_t bubble_up(struct ccc_fpq_ *fpq, char tmp[], size_t i);
static void bubble_down(struct ccc_fpq_ *, char tmp[], size_t);

ccc_result
ccc_fpq_alloc(ccc_flat_priority_queue *const fpq, size_t const new_capacity,
              ccc_alloc_fn *const fn)
{
    if (!fpq || !fn)
    {
        return CCC_INPUT_ERR;
    }
    return ccc_buf_alloc(&fpq->buf_, new_capacity, fn);
}

void
ccc_impl_fpq_in_place_heapify(struct ccc_fpq_ *const fpq, size_t const n)
{
    if (!fpq || ccc_buf_capacity(&fpq->buf_) < n + 1)
    {
        return;
    }
    (void)ccc_buf_size_set(&fpq->buf_, n);
    void *const tmp = ccc_buf_at(&fpq->buf_, n);
    for (size_t i = (n / 2) + 1; i--;)
    {
        bubble_down(fpq, tmp, i);
    }
}

ccc_result
ccc_fpq_heapify(ccc_flat_priority_queue *const fpq, void *const array,
                size_t const n, size_t const input_elem_size)
{
    if (!fpq || !array || input_elem_size != ccc_buf_elem_size(&fpq->buf_))
    {
        return CCC_INPUT_ERR;
    }
    if (n + 1 > ccc_buf_capacity(&fpq->buf_))
    {
        ccc_result const resize_res
            = ccc_buf_alloc(&fpq->buf_, n + 1, fpq->buf_.alloc_);
        if (resize_res != CCC_OK)
        {
            return resize_res;
        }
    }
    (void)ccc_buf_size_set(&fpq->buf_, n);
    (void)memcpy(ccc_buf_begin(&fpq->buf_), array, n * input_elem_size);
    void *const tmp = ccc_buf_at(&fpq->buf_, n);
    for (size_t i = (n / 2) + 1; i--;)
    {
        bubble_down(fpq, tmp, i);
    }
    return CCC_OK;
}

void *
ccc_fpq_push(ccc_flat_priority_queue *const fpq, void const *const e)
{
    if (!fpq || !e)
    {
        return NULL;
    }
    void *new = ccc_buf_alloc_back(&fpq->buf_);
    if (ccc_buf_size(&fpq->buf_) == ccc_buf_capacity(&fpq->buf_))
    {
        new = NULL;
        ccc_result const extra_space = ccc_buf_alloc(
            &fpq->buf_, ccc_buf_capacity(&fpq->buf_) * 2, fpq->buf_.alloc_);
        if (extra_space == CCC_OK)
        {
            new = ccc_buf_back(&fpq->buf_);
        }
    }
    if (!new)
    {
        return NULL;
    }
    if (new != e)
    {
        memcpy(new, e, ccc_buf_elem_size(&fpq->buf_));
    }
    size_t const buf_sz = ccc_buf_size(&fpq->buf_);
    size_t i = buf_sz - 1;
    if (buf_sz > 1)
    {
        void *const tmp = ccc_buf_at(&fpq->buf_, ccc_buf_size(&fpq->buf_));
        i = bubble_up(fpq, tmp, i);
    }
    else
    {
        i = 0;
    }
    return ccc_buf_at(&fpq->buf_, i);
}

ccc_result
ccc_fpq_pop(ccc_flat_priority_queue *const fpq)
{
    if (!fpq || ccc_buf_is_empty(&fpq->buf_))
    {
        return CCC_INPUT_ERR;
    }
    if (ccc_buf_size(&fpq->buf_) == 1)
    {
        (void)ccc_buf_pop_back(&fpq->buf_);
        return CCC_OK;
    }
    void *const tmp = ccc_buf_at(&fpq->buf_, ccc_buf_size(&fpq->buf_));
    swap(fpq, tmp, 0, ccc_buf_size(&fpq->buf_) - 1);
    (void)ccc_buf_pop_back(&fpq->buf_);
    bubble_down(fpq, tmp, 0);
    return CCC_OK;
}

ccc_result
ccc_fpq_erase(ccc_flat_priority_queue *const fpq, void *const e)
{
    if (!fpq || !e || ccc_buf_is_empty(&fpq->buf_))
    {
        return CCC_INPUT_ERR;
    }
    if (ccc_buf_size(&fpq->buf_) == 1)
    {
        (void)ccc_buf_pop_back(&fpq->buf_);
        return CCC_OK;
    }
    /* Important to remember this key now to avoid confusion later once the
       elements are swapped and we lose access to original handle index. */
    size_t const swap_location = index_of(fpq, e);
    if (swap_location == ccc_buf_size(&fpq->buf_) - 1)
    {
        (void)ccc_buf_pop_back(&fpq->buf_);
        return CCC_OK;
    }
    void *const tmp = ccc_buf_at(&fpq->buf_, ccc_buf_size(&fpq->buf_));
    swap(fpq, tmp, swap_location, ccc_buf_size(&fpq->buf_) - 1);
    void *const erased = at(fpq, ccc_buf_size(&fpq->buf_) - 1);
    (void)ccc_buf_pop_back(&fpq->buf_);
    ccc_threeway_cmp const erased_cmp
        = fpq->cmp_((ccc_cmp){at(fpq, swap_location), erased, fpq->buf_.aux_});
    if (erased_cmp == fpq->order_)
    {
        (void)bubble_up(fpq, tmp, swap_location);
    }
    else if (erased_cmp != CCC_EQL)
    {
        bubble_down(fpq, tmp, swap_location);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return CCC_OK;
}

bool
ccc_fpq_update(ccc_flat_priority_queue *const fpq, void *const e,
               ccc_update_fn *fn, void *aux)
{
    if (!fpq || !e || !fn || ccc_buf_is_empty(&fpq->buf_))
    {
        return false;
    }
    fn((ccc_user_type){e, aux});
    void *tmp = ccc_buf_at(&fpq->buf_, ccc_buf_size(&fpq->buf_));
    size_t const i = index_of(fpq, e);
    if (!i)
    {
        bubble_down(fpq, tmp, 0);
        return true;
    }
    ccc_threeway_cmp const parent_cmp = fpq->cmp_(
        (ccc_cmp){at(fpq, i), at(fpq, (i - 1) / 2), fpq->buf_.aux_});
    if (parent_cmp == fpq->order_)
    {
        (void)bubble_up(fpq, tmp, i);
        return true;
    }
    if (parent_cmp != CCC_EQL)
    {
        bubble_down(fpq, tmp, i);
        return true;
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return true;
}

/* There are no efficiency benefits in knowing an increase will occur. */
bool
ccc_fpq_increase(ccc_flat_priority_queue *const fpq, void *const e,
                 ccc_update_fn *const fn, void *const aux)
{
    return ccc_fpq_update(fpq, e, fn, aux);
}

/* There are no efficiency benefits in knowing an decrease will occur. */
bool
ccc_fpq_decrease(ccc_flat_priority_queue *const fpq, void *const e,
                 ccc_update_fn *const fn, void *const aux)
{
    return ccc_fpq_update(fpq, e, fn, aux);
}

void *
ccc_fpq_front(ccc_flat_priority_queue const *const fpq)
{
    if (!fpq || ccc_buf_is_empty(&fpq->buf_))
    {
        return NULL;
    }
    return at(fpq, 0);
}

bool
ccc_fpq_is_empty(ccc_flat_priority_queue const *const fpq)
{
    return fpq ? ccc_buf_is_empty(&fpq->buf_) : true;
}

size_t
ccc_fpq_size(ccc_flat_priority_queue const *const fpq)
{
    return fpq ? ccc_buf_size(&fpq->buf_) : 0;
}

ccc_threeway_cmp
ccc_fpq_order(ccc_flat_priority_queue const *const fpq)
{
    return fpq ? fpq->order_ : CCC_CMP_ERR;
}

ccc_result
ccc_fpq_clear(ccc_flat_priority_queue *const fpq, ccc_destructor_fn *const fn)
{
    if (!fpq)
    {
        return CCC_INPUT_ERR;
    }
    if (fn)
    {
        size_t const sz = ccc_buf_size(&fpq->buf_);
        for (size_t i = 0; i < sz; ++i)
        {
            fn((ccc_user_type){.user_type = at(fpq, i), .aux = fpq->buf_.aux_});
        }
    }
    return ccc_buf_size_set(&fpq->buf_, 0);
}

ccc_result
ccc_fpq_clear_and_free(ccc_flat_priority_queue *const fpq,
                       ccc_destructor_fn *const fn)
{
    if (!fpq)
    {
        return CCC_INPUT_ERR;
    }
    if (fn)
    {
        size_t const sz = ccc_buf_size(&fpq->buf_);
        for (size_t i = 0; i < sz; ++i)
        {
            fn((ccc_user_type){.user_type = at(fpq, i), .aux = fpq->buf_.aux_});
        }
    }
    return ccc_buf_alloc(&fpq->buf_, 0, fpq->buf_.alloc_);
}

bool
ccc_fpq_validate(ccc_flat_priority_queue const *const fpq)
{
    if (!fpq)
    {
        return false;
    }
    size_t const sz = ccc_buf_size(&fpq->buf_);
    if (sz <= 1)
    {
        return true;
    }
    for (size_t i = 0, left = (i * 2) + 1, right = (i * 2) + 2;
         i <= (sz - 2) / 2; ++i, left = (i * 2) + 1, right = (i * 2) + 2)
    {
        void *const cur = at(fpq, i);
        /* Putting the child in the comparison function first evaluates
           the childs three way comparison in relation to the parent. If
           the child beats the parent in total ordering (min/max) something
           has gone wrong. */
        if (left < sz && wins(fpq, at(fpq, left), cur))
        {
            return false;
        }
        if (right < sz && wins(fpq, at(fpq, right), cur))
        {
            return false;
        }
    }
    return true;
}

/*===========================   Implementation   ========================== */

size_t
ccc_impl_fpq_bubble_up(struct ccc_fpq_ *const fpq, char tmp[], size_t i)
{
    return bubble_up(fpq, tmp, i);
}

/*===============================  Static Helpers  =========================*/

static inline size_t
bubble_up(struct ccc_fpq_ *const fpq, char tmp[], size_t i)
{
    for (size_t parent = (i - 1) / 2; i; i = parent, parent = (parent - 1) / 2)
    {
        if (!wins(fpq, at(fpq, i), at(fpq, parent)))
        {
            return i;
        }
        swap(fpq, tmp, parent, i);
    }
    return 0;
}

static inline void
bubble_down(struct ccc_fpq_ *const fpq, char tmp[], size_t i)
{
    size_t const sz = ccc_buf_size(&fpq->buf_);
    for (size_t next = i, left = (i * 2) + 1, right = left + 1; left < sz;
         i = next, left = (i * 2) + 1, right = left + 1)
    {
        /* Avoid one comparison call if there is no right child. */
        next = right < sz && wins(fpq, at(fpq, right), at(fpq, left)) ? right
                                                                      : left;
        /* If the child beats the parent we must swap. Equal is ok to break. */
        if (!wins(fpq, at(fpq, next), at(fpq, i)))
        {
            break;
        }
        swap(fpq, tmp, next, i);
    }
}

static inline void
swap(struct ccc_fpq_ *const fpq, char tmp[], size_t const i, size_t const j)
{
    [[maybe_unused]] ccc_result const res = ccc_buf_swap(&fpq->buf_, tmp, i, j);
    assert(res == CCC_OK);
}

/* Thin wrapper just for sanity checking in debug mode as index should always
   be valid when this function is used. */
static inline void *
at(struct ccc_fpq_ const *const fpq, size_t const i)
{
    void *const addr = ccc_buf_at(&fpq->buf_, i);
    assert(addr);
    return addr;
}

/* Flat pqueue code that uses indices of the underlying buffer should always
   be within the buffer range. It should never exceed the current size and
   start at or after the buffer base. Only checked in debug. */
static inline size_t
index_of(struct ccc_fpq_ const *const fpq, void const *const slot)
{
    assert(slot >= ccc_buf_begin(&fpq->buf_));
    size_t const i = (((char *)slot) - ((char *)ccc_buf_begin(&fpq->buf_)))
                     / ccc_buf_elem_size(&fpq->buf_);
    assert(i < ccc_buf_size(&fpq->buf_));
    return i;
}

static inline bool
wins(struct ccc_fpq_ const *const fpq, void const *const winner,
     void const *const loser)
{
    return fpq->cmp_((ccc_cmp){winner, loser, fpq->buf_.aux_}) == fpq->order_;
}
