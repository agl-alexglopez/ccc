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
    START_CAP = 8,
};

/*=====================      Prototypes      ================================*/

static void *at(struct CCC_fpq const *, size_t);
static size_t index_of(struct CCC_fpq const *, void const *);
static CCC_tribool wins(struct CCC_fpq const *, size_t winner, size_t loser);
static CCC_threeway_cmp cmp(struct CCC_fpq const *, size_t lhs, size_t rhs);
static void swap(struct CCC_fpq *, void *tmp, size_t, size_t);
static size_t bubble_up(struct CCC_fpq *fpq, void *tmp, size_t i);
static size_t bubble_down(struct CCC_fpq *, void *tmp, size_t, size_t);
static size_t update_fixup(struct CCC_fpq *, void *e, void *tmp);
static void heapify(struct CCC_fpq *fpq, size_t n, void *);
static void destroy_each(struct CCC_fpq *fpq, CCC_any_type_destructor_fn *fn);

/*=====================       Interface      ================================*/

CCC_result
CCC_fpq_heapify(CCC_flat_priority_queue *const fpq, void *const tmp,
                void *const array, size_t const n,
                size_t const input_sizeof_type)
{
    if (!fpq || !array || !tmp || array == fpq->buf.mem
        || input_sizeof_type != fpq->buf.sizeof_type)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (n > fpq->buf.capacity)
    {
        CCC_result const resize_res
            = CCC_buf_alloc(&fpq->buf, n, fpq->buf.alloc);
        if (resize_res != CCC_RESULT_OK)
        {
            return resize_res;
        }
    }
    (void)memcpy(fpq->buf.mem, array, n * input_sizeof_type);
    heapify(fpq, n, tmp);
    return CCC_RESULT_OK;
}

CCC_result
CCC_fpq_heapify_inplace(CCC_flat_priority_queue *fpq, void *const tmp,
                        size_t const n)
{
    if (!fpq || !tmp || n > fpq->buf.capacity)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    heapify(fpq, n, tmp);
    return CCC_RESULT_OK;
}

CCC_buffer
CCC_fpq_heapsort(CCC_flat_priority_queue *const fpq, void *const tmp)
{
    CCC_buffer ret = {};
    if (!fpq || !tmp)
    {
        return ret;
    }
    ret = fpq->buf;
    if (fpq->buf.count > 1)
    {
        size_t end = fpq->buf.count;
        while (--end)
        {
            swap(fpq, tmp, 0, end);
            (void)bubble_down(fpq, tmp, 0, end);
        }
    }
    *fpq = (CCC_flat_priority_queue){};
    return ret;
}

void *
CCC_fpq_push(CCC_flat_priority_queue *const fpq, void const *const elem,
             void *const tmp)
{
    if (!fpq || !elem || !tmp)
    {
        return NULL;
    }
    void *const new = CCC_buf_alloc_back(&fpq->buf);
    if (!new)
    {
        return NULL;
    }
    if (new != elem)
    {
        (void)memcpy(new, elem, fpq->buf.sizeof_type);
    }
    assert(tmp);
    size_t const i = bubble_up(fpq, tmp, fpq->buf.count - 1);
    assert(i < fpq->buf.count);
    return CCC_buf_at(&fpq->buf, i);
}

CCC_result
CCC_fpq_pop(CCC_flat_priority_queue *const fpq, void *const tmp)
{
    if (!fpq || !tmp || !fpq->buf.count)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    --fpq->buf.count;
    if (!fpq->buf.count)
    {
        return CCC_RESULT_OK;
    }
    swap(fpq, tmp, 0, fpq->buf.count);
    (void)bubble_down(fpq, tmp, 0, fpq->buf.count);
    return CCC_RESULT_OK;
}

