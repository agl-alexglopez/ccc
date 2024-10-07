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
static struct ccc_pq_elem_ *delete(struct ccc_pq_ *, struct ccc_pq_elem_ *);
static struct ccc_pq_elem_ *delete_min(struct ccc_pq_ *, struct ccc_pq_elem_ *);
static void clear_node(struct ccc_pq_elem_ *);
static void cut_child(struct ccc_pq_elem_ *);
static void *struct_base(struct ccc_pq_ const *, struct ccc_pq_elem_ const *e);
static ccc_threeway_cmp cmp(struct ccc_pq_ const *,
                            struct ccc_pq_elem_ const *a,
                            struct ccc_pq_elem_ const *b);
static void print_pq(struct ccc_pq_ const *ppq, struct ccc_pq_elem_ const *root,
                     ccc_print_fn *fn);

/*=========================  Interface Functions   ==========================*/

void *
ccc_pq_front(ccc_priority_queue const *const ppq)
{
    return ppq->root_ ? struct_base(ppq, ppq->root_) : NULL;
}

void
ccc_pq_push(ccc_priority_queue *const ppq, ccc_pq_elem *const e)
{
    if (!e || !ppq)
    {
        return;
    }
    init_node(e);
    if (ppq->alloc_)
    {
        void *node = ppq->alloc_(NULL, ppq->elem_sz_);
        if (!node)
        {
            return;
        }
        (void)memcpy(node, struct_base(ppq, e), ppq->elem_sz_);
    }
    ppq->root_ = merge(ppq, ppq->root_, e);
    ++ppq->sz_;
}

void
ccc_pq_pop(ccc_priority_queue *const ppq)
{
    if (!ppq->root_)
    {
        return;
    }
    struct ccc_pq_elem_ *const popped = ppq->root_;
    ppq->root_ = delete_min(ppq, ppq->root_);
    ppq->sz_--;
    clear_node(popped);
    if (ppq->alloc_)
    {
        ppq->alloc_(struct_base(ppq, popped), 0);
    }
}

void
ccc_pq_erase(ccc_priority_queue *const ppq, ccc_pq_elem *const e)
{
    if (!ppq->root_ || !e->next_sibling_ || !e->prev_sibling_)
    {
        return;
    }
    ppq->root_ = delete (ppq, e);
    ppq->sz_--;
    clear_node(e);
    if (ppq->alloc_)
    {
        ppq->alloc_(struct_base(ppq, e), 0);
    }
}

void
ccc_pq_clear(ccc_priority_queue *const ppq, ccc_destructor_fn *fn)
{
    while (!ccc_pq_empty(ppq))
    {
        fn((ccc_user_type_mut){.user_type = ccc_pq_front(ppq),
                               .aux = ppq->aux_});
        ccc_pq_pop(ppq);
    }
}

bool
ccc_pq_empty(ccc_priority_queue const *const ppq)
{
    return !ppq->sz_;
}

size_t
ccc_pq_size(ccc_priority_queue const *const ppq)
{
    return ppq->sz_;
}

/* This is a difficult function. Without knowing if this new value is greater
   or less than the previous we must always perform a delete and reinsert if
   the value has not broken total order with the parent. It is not sufficient
   to check if the value has exceeded the value of the first left child as
   any sibling of that left child may be bigger than or smaller than that
   left child value. */
bool
ccc_pq_update(ccc_priority_queue *const ppq, ccc_pq_elem *const e,
              ccc_update_fn *const fn, void *const aux)
{
    if (!e->next_sibling_ || !e->prev_sibling_)
    {
        return false;
    }
    fn((ccc_user_type_mut){struct_base(ppq, e), aux});
    if (e->parent_ && cmp(ppq, e, e->parent_) == ppq->order_)
    {
        cut_child(e);
        ppq->root_ = merge(ppq, ppq->root_, e);
        return true;
    }
    ppq->root_ = delete (ppq, e);
    init_node(e);
    ppq->root_ = merge(ppq, ppq->root_, e);
    return true;
}

/* Preferable to use this function if it is known the value is increasing.
   Much more efficient. */
bool
ccc_pq_increase(ccc_priority_queue *const ppq, ccc_pq_elem *const e,
                ccc_update_fn *fn, void *aux)
{
    if (!e->next_sibling_ || !e->prev_sibling_)
    {
        return false;
    }
    if (ppq->order_ == CCC_GRT)
    {
        fn((ccc_user_type_mut){struct_base(ppq, e), aux});
        cut_child(e);
    }
    else
    {
        ppq->root_ = delete (ppq, e);
        fn((ccc_user_type_mut){struct_base(ppq, e), aux});
        init_node(e);
    }
    ppq->root_ = merge(ppq, ppq->root_, e);
    return true;
}

/* Preferable to use this function if it is known the value is decreasing.
   Much more efficient. */
bool
ccc_pq_decrease(ccc_priority_queue *const ppq, ccc_pq_elem *const e,
                ccc_update_fn *fn, void *aux)
{
    if (!e->next_sibling_ || !e->prev_sibling_)
    {
        return false;
    }
    if (ppq->order_ == CCC_LES)
    {
        fn((ccc_user_type_mut){struct_base(ppq, e), aux});
        cut_child(e);
    }
    else
    {
        ppq->root_ = delete (ppq, e);
        fn((ccc_user_type_mut){struct_base(ppq, e), aux});
        init_node(e);
    }
    ppq->root_ = merge(ppq, ppq->root_, e);
    return true;
}

