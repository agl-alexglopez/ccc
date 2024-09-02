#ifndef CCC_IMPL_SINGLY_LINKED_LIST_H
#define CCC_IMPL_SINGLY_LINKED_LIST_H

#include "types.h"

#include <assert.h>
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

void ccc_impl_sll_push_front(struct ccc_impl_singly_linked_list *,
                             struct ccc_impl_sll_elem *);
struct ccc_impl_sll_elem *
ccc_impl_sll_elem_in(struct ccc_impl_singly_linked_list const *,
                     void const *user_struct);

#define CCC_IMPL_LIST_EMPLACE_FRONT(list_ptr, struct_initializer...)           \
    ({                                                                         \
        typeof(struct_initializer) *_sll_res_;                                 \
        struct ccc_impl_singly_linked_list *_sll_ = &(list_ptr)->impl;         \
        assert(sizeof(*_sll_res_) == _sll_->elem_sz);                          \
        if (!_sll_->alloc)                                                     \
        {                                                                      \
            _sll_res_ = NULL;                                                  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            _sll_res_ = _sll_->alloc(NULL, _sll_->elem_sz);                    \
            if (_sll_res_)                                                     \
            {                                                                  \
                *_sll_res_ = struct_initializer;                               \
                ccc_impl_sll_push_front(                                       \
                    _sll_, ccc_impl_sll_elem_in(_sll_, _sll_res_));            \
            }                                                                  \
        }                                                                      \
        _sll_res_;                                                             \
    })
#endif /* CCC_IMPL_SINGLY_LINKED_LIST_H */
