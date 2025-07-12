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

#endif /* BUF_UTIL_H */
