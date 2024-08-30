#include "list.h"
#include "impl_list.h"
#include <stdint.h>
#include <string.h>

static void *struct_base(struct ccc_impl_list const *,
                         struct ccc_impl_list_elem const *);

void *
ccc_list_push_front(ccc_list *l, ccc_list_elem *struct_handle)
{
    if (l->impl.fn)
    {
        void *node = l->impl.fn(NULL, l->impl.elem_sz);
        if (!node)
        {
            return NULL;
        }
        memcpy(node, struct_base(&l->impl, &struct_handle->impl),
               l->impl.elem_sz);
    }
    ccc_impl_list_push_front(&l->impl, &struct_handle->impl);
    return struct_base(&l->impl, l->impl.sentinel.n);
}

void *
ccc_list_push_back(ccc_list *l, ccc_list_elem *struct_handle)
{
    if (l->impl.fn)
    {
        void *node = l->impl.fn(NULL, l->impl.elem_sz);
        if (!node)
        {
            return NULL;
        }
        memcpy(node, struct_base(&l->impl, &struct_handle->impl),
               l->impl.elem_sz);
    }
    ccc_impl_list_push_back(&l->impl, &struct_handle->impl);
    return struct_base(&l->impl, l->impl.sentinel.p);
}

void *
ccc_list_front(ccc_list const *l)
{
    if (!l->impl.sz)
    {
        return NULL;
    }
    return struct_base(&l->impl, l->impl.sentinel.n);
}

void *
ccc_list_back(ccc_list const *l)
{
    if (!l->impl.sz)
    {
        return NULL;
    }
    return struct_base(&l->impl, l->impl.sentinel.p);
}

void
ccc_list_pop_front(ccc_list *l)
{
    if (!l->impl.sz)
    {
        return;
    }
    struct ccc_impl_list_elem *remove = l->impl.sentinel.n;
    remove->n->p = &l->impl.sentinel;
    l->impl.sentinel.n = remove->n;
    if (l->impl.fn)
    {
        l->impl.fn(struct_base(&l->impl, remove), 0);
    }
    --l->impl.sz;
}

void
ccc_list_pop_back(ccc_list *l)
{
    if (!l->impl.sz)
    {
        return;
    }
    struct ccc_impl_list_elem *remove = l->impl.sentinel.p;
    remove->p->n = &l->impl.sentinel;
    l->impl.sentinel.p = remove->p;
    if (l->impl.fn)
    {
        l->impl.fn(struct_base(&l->impl, remove), 0);
    }
    --l->impl.sz;
}

void
ccc_impl_list_push_back(struct ccc_impl_list *const l,
                        struct ccc_impl_list_elem *const e)
{
    e->n = &l->sentinel;
    e->p = l->sentinel.p;
    l->sentinel.p->n = e;
    l->sentinel.p = e;
    ++l->sz;
}

void
ccc_impl_list_push_front(struct ccc_impl_list *const l,
                         struct ccc_impl_list_elem *const e)
{
    e->p = &l->sentinel;
    e->n = l->sentinel.n;
    l->sentinel.n->p = e;
    l->sentinel.n = e;
    ++l->sz;
}

ccc_list_elem *
ccc_list_head(ccc_list const *const l)
{
    if (!l || !l->impl.sz)
    {
        return NULL;
    }
    return (ccc_list_elem *)l->impl.sentinel.n;
}

ccc_list_elem *
ccc_list_tail(ccc_list const *const l)
{
    if (!l || !l->impl.sz)
    {
        return NULL;
    }
    return (ccc_list_elem *)l->impl.sentinel.p;
}

void
ccc_list_splice(ccc_list_elem *pos, ccc_list_elem *const to_cut)
{
    if (!to_cut || !pos || &pos->impl == &to_cut->impl
        || to_cut->impl.n == &pos->impl)
    {
        return;
    }
    to_cut->impl.n->p = to_cut->impl.p;
    to_cut->impl.p->n = to_cut->impl.n;

    to_cut->impl.p = pos->impl.p;
    to_cut->impl.n = &pos->impl;
    pos->impl.p->n = &to_cut->impl;
    pos->impl.p = &to_cut->impl;
}

void *
ccc_list_begin(ccc_list const *const l)
{
    if (!l || l->impl.sentinel.n == &l->impl.sentinel)
    {
        return NULL;
    }
    return struct_base(&l->impl, l->impl.sentinel.n);
}

void *
ccc_list_next(ccc_list const *const l, ccc_list_elem const *e)
{
    if (!e || e->impl.n == &l->impl.sentinel)
    {
        return NULL;
    }
    return struct_base(&l->impl, e->impl.n);
}

size_t
ccc_list_size(ccc_list const *const l)
{
    return l->impl.sz;
}

bool
ccc_list_empty(ccc_list const *const l)
{
    return !l->impl.sz;
}

void
ccc_list_clear(ccc_list *const l, ccc_destructor_fn *fn)
{
    while (!ccc_list_empty(l))
    {
        void *const node = ccc_list_front(l);
        if (fn)
        {
            fn(node);
        }
        ccc_list_pop_front(l);
    }
}

bool
ccc_list_validate(ccc_list const *const l)
{
    size_t size = 0;
    for (struct ccc_impl_list_elem *e = l->impl.sentinel.n;
         e != &l->impl.sentinel; e = e->n, ++size)
    {
        if (!e || !e->n || !e->p)
        {
            return false;
        }
        if (size > l->impl.sz)
        {
            return false;
        }
    }
    return size == l->impl.sz;
}

struct ccc_impl_list_elem *
ccc_impl_list_elem_in(struct ccc_impl_list const *const l,
                      void const *const user_struct)
{
    return (struct ccc_impl_list_elem *)((uint8_t *)user_struct
                                         + l->list_elem_offset);
}

static inline void *
struct_base(struct ccc_impl_list const *const l,
            struct ccc_impl_list_elem const *const e)
{
    return ((uint8_t *)&e->n) - l->list_elem_offset;
}
