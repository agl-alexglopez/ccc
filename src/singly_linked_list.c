#include "singly_linked_list.h"

#include <string.h>

static void *struct_base(struct ccc_impl_singly_linked_list const *,
                         struct ccc_impl_sll_elem const *);

void *
ccc_sll_push_front(ccc_singly_linked_list *fl, ccc_sll_elem *struct_handle)
{
    if (fl->impl.fn)
    {
        void *node = fl->impl.fn(NULL, fl->impl.elem_sz);
        if (!node)
        {
            return NULL;
        }

        memcpy(node, struct_base(&fl->impl, &struct_handle->impl),
               fl->impl.elem_sz);
    }
    ccc_impl_sll_push_front(&fl->impl, &struct_handle->impl);
    return struct_base(&fl->impl, fl->impl.sentinel.n);
}

void *
ccc_sll_front(ccc_singly_linked_list const *fl)
{
    if (!fl || fl->impl.sentinel.n == &fl->impl.sentinel)
    {
        return NULL;
    }
    return struct_base(&fl->impl, fl->impl.sentinel.n);
}

void
ccc_sll_pop_front(ccc_singly_linked_list *fl)
{
    if (!fl->impl.sz)
    {
        return;
    }
    struct ccc_impl_sll_elem *remove = fl->impl.sentinel.n;
    fl->impl.sentinel.n = remove->n;
    if (fl->impl.fn)
    {
        fl->impl.fn(struct_base(&fl->impl, remove), 0);
    }
    --fl->impl.sz;
}

void
ccc_impl_sll_push_front(struct ccc_impl_singly_linked_list *const fl,
                        struct ccc_impl_sll_elem *const e)
{
    e->n = fl->sentinel.n;
    fl->sentinel.n = e;
    ++fl->sz;
}

bool
ccc_sll_validate(ccc_singly_linked_list const *const fl)
{
    size_t size = 0;
    for (struct ccc_impl_sll_elem *e = fl->impl.sentinel.n;
         e != &fl->impl.sentinel; e = e->n, ++size)
    {
        if (!e || !e->n)
        {
            return false;
        }
        if (size > fl->impl.sz)
        {
            return false;
        }
    }
    return size == fl->impl.sz;
}

struct ccc_impl_sll_elem *
ccc_impl_sll_elem_in(struct ccc_impl_singly_linked_list const *const fl,
                     void const *const user_struct)
{
    return (struct ccc_impl_sll_elem *)((uint8_t *)user_struct
                                        + fl->sll_elem_offset);
}

static inline void *
struct_base(struct ccc_impl_singly_linked_list const *const l,
            struct ccc_impl_sll_elem const *const e)
{
    return ((uint8_t *)&e->n) - l->sll_elem_offset;
}
