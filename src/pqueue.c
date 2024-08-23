#include "pqueue.h"
#include "impl_pqueue.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*=========================  Function Prototypes   ==========================*/

static struct ccc_impl_pq_elem *merge(struct ccc_impl_pqueue *,
                                      struct ccc_impl_pq_elem *old,
                                      struct ccc_impl_pq_elem *new);
static void link_child(struct ccc_impl_pq_elem *parent,
                       struct ccc_impl_pq_elem *child);
static void init_node(struct ccc_impl_pq_elem *);
static size_t traversal_size(struct ccc_impl_pq_elem const *);
static bool has_valid_links(struct ccc_impl_pqueue const *,
                            struct ccc_impl_pq_elem const *parent,
                            struct ccc_impl_pq_elem const *child);
static struct ccc_impl_pq_elem *delete(struct ccc_impl_pqueue *,
                                       struct ccc_impl_pq_elem *);
static struct ccc_impl_pq_elem *delete_min(struct ccc_impl_pqueue *,
                                           struct ccc_impl_pq_elem *);
static void clear_node(struct ccc_impl_pq_elem *);
static void cut_child(struct ccc_impl_pq_elem *);
static void *struct_base(struct ccc_impl_pqueue const *,
                         struct ccc_impl_pq_elem const *e);
static ccc_threeway_cmp cmp(struct ccc_impl_pqueue const *,
                            struct ccc_impl_pq_elem const *a,
                            struct ccc_impl_pq_elem const *b);

/*=========================  Interface Functions   ==========================*/

void const *
ccc_pq_front(ccc_pqueue const *const ppq)
{
    return ppq->impl.root ? struct_base(&ppq->impl, ppq->impl.root) : NULL;
}

void
ccc_pq_push(ccc_pqueue *const ppq, ccc_pq_elem *const e)
{
    if (!e || !ppq)
    {
        return;
    }
    init_node(&e->impl);
    ppq->impl.root = merge(&ppq->impl, ppq->impl.root, &e->impl);
    ++ppq->impl.sz;
}

void *
ccc_pq_pop(ccc_pqueue *const ppq)
{
    if (!ppq->impl.root)
    {
        return NULL;
    }
    struct ccc_impl_pq_elem *const popped = ppq->impl.root;
    ppq->impl.root = delete_min(&ppq->impl, ppq->impl.root);
    ppq->impl.sz--;
    clear_node(popped);
    return struct_base(&ppq->impl, popped);
}

void *
ccc_pq_erase(ccc_pqueue *const ppq, ccc_pq_elem *const e)
{
    if (!ppq->impl.root || !e->impl.next_sibling || !e->impl.prev_sibling)
    {
        return NULL;
    }
    ppq->impl.root = delete (&ppq->impl, &e->impl);
    ppq->impl.sz--;
    clear_node(&e->impl);
    return struct_base(&ppq->impl, &e->impl);
}

void
ccc_pq_clear(ccc_pqueue *const ppq, ccc_destructor_fn *fn)
{
    while (!ccc_pq_empty(ppq))
    {
        fn(ccc_pq_pop(ppq));
    }
}

bool
ccc_pq_empty(ccc_pqueue const *const ppq)
{
    return !ppq->impl.sz;
}

size_t
ccc_pq_size(ccc_pqueue const *const ppq)
{
    return ppq->impl.sz;
}

/* This is a difficult function. Without knowing if this new value is greater
   or less than the previous we must always perform a delete and reinsert if
   the value has not broken total order with the parent. It is not sufficient
   to check if the value has exceeded the value of the first left child as
   any sibling of that left child may be bigger than or smaller than that
   left child value. */
bool
ccc_pq_update(ccc_pqueue *const ppq, ccc_pq_elem *const e,
              ccc_update_fn *const fn, void *const aux)
{
    if (!e->impl.next_sibling || !e->impl.prev_sibling)
    {
        return false;
    }
    fn((ccc_update){struct_base(&ppq->impl, &e->impl), aux});
    if (e->impl.parent
        && cmp(&ppq->impl, &e->impl, e->impl.parent) == ppq->impl.order)
    {
        cut_child(&e->impl);
        ppq->impl.root = merge(&ppq->impl, ppq->impl.root, &e->impl);
        return true;
    }
    ppq->impl.root = delete (&ppq->impl, &e->impl);
    init_node(&e->impl);
    ppq->impl.root = merge(&ppq->impl, ppq->impl.root, &e->impl);
    return true;
}

/* Preferable to use this function if it is known the value is increasing.
   Much more efficient. */
bool
ccc_pq_increase(ccc_pqueue *const ppq, ccc_pq_elem *const e, ccc_update_fn *fn,
                void *aux)
{
    if (!e->impl.next_sibling || !e->impl.prev_sibling)
    {
        return false;
    }
    if (ppq->impl.order == CCC_GRT)
    {
        fn((ccc_update){struct_base(&ppq->impl, &e->impl), aux});
        cut_child(&e->impl);
    }
    else
    {
        ppq->impl.root = delete (&ppq->impl, &e->impl);
        fn((ccc_update){struct_base(&ppq->impl, &e->impl), aux});
        init_node(&e->impl);
    }
    ppq->impl.root = merge(&ppq->impl, ppq->impl.root, &e->impl);
    return true;
}

