#include "singly_linked_list.h"
#include "impl_singly_linked_list.h"
#include "types.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static void *struct_base(struct ccc_sll_ const *, struct ccc_sll_elem_ const *);
static struct ccc_sll_elem_ *before(struct ccc_sll_ const *,
                                    struct ccc_sll_elem_ const *to_find);
static size_t len(struct ccc_sll_ const *, struct ccc_sll_elem_ const *begin,
                  struct ccc_sll_elem_ const *end);
static size_t extract_range(struct ccc_sll_ *, struct ccc_sll_elem_ *begin,
                            struct ccc_sll_elem_ *end);
static struct ccc_sll_elem_ *pop_front(struct ccc_sll_ *);

void *
ccc_sll_push_front(ccc_singly_linked_list *const sll,
                   ccc_sll_elem *const struct_handle)
{
    if (sll->alloc_)
    {
        void *node = sll->alloc_(NULL, sll->elem_sz_);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(sll, struct_handle), sll->elem_sz_);
    }
    ccc_impl_sll_push_front(sll, struct_handle);
    return struct_base(sll, sll->sentinel_.n_);
}

void *
ccc_sll_front(ccc_singly_linked_list const *const sll)
{
    if (!sll || sll->sentinel_.n_ == &sll->sentinel_)
    {
        return NULL;
    }
    return struct_base(sll, sll->sentinel_.n_);
}

ccc_sll_elem *
ccc_sll_begin_elem(ccc_singly_linked_list const *const sll)
{
    return sll->sentinel_.n_;
}

ccc_sll_elem *
ccc_sll_begin_sentinel(ccc_singly_linked_list const *const sll)
{
    return (ccc_sll_elem *)&sll->sentinel_;
}

ccc_result
ccc_sll_pop_front(ccc_singly_linked_list *const sll)
{
    if (!sll || !sll->sz_)
    {
        return CCC_INPUT_ERR;
    }
    struct ccc_sll_elem_ *remove = pop_front(sll);
    if (sll->alloc_)
    {
        (void)sll->alloc_(struct_base(sll, remove), 0);
    }
    return CCC_OK;
}

void
ccc_impl_sll_push_front(struct ccc_sll_ *const sll,
                        struct ccc_sll_elem_ *const e)
{
    e->n_ = sll->sentinel_.n_;
    sll->sentinel_.n_ = e;
    ++sll->sz_;
}

ccc_result
ccc_sll_splice(ccc_singly_linked_list *const pos_sll,
               ccc_sll_elem *const pos_before,
               ccc_singly_linked_list *const to_splice_sll,
               ccc_sll_elem *const to_splice)
{
    if (!pos_sll || !pos_before || !to_splice || !to_splice_sll)
    {
        return CCC_INPUT_ERR;
    }
    if (to_splice == pos_before || pos_before->n_ == to_splice)
    {
        return CCC_OK;
    }
    before(to_splice_sll, to_splice)->n_ = to_splice->n_;
    to_splice->n_ = pos_before->n_;
    pos_before->n_ = to_splice;
    if (pos_sll != to_splice_sll)
    {
        --to_splice_sll->sz_;
        ++pos_sll->sz_;
    }
    return CCC_OK;
}

ccc_result
ccc_sll_splice_range(ccc_singly_linked_list *const pos_sll,
                     ccc_sll_elem *const pos_before,
                     ccc_singly_linked_list *const to_splice_sll,
                     ccc_sll_elem *const to_splice_begin,
                     ccc_sll_elem *const to_splice_end)
{
    if (!pos_sll || !pos_before || !to_splice_begin || !to_splice_end
        || !to_splice_sll)
    {
        return CCC_INPUT_ERR;
    }
    if (to_splice_begin == pos_before || to_splice_end == pos_before
        || pos_before->n_ == to_splice_begin)
    {
        return CCC_OK;
    }
    if (to_splice_begin == to_splice_end)
    {
        (void)ccc_sll_splice(pos_sll, pos_before, to_splice_sll,
                             to_splice_begin);
        return CCC_OK;
    }
    struct ccc_sll_elem_ *found = before(to_splice_sll, to_splice_begin);
    found->n_ = to_splice_end->n_;

    to_splice_end->n_ = pos_before->n_;
    pos_before->n_ = to_splice_begin;
    if (pos_sll != to_splice_sll)
    {
        size_t const sz = len(to_splice_sll, to_splice_begin, to_splice_end);
        to_splice_sll->sz_ -= sz;
        pos_sll->sz_ += sz;
    }
    return CCC_OK;
}

