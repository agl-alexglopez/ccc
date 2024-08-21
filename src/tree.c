/* Author: Alexander G. Lopez
   --------------------------
   This is the internal implementation of a splay tree that
   runs all the data structure interfaces provided by this
   library.

   Citations:
   ---------------------------
   1. This is taken from my own work and research on heap
      allocator performance through various implementations.
      https://github.com/agl-alexglopez/heap-allocator-workshop
      /blob/main/lib/splaytree_topdown.c
   2. However, I based it off of work by Daniel Sleator, Carnegie Mellon
      University. Sleator's implementation of a topdown splay tree was
      instrumental in starting things off, but required extensive modification.
      I had to add the ability to track duplicates, update parent and child
      tracking, and unite the left and right cases for fun. See the .c file
      for my generalizable strategy to eliminate symmetric left and right cases
      for any binary tree code which I have been working on for a while and
      think is quite helpful!
      https://www.link.cs.cmu.edu/link/ftp-site/splaying/top-down-splay.c */
#include "depqueue.h"
#include "impl_tree.h"
#include "set.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Printing enum for printing tree structures if heap available. */
typedef enum ccc_print_link
{
    BRANCH = 0, /* ├── */
    LEAF = 1    /* └── */
} ccc_print_link;

/* Text coloring macros (ANSI character escapes) for printing function
   colorful output. Consider changing to a more portable library like
   ncurses.h. However, I don't want others to install ncurses just to explore
   the project. They already must install gnuplot. Hope this works. */
#define COLOR_BLK "\033[34;1m"
#define COLOR_BLU_BOLD "\033[38;5;12m"
#define COLOR_RED_BOLD "\033[38;5;9m"
#define COLOR_RED "\033[31;1m"
#define COLOR_CYN "\033[36;1m"
#define COLOR_GRN "\033[32;1m"
#define COLOR_NIL "\033[0m"
#define COLOR_ERR COLOR_RED "Error: " COLOR_NIL
#define PRINTER_INDENT (short)13
#define LR 2

ccc_tree_link const inorder_traversal = L;
ccc_tree_link const reverse_inorder_traversal = R;

/* =======================        Prototypes         ====================== */

static void init_node(ccc_tree *, ccc_node *);
static bool empty(ccc_tree const *);
static void multiset_insert(ccc_tree *, ccc_node *);
static ccc_node *find(ccc_tree *, ccc_node const *);
static bool contains(ccc_tree *, ccc_node const *);
static ccc_node *erase(ccc_tree *, ccc_node *);
static bool insert(ccc_tree *, ccc_node *);
static ccc_node *multiset_erase_max_or_min(ccc_tree *, ccc_node *,
                                           ccc_cmp_fn *);
static ccc_node *multiset_erase_node(ccc_tree *, ccc_node *);
static ccc_node *pop_dup_node(ccc_tree *, ccc_node *, ccc_node *);
static ccc_node *pop_front_dup(ccc_tree *, ccc_node *);
static ccc_node *remove_from_tree(ccc_tree *, ccc_node *);
static ccc_node *connect_new_root(ccc_tree *, ccc_node *, ccc_threeway_cmp);
static ccc_node *root(ccc_tree const *);
static ccc_node *max(ccc_tree const *);
static ccc_node *pop_max(ccc_tree *);
static ccc_node *pop_min(ccc_tree *);
static ccc_node *min(ccc_tree const *);
static ccc_node const *const_seek(ccc_tree *, ccc_node const *);
static ccc_node const *next(ccc_tree *, ccc_node const *, ccc_tree_link);
static ccc_node const *multiset_next(ccc_tree *, ccc_node const *,
                                     ccc_tree_link);
static ccc_range equal_range(ccc_tree *, ccc_node const *, ccc_node const *,
                             ccc_tree_link);
static ccc_threeway_cmp force_find_grt(void const *, void const *, void *);
static ccc_threeway_cmp force_find_les(void const *, void const *, void *);
static void link_trees(ccc_tree *, ccc_node *, ccc_tree_link, ccc_node *);
static inline bool has_dups(ccc_node const *, ccc_node const *);
static ccc_node *get_parent(ccc_tree *, ccc_node const *);
static void add_duplicate(ccc_tree *, ccc_node *, ccc_node *, ccc_node *);
static ccc_node *splay(ccc_tree *, ccc_node *, ccc_node const *, ccc_cmp_fn *);
static inline ccc_node const *next_tree_node(ccc_tree *, ccc_node const *,
                                             ccc_tree_link);
static ccc_node *range_begin(ccc_range const *);
static ccc_node *range_end(ccc_range const *);
static ccc_node *rrange_begin(ccc_rrange const *);
static ccc_node *rrange_end(ccc_rrange const *);
static void *struct_base(ccc_tree const *, ccc_node const *);
static ccc_threeway_cmp cmp(ccc_tree const *, ccc_node const *,
                            ccc_node const *, ccc_cmp_fn *);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/* =================  Double Ended Priority Queue Interface  ============== */

void
ccc_depq_clear(ccc_depqueue *pq, ccc_destructor_fn *destructor)
{
    while (!ccc_depq_empty(pq))
    {
        destructor(ccc_depq_pop_max(pq));
    }
}

bool
ccc_depq_empty(ccc_depqueue const *const pq)
{
    return empty(&pq->impl);
}

void *
ccc_depq_root(ccc_depqueue const *const pq)
{
    ccc_node const *const n = root(&pq->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl, n);
}

void *
ccc_depq_max(ccc_depqueue *const pq)
{
    ccc_node const *const n
        = splay(&pq->impl, pq->impl.root, &pq->impl.end, force_find_grt);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl, n);
}

void *
ccc_depq_const_max(ccc_depqueue const *const pq)
{
    ccc_node const *const n = max(&pq->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl, n);
}

