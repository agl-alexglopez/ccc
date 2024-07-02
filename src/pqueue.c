#include "pqueue.h"

/*=========================  Function Prototypes   ==========================*/

static struct pq_elem *fair_merge(struct pqueue *, struct pq_elem *old,
                                  struct pq_elem *new);
static void link_child(struct pq_elem *parent, struct pq_elem *child);
static void init_node(struct pq_elem *);
static size_t traversal_size(const struct pq_elem *);
static bool has_valid_links(const struct pqueue *, const struct pq_elem *parent,
                            const struct pq_elem *child);
static struct pq_elem *delete(struct pqueue *, struct pq_elem *);
static struct pq_elem *delete_min(struct pqueue *, struct pq_elem *);
static void clear_node(struct pq_elem *);
static void cut_child(struct pq_elem *);

/*=========================  Interface Functions   ==========================*/

const struct pq_elem *
pq_front(const struct pqueue *const ppq)
{
    return ppq->root;
}

void
pq_push(struct pqueue *const ppq, struct pq_elem *const e)
{
    if (!e || !ppq)
    {
        return;
    }
    init_node(e);
    ppq->root = fair_merge(ppq, ppq->root, e);
    ++ppq->sz;
}

struct pq_elem *
pq_pop(struct pqueue *const ppq)
{
    if (!ppq->root)
    {
        return NULL;
    }
    struct pq_elem *const popped = ppq->root;
    ppq->root = delete_min(ppq, ppq->root);
    ppq->sz--;
    clear_node(popped);
    return popped;
}

struct pq_elem *
pq_erase(struct pqueue *const ppq, struct pq_elem *const e)
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
pq_clear(struct pqueue *const ppq, pq_destructor_fn *fn)
{
    while (!pq_empty(ppq))
    {
        fn(pq_pop(ppq));
    }
}

bool
pq_empty(const struct pqueue *const ppq)
{
    return !ppq->sz;
}

size_t
pq_size(const struct pqueue *const ppq)
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
pq_update(struct pqueue *const ppq, struct pq_elem *const e,
          pq_update_fn *const fn, void *const aux)
{
    if (!e->next_sibling || !e->prev_sibling)
    {
        return false;
    }
    fn(e, aux);
    if (e->parent && ppq->cmp(e, e->parent, ppq->aux) == ppq->order)
    {
        cut_child(e);
        ppq->root = fair_merge(ppq, ppq->root, e);
        return true;
    }
    ppq->root = delete (ppq, e);
    init_node(e);
    ppq->root = fair_merge(ppq, ppq->root, e);
    return true;
}

/* Preferable to use this function if it is known the value is increasing.
   Much more efficient. */
bool
pq_increase(struct pqueue *const ppq, struct pq_elem *const e, pq_update_fn *fn,
            void *aux)
{
    if (!e->next_sibling || !e->prev_sibling)
    {
        return false;
    }
    if (ppq->order == PQGRT)
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
    ppq->root = fair_merge(ppq, ppq->root, e);
    return true;
}

/* Preferable to use this function if it is known the value is decreasing.
   Much more efficient. */
bool
pq_decrease(struct pqueue *const ppq, struct pq_elem *const e, pq_update_fn *fn,
            void *aux)
{
    if (!e->next_sibling || !e->prev_sibling)
    {
        return false;
    }
    if (ppq->order == PQLES)
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
    ppq->root = fair_merge(ppq, ppq->root, e);
    return true;
}

bool
pq_validate(const struct pqueue *const ppq)
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

enum pq_threeway_cmp
pq_order(const struct pqueue *const ppq)
{
    return ppq->order;
}

/*========================   Static Helpers   ================================*/

static void
init_node(struct pq_elem *e)
{
    e->left_child = e->parent = NULL;
    e->next_sibling = e->prev_sibling = e;
}

static void
clear_node(struct pq_elem *e)
{
    e->left_child = e->next_sibling = e->prev_sibling = e->parent = NULL;
}

static void
cut_child(struct pq_elem *child)
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

static struct pq_elem *delete(struct pqueue *ppq, struct pq_elem *root)
{
    if (ppq->root == root)
    {
        return delete_min(ppq, root);
    }
    cut_child(root);
    return fair_merge(ppq, ppq->root, delete_min(ppq, root));
}