CCC_result
CCC_fpq_erase(CCC_flat_priority_queue *const fpq, void *const elem,
              void *const tmp)
{
    if (!fpq || !elem || !tmp || !fpq->buf.count)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    size_t const i = index_of(fpq, elem);
    --fpq->buf.count;
    if (i == fpq->buf.count)
    {
        return CCC_RESULT_OK;
    }
    swap(fpq, tmp, i, fpq->buf.count);
    CCC_threeway_cmp const cmp_res = cmp(fpq, i, fpq->buf.count);
    if (cmp_res == fpq->order)
    {
        (void)bubble_up(fpq, tmp, i);
    }
    else if (cmp_res != CCC_EQL)
    {
        (void)bubble_down(fpq, tmp, i, fpq->buf.count);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return CCC_RESULT_OK;
}

void *
CCC_fpq_update(CCC_flat_priority_queue *const fpq, void *const elem,
               void *const tmp, CCC_any_type_update_fn *const fn,
               void *const aux)
{
    if (!fpq || !elem || !tmp || !fn || !fpq->buf.count)
    {
        return NULL;
    }
    fn((CCC_any_type){
        .any_type = elem,
        .aux = aux,
    });
    return CCC_buf_at(&fpq->buf, update_fixup(fpq, elem, tmp));
}

/* There are no efficiency benefits in knowing an increase will occur. */
void *
CCC_fpq_increase(CCC_flat_priority_queue *const fpq, void *const elem,
                 void *const tmp, CCC_any_type_update_fn *const fn,
                 void *const aux)
{
    return CCC_fpq_update(fpq, elem, tmp, fn, aux);
}

/* There are no efficiency benefits in knowing an decrease will occur. */
void *
CCC_fpq_decrease(CCC_flat_priority_queue *const fpq, void *const elem,
                 void *const tmp, CCC_any_type_update_fn *const fn,
                 void *const aux)
{
    return CCC_fpq_update(fpq, elem, tmp, fn, aux);
}

void *
CCC_fpq_front(CCC_flat_priority_queue const *const fpq)
{
    if (!fpq || !fpq->buf.count)
    {
        return NULL;
    }
    return at(fpq, 0);
}

CCC_tribool
CCC_fpq_is_empty(CCC_flat_priority_queue const *const fpq)
{
    if (!fpq)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return CCC_buf_is_empty(&fpq->buf);
}

CCC_ucount
CCC_fpq_count(CCC_flat_priority_queue const *const fpq)
{
    if (!fpq)
    {
        return (CCC_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return CCC_buf_count(&fpq->buf);
}

CCC_ucount
CCC_fpq_capacity(CCC_flat_priority_queue const *const fpq)
{
    if (!fpq)
    {
        return (CCC_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return CCC_buf_capacity(&fpq->buf);
}

void *
CCC_fpq_data(CCC_flat_priority_queue const *const fpq)
{
    return fpq ? CCC_buf_begin(&fpq->buf) : NULL;
}

CCC_threeway_cmp
CCC_fpq_order(CCC_flat_priority_queue const *const fpq)
{
    return fpq ? fpq->order : CCC_CMP_ERROR;
}

CCC_result
CCC_fpq_reserve(CCC_flat_priority_queue *const fpq, size_t const to_add,
                CCC_any_alloc_fn *const fn)
{
    if (!fpq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    return CCC_buf_reserve(&fpq->buf, to_add, fn);
}

CCC_result
CCC_fpq_copy(CCC_flat_priority_queue *const dst,
             CCC_flat_priority_queue const *const src,
             CCC_any_alloc_fn *const fn)
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
    CCC_any_alloc_fn *const dst_alloc = dst->buf.alloc;
    *dst = *src;
    dst->buf.mem = dst_mem;
    dst->buf.capacity = dst_cap;
    dst->buf.alloc = dst_alloc;
    if (!src->buf.count)
    {
        return CCC_RESULT_OK;
    }
    if (dst->buf.capacity < src->buf.capacity)
    {
        CCC_result const r = CCC_buf_alloc(&dst->buf, src->buf.capacity, fn);
        if (r != CCC_RESULT_OK)
        {
            return r;
        }
        dst->buf.capacity = src->buf.capacity;
    }
    if (!src->buf.mem || !dst->buf.mem)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* It is ok to only copy count elements because we know that all elements
       in a binary heap are contiguous from [0, C), where C is count. */
    (void)memcpy(dst->buf.mem, src->buf.mem,
                 src->buf.count * src->buf.sizeof_type);
    return CCC_RESULT_OK;
}

CCC_result
CCC_fpq_clear(CCC_flat_priority_queue *const fpq,
              CCC_any_type_destructor_fn *const fn)
{
    if (!fpq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (fn)
    {
        destroy_each(fpq, fn);
    }
    return CCC_buf_size_set(&fpq->buf, 0);
}

CCC_result
CCC_fpq_clear_and_free(CCC_flat_priority_queue *const fpq,
                       CCC_any_type_destructor_fn *const fn)
{
    if (!fpq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (fn)
    {
        destroy_each(fpq, fn);
    }
    return CCC_buf_alloc(&fpq->buf, 0, fpq->buf.alloc);
}

CCC_result
CCC_fpq_clear_and_free_reserve(CCC_flat_priority_queue *const fpq,
                               CCC_any_type_destructor_fn *const destructor,
                               CCC_any_alloc_fn *const alloc)
{
    if (!fpq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (destructor)
    {
        destroy_each(fpq, destructor);
    }
    return CCC_buf_alloc(&fpq->buf, 0, alloc);
}

CCC_tribool
CCC_fpq_validate(CCC_flat_priority_queue const *const fpq)
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
    for (size_t i = 0, left = (i * 2) + 1, right = (i * 2) + 2,
                end = (count - 2) / 2;
         i <= end; ++i, left = (i * 2) + 1, right = (i * 2) + 2)
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
CCC_impl_fpq_bubble_up(struct CCC_fpq *const fpq, void *const tmp, size_t i)
{
    return bubble_up(fpq, tmp, i);
}

void *
CCC_impl_fpq_update_fixup(struct CCC_fpq *const fpq, void *const elem,
                          void *const tmp)
{
    return CCC_buf_at(&fpq->buf, update_fixup(fpq, elem, tmp));
}

void
CCC_impl_fpq_in_place_heapify(struct CCC_fpq *const fpq, size_t const n,
                              void *const tmp)
{
    if (!fpq || fpq->buf.capacity < n)
    {
        return;
    }
    heapify(fpq, n, tmp);
}

/*====================     Static Helpers     ===============================*/

/* Orders the heap in O(N) time. Assumes n > 0 and n <= capacity. */
static void
heapify(struct CCC_fpq *const fpq, size_t const n, void *const tmp)
{
    assert(n);
    assert(n <= fpq->buf.capacity);
    fpq->buf.count = n;
    size_t i = ((n - 1) / 2) + 1;
    while (i--)
    {
        (void)bubble_down(fpq, tmp, i, fpq->buf.count);
    }
}

/* Fixes the position of element e after its key value has been changed. */
static size_t
update_fixup(struct CCC_fpq *const fpq, void *const e, void *const tmp)
{
    size_t const i = index_of(fpq, e);
    if (!i)
    {
        return bubble_down(fpq, tmp, 0, fpq->buf.count);
    }
    CCC_threeway_cmp const parent_cmp = cmp(fpq, i, (i - 1) / 2);
    if (parent_cmp == fpq->order)
    {
        return bubble_up(fpq, tmp, i);
    }
    if (parent_cmp != CCC_EQL)
    {
        return bubble_down(fpq, tmp, i, fpq->buf.count);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return i;
}

/* Returns the sorted position of the element starting at position i. */
static size_t
bubble_up(struct CCC_fpq *const fpq, void *const tmp, size_t i)
{
    for (size_t parent = (i - 1) / 2; i; i = parent, parent = (parent - 1) / 2)
    {
        /* Not winning here means we are in correct order or equal. */
        if (!wins(fpq, i, parent))
        {
            return i;
        }
        swap(fpq, tmp, i, parent);
    }
    return 0;
}

/* Returns the sorted position of the element starting at position i. */
static size_t
bubble_down(struct CCC_fpq *const fpq, void *const tmp, size_t i,
            size_t const count)
{
    for (size_t next = 0, left = (i * 2) + 1, right = left + 1; left < count;
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

/* Returns true if the winner (the "left hand side") wins the comparison.
   Winning in a three-way comparison means satisfying the total order of the
   priority queue. So, there is no winner if the elements are equal and this
   function would return false. If the winner is in the wrong order, thus
   losing the total order comparison, the function also returns false. */
static inline CCC_tribool
wins(struct CCC_fpq const *const fpq, size_t const winner, size_t const loser)
{
    return cmp(fpq, winner, loser) == fpq->order;
}

static inline CCC_threeway_cmp
cmp(struct CCC_fpq const *const fpq, size_t lhs, size_t rhs)
{
    return fpq->cmp((CCC_any_type_cmp){
        .any_type_lhs = at(fpq, lhs),
        .any_type_rhs = at(fpq, rhs),
        .aux = fpq->buf.aux,
    });
}

/* Swaps i and j using the underlying buffer capabilities. Not checked for
   an error in release. */
static inline void
swap(struct CCC_fpq *const fpq, void *const tmp, size_t const i, size_t const j)
{
    [[maybe_unused]] CCC_result const res = CCC_buf_swap(&fpq->buf, tmp, i, j);
    assert(res == CCC_RESULT_OK);
}

/* Thin wrapper just for sanity checking in debug mode as index should always
   be valid when this function is used. */
static inline void *
at(struct CCC_fpq const *const fpq, size_t const i)
{
    void *const addr = CCC_buf_at(&fpq->buf, i);
    assert(addr);
    return addr;
}

/* Flat priority queue code that uses indices of the underlying buffer should
   always be within the buffer range. It should never exceed the current size
   and start at or after the buffer base. Only checked in debug. */
static inline size_t
index_of(struct CCC_fpq const *const fpq, void const *const slot)
{
    assert(slot >= fpq->buf.mem);
    size_t const i
        = ((char *)slot - (char *)fpq->buf.mem) / fpq->buf.sizeof_type;
    assert(i < fpq->buf.count);
    return i;
}

static inline void
destroy_each(struct CCC_fpq *const fpq, CCC_any_type_destructor_fn *const fn)
{
    size_t const count = fpq->buf.count;
    for (size_t i = 0; i < count; ++i)
    {
        fn((CCC_any_type){
            .any_type = at(fpq, i),
            .aux = fpq->buf.aux,
        });
    }
}
