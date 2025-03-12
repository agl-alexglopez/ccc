#ifndef CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H
#define CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../buffer.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private */
struct ccc_fdeq_
{
    ccc_buffer buf_;
    ptrdiff_t front_;
};

/*=======================    Private Interface   ============================*/
/** @private */
void *ccc_impl_fdeq_alloc_front(struct ccc_fdeq_ *);
/** @private */
void *ccc_impl_fdeq_alloc_back(struct ccc_fdeq_ *);

/*=======================  Macro Implementations   ==========================*/

/** @private */
#define ccc_impl_fdeq_init(mem_ptr, alloc_fn, aux_data, capacity,              \
                           optional_size...)                                   \
    {                                                                          \
        .buf_                                                                  \
        = ccc_buf_init(mem_ptr, alloc_fn, aux_data, capacity, optional_size),  \
        .front_ = 0,                                                           \
    }

/** @private */
#define ccc_impl_fdeq_emplace_back(fdeq_ptr, value...)                         \
    (__extension__({                                                           \
        __auto_type fdeq_ptr_ = (fdeq_ptr);                                    \
        void *const fdeq_emplace_ret_ = NULL;                                  \
        if (fdeq_ptr_)                                                         \
        {                                                                      \
            void *const fdeq_emplace_ret_                                      \
                = ccc_impl_fdeq_alloc_back(fdeq_ptr_);                         \
            if (fdeq_emplace_ret_)                                             \
            {                                                                  \
                *((typeof(value) *)fdeq_emplace_ret_) = value;                 \
            }                                                                  \
        }                                                                      \
        fdeq_emplace_ret_;                                                     \
    }))

/** @private */
#define ccc_impl_fdeq_emplace_front(fdeq_ptr, value...)                        \
    (__extension__({                                                           \
        __auto_type fdeq_ptr_ = (fdeq_ptr);                                    \
        void *const fdeq_emplace_ret_ = NULL;                                  \
        if (fdeq_ptr_)                                                         \
        {                                                                      \
            void *const fdeq_emplace_ret_                                      \
                = ccc_impl_fdeq_alloc_front(fdeq_ptr_);                        \
            if (fdeq_emplace_ret_)                                             \
            {                                                                  \
                *((typeof(value) *)fdeq_emplace_ret_) = value;                 \
            }                                                                  \
        }                                                                      \
        fdeq_emplace_ret_;                                                     \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H */
