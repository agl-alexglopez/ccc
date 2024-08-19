#ifndef CCC_IMPL_FLAT_PQUEUE_H
#define CCC_IMPL_FLAT_PQUEUE_H

#include "buf.h"

typedef enum
{
    CCC_IMPL_FPQ_LES = -1,
    CCC_IMPL_FPQ_EQL,
    CCC_IMPL_FPQ_GRT,
} ccc_impl_fpq_threeway_cmp;

typedef enum
{
    CCC_IMPL_FPQ_OK = CCC_BUF_OK,
    CCC_IMPL_FPQ_FULL = CCC_BUF_FULL,
    CCC_IMPL_FPQ_ERR = CCC_BUF_ERR,
} ccc_impl_fpq_result;

typedef ccc_impl_fpq_threeway_cmp ccc_impl_fpq_cmp_fn(void const *,
                                                      void const *, void *aux);

typedef struct
{
    ccc_buf *buf;
    ccc_impl_fpq_cmp_fn *cmp;
    ccc_impl_fpq_threeway_cmp order;
    void *aux;
} ccc_impl_flat_pqueue;

void ccc_impl_fpq_bubble_up(ccc_impl_flat_pqueue *, uint8_t[], size_t);

/*=======================    Convenience Macros    ======================== */

#define CCC_IMPL_FPQ_INIT(mem_buf, struct_name, fpq_elem_field, cmp_order,     \
                          cmp_fn, aux_data)                                    \
    {                                                                          \
        .impl = {                                                              \
            .buf = (mem_buf),                                                  \
            .cmp = (ccc_impl_fpq_cmp_fn *)(cmp_fn),                            \
            .order = (ccc_impl_fpq_threeway_cmp)(cmp_order),                   \
            .aux = (aux_data),                                                 \
        },                                                                     \
    }

/* This macro "returns" a value thanks to clang and gcc statement expressions.
   See documentation in the flat pqueue header for usage. The ugly details
   of the macro are hidden here in the impl header. */
#define CCC_IMPL_FPQ_EMPLACE(fpq, struct_name, struct_initializer...)          \
    ({                                                                         \
        ccc_impl_fpq_result _impl_macro_result_;                               \
        {                                                                      \
            if (sizeof(struct_name)                                            \
                != ccc_buf_elem_size(((ccc_impl_flat_pqueue *)(fpq))->buf))    \
            {                                                                  \
                _impl_macro_result_ = CCC_IMPL_FPQ_ERR;                        \
            }                                                                  \
            else                                                               \
            {                                                                  \
                void *_macro_new_                                              \
                    = ccc_buf_alloc(((ccc_impl_flat_pqueue *)(fpq))->buf);     \
                if (!_macro_new_)                                              \
                {                                                              \
                    _impl_macro_result_ = CCC_IMPL_FPQ_FULL;                   \
                }                                                              \
                else                                                           \
                {                                                              \
                    *((struct_name *)_macro_new_)                              \
                        = (struct_name)struct_initializer;                     \
                    if (ccc_buf_size(((ccc_impl_flat_pqueue *)(fpq))->buf)     \
                        > 1)                                                   \
                    {                                                          \
                        uint8_t _macro_tmp_[ccc_buf_elem_size(                 \
                            ((ccc_impl_flat_pqueue *)(fpq))->buf)];            \
                        ccc_impl_fpq_bubble_up(                                \
                            ((ccc_impl_flat_pqueue *)(fpq)), _macro_tmp_,      \
                            ccc_buf_size(((ccc_impl_flat_pqueue *)(fpq))->buf) \
                                - 1);                                          \
                    }                                                          \
                    _impl_macro_result_ = CCC_IMPL_FPQ_OK;                     \
                }                                                              \
            }                                                                  \
        };                                                                     \
        _impl_macro_result_;                                                   \
    })

#endif /* CCC_IMPL_FLAT_PQUEUE_H */
