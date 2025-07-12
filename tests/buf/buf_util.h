#ifndef BUF_UTIL_H
#define BUF_UTIL_H

#include "ccc/buffer.h"
#include "ccc/types.h"

/** Sorts a buffer according to its size assuming that elements are stored from
indices [0, N) where N is the size not capacity of the buffer. Requires a
comparison function from the user and one swap slot equivalent to the size of an
element stored in the buffer. Elements are sorted in a non-decreasing order.
Therefore if a non-increasing list is needed simply reverse the return of
the comparison function for non-equivalent values. */
ccc_result sort(ccc_buffer *b, ccc_any_type_cmp_fn *fn, void *swap);

/** Compares the buffer contents as left hand side to the provided sequence of
elements as the right hand side. The sequence type must match the type stored in
the buffer because that type will be used to iterate through the sequence. */
ccc_threeway_cmp bufcmp(ccc_buffer const *lhs, size_t rhs_count,
                        void const *rhs);

/** Appends the provided range into the buffer. If the range will exceed
capacity of a fixed size buffer, only those elements which fit will be pushed
and a failure will be returned. If resizing is allowed the full range should
be appended unless reallocation fails in which case an error is returned. */
ccc_result append_range(ccc_buffer *b, size_t range_count, void const *range);

/** Returns max int between a and b. Ties go to a. */
static inline int
maxint(int a, int b)
{
    return a > b ? a : b;
}

#endif /* BUF_UTIL_H */
