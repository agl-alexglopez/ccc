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
#include "private/private_doubly_linked_list.h"
#include "types.h"

/*===========================   Prototypes    ===============================*/

static void *struct_base(struct CCC_Doubly_linked_list const *,
                         struct CCC_Doubly_linked_list_node const *);
static size_t extract_range(struct CCC_Doubly_linked_list const *l,
                            struct CCC_Doubly_linked_list_node *begin,
                            struct CCC_Doubly_linked_list_node *end);
static size_t erase_range(struct CCC_Doubly_linked_list const *l,
                          struct CCC_Doubly_linked_list_node *begin,
                          struct CCC_Doubly_linked_list_node *end);
static size_t len(struct CCC_Doubly_linked_list_node const *begin,
                  struct CCC_Doubly_linked_list_node const *end);
static struct CCC_Doubly_linked_list_node *
pop_front(struct CCC_Doubly_linked_list *);
static void push_front(struct CCC_Doubly_linked_list *l,
                       struct CCC_Doubly_linked_list_node *e);
static struct CCC_Doubly_linked_list_node *
elem_in(CCC_Doubly_linked_list const *, void const *struct_base);
static void push_back(CCC_Doubly_linked_list *,
                      struct CCC_Doubly_linked_list_node *);
static void splice_range(struct CCC_Doubly_linked_list_node *pos,
                         struct CCC_Doubly_linked_list_node *begin,
                         struct CCC_Doubly_linked_list_node *end);
static struct CCC_Doubly_linked_list_node *
first_less(struct CCC_Doubly_linked_list const *doubly_linked_list,
           struct CCC_Doubly_linked_list_node *);
static struct CCC_Doubly_linked_list_node *
merge(struct CCC_Doubly_linked_list *, struct CCC_Doubly_linked_list_node *,
      struct CCC_Doubly_linked_list_node *,
      struct CCC_Doubly_linked_list_node *);
static CCC_Order order(struct CCC_Doubly_linked_list const *doubly_linked_list,
                       struct CCC_Doubly_linked_list_node const *lhs,
                       struct CCC_Doubly_linked_list_node const *rhs);

/*===========================     Interface   ===============================*/