bool
ccc_pq_validate(ccc_priority_queue const *const ppq)
{
    if (ppq->root_ && ppq->root_->parent_)
    {
        return false;
    }
    if (!has_valid_links(ppq, NULL, ppq->root_))
    {
        return false;
    }
    if (traversal_size(ppq->root_) != ppq->sz_)
    {
        return false;
    }
    return true;
}

void
ccc_pq_print(ccc_priority_queue const *const pq, ccc_print_fn *const fn)
{
    print_pq(pq, pq->root_, fn);
}

ccc_threeway_cmp
ccc_pq_order(ccc_priority_queue const *const ppq)
{
    return ppq->order_;
}

/*========================   Static Helpers   ================================*/

static inline ccc_threeway_cmp
cmp(struct ccc_pq_ const *const ppq, struct ccc_pq_elem_ const *const a,
    struct ccc_pq_elem_ const *const b)
{
    return ppq->cmp_((ccc_cmp){.user_type_a = struct_base(ppq, a),
                               .user_type_b = struct_base(ppq, b),
                               .aux = ppq->aux_});
}

static inline void *
struct_base(struct ccc_pq_ const *const pq, struct ccc_pq_elem_ const *e)
{
    return ((char *)&(e->left_child_)) - pq->pq_elem_offset_;
}

static inline void
init_node(struct ccc_pq_elem_ *e)
{
    e->left_child_ = e->parent_ = NULL;
    e->next_sibling_ = e->prev_sibling_ = e;
}

static inline void
clear_node(struct ccc_pq_elem_ *e)
{
    e->left_child_ = e->next_sibling_ = e->prev_sibling_ = e->parent_ = NULL;
}

static inline void
cut_child(struct ccc_pq_elem_ *child)
{
    child->next_sibling_->prev_sibling_ = child->prev_sibling_;
    child->prev_sibling_->next_sibling_ = child->next_sibling_;
    if (child->parent_ && child == child->parent_->left_child_)
    {
        if (child->next_sibling_ == child)
        {
            child->parent_->left_child_ = NULL;
        }
        else
        {
            child->parent_->left_child_ = child->next_sibling_;
        }
    }
    child->parent_ = NULL;
}

static inline struct ccc_pq_elem_ *delete(struct ccc_pq_ *ppq,
                                          struct ccc_pq_elem_ *root)
{
    if (ppq->root_ == root)
    {
        return delete_min(ppq, root);
    }
    cut_child(root);
    return merge(ppq, ppq->root_, delete_min(ppq, root));
}

static inline struct ccc_pq_elem_ *
delete_min(struct ccc_pq_ *ppq, struct ccc_pq_elem_ *root)
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
        struct ccc_pq_elem_ *next = cur->next_sibling_;
        struct ccc_pq_elem_ *next_cur = cur->next_sibling_->next_sibling_;
        next->next_sibling_ = next->prev_sibling_ = NULL;
        cur->next_sibling_ = cur->prev_sibling_ = NULL;
        accumulator = merge(ppq, accumulator, merge(ppq, cur, next));
        cur = next_cur;
    }
    /* This covers the odd or even case for number of pairings. */
    root = cur != eldest ? merge(ppq, accumulator, cur) : accumulator;
    /* The root is always alone in its circular list at the end of merges. */
    root->next_sibling_ = root->prev_sibling_ = root;
    root->parent_ = NULL;
    return root;
}

static inline struct ccc_pq_elem_ *
merge(struct ccc_pq_ *const ppq, struct ccc_pq_elem_ *const old,
      struct ccc_pq_elem_ *const new)
{
    if (!old || !new || old == new)
    {
        return old ? old : new;
    }
    if (cmp(ppq, new, old) == ppq->order_)
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
    bool sibling_ring_lapped = false;
    struct ccc_pq_elem_ const *cur = root;
    while (!sibling_ring_lapped)
    {
        sz += 1 + traversal_size(cur->left_child_);
        cur = cur->next_sibling_;
        sibling_ring_lapped = cur == root;
    }
    return sz;
}

static bool
has_valid_links(struct ccc_pq_ const *const ppq,
                struct ccc_pq_elem_ const *const parent,
                struct ccc_pq_elem_ const *const child)
{
    if (!child)
    {
        return true;
    }
    struct ccc_pq_elem_ const *cur = child;
    ccc_threeway_cmp const wrong_order
        = ppq->order_ == CCC_LES ? CCC_GRT : CCC_LES;
    do
    {
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
        if (parent && (cmp(ppq, parent, cur) == wrong_order))
        {
            return false;
        }
        if (!has_valid_links(ppq, cur, cur->left_child_)) /* ! RECURSE ! */
        {
            return false;
        }
    } while ((cur = cur->next_sibling_) != child);
    return true;
}

static void
print_pq(struct ccc_pq_ const *const ppq, struct ccc_pq_elem_ const *const root,
         ccc_print_fn *const fn)
{
    if (!root)
    {
        return;
    }
    struct ccc_pq_elem_ const *cur = root;
    do
    {
        fn((ccc_user_type){.user_type = struct_base(ppq, cur),
                           .aux = ppq->aux_});
        print_pq(ppq, cur->left_child_, fn);
    } while ((cur = cur->next_sibling_) != root);
}

/* NOLINTEND(*misc-no-recursion) */
