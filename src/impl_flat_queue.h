#ifndef CCC_IMPL_FLAT_QUEUE_H
#define CCC_IMPL_FLAT_QUEUE_H

#include "buffer.h"
#include "types.h"

#include <stddef.h>

struct ccc_impl_flat_queue
{
    ccc_buffer buf;
    size_t front;
};

void *ccc_impl_fq_alloc(struct ccc_impl_flat_queue *);

#define CCC_IMPL_FQ_INIT(mem_ptr, capacity, type_name, alloc_fn)               \
    {                                                                          \
        .impl = {                                                              \
            .buf = CCC_BUF_INIT(mem_ptr, type_name, capacity, alloc_fn),       \
            .front = 0,                                                        \
        },                                                                     \
    }

#define CCC_IMPL_FQ_EMPLACE(fq_ptr, value...)                                  \
    ({                                                                         \
        void *_fq_emplace_ret_ = ccc_impl_fq_alloc(&(fq_ptr)->impl);           \
        if (_fq_emplace_ret_)                                                  \
        {                                                                      \
            *((typeof(value) *)_fq_emplace_ret_) = value;                      \
        }                                                                      \
        _fq_emplace_ret_;                                                      \
    })

#endif /* CCC_IMPL_FLAT_QUEUE_H */