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

static struct ccc_pq_elem_ *merge(struct ccc_pq_ *, struct ccc_pq_elem_ *old,
                                  struct ccc_pq_elem_ *new);
static void link_child(struct ccc_pq_elem_ *parent, struct ccc_pq_elem_ *child);
static void init_node(struct ccc_pq_elem_ *);
static size_t traversal_size(struct ccc_pq_elem_ const *);
static ccc_tribool has_valid_links(struct ccc_pq_ const *,
                                   struct ccc_pq_elem_ const *parent,
                                   struct ccc_pq_elem_ const *child);
static struct ccc_pq_elem_ *delete_node(struct ccc_pq_ *,
                                        struct ccc_pq_elem_ *);
static struct ccc_pq_elem_ *delete_min(struct ccc_pq_ *, struct ccc_pq_elem_ *);
static void clear_node(struct ccc_pq_elem_ *);
static void cut_child(struct ccc_pq_elem_ *);
static void *struct_base(struct ccc_pq_ const *, struct ccc_pq_elem_ const *e);
static void *elem_in(struct ccc_pq_ const *, struct ccc_pq_elem_ const *);
static ccc_threeway_cmp cmp(struct ccc_pq_ const *,
                            struct ccc_pq_elem_ const *a,
                            struct ccc_pq_elem_ const *b);
static void update_fixup(struct ccc_pq_ *, struct ccc_pq_elem_ *);
static void increase_fixup(struct ccc_pq_ *, struct ccc_pq_elem_ *);
static void decrease_fixup(struct ccc_pq_ *, struct ccc_pq_elem_ *);

/*=========================  Interface Functions   ==========================*/

void *
ccc_pq_front(ccc_priority_queue const *const pq)
{
    if (!pq)
    {
        return NULL;
    }
    return pq->root_ ? struct_base(pq, pq->root_) : NULL;
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
    if (pq->alloc_)
    {
        void *const node = pq->alloc_(NULL, pq->elem_sz_, pq->aux_);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, ret, pq->elem_sz_);
        ret = node;
        e = elem_in(pq, e);
    }
    pq->root_ = merge(pq, pq->root_, e);
    ++pq->sz_;
    return ret;
}

