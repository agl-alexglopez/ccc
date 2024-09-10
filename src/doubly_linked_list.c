#include "doubly_linked_list.h"
#include "impl_doubly_linked_list.h"
#include <stdint.h>
#include <string.h>

static void *struct_base(struct ccc_dll_ const *, struct ccc_dll_elem_ const *);

void *
ccc_dll_push_front(ccc_doubly_linked_list *l, ccc_dll_elem *struct_handle)
{
    if (l->impl_.alloc)
    {
        void *node = l->impl_.alloc(NULL, l->impl_.elem_sz);
        if (!node)
        {
            return NULL;
        }
        memcpy(node, struct_base(&l->impl_, &struct_handle->impl_),
               l->impl_.elem_sz);
    }
    ccc_impl_dll_push_front(&l->impl_, &struct_handle->impl_);
    return struct_base(&l->impl_, l->impl_.sentinel.n);
}

void *
ccc_dll_push_back(ccc_doubly_linked_list *l, ccc_dll_elem *struct_handle)
{
    if (l->impl_.alloc)
    {
        void *node = l->impl_.alloc(NULL, l->impl_.elem_sz);
        if (!node)
        {
            return NULL;
        }
        memcpy(node, struct_base(&l->impl_, &struct_handle->impl_),
               l->impl_.elem_sz);
    }
    ccc_impl_dll_push_back(&l->impl_, &struct_handle->impl_);
    return struct_base(&l->impl_, l->impl_.sentinel.p);
}

void *
ccc_dll_front(ccc_doubly_linked_list const *l)
{
    if (!l->impl_.sz)
    {
        return NULL;
    }
    return struct_base(&l->impl_, l->impl_.sentinel.n);
}

void *
ccc_dll_back(ccc_doubly_linked_list const *l)
{
    if (!l->impl_.sz)
    {
        return NULL;
    }
    return struct_base(&l->impl_, l->impl_.sentinel.p);
}

void
ccc_dll_pop_front(ccc_doubly_linked_list *l)
{
    if (!l->impl_.sz)
    {
        return;
    }
    struct ccc_dll_elem_ *remove = l->impl_.sentinel.n;
    remove->n->p = &l->impl_.sentinel;
    l->impl_.sentinel.n = remove->n;
    if (l->impl_.alloc)
    {
        l->impl_.alloc(struct_base(&l->impl_, remove), 0);
    }
    --l->impl_.sz;
}

void
ccc_dll_pop_back(ccc_doubly_linked_list *l)
{
    if (!l->impl_.sz)
    {
        return;
    }
    struct ccc_dll_elem_ *remove = l->impl_.sentinel.p;
    remove->p->n = &l->impl_.sentinel;
    l->impl_.sentinel.p = remove->p;
    if (l->impl_.alloc)
    {
        l->impl_.alloc(struct_base(&l->impl_, remove), 0);
    }
    --l->impl_.sz;
}

void
ccc_impl_dll_push_back(struct ccc_dll_ *const l, struct ccc_dll_elem_ *const e)
{
    e->n = &l->sentinel;
    e->p = l->sentinel.p;
    l->sentinel.p->n = e;
    l->sentinel.p = e;
    ++l->sz;
}

void
ccc_impl_dll_push_front(struct ccc_dll_ *const l, struct ccc_dll_elem_ *const e)
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
    return (ccc_dll_elem *)l->impl_.sentinel.n;
}

ccc_dll_elem *
ccc_dll_tail(ccc_doubly_linked_list const *const l)
{
    return (ccc_dll_elem *)l->impl_.sentinel.p;
}

void
ccc_dll_splice(ccc_dll_elem *pos, ccc_dll_elem *const to_cut)
{
    if (!to_cut || !pos || &pos->impl_ == &to_cut->impl_
        || to_cut->impl_.n == &pos->impl_)
    {
        return;
    }
    to_cut->impl_.n->p = to_cut->impl_.p;
    to_cut->impl_.p->n = to_cut->impl_.n;

    to_cut->impl_.p = pos->impl_.p;
    to_cut->impl_.n = &pos->impl_;
    pos->impl_.p->n = &to_cut->impl_;
    pos->impl_.p = &to_cut->impl_;
}

void
ccc_dll_splice_range(ccc_dll_elem *pos, ccc_dll_elem *begin, ccc_dll_elem *end)
{
    if (!begin || !end || !pos || &pos->impl_ == &begin->impl_
        || begin->impl_.n == &pos->impl_ || begin == end)
    {
        return;
    }
    end = (ccc_dll_elem *)end->impl_.p;

    end->impl_.n->p = begin->impl_.p;
    begin->impl_.p->n = end->impl_.n;

    begin->impl_.p = pos->impl_.p;
    end->impl_.n = &pos->impl_;
    pos->impl_.p->n = &begin->impl_;
    pos->impl_.p = &end->impl_;
}

void *
ccc_dll_begin(ccc_doubly_linked_list const *const l)
{
    if (!l || l->impl_.sentinel.n == &l->impl_.sentinel)
    {
        return NULL;
    }
    return struct_base(&l->impl_, l->impl_.sentinel.n);
}

void *
ccc_dll_rbegin(ccc_doubly_linked_list const *const l)
{
    if (!l || l->impl_.sentinel.p == &l->impl_.sentinel)
    {
        return NULL;
    }
    return struct_base(&l->impl_, l->impl_.sentinel.p);
}

void *
ccc_dll_end([[maybe_unused]] ccc_doubly_linked_list const *const l)
{
    return NULL;
}

void *
ccc_dll_rend([[maybe_unused]] ccc_doubly_linked_list const *const l)
{
    return NULL;
}

void *
ccc_dll_next(ccc_doubly_linked_list const *const l, ccc_dll_elem const *e)
{
    if (!e || e->impl_.n == &l->impl_.sentinel)
    {
        return NULL;
    }
    return struct_base(&l->impl_, e->impl_.n);
}

void *
ccc_dll_rnext(ccc_doubly_linked_list const *const l, ccc_dll_elem const *e)
{
    if (!e || e->impl_.p == &l->impl_.sentinel)
    {
        return NULL;
    }
    return struct_base(&l->impl_, e->impl_.p);
}

size_t
ccc_dll_size(ccc_doubly_linked_list const *const l)
{
    return l->impl_.sz;
}

bool
ccc_dll_empty(ccc_doubly_linked_list const *const l)
{
    return !l->impl_.sz;
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
    for (struct ccc_dll_elem_ *e = l->impl_.sentinel.n; e != &l->impl_.sentinel;
         e = e->n, ++size)
    {
        if (!e || !e->n || !e->p)
        {
            return false;
        }
        if (size > l->impl_.sz)
        {
            return false;
        }
    }
    return size == l->impl_.sz;
}

struct ccc_dll_elem_ *
ccc_dll_elem__in(struct ccc_dll_ const *const l, void const *const user_struct)
{
    return (struct ccc_dll_elem_ *)((uint8_t *)user_struct
                                    + l->dll_elem_offset);
}

static inline void *
struct_base(struct ccc_dll_ const *const l, struct ccc_dll_elem_ const *const e)
{
    return ((uint8_t *)&e->n) - l->dll_elem_offset;
}
