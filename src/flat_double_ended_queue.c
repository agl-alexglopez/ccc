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
#include "flat_double_ended_queue.h"
#include "impl/impl_flat_double_ended_queue.h"
#include "types.h"

enum : size_t
{
    START_CAPACITY = 8,
};

/*==========================    Prototypes    ===============================*/

static ccc_result maybe_resize(struct ccc_fdeq *, size_t, ccc_any_alloc_fn *);
static size_t index_of(struct ccc_fdeq const *, void const *);
static void *at(struct ccc_fdeq const *, size_t i);
static size_t increment(struct ccc_fdeq const *, size_t i);
static size_t decrement(struct ccc_fdeq const *, size_t i);
static size_t distance(struct ccc_fdeq const *, size_t, size_t);
static size_t rdistance(struct ccc_fdeq const *, size_t, size_t);
static ccc_result push_front_range(struct ccc_fdeq *fdeq, size_t n,
                                   char const *elems);
static ccc_result push_back_range(struct ccc_fdeq *fdeq, size_t n,
                                  char const *elems);
static size_t back_free_slot(struct ccc_fdeq const *);
static size_t front_free_slot(size_t front, size_t cap);
static size_t last_elem_index(struct ccc_fdeq const *);
static void *push_range(struct ccc_fdeq *fdeq, char const *pos, size_t n,
                        char const *elems);
static void *alloc_front(struct ccc_fdeq *fdeq);
static void *alloc_back(struct ccc_fdeq *fdeq);
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
        (void)memcpy(slot, elem, fdeq->buf.sizeof_type);
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
        (void)memcpy(slot, elem, fdeq->buf.sizeof_type);
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
                 : at(fdeq, fdeq->buf.count - n);
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
    if (ccc_buf_is_empty(&fdeq->buf))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    fdeq->front = increment(fdeq, fdeq->front);
    return ccc_buf_size_minus(&fdeq->buf, 1);
}

ccc_result
ccc_fdeq_pop_back(ccc_flat_double_ended_queue *const fdeq)
{
    if (!fdeq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    return ccc_buf_size_minus(&fdeq->buf, 1);
}

void *
ccc_fdeq_front(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq || ccc_buf_is_empty(&fdeq->buf))
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf, fdeq->front);
}

void *
ccc_fdeq_back(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq || ccc_buf_is_empty(&fdeq->buf))
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf, last_elem_index(fdeq));
}

ccc_tribool
ccc_fdeq_is_empty(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !fdeq->buf.count;
}

ccc_ucount
ccc_fdeq_count(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = fdeq->buf.count};
}

ccc_ucount
ccc_fdeq_capacity(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = fdeq->buf.capacity};
}

void *
ccc_fdeq_at(ccc_flat_double_ended_queue const *const fdeq, size_t const i)
{
    if (!fdeq || i >= fdeq->buf.capacity)
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf, (fdeq->front + i) % fdeq->buf.capacity);
}

void *
ccc_fdeq_begin(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq || !fdeq->buf.count)
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf, fdeq->front);
}

void *
ccc_fdeq_rbegin(ccc_flat_double_ended_queue const *const fdeq)
{
    if (!fdeq || !fdeq->buf.count)
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf, last_elem_index(fdeq));
}

void *
ccc_fdeq_next(ccc_flat_double_ended_queue const *const fdeq,
              void const *const iter_ptr)
{
    size_t const next_i = increment(fdeq, index_of(fdeq, iter_ptr));
    if (next_i == fdeq->front
        || distance(fdeq, next_i, fdeq->front) >= fdeq->buf.count)
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf, next_i);
}

void *
ccc_fdeq_rnext(ccc_flat_double_ended_queue const *const fdeq,
               void const *const iter_ptr)
{
    size_t const cur_i = index_of(fdeq, iter_ptr);
    size_t const next_i = decrement(fdeq, cur_i);
    size_t const rbegin = last_elem_index(fdeq);
    if (next_i == rbegin || rdistance(fdeq, next_i, rbegin) >= fdeq->buf.count)
    {
        return NULL;
    }
    return ccc_buf_at(&fdeq->buf, next_i);
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
    return fdeq ? ccc_buf_begin(&fdeq->buf) : NULL;
}