/* Preferable to use this function if it is known the value is decreasing.
   Much more efficient. */
bool
ccc_pq_decrease(ccc_pqueue *const ppq, ccc_pq_elem *const e, ccc_update_fn *fn,
                void *aux)
{
    if (!e->impl.next_sibling || !e->impl.prev_sibling)
    {
        return false;
    }
    if (ppq->impl.order == CCC_LES)
    {
        fn((ccc_update){struct_base(&ppq->impl, &e->impl), aux});
        cut_child(&e->impl);
    }
    else
    {
        ppq->impl.root = delete (&ppq->impl, &e->impl);
        fn((ccc_update){struct_base(&ppq->impl, &e->impl), aux});
        init_node(&e->impl);
    }
    ppq->impl.root = merge(&ppq->impl, ppq->impl.root, &e->impl);
    return true;
}

bool
ccc_pq_validate(ccc_pqueue const *const ppq)
{
    if (ppq->impl.root && ppq->impl.root->parent)
    {
        return false;
    }
    if (!has_valid_links(&ppq->impl, NULL, ppq->impl.root))
    {
        return false;
    }
    if (traversal_size(ppq->impl.root) != ppq->impl.sz)
    {
        return false;
    }
    return true;
}

ccc_threeway_cmp
ccc_pq_order(ccc_pqueue const *const ppq)
{
    return ppq->impl.order;
}

/*========================   Static Helpers   ================================*/

static inline ccc_threeway_cmp
cmp(struct ccc_impl_pqueue const *const ppq,
    struct ccc_impl_pq_elem const *const a,
    struct ccc_impl_pq_elem const *const b)
{
    return ppq->cmp(
        (ccc_cmp){struct_base(ppq, a), struct_base(ppq, b), ppq->aux});
}

static inline void *
struct_base(struct ccc_impl_pqueue const *const pq,
            struct ccc_impl_pq_elem const *e)
{
    return ((uint8_t *)&(e->left_child)) - pq->pq_elem_offset;
}

static inline void
init_node(struct ccc_impl_pq_elem *e)
{
    e->left_child = e->parent = NULL;
    e->next_sibling = e->prev_sibling = e;
}

static inline void
clear_node(struct ccc_impl_pq_elem *e)
{
    e->left_child = e->next_sibling = e->prev_sibling = e->parent = NULL;
}

static inline void
cut_child(struct ccc_impl_pq_elem *child)
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

static inline struct ccc_impl_pq_elem *delete(struct ccc_impl_pqueue *ppq,
                                              struct ccc_impl_pq_elem *root)
{
    if (ppq->root == root)
    {
        return delete_min(ppq, root);
    }
    cut_child(root);
    return merge(ppq, ppq->root, delete_min(ppq, root));
}

static inline struct ccc_impl_pq_elem *
delete_min(struct ccc_impl_pqueue *ppq, struct ccc_impl_pq_elem *root)
{
    if (!root->left_child)
    {
        return NULL;
    }
    struct ccc_impl_pq_elem *const eldest = root->left_child->next_sibling;
    struct ccc_impl_pq_elem *accumulator = root->left_child->next_sibling;
    struct ccc_impl_pq_elem *cur = root->left_child->next_sibling->next_sibling;
    while (cur != eldest && cur->next_sibling != eldest)
    {
        struct ccc_impl_pq_elem *next = cur->next_sibling;
        struct ccc_impl_pq_elem *next_cur = cur->next_sibling->next_sibling;
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

static inline struct ccc_impl_pq_elem *
merge(struct ccc_impl_pqueue *const ppq, struct ccc_impl_pq_elem *const old,
      struct ccc_impl_pq_elem *const new)
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
link_child(struct ccc_impl_pq_elem *const parent,
           struct ccc_impl_pq_elem *const child)
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
traversal_size(struct ccc_impl_pq_elem const *const root)
{
    if (!root)
    {
        return 0;
    }
    size_t sz = 0;
    bool sibling_ring_lapped = false;
    struct ccc_impl_pq_elem const *cur = root;
    while (!sibling_ring_lapped)
    {
        sz += 1 + traversal_size(cur->left_child);
        cur = cur->next_sibling;
        sibling_ring_lapped = cur == root;
    }
    return sz;
}

static bool
has_valid_links(struct ccc_impl_pqueue const *const ppq,
                struct ccc_impl_pq_elem const *const parent,
                struct ccc_impl_pq_elem const *const child)
{
    if (!child)
    {
        return true;
    }
    bool sibling_ring_lapped = false;
    struct ccc_impl_pq_elem const *cur = child;
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