bool
ccc_depq_is_max(ccc_depqueue *const pq, ccc_depq_elem const *const e)
{
    return !ccc_depq_rnext(pq, e);
}

void *
ccc_depq_min(ccc_depqueue *const pq)
{
    ccc_node const *const n
        = splay(&pq->impl, pq->impl.root, &pq->impl.end, force_find_les);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl, n);
}

void *
ccc_depq_const_min(ccc_depqueue const *const pq)
{
    ccc_node const *const n = min(&pq->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl, n);
}

bool
ccc_depq_is_min(ccc_depqueue *const pq, ccc_depq_elem const *const e)
{
    return !ccc_depq_next(pq, e);
}

void *
ccc_depq_begin(ccc_depqueue *pq)
{
    ccc_node const *const n = max(&pq->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl, n);
}

void *
ccc_depq_rbegin(ccc_depqueue *pq)
{
    ccc_node const *const n = min(&pq->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl, n);
}

void *
ccc_depq_next(ccc_depqueue *pq, ccc_depq_elem const *e)
{
    ccc_node const *const n
        = multiset_next(&pq->impl, &e->impl, reverse_inorder_traversal);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl, n);
}

void *
ccc_depq_rnext(ccc_depqueue *pq, ccc_depq_elem const *e)
{
    ccc_node const *const n
        = multiset_next(&pq->impl, &e->impl, inorder_traversal);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl, n);
}

ccc_range
ccc_depq_equal_range(ccc_depqueue *pq, ccc_depq_elem const *const begin,
                     ccc_depq_elem const *const end)
{
    return equal_range(&pq->impl, &begin->impl, &end->impl,
                       reverse_inorder_traversal);
}

void *
ccc_depq_begin_range(ccc_range const *const r)
{
    return range_begin(r);
}

void *
ccc_depq_end_range(ccc_range const *const r)
{
    return range_end(r);
}

ccc_rrange
ccc_depq_equal_rrange(ccc_depqueue *pq, ccc_depq_elem const *const rbegin,
                      ccc_depq_elem const *const rend)
{
    ccc_range const ret
        = equal_range(&pq->impl, &rbegin->impl, &rend->impl, inorder_traversal);
    return (ccc_rrange){
        .rbegin = ret.begin,
        .end = ret.end,
    };
}

void *
ccc_depq_begin_rrange(ccc_rrange const *const rr)
{
    return rrange_begin(rr);
}

void *
ccc_depq_end_rrange(ccc_rrange const *const rr)
{
    return rrange_end(rr);
}

void
ccc_depq_push(ccc_depqueue *pq, ccc_depq_elem *const e)
{
    multiset_insert(&pq->impl, &e->impl);
}

void *
ccc_depq_erase(ccc_depqueue *pq, ccc_depq_elem *const e)
{
    ccc_node const *const n = multiset_erase_node(&pq->impl, &e->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl, n);
}

bool
ccc_depq_update(ccc_depqueue *pq, ccc_depq_elem *const elem, ccc_update_fn *fn,
                void *aux)
{
    if (NULL == elem->impl.link[L] || NULL == elem->impl.link[R])
    {
        return false;
    }
    ccc_node *e = multiset_erase_node(&pq->impl, &elem->impl);
    if (!e)
    {
        return false;
    }
    fn(struct_base(&pq->impl, e), aux);
    multiset_insert(&pq->impl, e);
    return true;
}

bool
ccc_depq_contains(ccc_depqueue *const pq, ccc_depq_elem const *const elem)
{
    return contains(&pq->impl, &elem->impl);
}

void *
ccc_depq_pop_max(ccc_depqueue *pq)
{
    ccc_node *n = pop_max(&pq->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl, n);
}

void *
ccc_depq_pop_min(ccc_depqueue *pq)
{
    ccc_node *n = pop_min(&pq->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl, n);
}

size_t
ccc_depq_size(ccc_depqueue const *const pq)
{
    return pq->impl.size;
}

void
ccc_depq_print(ccc_depqueue const *const pq, ccc_depq_elem const *const start,
               ccc_print_fn *const fn)
{
    ccc_tree_print(&pq->impl, &start->impl, fn);
}

bool
ccc_depq_validate(ccc_depqueue const *pq)
{
    return ccc_tree_validate(&pq->impl);
}

/* ======================        ccc_set Interface       ======================
 */

void
ccc_set_clear(ccc_set *const set, ccc_destructor_fn *const destructor)
{

    while (!ccc_set_empty(set))
    {
        destructor(struct_base(&set->impl, pop_min(&set->impl)));
    }
}

bool
ccc_set_empty(ccc_set const *const s)
{
    return empty(&s->impl);
}

size_t
ccc_set_size(ccc_set const *const s)
{
    return s->impl.size;
}

bool
ccc_set_contains(ccc_set *s, ccc_set_elem const *se)
{
    return contains(&s->impl, &se->impl);
}

bool
ccc_set_insert(ccc_set *s, ccc_set_elem *se)
{
    return insert(&s->impl, &se->impl);
}

void *
ccc_set_begin(ccc_set *s)
{
    ccc_node const *const n = min(&s->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&s->impl, n);
}

void *
ccc_set_rbegin(ccc_set *s)
{
    ccc_node const *const n = max(&s->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&s->impl, n);
}

void *
ccc_set_next(ccc_set *s, ccc_set_elem const *e)
{
    ccc_node const *n = next(&s->impl, &e->impl, inorder_traversal);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&s->impl, n);
}

void *
ccc_set_rnext(ccc_set *s, ccc_set_elem const *e)
{
    ccc_node const *n = next(&s->impl, &e->impl, reverse_inorder_traversal);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&s->impl, n);
}

