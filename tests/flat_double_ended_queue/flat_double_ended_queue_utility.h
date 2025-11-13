#ifndef FDEQ_UTIL_H
#define FDEQ_UTIL_H

#include <stddef.h>

#include "checkers.h"
#include "flat_double_ended_queue.h"

enum Check_result create_queue(CCC_Flat_double_ended_queue *q, size_t n,
                               int const vals[]);

enum Check_result check_order(CCC_Flat_double_ended_queue const *q, size_t n,
                              int const order[]);

#endif /* FDEQ_UTIL_H */
