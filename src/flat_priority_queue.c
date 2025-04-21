/** Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "flat_priority_queue.h"
#include "impl/impl_flat_priority_queue.h"
#include "types.h"

enum : size_t
{
    SWAP_SPACE = 1,
};

/*=====================      Prototypes      ================================*/

static void *at(struct ccc_fpq const *, size_t);
static size_t index_of(struct ccc_fpq const *, void const *);
static ccc_tribool wins(struct ccc_fpq const *, size_t winner, size_t loser);
static ccc_threeway_cmp cmp(struct ccc_fpq const *, size_t lhs, size_t rhs);
static void swap(struct ccc_fpq *, char tmp[const], size_t, size_t);
static size_t bubble_up(struct ccc_fpq *fpq, char tmp[const], size_t i);
static size_t bubble_down(struct ccc_fpq *, char tmp[const], size_t);
static size_t update_fixup(struct ccc_fpq *, void *e);
static void inplace_heapify(struct ccc_fpq *fpq, size_t n);

/*=====================       Interface      ================================*/

ccc_result
ccc_fpq_alloc(ccc_flat_priority_queue *const fpq, size_t const new_capacity,
              ccc_any_alloc_fn *const fn)
{
    if (!fpq || !fn)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    return ccc_buf_alloc(&fpq->buf, new_capacity, fn);
}

ccc_result
ccc_fpq_heapify(ccc_flat_priority_queue *const fpq, void *const array,
                size_t const n, size_t const input_sizeof_type)
{
    if (!fpq || !array || array == ccc_buf_begin(&fpq->buf)
        || input_sizeof_type != fpq->buf.sizeof_type)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (n + 1 > fpq->buf.capacity)
    {
        ccc_result const resize_res
            = ccc_buf_alloc(&fpq->buf, n + 1, fpq->buf.alloc);
        if (resize_res != CCC_RESULT_OK)
        {
            return resize_res;
        }
    }
    (void)memcpy(ccc_buf_begin(&fpq->buf), array, n * input_sizeof_type);
    inplace_heapify(fpq, n);
    return CCC_RESULT_OK;
}

ccc_result
ccc_fpq_heapify_inplace(ccc_flat_priority_queue *fpq, size_t const n)
{
    if (!fpq || n + 1 > fpq->buf.capacity)
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
    if (fpq->buf.count + SWAP_SPACE >= fpq->buf.capacity)
    {
        ccc_result const extra_space
            = ccc_buf_alloc(&fpq->buf, fpq->buf.capacity * 2, fpq->buf.alloc);
        if (extra_space != CCC_RESULT_OK)
        {
            return NULL;
        }
    }
    void *const new = ccc_buf_alloc_back(&fpq->buf);
    if (!new)
    {
        return NULL;
    }
    if (new != e)
    {
        (void)memcpy(new, e, fpq->buf.sizeof_type);
    }
    size_t const buf_count = fpq->buf.count;
    size_t i = buf_count - 1;
    if (buf_count > 1)
    {
        void *const tmp = ccc_buf_at(&fpq->buf, fpq->buf.count);
        i = bubble_up(fpq, tmp, i);
    }
    else
    {
        i = 0;
    }
    return ccc_buf_at(&fpq->buf, i);
}

ccc_result
ccc_fpq_pop(ccc_flat_priority_queue *const fpq)
{
    if (!fpq || ccc_buf_is_empty(&fpq->buf))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (fpq->buf.count == 1)
    {
        (void)ccc_buf_pop_back(&fpq->buf);
        return CCC_RESULT_OK;
    }
    void *const tmp = ccc_buf_at(&fpq->buf, fpq->buf.count);
    swap(fpq, tmp, 0, fpq->buf.count - 1);
    (void)ccc_buf_pop_back(&fpq->buf);
    (void)bubble_down(fpq, tmp, 0);
    return CCC_RESULT_OK;
}

