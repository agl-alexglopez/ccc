#include "pair_pqueue.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

struct link
{
    struct ppq_elem *parent;
    struct ppq_elem *node;
};

struct lineage
{
    const struct ppq_elem *const parent;
    const struct ppq_elem *const child;
};

/*=========================  Function Prototypes   ==========================*/

static struct ppq_elem *fair_merge(struct pair_pqueue *, struct ppq_elem *old,
                                   struct ppq_elem *new);
static void link_child(struct link);
static void init_node(struct ppq_elem *);
static size_t traversal_size(const struct ppq_elem *);
static bool has_valid_links(const struct pair_pqueue *, struct lineage);
static struct ppq_elem *next_pairing(struct pair_pqueue *, struct ppq_elem **,
                                     struct ppq_elem *);
static struct ppq_elem *delete(struct pair_pqueue *, struct ppq_elem *);
static struct ppq_elem *delete_min(struct pair_pqueue *, struct ppq_elem *);
static void clear_node(struct ppq_elem *);
static void cut_child(struct ppq_elem *);

/*=========================  Interface Functions   ==========================*/

void
ppq_init(struct pair_pqueue *const ppq, enum ppq_threeway_cmp ppq_ordering,
         ppq_cmp_fn *const cmp, void *const aux)
{
    ppq->aux = aux;
    ppq->cmp = cmp;
    ppq->order = ppq_ordering;
    ppq->root = NULL;
    ppq->sz = 0;
}

const struct ppq_elem *
ppq_front(const struct pair_pqueue *const ppq)
{
    if (!ppq)
    {
        return NULL;
    }
    return ppq->root;
}

void
ppq_push(struct pair_pqueue *const ppq, struct ppq_elem *const e)
{
    if (!e || !ppq)
    {
        return;
    }
    init_node(e);
    ppq->root = fair_merge(ppq, ppq->root, e);
    ++ppq->sz;
}

struct ppq_elem *
ppq_pop(struct pair_pqueue *const ppq)
{
    if (!ppq->root)
    {
        return NULL;
    }
    struct ppq_elem *const popped = ppq->root;
    ppq->root = delete_min(ppq, ppq->root);
    ppq->sz--;
    clear_node(popped);
    return popped;
}

struct ppq_elem *
ppq_erase(struct pair_pqueue *const ppq, struct ppq_elem *const e)
{
    if (!ppq->root)
    {
        return NULL;
    }
    if (!e->next_sibling || !e->prev_sibling)
    {
        return NULL;
    }
    ppq->root = delete (ppq, e);
    ppq->sz--;
    clear_node(e);
    return e;
}

void
ppq_clear(struct pair_pqueue *const ppq, ppq_destructor_fn *fn)
{
    while (!ppq_empty(ppq))
    {
        fn(ppq_pop(ppq));
    }
}

bool
ppq_empty(const struct pair_pqueue *const ppq)
{
    return !ppq->sz;
}

size_t
ppq_size(const struct pair_pqueue *const ppq)
{
    return ppq->sz;
}

/* This is a difficult function. Without knowing if this new value is greater
   or less than the previous we must always perform a delete and reinsert if
   the value has not broken total order with the parent. It is not sufficient
   to check if the value has exceeded the value of the first left child as
   any sibling of that left child may be bigger than or smaller than that
   newest child value. */
bool
ppq_update(struct pair_pqueue *const ppq, struct ppq_elem *const e,
           ppq_update_fn *const fn, void *const aux)
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
ppq_increase(struct pair_pqueue *const ppq, struct ppq_elem *const e,
             ppq_update_fn *fn, void *aux)
{
    if (!e->next_sibling || !e->prev_sibling)
    {
        return false;
    }
    if (ppq->order == PPQGRT)
    {
        fn(e, aux);
        cut_child(e);
        ppq->root = fair_merge(ppq, ppq->root, e);
    }
    else
    {
        ppq->root = delete (ppq, e);
        fn(e, aux);
        init_node(e);
        ppq->root = fair_merge(ppq, ppq->root, e);
    }
    return true;
}

/* Preferable to use this function if it is known the value is decreasing.
   Much more efficient. */
bool
ppq_decrease(struct pair_pqueue *const ppq, struct ppq_elem *const e,
             ppq_update_fn *fn, void *aux)
{
    if (!e->next_sibling || !e->prev_sibling)
    {
        return false;
    }
    if (ppq->order == PPQLES)
    {
        fn(e, aux);
        cut_child(e);
        ppq->root = fair_merge(ppq, ppq->root, e);
    }
    else
    {
        ppq->root = delete (ppq, e);
        fn(e, aux);
        init_node(e);
        ppq->root = fair_merge(ppq, ppq->root, e);
    }
    return true;
}

bool
ppq_validate(const struct pair_pqueue *const ppq)
{
    if (ppq->root && ppq->root->parent)
    {
        return false;
    }
    if (!has_valid_links(ppq, (struct lineage){
                                  .parent = NULL,
                                  .child = ppq->root,
                              }))
    {
        return false;
    }
    if (traversal_size(ppq->root) != ppq->sz)
    {
        return false;
    }
    return true;
}

enum ppq_threeway_cmp
ppq_order(const struct pair_pqueue *const ppq)
{
    return ppq->order;
}

/*========================   Static Helpers   ================================*/

static void
init_node(struct ppq_elem *e)
{
    e->left_child = e->parent = NULL;
    e->next_sibling = e->prev_sibling = e;
}

static void
clear_node(struct ppq_elem *e)
{
    e->left_child = e->next_sibling = e->prev_sibling = e->parent = NULL;
}

static void
cut_child(struct ppq_elem *child)
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

