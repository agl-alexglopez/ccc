#include <stddef.h>
#include <string.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "buffer_util.h"
#include "ccc/buffer.h"
#include "ccc/types.h"

static inline void
swap(void *const tmp, void *const a, void *const b, size_t const absize)
{
    if (a != b)
    {
        (void)memcpy(tmp, a, absize);
        (void)memcpy(a, b, absize);
        (void)memcpy(b, tmp, absize);
    }
}

/** For now we do not use randomization for the partition selection meaning we
fall prey to the O(N^2) worst case runtime more easily. With void and iterators
it is complicated to select a randomized slot but it would still be possible.*/
static int *
partition(Buffer *const b, CCC_Type_comparator *const fn, void *const tmp,
          void *lo, void *hi)
{
    void *const pivot_val = hi;
    void *i = lo;
    for (void *j = lo; j < hi; j = buffer_next(b, j))
    {
        CCC_Order const cmp = fn((CCC_Type_comparator_context){
            .any_type_lhs = j,
            .any_type_rhs = pivot_val,
            .context = b->context,
        });
        if (cmp != CCC_ORDER_GREATER)
        {
            swap(tmp, i, j, b->sizeof_type);
            i = buffer_next(b, i);
        }
    }
    swap(tmp, i, hi, b->sizeof_type);
    return i;
}

/* NOLINTBEGIN(*misc-no-recursion*) */

/** Canonical C quicksort. See Wikipedia for the pseudocode or a breakdown of
different trade offs. See CLRS extra problems for eliminating two recursive
calls and reducing stack space to O(log(N)).

    https://en.wikipedia.org/wiki/Quicksort

This implementation does not try to be special or efficient. In fact because
this is meant to test the Buffer container, it uses iterators only to swap and
sort data. This is a fun way to test that part of the Buffer interface for
correctness and turns out to be pretty nice and clean. */
static void
sort_rec(Buffer *const b, CCC_Type_comparator *const fn, void *const tmp,
         void *lo, void *hi)
{
    while (lo < hi)
    {
        void const *const pivot_i = partition(b, fn, tmp, lo, hi);
        if ((char const *)pivot_i - (char const *)lo
            < (char const *)hi - (char const *)pivot_i)
        {
            sort_rec(b, fn, tmp, lo, buffer_rnext(b, pivot_i));
            lo = buffer_next(b, pivot_i);
        }
        else
        {
            sort_rec(b, fn, tmp, buffer_next(b, pivot_i), hi);
            hi = buffer_rnext(b, pivot_i);
        }
    }
}

/* NOLINTEND(*misc-no-recursion*) */

/** Sorts the provided Buffer in average time O(N * log(N)) and O(log(N))
stack space. This implementation does not try to be hyper efficient. In fact, we
test out using iterators here rather than indices. */
CCC_Result
sort(CCC_Buffer *const b, CCC_Type_comparator *const fn, void *const swap)
{
    if (!b || !fn || !swap)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (buffer_count(b).count)
    {
        sort_rec(b, fn, swap, buffer_begin(b), buffer_rbegin(b));
    }
    return CCC_RESULT_OK;
}

CCC_Order
bufcmp(CCC_Buffer const *const lhs, size_t const rhs_count,
       void const *const rhs)
{
    size_t const type_size = buffer_sizeof_type(lhs).count;
    size_t const buffer_size = buffer_count(lhs).count;
    if (buffer_size < rhs_count)
    {
        return CCC_ORDER_LESSER;
    }
    if (buffer_size < rhs_count)
    {
        return CCC_ORDER_GREATER;
    }
    int const cmp = memcmp(buffer_begin(lhs), rhs, buffer_size * type_size);
    if (cmp == 0)
    {
        return CCC_ORDER_EQUAL;
    }
    if (cmp < 0)
    {
        return CCC_ORDER_LESSER;
    }
    return CCC_ORDER_GREATER;
}

CCC_Result
append_range(CCC_Buffer *const b, size_t range_count, void const *const range)
{
    unsigned char const *p = range;
    size_t const sizeof_type = buffer_sizeof_type(b).count;
    while (range_count--)
    {
        void const *const appended = CCC_buffer_push_back(b, p);
        if (!appended)
        {
            return CCC_RESULT_FAIL;
        }
        p += sizeof_type;
    }
    return CCC_RESULT_OK;
}
