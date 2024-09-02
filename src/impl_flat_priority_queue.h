/* Author: Alexander Lopez
   File: impl_flat_pqueue.h
   ------------------------ */
#ifndef CCC_IMPL_FLAT_PRIORITY_QUEUE_H
#define CCC_IMPL_FLAT_PRIORITY_QUEUE_H

#include "buffer.h"
#include "types.h"

#include <assert.h>

struct ccc_impl_flat_priority_queue
{
    ccc_buffer buf;
    ccc_cmp_fn *cmp;
    ccc_threeway_cmp order;
    void *aux;
};

size_t ccc_impl_fpq_bubble_up(struct ccc_impl_flat_priority_queue *, uint8_t[],
                              size_t);

/*=======================    Convenience Macros    ======================== */

#define CCC_IMPL_FPQ_INIT(mem_ptr, capacity, type_name, cmp_order, alloc_fn,   \
                          cmp_fn, aux_data)                                    \
    {                                                                          \
        .impl = {                                                              \
            .buf = (ccc_buffer)CCC_BUF_INIT(mem_ptr, type_name, capacity,      \
                                            alloc_fn),                         \
            .cmp = (cmp_fn),                                                   \
            .order = (cmp_order),                                              \
            .aux = (aux_data),                                                 \
        },                                                                     \
    }

/* This macro "returns" a value thanks to clang and gcc statement expressions.
   See documentation in the flat pqueue header for usage. The ugly details
   of the macro are hidden here in the impl header. */
#define CCC_IMPL_FPQ_EMPLACE(fpq, type_initializer...)                         \
    ({                                                                         \
        typeof(type_initializer) *_fpq_res_;                                   \
        struct ccc_impl_flat_priority_queue *_fpq_ = &(fpq)->impl;             \
        assert(sizeof(*_fpq_res_) == ccc_buf_elem_size(&_fpq_->buf));          \
        _fpq_res_ = ccc_buf_alloc(&_fpq_->buf);                                \
        if (_fpq_res_)                                                         \
        {                                                                      \
            *_fpq_res_ = type_initializer;                                     \
            if (ccc_buf_size(&_fpq_->buf) > 1)                                 \
            {                                                                  \
                uint8_t _fpq_tmp_[ccc_buf_elem_size(&_fpq_->buf)];             \
                _fpq_res_ = ccc_buf_at(                                        \
                    &_fpq_->buf,                                               \
                    ccc_impl_fpq_bubble_up(_fpq_, _fpq_tmp_,                   \
                                           ccc_buf_size(&_fpq_->buf) - 1));    \
            }                                                                  \
            else                                                               \
            {                                                                  \
                _fpq_res_ = ccc_buf_at(&_fpq_->buf, 0);                        \
            }                                                                  \
        }                                                                      \
        _fpq_res_;                                                             \
    })

#endif /* CCC_IMPL_FLAT_PRIORITY_QUEUE_H */
