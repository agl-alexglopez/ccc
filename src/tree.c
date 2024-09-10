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
#include "double_ended_priority_queue.h"
#include "impl_tree.h"
#include "ordered_map.h"
#include "types.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Instead of thinking about left and right consider only links
   in the abstract sense. Put them in an array and then flip
   this enum and left and right code paths can be united into one */
typedef enum
{
    L = 0,
    R = 1
} om_link;

/* Trees are just a different interpretation of the same links used
   for doubly linked lists. We take advantage of this for duplicates. */
typedef enum
{
    P = 0,
    N = 1
} om_list_link;

/* Printing enum for printing tree structures if heap available. */
typedef enum
{
    BRANCH = 0, /* ├── */
    LEAF = 1    /* └── */
} om_print_link;

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

om_link const inorder_traversal = R;
om_link const reverse_inorder_traversal = L;

/* =======================        Prototypes         ====================== */

/* No return value. */

static void init_node(struct ccc_om_ *, struct ccc_om_elem_ *);
static void swap(uint8_t tmp[], void *, void *, size_t);
static void multimap_insert(struct ccc_om_ *, struct ccc_om_elem_ *);
static void link_trees(struct ccc_om_ *, struct ccc_om_elem_ *, om_link,
                       struct ccc_om_elem_ *);
static void add_duplicate(struct ccc_om_ *, struct ccc_om_elem_ *,
                          struct ccc_om_elem_ *, struct ccc_om_elem_ *);

/* Boolean returns */

static bool empty(struct ccc_om_ const *);
static bool contains(struct ccc_om_ *, void const *);
static bool has_dups(struct ccc_om_elem_ const *, struct ccc_om_elem_ const *);

/* Returning the user type that is stored in data structure. */

static void *struct_base(struct ccc_om_ const *, struct ccc_om_elem_ const *);
static void *find(struct ccc_om_ *, void const *);
static void *erase(struct ccc_om_ *, void const *key);
static void *alloc_insert(struct ccc_om_ *, struct ccc_om_elem_ *);
static void *multimap_erase_max_or_min(struct ccc_om_ *, ccc_key_cmp_fn *);
static void *multimap_erase_node(struct ccc_om_ *, struct ccc_om_elem_ *);
static void *connect_new_root(struct ccc_om_ *, struct ccc_om_elem_ *,
                              ccc_threeway_cmp);
static void *max(struct ccc_om_ const *);
static void *pop_max(struct ccc_om_ *);
static void *pop_min(struct ccc_om_ *);
static void *min(struct ccc_om_ const *);
static ccc_range equal_range(struct ccc_om_ *, void const *, void const *,
                             om_link);

/* Internal operations that take and return nodes for the tree. */

static struct ccc_om_elem_ *root(struct ccc_om_ const *);
static struct ccc_om_elem_ *
pop_dup_node(struct ccc_om_ *, struct ccc_om_elem_ *, struct ccc_om_elem_ *);
static struct ccc_om_elem_ *
pop_front_dup(struct ccc_om_ *, struct ccc_om_elem_ *, void const *old_key);
static struct ccc_om_elem_ *remove_from_tree(struct ccc_om_ *,
                                             struct ccc_om_elem_ *);
static struct ccc_om_elem_ const *next(struct ccc_om_ const *,
                                       struct ccc_om_elem_ const *, om_link);
static struct ccc_om_elem_ const *
multimap_next(struct ccc_om_ const *, struct ccc_om_elem_ const *, om_link);
static struct ccc_om_elem_ *splay(struct ccc_om_ *, struct ccc_om_elem_ *,
                                  void const *key, ccc_key_cmp_fn *);
static struct ccc_om_elem_ const *
next_tree_node(struct ccc_om_ const *, struct ccc_om_elem_ const *, om_link);
static struct ccc_om_elem_ *get_parent(struct ccc_om_ const *,
                                       struct ccc_om_elem_ const *);

/* Comparison function returns */

static ccc_threeway_cmp force_find_grt(ccc_key_cmp);
static ccc_threeway_cmp force_find_les(ccc_key_cmp);
/* The key comes first. It is the "left hand side" of the comparison. */
static ccc_threeway_cmp cmp(struct ccc_om_ const *, void const *key,
                            struct ccc_om_elem_ const *, ccc_key_cmp_fn *);

/* =================  Double Ended Priority Queue Interface  ============== */

void
ccc_depq_clear(ccc_double_ended_priority_queue *pq,
               ccc_destructor_fn *destructor)
{
    while (!ccc_depq_empty(pq))
    {
        void *popped = pop_min(&pq->impl_);
        if (destructor)
        {
            destructor(popped);
        }
        if (pq->impl_.alloc_)
        {
            pq->impl_.alloc_(popped, 0);
        }
    }
}

bool
ccc_depq_empty(ccc_double_ended_priority_queue const *const pq)
{
    return empty(&pq->impl_);
}

void *
ccc_depq_root(ccc_double_ended_priority_queue const *const pq)
{
    struct ccc_om_elem_ const *const n = root(&pq->impl_);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl_, n);
}

void *
ccc_depq_max(ccc_double_ended_priority_queue *const pq)
{
    struct ccc_om_elem_ const *const n
        = splay(&pq->impl_, pq->impl_.root_, NULL, force_find_grt);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl_, n);
}

bool
ccc_depq_is_max(ccc_double_ended_priority_queue *const pq,
                ccc_depq_elem const *const e)
{
    return !ccc_depq_rnext(pq, e);
}

void *
ccc_depq_min(ccc_double_ended_priority_queue *const pq)
{
    struct ccc_om_elem_ const *const n
        = splay(&pq->impl_, pq->impl_.root_, NULL, force_find_les);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&pq->impl_, n);
}

bool
ccc_depq_is_min(ccc_double_ended_priority_queue *const pq,
                ccc_depq_elem const *const e)
{
    return !ccc_depq_next(pq, e);
}

void *
ccc_depq_begin(ccc_double_ended_priority_queue const *const pq)
{
    return max(&pq->impl_);
}

