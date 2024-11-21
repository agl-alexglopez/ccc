#ifndef CCC_IMPL_PRIORITY_QUEUE_H
#define CCC_IMPL_PRIORITY_QUEUE_H

#include "../types.h"

/** @cond */
#include <stddef.h>
/** @endcond */

/** @private */
struct ccc_pq_elem_
{
    struct ccc_pq_elem_ *left_child_;
    struct ccc_pq_elem_ *next_sibling_;
    struct ccc_pq_elem_ *prev_sibling_;
    struct ccc_pq_elem_ *parent_;
};

/** @private */
struct ccc_pq_
{
    struct ccc_pq_elem_ *root_;
    size_t sz_;
    size_t pq_elem_offset_;
    size_t elem_sz_;
    ccc_alloc_fn *alloc_;
    ccc_cmp_fn *cmp_;
    ccc_threeway_cmp order_;
    void *aux_;
};

/* NOLINTBEGIN(readability-identifier-naming) */

#define ccc_impl_pq_init(struct_name, pq_elem_field, pq_order, alloc_fn,       \
                         cmp_fn, aux_data)                                     \
    {                                                                          \
        .root_ = NULL,                                                         \
        .sz_ = 0,                                                              \
        .pq_elem_offset_ = offsetof(struct_name, pq_elem_field),               \
        .elem_sz_ = sizeof(struct_name),                                       \
        .alloc_ = (alloc_fn),                                                  \
        .cmp_ = (cmp_fn),                                                      \
        .order_ = (pq_order),                                                  \
        .aux_ = (aux_data),                                                    \
    }

void ccc_impl_pq_push(struct ccc_pq_ *, struct ccc_pq_elem_ *);
struct ccc_pq_elem_ *ccc_impl_pq_elem_in(struct ccc_pq_ const *, void const *);
void ccc_impl_pq_update_fixup(struct ccc_pq_ *, struct ccc_pq_elem_ *);
void ccc_impl_pq_increase_fixup(struct ccc_pq_ *, struct ccc_pq_elem_ *);
void ccc_impl_pq_decrease_fixup(struct ccc_pq_ *, struct ccc_pq_elem_ *);

#define ccc_impl_pq_emplace(pq_ptr, lazy_value...)                             \
    (__extension__({                                                           \
        typeof(lazy_value) *pq_res_ = NULL;                                    \
        struct ccc_pq_ *pq_ = (pq_ptr);                                        \
        if (pq_)                                                               \
        {                                                                      \
            assert(sizeof(*pq_res_) == pq_->elem_sz_);                         \
            if (!pq_->alloc_)                                                  \
            {                                                                  \
                pq_res_ = NULL;                                                \
            }                                                                  \
            else                                                               \
            {                                                                  \
                pq_res_ = pq_->alloc_(NULL, pq_->elem_sz_, pq_->aux_);         \
                if (pq_res_)                                                   \
                {                                                              \
                    *pq_res_ = lazy_value;                                     \
                    ccc_impl_pq_push(pq_, ccc_impl_pq_elem_in(pq_, pq_res_));  \
                }                                                              \
            }                                                                  \
        }                                                                      \
        pq_res_;                                                               \
    }))

#define ccc_impl_pq_update_w(pq_ptr, pq_elem_ptr, update_closure_over_T...)    \
    (__extension__({                                                           \
        struct ccc_pq_ *const pq_ = (pq_ptr);                                  \
        bool pq_update_res_ = false;                                           \
        struct ccc_pq_elem_ *const pq_elem_ptr_ = (pq_elem_ptr);               \
        if (pq_ && pq_elem_ptr_ && pq_elem_ptr_->next_sibling_                 \
            && pq_elem_ptr_->prev_sibling_)                                    \
        {                                                                      \
            pq_update_res_ = true;                                             \
            {update_closure_over_T} ccc_impl_pq_update_fixup(pq_,              \
                                                             pq_elem_ptr_);    \
        }                                                                      \
        pq_update_res_;                                                        \
    }))

#define ccc_impl_pq_increase_w(pq_ptr, pq_elem_ptr,                            \
                               increase_closure_over_T...)                     \
    (__extension__({                                                           \
        struct ccc_pq_ *const pq_ = (pq_ptr);                                  \
        bool pq_increase_res_ = false;                                         \
        struct ccc_pq_elem_ *const pq_elem_ptr_ = (pq_elem_ptr);               \
        if (pq_ && pq_elem_ptr_ && pq_elem_ptr_->next_sibling_                 \
            && pq_elem_ptr_->prev_sibling_)                                    \
        {                                                                      \
            pq_increase_res_ = true;                                           \
            {increase_closure_over_T} ccc_impl_pq_increase_fixup(              \
                pq_, pq_elem_ptr_);                                            \
        }                                                                      \
        pq_increase_res_;                                                      \
    }))

#define ccc_impl_pq_decrease_w(pq_ptr, pq_elem_ptr,                            \
                               decrease_closure_over_T...)                     \
    (__extension__({                                                           \
        struct ccc_pq_ *const pq_ = (pq_ptr);                                  \
        bool pq_decrease_res_ = false;                                         \
        struct ccc_pq_elem_ *const pq_elem_ptr_ = (pq_elem_ptr);               \
        if (pq_ && pq_elem_ptr_ && pq_elem_ptr_->next_sibling_                 \
            && pq_elem_ptr_->prev_sibling_)                                    \
        {                                                                      \
            pq_decrease_res_ = true;                                           \
            {decrease_closure_over_T} ccc_impl_pq_decrease_fixup(              \
                pq_, pq_elem_ptr_);                                            \
        }                                                                      \
        pq_decrease_res_;                                                      \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_PRIORITY_QUEUE_H */
