#ifndef CCC_IMPL_SINGLY_LINKED_LIST_H
#define CCC_IMPL_SINGLY_LINKED_LIST_H

#include "types.h"

#include <stddef.h>

typedef struct ccc_impl_sll_elem
{
    struct ccc_impl_sll_elem *n;
} ccc_impl_sll_elem;

struct ccc_impl_singly_linked_list
{
    struct ccc_impl_sll_elem sentinel;
    size_t sz;
    size_t elem_sz;
    size_t sll_elem_offset;
    ccc_realloc_fn *fn;
    void *aux;
};

#define CCC_IMPL_SLL_INIT(sll_ptr, sll_name, struct_name, sll_elem_field,      \
                          realloc_fn, aux_data)                                \
    {                                                                          \
        {                                                                      \
            .sentinel.n = &(sll_name).impl.sentinel,                           \
            .elem_sz = sizeof(struct_name),                                    \
            .sll_elem_offset = offsetof(struct_name, sll_elem_field), .sz = 0, \
            .fn = (realloc_fn), .aux = (aux_data)                              \
        }                                                                      \
    }

void ccc_impl_sll_push_front(struct ccc_impl_singly_linked_list *,
                             struct ccc_impl_sll_elem *);
struct ccc_impl_sll_elem *
ccc_impl_sll_elem_in(struct ccc_impl_singly_linked_list const *,
                     void const *user_struct);

#define CCC_IMPL_LIST_EMPLACE_FRONT(list_ptr, struct_initializer...)           \
    ({                                                                         \
        typeof(struct_initializer) *_res_;                                     \
        struct ccc_impl_singly_linked_list *_sll_ = &(list_ptr)->impl;         \
        if (!_sll_->fn || sizeof(*_res_) != _sll_->elem_sz)                    \
        {                                                                      \
            _res_ = NULL;                                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            _res_ = _sll_->fn(NULL, _sll_->elem_sz);                           \
            if (_res_)                                                         \
            {                                                                  \
                *_res_ = struct_initializer;                                   \
                ccc_impl_sll_push_front(_sll_,                                 \
                                        ccc_impl_sll_elem_in(_sll_, _res_));   \
            }                                                                  \
        }                                                                      \
        _res_;                                                                 \
    })
#endif /* CCC_IMPL_SINGLY_LINKED_LIST_H */
