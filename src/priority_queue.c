#include "priority_queue.h"
#include "impl_priority_queue.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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

/*=========================  Interface Functions   ==========================*/

void *
ccc_pq_front(ccc_priority_queue const *const ppq)
{
    return ppq->impl_.root ? struct_base(&ppq->impl_, ppq->impl_.root) : NULL;
}

void
ccc_pq_push(ccc_priority_queue *const ppq, ccc_pq_elem *const e)
{
    if (!e || !ppq)
    {
        return;
    }
    init_node(&e->impl_);
    if (ppq->impl_.alloc)
    {
        void *node = ppq->impl_.alloc(NULL, ppq->impl_.elem_sz);
        if (!node)
        {
            return;
        }
        memcpy(node, struct_base(&ppq->impl_, &e->impl_), ppq->impl_.elem_sz);
    }
    ppq->impl_.root = merge(&ppq->impl_, ppq->impl_.root, &e->impl_);
    ++ppq->impl_.sz;
}

void
ccc_pq_pop(ccc_priority_queue *const ppq)
{
    if (!ppq->impl_.root)
    {
        return;
    }
    struct ccc_pq_elem_ *const popped = ppq->impl_.root;
    ppq->impl_.root = delete_min(&ppq->impl_, ppq->impl_.root);
    ppq->impl_.sz--;
    clear_node(popped);
    if (ppq->impl_.alloc)
    {
        ppq->impl_.alloc(struct_base(&ppq->impl_, popped), 0);
    }
}

void
ccc_pq_erase(ccc_priority_queue *const ppq, ccc_pq_elem *const e)
{
    if (!ppq->impl_.root || !e->impl_.next_sibling || !e->impl_.prev_sibling)
    {
        return;
    }
    ppq->impl_.root = delete (&ppq->impl_, &e->impl_);
    ppq->impl_.sz--;
    clear_node(&e->impl_);
    if (ppq->impl_.alloc)
    {
        ppq->impl_.alloc(struct_base(&ppq->impl_, &e->impl_), 0);
    }
}

void
ccc_pq_clear(ccc_priority_queue *const ppq, ccc_destructor_fn *fn)
{
    while (!ccc_pq_empty(ppq))
    {
        fn(ccc_pq_front(ppq));
        ccc_pq_pop(ppq);
    }
}

bool
ccc_pq_empty(ccc_priority_queue const *const ppq)
{
    return !ppq->impl_.sz;
}

size_t
ccc_pq_size(ccc_priority_queue const *const ppq)
{
    return ppq->impl_.sz;
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
    if (!e->impl_.next_sibling || !e->impl_.prev_sibling)
    {
        return false;
    }
    fn((ccc_update){struct_base(&ppq->impl_, &e->impl_), aux});
    if (e->impl_.parent
        && cmp(&ppq->impl_, &e->impl_, e->impl_.parent) == ppq->impl_.order)
    {
        cut_child(&e->impl_);
        ppq->impl_.root = merge(&ppq->impl_, ppq->impl_.root, &e->impl_);
        return true;
    }
    ppq->impl_.root = delete (&ppq->impl_, &e->impl_);
    init_node(&e->impl_);
    ppq->impl_.root = merge(&ppq->impl_, ppq->impl_.root, &e->impl_);
    return true;
}

/* Preferable to use this function if it is known the value is increasing.
   Much more efficient. */
bool
ccc_pq_increase(ccc_priority_queue *const ppq, ccc_pq_elem *const e,
                ccc_update_fn *fn, void *aux)
{
    if (!e->impl_.next_sibling || !e->impl_.prev_sibling)
    {
        return false;
    }
    if (ppq->impl_.order == CCC_GRT)
    {
        fn((ccc_update){struct_base(&ppq->impl_, &e->impl_), aux});
        cut_child(&e->impl_);
    }
    else
    {
        ppq->impl_.root = delete (&ppq->impl_, &e->impl_);
        fn((ccc_update){struct_base(&ppq->impl_, &e->impl_), aux});
        init_node(&e->impl_);
    }
    ppq->impl_.root = merge(&ppq->impl_, ppq->impl_.root, &e->impl_);
    return true;
}

/* Preferable to use this function if it is known the value is decreasing.
   Much more efficient. */
bool
ccc_pq_decrease(ccc_priority_queue *const ppq, ccc_pq_elem *const e,
                ccc_update_fn *fn, void *aux)
{
    if (!e->impl_.next_sibling || !e->impl_.prev_sibling)
    {
        return false;
    }
    if (ppq->impl_.order == CCC_LES)
    {
        fn((ccc_update){struct_base(&ppq->impl_, &e->impl_), aux});
        cut_child(&e->impl_);
    }
    else
    {
        ppq->impl_.root = delete (&ppq->impl_, &e->impl_);
        fn((ccc_update){struct_base(&ppq->impl_, &e->impl_), aux});
        init_node(&e->impl_);
    }
    ppq->impl_.root = merge(&ppq->impl_, ppq->impl_.root, &e->impl_);
    return true;
}

