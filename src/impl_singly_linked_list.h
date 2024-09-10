#ifndef CCC_IMPL_SINGLY_LINKED_LIST_H
#define CCC_IMPL_SINGLY_LINKED_LIST_H

#include "types.h"

#include <assert.h>
#include <stddef.h>

typedef struct ccc_sll_elem_
{
    struct ccc_sll_elem_ *n_;
} ccc_sll_elem_;

struct ccc_sll_
{
    struct ccc_sll_elem_ sentinel_;
    size_t sz_;
    size_t elem_sz_;
    size_t sll_elem_offset_;
    ccc_alloc_fn *alloc_;
    void *aux_;
};

#define CCC_IMPL_SLL_INIT(sll_ptr, sll_name, struct_name, sll_elem_field,      \
                          alloc_fn, aux_data)                                  \
    {                                                                          \
        {                                                                      \
            .sentinel_.n_ = &(sll_name).impl_.sentinel_,                       \
            .elem_sz_ = sizeof(struct_name),                                   \
            .sll_elem_offset_ = offsetof(struct_name, sll_elem_field),         \
            .sz_ = 0, .alloc_ = (alloc_fn), .aux_ = (aux_data)                 \
        }                                                                      \
    }

void ccc_impl_sll_push_front(struct ccc_sll_ *, struct ccc_sll_elem_ *);
struct ccc_sll_elem_ *ccc_sll_elem__in(struct ccc_sll_ const *,
                                       void const *user_struct);

#define CCC_IMPL_LIST_EMPLACE_FRONT(list_ptr, struct_initializer...)           \
    ({                                                                         \
        typeof(struct_initializer) *sll_res_;                                  \
        struct ccc_sll_ *sll_ = &(list_ptr)->impl_;                            \
        assert(sizeof(*sll_res_) == sll_->elem_sz_);                           \
        if (!sll_->alloc_)                                                     \
        {                                                                      \
            sll_res_ = NULL;                                                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            sll_res_ = sll_->alloc_(NULL, sll_->elem_sz_);                     \
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
