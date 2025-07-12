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
#include <stddef.h>
#include <string.h>

#include "impl/impl_priority_queue.h"
#include "priority_queue.h"
#include "types.h"

/*=========================  Function Prototypes   ==========================*/

static void *elem_in(struct ccc_pq const *, struct ccc_pq_elem const *);
static struct ccc_pq_elem *merge(struct ccc_pq *, struct ccc_pq_elem *old,
                                 struct ccc_pq_elem *new);
static void link_child(struct ccc_pq_elem *parent, struct ccc_pq_elem *child);
static void init_node(struct ccc_pq_elem *);
static size_t traversal_count(struct ccc_pq_elem const *);
static ccc_tribool has_valid_links(struct ccc_pq const *,
                                   struct ccc_pq_elem const *parent,
                                   struct ccc_pq_elem const *child);
static struct ccc_pq_elem *delete_node(struct ccc_pq *, struct ccc_pq_elem *);
static struct ccc_pq_elem *delete_min(struct ccc_pq *, struct ccc_pq_elem *);
static void clear_node(struct ccc_pq_elem *);
static void cut_child(struct ccc_pq_elem *);
static void *struct_base(struct ccc_pq const *, struct ccc_pq_elem const *e);
static ccc_threeway_cmp cmp(struct ccc_pq const *, struct ccc_pq_elem const *,
                            struct ccc_pq_elem const *);
static void update_fixup(struct ccc_pq *, struct ccc_pq_elem *,
                         ccc_any_type_update_fn *, void *);
static void increase_fixup(struct ccc_pq *, struct ccc_pq_elem *,
                           ccc_any_type_update_fn *, void *);
static void decrease_fixup(struct ccc_pq *, struct ccc_pq_elem *,
                           ccc_any_type_update_fn *, void *);

/*=========================  Interface Functions   ==========================*/

void *
ccc_pq_front(ccc_priority_queue const *const pq)
{
    if (!pq)
    {
        return NULL;
    }
    return pq->root ? struct_base(pq, pq->root) : NULL;
}

void *
ccc_pq_push(ccc_priority_queue *const pq, ccc_pq_elem *e)
{
    if (!e || !pq)
    {
        return NULL;
    }
    init_node(e);
    void *ret = struct_base(pq, e);
    if (pq->alloc)
    {
        void *const node = pq->alloc(NULL, pq->sizeof_type, pq->aux);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, ret, pq->sizeof_type);
        ret = node;
        e = elem_in(pq, e);
    }
    pq->root = merge(pq, pq->root, e);
    ++pq->count;
    return ret;
}

ccc_result
ccc_pq_pop(ccc_priority_queue *const pq)
{
    if (!pq || !pq->root)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    struct ccc_pq_elem *const popped = pq->root;
    pq->root = delete_min(pq, pq->root);
    pq->count--;
    clear_node(popped);
    if (pq->alloc)
    {
        (void)pq->alloc(struct_base(pq, popped), 0, pq->aux);
    }
    return CCC_RESULT_OK;
}

void *
ccc_pq_extract(ccc_priority_queue *const pq, ccc_pq_elem *const e)
{
    if (!pq || !e || !pq->root || !e->next || !e->prev)
    {
        return NULL;
    }
    pq->root = delete_node(pq, e);
    pq->count--;
    clear_node(e);
    return struct_base(pq, e);
}