ccc_result
ccc_fpq_erase(ccc_flat_priority_queue *const fpq, void *const e)
{
    if (!fpq || !e || ccc_buf_is_empty(&fpq->buf))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (fpq->buf.count == 1)
    {
        (void)ccc_buf_pop_back(&fpq->buf);
        return CCC_RESULT_OK;
    }
    /* Important to remember this key now to avoid confusion later once the
       elements are swapped and we lose access to original handle index. */
    size_t const i = index_of(fpq, e);
    if (i == fpq->buf.count - 1)
    {
        (void)ccc_buf_pop_back(&fpq->buf);
        return CCC_RESULT_OK;
    }
    void *const tmp = ccc_buf_at(&fpq->buf, fpq->buf.count);
    swap(fpq, tmp, i, fpq->buf.count - 1);
    ccc_threeway_cmp const cmp_res = cmp(fpq, i, fpq->buf.count - 1);
    (void)ccc_buf_pop_back(&fpq->buf);
    if (cmp_res == fpq->order)
    {
        (void)bubble_up(fpq, tmp, i);
    }
    else if (cmp_res != CCC_EQL)
    {
        (void)bubble_down(fpq, tmp, i);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return CCC_RESULT_OK;
}

void *
ccc_fpq_update(ccc_flat_priority_queue *const fpq, void *const e,
               ccc_any_type_update_fn *const fn, void *const aux)
{
    if (!fpq || !e || !fn || ccc_buf_is_empty(&fpq->buf))
    {
        return NULL;
    }
    fn((ccc_any_type){
        .any_type = e,
        .aux = aux,
    });
    return ccc_buf_at(&fpq->buf, update_fixup(fpq, e));
}

/* There are no efficiency benefits in knowing an increase will occur. */
void *
ccc_fpq_increase(ccc_flat_priority_queue *const fpq, void *const e,
                 ccc_any_type_update_fn *const fn, void *const aux)
{
    return ccc_fpq_update(fpq, e, fn, aux);
}

/* There are no efficiency benefits in knowing an decrease will occur. */
void *
ccc_fpq_decrease(ccc_flat_priority_queue *const fpq, void *const e,
                 ccc_any_type_update_fn *const fn, void *const aux)
{
    return ccc_fpq_update(fpq, e, fn, aux);
}

void *
ccc_fpq_front(ccc_flat_priority_queue const *const fpq)
{
    if (!fpq || ccc_buf_is_empty(&fpq->buf))
    {
        return NULL;
    }
    return at(fpq, 0);
}

ccc_tribool
ccc_fpq_is_empty(ccc_flat_priority_queue const *const fpq)
{
    if (!fpq)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return ccc_buf_is_empty(&fpq->buf);
}

ccc_ucount
ccc_fpq_size(ccc_flat_priority_queue const *const fpq)
{
    if (!fpq)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return ccc_buf_size(&fpq->buf);
}

ccc_ucount
ccc_fpq_capacity(ccc_flat_priority_queue const *const fpq)
{
    if (!fpq)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return ccc_buf_capacity(&fpq->buf);
}

void *
ccc_fpq_data(ccc_flat_priority_queue const *const fpq)
{
    return fpq ? ccc_buf_begin(&fpq->buf) : NULL;
}

ccc_threeway_cmp
ccc_fpq_order(ccc_flat_priority_queue const *const fpq)
{
    return fpq ? fpq->order : CCC_CMP_ERROR;
}

ccc_result
ccc_fpq_reserve(ccc_flat_priority_queue *const fpq, size_t const to_add,
                ccc_any_alloc_fn *const fn)
{
    if (!fpq || !fn)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    size_t const needed = fpq->buf.count + to_add + 1;
    if (needed <= fpq->buf.capacity)
    {
        return CCC_RESULT_OK;
    }
    return ccc_buf_alloc(&fpq->buf, needed, fn);
}

ccc_result
ccc_fpq_copy(ccc_flat_priority_queue *const dst,
             ccc_flat_priority_queue const *const src,
             ccc_any_alloc_fn *const fn)
{
    if (!dst || !src || src == dst
        || (dst->buf.capacity < src->buf.capacity && !fn))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Copy everything so we don't worry about staying in sync with future
       changes to buf container. But we have to give back original destination
       memory in case it has already been allocated. Alloc will remain the
       same as in dst initialization because that controls permission. */
    void *const dst_mem = dst->buf.mem;
    size_t const dst_cap = dst->buf.capacity;
    ccc_any_alloc_fn *const dst_alloc = dst->buf.alloc;
    *dst = *src;
    dst->buf.mem = dst_mem;
    dst->buf.capacity = dst_cap;
    dst->buf.alloc = dst_alloc;
    if (!src->buf.capacity)
    {
        return CCC_RESULT_OK;
    }
    if (dst->buf.capacity < src->buf.capacity)
    {
        ccc_result resize_res = ccc_buf_alloc(&dst->buf, src->buf.capacity, fn);
        if (resize_res != CCC_RESULT_OK)
        {
            return resize_res;
        }
        dst->buf.capacity = src->buf.capacity;
    }
    if (!src->buf.mem || !dst->buf.mem)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    (void)memcpy(dst->buf.mem, src->buf.mem,
                 src->buf.capacity * src->buf.sizeof_type);
    return CCC_RESULT_OK;
}

ccc_result
ccc_fpq_clear(ccc_flat_priority_queue *const fpq,
              ccc_any_type_destructor_fn *const fn)
{
    if (!fpq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (fn)
    {
        size_t const count = fpq->buf.count;
        for (size_t i = 0; i < count; ++i)
        {
            fn((ccc_any_type){
                .any_type = at(fpq, i),
                .aux = fpq->buf.aux,
            });
        }
    }
    return ccc_buf_size_set(&fpq->buf, 0);
}

ccc_result
ccc_fpq_clear_and_free(ccc_flat_priority_queue *const fpq,
                       ccc_any_type_destructor_fn *const fn)
{
    if (!fpq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (fn)
    {
        size_t const count = fpq->buf.count;
        for (size_t i = 0; i < count; ++i)
        {
            fn((ccc_any_type){
                .any_type = at(fpq, i),
                .aux = fpq->buf.aux,
            });
        }
    }
    return ccc_buf_alloc(&fpq->buf, 0, fpq->buf.alloc);
}

ccc_result
ccc_fpq_clear_and_free_reserve(ccc_flat_priority_queue *const fpq,
                               ccc_any_type_destructor_fn *const destructor,
                               ccc_any_alloc_fn *const alloc)
{
    if (!fpq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (destructor)
    {
        size_t const count = fpq->buf.count;
        for (size_t i = 0; i < count; ++i)
        {
            destructor((ccc_any_type){
                .any_type = at(fpq, i),
                .aux = fpq->buf.aux,
            });
        }
    }
    return ccc_buf_alloc(&fpq->buf, 0, alloc);
}

ccc_tribool
ccc_fpq_validate(ccc_flat_priority_queue const *const fpq)
{
    if (!fpq)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const count = fpq->buf.count;
    if (count <= 1)
    {
        return CCC_TRUE;
    }
    for (size_t i = 0, left = (i * 2) + 1, right = (i * 2) + 2;
         i <= (count - 2) / 2; ++i, left = (i * 2) + 1, right = (i * 2) + 2)
    {
        /* Putting the child in the comparison function first evaluates
           the child's three way comparison in relation to the parent. If
           the child beats the parent in total ordering (min/max) something
           has gone wrong. */
        if (left < count && wins(fpq, left, i))
        {
            return CCC_FALSE;
        }
        if (right < count && wins(fpq, right, i))
        {
            return CCC_FALSE;
        }
    }
    return CCC_TRUE;
}

/*===================     Private Interface     =============================*/

size_t
ccc_impl_fpq_bubble_up(struct ccc_fpq *const fpq, char tmp[const], size_t i)
{
    return bubble_up(fpq, tmp, i);
}

void *
ccc_impl_fpq_update_fixup(struct ccc_fpq *const fpq, void *const e)
{
    return ccc_buf_at(&fpq->buf, update_fixup(fpq, e));
}

void
ccc_impl_fpq_in_place_heapify(struct ccc_fpq *const fpq, size_t const n)
{
    if (!fpq || fpq->buf.capacity < n + 1)
    {
        return;
    }
    inplace_heapify(fpq, n);
}

/*====================     Static Helpers     ===============================*/

static void
inplace_heapify(struct ccc_fpq *const fpq, size_t const n)
{
    (void)ccc_buf_size_set(&fpq->buf, n);
    void *const tmp = ccc_buf_at(&fpq->buf, n);
    for (size_t i = (n / 2) + 1; i--;)
    {
        (void)bubble_down(fpq, tmp, i);
    }
}

/* Fixes the position of element e after its key value has been changed. */
static size_t
update_fixup(struct ccc_fpq *const fpq, void *const e)
{
    void *const tmp = ccc_buf_at(&fpq->buf, fpq->buf.count);
    size_t const i = index_of(fpq, e);
    if (!i)
    {
        return bubble_down(fpq, tmp, 0);
    }
    ccc_threeway_cmp const parent_cmp = cmp(fpq, i, (i - 1) / 2);
    if (parent_cmp == fpq->order)
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
static size_t
bubble_up(struct ccc_fpq *const fpq, char tmp[const], size_t i)
{
    for (size_t parent = (i - 1) / 2; i; i = parent, parent = (parent - 1) / 2)
    {
        /* Not winning here means we are in correct order or equal. */
        if (!wins(fpq, i, parent))
        {
            return i;
        }
        swap(fpq, tmp, parent, i);
    }
    return 0;
}

/* Returns the sorted position of the element starting at position i. */
static size_t
bubble_down(struct ccc_fpq *const fpq, char tmp[const], size_t i)
{
    size_t const count = fpq->buf.count;
    for (size_t next = i, left = (i * 2) + 1, right = left + 1; left < count;
         i = next, left = (i * 2) + 1, right = left + 1)
    {
        /* Avoid one comparison call if there is no right child. */
        next = (right < count && wins(fpq, right, left)) ? right : left;
        /* If the child beats the parent we must swap. Equal is OK to break. */
        if (!wins(fpq, next, i))
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
swap(struct ccc_fpq *const fpq, char tmp[const], size_t const i, size_t const j)
{
    [[maybe_unused]] ccc_result const res = ccc_buf_swap(&fpq->buf, tmp, i, j);
    assert(res == CCC_RESULT_OK);
}

/* Thin wrapper just for sanity checking in debug mode as index should always
   be valid when this function is used. */
static inline void *
at(struct ccc_fpq const *const fpq, size_t const i)
{
    void *const addr = ccc_buf_at(&fpq->buf, i);
    assert(addr);
    return addr;
}

/* Flat priority queue code that uses indices of the underlying buffer should
   always be within the buffer range. It should never exceed the current size
   and start at or after the buffer base. Only checked in debug. */
static inline size_t
index_of(struct ccc_fpq const *const fpq, void const *const slot)
{
    assert(slot >= ccc_buf_begin(&fpq->buf));
    size_t const i = ((((char *)slot) - ((char *)ccc_buf_begin(&fpq->buf)))
                      / fpq->buf.sizeof_type);
    assert(i < fpq->buf.count);
    return i;
}

/* Returns true if the winner (the "left hand side") wins the comparison.
   Winning in a three-way comparison means satisfying the total order of the
   priority queue. So, there is no winner if the elements are equal and this
   function would return false. If the winner is in the wrong order, thus
   losing the total order comparison, the function also returns false. */
static inline ccc_tribool
wins(struct ccc_fpq const *const fpq, size_t const winner, size_t const loser)
{
    return fpq->cmp((ccc_any_type_cmp){
               .any_type_lhs = at(fpq, winner),
               .any_type_rhs = at(fpq, loser),
               .aux = fpq->buf.aux,
           })
           == fpq->order;
}

static inline ccc_threeway_cmp
cmp(struct ccc_fpq const *const fpq, size_t lhs, size_t rhs)
{
    return fpq->cmp((ccc_any_type_cmp){
        .any_type_lhs = at(fpq, lhs),
        .any_type_rhs = at(fpq, rhs),
        .aux = fpq->buf.aux,
    });
}
