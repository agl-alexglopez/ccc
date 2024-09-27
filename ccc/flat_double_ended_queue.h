#ifndef CCC_FLAT_DOUBLE_ENDED_QUEUE_H
#define CCC_FLAT_DOUBLE_ENDED_QUEUE_H

#include "impl_flat_double_ended_queue.h"
#include "types.h"

typedef struct ccc_fdeq_ ccc_flat_double_ended_queue;

#define ccc_fdeq_init(mem_ptr, capacity, type_name, alloc_fn)                  \
    (ccc_flat_double_ended_queue)                                              \
        ccc_impl_fdeq_init(mem_ptr, capacity, type_name, alloc_fn)

#define ccc_fdeq_emplace(fq_ptr, value...) ccc_impl_fdeq_emplace(fq_ptr, value)

void *ccc_fdeq_push_back(ccc_flat_double_ended_queue *fq, void const *elem);
ccc_result ccc_fdeq_push_back_range(ccc_flat_double_ended_queue *fq, size_t n,
                                    void const *elems);
void *ccc_fdeq_push_front(ccc_flat_double_ended_queue *fq, void const *elem);
ccc_result ccc_fdeq_push_front_range(ccc_flat_double_ended_queue *fq, size_t n,
                                     void const *elems);
void *ccc_fdeq_insert_range(ccc_flat_double_ended_queue *fq, void *pos,
                            size_t n, void const *elems);
void *ccc_fdeq_at(ccc_flat_double_ended_queue const *fq, size_t i);

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

bool ccc_fdeq_validate(ccc_flat_double_ended_queue const *fq);

#ifdef FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
typedef ccc_flat_double_ended_queue flat_double_ended_queue;
#    define fdeq_init(args...) ccc_fdeq_init(args)
#    define fdeq_emplace(args...) ccc_fdeq_emplace(args)
#    define fdeq_push_back(args...) ccc_fdeq_push_back(args)
#    define fdeq_push_back_range(args...) ccc_fdeq_push_back_range(args)
#    define fdeq_push_front(args...) ccc_fdeq_push_front(args)
#    define fdeq_push_front_range(args...) ccc_fdeq_push_front_range(args)
#    define fdeq_insert_range(args...) ccc_fdeq_insert_range(args)
#    define fdeq_pop_front(args...) ccc_fdeq_pop_front(args)
#    define fdeq_pop_back(args...) ccc_fdeq_pop_back(args)
#    define fdeq_front(args...) ccc_fdeq_front(args)
#    define fdeq_back(args...) ccc_fdeq_back(args)
#    define fdeq_empty(args...) ccc_fdeq_empty(args)
#    define fdeq_size(args...) ccc_fdeq_size(args)
#    define fdeq_clear(args...) ccc_fdeq_clear(args)
#    define fdeq_clear_and_free(args...) ccc_fdeq_clear_and_free(args)
#    define fdeq_at(args...) ccc_fdeq_at(args)
#    define fdeq_begin(args...) ccc_fdeq_begin(args)
#    define fdeq_rbegin(args...) ccc_fdeq_rbegin(args)
#    define fdeq_next(args...) ccc_fdeq_next(args)
#    define fdeq_rnext(args...) ccc_fdeq_rnext(args)
#    define fdeq_end(args...) ccc_fdeq_end(args)
#    define fdeq_rend(args...) ccc_fdeq_rend(args)
#endif /* FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC */

#endif /* CCC_FLAT_DOUBLE_ENDED_QUEUE_H */
