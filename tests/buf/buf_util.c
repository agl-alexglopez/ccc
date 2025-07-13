#include <stddef.h>
#include <string.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "buf_util.h"
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

static int *
partition(buffer *const b, ccc_any_type_cmp_fn *const fn, void *const tmp,
          void *lo, void *hi)
{
    void *const pivot_val = hi;
    void *i = lo;
    for (void *j = lo; j < hi; j = buf_next(b, j))
    {
        ccc_threeway_cmp const cmp = fn((ccc_any_type_cmp){
            .any_type_lhs = j,
            .any_type_rhs = pivot_val,
            .aux = b->aux,
        });
        if (cmp != CCC_GRT)
        {
            swap(tmp, i, j, b->sizeof_type);
            i = buf_next(b, i);
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
this is meant to test the buffer container, it uses iterators only to swap and
sort data. This is a fun way to test that part of the buffer interface for
correctness and turns out to be pretty nice and clean. */
static void
sort_rec(buffer *const b, ccc_any_type_cmp_fn *const fn, void *const tmp,
         void *lo, void *hi)
{
    while (lo < hi)
    {
        void *pivot_i = partition(b, fn, tmp, lo, hi);
        if ((char *)pivot_i - (char *)lo < (char *)hi - (char *)pivot_i)
        {
            sort_rec(b, fn, tmp, lo, buf_rnext(b, pivot_i));
            lo = buf_next(b, pivot_i);
        }
        else
        {
            sort_rec(b, fn, tmp, buf_next(b, pivot_i), hi);
            hi = buf_rnext(b, pivot_i);
        }
    }
}

/* NOLINTEND(*misc-no-recursion*) */

/** Sorts the provided buffer in average time O(N * log(N)) and O(log(N))
stack space. This implementation does not try to be hyper efficient. In fact, we
test out using iterators here rather than indices. */
ccc_result
sort(ccc_buffer *const b, ccc_any_type_cmp_fn *const fn, void *const swap)
{
    if (!b || !fn || !swap)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (buf_count(b).count)
    {
        sort_rec(b, fn, swap, buf_begin(b), buf_rbegin(b));
    }
    return CCC_RESULT_OK;
}

ccc_threeway_cmp
bufcmp(ccc_buffer const *const lhs, size_t const rhs_count,
       void const *const rhs)
{
    size_t const type_size = buf_sizeof_type(lhs).count;
    size_t const buf_size = buf_count(lhs).count;
    if (buf_size < rhs_count)
    {
        return CCC_LES;
    }
    if (buf_size < rhs_count)
    {
        return CCC_GRT;
    }
    int const cmp = memcmp(buf_begin(lhs), rhs, buf_size * type_size);
    if (cmp == 0)
    {
        return CCC_EQL;
    }
    if (cmp < 0)
    {
        return CCC_LES;
    }
    return CCC_GRT;
}

ccc_result
append_range(ccc_buffer *const b, size_t range_count, void const *const range)
{
    unsigned char const *p = range;
    size_t const sizeof_type = buf_sizeof_type(b).count;
    while (range_count--)
    {
        void const *const appended = ccc_buf_push_back(b, p);
        if (!appended)
        {
            return CCC_RESULT_FAIL;
        }
        p += sizeof_type;
    }
    return CCC_RESULT_OK;
}
