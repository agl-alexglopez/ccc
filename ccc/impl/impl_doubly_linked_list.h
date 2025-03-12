#ifndef CCC_IMPL_DOUBLY_LINKED_LIST_H
#define CCC_IMPL_DOUBLY_LINKED_LIST_H

/** @cond */
#include <assert.h>
#include <stddef.h>
/** @endcond */

#include "../types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private */
typedef struct ccc_dll_elem_
{
    struct ccc_dll_elem_ *n_;
    struct ccc_dll_elem_ *p_;
} ccc_dll_elem_;

/** @private */
struct ccc_dll_
{
    struct ccc_dll_elem_ sentinel_;
    size_t elem_sz_;
    size_t dll_elem_offset_;
    ptrdiff_t sz_;
    ccc_alloc_fn *alloc_;
    ccc_cmp_fn *cmp_;
    void *aux_;
};

/*=======================     Private Interface   ===========================*/

/** @private */
void ccc_impl_dll_push_back(struct ccc_dll_ *, struct ccc_dll_elem_ *);
/** @private */
void ccc_impl_dll_push_front(struct ccc_dll_ *, struct ccc_dll_elem_ *);
/** @private */
struct ccc_dll_elem_ *ccc_impl_dll_elem_in(struct ccc_dll_ const *,
                                           void const *user_struct);

/*=======================     Macro Implementations   =======================*/

/** @private */
#define ccc_impl_dll_init(dll_name, struct_name, dll_elem_field, cmp_fn,       \
                          alloc_fn, aux_data)                                  \
    {                                                                          \
        .sentinel_.n_ = &(dll_name).sentinel_,                                 \
        .sentinel_.p_ = &(dll_name).sentinel_,                                 \
        .elem_sz_ = sizeof(struct_name),                                       \
        .dll_elem_offset_ = offsetof(struct_name, dll_elem_field),             \
        .sz_ = 0,                                                              \
        .alloc_ = (alloc_fn),                                                  \
        .cmp_ = (cmp_fn),                                                      \
        .aux_ = (aux_data),                                                    \
    }

/** @private */
#define ccc_impl_dll_emplace_back(dll_ptr, struct_initializer...)              \
    (__extension__({                                                           \
        typeof(struct_initializer) *dll_res_ = NULL;                           \
        struct ccc_dll_ *dll_ = (dll_ptr);                                     \
        if (dll_)                                                              \
        {                                                                      \
            if (dll_->alloc_)                                                  \
            {                                                                  \
                dll_res_ = dll_->alloc_(NULL, dll_->elem_sz_, dll_->aux_);     \
                if (dll_res_)                                                  \
                {                                                              \
                    *dll_res_ = (typeof(*dll_res_))struct_initializer;         \
                    ccc_impl_dll_push_back(                                    \
                        dll_, ccc_impl_dll_elem_in(dll_, dll_res_));           \
                }                                                              \
            }                                                                  \
        }                                                                      \
        dll_res_;                                                              \
    }))

/** @private */
#define ccc_impl_dll_emplace_front(dll_ptr, struct_initializer...)             \
    (__extension__({                                                           \
        typeof(struct_initializer) *dll_res_;                                  \
        struct ccc_dll_ *dll_ = (dll_ptr);                                     \
        assert(sizeof(*dll_res_) == dll_->elem_sz_);                           \
        if (!dll_->alloc_)                                                     \
        {                                                                      \
            dll_res_ = NULL;                                                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            dll_res_ = dll_->alloc_(NULL, dll_->elem_sz_, dll_->aux_);         \
            if (dll_res_)                                                      \
            {                                                                  \
                *dll_res_ = struct_initializer;                                \
                ccc_impl_dll_push_front(dll_,                                  \
                                        ccc_impl_dll_elem_in(dll_, dll_res_)); \
            }                                                                  \
        }                                                                      \
        dll_res_;                                                              \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_DOUBLY_LINKED_LIST_H */