void *
ccc_depq_rbegin(ccc_double_ended_priority_queue const *const pq)
{
    return min(&pq->impl_);
}

void *
ccc_depq_next(ccc_double_ended_priority_queue const *const pq,
              ccc_depq_elem const *e)
{
    struct ccc_om_elem_ const *const n
        = multimap_next(&pq->impl_, &e->impl_, reverse_inorder_traversal);
    return n == &pq->impl_.end_ ? NULL : struct_base(&pq->impl_, n);
}

void *
ccc_depq_rnext(ccc_double_ended_priority_queue const *const pq,
               ccc_depq_elem const *e)
{
    struct ccc_om_elem_ const *const n
        = multimap_next(&pq->impl_, &e->impl_, inorder_traversal);
    return n == &pq->impl_.end_ ? NULL : struct_base(&pq->impl_, n);
}

void *
ccc_depq_end([[maybe_unused]] ccc_double_ended_priority_queue const *const pq)
{
    return NULL;
}

void *
ccc_depq_rend([[maybe_unused]] ccc_double_ended_priority_queue const *const pq)
{
    return NULL;
}

ccc_range
ccc_depq_equal_range(ccc_double_ended_priority_queue *pq,
                     void const *const begin_key, void const *const end_key)
{
    return equal_range(&pq->impl_, begin_key, end_key,
                       reverse_inorder_traversal);
}

ccc_rrange
ccc_depq_equal_rrange(ccc_double_ended_priority_queue *pq,
                      void const *const rbegin_key, void const *const rend_key)
{
    ccc_range const ret
        = equal_range(&pq->impl_, rbegin_key, rend_key, inorder_traversal);
    return (ccc_rrange){{
        .rbegin_ = ret.impl_.begin_,
        .end_ = ret.impl_.end_,
    }};
}

ccc_result
ccc_depq_push(ccc_double_ended_priority_queue *pq, ccc_depq_elem *const e)
{
    if (pq->impl_.alloc_)
    {
        void *mem = pq->impl_.alloc_(NULL, pq->impl_.elem_sz_);
        if (!mem)
        {
            return CCC_MEM_ERR;
        }
        memcpy(mem, struct_base(&pq->impl_, &e->impl_), pq->impl_.elem_sz_);
    }
    multimap_insert(&pq->impl_, &e->impl_);
    return CCC_OK;
}

void *
ccc_depq_erase(ccc_double_ended_priority_queue *pq, ccc_depq_elem *const e)
{
    return multimap_erase_node(&pq->impl_, &e->impl_);
}

bool
ccc_depq_update(ccc_double_ended_priority_queue *pq, ccc_depq_elem *const elem,
                ccc_update_fn *fn, void *aux)
{
    if (NULL == elem->impl_.link_[L] || NULL == elem->impl_.link_[R])
    {
        return false;
    }
    void *e = multimap_erase_node(&pq->impl_, &elem->impl_);
    if (!e)
    {
        return false;
    }
    fn((ccc_update){e, aux});
    multimap_insert(&pq->impl_, &elem->impl_);
    return true;
}

bool
ccc_depq_contains(ccc_double_ended_priority_queue *const pq,
                  void const *const key)
{
    return contains(&pq->impl_, key);
}

void
ccc_depq_pop_max(ccc_double_ended_priority_queue *pq)
{
    void *n = pop_max(&pq->impl_);
    if (!n)
    {
        return;
    }
    if (pq->impl_.alloc_)
    {
        pq->impl_.alloc_(n, 0);
    }
}

void
ccc_depq_pop_min(ccc_double_ended_priority_queue *pq)
{
    struct ccc_om_elem_ *n = pop_min(&pq->impl_);
    if (!n)
    {
        return;
    }
    if (pq->impl_.alloc_)
    {
        pq->impl_.alloc_(struct_base(&pq->impl_, n), 0);
    }
}

size_t
ccc_depq_size(ccc_double_ended_priority_queue const *const pq)
{
    return pq->impl_.size_;
}

void
ccc_depq_print(ccc_double_ended_priority_queue const *const pq,
               ccc_depq_elem const *const start, ccc_print_fn *const fn)
{
    ccc_tree_print(&pq->impl_, &start->impl_, fn);
}

bool
ccc_depq_validate(ccc_double_ended_priority_queue const *pq)
{
    return ccc_tree_validate(&pq->impl_);
}

/* ======================        Map Interface      ====================== */

void
ccc_om_clear(ccc_ordered_map *const set, ccc_destructor_fn *const destructor)
{

    while (!ccc_om_empty(set))
    {
        void *popped = pop_min(&set->impl_);
        if (destructor)
        {
            destructor(popped);
        }
        if (set->impl_.alloc_)
        {
            set->impl_.alloc_(popped, 0);
        }
    }
}

bool
ccc_om_empty(ccc_ordered_map const *const s)
{
    return empty(&s->impl_);
}

size_t
ccc_om_size(ccc_ordered_map const *const s)
{
    return s->impl_.size_;
}

bool
ccc_om_contains(ccc_ordered_map *s, void const *const key)
{
    return contains(&s->impl_, key);
}

struct ccc_om_entry_
ccc_impl_tree_entry(struct ccc_om_ *t, void const *key)
{
    void *found = find(t, key);
    if (found)
    {
        return (struct ccc_om_entry_){
            .t_ = t,
            .entry_ = {
                .e_ = found,
                .stats_ = CCC_OM_ENTRY_OCCUPIED,
            },
        };
    }
    return (struct ccc_om_entry_){
        .t_ = t,
        .entry_ = {
            .e_ = found,
            .stats_ = CCC_OM_ENTRY_VACANT,
        },
    };
}

ccc_o_map_entry
ccc_om_entry(ccc_ordered_map *const s, void const *const key)
{
    return (ccc_o_map_entry){ccc_impl_tree_entry(&s->impl_, key)};
}

