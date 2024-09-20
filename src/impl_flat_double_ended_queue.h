#ifndef CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H
#define CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H

#include "buffer.h"
#include "types.h"

#include <stddef.h>

struct ccc_fdeq_
{
    ccc_buffer buf_;
    size_t front_;
};

enum ccc_impl_fdeq_alloc_slot
{
    CCC_IMPL_FDEQ_FRONT = 0,
    CCC_IMPL_FDEQ_BACK,
};

void *ccc_impl_fdeq_alloc(struct ccc_fdeq_ *, enum ccc_impl_fdeq_alloc_slot);

#define ccc_impl_fdeq_init(mem_ptr, capacity, type_name, alloc_fn)             \
    {                                                                          \
        .buf_ = ccc_buf_init(mem_ptr, type_name, capacity, alloc_fn),          \
        .front_ = 0,                                                           \
    }

#define ccc_impl_fdeq_emplace(fq_ptr, value...)                                \
    ({                                                                         \
        void *const fdeq_emplace_ret_ = ccc_impl_fdeq_alloc((fq_ptr));         \
        if (fdeq_emplace_ret_)                                                 \
        {                                                                      \
            *((typeof(value) *)fdeq_emplace_ret_) = value;                     \
        }                                                                      \
        fdeq_emplace_ret_;                                                     \
    })

#endif /* CCC_IMPL_FLAT_DOUBLE_ENDED_QUEUE_H */
