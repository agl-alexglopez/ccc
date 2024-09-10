#ifndef CCC_IMPL_FLAT_QUEUE_H
#define CCC_IMPL_FLAT_QUEUE_H

#include "buffer.h"
#include "types.h"

#include <stddef.h>

struct ccc_fq_
{
    ccc_buffer buf;
    size_t front;
};

void *ccc_impl_fq_alloc(struct ccc_fq_ *);

#define CCC_IMPL_FQ_INIT(mem_ptr, capacity, type_name, alloc_fn)               \
    {                                                                          \
        .impl_ = {                                                             \
            .buf = CCC_BUF_INIT(mem_ptr, type_name, capacity, alloc_fn),       \
            .front = 0,                                                        \
        },                                                                     \
    }

#define CCC_IMPL_FQ_EMPLACE(fq_ptr, value...)                                  \
    ({                                                                         \
        void *fq_emplace_ret_ = ccc_impl_fq_alloc(&(fq_ptr)->impl_);           \
        if (fq_emplace_ret_)                                                   \
        {                                                                      \
            *((typeof(value) *)fq_emplace_ret_) = value;                       \
        }                                                                      \
        fq_emplace_ret_;                                                       \
    })

#endif /* CCC_IMPL_FLAT_QUEUE_H */
