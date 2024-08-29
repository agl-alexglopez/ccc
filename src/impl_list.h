#ifndef CCC_IMPL_LIST_H
#define CCC_IMPL_LIST_H

#include "types.h"

#include <stddef.h>

typedef struct ccc_impl_list_elem
{
    struct ccc_impl_list_elem *n;
    struct ccc_impl_list_elem *p;
} ccc_impl_list_elem;

struct ccc_impl_list
{
    struct ccc_impl_list_elem sentinel;
    size_t elem_sz;
    size_t list_elem_offset;
    size_t sz;
    ccc_realloc_fn *fn;
    void *aux;
};

void ccc_impl_l_push_back(struct ccc_impl_list *, struct ccc_impl_list_elem *);
void ccc_impl_l_push_front(struct ccc_impl_list *, struct ccc_impl_list_elem *);
struct ccc_impl_list_elem *ccc_impl_elem_in(struct ccc_impl_list const *,
                                            void const *user_struct);

#define CCC_IMPL_L_INIT(list_ptr, list_name, struct_name, list_elem_field,     \
                        realloc_fn, aux_data)                                  \
    {                                                                          \
        {                                                                      \
            .sentinel.n = &(list_name).impl.sentinel,                          \
            .sentinel.p = &(list_name).impl.sentinel,                          \
            .elem_sz = sizeof(struct_name),                                    \
            .list_elem_offset = offsetof(struct_name, list_elem_field),        \
            .sz = 0, .fn = (realloc_fn), .aux = (aux_data)                     \
        }                                                                      \
    }

#define CCC_IMPL_L_EMPLACE_BACK(list_ptr, struct_initializer...)               \
    ({                                                                         \
        typeof(struct_initializer) *_res_;                                     \
        struct ccc_impl_list *_list_ = &(list_ptr)->impl;                      \
        if (!_list_->fn || sizeof(*_res_) != _list_->elem_sz)                  \
        {                                                                      \
            _res_ = NULL;                                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            _res_ = _list_->fn(NULL, _list_->elem_sz);                         \
            if (_res_)                                                         \
            {                                                                  \
                *_res_ = (typeof(*_res_))struct_initializer;                   \
                ccc_impl_l_push_back(_list_, ccc_impl_elem_in(_list_, _res_)); \
            }                                                                  \
        }                                                                      \
        _res_;                                                                 \
    })

#define CCC_IMPL_L_EMPLACE_FRONT(list_ptr, struct_initializer...)              \
    ({                                                                         \
        typeof(struct_initializer) *_res_;                                     \
        struct ccc_impl_list *_list_ = &(list_ptr)->impl;                      \
        if (!_list_->fn || sizeof(*_res_) != _list_->elem_sz)                  \
        {                                                                      \
            _res_ = NULL;                                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            _res_ = _list_->fn(NULL, _list_->elem_sz);                         \
            if (_res_)                                                         \
            {                                                                  \
                *_res_ = struct_initializer;                                   \
                ccc_impl_l_push_front(_list_,                                  \
                                      ccc_impl_elem_in(_list_, _res_));        \
            }                                                                  \
        }                                                                      \
        _res_;                                                                 \
    })

#endif /* CCC_IMPL_LIST_H */