void *
ccc_om_insert_entry(ccc_o_map_entry const *const e, ccc_o_map_elem *const elem)
{
    if (e->impl_.entry_.stats_ == CCC_OM_ENTRY_OCCUPIED)
    {
        elem->impl_
            = *ccc_impl_tree_elem_in_slot(e->impl_.t_, e->impl_.entry_.e_);
        memcpy(e->impl_.entry_.e_, struct_base(e->impl_.t_, &elem->impl_),
               e->impl_.t_->elem_sz_);
        return e->impl_.entry_.e_;
    }
    return alloc_insert(e->impl_.t_, &elem->impl_);
}

void *
ccc_om_or_insert(ccc_o_map_entry const *const e, ccc_o_map_elem *const elem)
{
    if (e->impl_.entry_.stats_ & CCC_OM_ENTRY_OCCUPIED)
    {
        return NULL;
    }
    return alloc_insert(e->impl_.t_, &elem->impl_);
}

ccc_o_map_entry
ccc_om_and_modify(ccc_o_map_entry const *const e, ccc_update_fn *const fn)
{
    if (e->impl_.entry_.stats_ & CCC_OM_ENTRY_OCCUPIED)
    {
        fn((ccc_update){
            .container = (void *const)e->impl_.entry_.e_,
            .aux = NULL,
        });
    }
    return *e;
}

ccc_o_map_entry
ccc_om_and_modify_with(ccc_o_map_entry const *const e, ccc_update_fn *fn,
                       void *aux)
{
    if (e->impl_.entry_.stats_ & CCC_OM_ENTRY_OCCUPIED)
    {
        fn((ccc_update){
            .container = (void *const)e->impl_.entry_.e_,
            .aux = aux,
        });
    }
    return *e;
}

