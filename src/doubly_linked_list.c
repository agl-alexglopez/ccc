#include "doubly_linked_list.h"
#include "impl_doubly_linked_list.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static void *struct_base(struct ccc_dll_ const *, struct ccc_dll_elem_ const *);

void *
ccc_dll_push_front(ccc_doubly_linked_list *l, ccc_dll_elem *struct_handle)
{
    if (l->alloc_)
    {
        void *node = l->alloc_(NULL, l->elem_sz_);
        if (!node)
        {
            return NULL;
        }
        memcpy(node, struct_base(l, struct_handle), l->elem_sz_);
    }
    ccc_impl_dll_push_front(l, struct_handle);
    return struct_base(l, l->sentinel_.n_);
}

void *
ccc_dll_push_back(ccc_doubly_linked_list *l, ccc_dll_elem *struct_handle)
{
    if (l->alloc_)
    {
        void *node = l->alloc_(NULL, l->elem_sz_);
        if (!node)
        {
            return NULL;
        }
        memcpy(node, struct_base(l, struct_handle), l->elem_sz_);
    }
    ccc_impl_dll_push_back(l, struct_handle);
    return struct_base(l, l->sentinel_.p_);
}

void *
ccc_dll_front(ccc_doubly_linked_list const *l)
{
    if (!l->sz_)
    {
        return NULL;
    }
    return struct_base(l, l->sentinel_.n_);
}

void *
ccc_dll_back(ccc_doubly_linked_list const *l)
{
    if (!l->sz_)
    {
        return NULL;
    }
    return struct_base(l, l->sentinel_.p_);
}

void
ccc_dll_pop_front(ccc_doubly_linked_list *l)
{
    if (!l->sz_)
    {
        return;
    }
    struct ccc_dll_elem_ *remove = l->sentinel_.n_;
    remove->n_->p_ = &l->sentinel_;
    l->sentinel_.n_ = remove->n_;
    if (l->alloc_)
    {
        l->alloc_(struct_base(l, remove), 0);
    }
    --l->sz_;
}

void
ccc_dll_pop_back(ccc_doubly_linked_list *l)
{
    if (!l->sz_)
    {
        return;
    }
    struct ccc_dll_elem_ *remove = l->sentinel_.p_;
    remove->p_->n_ = &l->sentinel_;
    l->sentinel_.p_ = remove->p_;
    if (l->alloc_)
    {
        l->alloc_(struct_base(l, remove), 0);
    }
    --l->sz_;
}

void
ccc_impl_dll_push_back(struct ccc_dll_ *const l, struct ccc_dll_elem_ *const e)
{
    e->n_ = &l->sentinel_;
    e->p_ = l->sentinel_.p_;
    l->sentinel_.p_->n_ = e;
    l->sentinel_.p_ = e;
    ++l->sz_;
}

void
ccc_impl_dll_push_front(struct ccc_dll_ *const l, struct ccc_dll_elem_ *const e)
{
    e->p_ = &l->sentinel_;
    e->n_ = l->sentinel_.n_;
    l->sentinel_.n_->p_ = e;
    l->sentinel_.n_ = e;
    ++l->sz_;
}

ccc_dll_elem *
ccc_dll_head(ccc_doubly_linked_list const *const l)
{
    return (ccc_dll_elem *)l->sentinel_.n_;
}

ccc_dll_elem *
ccc_dll_tail(ccc_doubly_linked_list const *const l)
{
    return (ccc_dll_elem *)l->sentinel_.p_;
}

void
ccc_dll_splice(ccc_dll_elem *pos, ccc_dll_elem *const to_cut)
{
    if (!to_cut || !pos || pos == to_cut || to_cut->n_ == pos)
    {
        return;
    }
    to_cut->n_->p_ = to_cut->p_;
    to_cut->p_->n_ = to_cut->n_;

    to_cut->p_ = pos->p_;
    to_cut->n_ = pos;
    pos->p_->n_ = to_cut;
    pos->p_ = to_cut;
}

void
ccc_dll_splice_range(ccc_dll_elem *pos, ccc_dll_elem *begin, ccc_dll_elem *end)
{
    if (!begin || !end || !pos || pos == begin || begin->n_ == pos
        || begin == end)
    {
        return;
    }
    end = end->p_;

    end->n_->p_ = begin->p_;
    begin->p_->n_ = end->n_;

    begin->p_ = pos->p_;
    end->n_ = pos;
    pos->p_->n_ = begin;
    pos->p_ = end;
}

void *
ccc_dll_begin(ccc_doubly_linked_list const *const l)
{
    if (!l || l->sentinel_.n_ == &l->sentinel_)
    {
        return NULL;
    }
    return struct_base(l, l->sentinel_.n_);
}

void *
ccc_dll_rbegin(ccc_doubly_linked_list const *const l)
{
    if (!l || l->sentinel_.p_ == &l->sentinel_)
    {
        return NULL;
    }
    return struct_base(l, l->sentinel_.p_);
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
    if (!e || e->n_ == &l->sentinel_)
    {
        return NULL;
    }
    return struct_base(l, e->n_);
}

void *
ccc_dll_rnext(ccc_doubly_linked_list const *const l, ccc_dll_elem const *e)
{
    if (!e || e->p_ == &l->sentinel_)
    {
        return NULL;
    }
    return struct_base(l, e->p_);
}

size_t
ccc_dll_size(ccc_doubly_linked_list const *const l)
{
    return l->sz_;
}

bool
ccc_dll_empty(ccc_doubly_linked_list const *const l)
{
    return !l->sz_;
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

void
ccc_dll_clear_and_free(ccc_doubly_linked_list *const l, ccc_destructor_fn *fn)
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
    for (struct ccc_dll_elem_ *e = l->sentinel_.n_; e != &l->sentinel_;
         e = e->n_, ++size)
    {
        if (!e || !e->n_ || !e->p_)
        {
            return false;
        }
        if (size > l->sz_)
        {
            return false;
        }
    }
    return size == l->sz_;
}

struct ccc_dll_elem_ *
ccc_dll_elem__in(struct ccc_dll_ const *const l, void const *const user_struct)
{
    return (struct ccc_dll_elem_ *)((char *)user_struct + l->dll_elem_offset_);
}

static inline void *
struct_base(struct ccc_dll_ const *const l, struct ccc_dll_elem_ const *const e)
{
    return ((char *)&e->n_) - l->dll_elem_offset_;
}