void *
CCC_doubly_linked_list_push_front(CCC_Doubly_linked_list *const l,
                                  CCC_Doubly_linked_list_node *elem)
{
    if (!l || !elem)
    {
        return NULL;
    }
    if (l->alloc)
    {
        void *const node = l->alloc((CCC_Allocator_context){
            .input = NULL,
            .bytes = l->sizeof_type,
            .context = l->context,
        });
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
CCC_doubly_linked_list_push_back(CCC_Doubly_linked_list *const l,
                                 CCC_Doubly_linked_list_node *elem)
{
    if (!l || !elem)
    {
        return NULL;
    }
    if (l->alloc)
    {
        void *const node = l->alloc((CCC_Allocator_context){
            .input = NULL,
            .bytes = l->sizeof_type,
            .context = l->context,
        });
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
CCC_doubly_linked_list_front(CCC_Doubly_linked_list const *l)
{
    if (!l || !l->count)
    {
        return NULL;
    }
    return struct_base(l, l->nil.n);
}

void *
CCC_doubly_linked_list_back(CCC_Doubly_linked_list const *l)
{
    if (!l || !l->count)
    {
        return NULL;
    }
    return struct_base(l, l->nil.p);
}

CCC_Result
CCC_doubly_linked_list_pop_front(CCC_Doubly_linked_list *const l)
{
    if (!l || !l->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Doubly_linked_list_node *remove = pop_front(l);
    if (l->alloc)
    {
        (void)l->alloc((CCC_Allocator_context){
            .input = struct_base(l, remove),
            .bytes = 0,
            .context = l->context,
        });
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_doubly_linked_list_pop_back(CCC_Doubly_linked_list *const l)
{
    if (!l || !l->count)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Doubly_linked_list_node *remove = l->nil.p;
    remove->p->n = &l->nil;
    l->nil.p = remove->p;
    if (remove != &l->nil)
    {
        remove->n = remove->p = NULL;
    }
    if (l->alloc)
    {
        (void)l->alloc((CCC_Allocator_context){
            .input = struct_base(l, remove),
            .bytes = 0,
            .context = l->context,
        });
    }
    --l->count;
    return CCC_RESULT_OK;
}

void *
CCC_doubly_linked_list_insert(CCC_Doubly_linked_list *const l,
                              CCC_Doubly_linked_list_node *const pos,
                              CCC_Doubly_linked_list_node *elem)
{
    if (!l || !pos->n || !pos->p)
    {
        return NULL;
    }
    if (l->alloc)
    {
        void *const node = l->alloc((CCC_Allocator_context){
            .input = NULL,
            .bytes = l->sizeof_type,
            .context = l->context,
        });
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
CCC_doubly_linked_list_erase(CCC_Doubly_linked_list *const l,
                             CCC_Doubly_linked_list_node *const elem)
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
        (void)l->alloc((CCC_Allocator_context){
            .input = struct_base(l, elem),
            .bytes = 0,
            .context = l->context,
        });
    }
    --l->count;
    return ret;
}

void *
CCC_doubly_linked_list_erase_range(
    CCC_Doubly_linked_list *const l,
    CCC_Doubly_linked_list_node *const elem_begin,
    CCC_Doubly_linked_list_node *elem_end)
{
    if (!l || !elem_begin || !elem_end || !elem_begin->n || !elem_begin->p
        || !elem_end->n || !elem_end->p || !l->count)
    {
        return NULL;
    }
    if (elem_begin == elem_end)
    {
        return CCC_doubly_linked_list_erase(l, elem_begin);
    }
    struct CCC_Doubly_linked_list_node const *const ret = elem_end;
    elem_end = elem_end->p;
    elem_end->n->p = elem_begin->p;
    elem_begin->p->n = elem_end->n;

    size_t const deleted = erase_range(l, elem_begin, elem_end);

    assert(deleted <= l->count);
    l->count -= deleted;
    return ret == &l->nil ? NULL : struct_base(l, ret);
}

CCC_Doubly_linked_list_node *
CCC_doubly_linked_list_node_begin(CCC_Doubly_linked_list const *const l)
{
    return l ? l->nil.n : NULL;
}

CCC_Doubly_linked_list_node *
CCC_doubly_linked_list_node_end(CCC_Doubly_linked_list const *const l)
{
    return l ? l->nil.p : NULL;
}

CCC_Doubly_linked_list_node *
CCC_doubly_linked_list_sentinel_end(CCC_Doubly_linked_list const *const l)
{
    return l ? (CCC_Doubly_linked_list_node *)&l->nil : NULL;
}

void *
CCC_doubly_linked_list_extract(CCC_Doubly_linked_list *const l,
                               CCC_Doubly_linked_list_node *const elem_in_list)
{
    if (!l || !elem_in_list || !elem_in_list->n || !elem_in_list->p
        || !l->count)
    {
        return NULL;
    }
    struct CCC_Doubly_linked_list_node const *const ret = elem_in_list->n;
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
CCC_doubly_linked_list_extract_range(
    CCC_Doubly_linked_list *const l,
    CCC_Doubly_linked_list_node *const elem_begin,
    CCC_Doubly_linked_list_node *elem_end)
{
    if (!l || !elem_begin || !elem_end || !elem_begin->n || !elem_begin->p
        || !elem_end->n || !elem_end->p || !l->count)
    {
        return NULL;
    }
    if (elem_begin == elem_end)
    {
        return CCC_doubly_linked_list_extract(l, elem_begin);
    }
    struct CCC_Doubly_linked_list_node const *const ret = elem_end;
    elem_end = elem_end->p;
    elem_end->n->p = elem_begin->p;
    elem_begin->p->n = elem_end->n;

    size_t const deleted = extract_range(l, elem_begin, elem_end);

    assert(deleted <= l->count);
    l->count -= deleted;
    return ret == &l->nil ? NULL : struct_base(l, ret);
}

CCC_Result
CCC_doubly_linked_list_splice(
    CCC_Doubly_linked_list *const pos_doubly_linked_list,
    CCC_Doubly_linked_list_node *pos,
    CCC_Doubly_linked_list *const to_cut_doubly_linked_list,
    CCC_Doubly_linked_list_node *const to_cut)
{
    if (!to_cut || !pos || !pos_doubly_linked_list
        || !to_cut_doubly_linked_list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (pos == to_cut || to_cut->n == pos
        || to_cut == &to_cut_doubly_linked_list->nil)
    {
        return CCC_RESULT_OK;
    }
    to_cut->n->p = to_cut->p;
    to_cut->p->n = to_cut->n;

    to_cut->p = pos->p;
    to_cut->n = pos;
    pos->p->n = to_cut;
    pos->p = to_cut;
    if (pos_doubly_linked_list != to_cut_doubly_linked_list)
    {
        ++pos_doubly_linked_list->count;
        --to_cut_doubly_linked_list->count;
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_doubly_linked_list_splice_range(
    CCC_Doubly_linked_list *const pos_doubly_linked_list,
    CCC_Doubly_linked_list_node *pos,
    CCC_Doubly_linked_list *const to_cut_doubly_linked_list,
    CCC_Doubly_linked_list_node *begin, CCC_Doubly_linked_list_node *end)
{
    if (!begin || !end || !pos || !pos_doubly_linked_list
        || !to_cut_doubly_linked_list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (pos == begin || pos == end || begin == end
        || begin == &to_cut_doubly_linked_list->nil
        || end->p == &to_cut_doubly_linked_list->nil)
    {
        return CCC_RESULT_OK;
    }
    struct CCC_Doubly_linked_list_node *const inclusive_end = end->p;
    splice_range(pos, begin, end);
    if (pos_doubly_linked_list != to_cut_doubly_linked_list)
    {
        size_t const count = len(begin, inclusive_end);
        assert(count <= to_cut_doubly_linked_list->count);
        pos_doubly_linked_list->count += count;
        to_cut_doubly_linked_list->count -= count;
    }
    return CCC_RESULT_OK;
}

void *
CCC_doubly_linked_list_begin(CCC_Doubly_linked_list const *const l)
{
    if (!l || l->nil.n == &l->nil)
    {
        return NULL;
    }
    return struct_base(l, l->nil.n);
}

void *
CCC_doubly_linked_list_rbegin(CCC_Doubly_linked_list const *const l)
{
    if (!l || l->nil.p == &l->nil)
    {
        return NULL;
    }
    return struct_base(l, l->nil.p);
}

void *
CCC_doubly_linked_list_end(CCC_Doubly_linked_list const *const)
{
    return NULL;
}

void *
CCC_doubly_linked_list_rend(CCC_Doubly_linked_list const *const)
{
    return NULL;
}

void *
CCC_doubly_linked_list_next(CCC_Doubly_linked_list const *const l,
                            CCC_Doubly_linked_list_node const *e)
{
    if (!l || !e || e->n == &l->nil)
    {
        return NULL;
    }
    return struct_base(l, e->n);
}

void *
CCC_doubly_linked_list_rnext(CCC_Doubly_linked_list const *const l,
                             CCC_Doubly_linked_list_node const *e)
{
    if (!l || !e || e->p == &l->nil)
    {
        return NULL;
    }
    return struct_base(l, e->p);
}

CCC_Count
CCC_doubly_linked_list_count(CCC_Doubly_linked_list const *const l)
{
    if (!l)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = l->count};
}

CCC_Tribool
CCC_doubly_linked_list_is_empty(CCC_Doubly_linked_list const *const l)
{
    return l ? !l->count : CCC_TRUE;
}

CCC_Result
CCC_doubly_linked_list_clear(CCC_Doubly_linked_list *const l,
                             CCC_Type_destructor *fn)
{
    if (!l)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    while (!CCC_doubly_linked_list_is_empty(l))
    {
        void *const node = struct_base(l, pop_front(l));
        if (fn)
        {
            fn((CCC_Type_context){.type = node, .context = l->context});
        }
        if (l->alloc)
        {
            (void)l->alloc((CCC_Allocator_context){
                .input = node,
                .bytes = 0,
                .context = l->context,
            });
        }
    }
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_doubly_linked_list_validate(CCC_Doubly_linked_list const *const l)
{
    if (!l)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t size = 0;
    for (struct CCC_Doubly_linked_list_node const *e = l->nil.n; e != &l->nil;
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
CCC_Tribool
CCC_doubly_linked_list_is_sorted(
    CCC_Doubly_linked_list const *const doubly_linked_list)
{
    if (!doubly_linked_list)
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (doubly_linked_list->count <= 1)
    {
        return CCC_TRUE;
    }
    for (struct CCC_Doubly_linked_list_node const *cur
         = doubly_linked_list->nil.n->n;
         cur != &doubly_linked_list->nil; cur = cur->n)
    {
        if (order(doubly_linked_list, cur->p, cur) == CCC_ORDER_GREATER)
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
CCC_doubly_linked_list_insert_sorted(
    CCC_Doubly_linked_list *const doubly_linked_list,
    CCC_Doubly_linked_list_node *e)
{
    if (!doubly_linked_list || !e)
    {
        return NULL;
    }
    if (doubly_linked_list->alloc)
    {
        void *const node = doubly_linked_list->alloc((CCC_Allocator_context){
            .input = NULL,
            .bytes = doubly_linked_list->sizeof_type,
            .context = doubly_linked_list->context,
        });
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(doubly_linked_list, e),
                     doubly_linked_list->sizeof_type);
        e = elem_in(doubly_linked_list, node);
    }
    struct CCC_Doubly_linked_list_node *pos = doubly_linked_list->nil.n;
    for (; pos != &doubly_linked_list->nil
           && order(doubly_linked_list, e, pos) != CCC_ORDER_LESSER;
         pos = pos->n)
    {}
    e->n = pos;
    e->p = pos->p;
    pos->p->n = e;
    pos->p = e;
    ++doubly_linked_list->count;
    return struct_base(doubly_linked_list, e);
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
CCC_Result
CCC_doubly_linked_list_sort(CCC_Doubly_linked_list *const doubly_linked_list)
{
    if (!doubly_linked_list)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* Algorithm is one pass if list is sorted. Merging is never true. */
    CCC_Tribool merging = CCC_FALSE;
    do
    {
        merging = CCC_FALSE;
        /* 0th index of the A list. The start of one list to merge. */
        struct CCC_Doubly_linked_list_node *a_0 = doubly_linked_list->nil.n;
        while (a_0 != &doubly_linked_list->nil)
        {
            /* The Nth index of list A (its size) aka 0th index of B list. */
            struct CCC_Doubly_linked_list_node *const a_n_b_0
                = first_less(doubly_linked_list, a_0);
            if (a_n_b_0 == &doubly_linked_list->nil)
            {
                break;
            }
            /* A picks up the exclusive end of this merge, B, in order
               to progress the sorting algorithm with the next run that needs
               fixing. Merge returns the end of B to indicate it is the final
               sentinel node yet to be examined. */
            a_0 = merge(doubly_linked_list, a_0, a_n_b_0,
                        first_less(doubly_linked_list, a_n_b_0));
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
static inline struct CCC_Doubly_linked_list_node *
merge(struct CCC_Doubly_linked_list *const doubly_linked_list,
      struct CCC_Doubly_linked_list_node *a_0,
      struct CCC_Doubly_linked_list_node *a_n_b_0,
      struct CCC_Doubly_linked_list_node *const b_n)
{
    assert(doubly_linked_list && a_0 && a_n_b_0 && b_n);
    while (a_0 != a_n_b_0 && a_n_b_0 != b_n)
    {
        if (order(doubly_linked_list, a_n_b_0, a_0) == CCC_ORDER_LESSER)
        {
            struct CCC_Doubly_linked_list_node *const lesser = a_n_b_0;
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
static inline struct CCC_Doubly_linked_list_node *
first_less(struct CCC_Doubly_linked_list const *const doubly_linked_list,
           struct CCC_Doubly_linked_list_node *start)
{
    assert(doubly_linked_list && start);
    do
    {
        start = start->n;
    }
    while (start != &doubly_linked_list->nil
           && order(doubly_linked_list, start, start->p) != CCC_ORDER_LESSER);
    return start;
}

/*=======================     Private Interface   ===========================*/

void
CCC_private_doubly_linked_list_push_back(
    struct CCC_Doubly_linked_list *const l,
    struct CCC_Doubly_linked_list_node *const e)
{
    if (!l || !e)
    {
        return;
    }
    push_back(l, e);
}

void
CCC_private_doubly_linked_list_push_front(
    struct CCC_Doubly_linked_list *const l,
    struct CCC_Doubly_linked_list_node *const e)
{
    if (!l || !e)
    {
        return;
    }
    push_front(l, e);
}

struct CCC_Doubly_linked_list_node *
CCC_private_doubly_linked_list_node_in(
    struct CCC_Doubly_linked_list const *const l, void const *const any_struct)
{
    return elem_in(l, any_struct);
}

/*=======================       Static Helpers    ===========================*/

/** Places the range `[begin, end)` at position before pos. This means end is
not moved or altered due to the exclusive range. If begin is equal to end the
function returns early changing no nodes. */
static inline void
splice_range(struct CCC_Doubly_linked_list_node *const pos,
             struct CCC_Doubly_linked_list_node *const begin,
             struct CCC_Doubly_linked_list_node *end)
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
push_front(struct CCC_Doubly_linked_list *const l,
           struct CCC_Doubly_linked_list_node *const e)
{
    e->p = &l->nil;
    e->n = l->nil.n;
    l->nil.n->p = e;
    l->nil.n = e;
    ++l->count;
}

static inline void
push_back(struct CCC_Doubly_linked_list *const l,
          struct CCC_Doubly_linked_list_node *const e)
{
    e->n = &l->nil;
    e->p = l->nil.p;
    l->nil.p->n = e;
    l->nil.p = e;
    ++l->count;
}

static inline struct CCC_Doubly_linked_list_node *
pop_front(struct CCC_Doubly_linked_list *const doubly_linked_list)
{
    struct CCC_Doubly_linked_list_node *const ret = doubly_linked_list->nil.n;
    ret->n->p = &doubly_linked_list->nil;
    doubly_linked_list->nil.n = ret->n;
    if (ret != &doubly_linked_list->nil)
    {
        ret->n = ret->p = NULL;
    }
    --doubly_linked_list->count;
    return ret;
}

static inline size_t
extract_range(struct CCC_Doubly_linked_list const *const l,
              struct CCC_Doubly_linked_list_node *begin,
              struct CCC_Doubly_linked_list_node *const end)
{
    if (begin != &l->nil)
    {
        begin->p = NULL;
    }
    size_t const count = len(begin, end);
    if (end != &l->nil)
    {
        end->n = NULL;
    }
    return count;
}

static size_t
erase_range(struct CCC_Doubly_linked_list const *const l,
            struct CCC_Doubly_linked_list_node *begin,
            struct CCC_Doubly_linked_list_node *const end)
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
        struct CCC_Doubly_linked_list_node *const next = begin->n;
        (void)l->alloc((CCC_Allocator_context){
            .input = struct_base(l, begin),
            .bytes = 0,
            .context = l->context,
        });
        begin = next;
    }
    (void)l->alloc((CCC_Allocator_context){
        .input = struct_base(l, end),
        .bytes = 0,
        .context = l->context,
    });
    return count;
}

/** Finds the length from [begin, end]. End is counted. */
static size_t
len(struct CCC_Doubly_linked_list_node const *begin,
    struct CCC_Doubly_linked_list_node const *const end)
{
    size_t s = 1;
    for (; begin != end; begin = begin->n, ++s)
    {}
    return s;
}

static inline void *
struct_base(struct CCC_Doubly_linked_list const *const l,
            struct CCC_Doubly_linked_list_node const *const e)
{
    return ((char *)&e->n) - l->doubly_linked_list_node_offset;
}

static inline struct CCC_Doubly_linked_list_node *
elem_in(struct CCC_Doubly_linked_list const *const doubly_linked_list,
        void const *const struct_base)
{
    return (struct CCC_Doubly_linked_list_node
                *)((char *)struct_base
                   + doubly_linked_list->doubly_linked_list_node_offset);
}

/** Calls the user provided three way comparison callback function on the user
type wrapping the provided intrusive handles. Returns the three way comparison
result value. */
static inline CCC_Order
order(struct CCC_Doubly_linked_list const *const doubly_linked_list,
      struct CCC_Doubly_linked_list_node const *const lhs,
      struct CCC_Doubly_linked_list_node const *const rhs)
{
    return doubly_linked_list->compare((CCC_Type_comparator_context){
        .type_lhs = struct_base(doubly_linked_list, lhs),
        .type_rhs = struct_base(doubly_linked_list, rhs),
        .context = doubly_linked_list->context,
    });
}
