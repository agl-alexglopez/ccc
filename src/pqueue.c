#include "pqueue.h"

#include <stdbool.h>
#include <stddef.h>

/*=========================  Function Prototypes   ==========================*/

static ccc_pq_elem *merge(ccc_pqueue *, ccc_pq_elem *old, ccc_pq_elem *new);
static void link_child(ccc_pq_elem *parent, ccc_pq_elem *child);
static void init_node(ccc_pq_elem *);
static size_t traversal_size(ccc_pq_elem const *);
static bool has_valid_links(ccc_pqueue const *, ccc_pq_elem const *parent,
                            ccc_pq_elem const *child);
static ccc_pq_elem *delete(ccc_pqueue *, ccc_pq_elem *);
static ccc_pq_elem *delete_min(ccc_pqueue *, ccc_pq_elem *);
static void clear_node(ccc_pq_elem *);
static void cut_child(ccc_pq_elem *);

/*=========================  Interface Functions   ==========================*/

ccc_pq_elem const *
ccc_pq_front(ccc_pqueue const *const ppq)
{
    return ppq->root;
}

void
ccc_pq_push(ccc_pqueue *const ppq, ccc_pq_elem *const e)
{
    if (!e || !ppq)
    {
        return;
    }
    init_node(e);
    ppq->root = merge(ppq, ppq->root, e);
    ++ppq->sz;
}

ccc_pq_elem *
ccc_pq_pop(ccc_pqueue *const ppq)
{
    if (!ppq->root)
    {
        return NULL;
    }
    ccc_pq_elem *const popped = ppq->root;
    ppq->root = delete_min(ppq, ppq->root);
    ppq->sz--;
    clear_node(popped);
    return popped;
}

ccc_pq_elem *
ccc_pq_erase(ccc_pqueue *const ppq, ccc_pq_elem *const e)
{
    if (!ppq->root || !e->next_sibling || !e->prev_sibling)
    {
        return NULL;
    }
    ppq->root = delete (ppq, e);
    ppq->sz--;
    clear_node(e);
    return e;
}

void
ccc_pq_clear(ccc_pqueue *const ppq, ccc_pq_destructor_fn *fn)
{
    while (!ccc_pq_empty(ppq))
    {
        fn(ccc_pq_pop(ppq));
    }
}

bool
ccc_pq_empty(ccc_pqueue const *const ppq)
{
    return !ppq->sz;
}

size_t
ccc_pq_size(ccc_pqueue const *const ppq)
{
    return ppq->sz;
}

/* This is a difficult function. Without knowing if this new value is greater
   or less than the previous we must always perform a delete and reinsert if
   the value has not broken total order with the parent. It is not sufficient
   to check if the value has exceeded the value of the first left child as
   any sibling of that left child may be bigger than or smaller than that
   left child value. */
bool
ccc_pq_update(ccc_pqueue *const ppq, ccc_pq_elem *const e,
              ccc_pq_update_fn *const fn, void *const aux)
{
    if (!e->next_sibling || !e->prev_sibling)
    {
        return false;
    }
    fn(e, aux);
    if (e->parent && ppq->cmp(e, e->parent, ppq->aux) == ppq->order)
    {
        cut_child(e);
        ppq->root = merge(ppq, ppq->root, e);
        return true;
    }
    ppq->root = delete (ppq, e);
    init_node(e);
    ppq->root = merge(ppq, ppq->root, e);
    return true;
}

/* Preferable to use this function if it is known the value is increasing.
   Much more efficient. */
bool
ccc_pq_increase(ccc_pqueue *const ppq, ccc_pq_elem *const e,
                ccc_pq_update_fn *fn, void *aux)
{
    if (!e->next_sibling || !e->prev_sibling)
    {
        return false;
    }
    if (ppq->order == CCC_PQ_GRT)
    {
        fn(e, aux);
        cut_child(e);
    }
    else
    {
        ppq->root = delete (ppq, e);
        fn(e, aux);
        init_node(e);
    }
    ppq->root = merge(ppq, ppq->root, e);
    return true;
}

