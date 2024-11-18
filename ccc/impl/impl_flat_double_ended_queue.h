#ifndef CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H
#define CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H

#include "../buffer.h"

/** @cond */
#include <stddef.h>
/** @endcond */

/** @private */
struct ccc_fdeq_
{
    ccc_buffer buf_;
    size_t front_;
};

void *ccc_impl_fdeq_alloc_front(struct ccc_fdeq_ *);
void *ccc_impl_fdeq_alloc_back(struct ccc_fdeq_ *);

#define ccc_impl_fdeq_init(mem_ptr, alloc_fn, aux_data, capacity,              \
                           optional_size...)                                   \
    {                                                                          \
        .buf_                                                                  \
        = ccc_buf_init(mem_ptr, alloc_fn, aux_data, capacity, optional_size),  \
        .front_ = 0,                                                           \
    }

/* NOLINTBEGIN(readability-identifier-naming) */

#define ccc_impl_fdeq_emplace_back(fdeq_ptr, value...)                         \
    ({                                                                         \
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
    })

#define ccc_impl_fdeq_emplace_front(fdeq_ptr, value...)                        \
    ({                                                                         \
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
    })

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H */