ccc_range
ccc_set_equal_range(ccc_set *s, ccc_set_elem const *const begin,
                    ccc_set_elem const *const end)
{
    return equal_range(&s->impl, &begin->impl, &end->impl, inorder_traversal);
}

void *
ccc_set_begin_range(ccc_range const *const r)
{
    return range_begin(r);
}

void *
ccc_set_end_range(ccc_range const *const r)
{
    return range_end(r);
}

ccc_rrange
ccc_set_equal_rrange(ccc_set *s, ccc_set_elem const *const rbegin,
                     ccc_set_elem const *const end)

{
    ccc_range const r = equal_range(&s->impl, &rbegin->impl, &end->impl,
                                    reverse_inorder_traversal);
    return (ccc_rrange){
        .rbegin = r.begin,
        .end = r.end,
    };
}

void *
ccc_set_begin_rrange(ccc_rrange const *const rr)
{
    return rrange_begin(rr);
}

void *
ccc_set_end_rrange(ccc_rrange const *rr)
{
    return rrange_end(rr);
}

void *
ccc_set_find(ccc_set *s, ccc_set_elem const *se)
{
    ccc_node *n = find(&s->impl, &se->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&s->impl, n);
}

void *
ccc_set_erase(ccc_set *s, ccc_set_elem *se)
{
    ccc_node *n = erase(&s->impl, &se->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&s->impl, n);
}

bool
ccc_set_const_contains(ccc_set *s, ccc_set_elem const *e)
{
    ccc_node const *n = const_seek(&s->impl, &e->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&s->impl, n);
}

bool
ccc_set_is_min(ccc_set *s, ccc_set_elem const *e)
{
    return !ccc_set_rnext(s, e);
}

bool
ccc_set_is_max(ccc_set *s, ccc_set_elem const *e)
{
    return !ccc_set_next(s, e);
}

void *
ccc_set_const_find(ccc_set *s, ccc_set_elem const *e)
{
    ccc_node const *n = const_seek(&s->impl, &e->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&s->impl, n);
}

void *
ccc_set_root(ccc_set const *const s)
{
    ccc_node *n = root(&s->impl);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&s->impl, n);
}

void
ccc_set_print(ccc_set const *const s, ccc_set_elem const *const root,
              ccc_print_fn *const fn)
{
    ccc_tree_print(&s->impl, &root->impl, fn);
}

bool
ccc_set_validate(ccc_set const *const s)
{
    return ccc_tree_validate(&s->impl);
}

/* ===========    Splay Tree Multiset and Set Implementations    ===========

      (40)0x7fffffffd5c8-0x7fffffffdac8(+1)
       ├──(29)R:0x7fffffffd968
       │   ├──(12)R:0x7fffffffd5a8-0x7fffffffdaa8(+1)
       │   │   ├──(2)R:0x7fffffffd548-0x7fffffffda48(+1)
       │   │   │   └──(1)R:0x7fffffffd4e8-0x7fffffffd9e8(+1)
       │   │   └──(9)L:0x7fffffffd668
       │   │       ├──(1)R:0x7fffffffd608
       │   │       └──(7)L:0x7fffffffd7e8
       │   │           ├──(3)R:0x7fffffffd728
       │   │           │   ├──(1)R:0x7fffffffd6c8
       │   │           │   └──(1)L:0x7fffffffd788
       │   │           └──(3)L:0x7fffffffd8a8
       │   │               ├──(1)R:0x7fffffffd848
       │   │               └──(1)L:0x7fffffffd908
       │   └──(16)L:0x7fffffffd568-0x7fffffffda68(+1)
       │       └──(15)R:0x7fffffffd588-0x7fffffffda88(+1)
       │           ├──(2)R:0x7fffffffd528-0x7fffffffda28(+1)
       │           │   └──(1)R:0x7fffffffd4c8-0x7fffffffd9c8(+1)
       │           └──(12)L:0x7fffffffd508-0x7fffffffda08(+1)
       │               └──(11)R:0x7fffffffd828
       │                   ├──(6)R:0x7fffffffd6a8
       │                   │   ├──(2)R:0x7fffffffd5e8
       │                   │   │   └──(1)L:0x7fffffffd648
       │                   │   └──(3)L:0x7fffffffd768
       │                   │       ├──(1)R:0x7fffffffd708
       │                   │       └──(1)L:0x7fffffffd7c8
       │                   └──(4)L:0x7fffffffd8e8
       │                       ├──(1)R:0x7fffffffd888
       │                       └──(2)L:0x7fffffffd4a8-0x7fffffffd9a8(+1)
       │                           └──(1)R:0x7fffffffd948
       └──(10)L:0x7fffffffd688
           ├──(1)R:0x7fffffffd628
           └──(8)L:0x7fffffffd808
               ├──(3)R:0x7fffffffd748
               │   ├──(1)R:0x7fffffffd6e8
               │   └──(1)L:0x7fffffffd7a8
               └──(4)L:0x7fffffffd8c8
                   ├──(1)R:0x7fffffffd868
                   └──(2)L:0x7fffffffd928
                       └──(1)L:0x7fffffffd988

   Pictured above is the heavy/light decomposition of a splay tree.
   The goal of a splay tree is to take advantage of "good" edges
   that drop half the weight of the tree, weight being the number of
   nodes rooted at X. Blue edges are "good" edges so if we
   have a mathematical bound on the cost of those edges, a splay tree
   then amortizes the cost of the red edges, leaving a solid O(lgN) runtime.
   You can't see the color here but check out the printing function.

   All types that use a splay tree are simply wrapper interfaces around
   the core splay tree operations. Splay trees can be used as priority
   queues, sets, and probably much more but we can implement all the
   needed functionality here rather than multiple times for each
   data structure. Through the use of typedefs we only have to write the
   actual code once and then just hand out interfaces as needed.

   =========================================================================
*/

