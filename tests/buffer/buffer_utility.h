#ifndef BUF_UTIL_H
#define BUF_UTIL_H

#include <stddef.h>

#include "ccc/buffer.h"
#include "ccc/types.h"

/** Sorts a Buffer according to its size assuming that elements are stored from
indices [0, N) where N is the size not capacity of the buffer. Requires a
comparison function from the user and one swap slot equivalent to the size of an
element stored in the buffer. Elements are sorted in a non-decreasing order.
Therefore if a non-increasing list is needed simply reverse the return of
the comparison function for non-equivalent values. */
CCC_Result sort(CCC_Buffer *b, CCC_Type_comparator *fn, void *swap);

/** Compares the Buffer contents as left hand side to the provided sequence of
elements as the right hand side. The sequence type must match the type stored in
the Buffer because that type will be used to iterate through the sequence. */
CCC_Order buforder(CCC_Buffer const *lhs, size_t rhs_count, void const *rhs);

/** Appends the provided range into the buffer. If the range will exceed
capacity of a fixed size buffer, only those elements which fit will be pushed
and a failure will be returned. If resizing is allowed the full range should
be appended unless reallocation fails in which case an error is returned. */
CCC_Result append_range(CCC_Buffer *b, size_t range_count, void const *range);

/** Returns max int between a and b. Ties go to a. */
static inline int
maxint(int a, int b)
{
    return a > b ? a : b;
}

#endif /* BUF_UTIL_H */
