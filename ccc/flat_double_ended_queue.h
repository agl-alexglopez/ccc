#ifndef CCC_FLAT_DOUBLE_ENDED_QUEUE_H
#define CCC_FLAT_DOUBLE_ENDED_QUEUE_H

#include "impl_flat_double_ended_queue.h"
#include "types.h"

typedef struct
{
    struct ccc_fdeq_ impl_;
} ccc_flat_double_ended_queue;

#define CCC_FDEQ_INIT(mem_ptr, capacity, type_name, alloc_fn)                  \
    (ccc_flat_double_ended_queue)                                              \
        CCC_IMPL_FDEQ_INIT(mem_ptr, capacity, type_name, alloc_fn)

#define CCC_FDEQ_EMPLACE(fq_ptr, value...) CCC_IMPL_FDEQ_EMPLACE(fq_ptr, value)

void *ccc_fdeq_push_back(ccc_flat_double_ended_queue *fq, void const *elem);
void *ccc_fdeq_push_front(ccc_flat_double_ended_queue *fq, void const *elem);
void ccc_fdeq_pop_front(ccc_flat_double_ended_queue *fq);
void ccc_fdeq_pop_back(ccc_flat_double_ended_queue *fq);

void *ccc_fdeq_front(ccc_flat_double_ended_queue const *fq);
void *ccc_fdeq_back(ccc_flat_double_ended_queue const *fq);

bool ccc_fdeq_empty(ccc_flat_double_ended_queue const *fq);
size_t ccc_fdeq_size(ccc_flat_double_ended_queue const *fq);
void ccc_fdeq_clear(ccc_flat_double_ended_queue *fq,
                    ccc_destructor_fn *destructor);
void ccc_fdeq_clear_and_free(ccc_flat_double_ended_queue *fq,
                             ccc_destructor_fn *destructor);

void *ccc_fdeq_begin(ccc_flat_double_ended_queue const *fq);
void *ccc_fdeq_rbegin(ccc_flat_double_ended_queue const *fq);

void *ccc_fdeq_next(ccc_flat_double_ended_queue const *fq,
                    void const *iter_ptr);
void *ccc_fdeq_rnext(ccc_flat_double_ended_queue const *fq,
                     void const *iter_ptr);

void *ccc_fdeq_end(ccc_flat_double_ended_queue const *fq);
void *ccc_fdeq_rend(ccc_flat_double_ended_queue const *fq);

#endif /* CCC_FLAT_DOUBLE_ENDED_QUEUE_H */
