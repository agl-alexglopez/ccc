/* Author: Alexander Lopez
   File: impl_flat_pqueue.h
   ------------------------ */
#ifndef CCC_IMPL_FLAT_PRIORITY_QUEUE_H
#define CCC_IMPL_FLAT_PRIORITY_QUEUE_H

#include "../buffer.h"
#include "../types.h"

/** @cond */
#include <assert.h>
#include <stddef.h>
/** @endcond */

/** @private */
struct ccc_fpq_
{
    ccc_buffer buf_;
    ccc_cmp_fn *cmp_;
    ccc_threeway_cmp order_;
};

size_t ccc_impl_fpq_bubble_up(struct ccc_fpq_ *, char[], size_t);
void ccc_impl_fpq_in_place_heapify(struct ccc_fpq_ *, size_t n);
void *ccc_impl_fpq_update_fixup(struct ccc_fpq_ *, void *);

/* NOLINTBEGIN(readability-identifier-naming) */

/*=======================    Convenience Macros    ======================== */

#define ccc_impl_fpq_init(mem_ptr, capacity, cmp_order, alloc_fn, cmp_fn,      \
                          aux_data)                                            \
    {                                                                          \
        .buf_ = ccc_buf_init(mem_ptr, alloc_fn, aux_data, capacity),           \
        .cmp_ = (cmp_fn),                                                      \
        .order_ = (cmp_order),                                                 \
    }

#define ccc_impl_fpq_heapify_init(mem_ptr, capacity, size, cmp_order,          \
                                  alloc_fn, cmp_fn, aux_data)                  \
    (__extension__({                                                           \
        __auto_type fpq_heapify_mem_ = (mem_ptr);                              \
        struct ccc_fpq_ fpq_heapify_res_                                       \
            = ccc_impl_fpq_init(fpq_heapify_mem_, capacity, cmp_order,         \
                                alloc_fn, cmp_fn, aux_data);                   \
        ccc_impl_fpq_in_place_heapify(&fpq_heapify_res_, (size));              \
        fpq_heapify_res_;                                                      \
    }))

/* This macro "returns" a value thanks to clang and gcc statement expressions.
   See documentation in the flat pqueue header for usage. The ugly details
   of the macro are hidden here in the impl header. */
#define ccc_impl_fpq_emplace(fpq, type_initializer...)                         \
    (__extension__({                                                           \
        typeof(type_initializer) *fpq_res_;                                    \
        struct ccc_fpq_ *fpq_ = (fpq);                                         \
        assert(sizeof(*fpq_res_) == ccc_buf_elem_size(&fpq_->buf_));           \
        fpq_res_ = ccc_buf_alloc_back(&fpq_->buf_);                            \
        if (ccc_buf_size(&fpq_->buf_) == ccc_buf_capacity(&fpq_->buf_))        \
        {                                                                      \
            fpq_res_ = NULL;                                                   \
            ccc_result const extra_space_ = ccc_buf_alloc(                     \
                &fpq_->buf_, ccc_buf_capacity(&fpq_->buf_) * 2,                \
                fpq_->buf_.alloc_);                                            \
            if (extra_space_ == CCC_OK)                                        \
            {                                                                  \
                fpq_res_ = ccc_buf_back(&fpq_->buf_);                          \
            }                                                                  \
        }                                                                      \
        if (fpq_res_)                                                          \
        {                                                                      \
            *fpq_res_ = type_initializer;                                      \
            if (ccc_buf_size(&fpq_->buf_) > 1)                                 \
            {                                                                  \
                void *fpq_tmp_                                                 \
                    = ccc_buf_at(&fpq_->buf_, ccc_buf_size(&fpq_->buf_));      \
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
    }))

/* Only one update fn is needed because there is no advantage to updates if
   it is known they are min/max increase/decrease etc. */
#define ccc_impl_fpq_update_w(fpq_ptr, T_ptr, update_closure_over_T...)        \
    (__extension__({                                                           \
        struct ccc_fpq_ *const fpq_ = (fpq_ptr);                               \
        void *fpq_update_res_ = NULL;                                          \
        void *const fpq_t_ptr_ = (T_ptr);                                      \
        if (fpq_ && fpq_t_ptr_ && !ccc_buf_is_empty(&fpq_->buf_))              \
        {                                                                      \
            {update_closure_over_T} fpq_update_res_                            \
                = ccc_impl_fpq_update_fixup(fpq_, fpq_t_ptr_);                 \
        }                                                                      \
        fpq_update_res_;                                                       \
    }))

#define ccc_impl_fpq_increase_w(fpq_ptr, T_ptr, increase_closure_over_T...)    \
    ccc_impl_fpq_update_w(fpq_ptr, T_ptr, increase_closure_over_T)

#define ccc_impl_fpq_decrease_w(fpq_ptr, T_ptr, decrease_closure_over_T...)    \
    ccc_impl_fpq_update_w(fpq_ptr, T_ptr, decrease_closure_over_T)

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_PRIORITY_QUEUE_H */
