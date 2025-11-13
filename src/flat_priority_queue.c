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
#include "private/private_flat_priority_queue.h"
#include "types.h"

enum : size_t
{
    START_CAP = 8,
};

/*=====================      Prototypes      ================================*/

static void *at(struct CCC_Flat_priority_queue const *, size_t);
static size_t index_of(struct CCC_Flat_priority_queue const *, void const *);
static CCC_Tribool wins(struct CCC_Flat_priority_queue const *, size_t winner,
                        size_t loser);
static CCC_Order order(struct CCC_Flat_priority_queue const *, size_t lhs,
                       size_t rhs);
static void swap(struct CCC_Flat_priority_queue *, void *tmp, size_t, size_t);
static size_t bubble_up(struct CCC_Flat_priority_queue *flat_priority_queue,
                        void *tmp, size_t i);
static size_t bubble_down(struct CCC_Flat_priority_queue *, void *tmp, size_t,
                          size_t);
static size_t update_fixup(struct CCC_Flat_priority_queue *, void *e,
                           void *tmp);
static void heapify(struct CCC_Flat_priority_queue *flat_priority_queue,
                    size_t n, void *);
static void destroy_each(struct CCC_Flat_priority_queue *flat_priority_queue,
                         CCC_Type_destructor *fn);

/*=====================       Interface      ================================*/

