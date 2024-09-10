/* Author: Alexander Lopez
   File: impl_flat_pqueue.h
   ------------------------ */
#ifndef CCC_IMPL_FLAT_PRIORITY_QUEUE_H
#define CCC_IMPL_FLAT_PRIORITY_QUEUE_H

#include "buffer.h"
#include "types.h"

#include <assert.h>

struct ccc_fpq_
{
    ccc_buffer buf_;
    ccc_cmp_fn *cmp_;
    ccc_threeway_cmp order_;
    void *aux_;
};

size_t ccc_impl_fpq_bubble_up(struct ccc_fpq_ *, uint8_t[], size_t);

/*=======================    Convenience Macros    ======================== */

#define CCC_IMPL_FPQ_INIT(mem_ptr, capacity, type_name, cmp_order, alloc_fn,   \
                          cmp_fn, aux_data)                                    \
    {                                                                          \
        {                                                                      \
            .buf_ = CCC_BUF_INIT(mem_ptr, type_name, capacity, alloc_fn),      \
            .cmp_ = (cmp_fn), .order_ = (cmp_order), .aux_ = (aux_data),       \
        }                                                                      \
    }

/* This macro "returns" a value thanks to clang and gcc statement expressions.
   See documentation in the flat pqueue header for usage. The ugly details
   of the macro are hidden here in the impl header. */
#define CCC_IMPL_FPQ_EMPLACE(fpq, type_initializer...)                         \
    ({                                                                         \
        typeof(type_initializer) *fpq_res_;                                    \
        struct ccc_fpq_ *fpq_ = &(fpq)->impl_;                                 \
        assert(sizeof(*fpq_res_) == ccc_buf_elem_size(&fpq_->buf_));           \
        fpq_res_ = ccc_buf_alloc(&fpq_->buf_);                                 \
        if (fpq_res_)                                                          \
        {                                                                      \
            *fpq_res_ = type_initializer;                                      \
            if (ccc_buf_size(&fpq_->buf_) > 1)                                 \
            {                                                                  \
                uint8_t fpq_tmp_[ccc_buf_elem_size(&fpq_->buf_)];              \
                fpq_res_ = ccc_buf_at(                                         \
                    &fpq_->buf_,                                               \
                    ccc_impl_fpq_bubble_up(fpq_, fpq_tmp_,                     \
                                           ccc_buf_size(&fpq_->buf_) - 1));    \
            }                                                                  \
            else                                                               \
            {                                                                  \
                fpq_res_ = ccc_buf_at(&fpq_->buf_, 0);                         \
            }                                                                  \
        }                                                                      \
        fpq_res_;                                                              \
    })

#endif /* CCC_IMPL_FLAT_PRIORITY_QUEUE_H */