static void
init_node(ccc_tree *t, ccc_node *n)
{
    n->link[L] = &t->end;
    n->link[R] = &t->end;
    n->parent_or_dups = &t->end;
}

static bool
empty(ccc_tree const *const t)
{
    return !t->size;
}

static ccc_node *
root(ccc_tree const *const t)
{
    return t->root;
}

static ccc_node *
max(ccc_tree const *const t)
{
    ccc_node *m = t->root;
    for (; m->link[R] != &t->end; m = m->link[R])
    {}
    return m == &t->end ? NULL : m;
}

static ccc_node *
min(ccc_tree const *t)
{
    ccc_node *m = t->root;
    for (; m->link[L] != &t->end; m = m->link[L])
    {}
    return m == &t->end ? NULL : m;
}

static ccc_node const *
const_seek(ccc_tree *const t, ccc_node const *const n)
{
    ccc_node const *seek = n;
    while (seek != &t->end)
    {
        ccc_threeway_cmp const cur_cmp = cmp(t, n, seek, t->cmp);
        if (cur_cmp == CCC_EQL)
        {
            return seek;
        }
        seek = seek->link[CCC_GRT == cur_cmp];
    }
    return NULL;
}

static ccc_node *
pop_max(ccc_tree *t)
{
    return multiset_erase_max_or_min(t, &t->end, force_find_grt);
}

static ccc_node *
pop_min(ccc_tree *t)
{
    return multiset_erase_max_or_min(t, &t->end, force_find_les);
}

static inline bool
is_dup_head_next(ccc_node const *i)
{
    return i->link[R]->parent_or_dups != NULL;
}

static inline bool
is_dup_head(ccc_node *end, ccc_node const *i)
{
    return i != end && i->link[P] != end && i->link[P]->link[N] == i;
}

static ccc_node const *
multiset_next(ccc_tree *t, ccc_node const *i, ccc_tree_link const traversal)
{
    /* An arbitrary node in a doubly linked list of duplicates. */
    if (NULL == i->parent_or_dups)
    {
        /* We have finished the lap around the duplicate list. */
        if (is_dup_head_next(i))
        {
            return next_tree_node(t, i->link[N], traversal);
        }
        return i->link[N];
    }
    /* The special head node of a doubly linked list of duplicates. */
    if (is_dup_head(&t->end, i))
    {
        /* The duplicate head can be the only node in the list. */
        if (is_dup_head_next(i))
        {
            return next_tree_node(t, i, traversal);
        }
        return i->link[N];
    }
    if (has_dups(&t->end, i))
    {
        return i->parent_or_dups;
    }
    return next(t, i, traversal);
}

static inline ccc_node const *
next_tree_node(ccc_tree *t, ccc_node const *head, ccc_tree_link const traversal)
{
    if (head->parent_or_dups == &t->end)
    {
        return next(t, t->root, traversal);
    }
    ccc_node const *parent = head->parent_or_dups;
    if (parent->link[L] != &t->end && parent->link[L]->parent_or_dups == head)
    {
        return next(t, parent->link[L], traversal);
    }
    if (parent->link[R] != &t->end && parent->link[R]->parent_or_dups == head)
    {
        return next(t, parent->link[R], traversal);
    }
    return NULL;
}

static ccc_node const *
next(ccc_tree *t, ccc_node const *n, ccc_tree_link const traversal)
{
    if (!n || n == &t->end || get_parent(t, t->root) != &t->end)
    {
        return NULL;
    }
    /* Using a helper node simplifies the code greatly. */
    t->end.link[traversal] = t->root;
    t->end.link[!traversal] = &t->end;
    /* The node is a parent, backtracked to, or the end. */
    if (n->link[!traversal] != &t->end)
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = n->link[!traversal]; n->link[traversal] != &t->end;
             n = n->link[traversal])
        {}
        return n;
    }
    /* A leaf. Work our way back up skpping nodes we already visited. */
    ccc_node *p = get_parent(t, n);
    for (; p->link[!traversal] == n; n = p, p = get_parent(t, p))
    {}
    /* This is where the end node is helpful. We get to it eventually. */
    return p == &t->end ? NULL : p;
}

static ccc_range
equal_range(ccc_tree *t, ccc_node const *begin, ccc_node const *end,
            ccc_tree_link const traversal)
{
    /* As with most BST code the cases are perfectly symmetrical. If we
       are seeking an increasing or decreasing range we need to make sure
       we follow the [inclusive, exclusive) range rule. This means double
       checking we don't need to progress to the next greatest or next
       lesser element depending on the direction we are traversing. */
    ccc_threeway_cmp const grt_or_les[2] = {CCC_GRT, CCC_LES};
    ccc_node const *b = splay(t, t->root, begin, t->cmp);
    if (cmp(t, begin, b, t->cmp) == grt_or_les[traversal])
    {
        b = next(t, b, traversal);
    }
    ccc_node const *e = splay(t, t->root, end, t->cmp);
    if (cmp(t, end, e, t->cmp) == grt_or_les[traversal])
    {
        e = next(t, e, traversal);
    }
    return (ccc_range){
        .begin = b ? struct_base(t, b) : NULL,
        .end = e ? struct_base(t, e) : NULL,
    };
}

static ccc_node *
range_begin(ccc_range const *const r)
{
    return r->begin;
}

static ccc_node *
range_end(ccc_range const *const r)
{
    return r->end;
}

static ccc_node *
rrange_begin(ccc_rrange const *const rr)
{
    return rr->rbegin;
}

static ccc_node *
rrange_end(ccc_rrange const *const rr)
{
    return rr->end;
}