void *
ccc_sll_extract(ccc_singly_linked_list *const extract_sll,
                ccc_sll_elem *const extract)
{
    if (!extract_sll || !extract || !extract_sll->sz_
        || extract == &extract_sll->sentinel_)
    {
        return NULL;
    }
    struct ccc_sll_elem_ const *const ret = extract->n_;
    before(extract_sll, extract)->n_ = extract->n_;
    extract->n_ = NULL;
    --extract_sll->sz_;
    return ret == &extract_sll->sentinel_ ? NULL
                                          : struct_base(extract_sll, ret);
}

void *
ccc_sll_extract_range(ccc_singly_linked_list *const extract_sll,
                      ccc_sll_elem *const extract_begin,
                      ccc_sll_elem *extract_end)
{
    if (!extract_sll || !extract_begin || !extract_end || !extract_sll->sz_
        || extract_begin == &extract_sll->sentinel_
        || extract_end == &extract_sll->sentinel_)
    {
        return NULL;
    }
    struct ccc_sll_elem_ const *const ret = extract_end->n_;
    before(extract_sll, extract_begin)->n_ = extract_end->n_;
    size_t const deleted
        = extract_range(extract_sll, extract_begin, extract_end);
    assert(deleted <= extract_sll->sz_);
    extract_sll->sz_ -= deleted;
    return ret == &extract_sll->sentinel_ ? NULL
                                          : struct_base(extract_sll, ret);
}

inline void *
ccc_sll_begin(ccc_singly_linked_list const *const sll)
{
    if (!sll || sll->sentinel_.n_ == &sll->sentinel_)
    {
        return NULL;
    }
    return struct_base(sll, sll->sentinel_.n_);
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
    if (!iter_handle || iter_handle->n_ == &sll->sentinel_)
    {
        return NULL;
    }
    return struct_base(sll, iter_handle->n_);
}

ccc_result
ccc_sll_clear(ccc_singly_linked_list *const sll, ccc_destructor_fn *const fn)
{
    if (!sll)
    {
        return CCC_INPUT_ERR;
    }
    while (!ccc_sll_is_empty(sll))
    {
        void *const mem = struct_base(sll, pop_front(sll));
        if (fn)
        {
            fn((ccc_user_type_mut){.user_type = mem, .aux = sll->aux_});
        }
        if (sll->alloc_)
        {
            (void)sll->alloc_(mem, 0);
        }
    }
    return CCC_OK;
}

bool
ccc_sll_validate(ccc_singly_linked_list const *const sll)
{
    size_t size = 0;
    for (struct ccc_sll_elem_ *e = sll->sentinel_.n_; e != &sll->sentinel_;
         e = e->n_, ++size)
    {
        if (size >= sll->sz_)
        {
            return false;
        }
        if (!e || !e->n_ || e->n_ == e)
        {
            return false;
        }
    }
    return size == sll->sz_;
}

size_t
ccc_sll_size(ccc_singly_linked_list const *const sll)
{
    return sll->sz_;
}

bool
ccc_sll_is_empty(ccc_singly_linked_list const *const sll)
{
    return !sll->sz_;
}

struct ccc_sll_elem_ *
ccc_sll_elem_in(struct ccc_sll_ const *const sll, void const *const user_struct)
{
    return (struct ccc_sll_elem_ *)((char *)user_struct
                                    + sll->sll_elem_offset_);
}

static inline struct ccc_sll_elem_ *
pop_front(struct ccc_sll_ *const sll)
{
    struct ccc_sll_elem_ *remove = sll->sentinel_.n_;
    sll->sentinel_.n_ = remove->n_;
    remove->n_ = NULL;
    --sll->sz_;
    return remove;
}

static inline struct ccc_sll_elem_ *
before(struct ccc_sll_ const *const sll,
       struct ccc_sll_elem_ const *const to_find)
{
    struct ccc_sll_elem_ const *i = &sll->sentinel_;
    for (; i->n_ != to_find; i = i->n_)
    {}
    return (struct ccc_sll_elem_ *)i;
}

static inline size_t
extract_range([[maybe_unused]] struct ccc_sll_ *const sll,
              struct ccc_sll_elem_ *begin, struct ccc_sll_elem_ *const end)
{
    size_t sz = 1;
    for (struct ccc_sll_elem_ *next = NULL; begin != end; begin = next, ++sz)
    {
        assert(sz <= sll->sz_);
        next = begin->n_;
        begin->n_ = NULL;
    }
    begin->n_ = NULL;
    return sz;
}

static inline size_t
len([[maybe_unused]] struct ccc_sll_ const *const sll,
    struct ccc_sll_elem_ const *begin, struct ccc_sll_elem_ const *const end)
{
    size_t s = 1;
    for (; begin != end; begin = begin->n_, ++s)
    {
        assert(s <= sll->sz_);
    }
    return s;
}

static inline void *
struct_base(struct ccc_sll_ const *const l, struct ccc_sll_elem_ const *const e)
{
    return ((char *)&e->n_) - l->sll_elem_offset_;
}