ccc_entry
ccc_om_insert(ccc_ordered_map *const s, ccc_o_map_elem *const out_handle)
{
    void *found = find(
        &s->impl_, ccc_impl_tree_key_from_node(&s->impl_, &out_handle->impl_));
    if (found)
    {
        assert(s->impl_.root_ != &s->impl_.end_);
        *out_handle = *(ccc_o_map_elem *)s->impl_.root_;
        uint8_t tmp[s->impl_.elem_sz_];
        void *user_struct = struct_base(&s->impl_, &out_handle->impl_);
        void *ret = struct_base(&s->impl_, s->impl_.root_);
        swap(tmp, user_struct, ret, s->impl_.elem_sz_);
        return (ccc_entry){{.e_ = ret, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *inserted = alloc_insert(&s->impl_, &out_handle->impl_);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_ERROR}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_om_remove(ccc_ordered_map *const s, ccc_o_map_elem *const out_handle)
{
    void *n = erase(&s->impl_,
                    ccc_impl_tree_key_from_node(&s->impl_, &out_handle->impl_));
    if (!n)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
    }
    if (s->impl_.alloc_)
    {
        void *user_struct = struct_base(&s->impl_, &out_handle->impl_);
        memcpy(user_struct, n, s->impl_.elem_sz_);
        s->impl_.alloc_(n, 0);
        return (ccc_entry){{.e_ = user_struct, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = n, .stats_ = CCC_ENTRY_OCCUPIED}};
}

ccc_entry
ccc_om_remove_entry(ccc_o_map_entry *const e)
{
    if (e->impl_.entry_.stats_ == CCC_OM_ENTRY_OCCUPIED)
    {
        void *erased
            = erase(e->impl_.t_,
                    ccc_impl_tree_key_in_slot(e->impl_.t_, e->impl_.entry_.e_));
        assert(erased);
        if (e->impl_.t_->alloc_)
        {
            e->impl_.t_->alloc_(erased, 0);
            return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_OCCUPIED}};
        }
        return (ccc_entry){{.e_ = erased, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

void *
ccc_om_get_mut(ccc_ordered_map *s, void const *const key)
{
    return (void *)find(&s->impl_, key);
}

void const *
ccc_om_get(ccc_ordered_map *s, void const *const key)
{
    return find(&s->impl_, key);
}

void *
ccc_om_unwrap_mut(ccc_o_map_entry const *const e)
{
    return (void *)ccc_om_unwrap(e);
}

void const *
ccc_om_unwrap(ccc_o_map_entry const *const e)
{
    if (e->impl_.entry_.stats_ == CCC_OM_ENTRY_OCCUPIED)
    {
        return e->impl_.entry_.e_;
    }
    return NULL;
}

void *
ccc_om_begin(ccc_ordered_map const *const s)
{
    return min(&s->impl_);
}

void *
ccc_om_rbegin(ccc_ordered_map const *const s)
{
    return max(&s->impl_);
}

void *
ccc_om_end([[maybe_unused]] ccc_ordered_map const *const s)
{
    return NULL;
}

void *
ccc_om_rend([[maybe_unused]] ccc_ordered_map const *const s)
{
    return NULL;
}

void *
ccc_om_next(ccc_ordered_map const *const s, ccc_o_map_elem const *const e)
{
    struct ccc_om_elem_ const *n
        = next(&s->impl_, &e->impl_, inorder_traversal);
    return n == &s->impl_.end_ ? NULL : struct_base(&s->impl_, n);
}

void *
ccc_om_rnext(ccc_ordered_map const *const s, ccc_o_map_elem const *const e)
{
    struct ccc_om_elem_ const *n
        = next(&s->impl_, &e->impl_, reverse_inorder_traversal);
    return n == &s->impl_.end_ ? NULL : struct_base(&s->impl_, n);
}

ccc_range
ccc_om_equal_range(ccc_ordered_map *s, void const *const begin_key,
                   void const *const end_key)
{
    return equal_range(&s->impl_, begin_key, end_key, inorder_traversal);
}

ccc_rrange
ccc_om_equal_rrange(ccc_ordered_map *s, void const *const rbegin_key,
                    void const *const end_key)

{
    ccc_range const r = equal_range(&s->impl_, rbegin_key, end_key,
                                    reverse_inorder_traversal);
    return (ccc_rrange){{
        .rbegin_ = r.impl_.begin_,
        .end_ = r.impl_.end_,
    }};
}

void *
ccc_om_root(ccc_ordered_map const *const s)
{
    struct ccc_om_elem_ *n = root(&s->impl_);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&s->impl_, n);
}

void
ccc_om_print(ccc_ordered_map const *const s, ccc_o_map_elem const *const root,
             ccc_print_fn *const fn)
{
    ccc_tree_print(&s->impl_, &root->impl_, fn);
}

bool
ccc_om_insert_error(ccc_o_map_entry const *const e)
{
    return e->impl_.entry_.stats_ & CCC_OM_ENTRY_INSERT_ERROR;
}

bool
ccc_om_validate(ccc_ordered_map const *const s)
{
    return ccc_tree_validate(&s->impl_);
}

/* Splay Tree Multimap and Map Implementations
   -------------------------------------------
   The goal of a splay tree is to take advantage of "good" edges
   that drop half the weight of the tree, weight being the number of
   nodes rooted at a node. Blue edges are "good" edges so if we
   have a mathematical bound on the cost of those edges, a splay tree
   then amortizes the cost of the red edges, leaving a solid O(lgN) runtime.

   All types that use a splay tree are simply wrapper interfaces around
   the core splay tree operations. Splay trees can be used as priority
   queues, maps, and probably much more but we can implement all the
   needed functionality here rather than multiple times for each
   data structure.*/

/*==========================  Private Interface  ============================*/

void *
ccc_impl_tree_key_in_slot(struct ccc_om_ const *const t, void const *const slot)
{
    return (uint8_t *)slot + t->key_offset_;
}

void *
ccc_impl_tree_key_from_node(struct ccc_om_ const *const t,
                            struct ccc_om_elem_ const *const n)
{
    return (uint8_t *)struct_base(t, n) + t->key_offset_;
}

struct ccc_om_elem_ *
ccc_impl_tree_elem_in_slot(struct ccc_om_ const *t, void const *slot)
{

    return (struct ccc_om_elem_ *)((uint8_t *)slot + t->node_elem_offset_);
}

/*======================  Static Splay Tree Helpers  ========================*/

static void
init_node(struct ccc_om_ *t, struct ccc_om_elem_ *n)
{
    n->link_[L] = &t->end_;
    n->link_[R] = &t->end_;
    n->parent_or_dups_ = &t->end_;
}

static bool
empty(struct ccc_om_ const *const t)
{
    return !t->size_;
}

static struct ccc_om_elem_ *
root(struct ccc_om_ const *const t)
{
    return t->root_;
}

static void *
max(struct ccc_om_ const *const t)
{
    if (!t->size_)
    {
        return NULL;
    }
    struct ccc_om_elem_ *m = t->root_;
    for (; m->link_[R] != &t->end_; m = m->link_[R])
    {}
    return struct_base(t, m);
}

static void *
min(struct ccc_om_ const *t)
{
    if (!t->size_)
    {
        return NULL;
    }
    struct ccc_om_elem_ *m = t->root_;
    for (; m->link_[L] != &t->end_; m = m->link_[L])
    {}
    return struct_base(t, m);
}

static void *
pop_max(struct ccc_om_ *t)
{
    return multimap_erase_max_or_min(t, force_find_grt);
}

static void *
pop_min(struct ccc_om_ *t)
{
    return multimap_erase_max_or_min(t, force_find_les);
}

static inline bool
is_dup_head_next(struct ccc_om_elem_ const *i)
{
    return i->link_[R]->parent_or_dups_ != NULL;
}

static inline bool
is_dup_head(struct ccc_om_elem_ const *const end, struct ccc_om_elem_ const *i)
{
    return i != end && i->link_[P] != end && i->link_[P]->link_[N] == i;
}

static struct ccc_om_elem_ const *
multimap_next(struct ccc_om_ const *const t, struct ccc_om_elem_ const *i,
              om_link const traversal)
{
    /* An arbitrary node in a doubly linked list of duplicates. */
    if (NULL == i->parent_or_dups_)
    {
        /* We have finished the lap around the duplicate list. */
        if (is_dup_head_next(i))
        {
            return next_tree_node(t, i->link_[N], traversal);
        }
        return i->link_[N];
    }
    /* The special head node of a doubly linked list of duplicates. */
    if (is_dup_head(&t->end_, i))
    {
        /* The duplicate head can be the only node in the list. */
        if (is_dup_head_next(i))
        {
            return next_tree_node(t, i, traversal);
        }
        return i->link_[N];
    }
    if (has_dups(&t->end_, i))
    {
        return i->parent_or_dups_;
    }
    return next(t, i, traversal);
}

static inline struct ccc_om_elem_ const *
next_tree_node(struct ccc_om_ const *const t,
               struct ccc_om_elem_ const *const head, om_link const traversal)
{
    if (head->parent_or_dups_ == &t->end_)
    {
        return next(t, t->root_, traversal);
    }
    struct ccc_om_elem_ const *parent = head->parent_or_dups_;
    if (parent->link_[L] != &t->end_
        && parent->link_[L]->parent_or_dups_ == head)
    {
        return next(t, parent->link_[L], traversal);
    }
    if (parent->link_[R] != &t->end_
        && parent->link_[R]->parent_or_dups_ == head)
    {
        return next(t, parent->link_[R], traversal);
    }
    return &t->end_;
}

static struct ccc_om_elem_ const *
next(struct ccc_om_ const *const t, struct ccc_om_elem_ const *n,
     om_link const traversal)
{
    if (!n || n == &t->end_)
    {
        return NULL;
    }
    assert(get_parent(t, t->root_) == &t->end_);
    /* The node is a parent, backtracked to, or the end. */
    if (n->link_[traversal] != &t->end_)
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = n->link_[traversal]; n->link_[!traversal] != &t->end_;
             n = n->link_[!traversal])
        {}
        return n;
    }
    /* A leaf. Now it is time to visit the closest parent not yet visited.
       The old stack overflow question I read about this type of iteration
       (Boost's method, can't find the post anymore?) had the sentinel node
       make the root its traversal child, but this means we would have to
       write to the sentinel on every call to next. I want multiple threads to
       iterate freely without undefined data race writes to memory locations.
       So more expensive loop.*/
    struct ccc_om_elem_ *p = get_parent(t, n);
    for (; p != &t->end_ && p->link_[!traversal] != n;
         n = p, p = get_parent(t, n))
    {}
    return p;
}

static ccc_range
equal_range(struct ccc_om_ *t, void const *begin_key, void const *end_key,
            om_link const traversal)
{
    /* As with most BST code the cases are perfectly symmetrical. If we
       are seeking an increasing or decreasing range we need to make sure
       we follow the [inclusive, exclusive) range rule. This means double
       checking we don't need to progress to the next greatest or next
       lesser element depending on the direction we are traversing. */
    ccc_threeway_cmp const grt_or_les[2] = {CCC_LES, CCC_GRT};
    struct ccc_om_elem_ const *b = splay(t, t->root_, begin_key, t->cmp_);
    if (cmp(t, begin_key, b, t->cmp_) == grt_or_les[traversal])
    {
        b = next(t, b, traversal);
    }
    struct ccc_om_elem_ const *e = splay(t, t->root_, end_key, t->cmp_);
    if (cmp(t, end_key, e, t->cmp_) == grt_or_les[traversal])
    {
        e = next(t, e, traversal);
    }
    return (ccc_range){{
        .begin_ = b == &t->end_ ? NULL : struct_base(t, b),
        .end_ = e == &t->end_ ? NULL : struct_base(t, e),
    }};
}

static void *
find(struct ccc_om_ *t, void const *const key)
{
    if (t->root_ == &t->end_)
    {
        return NULL;
    }
    t->root_ = splay(t, t->root_, key, t->cmp_);
    return cmp(t, key, t->root_, t->cmp_) == CCC_EQL ? struct_base(t, t->root_)
                                                     : NULL;
}

static bool
contains(struct ccc_om_ *t, void const *key)
{
    t->root_ = splay(t, t->root_, key, t->cmp_);
    return cmp(t, key, t->root_, t->cmp_) == CCC_EQL;
}

static void *
alloc_insert(struct ccc_om_ *t, struct ccc_om_elem_ *out_handle)
{
    init_node(t, out_handle);
    ccc_threeway_cmp root_cmp = CCC_CMP_ERR;
    if (!empty(t))
    {
        void const *const key = ccc_impl_tree_key_from_node(t, out_handle);
        t->root_ = splay(t, t->root_, key, t->cmp_);
        root_cmp = cmp(t, key, t->root_, t->cmp_);
        if (CCC_EQL == root_cmp)
        {
            return NULL;
        }
    }
    if (t->alloc_)
    {
        void *node = t->alloc_(NULL, t->elem_sz_);
        if (!node)
        {
            return NULL;
        }
        memcpy(node, struct_base(t, out_handle), t->elem_sz_);
        out_handle = ccc_impl_tree_elem_in_slot(t, node);
    }
    if (empty(t))
    {
        t->root_ = out_handle;
        t->size_++;
        return struct_base(t, out_handle);
    }
    assert(root_cmp != CCC_CMP_ERR);
    t->size_++;
    return connect_new_root(t, out_handle, root_cmp);
}

void *
ccc_impl_tree_insert(struct ccc_om_ *t, struct ccc_om_elem_ *n)
{
    init_node(t, n);
    if (empty(t))
    {
        t->root_ = n;
        t->size_++;
        return struct_base(t, n);
    }
    void const *const key = ccc_impl_tree_key_from_node(t, n);
    t->root_ = splay(t, t->root_, key, t->cmp_);
    ccc_threeway_cmp const root_cmp = cmp(t, key, t->root_, t->cmp_);
    if (CCC_EQL == root_cmp)
    {
        return NULL;
    }
    t->size_++;
    return connect_new_root(t, n, root_cmp);
}

static void
multimap_insert(struct ccc_om_ *t, struct ccc_om_elem_ *out_handle)
{
    init_node(t, out_handle);
    if (empty(t))
    {
        t->root_ = out_handle;
        t->size_ = 1;
        return;
    }
    t->size_++;
    void const *const key = ccc_impl_tree_key_from_node(t, out_handle);
    t->root_ = splay(t, t->root_, key, t->cmp_);

    ccc_threeway_cmp const root_cmp = cmp(t, key, t->root_, t->cmp_);
    if (CCC_EQL == root_cmp)
    {
        add_duplicate(t, t->root_, out_handle, &t->end_);
        return;
    }
    (void)connect_new_root(t, out_handle, root_cmp);
}

static void *
connect_new_root(struct ccc_om_ *t, struct ccc_om_elem_ *new_root,
                 ccc_threeway_cmp cmp_result)
{
    om_link const link = CCC_GRT == cmp_result;
    link_trees(t, new_root, link, t->root_->link_[link]);
    link_trees(t, new_root, !link, t->root_);
    t->root_->link_[link] = &t->end_;
    t->root_ = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link_trees(t, &t->end_, 0, t->root_);
    return struct_base(t, new_root);
}

static void
add_duplicate(struct ccc_om_ *t, struct ccc_om_elem_ *tree_node,
              struct ccc_om_elem_ *add, struct ccc_om_elem_ *parent)
{
    /* This is a circular doubly linked list with O(1) append to back
       to maintain round robin fairness for any use of this queue.
       the oldest duplicate should be in the tree so we will add new dup
       to the back. The head then needs to point to new tail and new
       tail points to already in place head that tree points to.
       This operation still works if we previously had size 1 list. */
    if (!has_dups(&t->end_, tree_node))
    {
        add->parent_or_dups_ = parent;
        tree_node->parent_or_dups_ = add;
        add->link_[N] = add;
        add->link_[P] = add;
        return;
    }
    add->parent_or_dups_ = NULL;
    struct ccc_om_elem_ *list_head = tree_node->parent_or_dups_;
    struct ccc_om_elem_ *tail = list_head->link_[P];
    tail->link_[N] = add;
    list_head->link_[P] = add;
    add->link_[N] = list_head;
    add->link_[P] = tail;
}

static void *
erase(struct ccc_om_ *t, void const *const key)
{
    if (empty(t))
    {
        return NULL;
    }
    struct ccc_om_elem_ *ret = splay(t, t->root_, key, t->cmp_);
    ccc_threeway_cmp const found = cmp(t, key, ret, t->cmp_);
    if (found != CCC_EQL)
    {
        return NULL;
    }
    ret = remove_from_tree(t, ret);
    ret->link_[L] = ret->link_[R] = ret->parent_or_dups_ = NULL;
    t->size_--;
    return struct_base(t, ret);
}

/* We need to mindful of what the user is asking for. If they want any
   max or min, we have provided a dummy node and dummy compare function
   that will force us to return the max or min. So this operation
   simply grabs the first node available in the tree for round robin.
   This function expects to be passed the t->impl_il as the node and a
   comparison function that forces either the max or min to be searched. */
static void *
multimap_erase_max_or_min(struct ccc_om_ *t, ccc_key_cmp_fn *force_max_or_min)
{
    if (!t || !force_max_or_min)
    {
        return NULL;
    }
    if (empty(t))
    {
        return NULL;
    }
    t->size_--;
    struct ccc_om_elem_ *ret = splay(t, t->root_, NULL, force_max_or_min);
    if (has_dups(&t->end_, ret))
    {
        ret = pop_front_dup(t, ret, ccc_impl_tree_key_from_node(t, ret));
    }
    else
    {
        ret = remove_from_tree(t, ret);
    }
    ret->link_[L] = ret->link_[R] = ret->parent_or_dups_ = NULL;
    return struct_base(t, ret);
}

/* We need to mindful of what the user is asking for. This is a request
   to erase the exact node provided in the argument. So extra care is
   taken to only delete that node, especially if a different node with
   the same size is splayed to the root and we are a duplicate in the
   list. */
static void *
multimap_erase_node(struct ccc_om_ *t, struct ccc_om_elem_ *node)
{
    /* This is what we set removed nodes to so this is a mistaken query */
    if (NULL == node->link_[R] || NULL == node->link_[L])
    {
        return NULL;
    }
    if (empty(t))
    {
        return NULL;
    }
    t->size_--;
    /* Special case that this must be a duplicate that is in the
       linked list but it is not the special head node. So, it
       is a quick snip to get it out. */
    if (NULL == node->parent_or_dups_)
    {
        node->link_[P]->link_[N] = node->link_[N];
        node->link_[N]->link_[P] = node->link_[P];
        return struct_base(t, node);
    }
    void const *const key = ccc_impl_tree_key_from_node(t, node);
    struct ccc_om_elem_ *ret = splay(t, t->root_, key, t->cmp_);
    if (cmp(t, key, ret, t->cmp_) != CCC_EQL)
    {
        return NULL;
    }
    if (has_dups(&t->end_, ret))
    {
        ret = pop_dup_node(t, node, ret);
    }
    else
    {
        ret = remove_from_tree(t, ret);
    }
    ret->link_[L] = ret->link_[R] = ret->parent_or_dups_ = NULL;
    return struct_base(t, ret);
}

/* This function assumes that splayed is the new root of the tree */
static struct ccc_om_elem_ *
pop_dup_node(struct ccc_om_ *t, struct ccc_om_elem_ *dup,
             struct ccc_om_elem_ *splayed)
{
    if (dup == splayed)
    {
        return pop_front_dup(t, splayed,
                             ccc_impl_tree_key_from_node(t, splayed));
    }
    /* This is the head of the list of duplicates and no dups left. */
    if (dup->link_[N] == dup)
    {
        splayed->parent_or_dups_ = &t->end_;
        return dup;
    }
    /* The dup is the head. There is an arbitrary number of dups after the
       head so replace head. Update the tail at back of the list. Easy to
       forget hard to catch because bugs are often delayed. */
    dup->link_[P]->link_[N] = dup->link_[N];
    dup->link_[N]->link_[P] = dup->link_[P];
    dup->link_[N]->parent_or_dups_ = dup->parent_or_dups_;
    splayed->parent_or_dups_ = dup->link_[N];
    return dup;
}

static struct ccc_om_elem_ *
pop_front_dup(struct ccc_om_ *t, struct ccc_om_elem_ *old,
              void const *const old_key)
{
    struct ccc_om_elem_ *parent = old->parent_or_dups_->parent_or_dups_;
    struct ccc_om_elem_ *tree_replacement = old->parent_or_dups_;
    if (old == t->root_)
    {
        t->root_ = tree_replacement;
    }
    else
    {
        /* Comparing sizes with the root's parent is undefined. */
        parent->link_[CCC_GRT == cmp(t, old_key, parent, t->cmp_)]
            = tree_replacement;
    }

    struct ccc_om_elem_ *new_list_head = old->parent_or_dups_->link_[N];
    struct ccc_om_elem_ *list_tail = old->parent_or_dups_->link_[P];
    bool const circular_list_empty = new_list_head->link_[N] == new_list_head;

    new_list_head->link_[P] = list_tail;
    new_list_head->parent_or_dups_ = parent;
    list_tail->link_[N] = new_list_head;
    tree_replacement->link_[L] = old->link_[L];
    tree_replacement->link_[R] = old->link_[R];
    tree_replacement->parent_or_dups_ = new_list_head;

    link_trees(t, tree_replacement, L, tree_replacement->link_[L]);
    link_trees(t, tree_replacement, R, tree_replacement->link_[R]);
    if (circular_list_empty)
    {
        tree_replacement->parent_or_dups_ = parent;
    }
    return old;
}

static inline struct ccc_om_elem_ *
remove_from_tree(struct ccc_om_ *t, struct ccc_om_elem_ *ret)
{
    if (ret->link_[L] == &t->end_)
    {
        t->root_ = ret->link_[R];
        link_trees(t, &t->end_, 0, t->root_);
    }
    else
    {
        t->root_ = splay(t, ret->link_[L], ccc_impl_tree_key_from_node(t, ret),
                         t->cmp_);
        link_trees(t, t->root_, R, ret->link_[R]);
    }
    return ret;
}

static struct ccc_om_elem_ *
splay(struct ccc_om_ *t, struct ccc_om_elem_ *root, void const *const key,
      ccc_key_cmp_fn *cmp_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    t->end_.link_[L] = t->end_.link_[R] = t->end_.parent_or_dups_ = &t->end_;
    struct ccc_om_elem_ *l_r_subtrees[LR] = {&t->end_, &t->end_};
    for (;;)
    {
        ccc_threeway_cmp const root_cmp = cmp(t, key, root, cmp_fn);
        om_link const dir = CCC_GRT == root_cmp;
        if (CCC_EQL == root_cmp || root->link_[dir] == &t->end_)
        {
            break;
        }
        ccc_threeway_cmp const child_cmp
            = cmp(t, key, root->link_[dir], cmp_fn);
        om_link const dir_from_child = CCC_GRT == child_cmp;
        /* A straight line has formed from root->child->elem. An opportunity
           to splay and heal the tree arises. */
        if (CCC_EQL != child_cmp && dir == dir_from_child)
        {
            struct ccc_om_elem_ *const pivot = root->link_[dir];
            link_trees(t, root, dir, pivot->link_[!dir]);
            link_trees(t, pivot, !dir, root);
            root = pivot;
            if (root->link_[dir] == &t->end_)
            {
                break;
            }
        }
        link_trees(t, l_r_subtrees[!dir], dir, root);
        l_r_subtrees[!dir] = root;
        root = root->link_[dir];
    }
    link_trees(t, l_r_subtrees[L], R, root->link_[L]);
    link_trees(t, l_r_subtrees[R], L, root->link_[R]);
    link_trees(t, root, L, t->end_.link_[R]);
    link_trees(t, root, R, t->end_.link_[L]);
    t->root_ = root;
    link_trees(t, &t->end_, 0, t->root_);
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
link_trees(struct ccc_om_ *t, struct ccc_om_elem_ *parent, om_link dir,
           struct ccc_om_elem_ *subtree)
{
    parent->link_[dir] = subtree;
    if (has_dups(&t->end_, subtree))
    {
        subtree->parent_or_dups_->parent_or_dups_ = parent;
        return;
    }
    subtree->parent_or_dups_ = parent;
}

/* This is tricky but because of how we store our nodes we always have an
   O(1) check available to us to tell whether a node in a tree is storing
   duplicates without any auxiliary data structures or struct fields.

   All nodes are in the tree tracking their parent. If we add duplicates,
   duplicates form a circular doubly linked list and the tree node
   uses its parent pointer to track the duplicate(s). The first duplicate then
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
has_dups(struct ccc_om_elem_ const *const end,
         struct ccc_om_elem_ const *const n)
{
    return n != end && n->parent_or_dups_ != end
           && n->parent_or_dups_->link_[L] != end
           && n->parent_or_dups_->link_[P]->link_[N] == n->parent_or_dups_;
}

static inline struct ccc_om_elem_ *
get_parent(struct ccc_om_ const *const t, struct ccc_om_elem_ const *const n)
{
    return has_dups(&t->end_, n) ? n->parent_or_dups_->parent_or_dups_
                                 : n->parent_or_dups_;
}

static inline void *
struct_base(struct ccc_om_ const *const t, struct ccc_om_elem_ const *const n)
{
    /* Link is the first field of the struct and is an array so no need to get
       pointer address of [0] element of array. That's the same as just the
       array field. */
    return ((uint8_t *)n->link_) - t->node_elem_offset_;
}

static inline ccc_threeway_cmp
cmp(struct ccc_om_ const *const t, void const *const key,
    struct ccc_om_elem_ const *const node, ccc_key_cmp_fn *const fn)
{
    return fn((ccc_key_cmp){
        .container = struct_base(t, node),
        .key = key,
        .aux = t->aux_,
    });
}

static inline void
swap(uint8_t tmp[], void *const a, void *const b, size_t elem_sz)
{
    if (a == b)
    {
        return;
    }
    memcpy(tmp, a, elem_sz);
    memcpy(a, b, elem_sz);
    memcpy(b, tmp, elem_sz);
}

/* We can trick our splay tree into giving us the max via splaying
   without any input from the user. Our seach evaluates a threeway
   comparison to decide which branch to take in the tree or if we
   found the desired element. Simply force the function to always
   return one or the other and we will end up at the max or min
   NOLINTBEGIN(*swappable-parameters) */
static ccc_threeway_cmp
force_find_grt(ccc_key_cmp const cmp)
{
    (void)cmp;
    return CCC_GRT;
}

static ccc_threeway_cmp
force_find_les(ccc_key_cmp const cmp)
{
    (void)cmp;
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
    struct ccc_om_elem_ const *low;
    struct ccc_om_elem_ const *root;
    struct ccc_om_elem_ const *high;
};

struct parent_status
{
    bool correct;
    struct ccc_om_elem_ const *parent;
};

static size_t
count_dups(struct ccc_om_ const *const t, struct ccc_om_elem_ const *const n)
{
    if (!has_dups(&t->end_, n))
    {
        return 0;
    }
    size_t dups = 1;
    for (struct ccc_om_elem_ *cur = n->parent_or_dups_->link_[N];
         cur != n->parent_or_dups_; cur = cur->link_[N])
    {
        ++dups;
    }
    return dups;
}

static size_t
recursive_size(struct ccc_om_ const *const t,
               struct ccc_om_elem_ const *const r)
{
    if (r == &t->end_)
    {
        return 0;
    }
    size_t s = count_dups(t, r) + 1;
    return s + recursive_size(t, r->link_[R]) + recursive_size(t, r->link_[L]);
}

static bool
are_subtrees_valid(struct ccc_om_ const *t, struct ccc_tree_range const r,
                   struct ccc_om_elem_ const *const nil)
{
    if (!r.root)
    {
        return false;
    }
    if (r.root == nil)
    {
        return true;
    }
    if (r.low != nil
        && cmp(t, ccc_impl_tree_key_from_node(t, r.low), r.root, t->cmp_)
               != CCC_LES)
    {
        return false;
    }
    if (r.high != nil
        && cmp(t, ccc_impl_tree_key_from_node(t, r.high), r.root, t->cmp_)
               != CCC_GRT)
    {
        return false;
    }
    return are_subtrees_valid(t,
                              (struct ccc_tree_range){
                                  .low = r.low,
                                  .root = r.root->link_[L],
                                  .high = r.root,
                              },
                              nil)
           && are_subtrees_valid(t,
                                 (struct ccc_tree_range){
                                     .low = r.root,
                                     .root = r.root->link_[R],
                                     .high = r.high,
                                 },
                                 nil);
}

static struct parent_status
child_tracks_parent(struct ccc_om_ const *const t,
                    struct ccc_om_elem_ const *const parent,
                    struct ccc_om_elem_ const *const root)
{
    if (has_dups(&t->end_, root))
    {
        struct ccc_om_elem_ *p = root->parent_or_dups_->parent_or_dups_;
        if (p != parent)
        {
            return (struct parent_status){false, p};
        }
    }
    else if (root->parent_or_dups_ != parent)
    {
        struct ccc_om_elem_ *p = root->parent_or_dups_->parent_or_dups_;
        return (struct parent_status){false, p};
    }
    return (struct parent_status){true, parent};
}

static bool
is_duplicate_storing_parent(struct ccc_om_ const *const t,
                            struct ccc_om_elem_ const *const parent,
                            struct ccc_om_elem_ const *const root)
{
    if (root == &t->end_)
    {
        return true;
    }
    if (!child_tracks_parent(t, parent, root).correct)
    {
        return false;
    }
    return is_duplicate_storing_parent(t, root, root->link_[L])
           && is_duplicate_storing_parent(t, root, root->link_[R]);
}

/* Validate tree prefers to use recursion to examine the tree over the
   provided iterators of any implementation so as to avoid using a
   flawed implementation of such iterators. This should help be more
   sure that the implementation is correct because it follows the
   truth of the provided pointers with its own stack as backtracking
   information. */
bool
ccc_tree_validate(struct ccc_om_ const *const t)
{
    if (!are_subtrees_valid(t,
                            (struct ccc_tree_range){
                                .low = &t->end_,
                                .root = t->root_,
                                .high = &t->end_,
                            },
                            &t->end_))
    {
        return false;
    }
    if (!is_duplicate_storing_parent(t, &t->end_, t->root_))
    {
        return false;
    }
    if (recursive_size(t, t->root_) != t->size_)
    {
        return false;
    }
    return true;
}

static size_t
get_subtree_size(struct ccc_om_elem_ const *const root, void const *const nil)
{
    if (root == nil)
    {
        return 0;
    }
    return 1 + get_subtree_size(root->link_[L], nil)
           + get_subtree_size(root->link_[R], nil);
}

static char const *
get_edge_color(struct ccc_om_elem_ const *const root, size_t const parent_size,
               struct ccc_om_elem_ const *const nil)
{
    if (root == nil)
    {
        return "";
    }
    return get_subtree_size(root, nil) <= parent_size / 2 ? COLOR_BLU_BOLD
                                                          : COLOR_RED_BOLD;
}

static void
print_node(struct ccc_om_ const *const t,
           struct ccc_om_elem_ const *const parent,
           struct ccc_om_elem_ const *const root, ccc_print_fn *const fn_print)
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
    if (has_dups(&t->end_, root))
    {
        int duplicates = 1;
        struct ccc_om_elem_ const *head = root->parent_or_dups_;
        if (head != &t->end_)
        {
            fn_print(struct_base(t, head));
            for (struct ccc_om_elem_ *i = head->link_[N]; i != head;
                 i = i->link_[N], ++duplicates)
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
print_inner_tree(struct ccc_om_elem_ const *const root,
                 size_t const parent_size,
                 struct ccc_om_elem_ const *const parent,
                 char const *const prefix, char const *const prefix_color,
                 om_print_link const node_type, om_link const dir,
                 struct ccc_om_ const *const t, ccc_print_fn *const fn_print)
{
    if (root == &t->end_)
    {
        return;
    }
    size_t subtree_size = get_subtree_size(root, &t->end_);
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
        = get_edge_color(root->link_[L], subtree_size, &t->end_);
    if (root->link_[R] == &t->end_)
    {
        print_inner_tree(root->link_[L], subtree_size, root, str,
                         left_edge_color, LEAF, L, t, fn_print);
    }
    else if (root->link_[L] == &t->end_)
    {
        print_inner_tree(root->link_[R], subtree_size, root, str,
                         left_edge_color, LEAF, R, t, fn_print);
    }
    else
    {
        print_inner_tree(root->link_[R], subtree_size, root, str,
                         left_edge_color, BRANCH, R, t, fn_print);
        print_inner_tree(root->link_[L], subtree_size, root, str,
                         left_edge_color, LEAF, L, t, fn_print);
    }
    free(str);
}

/* Should be pretty straightforward output. Red node means there
   is an error in parent tracking. The child does not track the parent
   correctly if this occurs and this will cause subtle delayed bugs. */
void
ccc_tree_print(struct ccc_om_ const *const t,
               struct ccc_om_elem_ const *const root,
               ccc_print_fn *const fn_print)
{
    if (root == &t->end_)
    {
        return;
    }
    size_t subtree_size = get_subtree_size(root, &t->end_);
    printf("\n%s(%zu)%s", COLOR_CYN, subtree_size, COLOR_NIL);
    print_node(t, &t->end_, root, fn_print);

    char const *left_edge_color
        = get_edge_color(root->link_[L], subtree_size, &t->end_);
    if (root->link_[R] == &t->end_)
    {
        print_inner_tree(root->link_[L], subtree_size, root, "",
                         left_edge_color, LEAF, L, t, fn_print);
    }
    else if (root->link_[L] == &t->end_)
    {
        print_inner_tree(root->link_[R], subtree_size, root, "",
                         left_edge_color, LEAF, R, t, fn_print);
    }
    else
    {
        print_inner_tree(root->link_[R], subtree_size, root, "",
                         left_edge_color, BRANCH, R, t, fn_print);
        print_inner_tree(root->link_[L], subtree_size, root, "",
                         left_edge_color, LEAF, L, t, fn_print);
    }
}

/* NOLINTEND(*misc-no-recursion) */