ccc_result
ccc_pq_erase(ccc_priority_queue *const pq, ccc_pq_elem *const e)
{
    if (!pq || !e || !pq->root || !e->next || !e->prev)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    pq->root = delete_node(pq, e);
    pq->count--;
    if (pq->alloc)
    {
        (void)pq->alloc(struct_base(pq, e), 0, pq->aux);
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_pq_clear(ccc_priority_queue *const pq, ccc_any_type_destructor_fn *const fn)
{
    if (!pq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    while (!ccc_pq_is_empty(pq))
    {
        if (fn)
        {
            fn((ccc_any_type){
                .any_type = ccc_pq_front(pq),
                .aux = pq->aux,
            });
        }
        (void)ccc_pq_pop(pq);
    }
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_pq_is_empty(ccc_priority_queue const *const pq)
{
    if (!pq)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !pq->count;
}

ccc_ucount
ccc_pq_count(ccc_priority_queue const *const pq)
{
    if (!pq)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = pq->count};
}

/* This is a difficult function. Without knowing if this new value is greater
   or less than the previous we must always perform a delete and reinsert if
   the value has not broken total order with the parent. It is not sufficient
   to check if the value has exceeded the value of the first left child as
   any sibling of that left child may be bigger than or smaller than that
   left child value. */
void *
ccc_pq_update(ccc_priority_queue *const pq, ccc_pq_elem *const e,
              ccc_any_type_update_fn *const fn, void *const aux)
{
    if (!pq || !e || !fn || !e->next || !e->prev)
    {
        return NULL;
    }
    update_fixup(pq, e, fn, aux);
    return struct_base(pq, e);
}

/* Preferable to use this function if it is known the value is increasing.
   Much more efficient. */
void *
ccc_pq_increase(ccc_priority_queue *const pq, ccc_pq_elem *const e,
                ccc_any_type_update_fn *const fn, void *const aux)
{
    if (!pq || !e || !fn || !e->next || !e->prev)
    {
        return NULL;
    }
    increase_fixup(pq, e, fn, aux);
    return struct_base(pq, e);
}

/* Preferable to use this function if it is known the value is decreasing.
   Much more efficient. */
void *
ccc_pq_decrease(ccc_priority_queue *const pq, ccc_pq_elem *const e,
                ccc_any_type_update_fn *const fn, void *const aux)
{
    if (!pq || !e || !fn || !e->next || !e->prev)
    {
        return NULL;
    }
    decrease_fixup(pq, e, fn, aux);
    return struct_base(pq, e);
}

ccc_tribool
ccc_pq_validate(ccc_priority_queue const *const pq)
{
    if (!pq || (pq->root && pq->root->parent))
    {
        return CCC_FALSE;
    }
    if (!has_valid_links(pq, NULL, pq->root))
    {
        return CCC_FALSE;
    }
    if (traversal_count(pq->root) != pq->count)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

ccc_threeway_cmp
ccc_pq_order(ccc_priority_queue const *const pq)
{
    return pq ? pq->order : CCC_CMP_ERROR;
}

/*=========================  Private Interface     ==========================*/

void
ccc_impl_pq_push(struct ccc_pq *const pq, struct ccc_pq_elem *const e)
{
    (void)ccc_pq_push(pq, e);
}

struct ccc_pq_elem *
ccc_impl_pq_elem_in(struct ccc_pq const *const pq, void const *const any_struct)
{
    return elem_in(pq, any_struct);
}

ccc_threeway_cmp
ccc_impl_pq_cmp(struct ccc_pq const *const pq,
                struct ccc_pq_elem const *const lhs,
                struct ccc_pq_elem const *const rhs)
{
    return cmp(pq, lhs, rhs);
}

struct ccc_pq_elem *
ccc_impl_pq_merge(struct ccc_pq *const pq, struct ccc_pq_elem *const old,
                  struct ccc_pq_elem *const new)
{
    return merge(pq, old, new);
}

void
ccc_impl_pq_cut_child(struct ccc_pq_elem *const child)
{
    cut_child(child);
}

void
ccc_impl_pq_init_node(struct ccc_pq_elem *const child)
{
    init_node(child);
}

struct ccc_pq_elem *
ccc_impl_pq_delete_node(struct ccc_pq *const pq, struct ccc_pq_elem *const root)
{
    return delete_node(pq, root);
}

void *
ccc_impl_pq_struct_base(struct ccc_pq const *const pq,
                        struct ccc_pq_elem const *const e)
{
    return struct_base(pq, e);
}

/*========================   Static Helpers  ================================*/

static void
update_fixup(struct ccc_pq *const pq, struct ccc_pq_elem *const e,
             ccc_any_type_update_fn *const fn, void *aux)
{
    if (e->parent && cmp(pq, e, e->parent) == pq->order)
    {
        cut_child(e);
        fn((ccc_any_type){
            .any_type = struct_base(pq, e),
            .aux = aux,
        });
        pq->root = merge(pq, pq->root, e);
        return;
    }
    pq->root = delete_node(pq, e);
    init_node(e);
    fn((ccc_any_type){
        .any_type = struct_base(pq, e),
        .aux = aux,
    });
    pq->root = merge(pq, pq->root, e);
}

static void
increase_fixup(struct ccc_pq *const pq, struct ccc_pq_elem *const e,
               ccc_any_type_update_fn *const fn, void *aux)
{
    if (pq->order == CCC_GRT)
    {
        cut_child(e);
    }
    else
    {
        pq->root = delete_node(pq, e);
        init_node(e);
    }
    fn((ccc_any_type){
        .any_type = struct_base(pq, e),
        .aux = aux,
    });
    pq->root = merge(pq, pq->root, e);
}

static void
decrease_fixup(struct ccc_pq *const pq, struct ccc_pq_elem *const e,
               ccc_any_type_update_fn *const fn, void *aux)
{
    if (pq->order == CCC_LES)
    {
        cut_child(e);
    }
    else
    {
        pq->root = delete_node(pq, e);
        init_node(e);
    }
    fn((ccc_any_type){
        .any_type = struct_base(pq, e),
        .aux = aux,
    });
    pq->root = merge(pq, pq->root, e);
}

static void
cut_child(struct ccc_pq_elem *const child)
{
    child->next->prev = child->prev;
    child->prev->next = child->next;
    if (child->parent && child == child->parent->child)
    {
        child->parent->child = child->next == child ? NULL : child->next;
    }
    child->parent = NULL;
}

static struct ccc_pq_elem *
delete_node(struct ccc_pq *const pq, struct ccc_pq_elem *const root)
{
    if (pq->root == root)
    {
        return delete_min(pq, root);
    }
    cut_child(root);
    return merge(pq, pq->root, delete_min(pq, root));
}

/* Uses Fredman et al. oldest to youngest pairing method mentioned on pg 124
of the paper to pair nodes in one pass. Of all the variants for pairing given
in the paper this one is the back-to-front variant and the only one for which
the runtime analysis holds identically to the two-pass standard variant. A
non-trivial example for min heap.

```
< = next_sibling
> = prev_sibling

  ┌<1>┐
  └/──┘
┌<9>─<1>─<9>─<7>─<8>┐
└───────────────────┘
        |
        v
┌<9>─<1>─<7>─<8>┐
└────────/──────┘
      ┌<9>┐
      └───┘
        |
        v
┌<9>─<1>─<7>┐
└────────/──┘
      ┌<8>─<9>┐
      └───────┘
        |
        v
  ┌<1>─<7>┐
  └/────/─┘
┌<9>┐┌<8>─<9>┐
└───┘└───────┘
        |
        v
    ┌<1>┐
    └/──┘
  ┌<7>─<9>┐
  └/──────┘
┌<8>─<9>┐
└───────┘
```

Delete min is the slowest operation offered by the priority queue and in
part contributes to the amortized `o(log(N))` runtime of the decrease key
operation. */
static struct ccc_pq_elem *
delete_min(struct ccc_pq *const pq, struct ccc_pq_elem *root)
{
    if (!root->child)
    {
        return NULL;
    }
    struct ccc_pq_elem *const eldest = root->child->next;
    struct ccc_pq_elem *accumulator = root->child->next;
    struct ccc_pq_elem *cur = root->child->next->next;
    while (cur != eldest && cur->next != eldest)
    {
        struct ccc_pq_elem *const next = cur->next;
        struct ccc_pq_elem *const next_cur = cur->next->next;
        next->next = next->prev = NULL;
        cur->next = cur->prev = NULL;
        /* Double merge ensures `O(log(N))` steps rather than O(N). */
        accumulator = merge(pq, accumulator, merge(pq, cur, next));
        cur = next_cur;
    }
    /* This covers the odd or even case for number of pairings. */
    root = cur == eldest ? accumulator : merge(pq, accumulator, cur);
    /* The root is always alone in its circular list at the end of merges. */
    root->next = root->prev = root;
    root->parent = NULL;
    return root;
}

/** Merges two priority queues, making the winner by ordering the root and
pushing the loser to the left child ring. Old should be the element that has
been in the queue longer and new, newer. This algorithm will still work if this
argument ordering is not respected and it does not change runtime, but it is how
to comply with the strategy outlined in the Fredman et. al. paper. */
static struct ccc_pq_elem *
merge(struct ccc_pq *const pq, struct ccc_pq_elem *const old,
      struct ccc_pq_elem *const new)
{
    if (!old || !new || old == new)
    {
        return old ? old : new;
    }
    if (cmp(pq, new, old) == pq->order)
    {
        link_child(new, old);
        return new;
    }
    link_child(old, new);
    return old;
}

/* Oldest nodes shuffle down, new drops in to replace. This supports the
ring representation from Fredman et al., pg 125, fig 14 where the left
child's next pointer wraps to the last element in the list. The previous
pointer is to support faster deletes and decrease key operations.

< = next_sibling
> = prev_sibling

         A        A            A
        ╱        ╱            ╱
    ┌─<B>─┐  ┌─<C>──<B>─┐ ┌─<D>──<C>──<B>─┐
    └─────┘  └──────────┘ └───────────────┘

Pairing in the delete min phase would then start at B in this example and work
towards D. That is the oldest to youngest order mentioned in the paper and
helps set up the one-pass back-to-front variant mentioned in the paper allowing
the same runtime guarantees as the two pass standard pairing heap. */
static void
link_child(struct ccc_pq_elem *const parent, struct ccc_pq_elem *const child)
{
    if (parent->child)
    {
        child->next = parent->child->next;
        child->prev = parent->child;
        parent->child->next->prev = child;
        parent->child->next = child;
    }
    else
    {
        child->next = child->prev = child;
    }
    parent->child = child;
    child->parent = parent;
}

static inline ccc_threeway_cmp
cmp(struct ccc_pq const *const pq, struct ccc_pq_elem const *const lhs,
    struct ccc_pq_elem const *const rhs)
{
    return pq->cmp((ccc_any_type_cmp){.any_type_lhs = struct_base(pq, lhs),
                                      .any_type_rhs = struct_base(pq, rhs),
                                      .aux = pq->aux});
}

static inline void *
struct_base(struct ccc_pq const *const pq, struct ccc_pq_elem const *const e)
{
    return ((char *)&(e->child)) - pq->pq_elem_offset;
}

static inline void *
elem_in(struct ccc_pq const *const pq, struct ccc_pq_elem const *const e)
{
    return ((char *)&(e->child)) + pq->pq_elem_offset;
}

static inline void
init_node(struct ccc_pq_elem *const e)
{
    e->child = e->parent = NULL;
    e->next = e->prev = e;
}

static inline void
clear_node(struct ccc_pq_elem *const e)
{
    e->child = e->next = e->prev = e->parent = NULL;
}

/*========================     Validation ================================*/

/* NOLINTBEGIN(*misc-no-recursion) */

static size_t
traversal_count(struct ccc_pq_elem const *const root)
{
    if (!root)
    {
        return 0;
    }
    size_t count = 0;
    struct ccc_pq_elem const *cur = root;
    do
    {
        count += 1 + traversal_count(cur->child);
    }
    while ((cur = cur->next) != root);
    return count;
}

static ccc_tribool
has_valid_links(struct ccc_pq const *const pq,
                struct ccc_pq_elem const *const parent,
                struct ccc_pq_elem const *const child)
{
    if (!child)
    {
        return CCC_TRUE;
    }
    struct ccc_pq_elem const *cur = child;
    ccc_threeway_cmp const wrong_order
        = pq->order == CCC_LES ? CCC_GRT : CCC_LES;
    do
    {
        /* Reminder: Don't combine these if checks into one. Separating them
           makes it easier to find the problem when stepping through gdb. */
        if (!cur)
        {
            return CCC_FALSE;
        }
        if (parent && child->parent != parent)
        {
            return CCC_FALSE;
        }
        if (parent && parent->child != child->parent->child)
        {
            return CCC_FALSE;
        }
        if (child->next->prev != child || child->prev->next != child)
        {
            return CCC_FALSE;
        }
        if (parent && (cmp(pq, parent, cur) == wrong_order))
        {
            return CCC_FALSE;
        }
        if (!has_valid_links(pq, cur, cur->child)) /* ! RECURSE ! */
        {
            return CCC_FALSE;
        }
    }
    while ((cur = cur->next) != child);
    return CCC_TRUE;
}

/* NOLINTEND(*misc-no-recursion) */