ccc_result
ccc_pq_pop(ccc_priority_queue *const pq)
{
    if (!pq || !pq->root_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    struct ccc_pq_elem_ *const popped = pq->root_;
    pq->root_ = delete_min(pq, pq->root_);
    pq->sz_--;
    clear_node(popped);
    if (pq->alloc_)
    {
        (void)pq->alloc_(struct_base(pq, popped), 0, pq->aux_);
    }
    return CCC_RESULT_OK;
}

void *
ccc_pq_extract(ccc_priority_queue *const pq, ccc_pq_elem *const e)
{
    if (!pq || !e || !pq->root_ || !e->next_sibling_ || !e->prev_sibling_)
    {
        return NULL;
    }
    pq->root_ = delete_node(pq, e);
    pq->sz_--;
    clear_node(e);
    return struct_base(pq, e);
}

ccc_result
ccc_pq_erase(ccc_priority_queue *const pq, ccc_pq_elem *const e)
{
    if (!pq || !e || !pq->root_ || !e->next_sibling_ || !e->prev_sibling_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    pq->root_ = delete_node(pq, e);
    pq->sz_--;
    if (pq->alloc_)
    {
        (void)pq->alloc_(struct_base(pq, e), 0, pq->aux_);
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_pq_clear(ccc_priority_queue *const pq, ccc_destructor_fn *const fn)
{
    if (!pq)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    while (!ccc_pq_is_empty(pq))
    {
        if (fn)
        {
            fn((ccc_user_type){.user_type = ccc_pq_front(pq), .aux = pq->aux_});
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
    return !pq->sz_;
}

ccc_ucount
ccc_pq_size(ccc_priority_queue const *const pq)
{
    if (!pq)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = pq->sz_};
}

/* This is a difficult function. Without knowing if this new value is greater
   or less than the previous we must always perform a delete and reinsert if
   the value has not broken total order with the parent. It is not sufficient
   to check if the value has exceeded the value of the first left child as
   any sibling of that left child may be bigger than or smaller than that
   left child value. */
ccc_tribool
ccc_pq_update(ccc_priority_queue *const pq, ccc_pq_elem *const e,
              ccc_update_fn *const fn, void *const aux)
{
    if (!pq || !e || !fn || !e->next_sibling_ || !e->prev_sibling_)
    {
        return CCC_TRIBOOL_ERROR;
    }
    fn((ccc_user_type){struct_base(pq, e), aux});
    update_fixup(pq, e);
    return CCC_TRUE;
}

/* Preferable to use this function if it is known the value is increasing.
   Much more efficient. */
ccc_tribool
ccc_pq_increase(ccc_priority_queue *const pq, ccc_pq_elem *const e,
                ccc_update_fn *const fn, void *const aux)
{
    if (!pq || !e || !fn || !e->next_sibling_ || !e->prev_sibling_)
    {
        return CCC_TRIBOOL_ERROR;
    }
    fn((ccc_user_type){struct_base(pq, e), aux});
    increase_fixup(pq, e);
    return CCC_TRUE;
}

/* Preferable to use this function if it is known the value is decreasing.
   Much more efficient. */
ccc_tribool
ccc_pq_decrease(ccc_priority_queue *const pq, ccc_pq_elem *const e,
                ccc_update_fn *const fn, void *const aux)
{
    if (!pq || !e || !fn || !e->next_sibling_ || !e->prev_sibling_)
    {
        return CCC_TRIBOOL_ERROR;
    }
    fn((ccc_user_type){struct_base(pq, e), aux});
    decrease_fixup(pq, e);
    return CCC_TRUE;
}

ccc_tribool
ccc_pq_validate(ccc_priority_queue const *const pq)
{
    if (!pq || (pq->root_ && pq->root_->parent_))
    {
        return CCC_FALSE;
    }
    if (!has_valid_links(pq, NULL, pq->root_))
    {
        return CCC_FALSE;
    }
    if (traversal_size(pq->root_) != pq->sz_)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

ccc_threeway_cmp
ccc_pq_order(ccc_priority_queue const *const pq)
{
    return pq ? pq->order_ : CCC_CMP_ERROR;
}

/*=========================  Private Interface     ==========================*/

void
ccc_impl_pq_push(struct ccc_pq_ *const pq, struct ccc_pq_elem_ *const e)
{
    (void)ccc_pq_push(pq, e);
}

struct ccc_pq_elem_ *
ccc_impl_pq_elem_in(struct ccc_pq_ const *const pq,
                    void const *const user_struct)
{
    return elem_in(pq, user_struct);
}

void
ccc_impl_pq_update_fixup(struct ccc_pq_ *const pq, struct ccc_pq_elem_ *const e)
{
    return update_fixup(pq, e);
}

void
ccc_impl_pq_increase_fixup(struct ccc_pq_ *const pq,
                           struct ccc_pq_elem_ *const e)
{
    return increase_fixup(pq, e);
}

void
ccc_impl_pq_decrease_fixup(struct ccc_pq_ *const pq,
                           struct ccc_pq_elem_ *const e)
{
    return decrease_fixup(pq, e);
}

/*========================   Static Helpers   ================================*/

static void
update_fixup(struct ccc_pq_ *const pq, struct ccc_pq_elem_ *const e)
{
    if (e->parent_ && cmp(pq, e, e->parent_) == pq->order_)
    {
        cut_child(e);
        pq->root_ = merge(pq, pq->root_, e);
        return;
    }
    pq->root_ = delete_node(pq, e);
    init_node(e);
    pq->root_ = merge(pq, pq->root_, e);
}

static void
increase_fixup(struct ccc_pq_ *const pq, struct ccc_pq_elem_ *const e)
{
    if (pq->order_ == CCC_GRT)
    {
        cut_child(e);
    }
    else
    {
        pq->root_ = delete_node(pq, e);
        init_node(e);
    }
    pq->root_ = merge(pq, pq->root_, e);
}

static void
decrease_fixup(struct ccc_pq_ *const pq, struct ccc_pq_elem_ *const e)
{
    if (pq->order_ == CCC_LES)
    {
        cut_child(e);
    }
    else
    {
        pq->root_ = delete_node(pq, e);
        init_node(e);
    }
    pq->root_ = merge(pq, pq->root_, e);
}

static void
cut_child(struct ccc_pq_elem_ *const child)
{
    child->next_sibling_->prev_sibling_ = child->prev_sibling_;
    child->prev_sibling_->next_sibling_ = child->next_sibling_;
    if (child->parent_ && child == child->parent_->left_child_)
    {
        child->parent_->left_child_
            = child->next_sibling_ == child ? NULL : child->next_sibling_;
    }
    child->parent_ = NULL;
}

static struct ccc_pq_elem_ *
delete_node(struct ccc_pq_ *const pq, struct ccc_pq_elem_ *const root)
{
    if (pq->root_ == root)
    {
        return delete_min(pq, root);
    }
    cut_child(root);
    return merge(pq, pq->root_, delete_min(pq, root));
}

/* Uses Fredman et al. oldest to youngest pairing method mentioned on pg 124 of
the paper to pair nodes in one pass. A non-trivial example for min heap.

< = next_sibling_
> = prev_sibling_

   ┌<1>┐
   └───┘
   /
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
     └───┘
     /
  ┌<7>─<9>┐
  └/──────┘
┌<8>─<9>┐
└───────┘

Delete min is the slowest operation offered by the priority queue and in part
contributes to the amortized o(lg n) runtime of the decrease key operation. */
static struct ccc_pq_elem_ *
delete_min(struct ccc_pq_ *const pq, struct ccc_pq_elem_ *root)
{
    if (!root->left_child_)
    {
        return NULL;
    }
    struct ccc_pq_elem_ *const eldest = root->left_child_->next_sibling_;
    struct ccc_pq_elem_ *accumulator = root->left_child_->next_sibling_;
    struct ccc_pq_elem_ *cur = root->left_child_->next_sibling_->next_sibling_;
    while (cur != eldest && cur->next_sibling_ != eldest)
    {
        struct ccc_pq_elem_ *const next = cur->next_sibling_;
        struct ccc_pq_elem_ *const next_cur = cur->next_sibling_->next_sibling_;
        next->next_sibling_ = next->prev_sibling_ = NULL;
        cur->next_sibling_ = cur->prev_sibling_ = NULL;
        accumulator = merge(pq, accumulator, merge(pq, cur, next));
        cur = next_cur;
    }
    /* This covers the odd or even case for number of pairings. */
    root = cur == eldest ? accumulator : merge(pq, accumulator, cur);
    /* The root is always alone in its circular list at the end of merges. */
    root->next_sibling_ = root->prev_sibling_ = root;
    root->parent_ = NULL;
    return root;
}

static struct ccc_pq_elem_ *
merge(struct ccc_pq_ *const pq, struct ccc_pq_elem_ *const old,
      struct ccc_pq_elem_ *const new)
{
    if (!old || !new || old == new)
    {
        return old ? old : new;
    }
    if (cmp(pq, new, old) == pq->order_)
    {
        link_child(new, old);
        return new;
    }
    link_child(old, new);
    return old;
}

/* Oldest nodes shuffle down, new drops in to replace. This supports the ring
representation from Fredman et al., pg 125, fig 14 where the left child's next
pointer wraps to the last element in the list. The previous pointer is to
support faster deletes and decrease key operations.

< = next_sibling_
> = prev_sibling_

         A        A            A
        ╱        ╱            ╱
    ┌─<B>─┐  ┌─<C>──<B>─┐ ┌─<D>──<C>──<B>─┐
    └─────┘  └──────────┘ └───────────────┘

Pairing in the delete min phase would then start B in this example and work
towards D. That is the oldest to youngest order mentioned in the paper. */
static void
link_child(struct ccc_pq_elem_ *const parent, struct ccc_pq_elem_ *const child)
{
    if (parent->left_child_)
    {
        child->next_sibling_ = parent->left_child_->next_sibling_;
        child->prev_sibling_ = parent->left_child_;
        parent->left_child_->next_sibling_->prev_sibling_ = child;
        parent->left_child_->next_sibling_ = child;
    }
    else
    {
        child->next_sibling_ = child->prev_sibling_ = child;
    }
    parent->left_child_ = child;
    child->parent_ = parent;
}

static inline ccc_threeway_cmp
cmp(struct ccc_pq_ const *const pq, struct ccc_pq_elem_ const *const a,
    struct ccc_pq_elem_ const *const b)
{
    return pq->cmp_((ccc_cmp){.user_type_lhs = struct_base(pq, a),
                              .user_type_rhs = struct_base(pq, b),
                              .aux = pq->aux_});
}

static inline void *
struct_base(struct ccc_pq_ const *const pq, struct ccc_pq_elem_ const *const e)
{
    return ((char *)&(e->left_child_)) - pq->pq_elem_offset_;
}

static inline void *
elem_in(struct ccc_pq_ const *const pq, struct ccc_pq_elem_ const *const e)
{
    return ((char *)&(e->left_child_)) + pq->pq_elem_offset_;
}

static inline void
init_node(struct ccc_pq_elem_ *const e)
{
    e->left_child_ = e->parent_ = NULL;
    e->next_sibling_ = e->prev_sibling_ = e;
}

static inline void
clear_node(struct ccc_pq_elem_ *const e)
{
    e->left_child_ = e->next_sibling_ = e->prev_sibling_ = e->parent_ = NULL;
}

/*========================     Validation    ================================*/

/* NOLINTBEGIN(*misc-no-recursion) */

static size_t
traversal_size(struct ccc_pq_elem_ const *const root)
{
    if (!root)
    {
        return 0;
    }
    size_t sz = 0;
    struct ccc_pq_elem_ const *cur = root;
    do
    {
        sz += 1 + traversal_size(cur->left_child_);
    } while ((cur = cur->next_sibling_) != root);
    return sz;
}

static ccc_tribool
has_valid_links(struct ccc_pq_ const *const pq,
                struct ccc_pq_elem_ const *const parent,
                struct ccc_pq_elem_ const *const child)
{
    if (!child)
    {
        return CCC_TRUE;
    }
    struct ccc_pq_elem_ const *cur = child;
    ccc_threeway_cmp const wrong_order
        = pq->order_ == CCC_LES ? CCC_GRT : CCC_LES;
    do
    {
        /* Reminder: Don't combine these if checks into one. Separating them
           makes it easier to find the problem when stepping through gdb. */
        if (!cur)
        {
            return CCC_FALSE;
        }
        if (parent && child->parent_ != parent)
        {
            return CCC_FALSE;
        }
        if (parent && parent->left_child_ != child->parent_->left_child_)
        {
            return CCC_FALSE;
        }
        if (child->next_sibling_->prev_sibling_ != child
            || child->prev_sibling_->next_sibling_ != child)
        {
            return CCC_FALSE;
        }
        if (parent && (cmp(pq, parent, cur) == wrong_order))
        {
            return CCC_FALSE;
        }
        if (!has_valid_links(pq, cur, cur->left_child_)) /* ! RECURSE ! */
        {
            return CCC_FALSE;
        }
    } while ((cur = cur->next_sibling_) != child);
    return CCC_TRUE;
}

/* NOLINTEND(*misc-no-recursion) */
