#include "pair_pqueue.h"
#include "attrib.h"

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

static struct ppq_elem *merge(struct pair_pqueue *, struct ppq_elem *,
                              struct ppq_elem *);
static void link_child(struct link);
static void init_node(struct ppq_elem *);
static size_t traversal_size(const struct ppq_elem *);
static bool is_strictly_ordered(const struct pair_pqueue *, struct lineage);

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
    ppq->root = merge(ppq, ppq->root, e);
    ++ppq->sz;
}

static struct ppq_elem *
accumulate_pair(struct pair_pqueue *const ppq,
                struct ppq_elem **const accumulator, struct ppq_elem *a)
{
    struct ppq_elem *b = a->next_sibling;
    struct ppq_elem *next = a->next_sibling->next_sibling;

    a->next_sibling->next_sibling = NULL;
    a->next_sibling = NULL;

    *accumulator = merge(ppq, *accumulator, merge(ppq, a, b));
    return next;
}

struct ppq_elem *
ppq_pop(struct pair_pqueue *const ppq)
{
    if (!ppq->root)
    {
        return NULL;
    }
    struct ppq_elem *const popped = ppq->root;
    if (!popped->left_child)
    {
        ppq->root = NULL;
        ppq->sz = 0;
        return popped;
    }
    struct ppq_elem *const head = ppq->root->left_child->next_sibling;
    struct ppq_elem *cur = head->next_sibling;
    struct ppq_elem *accumulator = head;
    while (cur != head && cur->next_sibling != head)
    {
        cur = accumulate_pair(ppq, &accumulator, cur);
    }
    if (cur != head)
    {
        ppq->root = merge(ppq, cur, accumulator);
    }
    else
    {
        ppq->root = accumulator;
    }
    ppq->root->next_sibling = ppq->root;
    ppq->sz--;
    return popped;
}

struct ppq_elem *
ppq_erase(struct pair_pqueue *const ppq, struct ppq_elem *const e)
{
    (void)ppq;
    (void)e;
    UNIMPLEMENTED();
}

void
ppq_clear(struct pair_pqueue *const ppq, ppq_destructor_fn *fn)
{
    (void)ppq;
    (void)fn;
    UNIMPLEMENTED();
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

bool
ppq_update(struct pair_pqueue *const ppq, struct ppq_elem *const e,
           ppq_update_fn *const fn, void *const aux)
{
    (void)ppq;
    (void)e;
    (void)fn;
    (void)aux;
    UNIMPLEMENTED();
}

bool
ppq_validate(const struct pair_pqueue *const ppq)
{
    if (!is_strictly_ordered(ppq, (struct lineage){
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
    e->left_child = NULL;
    e->next_sibling = e;
    e->parent = NULL;
}

static struct ppq_elem *
merge(struct pair_pqueue *const ppq, struct ppq_elem *const a,
      struct ppq_elem *const b)
{
    if (!a || !b || a == b)
    {
        return a ? a : b;
    }
    if (ppq->cmp(a, b, ppq->aux) == ppq->order)
    {
        link_child((struct link){
            .parent = a,
            .node = b,
        });
        return a;
    }
    link_child((struct link){
        .parent = b,
        .node = a,
    });
    return b;
}

static void
link_child(struct link l)
{
    if (l.parent->left_child)
    {
        l.node->next_sibling = l.parent->left_child->next_sibling;
        l.parent->left_child->next_sibling = l.node;
        l.parent->left_child = l.node;
    }
    else
    {
        l.parent->left_child = l.node;
        l.node->next_sibling = l.node;
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
is_strictly_ordered(const struct pair_pqueue *const ppq, const struct lineage l)
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
        if (l.parent && (ppq->cmp(l.parent, cur, ppq->aux) == wrong_order))
        {
            return false;
        }
        if (!is_strictly_ordered(ppq, (struct lineage){
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