CCC_Result
CCC_flat_priority_queue_heapify(
    CCC_Flat_priority_queue *const flat_priority_queue, void *const tmp,
    void *const array, size_t const n, size_t const input_sizeof_type)
{
    if (!flat_priority_queue || !array || !tmp
        || array == flat_priority_queue->buf.mem
        || input_sizeof_type != flat_priority_queue->buf.sizeof_type)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (n > flat_priority_queue->buf.capacity)
    {
        CCC_Result const resize_res = CCC_buffer_alloc(
            &flat_priority_queue->buf, n, flat_priority_queue->buf.alloc);
        if (resize_res != CCC_RESULT_OK)
        {
            return resize_res;
        }
    }
    (void)memcpy(flat_priority_queue->buf.mem, array, n * input_sizeof_type);
    heapify(flat_priority_queue, n, tmp);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_flat_priority_queue_heapify_inplace(
    CCC_Flat_priority_queue *flat_priority_queue, void *const tmp,
    size_t const n)
{
    if (!flat_priority_queue || !tmp || n > flat_priority_queue->buf.capacity)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    heapify(flat_priority_queue, n, tmp);
    return CCC_RESULT_OK;
}

CCC_Buffer
CCC_flat_priority_queue_heapsort(
    CCC_Flat_priority_queue *const flat_priority_queue, void *const tmp)
{
    CCC_Buffer ret = {};
    if (!flat_priority_queue || !tmp)
    {
        return ret;
    }
    ret = flat_priority_queue->buf;
    if (flat_priority_queue->buf.count > 1)
    {
        size_t end = flat_priority_queue->buf.count;
        while (--end)
        {
            swap(flat_priority_queue, tmp, 0, end);
            (void)bubble_down(flat_priority_queue, tmp, 0, end);
        }
    }
    *flat_priority_queue = (CCC_Flat_priority_queue){};
    return ret;
}

void *
CCC_flat_priority_queue_push(CCC_Flat_priority_queue *const flat_priority_queue,
                             void const *const elem, void *const tmp)
{
    if (!flat_priority_queue || !elem || !tmp)
    {
        return NULL;
    }
    void *const new = CCC_buffer_alloc_back(&flat_priority_queue->buf);
    if (!new)
    {
        return NULL;
    }
    if (new != elem)
    {
        (void)memcpy(new, elem, flat_priority_queue->buf.sizeof_type);
    }
    assert(tmp);
    size_t const i = bubble_up(flat_priority_queue, tmp,
                               flat_priority_queue->buf.count - 1);
    assert(i < flat_priority_queue->buf.count);
    return CCC_buffer_at(&flat_priority_queue->buf, i);
}

CCC_Result
CCC_flat_priority_queue_pop(CCC_Flat_priority_queue *const flat_priority_queue,
                            void *const tmp)
{
    if (!flat_priority_queue || !tmp || !flat_priority_queue->buf.count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    --flat_priority_queue->buf.count;
    if (!flat_priority_queue->buf.count)
    {
        return CCC_RESULT_OK;
    }
    swap(flat_priority_queue, tmp, 0, flat_priority_queue->buf.count);
    (void)bubble_down(flat_priority_queue, tmp, 0,
                      flat_priority_queue->buf.count);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_flat_priority_queue_erase(
    CCC_Flat_priority_queue *const flat_priority_queue, void *const elem,
    void *const tmp)
{
    if (!flat_priority_queue || !elem || !tmp
        || !flat_priority_queue->buf.count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    size_t const i = index_of(flat_priority_queue, elem);
    --flat_priority_queue->buf.count;
    if (i == flat_priority_queue->buf.count)
    {
        return CCC_RESULT_OK;
    }
    swap(flat_priority_queue, tmp, i, flat_priority_queue->buf.count);
    CCC_Order const order_res
        = order(flat_priority_queue, i, flat_priority_queue->buf.count);
    if (order_res == flat_priority_queue->order)
    {
        (void)bubble_up(flat_priority_queue, tmp, i);
    }
    else if (order_res != CCC_ORDER_EQUAL)
    {
        (void)bubble_down(flat_priority_queue, tmp, i,
                          flat_priority_queue->buf.count);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return CCC_RESULT_OK;
}

void *
CCC_flat_priority_queue_update(
    CCC_Flat_priority_queue *const flat_priority_queue, void *const elem,
    void *const tmp, CCC_Type_updater *const fn, void *const context)
{
    if (!flat_priority_queue || !elem || !tmp || !fn
        || !flat_priority_queue->buf.count)
    {
        return NULL;
    }
    fn((CCC_Type_context){
        .type = elem,
        .context = context,
    });
    return CCC_buffer_at(&flat_priority_queue->buf,
                         update_fixup(flat_priority_queue, elem, tmp));
}

/* There are no efficiency benefits in knowing an increase will occur. */
void *
CCC_flat_priority_queue_increase(
    CCC_Flat_priority_queue *const flat_priority_queue, void *const elem,
    void *const tmp, CCC_Type_updater *const fn, void *const context)
{
    return CCC_flat_priority_queue_update(flat_priority_queue, elem, tmp, fn,
                                          context);
}

/* There are no efficiency benefits in knowing an decrease will occur. */
void *
CCC_flat_priority_queue_decrease(
    CCC_Flat_priority_queue *const flat_priority_queue, void *const elem,
    void *const tmp, CCC_Type_updater *const fn, void *const context)
{
    return CCC_flat_priority_queue_update(flat_priority_queue, elem, tmp, fn,
                                          context);
}

void *
CCC_flat_priority_queue_front(
    CCC_Flat_priority_queue const *const flat_priority_queue)
{
    if (!flat_priority_queue || !flat_priority_queue->buf.count)
    {
        return NULL;
    }
    return at(flat_priority_queue, 0);
}

CCC_Tribool
CCC_flat_priority_queue_is_empty(
    CCC_Flat_priority_queue const *const flat_priority_queue)
{
    if (!flat_priority_queue)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return CCC_buffer_is_empty(&flat_priority_queue->buf);
}

CCC_Count
CCC_flat_priority_queue_count(
    CCC_Flat_priority_queue const *const flat_priority_queue)
{
    if (!flat_priority_queue)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return CCC_buffer_count(&flat_priority_queue->buf);
}

CCC_Count
CCC_flat_priority_queue_capacity(
    CCC_Flat_priority_queue const *const flat_priority_queue)
{
    if (!flat_priority_queue)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return CCC_buffer_capacity(&flat_priority_queue->buf);
}

void *
CCC_flat_priority_queue_data(
    CCC_Flat_priority_queue const *const flat_priority_queue)
{
    return flat_priority_queue ? CCC_buffer_begin(&flat_priority_queue->buf)
                               : NULL;
}

CCC_Order
CCC_flat_priority_queue_order(
    CCC_Flat_priority_queue const *const flat_priority_queue)
{
    return flat_priority_queue ? flat_priority_queue->order : CCC_ORDER_ERROR;
}

CCC_Result
CCC_flat_priority_queue_reserve(
    CCC_Flat_priority_queue *const flat_priority_queue, size_t const to_add,
    CCC_Allocator *const fn)
{
    if (!flat_priority_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    return CCC_buffer_reserve(&flat_priority_queue->buf, to_add, fn);
}

CCC_Result
CCC_flat_priority_queue_copy(CCC_Flat_priority_queue *const dst,
                             CCC_Flat_priority_queue const *const src,
                             CCC_Allocator *const fn)
{
    if (!dst || !src || src == dst
        || (dst->buf.capacity < src->buf.capacity && !fn))
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* Copy everything so we don't worry about staying in sync with future
       changes to buf container. But we have to give back original destination
       memory in case it has already been allocated. Alloc will remain the
       same as in dst initialization because that controls permission. */
    void *const dst_mem = dst->buf.mem;
    size_t const dst_cap = dst->buf.capacity;
    CCC_Allocator *const dst_alloc = dst->buf.alloc;
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
        CCC_Result const r = CCC_buffer_alloc(&dst->buf, src->buf.capacity, fn);
        if (r != CCC_RESULT_OK)
        {
            return r;
        }
        dst->buf.capacity = src->buf.capacity;
    }
    if (!src->buf.mem || !dst->buf.mem)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* It is ok to only copy count elements because we know that all elements
       in a binary heap are contiguous from [0, C), where C is count. */
    (void)memcpy(dst->buf.mem, src->buf.mem,
                 src->buf.count * src->buf.sizeof_type);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_flat_priority_queue_clear(
    CCC_Flat_priority_queue *const flat_priority_queue,
    CCC_Type_destructor *const fn)
{
    if (!flat_priority_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (fn)
    {
        destroy_each(flat_priority_queue, fn);
    }
    return CCC_buffer_size_set(&flat_priority_queue->buf, 0);
}

CCC_Result
CCC_flat_priority_queue_clear_and_free(
    CCC_Flat_priority_queue *const flat_priority_queue,
    CCC_Type_destructor *const fn)
{
    if (!flat_priority_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (fn)
    {
        destroy_each(flat_priority_queue, fn);
    }
    return CCC_buffer_alloc(&flat_priority_queue->buf, 0,
                            flat_priority_queue->buf.alloc);
}

CCC_Result
CCC_flat_priority_queue_clear_and_free_reserve(
    CCC_Flat_priority_queue *const flat_priority_queue,
    CCC_Type_destructor *const destructor, CCC_Allocator *const alloc)
{
    if (!flat_priority_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (destructor)
    {
        destroy_each(flat_priority_queue, destructor);
    }
    return CCC_buffer_alloc(&flat_priority_queue->buf, 0, alloc);
}

CCC_Tribool
CCC_flat_priority_queue_validate(
    CCC_Flat_priority_queue const *const flat_priority_queue)
{
    if (!flat_priority_queue)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const count = flat_priority_queue->buf.count;
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
        if (left < count && wins(flat_priority_queue, left, i))
        {
            return CCC_FALSE;
        }
        if (right < count && wins(flat_priority_queue, right, i))
        {
            return CCC_FALSE;
        }
    }
    return CCC_TRUE;
}

/*===================     Private Interface     =============================*/

size_t
CCC_private_flat_priority_queue_bubble_up(
    struct CCC_Flat_priority_queue *const flat_priority_queue, void *const tmp,
    size_t i)
{
    return bubble_up(flat_priority_queue, tmp, i);
}

void *
CCC_private_flat_priority_queue_update_fixup(
    struct CCC_Flat_priority_queue *const flat_priority_queue, void *const elem,
    void *const tmp)
{
    return CCC_buffer_at(&flat_priority_queue->buf,
                         update_fixup(flat_priority_queue, elem, tmp));
}

void
CCC_private_flat_priority_queue_in_place_heapify(
    struct CCC_Flat_priority_queue *const flat_priority_queue, size_t const n,
    void *const tmp)
{
    if (!flat_priority_queue || flat_priority_queue->buf.capacity < n)
    {
        return;
    }
    heapify(flat_priority_queue, n, tmp);
}

/*====================     Static Helpers     ===============================*/

/* Orders the heap in O(N) time. Assumes n > 0 and n <= capacity. */
static void
heapify(struct CCC_Flat_priority_queue *const flat_priority_queue,
        size_t const n, void *const tmp)
{
    assert(n);
    assert(n <= flat_priority_queue->buf.capacity);
    flat_priority_queue->buf.count = n;
    size_t i = ((n - 1) / 2) + 1;
    while (i--)
    {
        (void)bubble_down(flat_priority_queue, tmp, i,
                          flat_priority_queue->buf.count);
    }
}

/* Fixes the position of element e after its key value has been changed. */
static size_t
update_fixup(struct CCC_Flat_priority_queue *const flat_priority_queue,
             void *const e, void *const tmp)
{
    size_t const i = index_of(flat_priority_queue, e);
    if (!i)
    {
        return bubble_down(flat_priority_queue, tmp, 0,
                           flat_priority_queue->buf.count);
    }
    CCC_Order const parent_order = order(flat_priority_queue, i, (i - 1) / 2);
    if (parent_order == flat_priority_queue->order)
    {
        return bubble_up(flat_priority_queue, tmp, i);
    }
    if (parent_order != CCC_ORDER_EQUAL)
    {
        return bubble_down(flat_priority_queue, tmp, i,
                           flat_priority_queue->buf.count);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return i;
}

/* Returns the sorted position of the element starting at position i. */
static size_t
bubble_up(struct CCC_Flat_priority_queue *const flat_priority_queue,
          void *const tmp, size_t i)
{
    for (size_t parent = (i - 1) / 2; i; i = parent, parent = (parent - 1) / 2)
    {
        /* Not winning here means we are in correct order or equal. */
        if (!wins(flat_priority_queue, i, parent))
        {
            return i;
        }
        swap(flat_priority_queue, tmp, i, parent);
    }
    return 0;
}

/* Returns the sorted position of the element starting at position i. */
static size_t
bubble_down(struct CCC_Flat_priority_queue *const flat_priority_queue,
            void *const tmp, size_t i, size_t const count)
{
    for (size_t next = 0, left = (i * 2) + 1, right = left + 1; left < count;
         i = next, left = (i * 2) + 1, right = left + 1)
    {
        /* Avoid one comparison call if there is no right child. */
        next = (right < count && wins(flat_priority_queue, right, left)) ? right
                                                                         : left;
        /* If the child beats the parent we must swap. Equal is OK to break. */
        if (!wins(flat_priority_queue, next, i))
        {
            return i;
        }
        swap(flat_priority_queue, tmp, next, i);
    }
    return i;
}

/* Returns true if the winner (the "left hand side") wins the comparison.
   Winning in a three-way comparison means satisfying the total order of the
   priority queue. So, there is no winner if the elements are equal and this
   function would return false. If the winner is in the wrong order, thus
   losing the total order comparison, the function also returns false. */
static inline CCC_Tribool
wins(struct CCC_Flat_priority_queue const *const flat_priority_queue,
     size_t const winner, size_t const loser)
{
    return order(flat_priority_queue, winner, loser)
        == flat_priority_queue->order;
}

static inline CCC_Order
order(struct CCC_Flat_priority_queue const *const flat_priority_queue,
      size_t lhs, size_t rhs)
{
    return flat_priority_queue->compare((CCC_Type_comparator_context){
        .type_lhs = at(flat_priority_queue, lhs),
        .type_rhs = at(flat_priority_queue, rhs),
        .context = flat_priority_queue->buf.context,
    });
}

/* Swaps i and j using the underlying Buffer capabilities. Not checked for
   an error in release. */
static inline void
swap(struct CCC_Flat_priority_queue *const flat_priority_queue, void *const tmp,
     size_t const i, size_t const j)
{
    [[maybe_unused]] CCC_Result const res
        = CCC_buffer_swap(&flat_priority_queue->buf, tmp, i, j);
    assert(res == CCC_RESULT_OK);
}

/* Thin wrapper just for sanity checking in debug mode as index should always
   be valid when this function is used. */
static inline void *
at(struct CCC_Flat_priority_queue const *const flat_priority_queue,
   size_t const i)
{
    void *const addr = CCC_buffer_at(&flat_priority_queue->buf, i);
    assert(addr);
    return addr;
}

/* Flat priority queue code that uses indices of the underlying Buffer should
   always be within the Buffer range. It should never exceed the current size
   and start at or after the Buffer base. Only checked in debug. */
static inline size_t
index_of(struct CCC_Flat_priority_queue const *const flat_priority_queue,
         void const *const slot)
{
    assert(slot >= flat_priority_queue->buf.mem);
    size_t const i = ((char *)slot - (char *)flat_priority_queue->buf.mem)
                   / flat_priority_queue->buf.sizeof_type;
    assert(i < flat_priority_queue->buf.count);
    return i;
}

static inline void
destroy_each(struct CCC_Flat_priority_queue *const flat_priority_queue,
             CCC_Type_destructor *const fn)
{
    size_t const count = flat_priority_queue->buf.count;
    for (size_t i = 0; i < count; ++i)
    {
        fn((CCC_Type_context){
            .type = at(flat_priority_queue, i),
            .context = flat_priority_queue->buf.context,
        });
    }
}
