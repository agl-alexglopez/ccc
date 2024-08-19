/* Author: Alexander Lopez
   File: impl_flat_pqueue.h
   ------------------------
   This file exists for a few reasons. It helps clean up the interface for
   users of the non impl header. We can hide the internals of the data
   structure struct by wrapping the implementation in the user facing
   struct. This library does not allocate memory without permission so only
   exposing a pointer to the data structure on the user side will not work,
   as that forces us to allocate memory to store the actual structure on the
   heap, unnacceptable. We also can implement very helpful but ugly macros and
   hide any complexity from the user, wrapping the macro for cleaner use
   in the user facing header.

   The types in any implementation file should not be typedef'd when possible
   to make it harder for the user to use the impl version of the data structure
   by mistake. Unfortunately, users may still have access to these types even
   when only including the user facing header. User facing headers typedef all
   types to further promote opaqueness, because only the provided functions
   should be used. Implementers do not require such obfuscation. */
#ifndef CCC_IMPL_FLAT_PQUEUE_H
#define CCC_IMPL_FLAT_PQUEUE_H

#include "buf.h"

enum ccc_impl_fpq_threeway_cmp
{
    CCC_IMPL_FPQ_LES = -1,
    CCC_IMPL_FPQ_EQL,
    CCC_IMPL_FPQ_GRT,
};

enum ccc_impl_fpq_result
{
    CCC_IMPL_FPQ_OK = CCC_BUF_OK,
    CCC_IMPL_FPQ_FULL = CCC_BUF_FULL,
    CCC_IMPL_FPQ_ERR = CCC_BUF_ERR,
};

typedef enum ccc_impl_fpq_threeway_cmp
ccc_impl_fpq_cmp_fn(void const *, void const *, void *aux);

struct ccc_impl_flat_pqueue
{
    ccc_buf *buf;
    ccc_impl_fpq_cmp_fn *cmp;
    enum ccc_impl_fpq_threeway_cmp order;
    void *aux;
};

void ccc_impl_fpq_bubble_up(struct ccc_impl_flat_pqueue *, uint8_t[], size_t);

/*=======================    Convenience Macros    ======================== */

#define CCC_IMPL_FPQ_INIT(mem_buf, struct_name, fpq_elem_field, cmp_order,     \
                          cmp_fn, aux_data)                                    \
    {                                                                          \
        .impl = {                                                              \
            .buf = (mem_buf),                                                  \
            .cmp = (ccc_impl_fpq_cmp_fn *)(cmp_fn),                            \
            .order = (enum ccc_impl_fpq_threeway_cmp)(cmp_order),              \
            .aux = (aux_data),                                                 \
        },                                                                     \
    }

/* This macro "returns" a value thanks to clang and gcc statement expressions.
   See documentation in the flat pqueue header for usage. The ugly details
   of the macro are hidden here in the impl header. */
#define CCC_IMPL_FPQ_EMPLACE(fpq, struct_name, struct_initializer...)          \
    ({                                                                         \
        enum ccc_impl_fpq_result _impl_macro_result_;                          \
        {                                                                      \
            if (sizeof(struct_name)                                            \
                != ccc_buf_elem_size(                                          \
                    ((struct ccc_impl_flat_pqueue *)(fpq))->buf))              \
            {                                                                  \
                _impl_macro_result_ = CCC_IMPL_FPQ_ERR;                        \
            }                                                                  \
            else                                                               \
            {                                                                  \
                void *_macro_new_ = ccc_buf_alloc(                             \
                    ((struct ccc_impl_flat_pqueue *)(fpq))->buf);              \
                if (!_macro_new_)                                              \
                {                                                              \
                    _impl_macro_result_ = CCC_IMPL_FPQ_FULL;                   \
                }                                                              \
                else                                                           \
                {                                                              \
                    *((struct_name *)_macro_new_)                              \
                        = (struct_name)struct_initializer;                     \
                    if (ccc_buf_size(                                          \
                            ((struct ccc_impl_flat_pqueue *)(fpq))->buf)       \
                        > 1)                                                   \
                    {                                                          \
                        uint8_t _macro_tmp_[ccc_buf_elem_size(                 \
                            ((struct ccc_impl_flat_pqueue *)(fpq))->buf)];     \
                        ccc_impl_fpq_bubble_up(                                \
                            ((struct ccc_impl_flat_pqueue *)(fpq)),            \
                            _macro_tmp_,                                       \
                            ccc_buf_size(                                      \
                                ((struct ccc_impl_flat_pqueue *)(fpq))->buf)   \
                                - 1);                                          \
                    }                                                          \
                    _impl_macro_result_ = CCC_IMPL_FPQ_OK;                     \
                }                                                              \
            }                                                                  \
        };                                                                     \
        _impl_macro_result_;                                                   \
    })

#endif /* CCC_IMPL_FLAT_PQUEUE_H */