bool
ccc_pq_validate(ccc_priority_queue const *const ppq)
{
    if (ppq->impl_.root && ppq->impl_.root->parent)
    {
        return false;
    }
    if (!has_valid_links(&ppq->impl_, NULL, ppq->impl_.root))
    {
        return false;
    }
    if (traversal_size(ppq->impl_.root) != ppq->impl_.sz)
    {
        return false;
    }
    return true;
}

ccc_threeway_cmp
ccc_pq_order(ccc_priority_queue const *const ppq)
{
    return ppq->impl_.order;
}

/*========================   Static Helpers   ================================*/

static inline ccc_threeway_cmp
cmp(struct ccc_pq_ const *const ppq, struct ccc_pq_elem_ const *const a,
    struct ccc_pq_elem_ const *const b)
{
    return ppq->cmp(
        (ccc_cmp){struct_base(ppq, a), struct_base(ppq, b), ppq->aux});
}

static inline void *
struct_base(struct ccc_pq_ const *const pq, struct ccc_pq_elem_ const *e)
{
    return ((uint8_t *)&(e->left_child)) - pq->pq_elem_offset;
}

static inline void
init_node(struct ccc_pq_elem_ *e)
{
    e->left_child = e->parent = NULL;
    e->next_sibling = e->prev_sibling = e;
}

static inline void
clear_node(struct ccc_pq_elem_ *e)
{
    e->left_child = e->next_sibling = e->prev_sibling = e->parent = NULL;
}

static inline void
cut_child(struct ccc_pq_elem_ *child)
{
    child->next_sibling->prev_sibling = child->prev_sibling;
    child->prev_sibling->next_sibling = child->next_sibling;
    if (child->parent && child == child->parent->left_child)
    {
        if (child->next_sibling == child)
        {
            child->parent->left_child = NULL;
        }
        else
        {
            child->parent->left_child = child->next_sibling;
        }
    }
    child->parent = NULL;
}

static inline struct ccc_pq_elem_ *delete(struct ccc_pq_ *ppq,
                                          struct ccc_pq_elem_ *root)
{
    if (ppq->root == root)
    {
        return delete_min(ppq, root);
    }
    cut_child(root);
    return merge(ppq, ppq->root, delete_min(ppq, root));
}

static inline struct ccc_pq_elem_ *
delete_min(struct ccc_pq_ *ppq, struct ccc_pq_elem_ *root)
{
    if (!root->left_child)
    {
        return NULL;
    }
    struct ccc_pq_elem_ *const eldest = root->left_child->next_sibling;
    struct ccc_pq_elem_ *accumulator = root->left_child->next_sibling;
    struct ccc_pq_elem_ *cur = root->left_child->next_sibling->next_sibling;
    while (cur != eldest && cur->next_sibling != eldest)
    {
        struct ccc_pq_elem_ *next = cur->next_sibling;
        struct ccc_pq_elem_ *next_cur = cur->next_sibling->next_sibling;
        next->next_sibling = next->prev_sibling = NULL;
        cur->next_sibling = cur->prev_sibling = NULL;
        accumulator = merge(ppq, accumulator, merge(ppq, cur, next));
        cur = next_cur;
    }
    /* This covers the odd or even case for number of pairings. */
    root = cur != eldest ? merge(ppq, accumulator, cur) : accumulator;
    /* The root is always alone in its circular list at the end of merges. */
    root->next_sibling = root->prev_sibling = root;
    root->parent = NULL;
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
    if (cmp(ppq, new, old) == ppq->order)
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
    if (parent->left_child)
    {
        child->next_sibling = parent->left_child->next_sibling;
        child->prev_sibling = parent->left_child;
        parent->left_child->next_sibling->prev_sibling = child;
        parent->left_child->next_sibling = child;
    }
    else
    {
        child->next_sibling = child->prev_sibling = child;
    }
    parent->left_child = child;
    child->parent = parent;
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
        sz += 1 + traversal_size(cur->left_child);
        cur = cur->next_sibling;
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
    bool sibling_ring_lapped = false;
    struct ccc_pq_elem_ const *cur = child;
    ccc_threeway_cmp const wrong_order
        = ppq->order == CCC_LES ? CCC_GRT : CCC_LES;
    while (!sibling_ring_lapped)
    {
        if (!cur)
        {
            return false;
        }
        if (parent && child->parent != parent)
        {
            return false;
        }
        if (parent && parent->left_child != child->parent->left_child)
        {
            return false;
        }
        if (child->next_sibling->prev_sibling != child
            || child->prev_sibling->next_sibling != child)
        {
            return false;
        }
        if (parent && (cmp(ppq, parent, cur) == wrong_order))
        {
            return false;
        }
        if (!has_valid_links(ppq, cur, cur->left_child))
        {
            return false;
        }
        cur = cur->next_sibling;
        sibling_ring_lapped = cur == child;
    }
    return true;
}

/* NOLINTEND(*misc-no-recursion) */
