#define BUFFER_USING_NAMESPACE_CCC

#include "buf_util.h"
#include "ccc/buffer.h"
#include "random.h"

static int
partition(buffer *const b, int lo, int hi)
{
    /* Many people suggest random pivot over high index pivot. */
    (void)buf_swap(b, &(int){}, rand_range(lo, hi), hi);
    int const pivot_val = *(int *)buf_at(b, hi);
    int i = lo;
    for (int j = lo; j < hi; ++j)
    {
        int const cur = *(int *)buf_at(b, j);
        if (cur <= pivot_val)
        {
            (void)buf_swap(b, &(int){}, i, j);
            ++i;
        }
    }
    (void)buf_swap(b, &(int){}, i, hi);
    return i;
}

/* NOLINTBEGIN(*misc-no-recursion*) */

/** Canonical C quicksort. See Wikipedia for the pseudocode.
https://en.wikipedia.org/wiki/Quicksort */
static void
sort_rec(buffer *const b, int lo, int const hi)
{
    while (lo < hi)
    {
        int const pivot_i = partition(b, lo, hi);
        sort_rec(b, lo, pivot_i - 1);
        lo = pivot_i + 1;
    }
}

/* NOLINTEND(*misc-no-recursion*) */

ccc_result
sort(ccc_buffer *const b)
{
    if (!b)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (buf_size(b).count)
    {
        sort_rec(b, 0, buf_size(b).count - 1);
    }
    return CCC_RESULT_OK;
}