/* Preferable to use this function if it is known the value is decreasing.
   Much more efficient. */
bool
ccc_pq_decrease(ccc_pqueue *const ppq, ccc_pq_elem *const e,
                ccc_pq_update_fn *fn, void *aux)
{
    if (!e->next_sibling || !e->prev_sibling)
    {
        return false;
    }
    if (ppq->order == CCC_PQ_LES)
    {
        fn(e, aux);
        cut_child(e);
    }
    else
    {
        ppq->root = delete (ppq, e);
        fn(e, aux);
        init_node(e);
    }
    ppq->root = merge(ppq, ppq->root, e);
    return true;
}

bool
ccc_pq_validate(ccc_pqueue const *const ppq)
{
    if (ppq->root && ppq->root->parent)
    {
        return false;
    }
    if (!has_valid_links(ppq, NULL, ppq->root))
    {
        return false;
    }
    if (traversal_size(ppq->root) != ppq->sz)
    {
        return false;
    }
    return true;
}

ccc_pq_threeway_cmp
ccc_pq_order(ccc_pqueue const *const ppq)
{
    return ppq->order;
}

/*========================   Static Helpers   ================================*/

static void
init_node(ccc_pq_elem *e)
{
    e->left_child = e->parent = NULL;
    e->next_sibling = e->prev_sibling = e;
}

static void
clear_node(ccc_pq_elem *e)
{
    e->left_child = e->next_sibling = e->prev_sibling = e->parent = NULL;
}

static void
cut_child(ccc_pq_elem *child)
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

static ccc_pq_elem *delete(ccc_pqueue *ppq, ccc_pq_elem *root)
{
    if (ppq->root == root)
    {
        return delete_min(ppq, root);
    }
    cut_child(root);
    return merge(ppq, ppq->root, delete_min(ppq, root));
}

static ccc_pq_elem *
delete_min(ccc_pqueue *ppq, ccc_pq_elem *root)
{
    if (!root->left_child)
    {
        return NULL;
    }
    ccc_pq_elem *const eldest = root->left_child->next_sibling;
    ccc_pq_elem *accumulator = root->left_child->next_sibling;
    ccc_pq_elem *cur = root->left_child->next_sibling->next_sibling;
    while (cur != eldest && cur->next_sibling != eldest)
    {
        ccc_pq_elem *next = cur->next_sibling;
        ccc_pq_elem *next_cur = cur->next_sibling->next_sibling;
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

static inline ccc_pq_elem *
merge(ccc_pqueue *const ppq, ccc_pq_elem *const old, ccc_pq_elem *const new)
{
    if (!old || !new || old == new)
    {
        return old ? old : new;
    }
    if (ppq->cmp(new, old, ppq->aux) == ppq->order)
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
link_child(ccc_pq_elem *const parent, ccc_pq_elem *const child)
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
traversal_size(ccc_pq_elem const *const root)
{
    if (!root)
    {
        return 0;
    }
    size_t sz = 0;
    bool sibling_ring_lapped = false;
    ccc_pq_elem const *cur = root;
    while (!sibling_ring_lapped)
    {
        sz += 1 + traversal_size(cur->left_child);
        cur = cur->next_sibling;
        sibling_ring_lapped = cur == root;
    }
    return sz;
}

static bool
has_valid_links(ccc_pqueue const *const ppq, ccc_pq_elem const *const parent,
                ccc_pq_elem const *const child)
{
    if (!child)
    {
        return true;
    }
    bool sibling_ring_lapped = false;
    ccc_pq_elem const *cur = child;
    ccc_pq_threeway_cmp const wrong_order = ppq->order == CCC_PQ_LES ? CCC_PQ_GRT : CCC_PQ_LES;
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
        if (parent && (ppq->cmp(parent, cur, ppq->aux) == wrong_order))
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