static ccc_node *
find(ccc_tree *t, ccc_node const *elem)
{
    t->root = splay(t, t->root, elem, t->cmp);
    return cmp(t, elem, t->root, t->cmp) == CCC_EQL ? t->root : NULL;
}

static bool
contains(ccc_tree *t, ccc_node const *dummy_key)
{
    t->root = splay(t, t->root, dummy_key, t->cmp);
    return cmp(t, dummy_key, t->root, t->cmp) == CCC_EQL;
}

static bool
insert(ccc_tree *t, ccc_node *elem)
{
    init_node(t, elem);
    if (empty(t))
    {
        t->root = elem;
        t->size++;
        return true;
    }
    t->root = splay(t, t->root, elem, t->cmp);
    ccc_threeway_cmp const root_cmp = cmp(t, elem, t->root, t->cmp);
    if (CCC_EQL == root_cmp)
    {
        return false;
    }
    t->size++;
    return connect_new_root(t, elem, root_cmp);
}

static void
multiset_insert(ccc_tree *t, ccc_node *elem)
{
    init_node(t, elem);
    if (empty(t))
    {
        t->root = elem;
        t->size = 1;
        return;
    }
    t->size++;
    t->root = splay(t, t->root, elem, t->cmp);

    ccc_threeway_cmp const root_cmp = cmp(t, elem, t->root, t->cmp);
    if (CCC_EQL == root_cmp)
    {
        add_duplicate(t, t->root, elem, &t->end);
        return;
    }
    (void)connect_new_root(t, elem, root_cmp);
}

static ccc_node *
connect_new_root(ccc_tree *t, ccc_node *new_root, ccc_threeway_cmp cmp_result)
{
    ccc_tree_link const link = CCC_GRT == cmp_result;
    link_trees(t, new_root, link, t->root->link[link]);
    link_trees(t, new_root, !link, t->root);
    t->root->link[link] = &t->end;
    t->root = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link_trees(t, &t->end, 0, t->root);
    return new_root;
}

static void
add_duplicate(ccc_tree *t, ccc_node *tree_node, ccc_node *add, ccc_node *parent)
{
    /* This is a circular doubly linked list with O(1) append to back
       to maintain round robin fairness for any use of this queue.
       the oldest duplicate should be in the tree so we will add new dup
       to the back. The head then needs to point to new tail and new
       tail points to already in place head that tree points to.
       This operation still works if we previously had size 1 list. */
    if (!has_dups(&t->end, tree_node))
    {
        add->parent_or_dups = parent;
        tree_node->parent_or_dups = add;
        add->link[N] = add;
        add->link[P] = add;
        return;
    }
    add->parent_or_dups = NULL;
    ccc_node *list_head = tree_node->parent_or_dups;
    ccc_node *tail = list_head->link[P];
    tail->link[N] = add;
    list_head->link[P] = add;
    add->link[N] = list_head;
    add->link[P] = tail;
}

static ccc_node *
erase(ccc_tree *t, ccc_node *elem)
{
    if (empty(t))
    {
        return NULL;
    }
    ccc_node *ret = splay(t, t->root, elem, t->cmp);
    ccc_threeway_cmp const found = cmp(t, elem, ret, t->cmp);
    if (found != CCC_EQL)
    {
        return NULL;
    }
    ret = remove_from_tree(t, ret);
    ret->link[L] = ret->link[R] = ret->parent_or_dups = NULL;
    t->size--;
    return ret;
}

/* We need to mindful of what the user is asking for. If they want any
   max or min, we have provided a dummy node and dummy compare function
   that will force us to return the max or min. So this operation
   simply grabs the first node available in the tree for round robin.
   This function expects to be passed the t->implil as the node and a
   comparison function that forces either the max or min to be searched. */
static ccc_node *
multiset_erase_max_or_min(ccc_tree *t, ccc_node *tnil,
                          ccc_cmp_fn *force_max_or_min)
{
    if (!t || !tnil || !force_max_or_min)
    {
        return NULL;
    }
    if (empty(t))
    {
        return NULL;
    }
    t->size--;

    ccc_node *ret = splay(t, t->root, tnil, force_max_or_min);
    if (has_dups(&t->end, ret))
    {
        ret = pop_front_dup(t, ret);
    }
    else
    {
        ret = remove_from_tree(t, ret);
    }
    ret->link[L] = ret->link[R] = ret->parent_or_dups = NULL;
    return ret;
}

/* We need to mindful of what the user is asking for. This is a request
   to erase the exact node provided in the argument. So extra care is
   taken to only delete that node, especially if a different node with
   the same size is splayed to the root and we are a duplicate in the
   list. */
static ccc_node *
multiset_erase_node(ccc_tree *t, ccc_node *node)
{
    /* This is what we set removed nodes to so this is a mistaken query */
    if (NULL == node->link[R] || NULL == node->link[L])
    {
        return NULL;
    }
    if (empty(t))
    {
        return NULL;
    }
    t->size--;
    /* Special case that this must be a duplicate that is in the
       linked list but it is not the special head node. So, it
       is a quick snip to get it out. */
    if (NULL == node->parent_or_dups)
    {
        node->link[P]->link[N] = node->link[N];
        node->link[N]->link[P] = node->link[P];
        return node;
    }
    ccc_node *ret = splay(t, t->root, node, t->cmp);
    if (cmp(t, node, ret, t->cmp) != CCC_EQL)
    {
        return NULL;
    }
    if (has_dups(&t->end, ret))
    {
        ret = pop_dup_node(t, node, ret);
    }
    else
    {
        ret = remove_from_tree(t, ret);
    }
    ret->link[L] = ret->link[R] = ret->parent_or_dups = NULL;
    return ret;
}

