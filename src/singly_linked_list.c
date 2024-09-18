#include "singly_linked_list.h"
#include "impl_singly_linked_list.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static void *struct_base(struct ccc_sll_ const *, struct ccc_sll_elem_ const *);

void *
ccc_sll_push_front(ccc_singly_linked_list *const sll,
                   ccc_sll_elem *const struct_handle)
{
    if (sll->impl_.alloc_)
    {
        void *node = sll->impl_.alloc_(NULL, sll->impl_.elem_sz_);
        if (!node)
        {
            return NULL;
        }
        memcpy(node, struct_base(&sll->impl_, &struct_handle->impl_),
               sll->impl_.elem_sz_);
    }
    ccc_impl_sll_push_front(&sll->impl_, &struct_handle->impl_);
    return struct_base(&sll->impl_, sll->impl_.sentinel_.n_);
}

void *
ccc_sll_front(ccc_singly_linked_list const *const sll)
{
    if (!sll || sll->impl_.sentinel_.n_ == &sll->impl_.sentinel_)
    {
        return NULL;
    }
    return struct_base(&sll->impl_, sll->impl_.sentinel_.n_);
}

ccc_sll_elem *
ccc_sll_head(ccc_singly_linked_list const *const sll)
{
    return (ccc_sll_elem *)sll->impl_.sentinel_.n_;
}

void
ccc_sll_pop_front(ccc_singly_linked_list *const sll)
{
    if (!sll->impl_.sz_)
    {
        return;
    }
    struct ccc_sll_elem_ *remove = sll->impl_.sentinel_.n_;
    sll->impl_.sentinel_.n_ = remove->n_;
    if (sll->impl_.alloc_)
    {
        sll->impl_.alloc_(struct_base(&sll->impl_, remove), 0);
    }
    --sll->impl_.sz_;
}

void
ccc_impl_sll_push_front(struct ccc_sll_ *const sll,
                        struct ccc_sll_elem_ *const e)
{
    e->n_ = sll->sentinel_.n_;
    sll->sentinel_.n_ = e;
    ++sll->sz_;
}

inline void *
ccc_sll_begin(ccc_singly_linked_list const *const sll)
{
    if (!sll || sll->impl_.sentinel_.n_ == &sll->impl_.sentinel_)
    {
        return NULL;
    }
    return struct_base(&sll->impl_, sll->impl_.sentinel_.n_);
}

inline void *
ccc_sll_end([[maybe_unused]] ccc_singly_linked_list const *const sll)
{
    return NULL;
}

inline void *
ccc_sll_next(ccc_singly_linked_list const *const sll,
             ccc_sll_elem const *const iter_handle)
{
    if (!iter_handle || iter_handle->impl_.n_ == &sll->impl_.sentinel_)
    {
        return NULL;
    }
    return struct_base(&sll->impl_, iter_handle->impl_.n_);
}

bool
ccc_sll_validate(ccc_singly_linked_list const *const sll)
{
    size_t size = 0;
    for (struct ccc_sll_elem_ *e = sll->impl_.sentinel_.n_;
         e != &sll->impl_.sentinel_; e = e->n_, ++size)
    {
        if (!e || !e->n_)
        {
            return false;
        }
        if (size > sll->impl_.sz_)
        {
            return false;
        }
    }
    return size == sll->impl_.sz_;
}

size_t
ccc_sll_size(ccc_singly_linked_list const *const sll)
{
    return sll->impl_.sz_;
}

bool
ccc_sll_empty(ccc_singly_linked_list const *const sll)
{
    return !sll->impl_.sz_;
}

struct ccc_sll_elem_ *
ccc_sll_elem_in(struct ccc_sll_ const *const sll, void const *const user_struct)
{
    return (struct ccc_sll_elem_ *)((uint8_t *)user_struct
                                    + sll->sll_elem_offset_);
}

static inline void *
struct_base(struct ccc_sll_ const *const l, struct ccc_sll_elem_ const *const e)
{
    return ((uint8_t *)&e->n_) - l->sll_elem_offset_;
}
