#ifndef CCC_IMPL_SINGLY_LINKED_LIST_H
#define CCC_IMPL_SINGLY_LINKED_LIST_H

#include "types.h"

#include <assert.h>
#include <stddef.h>

typedef struct ccc_sll_elem_
{
    struct ccc_sll_elem_ *n;
} ccc_sll_elem_;

struct ccc_sll_
{
    struct ccc_sll_elem_ sentinel;
    size_t sz;
    size_t elem_sz;
    size_t sll_elem_offset;
    ccc_alloc_fn *alloc;
    void *aux;
};

#define CCC_IMPL_SLL_INIT(sll_ptr, sll_name, struct_name, sll_elem_field,      \
                          alloc_fn, aux_data)                                  \
    {                                                                          \
        {                                                                      \
            .sentinel.n = &(sll_name).impl.sentinel,                           \
            .elem_sz = sizeof(struct_name),                                    \
            .sll_elem_offset = offsetof(struct_name, sll_elem_field), .sz = 0, \
            .alloc = (alloc_fn), .aux = (aux_data)                             \
        }                                                                      \
    }

void ccc_impl_sll_push_front(struct ccc_sll_ *, struct ccc_sll_elem_ *);
struct ccc_sll_elem_ *ccc_sll_elem__in(struct ccc_sll_ const *,
                                       void const *user_struct);

#define CCC_IMPL_LIST_EMPLACE_FRONT(list_ptr, struct_initializer...)           \
    ({                                                                         \
        typeof(struct_initializer) *sll_res_;                                  \
        struct ccc_sll_ *sll_ = &(list_ptr)->impl;                             \
        assert(sizeof(*sll_res_) == sll_->elem_sz);                            \
        if (!sll_->alloc)                                                      \
        {                                                                      \
            sll_res_ = NULL;                                                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            sll_res_ = sll_->alloc(NULL, sll_->elem_sz);                       \
            if (sll_res_)                                                      \
            {                                                                  \
                *sll_res_ = struct_initializer;                                \
                ccc_impl_sll_push_front(sll_,                                  \
                                        ccc_sll_elem__in(sll_, sll_res_));     \
            }                                                                  \
        }                                                                      \
        sll_res_;                                                              \
    })
#endif /* CCC_IMPL_SINGLY_LINKED_LIST_H */