/* This function assumes that splayed is the new root of the tree */
static ccc_node *
pop_dup_node(ccc_tree *t, ccc_node *dup, ccc_node *splayed)
{
    if (dup == splayed)
    {
        return pop_front_dup(t, splayed);
    }
    /* This is the head of the list of duplicates and no dups left. */
    if (dup->link[N] == dup)
    {
        splayed->parent_or_dups = &t->end;
        return dup;
    }
    /* The dup is the head. There is an arbitrary number of dups after the
       head so replace head. Update the tail at back of the list. Easy to
       forget hard to catch because bugs are often delayed. */
    dup->link[P]->link[N] = dup->link[N];
    dup->link[N]->link[P] = dup->link[P];
    dup->link[N]->parent_or_dups = dup->parent_or_dups;
    splayed->parent_or_dups = dup->link[N];
    return dup;
}

static ccc_node *
pop_front_dup(ccc_tree *t, ccc_node *old)
{
    ccc_node *parent = old->parent_or_dups->parent_or_dups;
    ccc_node *tree_replacement = old->parent_or_dups;
    if (old == t->root)
    {
        t->root = tree_replacement;
    }
    else
    {
        /* Comparing sizes with the root's parent is undefined. */
        parent->link[CCC_GRT == cmp(t, old, parent, t->cmp)] = tree_replacement;
    }

    ccc_node *new_list_head = old->parent_or_dups->link[N];
    ccc_node *list_tail = old->parent_or_dups->link[P];
    bool const circular_list_empty = new_list_head->link[N] == new_list_head;

    new_list_head->link[P] = list_tail;
    new_list_head->parent_or_dups = parent;
    list_tail->link[N] = new_list_head;
    tree_replacement->link[L] = old->link[L];
    tree_replacement->link[R] = old->link[R];
    tree_replacement->parent_or_dups = new_list_head;

    link_trees(t, tree_replacement, L, tree_replacement->link[L]);
    link_trees(t, tree_replacement, R, tree_replacement->link[R]);
    if (circular_list_empty)
    {
        tree_replacement->parent_or_dups = parent;
    }
    return old;
}

static inline ccc_node *
remove_from_tree(ccc_tree *t, ccc_node *ret)
{
    if (ret->link[L] == &t->end)
    {
        t->root = ret->link[R];
        link_trees(t, &t->end, 0, t->root);
    }
    else
    {
        t->root = splay(t, ret->link[L], ret, t->cmp);
        link_trees(t, t->root, R, ret->link[R]);
    }
    return ret;
}

static ccc_node *
splay(ccc_tree *t, ccc_node *root, ccc_node const *elem, ccc_cmp_fn *cmp_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    t->end.link[L] = t->end.link[R] = t->end.parent_or_dups = &t->end;
    ccc_node *l_r_subtrees[LR] = {&t->end, &t->end};
    for (;;)
    {
        ccc_threeway_cmp const root_cmp = cmp(t, elem, root, cmp_fn);
        ccc_tree_link const dir = CCC_GRT == root_cmp;
        if (CCC_EQL == root_cmp || root->link[dir] == &t->end)
        {
            break;
        }
        ccc_threeway_cmp const child_cmp
            = cmp(t, elem, root->link[dir], cmp_fn);
        ccc_tree_link const dir_from_child = CCC_GRT == child_cmp;
        /* A straight line has formed from root->child->elem. An opportunity
           to splay and heal the tree arises. */
        if (CCC_EQL != child_cmp && dir == dir_from_child)
        {
            ccc_node *const pivot = root->link[dir];
            link_trees(t, root, dir, pivot->link[!dir]);
            link_trees(t, pivot, !dir, root);
            root = pivot;
            if (root->link[dir] == &t->end)
            {
                break;
            }
        }
        link_trees(t, l_r_subtrees[!dir], dir, root);
        l_r_subtrees[!dir] = root;
        root = root->link[dir];
    }
    link_trees(t, l_r_subtrees[L], R, root->link[L]);
    link_trees(t, l_r_subtrees[R], L, root->link[R]);
    link_trees(t, root, L, t->end.link[R]);
    link_trees(t, root, R, t->end.link[L]);
    t->root = root;
    link_trees(t, &t->end, 0, t->root);
    return root;
}

/* This function has proven to be VERY important. The nil node often
   has garbage values associated with real nodes in our tree and if we access
   them by mistake it's bad! But the nil is also helpful for some invariant
   coding patters and reducing if checks all over the place. Links a parent
   to a subtree updating the parents child pointer in the direction specified
   and updating the subtree parent field to point back to parent. This last
   step is critical and easy to miss or mess up. */
static inline void
link_trees(ccc_tree *t, ccc_node *parent, ccc_tree_link dir, ccc_node *subtree)
{
    parent->link[dir] = subtree;
    if (has_dups(&t->end, subtree))
    {
        subtree->parent_or_dups->parent_or_dups = parent;
        return;
    }
    subtree->parent_or_dups = parent;
}

/* This is tricky but because of how we store our nodes we always have an
   O(1) check available to us to tell whether a node in a tree is storing
   duplicates without any auxiliary data structures or struct fields.

   All nodes are in the tree tracking their parent. If we add duplicates,
   duplicates form a circular doubly linked list and the tree node
   uses its parent pointer to track the duplicate(s). The duplicate then
   tracks the parent for the tree node. Therefore, we will always know
   how to identify a tree node that stores a duplicate. A tree node with
   a duplicate uses its parent field to point to a node that can
   find itself by checking its doubly linked list. A node in a tree
   could never do this because there is no route back to a node from
   its child pointers by definition of a binary tree. However, we must be
   careful not to access the end helper becuase it can store any pointers
   in its fields that should not be accessed for directions.

                             *────┐
                           ┌─┴─┐  ├──┐
                           *   *──*──*
                          ┌┴┐ ┌┴┐ └──┘
                          * * * *

   Consider the above tree where one node is tracking duplicates. It
   sacrifices its parent field to track a duplicate. The head duplicate
   tracks the parent and uses its left/right fields to track previous/next
   in a circular list. So, we always know via pointers if we find a
   tree node that stores duplicates. By extension this means we can
   also identify if we ARE a duplicate but that check is not part
   of this function. */
