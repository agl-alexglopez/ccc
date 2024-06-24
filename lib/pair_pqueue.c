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
static bool has_valid_links(const struct pair_pqueue *, struct lineage);
static struct ppq_elem *accumulate_pair(struct pair_pqueue *,
                                        struct ppq_elem **, struct ppq_elem *);
static struct ppq_elem *delete(struct pair_pqueue *, struct ppq_elem *);
static struct ppq_elem *delete_min(struct pair_pqueue *, struct ppq_elem *);

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

struct ppq_elem *
ppq_pop(struct pair_pqueue *const ppq)
{
    if (!ppq->root)
    {
        return NULL;
    }
    struct ppq_elem *const popped = ppq->root;
    if (ppq->sz == 1ULL)
    {
        ppq->root = NULL;
        ppq->sz = 0;
        return popped;
    }
    ppq->root = delete_min(ppq, ppq->root);
    ppq->sz--;
    return popped;
}

struct ppq_elem *
ppq_erase(struct pair_pqueue *const ppq, struct ppq_elem *const e)
{
    ppq->root = delete (ppq, e);
    ppq->sz--;
    return e;
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
    ppq->root = delete (ppq, e);
    fn(e, aux);
    init_node(e);
    ppq->root = merge(ppq, ppq->root, e);
    return true;
}

bool
ppq_validate(const struct pair_pqueue *const ppq)
{
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
    e->left_child = NULL;
    e->next_sibling = e;
    e->prev_sibling = e;
    e->parent = NULL;
}

static struct ppq_elem *delete(struct pair_pqueue *ppq, struct ppq_elem *root)
{
    if (ppq->root == root)
    {
        ppq->root = delete_min(ppq, root);
        return root;
    }
    root->next_sibling->prev_sibling = root->prev_sibling;
    root->prev_sibling->next_sibling = root->next_sibling;
    if (root == root->parent->left_child)
    {
        root->parent->left_child = root->prev_sibling;
    }
    root->parent = NULL;
    return merge(ppq, delete_min(ppq, root), ppq->root);
}

static struct ppq_elem *
delete_min(struct pair_pqueue *ppq, struct ppq_elem *root)
{
    struct ppq_elem *const head = root->left_child->next_sibling;
    struct ppq_elem *cur = head->next_sibling;
    struct ppq_elem *accumulator = head;
    while (cur != head && cur->next_sibling != head)
    {
        cur = accumulate_pair(ppq, &accumulator, cur);
    }
    struct ppq_elem *const new_root
        = cur != head ? merge(ppq, cur, accumulator) : accumulator;
    new_root->next_sibling = new_root->prev_sibling = new_root;
    return new_root;
}

static struct ppq_elem *
accumulate_pair(struct pair_pqueue *const ppq,
                struct ppq_elem **const accumulator, struct ppq_elem *a)
{
    struct ppq_elem *b = a->next_sibling;
    struct ppq_elem *next = b->next_sibling;

    b->next_sibling = b->prev_sibling = NULL;
    a->next_sibling = a->prev_sibling = NULL;

    *accumulator = merge(ppq, *accumulator, merge(ppq, a, b));
    return next;
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
        l.node->prev_sibling = l.parent->left_child;
        l.parent->left_child->next_sibling->prev_sibling = l.node;
        l.parent->left_child->next_sibling = l.node;
        l.parent->left_child = l.node;
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
