#ifndef FDEQ_UTIL_H
#define FDEQ_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "flat_double_ended_queue.h"

enum check_result create_queue(ccc_flat_double_ended_queue *q, size_t n,
                               int const vals[]);

enum check_result check_order(ccc_flat_double_ended_queue const *q, size_t n,
                              int const order[]);

#endif /* FDEQ_UTIL_H */
