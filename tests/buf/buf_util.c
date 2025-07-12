#include <stddef.h>
#include <string.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "buf_util.h"
#include "ccc/buffer.h"
#include "ccc/types.h"
#include "random.h"

static int
partition(buffer *const b, ccc_any_type_cmp_fn *const fn, void *const swap,
          int lo, int hi)
{
    /* Many people suggest random pivot over high index pivot. */
    (void)buf_swap(b, swap, rand_range(lo, hi), hi);
    void *const pivot_val = buf_at(b, hi);
    int i = lo;
    for (int j = lo; j < hi; ++j)
    {
        ccc_threeway_cmp const cmp = fn((ccc_any_type_cmp){
            .any_type_lhs = buf_at(b, j),
            .any_type_rhs = pivot_val,
            .aux = b->aux,
        });
        if (cmp != CCC_GRT)
        {
            (void)buf_swap(b, swap, i, j);
            ++i;
        }
    }
    (void)buf_swap(b, swap, i, hi);
    return i;
}

/* NOLINTBEGIN(*misc-no-recursion*) */

/** Canonical C quicksort. See Wikipedia for the pseudocode.
https://en.wikipedia.org/wiki/Quicksort */
static void
sort_rec(buffer *const b, ccc_any_type_cmp_fn *const fn, void *const swap,
         int lo, int hi)
{
    while (lo < hi)
    {
        int const pivot_i = partition(b, fn, swap, lo, hi);
        if (pivot_i - lo < hi - pivot_i)
        {
            sort_rec(b, fn, swap, lo, pivot_i - 1);
            lo = pivot_i + 1;
        }
        else
        {
            sort_rec(b, fn, swap, pivot_i + 1, hi);
            hi = pivot_i - 1;
        }
    }
}

/* NOLINTEND(*misc-no-recursion*) */

ccc_result
sort(ccc_buffer *const b, ccc_any_type_cmp_fn *const fn, void *const swap)
{
    if (!b || !fn || !swap)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (buf_size(b).count)
    {
        sort_rec(b, fn, swap, 0, buf_size(b).count - 1);
    }
    return CCC_RESULT_OK;
}

ccc_threeway_cmp
bufcmp(ccc_buffer const *const lhs, size_t const rhs_count,
       void const *const rhs)
{
    size_t const type_size = buf_sizeof_type(lhs).count;
    size_t const buf_size = buf_size(lhs).count;
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