/* Performs a right to left one pass pairing merge in O(lgN) time. A variation
   on the original paper's right to left one pass merge that aims at
   ensuring round robin fairness among duplicate nodes at no extra runtime
   cost. */
static struct pq_elem *
delete_min(struct pqueue *ppq, struct pq_elem *root)
{
    if (!root->left_child)
    {
        return NULL;
    }
    struct pq_elem *const eldest = root->left_child;
    struct pq_elem *cur = eldest->next_sibling;
    struct pq_elem *accumulator = eldest;
    while (cur != eldest && cur->next_sibling != eldest)
    {
        struct pq_elem *next = cur->next_sibling;
        struct pq_elem *next_cur = cur->next_sibling->next_sibling;
        next->next_sibling = next->prev_sibling = NULL;
        cur->next_sibling = cur->prev_sibling = NULL;
        accumulator = fair_merge(ppq, accumulator, fair_merge(ppq, cur, next));
        cur = next_cur;
    }
    /* This covers the odd or even case for number of pairings. */
    root = cur != eldest ? fair_merge(ppq, accumulator, cur) : accumulator;
    /* The root is always alone in its circular list at the end of merges. */
    root->next_sibling = root->prev_sibling = root;
    root->parent = NULL;
    return root;
}

/* Merges nodes ensuring round robin fairness among duplicates. Note the
   parameter names closely. The sibling ring is ordered by oldest as left
   child of parent and newest at back of doubly linked list. Nodes that
   are equal are therefore guaranteed to be popped in round robin order
   if these parameters are respected whenever merging occurs. */
static struct pq_elem *
fair_merge(struct pqueue *const ppq, struct pq_elem *const old,
           struct pq_elem *const new)
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

/* To ensure round robin fairiness and simplify memory access in the pairing
   queue, the oldest sibling shall remain the left child of the parent. Newer
   elements are tacked on to the end of the circular doubly linked list of
   elements. Here is a simple series of adding three arbitrary elements
   to the ring of siblings. Note that the reduced memory access of keeping
   the oldest as left child is only possible due to the doubly linked list
   we use to enable arbitrary erase in the heap. With a singly linked list
   you would have to follow the original paper guidelines and lose the
   ability for fast erase and update:

         a       a       a
        ╱   ->  ╱   ->  ╱
      ┌b┐     ┌b─c┐   ┌b─c─d┐
      └─┘     └───┘   └─────┘

    Then, when we iterate through the list in a delete min operation the
    oldest child/sibling becomes the acumulator first ensuring round robin
    fairness among duplicates. Thus, a one pass merge from left to right
    is acheived that maintains the pairing heap runtime promises. */
static void
link_child(struct pq_elem *const parent, struct pq_elem *const child)
{
    if (parent->left_child)
    {
        child->next_sibling = parent->left_child;
        child->prev_sibling = parent->left_child->prev_sibling;
        parent->left_child->prev_sibling->next_sibling = child;
        parent->left_child->prev_sibling = child;
    }
    else
    {
        parent->left_child = child;
        child->next_sibling = child->prev_sibling = child;
    }
    child->parent = parent;
}

/*========================     Validation    ================================*/

/* NOLINTBEGIN(*misc-no-recursion) */

static size_t
traversal_size(const struct pq_elem *const root)
{
    if (!root)
    {
        return 0;
    }
    size_t sz = 0;
    bool sibling_ring_lapped = false;
    const struct pq_elem *cur = root;
    while (!sibling_ring_lapped)
    {
        sz += 1 + traversal_size(cur->left_child);
        cur = cur->next_sibling;
        sibling_ring_lapped = cur == root;
    }
    return sz;
}

static bool
has_valid_links(const struct pqueue *const ppq,
                const struct pq_elem *const parent,
                const struct pq_elem *const child)
{
    if (!child)
    {
        return true;
    }
    bool sibling_ring_lapped = false;
    const struct pq_elem *cur = child;
    const enum pq_threeway_cmp wrong_order
        = ppq->order == PQLES ? PQGRT : PQLES;
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