ccc_result
ccc_fdeq_copy(ccc_flat_double_ended_queue *const dst,
              ccc_flat_double_ended_queue const *const src,
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
       same as in dst initialization because that controls whether the fdeq
       is a ring buffer or dynamic fdeq. */
    void *const dst_mem = dst->buf.mem;
    size_t const dst_cap = dst->buf.capacity;
    ccc_any_alloc_fn *const dst_alloc = dst->buf.alloc;
    *dst = *src;
    dst->buf.mem = dst_mem;
    dst->buf.capacity = dst_cap;
    dst->buf.alloc = dst_alloc;
    /* Copying from an empty source is odd but supported. */
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
    if (!dst->buf.mem || !src->buf.mem)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (dst->buf.capacity > src->buf.capacity)
    {
        size_t const first_chunk
            = min(src->buf.count, src->buf.capacity - src->front);
        (void)memcpy(dst->buf.mem, ccc_buf_at(&src->buf, src->front),
                     src->buf.sizeof_type * first_chunk);
        if (first_chunk < src->buf.count)
        {
            (void)memcpy((char *)dst->buf.mem
                             + (src->buf.sizeof_type * first_chunk),
                         src->buf.mem,
                         src->buf.sizeof_type * (src->buf.count - first_chunk));
        }
        dst->front = 0;
        return CCC_RESULT_OK;
    }
    (void)memcpy(dst->buf.mem, src->buf.mem,
                 src->buf.capacity * src->buf.sizeof_type);
    return CCC_RESULT_OK;
}

ccc_result
ccc_fdeq_reserve(ccc_flat_double_ended_queue *const fdeq, size_t const to_add,
                 ccc_any_alloc_fn *const fn)
{
    if (!fdeq || !fn)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    return maybe_resize(fdeq, to_add, fn);
}