static inline bool
has_dups(ccc_node const *const end, ccc_node const *const n)
{
    return n != end && n->parent_or_dups != end
           && n->parent_or_dups->link[L] != end
           && n->parent_or_dups->link[P]->link[N] == n->parent_or_dups;
}

static inline ccc_node *
get_parent(ccc_tree *t, ccc_node const *n)
{
    return has_dups(&t->end, n) ? n->parent_or_dups->parent_or_dups
                                : n->parent_or_dups;
}

static inline void *
struct_base(ccc_tree const *const t, ccc_node const *const n)
{
    /* Link is the first field of the struct and is an array so no need to get
       pointer address of [0] element of array. That's the same as just the
       array field. */
    return ((uint8_t *)n->link) - t->node_elem_offset;
}

static inline ccc_threeway_cmp
cmp(ccc_tree const *const t, ccc_node const *const a, ccc_node const *const b,
    ccc_cmp_fn *const fn)
{
    return fn(struct_base(t, a), struct_base(t, b), t->aux);
}

/* We can trick our splay tree into giving us the max via splaying
   without any input from the user. Our seach evaluates a threeway
   comparison to decide which branch to take in the tree or if we
   found the desired element. Simply force the function to always
   return one or the other and we will end up at the max or min
   NOLINTBEGIN(*swappable-parameters) */
static ccc_threeway_cmp
force_find_grt(void const *const a, void const *const b, void *const aux)
{
    (void)a;
    (void)b;
    (void)aux;
    return CCC_GRT;
}

static ccc_threeway_cmp
force_find_les(void const *const a, void const *const b, void *const aux)
{
    (void)a;
    (void)b;
    (void)aux;
    return CCC_LES;
}

/* NOLINTEND(*swappable-parameters) NOLINTBEGIN(*misc-no-recursion) */

/* ======================        Debugging           ====================== */

/* This section has recursion so it should probably not be used in
   a custom operating system environment with constrained stack space.
   Needless to mention the stdlib.h heap implementation that would need
   to be replaced with the custom OS drop in. */

/* Validate binary tree invariants with ranges. Use a recursive method that
   does not rely upon the implementation of iterators or any other possibly
   buggy implementation. A pure functional range check will provide the most
   reliable check regardless of implementation changes throughout code base. */
struct ccc_tree_range
{
    ccc_node const *low;
    ccc_node const *root;
    ccc_node const *high;
};

struct parent_status
{
    bool correct;
    ccc_node const *parent;
};

static size_t
count_dups(ccc_tree const *const t, ccc_node const *const n)
{
    if (!has_dups(&t->end, n))
    {
        return 0;
    }
    size_t dups = 1;
    for (ccc_node *cur = n->parent_or_dups->link[N]; cur != n->parent_or_dups;
         cur = cur->link[N])
    {
        ++dups;
    }
    return dups;
}

static size_t
recursive_size(ccc_tree const *const t, ccc_node const *const r)
{
    if (r == &t->end)
    {
        return 0;
    }
    size_t s = count_dups(t, r) + 1;
    return s + recursive_size(t, r->link[R]) + recursive_size(t, r->link[L]);
}

static bool
are_subtrees_valid(ccc_tree const *t, struct ccc_tree_range const r,
                   ccc_node const *const nil)
{
    if (!r.root)
    {
        return false;
    }
    if (r.root == nil)
    {
        return true;
    }
    if (r.low != nil && cmp(t, r.root, r.low, t->cmp) != CCC_GRT)
    {
        return false;
    }
    if (r.high != nil && cmp(t, r.root, r.high, t->cmp) != CCC_LES)
    {
        return false;
    }
    return are_subtrees_valid(t,
                              (struct ccc_tree_range){
                                  .low = r.low,
                                  .root = r.root->link[L],
                                  .high = r.root,
                              },
                              nil)
           && are_subtrees_valid(t,
                                 (struct ccc_tree_range){
                                     .low = r.root,
                                     .root = r.root->link[R],
                                     .high = r.high,
                                 },
                                 nil);
}

static struct parent_status
child_tracks_parent(ccc_tree const *const t, ccc_node const *const parent,
                    ccc_node const *const root)
{
    if (has_dups(&t->end, root))
    {
        ccc_node *p = root->parent_or_dups->parent_or_dups;
        if (p != parent)
        {
            return (struct parent_status){false, p};
        }
    }
    else if (root->parent_or_dups != parent)
    {
        ccc_node *p = root->parent_or_dups->parent_or_dups;
        return (struct parent_status){false, p};
    }
    return (struct parent_status){true, parent};
}

static bool
is_duplicate_storing_parent(ccc_tree const *const t,
                            ccc_node const *const parent,
                            ccc_node const *const root)
{
    if (root == &t->end)
    {
        return true;
    }
    if (!child_tracks_parent(t, parent, root).correct)
    {
        return false;
    }
    return is_duplicate_storing_parent(t, root, root->link[L])
           && is_duplicate_storing_parent(t, root, root->link[R]);
}

/* Validate tree prefers to use recursion to examine the tree over the
   provided iterators of any implementation so as to avoid using a
   flawed implementation of such iterators. This should help be more
   sure that the implementation is correct because it follows the
   truth of the provided pointers with its own stack as backtracking
   information. */
