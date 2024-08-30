#ifndef CCC_IMPL_DOUBLY_LINKED_LIST_H
#define CCC_IMPL_DOUBLY_LINKED_LIST_H

#include "types.h"

#include <stddef.h>

typedef struct ccc_impl_dll_elem
{
    struct ccc_impl_dll_elem *n;
    struct ccc_impl_dll_elem *p;
} ccc_impl_dll_elem;

struct ccc_impl_doubly_linked_list
{
    struct ccc_impl_dll_elem sentinel;
    size_t elem_sz;
    size_t dll_elem_offset;
    size_t sz;
    ccc_realloc_fn *fn;
    void *aux;
};

void ccc_impl_dll_push_back(struct ccc_impl_doubly_linked_list *,
                            struct ccc_impl_dll_elem *);
void ccc_impl_dll_push_front(struct ccc_impl_doubly_linked_list *,
                             struct ccc_impl_dll_elem *);
struct ccc_impl_dll_elem *
ccc_impl_dll_elem_in(struct ccc_impl_doubly_linked_list const *,
                     void const *user_struct);

#define CCC_IMPL_DLL_INIT(dll_ptr, dll_name, struct_name, dll_elem_field,      \
                          realloc_fn, aux_data)                                \
    {                                                                          \
        {                                                                      \
            .sentinel.n = &(dll_name).impl.sentinel,                           \
            .sentinel.p = &(dll_name).impl.sentinel,                           \
            .elem_sz = sizeof(struct_name),                                    \
            .dll_elem_offset = offsetof(struct_name, dll_elem_field), .sz = 0, \
            .fn = (realloc_fn), .aux = (aux_data)                              \
        }                                                                      \
    }

#define CCC_IMPL_DLL_EMPLACE_BACK(dll_ptr, struct_initializer...)              \
    ({                                                                         \
        typeof(struct_initializer) *_res_;                                     \
        struct ccc_impl_doubly_linked_list *_dll_ = &(dll_ptr)->impl;          \
        if (!_dll_->fn || sizeof(*_res_) != _dll_->elem_sz)                    \
        {                                                                      \
            _res_ = NULL;                                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            _res_ = _dll_->fn(NULL, _dll_->elem_sz);                           \
            if (_res_)                                                         \
            {                                                                  \
                *_res_ = (typeof(*_res_))struct_initializer;                   \
                ccc_impl_dll_push_back(_dll_,                                  \
                                       ccc_impl_dll_elem_in(_dll_, _res_));    \
            }                                                                  \
        }                                                                      \
        _res_;                                                                 \
    })

#define CCC_IMPL_DLL_EMPLACE_FRONT(dll_ptr, struct_initializer...)             \
    ({                                                                         \
        typeof(struct_initializer) *_res_;                                     \
        struct ccc_impl_doubly_linked_list *_dll_ = &(dll_ptr)->impl;          \
        if (!_dll_->fn || sizeof(*_res_) != _dll_->elem_sz)                    \
        {                                                                      \
            _res_ = NULL;                                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            _res_ = _dll_->fn(NULL, _dll_->elem_sz);                           \
            if (_res_)                                                         \
            {                                                                  \
                *_res_ = struct_initializer;                                   \
                ccc_impl_dll_push_front(_dll_,                                 \
                                        ccc_impl_dll_elem_in(_dll_, _res_));   \
            }                                                                  \
        }                                                                      \
        _res_;                                                                 \
    })

#endif /* CCC_IMPL_DOUBLY_LINKED_LIST_H */
