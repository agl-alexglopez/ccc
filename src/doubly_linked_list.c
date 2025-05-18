/** Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */
/* Citation:
[1] See the sort methods for citations and change lists regarding the pintOS
educational operating system natural merge sort algorithm used for linked lists.
Code in the pintOS source is at  `src/lib/kernel.list.c`, but this may change if
they refactor. */
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "doubly_linked_list.h"
#include "impl/impl_doubly_linked_list.h"
#include "types.h"

/*===========================   Prototypes    ===============================*/

static void *struct_base(struct ccc_dll const *, struct ccc_dll_elem const *);
static size_t extract_range(struct ccc_dll const *l, struct ccc_dll_elem *begin,
                            struct ccc_dll_elem *end);
static size_t erase_range(struct ccc_dll const *l, struct ccc_dll_elem *begin,
                          struct ccc_dll_elem *end);
static size_t len(struct ccc_dll_elem const *begin,
                  struct ccc_dll_elem const *end);
static struct ccc_dll_elem *pop_front(struct ccc_dll *);
static void push_front(struct ccc_dll *l, struct ccc_dll_elem *e);
static struct ccc_dll_elem *elem_in(ccc_doubly_linked_list const *,
                                    void const *struct_base);
static void push_back(ccc_doubly_linked_list *, struct ccc_dll_elem *);
static void splice_range(struct ccc_dll_elem *pos, struct ccc_dll_elem *begin,
                         struct ccc_dll_elem *end);
static struct ccc_dll_elem *first_less(struct ccc_dll const *dll,
                                       struct ccc_dll_elem *);
static struct ccc_dll_elem *merge(struct ccc_dll *, struct ccc_dll_elem *,
                                  struct ccc_dll_elem *, struct ccc_dll_elem *);
static ccc_threeway_cmp cmp(struct ccc_dll const *dll,
                            struct ccc_dll_elem const *lhs,
                            struct ccc_dll_elem const *rhs);

/*===========================     Interface   ===============================*/

void *
ccc_dll_push_front(ccc_doubly_linked_list *const l, ccc_dll_elem *elem)
{
    if (!l || !elem)
    {
        return NULL;
    }
    if (l->alloc)
    {
        void *const node = l->alloc(NULL, l->sizeof_type, l->aux);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(l, elem), l->sizeof_type);
        elem = elem_in(l, node);
    }
    push_front(l, elem);
    return struct_base(l, l->nil.n);
}

void *
ccc_dll_push_back(ccc_doubly_linked_list *const l, ccc_dll_elem *elem)
{
    if (!l || !elem)
    {
        return NULL;
    }
    if (l->alloc)
    {
        void *const node = l->alloc(NULL, l->sizeof_type, l->aux);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(l, elem), l->sizeof_type);
        elem = elem_in(l, node);
    }
    push_back(l, elem);
    return struct_base(l, l->nil.p);
}

void *
ccc_dll_front(ccc_doubly_linked_list const *l)
{
    if (!l || !l->count)
    {
        return NULL;
    }
    return struct_base(l, l->nil.n);
}

void *
ccc_dll_back(ccc_doubly_linked_list const *l)
{
    if (!l || !l->count)
    {
        return NULL;
    }
    return struct_base(l, l->nil.p);
}

