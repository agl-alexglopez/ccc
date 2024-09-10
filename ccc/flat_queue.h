#ifndef CCC_FLAT_QUEUE_H
#define CCC_FLAT_QUEUE_H

#include "impl_flat_queue.h"
#include "types.h"

typedef struct
{
    struct ccc_fq_ impl_;
} ccc_flat_queue;

#define CCC_FQ_INIT(mem_ptr, capacity, type_name, alloc_fn)                    \
    (ccc_flat_queue) CCC_IMPL_FQ_INIT(mem_ptr, capacity, type_name, alloc_fn)

#define FQ_EMPLACE(fq_ptr, value...) CCC_IMPL_FQ_EMPLACE(fq_ptr, value)

void *ccc_fq_push(ccc_flat_queue *fq, void *elem);
void ccc_fq_pop(ccc_flat_queue *fq);
void *ccc_fq_front(ccc_flat_queue const *fq);
bool ccc_fq_empty(ccc_flat_queue const *fq);
size_t ccc_fq_size(ccc_flat_queue const *fq);
void ccc_fq_clear(ccc_flat_queue *fq, ccc_destructor_fn *destructor);
void ccc_fq_clear_and_free(ccc_flat_queue *fq, ccc_destructor_fn *destructor);

#endif /* CCC_FLAT_QUEUE_H */
