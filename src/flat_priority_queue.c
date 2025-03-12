#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "buffer.h"
#include "flat_priority_queue.h"
#include "impl/impl_flat_priority_queue.h"
#include "types.h"

static ptrdiff_t const swap_space = 1;

/*=====================      Prototypes      ================================*/

static void *at(struct ccc_fpq_ const *, ptrdiff_t);
static ptrdiff_t index_of(struct ccc_fpq_ const *, void const *);
static ccc_tribool wins(struct ccc_fpq_ const *, void const *winner,
                        void const *loser);
static void swap(struct ccc_fpq_ *, char tmp[], ptrdiff_t, ptrdiff_t);
static ptrdiff_t bubble_up(struct ccc_fpq_ *fpq, char tmp[], ptrdiff_t i);
static ptrdiff_t bubble_down(struct ccc_fpq_ *, char tmp[], ptrdiff_t);
static ptrdiff_t update_fixup(struct ccc_fpq_ *, void *e);
static void inplace_heapify(struct ccc_fpq_ *fpq, ptrdiff_t n);

/*=====================       Interface      ================================*/

ccc_result
ccc_fpq_alloc(ccc_flat_priority_queue *const fpq, ptrdiff_t const new_capacity,
              ccc_alloc_fn *const fn)
{
    if (!fpq || !fn)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    return ccc_buf_alloc(&fpq->buf_, new_capacity, fn);
}

ccc_result
ccc_fpq_heapify(ccc_flat_priority_queue *const fpq, void *const array,
                ptrdiff_t const n, size_t const input_elem_size)
{
    if (!fpq || !array || array == ccc_buf_begin(&fpq->buf_)
        || input_elem_size != ccc_buf_elem_size(&fpq->buf_))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (n + 1 > ccc_buf_capacity(&fpq->buf_))
    {
        ccc_result const resize_res
            = ccc_buf_alloc(&fpq->buf_, n + 1, fpq->buf_.alloc_);
        if (resize_res != CCC_RESULT_OK)
        {
            return resize_res;
        }
    }
    (void)memcpy(ccc_buf_begin(&fpq->buf_), array, n * input_elem_size);
    inplace_heapify(fpq, n);
    return CCC_RESULT_OK;
}

ccc_result
ccc_fpq_heapify_inplace(ccc_flat_priority_queue *fpq, ptrdiff_t n)
{
    if (!fpq || n + 1 > ccc_buf_capacity(&fpq->buf_))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    inplace_heapify(fpq, n);
    return CCC_RESULT_OK;
}

void *
ccc_fpq_push(ccc_flat_priority_queue *const fpq, void const *const e)
{
    if (!fpq || !e)
    {
        return NULL;
    }
    if (ccc_buf_size(&fpq->buf_) + swap_space >= ccc_buf_capacity(&fpq->buf_))
    {
        ccc_result const extra_space = ccc_buf_alloc(
            &fpq->buf_, ccc_buf_capacity(&fpq->buf_) * 2, fpq->buf_.alloc_);
        if (extra_space != CCC_RESULT_OK)
        {
            return NULL;
        }
    }
    void *const new = ccc_buf_alloc_back(&fpq->buf_);
    if (!new)
    {
        return NULL;
    }
    if (new != e)
    {
        (void)memcpy(new, e, ccc_buf_elem_size(&fpq->buf_));
    }
    ptrdiff_t const buf_sz = ccc_buf_size(&fpq->buf_);
    ptrdiff_t i = buf_sz - 1;
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
        return CCC_RESULT_ARG_ERROR;
    }
    if (ccc_buf_size(&fpq->buf_) == 1)
    {
        (void)ccc_buf_pop_back(&fpq->buf_);
        return CCC_RESULT_OK;
    }
    void *const tmp = ccc_buf_at(&fpq->buf_, ccc_buf_size(&fpq->buf_));
    swap(fpq, tmp, 0, ccc_buf_size(&fpq->buf_) - 1);
    (void)ccc_buf_pop_back(&fpq->buf_);
    (void)bubble_down(fpq, tmp, 0);
    return CCC_RESULT_OK;
}