bool
ccc_tree_validate(ccc_tree const *const t)
{
    if (!are_subtrees_valid(t,
                            (struct ccc_tree_range){
                                .low = &t->end,
                                .root = t->root,
                                .high = &t->end,
                            },
                            &t->end))
    {
        return false;
    }
    if (!is_duplicate_storing_parent(t, &t->end, t->root))
    {
        return false;
    }
    if (recursive_size(t, t->root) != t->size)
    {
        return false;
    }
    return true;
}

static size_t
get_subtree_size(ccc_node const *const root, void const *const nil)
{
    if (root == nil)
    {
        return 0;
    }
    return 1 + get_subtree_size(root->link[L], nil)
           + get_subtree_size(root->link[R], nil);
}

static char const *
get_edge_color(ccc_node const *const root, size_t const parent_size,
               ccc_node const *const nil)
{
    if (root == nil)
    {
        return "";
    }
    return get_subtree_size(root, nil) <= parent_size / 2 ? COLOR_BLU_BOLD
                                                          : COLOR_RED_BOLD;
}

static void
print_node(ccc_tree const *const t, ccc_node const *const parent,
           ccc_node const *const root, ccc_print_fn *const fn_print)
{
    fn_print(struct_base(t, root));
    struct parent_status stat = child_tracks_parent(t, parent, root);
    if (!stat.correct)
    {
        printf("%s", COLOR_RED);
        fn_print(struct_base(t, stat.parent));
        printf("%s", COLOR_NIL);
    }
    printf(COLOR_CYN);
    /* If a node is a duplicate, we will give it a special mark among nodes. */
    if (has_dups(&t->end, root))
    {
        int duplicates = 1;
        ccc_node const *head = root->parent_or_dups;
        if (head != &t->end)
        {
            fn_print(struct_base(t, head));
            for (ccc_node *i = head->link[N]; i != head;
                 i = i->link[N], ++duplicates)
            {
                fn_print(struct_base(t, i));
            }
        }
        printf("(+%d)", duplicates);
    }
    printf(COLOR_NIL);
    printf("\n");
}

/* I know this function is rough but it's tricky to focus on edge color rather
   than node color. Don't care about pretty code here, need thorough debug.
   I want to convert to iterative stack when I get the chance. */
static void
print_inner_tree(ccc_node const *const root, size_t const parent_size,
                 ccc_node const *const parent, char const *const prefix,
                 char const *const prefix_color, ccc_print_link const node_type,
                 ccc_tree_link const dir, ccc_tree const *const t,
                 ccc_print_fn *const fn_print)
{
    if (root == &t->end)
    {
        return;
    }
    size_t subtree_size = get_subtree_size(root, &t->end);
    printf("%s", prefix);
    printf("%s%s%s",
           subtree_size <= parent_size / 2 ? COLOR_BLU_BOLD : COLOR_RED_BOLD,
           node_type == LEAF ? " └──" : " ├──", COLOR_NIL);
    printf(COLOR_CYN);
    printf("(%zu)", subtree_size);
    dir == L ? printf("L:" COLOR_NIL) : printf("R:" COLOR_NIL);

    print_node(t, parent, root, fn_print);

    char *str = NULL;
    int const string_length
        = snprintf(NULL, 0, "%s%s%s", prefix, prefix_color, /* NOLINT */
                   node_type == LEAF ? "     " : " │   ");
    if (string_length > 0)
    {
        /* NOLINTNEXTLINE */
        str = malloc(string_length + 1);
        /* NOLINTNEXTLINE */
        (void)snprintf(str, string_length, "%s%s%s", prefix, prefix_color,
                       node_type == LEAF ? "     " : " │   ");
    }
    if (str == NULL)
    {
        printf(COLOR_ERR "memory exceeded. Cannot display tree." COLOR_NIL);
        return;
    }

    char const *left_edge_color
        = get_edge_color(root->link[L], subtree_size, &t->end);
    if (root->link[R] == &t->end)
    {
        print_inner_tree(root->link[L], subtree_size, root, str,
                         left_edge_color, LEAF, L, t, fn_print);
    }
    else if (root->link[L] == &t->end)
    {
        print_inner_tree(root->link[R], subtree_size, root, str,
                         left_edge_color, LEAF, R, t, fn_print);
    }
    else
    {
        print_inner_tree(root->link[R], subtree_size, root, str,
                         left_edge_color, BRANCH, R, t, fn_print);
        print_inner_tree(root->link[L], subtree_size, root, str,
                         left_edge_color, LEAF, L, t, fn_print);
    }
    free(str);
}

/* Should be pretty straightforward output. Red node means there
   is an error in parent tracking. The child does not track the parent
   correctly if this occurs and this will cause subtle delayed bugs. */
void
ccc_tree_print(ccc_tree const *const t, ccc_node const *const root,
               ccc_print_fn *const fn_print)
{
    if (root == &t->end)
    {
        return;
    }
    size_t subtree_size = get_subtree_size(root, &t->end);
    printf("\n%s(%zu)%s", COLOR_CYN, subtree_size, COLOR_NIL);
    print_node(t, &t->end, root, fn_print);

    char const *left_edge_color
        = get_edge_color(root->link[L], subtree_size, &t->end);
    if (root->link[R] == &t->end)
    {
        print_inner_tree(root->link[L], subtree_size, root, "", left_edge_color,
                         LEAF, L, t, fn_print);
    }
    else if (root->link[L] == &t->end)
    {
        print_inner_tree(root->link[R], subtree_size, root, "", left_edge_color,
                         LEAF, R, t, fn_print);
    }
    else
    {
        print_inner_tree(root->link[R], subtree_size, root, "", left_edge_color,
                         BRANCH, R, t, fn_print);
        print_inner_tree(root->link[L], subtree_size, root, "", left_edge_color,
                         LEAF, L, t, fn_print);
    }
}

/* NOLINTEND(*misc-no-recursion) */