ccc_result
ccc_fdeq_clear(ccc_flat_double_ended_queue *const fdeq,
               ccc_any_type_destructor_fn *const destructor)
{
    if (!fdeq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!destructor)
    {
        fdeq->front = 0;
        return ccc_buf_size_set(&fdeq->buf, 0);
    }
    size_t const back = back_free_slot(fdeq);
    for (size_t i = fdeq->front; i != back; i = increment(fdeq, i))
    {
        destructor((ccc_any_type){
            .any_type = ccc_buf_at(&fdeq->buf, i),
            .aux = fdeq->buf.aux,
        });
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_fdeq_clear_and_free(ccc_flat_double_ended_queue *const fdeq,
                        ccc_any_type_destructor_fn *const destructor)
{
    if (!fdeq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!destructor)
    {
        fdeq->buf.count = fdeq->front = 0;
        return ccc_buf_alloc(&fdeq->buf, 0, fdeq->buf.alloc);
    }
    size_t const back = back_free_slot(fdeq);
    for (size_t i = fdeq->front; i != back; i = increment(fdeq, i))
    {
        destructor((ccc_any_type){
            .any_type = ccc_buf_at(&fdeq->buf, i),
            .aux = fdeq->buf.aux,
        });
    }
    ccc_result const r = ccc_buf_alloc(&fdeq->buf, 0, fdeq->buf.alloc);
    if (r == CCC_RESULT_OK)
    {
        fdeq->buf.count = fdeq->front = 0;
    }
    return r;
}

ccc_result
ccc_fdeq_clear_and_free_reserve(ccc_flat_double_ended_queue *const fdeq,
                                ccc_any_type_destructor_fn *const destructor,
                                ccc_any_alloc_fn *const alloc)
{
    if (!fdeq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!destructor)
    {
        fdeq->buf.count = fdeq->front = 0;
        return ccc_buf_alloc(&fdeq->buf, 0, alloc);
    }
    size_t const back = back_free_slot(fdeq);
    for (size_t i = fdeq->front; i != back; i = increment(fdeq, i))
    {
        destructor((ccc_any_type){
            .any_type = ccc_buf_at(&fdeq->buf, i),
            .aux = fdeq->buf.aux,
        });
    }
    ccc_result const r = ccc_buf_alloc(&fdeq->buf, 0, alloc);
    if (r == CCC_RESULT_OK)
    {
        fdeq->buf.count = fdeq->front = 0;
    }
    return r;
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
    if (ccc_buf_i(&fdeq->buf, iter).count != fdeq->front)
    {
        return CCC_FALSE;
    }
    size_t size = 0;
    for (; iter != ccc_fdeq_end(fdeq); iter = ccc_fdeq_next(fdeq, iter), ++size)
    {
        if (size >= ccc_fdeq_count(fdeq).count)
        {
            return CCC_FALSE;
        }
    }
    if (size != ccc_fdeq_count(fdeq).count)
    {
        return CCC_FALSE;
    }
    size = 0;
    iter = ccc_fdeq_rbegin(fdeq);
    if (ccc_buf_i(&fdeq->buf, iter).count != last_elem_index(fdeq))
    {
        return CCC_FALSE;
    }
    for (; iter != ccc_fdeq_rend(fdeq);
         iter = ccc_fdeq_rnext(fdeq, iter), ++size)
    {
        if (size >= ccc_fdeq_count(fdeq).count)
        {
            return CCC_FALSE;
        }
    }
    return size == ccc_fdeq_count(fdeq).count;
}

/*======================   Private Interface   ==============================*/

void *
ccc_impl_fdeq_alloc_back(struct ccc_fdeq *const fdeq)
{
    return alloc_back(fdeq);
}

void *
ccc_impl_fdeq_alloc_front(struct ccc_fdeq *const fdeq)
{
    return alloc_front(fdeq);
}

/*======================     Static Helpers    ==============================*/

static void *
alloc_front(struct ccc_fdeq *const fdeq)
{
    ccc_tribool const full
        = maybe_resize(fdeq, 1, fdeq->buf.alloc) != CCC_RESULT_OK;
    /* Should have been able to resize. Bad error. */
    if (fdeq->buf.alloc && full)
    {
        return NULL;
    }
    fdeq->front = front_free_slot(fdeq->front, fdeq->buf.capacity);
    void *const new_slot = ccc_buf_at(&fdeq->buf, fdeq->front);
    if (!full)
    {
        (void)ccc_buf_size_plus(&fdeq->buf, 1);
    }
    return new_slot;
}

static void *
alloc_back(struct ccc_fdeq *const fdeq)
{
    ccc_tribool const full
        = maybe_resize(fdeq, 1, fdeq->buf.alloc) != CCC_RESULT_OK;
    /* Should have been able to resize. Bad error. */
    if (fdeq->buf.alloc && full)
    {
        return NULL;
    }
    void *const new_slot = ccc_buf_at(&fdeq->buf, back_free_slot(fdeq));
    /* If no reallocation policy is given we are a ring buffer. */
    if (full)
    {
        fdeq->front = increment(fdeq, fdeq->front);
    }
    else
    {
        (void)ccc_buf_size_plus(&fdeq->buf, 1);
    }
    return new_slot;
}

static ccc_result
push_back_range(struct ccc_fdeq *const fdeq, size_t const n, char const *elems)
{
    size_t const sizeof_type = fdeq->buf.sizeof_type;
    ccc_tribool const full
        = maybe_resize(fdeq, n, fdeq->buf.alloc) != CCC_RESULT_OK;
    size_t const cap = fdeq->buf.capacity;
    if (fdeq->buf.alloc && full)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    /* If a range is too large we can make various simplifications to preserve
       the final capacity elements. */
    if (n >= cap)
    {
        elems += ((n - cap) * sizeof_type);
        fdeq->front = 0;
        (void)memcpy(ccc_buf_at(&fdeq->buf, 0), elems, sizeof_type * cap);
        (void)ccc_buf_size_set(&fdeq->buf, cap);
        return CCC_RESULT_OK;
    }
    size_t const new_size = fdeq->buf.count + n;
    size_t const back_slot = back_free_slot(fdeq);
    size_t const chunk = min(n, cap - back_slot);
    size_t const remainder_back_slot = (back_slot + chunk) % cap;
    size_t const remainder = (n - chunk);
    char const *const second_chunk = elems + (chunk * sizeof_type);
    (void)memcpy(ccc_buf_at(&fdeq->buf, back_slot), elems, chunk * sizeof_type);
    if (remainder)
    {
        (void)memcpy(ccc_buf_at(&fdeq->buf, remainder_back_slot), second_chunk,
                     remainder * sizeof_type);
    }
    if (new_size > cap)
    {
        fdeq->front = (fdeq->front + (new_size - cap)) % cap;
    }
    (void)ccc_buf_size_set(&fdeq->buf, min(cap, new_size));
    return CCC_RESULT_OK;
}

static ccc_result
push_front_range(struct ccc_fdeq *const fdeq, size_t const n, char const *elems)
{
    size_t const sizeof_type = fdeq->buf.sizeof_type;
    ccc_tribool const full
        = maybe_resize(fdeq, n, fdeq->buf.alloc) != CCC_RESULT_OK;
    size_t const cap = fdeq->buf.capacity;
    if (fdeq->buf.alloc && full)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    /* If a range is too large we can make various simplifications to preserve
       the final capacity elements. */
    if (n >= cap)
    {
        elems += ((n - cap) * sizeof_type);
        fdeq->front = 0;
        (void)memcpy(ccc_buf_at(&fdeq->buf, 0), elems, sizeof_type * cap);
        (void)ccc_buf_size_set(&fdeq->buf, cap);
        return CCC_RESULT_OK;
    }
    size_t const space_ahead = front_free_slot(fdeq->front, cap) + 1;
    size_t const i = n > space_ahead ? 0 : space_ahead - n;
    size_t const chunk = min(n, space_ahead);
    size_t const remainder = (n - chunk);
    char const *const first_chunk = elems + ((n - chunk) * sizeof_type);
    (void)memcpy(ccc_buf_at(&fdeq->buf, i), first_chunk, chunk * sizeof_type);
    if (remainder)
    {
        (void)memcpy(ccc_buf_at(&fdeq->buf, cap - remainder), elems,
                     remainder * sizeof_type);
    }
    (void)ccc_buf_size_set(&fdeq->buf, min(cap, fdeq->buf.count + n));
    fdeq->front = remainder ? cap - remainder : i;
    return CCC_RESULT_OK;
}

static void *
push_range(struct ccc_fdeq *const fdeq, char const *const pos, size_t n,
           char const *elems)
{
    size_t const sizeof_type = fdeq->buf.sizeof_type;
    ccc_tribool const full
        = maybe_resize(fdeq, n, fdeq->buf.alloc) != CCC_RESULT_OK;
    if (fdeq->buf.alloc && full)
    {
        return NULL;
    }
    size_t const cap = fdeq->buf.capacity;
    size_t const new_size = fdeq->buf.count + n;
    if (n >= cap)
    {
        elems += ((n - cap) * sizeof_type);
        fdeq->front = 0;
        void *const ret = ccc_buf_at(&fdeq->buf, 0);
        (void)memcpy(ret, elems, sizeof_type * cap);
        (void)ccc_buf_size_set(&fdeq->buf, cap);
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
    (void)memmove(ccc_buf_at(&fdeq->buf, move_i), ccc_buf_at(&fdeq->buf, pos_i),
                  move_chunk * sizeof_type);
    if (move_remain)
    {
        size_t const move_remain_i = (move_i + move_chunk) % cap;
        size_t const remaining_start_i = (pos_i + move_chunk) % cap;
        (void)memmove(ccc_buf_at(&fdeq->buf, move_remain_i),
                      ccc_buf_at(&fdeq->buf, remaining_start_i),
                      move_remain * sizeof_type);
    }
    size_t const elems_chunk = min(n, cap - pos_i);
    size_t const elems_remain = n - elems_chunk;
    (void)memcpy(ccc_buf_at(&fdeq->buf, pos_i), elems,
                 elems_chunk * sizeof_type);
    if (elems_remain)
    {
        char const *const second_chunk = elems + (elems_chunk * sizeof_type);
        size_t const second_chunk_i = (pos_i + elems_chunk) % cap;
        (void)memcpy(ccc_buf_at(&fdeq->buf, second_chunk_i), second_chunk,
                     elems_remain * sizeof_type);
    }
    if (new_size > cap)
    {
        /* Wrapping behavior stops if it would overwrite the start of the
           range being inserted. This is to preserve as much info about
           the range as possible. If wrapping occurs the range is the new
           front. */
        size_t const excess = (new_size - cap);
        size_t const front_to_pos_dist = (pos_i + cap - fdeq->front) % cap;
        fdeq->front = (fdeq->front + min(excess, front_to_pos_dist)) % cap;
    }
    (void)ccc_buf_size_set(&fdeq->buf, min(cap, new_size));
    return ccc_buf_at(&fdeq->buf, pos_i);
}

static ccc_result
maybe_resize(struct ccc_fdeq *const q, size_t const additional_elems_to_add,
             ccc_any_alloc_fn *const fn)
{
    size_t required = q->buf.count + additional_elems_to_add;
    if (required < q->buf.count)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (required <= q->buf.capacity)
    {
        return CCC_RESULT_OK;
    }
    if (!fn)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    size_t const sizeof_type = q->buf.sizeof_type;
    if (!q->buf.capacity && additional_elems_to_add == 1)
    {
        required = START_CAPACITY;
    }
    else if (additional_elems_to_add == 1)
    {
        required = q->buf.capacity * 2;
    }
    void *const new_mem = fn(NULL, sizeof_type * required, q->buf.aux);
    if (!new_mem)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    if (q->buf.count)
    {
        size_t const first_chunk
            = min(q->buf.count, q->buf.capacity - q->front);
        (void)memcpy(new_mem, ccc_buf_at(&q->buf, q->front),
                     sizeof_type * first_chunk);
        if (first_chunk < q->buf.count)
        {
            (void)memcpy((char *)new_mem + (sizeof_type * first_chunk),
                         ccc_buf_begin(&q->buf),
                         sizeof_type * (q->buf.count - first_chunk));
        }
    }
    (void)ccc_buf_alloc(&q->buf, 0, fn);
    q->buf.mem = new_mem;
    q->front = 0;
    q->buf.capacity = required;
    return CCC_RESULT_OK;
}

/** Returns the distance between the current iter position and the origin
position. Distance is calculated in ascending indices, meaning the result is
the number of forward steps in the buffer origin would need to take reach iter,
possibly accounting for wrapping around the end of the buffer. */
static inline size_t
distance(struct ccc_fdeq const *const fdeq, size_t const iter,
         size_t const origin)
{
    return iter > origin ? iter - origin : (fdeq->buf.capacity - origin) + iter;
}

/** Returns the rdistance between the current iter position and the origin
position. Rdistance is calculated in descending indices, meaning the result is
the number of backward steps in the buffer origin would need to take to reach
iter, possibly accounting for wrapping around the beginning of buffer. */
static inline size_t
rdistance(struct ccc_fdeq const *const fdeq, size_t const iter,
          size_t const origin)
{
    return iter > origin ? (fdeq->buf.capacity - iter) + origin : origin - iter;
}

static inline size_t
index_of(struct ccc_fdeq const *const fdeq, void const *const pos)
{
    assert(pos >= ccc_buf_begin(&fdeq->buf));
    assert(pos < ccc_buf_capacity_end(&fdeq->buf));
    return (size_t)(((char *)pos - (char *)ccc_buf_begin(&fdeq->buf))
                    / fdeq->buf.sizeof_type);
}

static inline void *
at(struct ccc_fdeq const *const fdeq, size_t const i)
{
    return ccc_buf_at(&fdeq->buf, (fdeq->front + i) % fdeq->buf.capacity);
}

static inline size_t
increment(struct ccc_fdeq const *const fdeq, size_t const i)
{
    return i == (fdeq->buf.capacity - 1) ? 0 : i + 1;
}

static inline size_t
decrement(struct ccc_fdeq const *const fdeq, size_t const i)
{
    return i ? i - 1 : fdeq->buf.capacity - 1;
}

static inline size_t
back_free_slot(struct ccc_fdeq const *const fdeq)
{
    return (fdeq->front + fdeq->buf.count) % fdeq->buf.capacity;
}

static inline size_t
front_free_slot(size_t const front, size_t const cap)
{
    return front ? front - 1 : cap - 1;
}

/** Returns index of the last element in the fdeq or front if empty. */
static inline size_t
last_elem_index(struct ccc_fdeq const *const fdeq)
{
    return fdeq->buf.count
             ? (fdeq->front + fdeq->buf.count - 1) % fdeq->buf.capacity
             : fdeq->front;
}

static inline size_t
min(size_t const a, size_t const b)
{
    return a < b ? a : b;
}
