#include "singly_linked_list.h"

#include <string.h>

static void *struct_base(struct ccc_sll_ const *, struct ccc_sll_elem_ const *);

void *
ccc_sll_push_front(ccc_singly_linked_list *const sll,
                   ccc_sll_elem *const struct_handle)
{
    if (sll->impl_.alloc)
    {
        void *node = sll->impl_.alloc(NULL, sll->impl_.elem_sz);
        if (!node)
        {
            return NULL;
        }
        memcpy(node, struct_base(&sll->impl_, &struct_handle->impl_),
               sll->impl_.elem_sz);
    }
    ccc_impl_sll_push_front(&sll->impl_, &struct_handle->impl_);
    return struct_base(&sll->impl_, sll->impl_.sentinel.n);
}

void *
ccc_sll_front(ccc_singly_linked_list const *const sll)
{
    if (!sll || sll->impl_.sentinel.n == &sll->impl_.sentinel)
    {
        return NULL;
    }
    return struct_base(&sll->impl_, sll->impl_.sentinel.n);
}

ccc_sll_elem *
ccc_sll_head(ccc_singly_linked_list const *const sll)
{
    return (ccc_sll_elem *)sll->impl_.sentinel.n;
}

void
ccc_sll_pop_front(ccc_singly_linked_list *const sll)
{
    if (!sll->impl_.sz)
    {
        return;
    }
    struct ccc_sll_elem_ *remove = sll->impl_.sentinel.n;
    sll->impl_.sentinel.n = remove->n;
    if (sll->impl_.alloc)
    {
        sll->impl_.alloc(struct_base(&sll->impl_, remove), 0);
    }
    --sll->impl_.sz;
}

void
ccc_impl_sll_push_front(struct ccc_sll_ *const sll,
                        struct ccc_sll_elem_ *const e)
{
    e->n = sll->sentinel.n;
    sll->sentinel.n = e;
    ++sll->sz;
}

inline void *
ccc_sll_begin(ccc_singly_linked_list const *const sll)
{
    if (!sll || sll->impl_.sentinel.n == &sll->impl_.sentinel)
    {
        return NULL;
    }
    return struct_base(&sll->impl_, sll->impl_.sentinel.n);
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
    if (!iter_handle || iter_handle->impl_.n == &sll->impl_.sentinel)
    {
        return NULL;
    }
    return struct_base(&sll->impl_, iter_handle->impl_.n);
}

bool
ccc_sll_validate(ccc_singly_linked_list const *const sll)
{
    size_t size = 0;
    for (struct ccc_sll_elem_ *e = sll->impl_.sentinel.n;
         e != &sll->impl_.sentinel; e = e->n, ++size)
    {
        if (!e || !e->n)
        {
            return false;
        }
        if (size > sll->impl_.sz)
        {
            return false;
        }
    }
    return size == sll->impl_.sz;
}

struct ccc_sll_elem_ *
ccc_sll_elem__in(struct ccc_sll_ const *const sll,
                 void const *const user_struct)
{
    return (struct ccc_sll_elem_ *)((uint8_t *)user_struct
                                    + sll->sll_elem_offset);
}

static inline void *
struct_base(struct ccc_sll_ const *const l, struct ccc_sll_elem_ const *const e)
{
    return ((uint8_t *)&e->n) - l->sll_elem_offset;
}
