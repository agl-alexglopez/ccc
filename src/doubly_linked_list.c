#include "doubly_linked_list.h"
#include "impl_doubly_linked_list.h"
#include <stdint.h>
#include <string.h>

static void *struct_base(struct ccc_impl_doubly_linked_list const *,
                         struct ccc_impl_dll_elem const *);

void *
ccc_dll_push_front(ccc_doubly_linked_list *l, ccc_dll_elem *struct_handle)
{
    if (l->impl.alloc)
    {
        void *node = l->impl.alloc(NULL, l->impl.elem_sz);
        if (!node)
        {
            return NULL;
        }
        memcpy(node, struct_base(&l->impl, &struct_handle->impl),
               l->impl.elem_sz);
    }
    ccc_impl_dll_push_front(&l->impl, &struct_handle->impl);
    return struct_base(&l->impl, l->impl.sentinel.n);
}

void *
ccc_dll_push_back(ccc_doubly_linked_list *l, ccc_dll_elem *struct_handle)
{
    if (l->impl.alloc)
    {
        void *node = l->impl.alloc(NULL, l->impl.elem_sz);
        if (!node)
        {
            return NULL;
        }
        memcpy(node, struct_base(&l->impl, &struct_handle->impl),
               l->impl.elem_sz);
    }
    ccc_impl_dll_push_back(&l->impl, &struct_handle->impl);
    return struct_base(&l->impl, l->impl.sentinel.p);
}

void *
ccc_dll_front(ccc_doubly_linked_list const *l)
{
    if (!l->impl.sz)
    {
        return NULL;
    }
    return struct_base(&l->impl, l->impl.sentinel.n);
}

void *
ccc_dll_back(ccc_doubly_linked_list const *l)
{
    if (!l->impl.sz)
    {
        return NULL;
    }
    return struct_base(&l->impl, l->impl.sentinel.p);
}

void
ccc_dll_pop_front(ccc_doubly_linked_list *l)
{
    if (!l->impl.sz)
    {
        return;
    }
    struct ccc_impl_dll_elem *remove = l->impl.sentinel.n;
    remove->n->p = &l->impl.sentinel;
    l->impl.sentinel.n = remove->n;
    if (l->impl.alloc)
    {
        l->impl.alloc(struct_base(&l->impl, remove), 0);
    }
    --l->impl.sz;
}

void
ccc_dll_pop_back(ccc_doubly_linked_list *l)
{
    if (!l->impl.sz)
    {
        return;
    }
    struct ccc_impl_dll_elem *remove = l->impl.sentinel.p;
    remove->p->n = &l->impl.sentinel;
    l->impl.sentinel.p = remove->p;
    if (l->impl.alloc)
    {
        l->impl.alloc(struct_base(&l->impl, remove), 0);
    }
    --l->impl.sz;
}

void
ccc_impl_dll_push_back(struct ccc_impl_doubly_linked_list *const l,
                       struct ccc_impl_dll_elem *const e)
{
    e->n = &l->sentinel;
    e->p = l->sentinel.p;
    l->sentinel.p->n = e;
    l->sentinel.p = e;
    ++l->sz;
}

void
ccc_impl_dll_push_front(struct ccc_impl_doubly_linked_list *const l,
                        struct ccc_impl_dll_elem *const e)
{
    e->p = &l->sentinel;
    e->n = l->sentinel.n;
    l->sentinel.n->p = e;
    l->sentinel.n = e;
    ++l->sz;
}

ccc_dll_elem *
ccc_dll_head(ccc_doubly_linked_list const *const l)
{
    return (ccc_dll_elem *)l->impl.sentinel.n;
}

ccc_dll_elem *
ccc_dll_tail(ccc_doubly_linked_list const *const l)
{
    return (ccc_dll_elem *)l->impl.sentinel.p;
}

void
ccc_dll_splice(ccc_dll_elem *pos, ccc_dll_elem *const to_cut)
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

void
ccc_dll_splice_range(ccc_dll_elem *pos, ccc_dll_elem *begin, ccc_dll_elem *end)
{
    if (!begin || !end || !pos || &pos->impl == &begin->impl
        || begin->impl.n == &pos->impl || begin == end)
    {
        return;
    }
    end = (ccc_dll_elem *)end->impl.p;

    end->impl.n->p = begin->impl.p;
    begin->impl.p->n = end->impl.n;

    begin->impl.p = pos->impl.p;
    end->impl.n = &pos->impl;
    pos->impl.p->n = &begin->impl;
    pos->impl.p = &end->impl;
}

void *
ccc_dll_begin(ccc_doubly_linked_list const *const l)
{
    if (!l || l->impl.sentinel.n == &l->impl.sentinel)
    {
        return NULL;
    }
    return struct_base(&l->impl, l->impl.sentinel.n);
}

void *
ccc_dll_next(ccc_doubly_linked_list const *const l, ccc_dll_elem const *e)
{
    if (!e || e->impl.n == &l->impl.sentinel)
    {
        return NULL;
    }
    return struct_base(&l->impl, e->impl.n);
}

size_t
ccc_dll_size(ccc_doubly_linked_list const *const l)
{
    return l->impl.sz;
}

bool
ccc_dll_empty(ccc_doubly_linked_list const *const l)
{
    return !l->impl.sz;
}

void
ccc_dll_clear(ccc_doubly_linked_list *const l, ccc_destructor_fn *fn)
{
    while (!ccc_dll_empty(l))
    {
        void *const node = ccc_dll_front(l);
        if (fn)
        {
            fn(node);
        }
        ccc_dll_pop_front(l);
    }
}

bool
ccc_dll_validate(ccc_doubly_linked_list const *const l)
{
    size_t size = 0;
    for (struct ccc_impl_dll_elem *e = l->impl.sentinel.n;
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

struct ccc_impl_dll_elem *
ccc_impl_dll_elem_in(struct ccc_impl_doubly_linked_list const *const l,
                     void const *const user_struct)
{
    return (struct ccc_impl_dll_elem *)((uint8_t *)user_struct
                                        + l->dll_elem_offset);
}

static inline void *
struct_base(struct ccc_impl_doubly_linked_list const *const l,
            struct ccc_impl_dll_elem const *const e)
{
    return ((uint8_t *)&e->n) - l->dll_elem_offset;
}