static struct ppq_elem *delete(struct pair_pqueue *ppq, struct ppq_elem *root)
{
    if (ppq->root == root)
    {
        return delete_min(ppq, root);
    }
    cut_child(root);
    return fair_merge(ppq, ppq->root, delete_min(ppq, root));
}

static struct ppq_elem *
delete_min(struct pair_pqueue *ppq, struct ppq_elem *root)
{
    if (!root->left_child)
    {
        return NULL;
    }
    struct ppq_elem *const eldest = root->left_child;
    struct ppq_elem *cur = eldest->next_sibling;
    struct ppq_elem *accumulator = eldest;
    while (cur != eldest && cur->next_sibling != eldest)
    {
        cur = next_pairing(ppq, &accumulator, cur);
    }
    /* This covers the odd or even case for number of pairings. */
    root = cur != eldest ? fair_merge(ppq, accumulator, cur) : accumulator;
    /* The root is always alone in its circular list at the end of merges. */
    root->next_sibling = root->prev_sibling = root;
    root->parent = NULL;
    return root;
}

/* credit for this way of breaking down accumulation (keneoneth):
   https://github.com/keneoneth/priority-queue-benchmark
   My method required some modifications due to my use of circular
   doubly linked list and desire for round robin fairness. Merges next
   pair into the accumulator and updates accumulator with new root if
   new winning node is found. Returns the node after the next pair. */
static struct ppq_elem *
next_pairing(struct pair_pqueue *const ppq, struct ppq_elem **const accumulator,
             struct ppq_elem *old)
{
    struct ppq_elem *new = old->next_sibling;
    struct ppq_elem *newest = new->next_sibling;

    new->next_sibling = new->prev_sibling = NULL;
    old->next_sibling = old->prev_sibling = NULL;

    *accumulator = fair_merge(ppq, *accumulator, fair_merge(ppq, old, new));
    return newest;
}

/* Merges nodes ensuring round robin fairness among duplicates. Note the
   parameter names closely. The sibling ring is ordered by oldest as left
   child of parent and newest at back of doubly linked list. Nodes that
   are equal are therefore guaranteed to be popped in round robin order
   if these parameters are respected whenever merging occurs. */
static struct ppq_elem *
fair_merge(struct pair_pqueue *const ppq, struct ppq_elem *const old,
           struct ppq_elem *const new)
{
    if (!old || !new || old == new)
    {
        return old ? old : new;
    }
    if (ppq->cmp(new, old, ppq->aux) == ppq->order)
    {
        link_child((struct link){
            .parent = new,
            .node = old,
        });
        return new;
    }
    link_child((struct link){
        .parent = old,
        .node = new,
    });
    return old;
}

/* To ensure round robin fairiness and simplify memory access in the pairing
   queue, the oldest sibling shall remain the left child of the parent. Newer
   elements are tacked on to the end of the circular doubly linked list of
   elements. Here is a simple series of adding three arbitrary elements
   to the ring of siblings. Note that the reduced memory access of keeping
   the oldest as left child is only possible due to the doubly linked list
   we use to enable arbitrary erase in the heap. With singly linked list
   you would have to follow the original paper guidelines and lose the
   ability for fast erase and update:

         a       a       a
        ╱   ->  ╱   ->  ╱
      ┌b┐     ┌b─c┐   ┌b─c─d┐
      └─┘     └───┘   └─────┘

    Then, when we iterate through the list in a delete min operation the
    oldest child/sibling becomes the acumulator first ensuring round robin
    fairness among duplicates. Thus, a one pass merge from left to right
    is acheived that maintains the pairing heap runtime promises.
   */
static void
link_child(struct link l)
{
    if (l.parent->left_child)
    {
        l.node->next_sibling = l.parent->left_child;
        l.node->prev_sibling = l.parent->left_child->prev_sibling;
        l.parent->left_child->prev_sibling->next_sibling = l.node;
        l.parent->left_child->prev_sibling = l.node;
    }
    else
    {
        l.parent->left_child = l.node;
        l.node->next_sibling = l.node->prev_sibling = l.node;
    }
    l.node->parent = l.parent;
}

/*========================     Validation    ================================*/

/* NOLINTBEGIN(*misc-no-recursion) */

static size_t
traversal_size(const struct ppq_elem *const root)
{
    if (!root)
    {
        return 0;
    }
    size_t sz = 0;
    bool sibling_ring_lapped = false;
    const struct ppq_elem *cur = root;
    while (!sibling_ring_lapped)
    {
        sz += 1 + traversal_size(cur->left_child);
        cur = cur->next_sibling;
        sibling_ring_lapped = cur == root;
    }
    return sz;
}

static bool
has_valid_links(const struct pair_pqueue *const ppq, const struct lineage l)
{
    if (!l.child)
    {
        return true;
    }
    bool sibling_ring_lapped = false;
    const struct ppq_elem *cur = l.child;
    const enum ppq_threeway_cmp wrong_order
        = ppq->order == PPQLES ? PPQGRT : PPQLES;
    while (!sibling_ring_lapped)
    {
        if (!cur)
        {
            return false;
        }
        if (l.parent && l.child->parent != l.parent)
        {
            return false;
        }
        if (l.child->next_sibling->prev_sibling != l.child
            || l.child->prev_sibling->next_sibling != l.child)
        {
            return false;
        }
        if (l.parent && (ppq->cmp(l.parent, cur, ppq->aux) == wrong_order))
        {
            return false;
        }
        if (!has_valid_links(ppq, (struct lineage){
                                      .parent = cur,
                                      .child = cur->left_child,
                                  }))
        {
            return false;
        }
        cur = cur->next_sibling;
        sibling_ring_lapped = cur == l.child;
    }
    return true;
}

/* NOLINTEND(*misc-no-recursion) */