ccc_result
ccc_fpq_erase(ccc_flat_priority_queue *const fpq, void *const e)
{
    if (!fpq || !e || ccc_buf_is_empty(&fpq->buf_))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (ccc_buf_size(&fpq->buf_) == 1)
    {
        (void)ccc_buf_pop_back(&fpq->buf_);
        return CCC_RESULT_OK;
    }
    /* Important to remember this key now to avoid confusion later once the
       elements are swapped and we lose access to original handle index. */
    ptrdiff_t const swap_location = index_of(fpq, e);
    if (swap_location == ccc_buf_size(&fpq->buf_) - 1)
    {
        (void)ccc_buf_pop_back(&fpq->buf_);
        return CCC_RESULT_OK;
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
        (void)bubble_down(fpq, tmp, swap_location);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return CCC_RESULT_OK;
}

void *
ccc_fpq_update(ccc_flat_priority_queue *const fpq, void *const e,
               ccc_update_fn *const fn, void *const aux)
{
    if (!fpq || !e || !fn || ccc_buf_is_empty(&fpq->buf_))
    {
        return NULL;
    }
    fn((ccc_user_type){e, aux});
    return ccc_buf_at(&fpq->buf_, update_fixup(fpq, e));
}

/* There are no efficiency benefits in knowing an increase will occur. */
void *
ccc_fpq_increase(ccc_flat_priority_queue *const fpq, void *const e,
                 ccc_update_fn *const fn, void *const aux)
{
    return ccc_fpq_update(fpq, e, fn, aux);
}

/* There are no efficiency benefits in knowing an decrease will occur. */
void *
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

ptrdiff_t
ccc_fpq_i(ccc_flat_priority_queue const *const fpq, void const *const e)
{
    if (!fpq || !e || e < ccc_buf_begin(&fpq->buf_)
        || e >= ccc_buf_end(&fpq->buf_))
    {
        return -1;
    }
    return ccc_buf_i(&fpq->buf_, e);
}

ccc_tribool
ccc_fpq_is_empty(ccc_flat_priority_queue const *const fpq)
{
    if (!fpq)
    {
        return CCC_BOOL_ERR;
    }
    return ccc_buf_is_empty(&fpq->buf_);
}

ptrdiff_t
ccc_fpq_size(ccc_flat_priority_queue const *const fpq)
{
    return fpq ? ccc_buf_size(&fpq->buf_) : 0;
}

ptrdiff_t
ccc_fpq_capacity(ccc_flat_priority_queue const *const fpq)
{
    return fpq ? ccc_buf_capacity(&fpq->buf_) : 0;
}

void *
ccc_fpq_data(ccc_flat_priority_queue const *const fpq)
{
    return fpq ? ccc_buf_begin(&fpq->buf_) : NULL;
}

ccc_threeway_cmp
ccc_fpq_order(ccc_flat_priority_queue const *const fpq)
{
    return fpq ? fpq->order_ : CCC_CMP_ERR;
}

ccc_result
ccc_fpq_copy(ccc_flat_priority_queue *const dst,
             ccc_flat_priority_queue const *const src, ccc_alloc_fn *const fn)
{
    if (!dst || !src || src == dst
        || (dst->buf_.capacity_ < src->buf_.capacity_ && !fn))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Copy everything so we don't worry about staying in sync with future
       changes to buf container. But we have to give back original destination
       memory in case it has already been allocated. Alloc will remain the
       same as in dst initialization because that controls permission. */
    void *const dst_mem = dst->buf_.mem_;
    ptrdiff_t const dst_cap = dst->buf_.capacity_;
    ccc_alloc_fn *const dst_alloc = dst->buf_.alloc_;
    *dst = *src;
    dst->buf_.mem_ = dst_mem;
    dst->buf_.capacity_ = dst_cap;
    dst->buf_.alloc_ = dst_alloc;
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
    (void)memcpy(dst->buf_.mem_, src->buf_.mem_,
                 src->buf_.capacity_ * src->buf_.elem_sz_);
    return CCC_RESULT_OK;
}

ccc_result
ccc_fpq_clear(ccc_flat_priority_queue *const fpq, ccc_destructor_fn *const fn)
{
    if (!fpq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (fn)
    {
        ptrdiff_t const sz = ccc_buf_size(&fpq->buf_);
        for (ptrdiff_t i = 0; i < sz; ++i)
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
        return CCC_RESULT_ARG_ERROR;
    }
    if (fn)
    {
        ptrdiff_t const sz = ccc_buf_size(&fpq->buf_);
        for (ptrdiff_t i = 0; i < sz; ++i)
        {
            fn((ccc_user_type){.user_type = at(fpq, i), .aux = fpq->buf_.aux_});
        }
    }
    return ccc_buf_alloc(&fpq->buf_, 0, fpq->buf_.alloc_);
}

ccc_tribool
ccc_fpq_validate(ccc_flat_priority_queue const *const fpq)
{
    if (!fpq)
    {
        return CCC_BOOL_ERR;
    }
    ptrdiff_t const sz = ccc_buf_size(&fpq->buf_);
    if (sz <= 1)
    {
        return CCC_TRUE;
    }
    for (ptrdiff_t i = 0, left = (i * 2) + 1, right = (i * 2) + 2;
         i <= (sz - 2) / 2; ++i, left = (i * 2) + 1, right = (i * 2) + 2)
    {
        void *const cur = at(fpq, i);
        /* Putting the child in the comparison function first evaluates
           the child's three way comparison in relation to the parent. If
           the child beats the parent in total ordering (min/max) something
           has gone wrong. */
        if (left < sz && wins(fpq, at(fpq, left), cur))
        {
            return CCC_FALSE;
        }
        if (right < sz && wins(fpq, at(fpq, right), cur))
        {
            return CCC_FALSE;
        }
    }
    return CCC_TRUE;
}

/*===================     Private Interface     =============================*/

ptrdiff_t
ccc_impl_fpq_bubble_up(struct ccc_fpq_ *const fpq, char tmp[], ptrdiff_t i)
{
    return bubble_up(fpq, tmp, i);
}

void *
ccc_impl_fpq_update_fixup(struct ccc_fpq_ *const fpq, void *const e)
{
    return ccc_buf_at(&fpq->buf_, update_fixup(fpq, e));
}

void
ccc_impl_fpq_in_place_heapify(struct ccc_fpq_ *const fpq, ptrdiff_t const n)
{
    if (!fpq || ccc_buf_capacity(&fpq->buf_) < n + 1)
    {
        return;
    }
    inplace_heapify(fpq, n);
}

/*====================     Static Helpers     ===============================*/

static inline void
inplace_heapify(struct ccc_fpq_ *const fpq, ptrdiff_t const n)
{
    (void)ccc_buf_size_set(&fpq->buf_, n);
    void *const tmp = ccc_buf_at(&fpq->buf_, n);
    for (ptrdiff_t i = (n / 2) + 1; i--;)
    {
        (void)bubble_down(fpq, tmp, i);
    }
}

/* Fixes the position of element e after its key value has been changed. */
static inline ptrdiff_t
update_fixup(struct ccc_fpq_ *const fpq, void *const e)
{
    void *const tmp = ccc_buf_at(&fpq->buf_, ccc_buf_size(&fpq->buf_));
    ptrdiff_t const i = index_of(fpq, e);
    if (!i)
    {
        return bubble_down(fpq, tmp, 0);
    }
    ccc_threeway_cmp const parent_cmp = fpq->cmp_(
        (ccc_cmp){at(fpq, i), at(fpq, (i - 1) / 2), fpq->buf_.aux_});
    if (parent_cmp == fpq->order_)
    {
        return bubble_up(fpq, tmp, i);
    }
    if (parent_cmp != CCC_EQL)
    {
        return bubble_down(fpq, tmp, i);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return i;
}

/* Returns the sorted position of the element starting at position i. */
static inline ptrdiff_t
bubble_up(struct ccc_fpq_ *const fpq, char tmp[const], ptrdiff_t i)
{
    for (ptrdiff_t parent = (i - 1) / 2; i;
         i = parent, parent = (parent - 1) / 2)
    {
        /* Not winning here means we are in correct order or equal. */
        if (!wins(fpq, at(fpq, i), at(fpq, parent)))
        {
            return i;
        }
        swap(fpq, tmp, parent, i);
    }
    return 0;
}

/* Returns the sorted position of the element starting at position i. */
static inline ptrdiff_t
bubble_down(struct ccc_fpq_ *const fpq, char tmp[const], ptrdiff_t i)
{
    ptrdiff_t const sz = ccc_buf_size(&fpq->buf_);
    for (ptrdiff_t next = i, left = (i * 2) + 1, right = left + 1; left < sz;
         i = next, left = (i * 2) + 1, right = left + 1)
    {
        /* Avoid one comparison call if there is no right child. */
        next = (right < sz && wins(fpq, at(fpq, right), at(fpq, left))) ? right
                                                                        : left;
        /* If the child beats the parent we must swap. Equal is ok to break. */
        if (!wins(fpq, at(fpq, next), at(fpq, i)))
        {
            return i;
        }
        swap(fpq, tmp, next, i);
    }
    return i;
}

/* Swaps i and j using the underlying buffer capabilities. Not checked for
   an error in release. */
static inline void
swap(struct ccc_fpq_ *const fpq, char tmp[const], ptrdiff_t const i,
     ptrdiff_t const j)
{
    [[maybe_unused]] ccc_result const res = ccc_buf_swap(&fpq->buf_, tmp, i, j);
    assert(res == CCC_RESULT_OK);
}

/* Thin wrapper just for sanity checking in debug mode as index should always
   be valid when this function is used. */
static inline void *
at(struct ccc_fpq_ const *const fpq, ptrdiff_t const i)
{
    void *const addr = ccc_buf_at(&fpq->buf_, i);
    assert(addr);
    return addr;
}

/* Flat pq code that uses indices of the underlying buffer should always
   be within the buffer range. It should never exceed the current size and
   start at or after the buffer base. Only checked in debug. */
static inline ptrdiff_t
index_of(struct ccc_fpq_ const *const fpq, void const *const slot)
{
    assert(slot >= ccc_buf_begin(&fpq->buf_));
    ptrdiff_t const i
        = (ptrdiff_t)((((char *)slot) - ((char *)ccc_buf_begin(&fpq->buf_)))
                      / ccc_buf_elem_size(&fpq->buf_));
    assert(i < ccc_buf_size(&fpq->buf_));
    return i;
}

/* Returns true if the winner (the "left hand side") wins the comparison.
   Winning in a three-way comparison means satisfying the total order of the
   priority queue. So, there is no winner if the elements are equal and this
   function would return false. If the winner is in the wrong order, thus
   losing the total order comparison, the function also returns false. */
static inline ccc_tribool
wins(struct ccc_fpq_ const *const fpq, void const *const winner,
     void const *const loser)
{
    return fpq->cmp_((ccc_cmp){winner, loser, fpq->buf_.aux_}) == fpq->order_;
}
