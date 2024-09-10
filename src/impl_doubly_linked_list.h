#ifndef CCC_IMPL_DOUBLY_LINKED_LIST_H
#define CCC_IMPL_DOUBLY_LINKED_LIST_H

#include "types.h"

#include <assert.h>
#include <stddef.h>

typedef struct ccc_dll_elem_
{
    struct ccc_dll_elem_ *n;
    struct ccc_dll_elem_ *p;
} ccc_dll_elem_;

struct ccc_dll_
{
    struct ccc_dll_elem_ sentinel;
    size_t elem_sz;
    size_t dll_elem_offset;
    size_t sz;
    ccc_alloc_fn *alloc;
    ccc_cmp_fn *cmp;
    void *aux;
};

void ccc_impl_dll_push_back(struct ccc_dll_ *, struct ccc_dll_elem_ *);
void ccc_impl_dll_push_front(struct ccc_dll_ *, struct ccc_dll_elem_ *);
struct ccc_dll_elem_ *ccc_dll_elem__in(struct ccc_dll_ const *,
                                       void const *user_struct);

#define CCC_IMPL_DLL_INIT(dll_ptr, dll_name, struct_name, dll_elem_field,      \
                          alloc_fn, cmp_fn, aux_data)                          \
    {                                                                          \
        {                                                                      \
            .sentinel.n = &(dll_name).impl_.sentinel,                          \
            .sentinel.p = &(dll_name).impl_.sentinel,                          \
            .elem_sz = sizeof(struct_name),                                    \
            .dll_elem_offset = offsetof(struct_name, dll_elem_field), .sz = 0, \
            .alloc = (alloc_fn), .cmp = (cmp_fn), .aux = (aux_data),           \
        }                                                                      \
    }

#define CCC_IMPL_DLL_EMPLACE_BACK(dll_ptr, struct_initializer...)              \
    ({                                                                         \
        typeof(struct_initializer) *dll_res_;                                  \
        struct ccc_dll_ *dll_ = &(dll_ptr)->impl_;                             \
        assert(sizeof(*dll_res_) == dll_->elem_sz);                            \
        if (!dll_->alloc)                                                      \
        {                                                                      \
            dll_res_ = NULL;                                                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            dll_res_ = dll_->alloc(NULL, dll_->elem_sz);                       \
            if (dll_res_)                                                      \
            {                                                                  \
                *dll_res_ = (typeof(*dll_res_))struct_initializer;             \
                ccc_impl_dll_push_back(dll_,                                   \
                                       ccc_dll_elem__in(dll_, dll_res_));      \
            }                                                                  \
        }                                                                      \
        dll_res_;                                                              \
    })

#define CCC_IMPL_DLL_EMPLACE_FRONT(dll_ptr, struct_initializer...)             \
    ({                                                                         \
        typeof(struct_initializer) *dll_res_;                                  \
        struct ccc_dll_ *dll_ = &(dll_ptr)->impl_;                             \
        assert(sizeof(*dll_res_) == dll_->elem_sz);                            \
        if (!dll_->alloc)                                                      \
        {                                                                      \
            dll_res_ = NULL;                                                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            dll_res_ = dll_->alloc(NULL, dll_->elem_sz);                       \
            if (dll_res_)                                                      \
            {                                                                  \
                *dll_res_ = struct_initializer;                                \
                ccc_impl_dll_push_front(dll_,                                  \
                                        ccc_dll_elem__in(dll_, dll_res_));     \
            }                                                                  \
        }                                                                      \
        dll_res_;                                                              \
    })

#endif /* CCC_IMPL_DOUBLY_LINKED_LIST_H */
