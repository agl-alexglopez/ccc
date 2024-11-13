#include "priority_queue.h"
#include "impl_priority_queue.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/*=========================  Function Prototypes   ==========================*/

static struct ccc_pq_elem_ *merge(struct ccc_pq_ *, struct ccc_pq_elem_ *old,
                                  struct ccc_pq_elem_ *new);
static void link_child(struct ccc_pq_elem_ *parent, struct ccc_pq_elem_ *child);
static void init_node(struct ccc_pq_elem_ *);
static size_t traversal_size(struct ccc_pq_elem_ const *);
static bool has_valid_links(struct ccc_pq_ const *,
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

ccc_result
ccc_pq_push(ccc_priority_queue *const pq, ccc_pq_elem *e)
{
    if (!e || !pq)
    {
        return CCC_INPUT_ERR;
    }
    init_node(e);
    if (pq->alloc_)
    {
        void *const node = pq->alloc_(NULL, pq->elem_sz_);
        if (!node)
        {
            return CCC_MEM_ERR;
        }
        (void)memcpy(node, struct_base(pq, e), pq->elem_sz_);
        e = elem_in(pq, e);
    }
    pq->root_ = merge(pq, pq->root_, e);
    ++pq->sz_;
    return CCC_OK;
}

ccc_result
ccc_pq_pop(ccc_priority_queue *const pq)
{
    if (!pq || !pq->root_)
    {
        return CCC_INPUT_ERR;
    }
    struct ccc_pq_elem_ *const popped = pq->root_;
    pq->root_ = delete_min(pq, pq->root_);
    pq->sz_--;
    clear_node(popped);
    if (pq->alloc_)
    {
        (void)pq->alloc_(struct_base(pq, popped), 0);
    }
    return CCC_OK;
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
        return CCC_INPUT_ERR;
    }
    pq->root_ = delete_node(pq, e);
    pq->sz_--;
    if (pq->alloc_)
    {
        (void)pq->alloc_(struct_base(pq, e), 0);
    }
    return CCC_OK;
}

ccc_result
ccc_pq_clear(ccc_priority_queue *const pq, ccc_destructor_fn *const fn)
{
    if (!pq)
    {
        return CCC_INPUT_ERR;
    }
    while (!ccc_pq_is_empty(pq))
    {
        if (fn)
        {
            fn((ccc_user_type){.user_type = ccc_pq_front(pq), .aux = pq->aux_});
        }
        (void)ccc_pq_pop(pq);
    }
    return CCC_OK;
}

bool
ccc_pq_is_empty(ccc_priority_queue const *const pq)
{
    return pq ? !pq->sz_ : true;
}

size_t
ccc_pq_size(ccc_priority_queue const *const pq)
{
    return pq ? pq->sz_ : 0;
}

/* This is a difficult function. Without knowing if this new value is greater
   or less than the previous we must always perform a delete and reinsert if
   the value has not broken total order with the parent. It is not sufficient
   to check if the value has exceeded the value of the first left child as
   any sibling of that left child may be bigger than or smaller than that
   left child value. */
bool
ccc_pq_update(ccc_priority_queue *const pq, ccc_pq_elem *const e,
              ccc_update_fn *const fn, void *const aux)
{
    if (!pq || !e || !fn || !e->next_sibling_ || !e->prev_sibling_)
    {
        return false;
    }
    fn((ccc_user_type){struct_base(pq, e), aux});
    if (e->parent_ && cmp(pq, e, e->parent_) == pq->order_)
    {
        cut_child(e);
        pq->root_ = merge(pq, pq->root_, e);
        return true;
    }
    pq->root_ = delete_node(pq, e);
    init_node(e);
    pq->root_ = merge(pq, pq->root_, e);
    return true;
}

/* Preferable to use this function if it is known the value is increasing.
   Much more efficient. */
bool
ccc_pq_increase(ccc_priority_queue *const pq, ccc_pq_elem *const e,
                ccc_update_fn *const fn, void *const aux)
{
    if (!pq || !e || !fn || !e->next_sibling_ || !e->prev_sibling_)
    {
        return false;
    }
    if (pq->order_ == CCC_GRT)
    {
        fn((ccc_user_type){struct_base(pq, e), aux});
        cut_child(e);
    }
    else
    {
        pq->root_ = delete_node(pq, e);
        fn((ccc_user_type){struct_base(pq, e), aux});
        init_node(e);
    }
    pq->root_ = merge(pq, pq->root_, e);
    return true;
}

/* Preferable to use this function if it is known the value is decreasing.
   Much more efficient. */
bool
ccc_pq_decrease(ccc_priority_queue *const pq, ccc_pq_elem *const e,
                ccc_update_fn *const fn, void *const aux)
{
    if (!pq || !e || !fn || !e->next_sibling_ || !e->prev_sibling_)
    {
        return false;
    }
    if (pq->order_ == CCC_LES)
    {
        fn((ccc_user_type){struct_base(pq, e), aux});
        cut_child(e);
    }
    else
    {
        pq->root_ = delete_node(pq, e);
        fn((ccc_user_type){struct_base(pq, e), aux});
        init_node(e);
    }
    pq->root_ = merge(pq, pq->root_, e);
    return true;
}

bool
ccc_pq_validate(ccc_priority_queue const *const pq)
{
    if (!pq || (pq->root_ && pq->root_->parent_))
    {
        return false;
    }
    if (!has_valid_links(pq, NULL, pq->root_))
    {
        return false;
    }
    if (traversal_size(pq->root_) != pq->sz_)
    {
        return false;
    }
    return true;
}

ccc_threeway_cmp
ccc_pq_order(ccc_priority_queue const *const pq)
{
    return pq ? pq->order_ : CCC_CMP_ERR;
}

/*========================   Static Helpers   ================================*/

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

static inline void
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

static inline struct ccc_pq_elem_ *
delete_node(struct ccc_pq_ *const pq, struct ccc_pq_elem_ *const root)
{
    if (pq->root_ == root)
    {
        return delete_min(pq, root);
    }
    cut_child(root);
    return merge(pq, pq->root_, delete_min(pq, root));
}

static inline struct ccc_pq_elem_ *
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
    root = cur != eldest ? merge(pq, accumulator, cur) : accumulator;
    /* The root is always alone in its circular list at the end of merges. */
    root->next_sibling_ = root->prev_sibling_ = root;
    root->parent_ = NULL;
    return root;
}

static inline struct ccc_pq_elem_ *
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

/* Oldest nodes shuffle down, new drops in to replace.
         a       a       a
        ╱   ->  ╱   ->  ╱
      ┌b┐     ┌c─b┐   ┌d─c─b┐
      └─┘     └───┘   └─────┘ */
static inline void
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

static bool
has_valid_links(struct ccc_pq_ const *const pq,
                struct ccc_pq_elem_ const *const parent,
                struct ccc_pq_elem_ const *const child)
{
    if (!child)
    {
        return true;
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
            return false;
        }
        if (parent && child->parent_ != parent)
        {
            return false;
        }
        if (parent && parent->left_child_ != child->parent_->left_child_)
        {
            return false;
        }
        if (child->next_sibling_->prev_sibling_ != child
            || child->prev_sibling_->next_sibling_ != child)
        {
            return false;
        }
        if (parent && (cmp(pq, parent, cur) == wrong_order))
        {
            return false;
        }
        if (!has_valid_links(pq, cur, cur->left_child_)) /* ! RECURSE ! */
        {
            return false;
        }
    } while ((cur = cur->next_sibling_) != child);
    return true;
}

/* NOLINTEND(*misc-no-recursion) */