ccc_result
ccc_dll_pop_front(ccc_doubly_linked_list *const l)
{
    if (!l || !l->count)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    struct ccc_dll_elem *remove = pop_front(l);
    if (l->alloc)
    {
        (void)l->alloc(struct_base(l, remove), 0, l->aux);
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_dll_pop_back(ccc_doubly_linked_list *const l)
{
    if (!l || !l->count)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    struct ccc_dll_elem *remove = l->nil.p;
    remove->p->n = &l->nil;
    l->nil.p = remove->p;
    if (remove != &l->nil)
    {
        remove->n = remove->p = NULL;
    }
    if (l->alloc)
    {
        (void)l->alloc(struct_base(l, remove), 0, l->aux);
    }
    --l->count;
    return CCC_RESULT_OK;
}

void *
ccc_dll_insert(ccc_doubly_linked_list *const l, ccc_dll_elem *const pos,
               ccc_dll_elem *elem)
{
    if (!l || !pos->n || !pos->p)
    {
        return NULL;
    }
    if (l->alloc)
    {
        void *const node = l->alloc(NULL, l->sizeof_type, l->aux);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(l, elem), l->sizeof_type);
        elem = elem_in(l, node);
    }
    elem->n = pos;
    elem->p = pos->p;

    pos->p->n = elem;
    pos->p = elem;
    ++l->count;
    return struct_base(l, elem);
}

void *
ccc_dll_erase(ccc_doubly_linked_list *const l, ccc_dll_elem *const elem)
{
    if (!l || !elem || !l->count)
    {
        return NULL;
    }
    elem->n->p = elem->p;
    elem->p->n = elem->n;
    void *const ret = elem->n == &l->nil ? NULL : struct_base(l, elem);
    if (l->alloc)
    {
        (void)l->alloc(struct_base(l, elem), 0, l->aux);
    }
    --l->count;
    return ret;
}

void *
ccc_dll_erase_range(ccc_doubly_linked_list *const l,
                    ccc_dll_elem *const elem_begin, ccc_dll_elem *elem_end)
{
    if (!l || !elem_begin || !elem_end || !elem_begin->n || !elem_begin->p
        || !elem_end->n || !elem_end->p || !l->count)
    {
        return NULL;
    }
    if (elem_begin == elem_end)
    {
        return ccc_dll_erase(l, elem_begin);
    }
    struct ccc_dll_elem const *const ret = elem_end;
    elem_end = elem_end->p;
    elem_end->n->p = elem_begin->p;
    elem_begin->p->n = elem_end->n;

    size_t const deleted = erase_range(l, elem_begin, elem_end);

    assert(deleted <= l->count);
    l->count -= deleted;
    return ret == &l->nil ? NULL : struct_base(l, ret);
}

ccc_dll_elem *
ccc_dll_begin_elem(ccc_doubly_linked_list const *const l)
{
    return l ? l->nil.n : NULL;
}

ccc_dll_elem *
ccc_dll_end_elem(ccc_doubly_linked_list const *const l)
{
    return l ? l->nil.p : NULL;
}

ccc_dll_elem *
ccc_dll_end_sentinel(ccc_doubly_linked_list const *const l)
{
    return l ? (struct ccc_dll_elem *)&l->nil : NULL;
}

void *
ccc_dll_extract(ccc_doubly_linked_list *const l,
                ccc_dll_elem *const elem_in_list)
{
    if (!l || !elem_in_list || !elem_in_list->n || !elem_in_list->p
        || !l->count)
    {
        return NULL;
    }
    struct ccc_dll_elem const *const ret = elem_in_list->n;
    elem_in_list->n->p = elem_in_list->p;
    elem_in_list->p->n = elem_in_list->n;
    if (elem_in_list != &l->nil)
    {
        elem_in_list->n = elem_in_list->p = NULL;
    }
    --l->count;
    return ret == &l->nil ? NULL : struct_base(l, ret);
}

void *
ccc_dll_extract_range(ccc_doubly_linked_list *const l,
                      ccc_dll_elem *const elem_begin, ccc_dll_elem *elem_end)
{
    if (!l || !elem_begin || !elem_end || !elem_begin->n || !elem_begin->p
        || !elem_end->n || !elem_end->p || !l->count)
    {
        return NULL;
    }
    if (elem_begin == elem_end)
    {
        return ccc_dll_extract(l, elem_begin);
    }
    struct ccc_dll_elem const *const ret = elem_end;
    elem_end = elem_end->p;
    elem_end->n->p = elem_begin->p;
    elem_begin->p->n = elem_end->n;

    size_t const deleted = extract_range(l, elem_begin, elem_end);

    assert(deleted <= l->count);
    l->count -= deleted;
    return ret == &l->nil ? NULL : struct_base(l, ret);
}

ccc_result
ccc_dll_splice(ccc_doubly_linked_list *const pos_dll, ccc_dll_elem *pos,
               ccc_doubly_linked_list *const to_cut_dll,
               ccc_dll_elem *const to_cut)
{
    if (!to_cut || !pos || !pos_dll || !to_cut_dll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (pos == to_cut || to_cut->n == pos || to_cut == &to_cut_dll->nil)
    {
        return CCC_RESULT_OK;
    }
    to_cut->n->p = to_cut->p;
    to_cut->p->n = to_cut->n;

    to_cut->p = pos->p;
    to_cut->n = pos;
    pos->p->n = to_cut;
    pos->p = to_cut;
    if (pos_dll != to_cut_dll)
    {
        ++pos_dll->count;
        --to_cut_dll->count;
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_dll_splice_range(ccc_doubly_linked_list *const pos_dll, ccc_dll_elem *pos,
                     ccc_doubly_linked_list *const to_cut_dll,
                     ccc_dll_elem *begin, ccc_dll_elem *end)
{
    if (!begin || !end || !pos || !pos_dll || !to_cut_dll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (pos == begin || pos == end || begin == end || begin == &to_cut_dll->nil
        || end->p == &to_cut_dll->nil)
    {
        return CCC_RESULT_OK;
    }
    struct ccc_dll_elem *const inclusive_end = end->p;
    splice_range(pos, begin, end);
    if (pos_dll != to_cut_dll)
    {
        size_t const count = len(begin, inclusive_end);
        assert(count <= to_cut_dll->count);
        pos_dll->count += count;
        to_cut_dll->count -= count;
    }
    return CCC_RESULT_OK;
}

void *
ccc_dll_begin(ccc_doubly_linked_list const *const l)
{
    if (!l || l->nil.n == &l->nil)
    {
        return NULL;
    }
    return struct_base(l, l->nil.n);
}

void *
ccc_dll_rbegin(ccc_doubly_linked_list const *const l)
{
    if (!l || l->nil.p == &l->nil)
    {
        return NULL;
    }
    return struct_base(l, l->nil.p);
}

void *
ccc_dll_end(ccc_doubly_linked_list const *const)
{
    return NULL;
}

void *
ccc_dll_rend(ccc_doubly_linked_list const *const)
{
    return NULL;
}

void *
ccc_dll_next(ccc_doubly_linked_list const *const l, ccc_dll_elem const *e)
{
    if (!l || !e || e->n == &l->nil)
    {
        return NULL;
    }
    return struct_base(l, e->n);
}

void *
ccc_dll_rnext(ccc_doubly_linked_list const *const l, ccc_dll_elem const *e)
{
    if (!l || !e || e->p == &l->nil)
    {
        return NULL;
    }
    return struct_base(l, e->p);
}

ccc_ucount
ccc_dll_size(ccc_doubly_linked_list const *const l)
{
    if (!l)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = l->count};
}

ccc_tribool
ccc_dll_is_empty(ccc_doubly_linked_list const *const l)
{
    return l ? !l->count : CCC_TRUE;
}

ccc_result
ccc_dll_clear(ccc_doubly_linked_list *const l, ccc_any_type_destructor_fn *fn)
{
    if (!l)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    while (!ccc_dll_is_empty(l))
    {
        void *const node = struct_base(l, pop_front(l));
        if (fn)
        {
            fn((ccc_any_type){.any_type = node, .aux = l->aux});
        }
        if (l->alloc)
        {
            (void)l->alloc(node, 0, l->aux);
        }
    }
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_dll_validate(ccc_doubly_linked_list const *const l)
{
    if (!l)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t size = 0;
    for (struct ccc_dll_elem const *e = l->nil.n; e != &l->nil;
         e = e->n, ++size)
    {
        if (size >= l->count)
        {
            return CCC_FALSE;
        }
        if (!e || !e->n || !e->p || e->n == e || e->p == e)
        {
            return CCC_FALSE;
        }
    }
    return size == l->count;
}

/*==========================     Sorting     ================================*/

/** Returns true if the list is sorted in non-decreasing order. The user should
flip the return values of their comparison function if they want a different
order for elements.*/
ccc_tribool
ccc_dll_is_sorted(ccc_doubly_linked_list const *const dll)
{
    if (!dll)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (dll->count <= 1)
    {
        return CCC_TRUE;
    }
    for (struct ccc_dll_elem const *cur = dll->nil.n->n; cur != &dll->nil;
         cur = cur->n)
    {
        if (cmp(dll, cur->p, cur) == CCC_GRT)
        {
            return CCC_FALSE;
        }
    }
    return CCC_TRUE;
}

/** Inserts an element in non-decreasing order. This means an element will go
to the end of a section of duplicate values which is good for round-robin style
list use. */
void *
ccc_dll_insert_sorted(ccc_doubly_linked_list *const dll, ccc_dll_elem *e)
{
    if (!dll || !e)
    {
        return NULL;
    }
    if (dll->alloc)
    {
        void *const node = dll->alloc(NULL, dll->sizeof_type, dll->aux);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(dll, e), dll->sizeof_type);
        e = elem_in(dll, node);
    }
    struct ccc_dll_elem *pos = dll->nil.n;
    for (; pos != &dll->nil && cmp(dll, e, pos) != CCC_LES; pos = pos->n)
    {}
    e->n = pos;
    e->p = pos->p;
    pos->p->n = e;
    pos->p = e;
    ++dll->count;
    return struct_base(dll, e);
}

/** Sorts the list into non-decreasing order according to the user comparison
callback function in `O(N * log(N))` time and `O(1)` space.

The following merging algorithm and associated helper functions are based on
the iterative natural merge sort used in the list module of the pintOS project
for learning operating systems. Currently the original is located at the
following path in the pintOS source code:

`src/lib/kernel/list.c`

However, if refactors change this location, seek the list intrusive container
module for original implementations. The code has been changed for the C
Container Collection as follows:

- there is a single sentinel node rather than two.
- splicing in the merge operation has been simplified along with other tweaks.
- comparison callbacks are handled with three way comparison.

If the runtime is not obvious from the code, consider that this algorithm runs
bottom up on sorted sub-ranges. It roughly "halves" the remaining sub-ranges
that need to be sorted by roughly "doubling" the length of a sorted range on
each merge step. Therefore the number of times we must perform the merge step is
`O(log(N))`. The most elements we would have to merge in the merge step is all
`N` elements so together that gives us the runtime of `O(N * log(N))`. */
ccc_result
ccc_dll_sort(ccc_doubly_linked_list *const dll)
{
    if (!dll)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Algorithm is one pass if list is sorted. Merging is never true. */
    ccc_tribool merging = CCC_FALSE;
    do
    {
        merging = CCC_FALSE;
        /* 0th index of the A list. The start of one list to merge. */
        struct ccc_dll_elem *a_0 = dll->nil.n;
        while (a_0 != &dll->nil)
        {
            /* The Nth index of list A (its size) aka 0th index of B list. */
            struct ccc_dll_elem *const a_n_b_0 = first_less(dll, a_0);
            if (a_n_b_0 == &dll->nil)
            {
                break;
            }
            /* A picks up the exclusive end of this merge, B, in order
               to progress the sorting algorithm with the next run that needs
               fixing. Merge returns the end of B to indicate it is the final
               sentinel node yet to be examined. */
            a_0 = merge(dll, a_0, a_n_b_0, first_less(dll, a_n_b_0));
            merging = CCC_TRUE;
        }
    }
    while (merging);
    return CCC_RESULT_OK;
}

/** Merges lists `[a_0, a_n_b_0)` with `[a_n_b_0, b_n)` to form `[a_0, b_n)`.
Returns the exclusive end of the range, `b_n`, once the merge sort is complete.

Notice that all ranges treat the end of their range as an exclusive sentinel for
consistency. This function assumes the provided lists are already sorted
separately. */
static inline struct ccc_dll_elem *
merge(struct ccc_dll *const dll, struct ccc_dll_elem *a_0,
      struct ccc_dll_elem *a_n_b_0, struct ccc_dll_elem *const b_n)
{
    assert(dll && a_0 && a_n_b_0 && b_n);
    while (a_0 != a_n_b_0 && a_n_b_0 != b_n)
    {
        if (cmp(dll, a_n_b_0, a_0) == CCC_LES)
        {
            struct ccc_dll_elem *const lesser = a_n_b_0;
            a_n_b_0 = lesser->n;
            lesser->n->p = lesser->p;
            lesser->p->n = lesser->n;
            lesser->p = a_0->p;
            lesser->n = a_0;
            a_0->p->n = lesser;
            a_0->p = lesser;
        }
        else
        {
            a_0 = a_0->n;
        }
    }
    return b_n;
}

/** Finds the first element lesser than it's previous element as defined by
the user comparison callback function. If no out of order element can be
found the list sentinel is returned. */
static inline struct ccc_dll_elem *
first_less(struct ccc_dll const *const dll, struct ccc_dll_elem *start)
{
    assert(dll && start);
    do
    {
        start = start->n;
    }
    while (start != &dll->nil && cmp(dll, start, start->p) != CCC_LES);
    return start;
}

/*=======================     Private Interface   ===========================*/

void
ccc_impl_dll_push_back(struct ccc_dll *const l, struct ccc_dll_elem *const e)
{
    if (!l || !e)
    {
        return;
    }
    push_back(l, e);
}

void
ccc_impl_dll_push_front(struct ccc_dll *const l, struct ccc_dll_elem *const e)
{
    if (!l || !e)
    {
        return;
    }
    push_front(l, e);
}

struct ccc_dll_elem *
ccc_impl_dll_elem_in(struct ccc_dll const *const l,
                     void const *const any_struct)
{
    return elem_in(l, any_struct);
}

/*=======================       Static Helpers    ===========================*/

/** Places the range `[begin, end)` at position before pos. This means end is
not moved or altered due to the exclusive range. If begin is equal to end the
function returns early changing no nodes. */
static inline void
splice_range(struct ccc_dll_elem *const pos, struct ccc_dll_elem *const begin,
             struct ccc_dll_elem *end)
{
    if (begin == end)
    {
        return;
    }
    end = end->p;
    end->n->p = begin->p;
    begin->p->n = end->n;

    begin->p = pos->p;
    end->n = pos;
    pos->p->n = begin;
    pos->p = end;
}

static inline void
push_front(struct ccc_dll *const l, struct ccc_dll_elem *const e)
{
    e->p = &l->nil;
    e->n = l->nil.n;
    l->nil.n->p = e;
    l->nil.n = e;
    ++l->count;
}

static inline void
push_back(ccc_doubly_linked_list *const l, struct ccc_dll_elem *const e)
{
    e->n = &l->nil;
    e->p = l->nil.p;
    l->nil.p->n = e;
    l->nil.p = e;
    ++l->count;
}

static inline struct ccc_dll_elem *
pop_front(struct ccc_dll *const dll)
{
    struct ccc_dll_elem *ret = dll->nil.n;
    ret->n->p = &dll->nil;
    dll->nil.n = ret->n;
    if (ret != &dll->nil)
    {
        ret->n = ret->p = NULL;
    }
    --dll->count;
    return ret;
}

static inline size_t
extract_range([[maybe_unused]] struct ccc_dll const *const l,
              struct ccc_dll_elem *begin, struct ccc_dll_elem *const end)
{
    if (begin != &l->nil)
    {
        begin->p = NULL;
    }
    size_t count = len(begin, end);
    if (end != &l->nil)
    {
        end->n = NULL;
    }
    return count;
}

static size_t
erase_range(struct ccc_dll const *const l, struct ccc_dll_elem *begin,
            struct ccc_dll_elem *const end)
{
    if (begin != &l->nil)
    {
        begin->p = NULL;
    }
    if (!l->alloc)
    {
        size_t const count = len(begin, end);
        if (end != &l->nil)
        {
            end->n = NULL;
        }
        return count;
    }
    size_t count = 1;
    for (; begin != end; ++count)
    {
        assert(count <= l->count);
        struct ccc_dll_elem *const next = begin->n;
        (void)l->alloc(struct_base(l, begin), 0, l->aux);
        begin = next;
    }
    (void)l->alloc(struct_base(l, end), 0, l->aux);
    return count;
}

/** Finds the length from [begin, end]. End is counted. */
static size_t
len(struct ccc_dll_elem const *begin, struct ccc_dll_elem const *const end)
{
    size_t s = 1;
    for (; begin != end; begin = begin->n, ++s)
    {}
    return s;
}

static inline void *
struct_base(struct ccc_dll const *const l, struct ccc_dll_elem const *const e)
{
    return ((char *)&e->n) - l->dll_elem_offset;
}

static inline struct ccc_dll_elem *
elem_in(ccc_doubly_linked_list const *const dll, void const *const struct_base)
{
    return (struct ccc_dll_elem *)((char *)struct_base + dll->dll_elem_offset);
}

/** Calls the user provided three way comparison callback function on the user
type wrapping the provided intrusive handles. Returns the three way comparison
result value. */
static inline ccc_threeway_cmp
cmp(struct ccc_dll const *const dll, struct ccc_dll_elem const *const lhs,
    struct ccc_dll_elem const *const rhs)
{
    return dll->cmp((ccc_any_type_cmp){
        .any_type_lhs = struct_base(dll, lhs),
        .any_type_rhs = struct_base(dll, rhs),
        .aux = dll->aux,
    });
}
